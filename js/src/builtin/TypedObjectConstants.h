







#ifndef builtin_TypedObjectConstants_h
#define builtin_TypedObjectConstants_h




#define JS_SETTYPEDARRAY_SAME_TYPE 0
#define JS_SETTYPEDARRAY_OVERLAPPING 1
#define JS_SETTYPEDARRAY_DISJOINT 2




#define JS_TYPEDARRAYLAYOUT_BUFFER_SLOT 0
#define JS_TYPEDARRAYLAYOUT_LENGTH_SLOT 1
#define JS_TYPEDARRAYLAYOUT_BYTEOFFSET_SLOT 2




#define JS_ARRAYBUFFER_FLAGS_SLOT            3

#define JS_ARRAYBUFFER_NEUTERED_FLAG 0x4




#define JS_TYPROTO_SLOTS                 0










#define JS_DESCR_SLOT_KIND               0  // Atomized string representation
#define JS_DESCR_SLOT_STRING_REPR        1  // Atomized string representation
#define JS_DESCR_SLOT_ALIGNMENT          2  // Alignment in bytes
#define JS_DESCR_SLOT_SIZE               3  // Size in bytes, else 0
#define JS_DESCR_SLOT_OPAQUE             4  // Atomized string representation
#define JS_DESCR_SLOT_TYPROTO            5  // Prototype for instances, if any
#define JS_DESCR_SLOT_ARRAYPROTO         6  // Lazily created prototype for arrays
#define JS_DESCR_SLOT_TRACE_LIST         7  // List of references for use in tracing


#define JS_DESCR_SLOT_TYPE               8  // Type code
#define JS_DESCR_SLOT_LANES              9


#define JS_DESCR_SLOT_ARRAY_ELEM_TYPE    8
#define JS_DESCR_SLOT_ARRAY_LENGTH       9


#define JS_DESCR_SLOT_STRUCT_FIELD_NAMES 8
#define JS_DESCR_SLOT_STRUCT_FIELD_TYPES 9
#define JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS 10


#define JS_DESCR_SLOTS                   11




#define JS_TYPEREPR_SCALAR_KIND         1
#define JS_TYPEREPR_REFERENCE_KIND      2
#define JS_TYPEREPR_STRUCT_KIND         3
#define JS_TYPEREPR_ARRAY_KIND          4
#define JS_TYPEREPR_SIMD_KIND           5




#define JS_SCALARTYPEREPR_INT8          0
#define JS_SCALARTYPEREPR_UINT8         1
#define JS_SCALARTYPEREPR_INT16         2
#define JS_SCALARTYPEREPR_UINT16        3
#define JS_SCALARTYPEREPR_INT32         4
#define JS_SCALARTYPEREPR_UINT32        5
#define JS_SCALARTYPEREPR_FLOAT32       6
#define JS_SCALARTYPEREPR_FLOAT64       7
#define JS_SCALARTYPEREPR_UINT8_CLAMPED 8
#define JS_SCALARTYPEREPR_FLOAT32X4     10
#define JS_SCALARTYPEREPR_INT32X4       11





#define JS_REFERENCETYPEREPR_ANY        0
#define JS_REFERENCETYPEREPR_OBJECT     1
#define JS_REFERENCETYPEREPR_STRING     2





#define JS_SIMDTYPEREPR_INT32         0
#define JS_SIMDTYPEREPR_FLOAT32       1
#define JS_SIMDTYPEREPR_FLOAT64       2

#endif
