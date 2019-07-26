








































#ifndef __REGISTRY_VTBL_H__
#define __REGISTRY_VTBL_H__

typedef struct nr_registry_module_ nr_registry_module;

typedef struct nr_registry_module_vtbl_ {
    int (*init)(nr_registry_module*);

    int (*get_char)(NR_registry name, char *out);
    int (*get_uchar)(NR_registry name, UCHAR *out);
    int (*get_int2)(NR_registry name, INT2 *out);
    int (*get_uint2)(NR_registry name, UINT2 *out);
    int (*get_int4)(NR_registry name, INT4 *out);
    int (*get_uint4)(NR_registry name, UINT4 *out);
    int (*get_int8)(NR_registry name, INT8 *out);
    int (*get_uint8)(NR_registry name, UINT8 *out);
    int (*get_double)(NR_registry name, double *out);
    int (*get_registry)(NR_registry name, NR_registry out);

    int (*get_bytes)(NR_registry name, UCHAR *out, size_t size, size_t *length);
    int (*get_string)(NR_registry name, char *out, size_t size);
    int (*get_length)(NR_registry name, size_t *length);
    int (*get_type)(NR_registry name, NR_registry_type type);

    int (*set_char)(NR_registry name, char data);
    int (*set_uchar)(NR_registry name, UCHAR data);
    int (*set_int2)(NR_registry name, INT2 data);
    int (*set_uint2)(NR_registry name, UINT2 data);
    int (*set_int4)(NR_registry name, INT4 data);
    int (*set_uint4)(NR_registry name, UINT4 data);
    int (*set_int8)(NR_registry name, INT8 data);
    int (*set_uint8)(NR_registry name, UINT8 data);
    int (*set_double)(NR_registry name, double data);
    int (*set_registry)(NR_registry name);

    int (*set_bytes)(NR_registry name, UCHAR *data, size_t length);
    int (*set_string)(NR_registry name, char *data);

    int (*del)(NR_registry name);

    int (*get_child_count)(NR_registry parent, size_t *count);
    int (*get_children)(NR_registry parent, NR_registry *data, size_t size, size_t *length);

    int (*fin)(NR_registry name);

    int (*dump)(int sorted);
} nr_registry_module_vtbl;

struct nr_registry_module_ {
    void *handle;
    nr_registry_module_vtbl *vtbl;
};

#endif

