#WARNING: All non-linux commands need checking

include_directories ( .. )
include_directories ( ../util )
include_directories ( . )

set ( root_src
  main.c
  lgeneral.h
  action.c
  action.h
  ai.c
  ai.h
  ai_group.c
  ai_group.h
  ai_tools.c
  ai_tools.h
  campaign.c
  campaign.h
  config.c
  config.h
  date.c
  date.h
  engine.c
  engine.h
  event.c
  event.h
  file.c
  file.h
  gui.c
  gui.h
  headquarters_dlg.c
  headquarters_dlg.h
  image.c
  image.h
  list.c
  list.h
  map.c
  map.h
  message_dlg.c
  message_dlg.h
  misc.c
  misc.h
  nation.c
  nation.h
  parser.c
  parser.h
  player.c
  player.h
  purchase_dlg.c
  purchase_dlg.h
  scenario.c
  scenario.h
  sdl.c
  sdl.h
  slot.c
  slot.h
  strat_map.c
  strat_map.h
  terrain.c
  terrain.h
  unit.c
  unit.h
  unit_lib.c
  unit_lib.h
  windows.c
  windows.h
)

if (${WITH_SOUND})
set ( audio_src
  audio.c
  audio.h
)
endif ()

set ( util_src
  ../util/hashtable.c
  ../util/hashtable.h
#  util/hashtable_itr.c
#  util/hashtable_itr.h
#  util/hashtable_private.h
#  util/hashtable_utility.c
#  util/hashtable_utility.h
  ../util/localize.c
  ../util/localize.h
#  util/paths.c
#  util/paths.h
  ../util/paths-linux.c
#  util/portability.c
)

#set ( intl_src
#  intl/bindtextdom.c
#  intl/cat-compat.c
#  intl/dcgettext.c
#  intl/dgettext.c
#  intl/explodename.c
#  intl/finddomain.c
#  intl/gettext.c
#  intl/gettext.h
#  intl/gettextP.h
#  intl/hash-string.h
#  intl/intl-compat.c
#  intl/l10nflist.c
#  intl/libgettext.h
#  intl/loadinfo.h
#  intl/loadmsgcat.c
#  intl/localealias.c
#  intl/textdomain.c
#)

set ( fpge_src
  FPGE/pg.c
  FPGE/pg.h
  FPGE/pgf.c
  FPGE/pgf.h
)

set ( data_install_dir bin )
if ( CMAKE_COMPILER_IS_GNUCC AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
  add_definitions ( -DNDEBUG )
endif ()
if ( CMAKE_COMPILER_IS_GNUCC AND ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR ENABLE_WARNING) )
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

set ( lgeneral_src ${root_src} ${audio_src} ${util_src} ${fpge_src} )

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
target_link_libraries ( lgeneral -lm ${system_libs} ${SDL_LIBRARY} ${SDLMIXER_LIBRARY} ${SDLIMAGE_LIBRARY} )

add_custom_command ( TARGET lgeneral
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data ${EXECUTABLE_OUTPUT_PATH}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${EXECUTABLE_OUTPUT_PATH}/Default/AI-modules
  COMMAND ${CMAKE_COMMAND} -E make_directory ${EXECUTABLE_OUTPUT_PATH}/lgeneral-pg/Save
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/Default/AI-modules/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/pg/Campaigns/Makefile* 
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/Default/Graphics/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/Default/Graphics/flags/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/Default/Graphics/terrain/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/Default/Graphics/units/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/pg/Maps/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/Default/Music/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/pg/Nations/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/pg/Scenario/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/Default/Sound/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/Default/Themes/Makefile*
#  COMMAND ${CMAKE_COMMAND} -E remove ${EXECUTABLE_OUTPUT_PATH}/pg/Units/Makefile*
)
#install ( DIRECTORY ${CMAKE_SOURCE_DIR}/bin/data DESTINATION ${data_install_dir} )

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
