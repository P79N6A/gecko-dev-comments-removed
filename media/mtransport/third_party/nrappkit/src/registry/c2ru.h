











































#ifndef __C2RU_H__
#define __C2RU_H__

#include <sys/types.h>
#include <r_types.h>
#include <r_data.h>
#include "registry_int.h"

int nr_c2ru_get_char(NR_registry parent, char *child, char **out);
int nr_c2ru_get_uchar(NR_registry parent, char *child, UCHAR **out);
int nr_c2ru_get_int2(NR_registry parent, char *child, INT2 **out);
int nr_c2ru_get_uint2(NR_registry parent, char *child, UINT2 **out);
int nr_c2ru_get_int4(NR_registry parent, char *child, INT4 **out);
int nr_c2ru_get_uint4(NR_registry parent, char *child, UINT4 **out);
int nr_c2ru_get_int8(NR_registry parent, char *child, INT8 **out);
int nr_c2ru_get_uint8(NR_registry parent, char *child, UINT8 **out);
int nr_c2ru_get_double(NR_registry parent, char *child, double **out);
int nr_c2ru_get_data(NR_registry parent, char *child, Data **out);
int nr_c2ru_get_string(NR_registry parent, char *child, char ***out);

int nr_c2ru_set_char(NR_registry parent, char *child, char *data);
int nr_c2ru_set_uchar(NR_registry parent, char *child, UCHAR *data);
int nr_c2ru_set_int2(NR_registry parent, char *child, INT2 *data);
int nr_c2ru_set_uint2(NR_registry parent, char *child, UINT2 *data);
int nr_c2ru_set_int4(NR_registry parent, char *child, INT4 *data);
int nr_c2ru_set_uint4(NR_registry parent, char *child, UINT4 *data);
int nr_c2ru_set_int8(NR_registry parent, char *child, INT8 *data);
int nr_c2ru_set_uint8(NR_registry parent, char *child, UINT8 *data);
int nr_c2ru_set_double(NR_registry parent, char *child, double *data);
int nr_c2ru_set_registry(NR_registry parent, char *child);
int nr_c2ru_set_data(NR_registry parent, char *child, Data *data);
int nr_c2ru_set_string(NR_registry parent, char *child, char **data);

int nr_c2ru_free_char(char *data);
int nr_c2ru_free_uchar(UCHAR *data);
int nr_c2ru_free_int2(INT2 *data);
int nr_c2ru_free_uint2(UINT2 *data);
int nr_c2ru_free_int4(INT4 *data);
int nr_c2ru_free_uint4(UINT4 *data);
int nr_c2ru_free_int8(INT8 *data);
int nr_c2ru_free_uint8(UINT8 *data);
int nr_c2ru_free_double(double *data);
int nr_c2ru_free_data(Data *data);
int nr_c2ru_free_string(char **data);

int nr_c2ru_get_children(NR_registry parent, char *child, void *ptr, size_t size, int (*get)(NR_registry, void*));
int nr_c2ru_set_children(NR_registry parent, char *child, void *ptr, int (*set)(NR_registry, void*), int (*label)(NR_registry, void*, char[NR_REG_MAX_NR_REGISTRY_LEN]));
int nr_c2ru_free_children(void *ptr, int (*free)(void*));

int nr_c2ru_make_registry(NR_registry parent, char *child, NR_registry out);

#endif
