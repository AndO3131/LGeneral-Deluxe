project ( LGeneral-Deluxe )

cmake_minimum_required ( VERSION 2.8 )
set ( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules" )

option ( DEV_BUILD "Development Build. Disable this for release builds" ON )
option ( BUILD_PACKAGE "Prepares build for creation of a package with CPack" ON )
option ( ENABLE_WARNING "Always show warnings (even for release builds)" OFF )
option ( FATAL_WARNING "Treat warnings as errors" OFF )

find_package ( SDL2 COMPONENTS mixer)

if ( NOT SDL_FOUND )
  message ( FATAL_ERROR "Can't find SDL which is required" )
else ()
  include_directories ( ${SDL_INCLUDE_DIR} )
  message ( "found SDL ${SDL_MAJOR}.${SDL_MINOR}.${SDL_MICRO} (${SDL_LIBRARY_DIRS}:${SDL_INCLUDE_DIR})" )
endif ()

if ( NOT SDLMIXER_FOUND )
  message ( WARNING "Can't find SDL_mixer" )
else ()
  include_directories ( ${SDLMIXER_INCLUDE_DIR} )
  message ( "found SDL_mixer ${SDL_MIXER_MAJOR}.${SDL_MIXER_MINOR}.${SDL_MIXER_MICRO} (${SDL_MIXER_LIBRARY_DIRS}:${SDLMIXER_INCLUDE_DIR})" )
endif ()

# Read version number
find_package ( Git )
if ( GIT_FOUND )
  message("git found: ${GIT_EXECUTABLE}")
  execute_process ( COMMAND ${GIT_EXECUTABLE} describe --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE git_describe_out
    ERROR_VARIABLE git_describe_error
    RESULT_VARIABLE git_describe_result
    )
  string ( REGEX MATCH "([a-z|0-9|.]*)-([0-9]*)-([a-z|0-9]*)([-|a-z]*)" git_commit "${git_describe_out}" )
  set ( git_tag ${CMAKE_MATCH_1} )
  set ( git_nb_commit ${CMAKE_MATCH_2} )
  set ( git_commit ${CMAKE_MATCH_3} )
  set ( git_dirty ${CMAKE_MATCH_4} )
endif()

if ( DEV_BUILD )
  # Append the commit to version number
  set ( CPACK_PACKAGE_VERSION_PATCH "${git_commit}${git_dirty}" )
  set ( CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}" )
  add_definitions( -DVERSION="${CPACK_PACKAGE_VERSION}" )
else ()
  set ( CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_NSIS_PACKAGE_NAME}" )
  set ( CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}" )
  add_definitions( -DVERSION="${CPACK_PACKAGE_VERSION}" )
endif ()

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
check_include_files (nl_types.h HAVE_NL_TYPES_H)
check_include_files (stdlib.h STDC_HEADERS)

#check_include_files (locale.h HAVE_LOCALE_H)
configure_file ( "${CMAKE_SOURCE_DIR}/cmake_config.h.in" "${CMAKE_SOURCE_DIR}/config.h" )

add_definitions( -DPACKAGE="lgeneral" )
add_definitions( -DGNULOCALEDIR=\"$(gnulocaledir)\" )

#files to compile
add_subdirectory ( src )
