CC=g++
CFLAGS=-Wall -c -O3 -DVKFLUID_LINUX
LIBS=-lX11 -lGL -lpng -lGLEW -lglfw

MAINCPP=Fluid3d.o Utility.o
CSHARED=pez.o pez.linux.o bstrlib.o
SHADERS=Fluid.glsl Raycast.glsl Light.glsl

run: fluid
	./fluid

fluid: $(MAINCPP) $(CSHARED) $(SHADERS)
	$(CC) $(MAINCPP) $(CSHARED) -o fluid $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o fluid
