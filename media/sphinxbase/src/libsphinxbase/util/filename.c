







































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sphinxbase/filename.h"

#ifdef _MSC_VER
#pragma warning (disable: 4996)
#endif

const char *
path2basename(const char *path)
{
    const char *result;

#if defined(_WIN32) || defined(__CYGWIN__)
    result = strrchr(path, '\\');
#else
    result = strrchr(path, '/');
#endif

    return (result == NULL ? path : result + 1);
}


void
path2dirname(const char *path, char *dir)
{
    size_t i, l;

    l = strlen(path);
#if defined(_WIN32) || defined(__CYGWIN__)
    for (i = l - 1; (i > 0) && !(path[i] == '/' || path[i] == '\\'); --i);
#else
    for (i = l - 1; (i > 0) && !(path[i] == '/'); --i);
#endif
    if (i == 0) {
        dir[0] = '.';
        dir[1] = '\0';
    } else {
        memcpy(dir, path, i);
        dir[i] = '\0';
    }
}



void
strip_fileext(const char *path, char *root)
{
    size_t i, l;

    l = strlen(path);
    for (i = l - 1; (i > 0) && (path[i] != '.'); --i);
    if (i == 0) {
        strcpy(root, path);     
    } else {
        strncpy(root, path, i);
    }
}


int
path_is_absolute(const char *path)
{
#if defined(_WIN32) && !defined(_WIN32_WCE) 
    return 
        (strlen(path) >= 3
         &&
         ((path[0] >= 'A' && path[0] <= 'Z')
          || (path[0] >= 'a' && path[0] <= 'z'))
         && path[1] == ':'
         && (path[2] == '/' || path[2] == '\\'));
#elif defined(_WIN32_WCE)
    return path[0] == '\\' || path[0] == '/';
#else 
    return path[0] == '/';
#endif
}
