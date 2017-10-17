# Locate GLFW3 library
# This module defines
#  GLEW_FOUND, if false, do not try to link to Lua 
#  GLEW_LIBRARIES
#  GLEW_INCLUDE_DIR, where to find glfw3.h
# Set GLEW_USE_STATIC=YES if you want to link against the static glew libraries


FIND_PATH(GLEW_INCLUDE_DIR GL/glew.h
  HINTS
	  $ENV{GLEW_DIR}
	  ${GLEW_DIR}
  PATH_SUFFIXES 
    include
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

FIND_LIBRARY(GLEW_LIBRARY_RELEASE
  NAMES glew32 GLEW
  HINTS
	 $ENV{GLEW_DIR}
	 ${GLEW_DIR}
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

FIND_LIBRARY(GLEW_LIBRARY_DEBUG
  NAMES glew32d GLEWd
  HINTS
	 $ENV{GLEW_DIR}
	 ${GLEW_DIR}
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

FIND_LIBRARY(GLEW_STATIC_LIBRARY_RELEASE
  NAMES glew32s GLEWs
  HINTS
	 $ENV{GLEW_DIR}
	 ${GLEW_DIR}
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

FIND_LIBRARY(GLEW_STATIC_LIBRARY_DEBUG
  NAMES glew32sd GLEWsd
  HINTS
	 $ENV{GLEW_DIR}
	 ${GLEW_DIR}
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


SET(GLEW_FOUND FALSE)
if(GLEW_INCLUDE_DIR)
    if(GLEW_USE_STATIC)
        MESSAGE(STATUS "GLEW - using STATIC libs")
        if(GLEW_STATIC_LIBRARY_RELEASE AND GLEW_STATIC_LIBRARY_DEBUG)
            MESSAGE(STATUS "GLEW - using BOTH STATIC libs")
            SET(GLEW_FOUND TRUE)
            SET(GLEW_LIBRARY debug ${GLEW_STATIC_LIBRARY_DEBUG} optimized ${GLEW_STATIC_LIBRARY_RELEASE})
            ADD_DEFINITIONS(-DGLEW_STATIC)
        elseif(GLEW_STATIC_LIBRARY_RELEASE)
            MESSAGE(STATUS "GLEW - using ONLY release STATIC libs")
            SET(GLEW_FOUND TRUE)
            ADD_DEFINITIONS(-DGLEW_STATIC)
            SET(GLEW_LIBRARY ${GLEW_STATIC_LIBRARY_RELEASE})
        endif()
    else()#!GLEW_USE_STATIC
        MESSAGE(STATUS "GLEW - NOT using STATIC libs")
        if(GLEW_LIBRARY_RELEASE AND GLEW_LIBRARY_DEBUG)
            SET(GLEW_FOUND TRUE)
            SET(GLEW_LIBRARY debug ${GLEW_LIBRARY_DEBUG} optimized ${GLEW_LIBRARY_RELEASE})
        elseif(GLEW_LIBRARY_RELEASE)
            SET(GLEW_FOUND TRUE)
            SET(GLEW_LIBRARY ${GLEW_LIBRARY_RELEASE})
        endif()
    endif()#GLEW_USE_STATIC
endif()#GLEW_INCLUDE_DIR

#INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set GLEW_FOUND to TRUE if 
# all listed variables are TRUE
#FIND_PACKAGE_HANDLE_STANDARD_ARGS(GLEW  DEFAULT_MSG  GLEW_LIBRARY GLEW_INCLUDE_DIR)

# MARK_AS_ADVANCED(GLEW_INCLUDE_DIR GLEW_LIBRARY)
