







#ifndef builtin_TypedObjectConstants_h
#define builtin_TypedObjectConstants_h




#define JS_TYPROTO_SLOT_DESCR            0
#define JS_TYPROTO_SLOTS                 1










#define JS_DESCR_SLOT_KIND               0  // Atomized string representation
#define JS_DESCR_SLOT_STRING_REPR        1  // Atomized string representation
#define JS_DESCR_SLOT_ALIGNMENT          2  // Alignment in bytes
#define JS_DESCR_SLOT_SIZE               3  // Size in bytes, else 0
#define JS_DESCR_SLOT_OPAQUE             4  // Atomized string representation
#define JS_DESCR_SLOT_TYPROTO            5  // Prototype for instances, if any
#define JS_DESCR_SLOT_TRACE_LIST         6  // List of references for use in tracing


#define JS_DESCR_SLOT_TYPE               7  // Type code


#define JS_DESCR_SLOT_ARRAY_ELEM_TYPE    7
#define JS_DESCR_SLOT_ARRAY_LENGTH       8


#define JS_DESCR_SLOT_STRUCT_FIELD_NAMES 7
#define JS_DESCR_SLOT_STRUCT_FIELD_TYPES 8
#define JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS 9


#define JS_DESCR_SLOTS                   10




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





#define JS_REFERENCETYPEREPR_ANY        0
#define JS_REFERENCETYPEREPR_OBJECT     1
#define JS_REFERENCETYPEREPR_STRING     2





#define JS_SIMDTYPEREPR_INT32         0
#define JS_SIMDTYPEREPR_FLOAT32       1

#endif
