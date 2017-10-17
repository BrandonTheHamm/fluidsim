cmake_minimum_required (VERSION 3.7)

if(POLICY CMP0015)
   cmake_policy(SET CMP0015 NEW)
endif(POLICY CMP0015)

project (vkfluid)
set(PROGRAM_NAME vkfluid)

set(THIRDPARTY_DIR "${CMAKE_SOURCE_DIR}/external")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
include(BHutils)

OPTION(VKFLUID_FULL_COMPILER_WARNINGS "Set to ON to enable full/verbose compiler warnings" OFF)

#FIXME(BH): Disable in optimized builds
ADD_DEFINITIONS(-DVKFLUID_LOGGING_ENABLED)

#FIXME(BH): Only for debug builds
ADD_DEFINITIONS(-DVKFLUID_WANT_ASSERTIONS)
ADD_DEFINITIONS(-DVKFLUID_DEBUG)


message(STATUS "GLFW3_DIR = ${GLFW3_DIR}")
OPTION(GLFW3_USE_STATIC "Set to ON to use GLFW static libs" OFF)
#SET(GLFW3_USE_STATIC YES)
find_package(GLFW3 REQUIRED)
IF(GLFW3_USE_STATIC)
	ADD_DEFINITIONS(-DGLFW3_USE_STATIC)
ENDIF()

message(STATUS "GLFW3_INCLUDE_DIR = '${GLFW3_INCLUDE_DIR}'")
INCLUDE_DIRECTORIES(${GLFW3_INCLUDE_DIR})
list(APPEND Invaders_LIBRARIES "${GLFW3_LIBRARY}")

file(GLOB VKFLUID_HEADERS *.h *.inl *.hpp *.hxx *.hh)
file(GLOB VKFLUID_SOURCES *.cpp *.c *.cc *.cxx)

IF(WIN32)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
  ADD_DEFINITIONS(-D_CRT_SECURE_NO_DEPRECATE)
  ADD_DEFINITIONS(-D_WINDOWS)
  ADD_DEFINITIONS(-D_WIN32)
  ADD_DEFINITIONS(-DGLFW_EXPOSE_NATIVE_WIN32)
  ADD_DEFINITIONS(/wd4710)
  ADD_DEFINITIONS(/wd4191)

  # OPTION(MSVC_USE_STATIC_RUNTIME "Use the static C runtime library" OFF)
  # if(MSVC_USE_STATIC_RUNTIME)
  #      # Force static runtime libraries
  #     FOREACH(flag
  #             CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
  #             CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_DEBUG_INIT
  #             CMAKE_CXX_FLAGS_RELEASE  CMAKE_CXX_FLAGS_RELWITHDEBINFO
  #             CMAKE_CXX_FLAGS_DEBUG  CMAKE_CXX_FLAGS_DEBUG_INIT)
  #         STRING(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
  #         SET("${flag}" "${${flag}}")
  #     ENDFOREACH()
  # endif()

  # TODO(BH): Disable RTTI

  OPTION(MSVC_ENABLE_CXX_EXCEPTIONS "Enable C++ exception handling" OFF)
  if(NOT MSVC_ENABLE_CXX_EXCEPTIONS)
      FOREACH(flag
              CMAKE_CXX_FLAGS
              CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
              CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_DEBUG_INIT
              CMAKE_CXX_FLAGS_RELEASE  CMAKE_CXX_FLAGS_RELWITHDEBINFO
              CMAKE_CXX_FLAGS_DEBUG  CMAKE_CXX_FLAGS_DEBUG_INIT)
          STRING(REPLACE "/EHsc"  " " "${flag}" "${${flag}}")
          SET("${flag}" "${${flag}}")
      ENDFOREACH()
  endif()

  if(CMAKE_CXX_FLAGS MATCHES "/EHsc ")
    string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    message(STATUS "CMAKE_CXX_FLAGS matches /EHsc before end of string -- replaced...")
    message(STATUS "")
  endif()

  if(CMAKE_CXX_FLAGS MATCHES "/EHsc$")
    string(REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    message(STATUS "CMAKE_CXX_FLAGS matches /EHsc at end of string -- replaced...")
    message(STATUS "")
  endif()

  message(STATUS "CMAKE_CXX_FLAGS after possible REPLACE operation:")
  message(STATUS "CMAKE_CXX_FLAGS='${CMAKE_CXX_FLAGS}'")
  message(STATUS "") 

  # SET(EXECUTABLE_FLAGS WIN32)
  SET(PLATFORM_LIBRARIES shell32.lib user32.lib imm32.lib gdi32.lib winmm.lib opengl32.lib mincore.lib version.lib)
elseif(UNIX AND NOT APPLE)
  SET(EXECUTABLE_FLAGS "")
  SET(PLATFORM_LIBRARIES "")
  ADD_DEFINITIONS(-Wno-missing-field-initializers)
  ADD_DEFINITIONS(-Wno-missing-braces)

  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
  
  # Debug flags
  if(VKFLUID_FULL_COMPILER_WARNINGS)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall")
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wextra -Wundef")
    endif(CMAKE_COMPILER_IS_GNUCXX)
  endif()

ENDIF()

find_package(OpenGL REQUIRED)
IF(${OPENGL_FOUND} MATCHES "TRUE")
    INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
    message(STATUS "OPENGL_INCLUDE_DIR = ${OPENGL_INCLUDE_DIR}")
ENDIF()

# Use FindVulkan module added with CMAKE 3.7
# if (NOT CMAKE_VERSION VERSION_LESS 3.7.0)
message(STATUS "Using module to find Vulkan")
find_package(Vulkan REQUIRED)

IF (NOT Vulkan_FOUND)
	message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
	message(STATUS ${Vulkan_LIBRARY})
	include_directories(${Vulkan_INCLUDE_DIR})
ENDIF()

# include_directories($ENV{VULKAN_SDK}/include)
IF(WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSEIF(UNIX AND NOT WIN32 AND NOT APPLE)
  find_package(XCB REQUIRED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_XCB_KHR")
ENDIF()

# Set preprocessor defines
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNOMINMAX -D_USE_MATH_DEFINES")

# Clang specific stuff
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch-enum")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


add_executable(${PROGRAM_NAME} ${EXECUTABLE_FLAGS}
    ${VKFLUID_SOURCES}
    ${VKFLUID_HEADERS}
)

target_link_libraries(${PROGRAM_NAME}
	${GLFW3_LIBRARY}
  	${OPENGL_LIBRARY}
	${Vulkan_LIBRARY}
)

