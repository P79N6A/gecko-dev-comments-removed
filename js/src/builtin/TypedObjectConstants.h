







#ifndef builtin_TypedObjectConstants_h
#define builtin_TypedObjectConstants_h




#define JS_TYPROTO_SLOT_DESCR            0
#define JS_TYPROTO_SLOTS                 1










#define JS_DESCR_SLOT_KIND               0  // Atomized string representation
#define JS_DESCR_SLOT_STRING_REPR        1  // Atomized string representation
#define JS_DESCR_SLOT_ALIGNMENT          2  // Alignment in bytes
#define JS_DESCR_SLOT_SIZE               3  // Size in bytes, if sized, else 0
#define JS_DESCR_SLOT_OPAQUE             4  // Atomized string representation
#define JS_DESCR_SLOT_TYPROTO            5  // Prototype for instances, if any


#define JS_DESCR_SLOT_TYPE               6  // Type code


#define JS_DESCR_SLOT_ARRAY_ELEM_TYPE    6


#define JS_DESCR_SLOT_SIZED_ARRAY_LENGTH 7


#define JS_DESCR_SLOT_STRUCT_FIELD_NAMES 6
#define JS_DESCR_SLOT_STRUCT_FIELD_TYPES 7
#define JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS 8


#define JS_DESCR_SLOTS                   9




#define JS_TYPEREPR_UNSIZED_ARRAY_KIND  0
#define JS_TYPEREPR_MAX_UNSIZED_KIND    0    // Unsized kinds go above here
#define JS_TYPEREPR_SCALAR_KIND         1
#define JS_TYPEREPR_REFERENCE_KIND      2
#define JS_TYPEREPR_STRUCT_KIND         3
#define JS_TYPEREPR_SIZED_ARRAY_KIND    4
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






#define JS_BUFVIEW_SLOT_BYTEOFFSET       0
#define JS_BUFVIEW_SLOT_LENGTH           1 // see (*) below
#define JS_BUFVIEW_SLOT_OWNER            2


#define JS_DATAVIEW_SLOT_DATA            3 // see (**) below
#define JS_DATAVIEW_SLOTS                3 // Number of slots for data views


#define JS_TYPEDARR_SLOT_DATA            3 // see (**) below
#define JS_TYPEDARR_SLOTS                3 // Number of slots for typed arrays


#define JS_TYPEDOBJ_SLOT_DATA            3
#define JS_TYPEDOBJ_SLOTS                3 // Number of slots for typed objs













#endif
