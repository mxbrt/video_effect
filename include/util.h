#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

long get_mtime(const char *path);
void die(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif  // UTIL_H
