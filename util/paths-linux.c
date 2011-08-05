/*
    Getting application paths.
    Copyright (C) 2005 Leo Savernik  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "paths.h"

#include <sys/types.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char *paths_exec_path(void) {
  static char *exec_path_buf;
  if (!exec_path_buf) {
    char file[512];
    unsigned size = 1;
    unsigned written = 0;
    snprintf(file, sizeof file, "/proc/%u/exe", getpid());
    do {
      size *= 2;
      exec_path_buf = realloc(exec_path_buf, size);
      if (!exec_path_buf
          || ((int)(written = readlink(file, exec_path_buf, size)) == -1)) {
        free(exec_path_buf);
        exec_path_buf = "";
        break;
      }
    } while (written == size);
    /* trim buffer to the exact length and NUL it */
    if (exec_path_buf[0]) {
      exec_path_buf = realloc(exec_path_buf, written + 1);
      exec_path_buf[written] = '\0';
    }
  }
  return exec_path_buf;
}

const char *paths_prefix(void) {
  static char *prefix_buf;
  if (!prefix_buf) {
    char *pos;
    prefix_buf = strdup(paths_exec_path());
    /* ignore executable's file name */
    pos = strrchr(prefix_buf, '/');
    prefix_buf[pos ? pos - prefix_buf : 0] = '\0';
    /* ignore trailing '/bin' if any */
    pos = strrchr(prefix_buf, '/');
    if (pos && memcmp(pos, "/bin", sizeof "/bin" - 1) == 0) *pos = '\0';
  }
  return prefix_buf;
}

/* kate: tab-indents on; space-indent on; replace-tabs off; indent-width 2; dynamic-word-wrap off; inden(t)-mode cstyle */
