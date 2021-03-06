# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindCygwin
# ----------
#
# this module looks for Cygwin

if (WIN32)
  if(CYGWIN_INSTALL_PATH)
    set(CYGWIN_BAT "${CYGWIN_INSTALL_PATH}/cygwin.bat")
  endif()

  find_program(CYGWIN_BAT
    cygwin.bat
    "C:/Cygwin"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Cygwin\\setup;rootdir]"
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Cygnus Solutions\\Cygwin\\mounts v2\\/;native]"
  )
  get_filename_component(CYGWIN_INSTALL_PATH "${CYGWIN_BAT}" DIRECTORY)
  mark_as_advanced(CYGWIN_BAT)

endif ()
