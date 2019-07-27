



























































#ifndef _LIBUTIL_FILENAME_H_
#define _LIBUTIL_FILENAME_H_


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>




#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif




SPHINXBASE_EXPORT
const char *path2basename(const char *path);





SPHINXBASE_EXPORT
void path2dirname(const char *path, char *dir);







SPHINXBASE_EXPORT
void strip_fileext(const char *file, char *root);




SPHINXBASE_EXPORT
int path_is_absolute(const char *file);

#ifdef __cplusplus
}
#endif


#endif
