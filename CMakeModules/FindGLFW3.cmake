# Locate GLFW3 library
# This module defines
#  GLFW3_FOUND, if false, do not try to link to Lua 
#  GLFW3_LIBRARIES
#  GLFW3_INCLUDE_DIR, where to find glfw3.h

# Set GLFW3_USE_STATIC=YES to link against the static version of GLFW

FIND_PATH(GLFW3_INCLUDE_DIR GLFW/glfw3.h
  HINTS
	  $ENV{GLFW3_DIR}
	  ${GLFW3_DIR}
  PATH_SUFFIXES 
    include
    # include/GLFW 
    # include/GLFW3 
    # include/glfw3 
    # include/glfw 
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw # Fink
    /opt/local # DarwinPorts
    /opt/csw # Blastwave
    /opt
)

FIND_LIBRARY(GLFW3_LIBRARY_RELEASE
  NAMES libglfw.so glfw.dll glfw GLFW glfw3 GLFW3
  HINTS
	 $ENV{GLFW3_DIR}
	 ${GLFW3_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
)

FIND_LIBRARY(GLFW3_LIBRARY_DEBUG
  NAMES libglfwd.so glfwd.dll glfwd GLFWd glfw3d GLFW3d 
  HINTS
	 $ENV{GLFW3_DIR}
	 ${GLFW3_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
)

FIND_LIBRARY(GLFW3_STATIC_LIBRARY_RELEASE
  NAMES libglfw3.a glfw3s.lib glfw3.lib glfw3 glfw3s glfws glfw
  HINTS
	 $ENV{GLFW3_DIR}
	 ${GLFW3_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
)

FIND_LIBRARY(GLFW3_STATIC_LIBRARY_DEBUG
  NAMES libglfw3d.a glfw3sd.lib glfw3d.lib glfw3d glfw3sd glfwsd glfwd
  HINTS
	 $ENV{GLFW3_DIR}
	 ${GLFW3_DIR}
  PATH_SUFFIXES lib64 lib
  PATHS
    ~/Library/Frameworks
    /Library/Frameworks
    /usr/local
    /usr
    /sw
    /opt/local
    /opt/csw
    /opt
)


set(GLFW3_FOUND FALSE)
if(GLFW3_INCLUDE_DIR)
    if(GLFW3_USE_STATIC)
        if(GLFW3_STATIC_LIBRARY_RELEASE AND GLFW3_STATIC_LIBRARY_DEBUG)
            SET(GLFW3_FOUND TRUE)
            SET(GLFW3_LIBRARY debug ${GLFW3_STATIC_LIBRARY_DEBUG} optimized ${GLFW3_STATIC_LIBRARY_RELEASE})
        elseif(GLFW3_STATIC_LIBRARY_RELEASE)
            SET(GLFW3_FOUND TRUE)
            SET(GLFW3_LIBRARY ${GLFW3_STATIC_LIBRARY_RELEASE})
        endif()
    else()#!GLFW3_USE_STATIC
        if(GLFW3_LIBRARY_RELEASE AND GLFW3_LIBRARY_DEBUG)
            SET(GLFW3_FOUND TRUE)
            SET(GLFW3_LIBRARY debug ${GLFW3_LIBRARY_DEBUG} optimized ${GLFW3_LIBRARY_RELEASE})
        elseif(GLFW3_LIBRARY_RELEASE)
            SET(GLFW3_FOUND TRUE)
            SET(GLFW3_LIBRARY ${GLFW3_LIBRARY_RELEASE})
        endif()
    endif()#GLFW3_USE_STATIC
endif()#GLFW3_INCLUDE_DIR


#INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set GLFW3_FOUND to TRUE if 
# all listed variables are TRUE
#FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLFW3  DEFAULT_MSG  GLFW3_LIBRARY GLFW3_INCLUDE_DIR)

# MARK_AS_ADVANCED(GLFW3_INCLUDE_DIR GLFW3_LIBRARY)
