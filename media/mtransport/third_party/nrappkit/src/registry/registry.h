











































#ifndef __REGISTRY_H__
#define __REGISTRY_H__

#include <stdio.h>
#include <sys/types.h>
#include <r_types.h>
#include <r_data.h>

#define NR_REG_MAX_NR_REGISTRY_LEN     128
#define NR_REG_MAX_TYPE_LEN            32

typedef char NR_registry[NR_REG_MAX_NR_REGISTRY_LEN];
typedef char NR_registry_type[NR_REG_MAX_TYPE_LEN];

extern NR_registry NR_TOP_LEVEL_REGISTRY;

extern void *NR_REG_MODE_LOCAL;
extern void *NR_REG_MODE_REMOTE;

int NR_reg_init(void *mode);

int NR_reg_initted(void);

int NR_reg_get_char(NR_registry name, char *out);
int NR_reg_get_uchar(NR_registry name, UCHAR *out);
int NR_reg_get_int2(NR_registry name, INT2 *out);
int NR_reg_get_uint2(NR_registry name, UINT2 *out);
int NR_reg_get_int4(NR_registry name, INT4 *out);
int NR_reg_get_uint4(NR_registry name, UINT4 *out);
int NR_reg_get_int8(NR_registry name, INT8 *out);
int NR_reg_get_uint8(NR_registry name, UINT8 *out);
int NR_reg_get_double(NR_registry name, double *out);
int NR_reg_get_registry(NR_registry name, NR_registry out);

int NR_reg_get_bytes(NR_registry name, UCHAR *out, size_t size, size_t *length);
int NR_reg_get_string(NR_registry name, char *out, size_t size);
int NR_reg_get_length(NR_registry name, size_t *length);
int NR_reg_get_type(NR_registry name, NR_registry_type type);


int NR_reg_get2_char(NR_registry prefix, char *name, char *);
int NR_reg_get2_uchar(NR_registry prefix, char *name, UCHAR *);
int NR_reg_get2_int2(NR_registry prefix, char *name, INT2 *);
int NR_reg_get2_uint2(NR_registry prefix, char *name, UINT2 *);
int NR_reg_get2_int4(NR_registry prefix, char *name, INT4 *);
int NR_reg_get2_uint4(NR_registry prefix, char *name, UINT4 *);
int NR_reg_get2_int8(NR_registry prefix, char *name,  INT8 *);
int NR_reg_get2_uint8(NR_registry prefix, char *name, UINT8 *);
int NR_reg_get2_double(NR_registry prefix, char *name, double *);
int NR_reg_get2_bytes(NR_registry prefix, char *name, UCHAR *out, size_t size, size_t *length);
int NR_reg_get2_string(NR_registry prefix, char *name, char *out, size_t size);

int NR_reg_alloc2_string(NR_registry prefix, char *name, char **);
int NR_reg_alloc2_data(NR_registry prefix, char *name, Data *);

int NR_reg_set_char(NR_registry name, char data);
int NR_reg_set_uchar(NR_registry name, UCHAR data);
int NR_reg_set_int2(NR_registry name, INT2 data);
int NR_reg_set_uint2(NR_registry name, UINT2 data);
int NR_reg_set_int4(NR_registry name, INT4 data);
int NR_reg_set_uint4(NR_registry name, UINT4 data);
int NR_reg_set_int8(NR_registry name, INT8 data);
int NR_reg_set_uint8(NR_registry name, UINT8 data);
int NR_reg_set_double(NR_registry name, double data);

int NR_reg_set_registry(NR_registry name);

int NR_reg_set_bytes(NR_registry name, UCHAR *data, size_t length);
int NR_reg_set_string(NR_registry name, char *data);

int NR_reg_set2_char(NR_registry prefix, char *name, char data);
int NR_reg_set2_uchar(NR_registry prefix, char *name, UCHAR data);
int NR_reg_set2_int2(NR_registry prefix, char *name, INT2 data);
int NR_reg_set2_uint2(NR_registry prefix, char *name, UINT2 data);
int NR_reg_set2_int4(NR_registry prefix, char *name, INT4 data);
int NR_reg_set2_uint4(NR_registry prefix, char *name, UINT4 data);
int NR_reg_set2_int8(NR_registry prefix, char *name, INT8 data);
int NR_reg_set2_uint8(NR_registry prefix, char *name, UINT8 data);
int NR_reg_set2_double(NR_registry prefix, char *name, double data);

int NR_reg_set2_bytes(NR_registry prefix, char *name, UCHAR *data, size_t length);
int NR_reg_set2_string(NR_registry prefix, char *name, char *data);

int NR_reg_del(NR_registry name);

int NR_reg_fin(NR_registry name);

int NR_reg_get_child_count(NR_registry parent, unsigned int *count);
int NR_reg_get_child_registry(NR_registry parent, unsigned int i, NR_registry child);
int NR_reg_get2_child_count(NR_registry base, NR_registry name, unsigned int *count);
int NR_reg_get2_child_registry(NR_registry base, NR_registry name, unsigned int i, NR_registry child);
int NR_reg_get_children(NR_registry parent, NR_registry children[], size_t size, size_t *length);

int NR_reg_dump(void);


int NR_reg_alloc_data(NR_registry name, Data *data);
int NR_reg_alloc_string(NR_registry name, char **data);

#define NR_REG_CB_ACTION_ADD      (1<<0)
#define NR_REG_CB_ACTION_CHANGE   (1<<1)
#define NR_REG_CB_ACTION_DELETE   (1<<2)
#define NR_REG_CB_ACTION_FINAL    (1<<6)
int NR_reg_register_callback(NR_registry name, char action, void (*cb)(void *cb_arg, char action, NR_registry name), void *cb_arg);
int NR_reg_unregister_callback(NR_registry name, char action, void (*cb)(void *cb_arg, char action, NR_registry name));

int NR_reg_make_registry(NR_registry parent, char *child, NR_registry out);
int NR_reg_make_child_registry(NR_registry parent, NR_registry descendant, unsigned int generation, NR_registry child);

#endif
