#WARNING: All non-linux commands need checking

project ( LGeneral-Deluxe )

cmake_minimum_required ( VERSION 2.8 )
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" )

option ( DEV_BUILD "Development Build. Disable this for release builds" ON )
option ( BUILD_PACKAGE "Prepares build for creation of a package with CPack" ON )
option ( ENABLE_WARNING "Always show warnings (even for release builds)" OFF )
option ( FATAL_WARNING "Treat warnings as errors" OFF )

find_package ( SDL2 COMPONENTS mixer image )

if ( NOT SDL_FOUND )
  message ( FATAL_ERROR "Can't find SDL which is required" )
else ()
  include_directories ( ${SDL_INCLUDE_DIR} )
  message ( "found SDL ${SDL_MAJOR}.${SDL_MINOR}.${SDL_MICRO} (${SDL_LIBRARY_DIRS}:${SDL_INCLUDE_DIR})" )
endif ()

if ( NOT SDLIMAGE_FOUND )
  message ( FATAL_ERROR "Can't find SDL_IMAGE which is required" )
else ()
  include_directories ( ${SDLIMAGE_INCLUDE_DIR} )
  message ( "found SDL_image ${SDL_IMAGE_MAJOR}.${SDL_IMAGE_MINOR}.${SDL_IMAGE_MICRO} (${SDL_IMAGE_LIBRARY_DIRS}:${SDLIMAGE_INCLUDE_DIR})" )
endif ()

if ( NOT SDLMIXER_FOUND )
  message ( WARNING "Can't find SDL_mixer" )
else ()
  include_directories ( ${SDLMIXER_INCLUDE_DIR} )
  add_definitions ( -DWITH_SOUND=1 )
  set ( WITH_SOUND 1 )
  message ( "found SDL_mixer ${SDL_MIXER_MAJOR}.${SDL_MIXER_MINOR}.${SDL_MIXER_MICRO} (${SDL_MIXER_LIBRARY_DIRS}:${SDLMIXER_INCLUDE_DIR})" )
endif ()

# Read version number
set ( CPACK_PACKAGE_VERSION_MAJOR "1" )
set ( CPACK_PACKAGE_VERSION_MINOR "2" )
set ( CPACK_PACKAGE_VERSION_PATCH "4" )
find_package ( Git )
if ( GIT_FOUND )
  message("git found: ${GIT_EXECUTABLE}")
# Get the latest abbreviated commit hash of the working branch
  execute_process(
    COMMAND ${GIT_EXECUTABLE} log -1 --format=%ad --date=iso
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
endif()

if ( DEV_BUILD )
  # Append the commit to version number
  set ( CPACK_PACKAGE_VERSION_PATCH "${GIT_COMMIT_HASH}" )
else ()
  set ( CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_NSIS_PACKAGE_NAME}" )
endif ()
set ( CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}" )
add_definitions( -DVERSION="${CPACK_PACKAGE_VERSION}" )

if ( BUILD_PACKAGE )
  if ( NOT DEV_BUILD )
    string ( LENGTH "${git_dirty}" is_dirty )
    if ( ${is_dirty} GREATER 0 )
      message ( FATAL_ERROR "Release package must be built from a clean tree" )
    endif ()
    if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
      message ( FATAL_ERROR "Release package can't be built from a debug build" )
    endif ()
  endif ( )
  set ( CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.lgeneral" )
  if ( NOT APPLE )
    set ( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/COPYING" )
  endif ()
  message ( "version:${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}"
)
endif()

#check functions
include (CheckFunctionExists)
check_function_exists (putenv HAVE_PUTENV)
check_function_exists (setenv HAVE_SETENV)
check_function_exists (stpcpy HAVE_STPCPY)
check_function_exists (catgets HAVE_CATGETS)

#check header files
check_include_files (nl_types.h HAVE_NL_TYPES_H)
check_include_files (stdlib.h STDC_HEADERS)
check_include_files (malloc.h MALLOC_H)
check_include_files (string.h STRING_H)
check_include_files (libintl.h _LIBC)
check_include_files (locale.h HAVE_LOCALE_H)

#check symbols
include (CheckSymbolExists)
check_symbol_exists(LC_MESSAGES "locale.h" HAVE_LC_MESSAGES) 

configure_file ( "${CMAKE_SOURCE_DIR}/cmake_config.h.in" "${CMAKE_SOURCE_DIR}/config.h" )

add_definitions( -DPACKAGE="lgeneral" )
add_definitions( -DGNULOCALEDIR=\"$(gnulocaledir)\" )

#files to compile
add_subdirectory ( src )
