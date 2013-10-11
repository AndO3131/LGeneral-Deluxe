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
configure_file ( "${CMAKE_SOURCE_DIR}/cmake_define.h.in" "${CMAKE_SOURCE_DIR}/cmake_define.h" )

add_definitions( -DPACKAGE="lgeneral" )
add_definitions( -DGNULOCALEDIR=\"$(gnulocaledir)\" )

#files to compile
add_subdirectory ( po )

include_directories ( src )
include_directories ( util )
include_directories ( . )

set ( root_src
  src/main.c
  src/lgeneral.h
  src/action.c
  src/action.h
  src/ai.c
  src/ai.h
  src/ai_group.c
  src/ai_group.h
  src/ai_tools.c
  src/ai_tools.h
  src/audio.c
  src/audio.h
  src/campaign.c
  src/campaign.h
  src/config.c
  src/config.h
  src/date.c
  src/date.h
  src/engine.c
  src/engine.h
  src/file.c
  src/file.h
  src/gui.c
  src/gui.h
  src/image.c
  src/image.h
  src/list.c
  src/list.h
  src/map.c
  src/map.h
  src/misc.c
  src/misc.h
  src/nation.c
  src/nation.h
  src/parser.c
  src/parser.h
  src/player.c
  src/player.h
  src/purchase_dlg.c
  src/purchase_dlg.h
  src/scenario.c
  src/scenario.h
  src/sdl.c
  src/sdl.h
  src/slot.c
  src/slot.h
  src/strat_map.c
  src/strat_map.h
  src/terrain.c
  src/terrain.h
  src/unit.c
  src/unit.h
  src/unit_lib.c
  src/unit_lib.h
  src/windows.c
  src/windows.h
)

set ( util_src
  util/hashtable.c
  util/hashtable.h
  util/hashtable_itr.c
  util/hashtable_itr.h
  util/hashtable_private.h
  util/hashtable_utility.c
  util/hashtable_utility.h
  util/localize.c
  util/localize.h
  util/paths.c
  util/paths.h
  util/paths-linux.c
  util/portability.c
)

set ( intl_src
  intl/bindtextdom.c
  intl/cat-compat.c
  intl/dcgettext.c
  intl/dgettext.c
  intl/explodename.c
  intl/finddomain.c
  intl/gettext.c
  intl/gettext.h
  intl/gettextP.h
  intl/hash-string.h
  intl/intl-compat.c
  intl/l10nflist.c
  intl/libgettext.h
  intl/loadinfo.h
  intl/loadmsgcat.c
  intl/localealias.c
  intl/textdomain.c
)

set ( data_install_dir bin )
if ( CMAKE_COMPILER_IS_GNUCXX AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
  add_definitions ( -D_DEBUG )
endif ()
if ( CMAKE_COMPILER_IS_GNUCXX AND ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR ENABLE_WARNING) )
    # Enable more GCC warnings if requested or we are doing a Debug build.
    add_definitions ( -Wall
      -Wsign-compare
      -Winit-self
      -Wmissing-include-dirs
      -Wunknown-pragmas
      -Wmissing-format-attribute
      -Wredundant-decls
      -Winline
      -Wformat-security
      -Wtype-limits
      -Wclobbered
      -Wempty-body
      -Wlogical-op
      -Wuninitialized
      -Wshadow
# These two warning options actually generate way too many warnings. Should be enabled later.
# -Wignored-qualifiers
# -Weffc++
      )
    if ( FATAL_WARNING )
      add_definitions ( -Werror )
    endif ( FATAL_WARNING )
endif()

if ( WIN32 )
  set ( application_type WIN32 )
endif ()

set ( lgeneral_src ${root_src} ${util_src} ${intl_src} )

set ( install_dest RUNTIME )
set ( set_exec_path ON )
set ( install_dest_dir bin )
if ( UNIX AND NOT APPLE )
  if ( "${DATADIR}" STREQUAL "" )
    set ( data_install_dir "share/lgeneral" )
  else ()
    set ( data_install_dir ${DATADIR} )
    add_definitions( -DDATADIR="${data_install_dir}/" )
  endif ()
endif ()

if ( set_exec_path )
  set ( EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin )
endif ()

if ( FORCE_INSTALL_DATA_TO_BIN )
  set ( data_install_dir bin )
endif ()

add_executable ( lgeneral ${application_type} ${lgeneral_src} )
install ( TARGETS lgeneral ${install_dest} DESTINATION bin )
# Extra link flags for Windows. They need to be set before the SDL/YAML link flags, otherwise you will get strange link errors ('Undefined reference to WinMain@16')
if ( WIN32 )
  set ( basic_windows_libs advapi32.lib shell32.lib shlwapi.lib )
  if ( MINGW )
    set ( basic_windows_libs ${basic_windows_libs} mingw32 -mwindows )
    set ( static_flags -static )
    set ( SDLMIXER_LIBRARY "${SDLMIXER_LIBRARY} -lwinmm" )
  endif ()
  set ( system_libs ${basic_windows_libs} SDLmain ${static_flags} )
endif ()
target_link_libraries ( lgeneral ${system_libs} ${SDLIMAGE_LIBRARY} ${SDLMIXER_LIBRARY} ${SDL_LIBRARY} )

add_custom_command ( TARGET lgeneral
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/data ${EXECUTABLE_OUTPUT_PATH}/data )
install ( DIRECTORY ${CMAKE_SOURCE_DIR}/bin/data DESTINATION ${data_install_dir} )

# Copy Windows DLLs to bin folder
if ( WIN32 )
  if ( CMAKE_CL_64 )
    file ( GLOB Windows_dll "${DEPS_DIR}/lib/x64/*.dll" )
  else ( )
    file ( GLOB Windows_dll "${DEPS_DIR}/lib/Win32/*.dll" )
  endif()
  foreach ( dll ${Windows_dll} )
    add_custom_command ( TARGET lgeneral
      POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy ${dll} ${EXECUTABLE_OUTPUT_PATH} )
    install ( FILES ${dll} DESTINATION bin)
  endforeach()
endif ()
