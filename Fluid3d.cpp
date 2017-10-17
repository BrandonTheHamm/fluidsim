#define GL3_PROTOTYPES
#include "gl3.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "pez.h"
#include "bstrlib.h"

#include "Utility.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

using namespace vmath;

typedef enum RendererTypeEnum 
{
    Renderer_OPENGL,
    Renderer_VULKAN,
} RendererTypeEnum;


typedef struct ContextRec 
{
    GLFWwindow *MainWindow;
    RendererTypeEnum RendererType;

    struct {
        float X, Y;
        int Buttons[GLFW_MOUSE_BUTTON_LAST];
    } MouseState;

    struct {
        double StartTime;
        unsigned FrameCount;
        float FramesPerSecond;
    } FrameStats;

    bool WindowWasResized;
    bool StopRequested;
} Context;

static Context global_context = {};

typedef void(*render_init_func)(void);
typedef void(*update_func)(float dtseconds);
typedef void(*render_func)(void);
typedef void(*finalize_frame_func)(void);

typedef struct Renderer
{
    render_init_func    Init;
    render_func         Render;
    update_func         Update;
    finalize_frame_func FinalizeFrame;
} Renderer;


#define OpenGLError GL_NO_ERROR == glGetError(),                        \
        "%s:%d - OpenGL Error - %s", __FILE__, __LINE__, __FUNCTION__   \

static struct {
    SlabPod Velocity;
    SlabPod Density;
    SlabPod Pressure;
    SlabPod Temperature;
} Slabs;

static struct {
    SurfacePod Divergence;
    SurfacePod Obstacles;
    SurfacePod LightCache;
    SurfacePod BlurredDensity;
} Surfaces;

static struct {
    Matrix4 Projection;
    Matrix4 Modelview;
    Matrix4 View;
    Matrix4 ModelviewProjection;
} Matrices;

static struct {
    GLuint CubeCenter;
    GLuint FullscreenQuad;
} Vaos;

static const Point3 EyePosition = Point3(0, 0, 2);
static GLuint RaycastProgram;
static GLuint LightProgram;
static GLuint BlurProgram;
static float FieldOfView = 0.7f;
static bool SimulateFluid = true;
static const float DefaultThetaX = 0;
static const float DefaultThetaY = 0.75f;
static float ThetaX = DefaultThetaX;
static float ThetaY = DefaultThetaY;
static int ViewSamples = GridWidth*2;
static int LightSamples = GridWidth;
static float Fips = -4;

PezConfig PezGetConfig()
{
    PezConfig config;
    config.Title = "Fluid3d";
    config.Width = 853*2;
    config.Height = 480*2;
    config.Multisampling = 0;
    config.VerticalSync = 0;
    return config;
}

void PezInitialize()
{
    PezConfig cfg = PezGetConfig();

    RaycastProgram = LoadProgram("Raycast.VS", "Raycast.GS", "Raycast.FS");
    LightProgram = LoadProgram("Fluid.Vertex", "Fluid.PickLayer", "Light.Cache");
    BlurProgram = LoadProgram("Fluid.Vertex", "Fluid.PickLayer", "Light.Blur");

    glGenVertexArrays(1, &Vaos.CubeCenter);
    glBindVertexArray(Vaos.CubeCenter);
    CreatePointVbo(0, 0, 0);
    glEnableVertexAttribArray(SlotPosition);
    glVertexAttribPointer(SlotPosition, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);

    glGenVertexArrays(1, &Vaos.FullscreenQuad);
    glBindVertexArray(Vaos.FullscreenQuad);
    CreateQuadVbo();
    glEnableVertexAttribArray(SlotPosition);
    glVertexAttribPointer(SlotPosition, 2, GL_SHORT, GL_FALSE, 2 * sizeof(short), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    Slabs.Velocity = CreateSlab(GridWidth, GridHeight, GridDepth, 3);
    Slabs.Density = CreateSlab(GridWidth, GridHeight, GridDepth, 1);
    Slabs.Pressure = CreateSlab(GridWidth, GridHeight, GridDepth, 1);
    Slabs.Temperature = CreateSlab(GridWidth, GridHeight, GridDepth, 1);
    Surfaces.Divergence = CreateVolume(GridWidth, GridHeight, GridDepth, 3);
    Surfaces.LightCache = CreateVolume(GridWidth, GridHeight, GridDepth, 1);
    Surfaces.BlurredDensity = CreateVolume(GridWidth, GridHeight, GridDepth, 1);
    InitSlabOps();
    Surfaces.Obstacles = CreateVolume(GridWidth, GridHeight, GridDepth, 3);
    CreateObstacles(Surfaces.Obstacles);
    ClearSurface(Slabs.Temperature.Ping, AmbientTemperature);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    pezCheck(OpenGLError);
}

void PezRenderOpenGL()
{
    pezCheck(OpenGLError);
    PezConfig cfg = PezGetConfig();

    // Blur and brighten the density map:
    bool BlurAndBrighten = true;
    if (BlurAndBrighten) {
        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, Surfaces.BlurredDensity.FboHandle);
        glViewport(0, 0, Slabs.Density.Ping.Width, Slabs.Density.Ping.Height);
        glBindVertexArray(Vaos.FullscreenQuad);
        glBindTexture(GL_TEXTURE_3D, Slabs.Density.Ping.ColorTexture);
        glUseProgram(BlurProgram);
        SetUniform("DensityScale", 5.0f);
        SetUniform("StepSize", sqrtf(2.0) / float(ViewSamples));
        SetUniform("InverseSize", recipPerElem(Vector3(float(GridWidth), float(GridHeight), float(GridDepth))));
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GridDepth);
    }
    pezCheck(OpenGLError);

    // Generate the light cache:
    bool CacheLights = true;
    if (CacheLights) {
        glDisable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, Surfaces.LightCache.FboHandle);
        glViewport(0, 0, Surfaces.LightCache.Width, Surfaces.LightCache.Height);
        glBindVertexArray(Vaos.FullscreenQuad);
        glBindTexture(GL_TEXTURE_3D, Surfaces.BlurredDensity.ColorTexture);
        glUseProgram(LightProgram);
        SetUniform("LightStep", sqrtf(2.0) / float(LightSamples));
        SetUniform("LightSamples", LightSamples);
        SetUniform("InverseSize", recipPerElem(Vector3(float(GridWidth), float(GridHeight), float(GridDepth))));
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, GridDepth);
    }

    // Perform raycasting:
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, cfg.Width, cfg.Height);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBindVertexArray(Vaos.CubeCenter);
    glActiveTexture(GL_TEXTURE0);
    if (BlurAndBrighten)
        glBindTexture(GL_TEXTURE_3D, Surfaces.BlurredDensity.ColorTexture);
    else
        glBindTexture(GL_TEXTURE_3D, Slabs.Density.Ping.ColorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, Surfaces.LightCache.ColorTexture);
    glUseProgram(RaycastProgram);
    SetUniform("ModelviewProjection", Matrices.ModelviewProjection);
    SetUniform("Modelview", Matrices.Modelview);
    SetUniform("ViewMatrix", Matrices.View);
    SetUniform("ProjectionMatrix", Matrices.Projection);
    SetUniform("ViewSamples", ViewSamples);
    SetUniform("EyePosition", EyePosition);
    SetUniform("Density", 0);
    SetUniform("LightCache", 1);
    SetUniform("RayOrigin", Vector4(transpose(Matrices.Modelview) * EyePosition).getXYZ());
    SetUniform("FocalLength", 1.0f / tanf(FieldOfView / 2.0f));
    SetUniform("WindowSize", float(cfg.Width), float(cfg.Height));
    SetUniform("StepSize", sqrtf(2.0) / float(ViewSamples));
    glDrawArrays(GL_POINTS, 0, 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, 0);
    glActiveTexture(GL_TEXTURE0);

    pezCheck(OpenGLError);
}

void PezUpdate(float seconds)
{
    pezCheck(OpenGLError);
    PezConfig cfg = PezGetConfig();

    float dt = seconds * 0.0001f;
    Vector3 up(1, 0, 0); Point3 target(0);
    Matrices.View = Matrix4::lookAt(EyePosition, target, up);
    Matrix4 modelMatrix = Matrix4::identity();
    modelMatrix *= Matrix4::rotationX(ThetaX);
    modelMatrix *= Matrix4::rotationY(ThetaY);
    Matrices.Modelview = Matrices.View * modelMatrix;
    Matrices.Projection = Matrix4::perspective(
        FieldOfView,
        float(cfg.Width) / cfg.Height, // Aspect Ratio
        0.0f,   // Near Plane
        1.0f);  // Far Plane
    Matrices.ModelviewProjection = Matrices.Projection * Matrices.Modelview;

    float fips = 1.0f / dt;
    float alpha = 0.05f;
    if (Fips < 0) Fips++;
    else if (Fips == 0) Fips = fips;
    else  Fips = fips * alpha + Fips * (1.0f - alpha);

    if (SimulateFluid) {
        glBindVertexArray(Vaos.FullscreenQuad);
        glViewport(0, 0, GridWidth, GridHeight);
        Advect(Slabs.Velocity.Ping, Slabs.Velocity.Ping, Surfaces.Obstacles, Slabs.Velocity.Pong, VelocityDissipation);
        SwapSurfaces(&Slabs.Velocity);
        Advect(Slabs.Velocity.Ping, Slabs.Temperature.Ping, Surfaces.Obstacles, Slabs.Temperature.Pong, TemperatureDissipation);
        SwapSurfaces(&Slabs.Temperature);
        Advect(Slabs.Velocity.Ping, Slabs.Density.Ping, Surfaces.Obstacles, Slabs.Density.Pong, DensityDissipation);
        SwapSurfaces(&Slabs.Density);
        ApplyBuoyancy(Slabs.Velocity.Ping, Slabs.Temperature.Ping, Slabs.Density.Ping, Slabs.Velocity.Pong);
        SwapSurfaces(&Slabs.Velocity);
        ApplyImpulse(Slabs.Temperature.Ping, ImpulsePosition, ImpulseTemperature);
        ApplyImpulse(Slabs.Density.Ping, ImpulsePosition, ImpulseDensity);
        ComputeDivergence(Slabs.Velocity.Ping, Surfaces.Obstacles, Surfaces.Divergence);
        ClearSurface(Slabs.Pressure.Ping, 0);
        for (int i = 0; i < NumJacobiIterations; ++i) {
            Jacobi(Slabs.Pressure.Ping, Surfaces.Divergence, Surfaces.Obstacles, Slabs.Pressure.Pong);
            SwapSurfaces(&Slabs.Pressure);
        }
        SubtractGradient(Slabs.Velocity.Ping, Slabs.Pressure.Ping, Surfaces.Obstacles, Slabs.Velocity.Pong);
        SwapSurfaces(&Slabs.Velocity);
    }
    pezCheck(OpenGLError);
}

void PezHandleMouse(int x, int y, int action)
{
    static bool MouseDown = false;
    static int StartX, StartY;
    static const float Speed = 0.005f;
    if (action == PEZ_DOWN) {
        StartX = x;
        StartY = y;
        MouseDown = true;
    } else if (MouseDown && action == PEZ_MOVE) {
        ThetaX = DefaultThetaX + Speed * (x - StartX);
        ThetaY = DefaultThetaY + Speed * (y - StartY);
    } else if (action == PEZ_UP) {
        MouseDown = false;
    }
}

static void HandleErrorGLFW(int code, const char *errordesc) {
    printf("GLFW error %d : %s\n", code, errordesc);
    assert(false);
    exit(EXIT_FAILURE);
}

static void HandleKeyInputGLFW(GLFWwindow *window, int key, int scancode, int action, int modifierFlags) {
    Context *ctx = &global_context;
    if(action == GLFW_RELEASE) {
        switch(key) {
            case GLFW_KEY_ESCAPE:
                ctx->StopRequested = true;
                break;

            case GLFW_KEY_P:
            case GLFW_KEY_SPACE:
                SimulateFluid = !SimulateFluid;
                break;

            default:
                break;
        }
    }
    // handle key press stuff???
}

static void HandleMouseMoveGLFW(GLFWwindow *window, double px, double py) {
    Context *ctx = &global_context;
    ctx->MouseState.X = (float)px;
    ctx->MouseState.Y = (float)py;
    //printf("Mouse moved to %f x %f\n", px, py);
    PezHandleMouse((int)ctx->MouseState.X, (int)ctx->MouseState.Y, PEZ_MOVE);
}

static void HandleMouseButtonInputGLFW(GLFWwindow *window, int button, int action, int modifiers) {
    Context *ctx = &global_context;
    //printf("Mouse button '%d' is %s at %f x %f\n", button, 
            //((action == GLFW_PRESS) ? "DOWN" : "UP"),
            //ctx->MouseState.X, ctx->MouseState.Y);

    int pezMouseAction = 0;
    if(action == GLFW_PRESS) {
        pezMouseAction = PEZ_DOWN;
    } else if(action == GLFW_RELEASE) {
        pezMouseAction = PEZ_UP;
    }

    PezHandleMouse((int)ctx->MouseState.X, (int)ctx->MouseState.Y, pezMouseAction);
}

static void HandleWindowResizeGLFW(GLFWwindow *window, int width, int height) {
    Context *ctx = &global_context;
    printf("Main window resized to %d x %d\n", width, height);
    ctx->WindowWasResized = true;
}


static void HandleCursorEnterGLFW(GLFWwindow *window, int entered) {
    Context *ctx = &global_context;
    if(entered == GLFW_TRUE) {
        printf("Mouse ENTERED!\n");
    } else {
        // release mouse
        printf("Mouse EXITED!\n");
    }
}

static void InitRendererOpenGL(void) {
    Context *ctx = &global_context;

    // make the context "current"
    glfwMakeContextCurrent(ctx->MainWindow);
    // clear any opengl error state
    glGetError();

    // Init shader library
    bstring name = bfromcstr(PezGetConfig().Title);
    pezSwInit("");

    pezSwAddPath("./", ".glsl");
    pezSwAddPath("../", ".glsl");

    char qualified_path[128];
    strcpy(qualified_path, pezResourcePath());
    strcpy(qualified_path, "/");
    pezSwAddPath(qualified_path, ".glsl");
    pezSwAddDirective("*", "#version 400");

    pezPrintString("OpenGL Version:  %s\n", glGetString(GL_VERSION));

    PezInitialize();

    glfwSwapInterval(0);
}


static void FinalizeFrameOpenGL(void) {
    Context *ctx = &global_context;
    glfwSwapBuffers(ctx->MainWindow);
}


int main(int argc, char *argv[]) {
    glfwSetErrorCallback(HandleErrorGLFW);

    Context *ctx = &global_context;

    if(!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return(EXIT_FAILURE);
    }

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *vidmode = glfwGetVideoMode(monitor);
    printf("Current video mode : %dx%d @ %d Hz\n", vidmode->width, vidmode->height, vidmode->refreshRate);

    if(ctx->RendererType == Renderer_VULKAN) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    } else if(ctx->RendererType == Renderer_OPENGL) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);

        // Nothing in this code prevents using a core profile, but note that the shaders require a 4.0+ context
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);

        if(PezGetConfig().Multisampling) {
            glfwWindowHint(GLFW_SAMPLES, 4);
        }
    } else {
        printf("ERROR - Unknown renderer type : %d\n", ctx->RendererType);
        assert(false);
        return(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);

    do {
        ctx->MainWindow = glfwCreateWindow(PezGetConfig().Width, PezGetConfig().Height, "vkFluid", NULL, NULL);
        if(!ctx->MainWindow) {
            printf("Failed to create GLFW window!\n");
            return(EXIT_FAILURE);
        }

        glfwSetKeyCallback(ctx->MainWindow, HandleKeyInputGLFW);

        glfwSetCursorEnterCallback(ctx->MainWindow, HandleCursorEnterGLFW);
        glfwSetMouseButtonCallback(ctx->MainWindow, HandleMouseButtonInputGLFW);
        glfwSetCursorPosCallback(ctx->MainWindow, HandleMouseMoveGLFW);

        glfwSetWindowSizeCallback(ctx->MainWindow, HandleWindowResizeGLFW);

        Renderer renderer = {};
        if(ctx->RendererType == Renderer_OPENGL) {
            renderer.Init = InitRendererOpenGL;
            renderer.Update = PezUpdate;
            renderer.Render = PezRenderOpenGL;
            renderer.FinalizeFrame = FinalizeFrameOpenGL;
        } else if (ctx->RendererType == Renderer_VULKAN) {
            // TODO(BH): Vulkan renderer stuff...
            //renderer.Init = InitRendererVulkan;
            //renderer.Update = UpdateVulkan;
            //renderer.Render = RenderVulkan;
            //renderer.FinalizeFrame = FinalizeFrameVulkan;
            printf("ERROR: Vulkan renderer not yet implemented!\n");
            assert(false);
        } else {
            printf("ERROR: Unknown renderer type '%d'\n", ctx->RendererType);
            assert(false);
        }

        renderer.Init();

        double PrevTime = glfwGetTime();
        ctx->FrameStats.StartTime = PrevTime;
        ctx->StopRequested = false;
        while(!ctx->StopRequested && !glfwWindowShouldClose(ctx->MainWindow)) {
            glfwPollEvents();

            double CurrentTime = glfwGetTime();
            double DeltaTime = CurrentTime - PrevTime;
            PrevTime = CurrentTime;

            renderer.Update((float)DeltaTime);

            renderer.Render();
            renderer.FinalizeFrame();

            if(++ctx->FrameStats.FrameCount > 100) {
                ctx->FrameStats.FramesPerSecond = (float)ctx->FrameStats.FrameCount / (CurrentTime - ctx->FrameStats.StartTime);
                ctx->FrameStats.FrameCount = 0;
                ctx->FrameStats.StartTime = CurrentTime;
                printf("FPS last 100 frames: %0.3f\n", ctx->FrameStats.FramesPerSecond);
            }
        }

        pezSwShutdown();
        glfwTerminate();

    } while(false);

    return(EXIT_SUCCESS);
}

