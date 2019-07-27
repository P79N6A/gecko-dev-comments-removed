











#include "mozilla/mozalloc.h"

#define hnj_malloc(size)      moz_xmalloc(size)
#define hnj_realloc(p, size)  moz_xrealloc(p, size)
#define hnj_free(p)           moz_free(p)








#include <stdio.h> 

#undef FILE
#define FILE hnjFile

#define fopen(path,mode)      hnjFopen(path,mode)
#define fclose(file)          hnjFclose(file)
#define fgets(buf,count,file) hnjFgets(buf,count,file)

typedef struct hnjFile_ hnjFile;

#ifdef __cplusplus
extern "C" {
#endif

hnjFile* hnjFopen(const char* aURISpec, const char* aMode);

int hnjFclose(hnjFile* f);

char* hnjFgets(char* s, int n, hnjFile* f);

#ifdef __cplusplus
}
#endif


