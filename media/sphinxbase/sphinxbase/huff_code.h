














































#ifndef __HUFF_CODE_H__
#define __HUFF_CODE_H__

#include <stdio.h>

#include <sphinxbase/sphinxbase_export.h>
#include <sphinxbase/prim_type.h>
#include <sphinxbase/cmd_ln.h>

typedef struct huff_code_s huff_code_t;




SPHINXBASE_EXPORT
huff_code_t *huff_code_build_int(int32 const *values, int32 const *frequencies, int nvals);




SPHINXBASE_EXPORT
huff_code_t *huff_code_build_str(char * const *values, int32 const *frequencies, int nvals);




SPHINXBASE_EXPORT
huff_code_t *huff_code_read(FILE *infh);




SPHINXBASE_EXPORT
int huff_code_write(huff_code_t *hc, FILE *outfh);




SPHINXBASE_EXPORT
int huff_code_dump(huff_code_t *hc, FILE *dumpfh);




SPHINXBASE_EXPORT
huff_code_t *huff_code_retain(huff_code_t *hc);




SPHINXBASE_EXPORT
int huff_code_free(huff_code_t *hc);




SPHINXBASE_EXPORT
FILE *huff_code_attach(huff_code_t *hc, FILE *fh, char const *mode);




SPHINXBASE_EXPORT
FILE *huff_code_detach(huff_code_t *hc);




SPHINXBASE_EXPORT
int huff_code_encode_int(huff_code_t *hc, int32 sym, uint32 *outcw);




SPHINXBASE_EXPORT
int huff_code_encode_str(huff_code_t *hc, char const *sym, uint32 *outcw);




SPHINXBASE_EXPORT
int huff_code_decode_int(huff_code_t *hc, int *outval,
                         char const **inout_data,
                         size_t *inout_data_len,
                         int *inout_offset);




SPHINXBASE_EXPORT
char const *huff_code_decode_str(huff_code_t *hc,
                                 char const **inout_data,
                                 size_t *inout_data_len,
                                 int *inout_offset);

#endif 
