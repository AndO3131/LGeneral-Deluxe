# Downloaded from https://github.com/jedbrown/cmake-modules
#
# Copyright $(git shortlog -s)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice, this
#   list of conditions and the following disclaimer in the documentation and/or
#   other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

SET(Git_FOUND FALSE)
 
FIND_PROGRAM(Git_EXECUTABLE git
  DOC "git command line client")
MARK_AS_ADVANCED(Git_EXECUTABLE)
 
IF(Git_EXECUTABLE)
  SET(Git_FOUND TRUE)
  MACRO(Git_WC_INFO dir prefix)
    EXECUTE_PROCESS(COMMAND ${Git_EXECUTABLE} rev-list -n 1 HEAD
       WORKING_DIRECTORY ${dir}
        ERROR_VARIABLE Git_error
       OUTPUT_VARIABLE ${prefix}_WC_REVISION_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT ${Git_error} EQUAL 0)
      MESSAGE(SEND_ERROR "Command \"${Git_EXECUTBALE} rev-list -n 1 HEAD\" in directory ${dir} failed with output:\n${Git_error}")
    ELSE(NOT ${Git_error} EQUAL 0)
      EXECUTE_PROCESS(COMMAND ${Git_EXECUTABLE} name-rev ${${prefix}_WC_REVISION_HASH}
         WORKING_DIRECTORY ${dir}
         OUTPUT_VARIABLE ${prefix}_WC_REVISION_NAME
          OUTPUT_STRIP_TRAILING_WHITESPACE)
    ENDIF(NOT ${Git_error} EQUAL 0)
  ENDMACRO(Git_WC_INFO)
ENDIF(Git_EXECUTABLE)
 
IF(NOT Git_FOUND)
  IF(NOT Git_FIND_QUIETLY)
    MESSAGE(STATUS "Git was not found")
  ELSE(NOT Git_FIND_QUIETLY)
    if(Git_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Git was not found")
    ENDIF(Git_FIND_REQUIRED)
  ENDIF(NOT Git_FIND_QUIETLY)
ENDIF(NOT Git_FOUND)
 
