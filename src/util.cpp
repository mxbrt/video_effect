#include "util.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

long get_mtime(const char* path) {
  struct stat stat_buf;
  if (stat(path, &stat_buf)) {
    die("stat %s: %s\n", path, strerror(errno));
  }
  return stat_buf.st_mtim.tv_sec;
}

void die(const char *format, ...) {
  va_list args;
  va_start(args, format);

  vfprintf(stderr, format, args);
  va_end(args);
  exit(1);
}
