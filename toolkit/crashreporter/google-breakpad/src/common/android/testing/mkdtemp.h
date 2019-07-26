



































#ifndef GOOGLE_BREAKPAD_COMMON_ANDROID_TESTING_MKDTEMP_H
#define GOOGLE_BREAKPAD_COMMON_ANDROID_TESTING_MKDTEMP_H

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

namespace {

char* mkdtemp(char* path) {
  if (path == NULL) {
    errno = EINVAL;
    return NULL;
  }

  
  const char kSuffix[] = "XXXXXX";
  const size_t kSuffixLen = strlen(kSuffix);
  char* path_end = path + strlen(path);

  if (static_cast<size_t>(path_end - path) < kSuffixLen ||
      memcmp(path_end - kSuffixLen, kSuffix, kSuffixLen) != 0) {
    errno = EINVAL;
    return NULL;
  }

  
  
  char* sep = strrchr(path, '/');
  if (sep != NULL) {
    struct stat st;
    int ret;
    *sep = '\0';  
    ret = stat(path, &st);
    *sep = '/';   
    if (ret < 0)
      return NULL;
    if (!S_ISDIR(st.st_mode)) {
      errno = ENOTDIR;
      return NULL;
    }
  }

  
  
  int tries;
  for (tries = 128; tries > 0; tries--) {
    int random = rand() % 1000000;

    snprintf(path_end - kSuffixLen, kSuffixLen + 1, "%0d", random);
    if (mkdir(path, 0700) == 0)
      return path;  

    if (errno != EEXIST)
      return NULL;
  }

  assert(errno == EEXIST);
  return NULL;
}

}  

#endif  
