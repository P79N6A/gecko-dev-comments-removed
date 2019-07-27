
















































































#ifndef _LIBUTIL_IO_H_
#define _LIBUTIL_IO_H_

#include <stdio.h>
#if !defined(_WIN32_WCE) && !(defined(__ADSPBLACKFIN__) && !defined(__linux__))
#include <sys/stat.h>
#endif


#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>











#ifdef __cplusplus
extern "C" {
#endif
#if 0

}
#endif





SPHINXBASE_EXPORT
FILE *fopen_comp (const char *file,		
		  const char *mode,		
		  int32 *ispipe	

	);




SPHINXBASE_EXPORT
void fclose_comp (FILE *fp,		
		  int32 ispipe	

	);





SPHINXBASE_EXPORT
FILE *fopen_compchk (const char *file,	
		     int32 *ispipe	

	);




SPHINXBASE_EXPORT
FILE *_myfopen(const char *file, const char *mode,
	       const char *pgm, int32 line);	
#define myfopen(file,mode)	_myfopen((file),(mode),__FILE__,__LINE__)









SPHINXBASE_EXPORT
int32 fread_retry(void *pointer, int32 size, int32 num_items, FILE *stream);











SPHINXBASE_EXPORT
char *fread_line(FILE *stream, size_t *out_len);




typedef struct lineiter_t {
    char *buf;
    FILE *fh;
    int32 bsiz;
    int32 len;
    int32 clean;
    int32 lineno;
} lineiter_t;




SPHINXBASE_EXPORT
lineiter_t *lineiter_start(FILE *fh);




SPHINXBASE_EXPORT
lineiter_t *lineiter_start_clean(FILE *fh);




SPHINXBASE_EXPORT
lineiter_t *lineiter_next(lineiter_t *li);




SPHINXBASE_EXPORT
void lineiter_free(lineiter_t *li);




SPHINXBASE_EXPORT
int lineiter_lineno(lineiter_t *li);


#ifdef _WIN32_WCE

#include <windows.h>
struct stat {
    DWORD st_mtime;
    DWORD st_size;
};
#endif 

#if defined(__ADSPBLACKFIN__) && !defined(__linux__)
struct stat {
    int32 st_mtime;
    int32 st_size;
};

#endif




typedef struct bit_encode_s bit_encode_t;




bit_encode_t *bit_encode_attach(FILE *outfh);




bit_encode_t *bit_encode_retain(bit_encode_t *be);






int bit_encode_free(bit_encode_t *be);




int bit_encode_write(bit_encode_t *be, unsigned char const *bits, int nbits);




int bit_encode_write_cw(bit_encode_t *be, uint32 codeword, int nbits);




int bit_encode_flush(bit_encode_t *be);










SPHINXBASE_EXPORT
int32 stat_retry (const char *file, struct stat *statbuf);





SPHINXBASE_EXPORT
int32 stat_mtime (const char *file);






SPHINXBASE_EXPORT
int build_directory(const char *path);

#ifdef __cplusplus
}
#endif

#endif
