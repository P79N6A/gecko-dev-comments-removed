







#ifndef builtin_TypedObjectConstants_h
#define builtin_TypedObjectConstants_h










#define JS_DESCR_SLOT_TYPE_REPR          0  // Associated Type Representation
#define JS_DESCR_SLOT_ALIGNMENT          1  // Alignment in bytes
#define JS_DESCR_SLOT_SIZE               2  // Size in bytes, if sized, else 0
#define JS_DESCR_SLOT_PROTO              3  // Prototype for instances, if any


#define JS_DESCR_SLOT_TYPE               4  // Type code


#define JS_DESCR_SLOT_ARRAY_ELEM_TYPE    4


#define JS_DESCR_SLOT_SIZED_ARRAY_LENGTH 5


#define JS_DESCR_SLOT_STRUCT_FIELD_NAMES 4
#define JS_DESCR_SLOT_STRUCT_FIELD_TYPES 5
#define JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS 6


#define JS_DESCR_SLOTS                   7











#define JS_TYPEREPR_SLOT_KIND      0 // One of the `kind` constants below
#define JS_TYPEREPR_SLOT_SIZE      1 // Size in bytes.
#define JS_TYPEREPR_SLOT_ALIGNMENT 2 // Alignment in bytes.


#define JS_TYPEREPR_SLOT_LENGTH    3 // Length of the array


#define JS_TYPEREPR_SLOT_TYPE      3 // One of the constants below


#define JS_TYPEREPR_SLOTS          4




#define JS_TYPEREPR_UNSIZED_ARRAY_KIND  0
#define JS_TYPEREPR_MAX_UNSIZED_KIND    0    // Unsized kinds go above here
#define JS_TYPEREPR_SCALAR_KIND         1
#define JS_TYPEREPR_REFERENCE_KIND      2
#define JS_TYPEREPR_STRUCT_KIND         3
#define JS_TYPEREPR_SIZED_ARRAY_KIND    4
#define JS_TYPEREPR_X4_KIND             5





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





#define JS_X4TYPEREPR_INT32         0
#define JS_X4TYPEREPR_FLOAT32       1




#define JS_TYPEDOBJ_SLOT_BYTEOFFSET       0
#define JS_TYPEDOBJ_SLOT_BYTELENGTH       1
#define JS_TYPEDOBJ_SLOT_OWNER            2
#define JS_TYPEDOBJ_SLOT_NEXT_VIEW        3

#define JS_DATAVIEW_SLOTS              4 // Number of slots for data views

#define JS_TYPEDOBJ_SLOT_LENGTH           4 // Length of array (see (*) below)
#define JS_TYPEDOBJ_SLOT_TYPE_DESCR       5 // For typed objects, type descr

#define JS_TYPEDOBJ_SLOT_DATA             7 // private slot, based on alloc kind
#define JS_TYPEDOBJ_SLOTS                 6 // Number of slots for typed objs






#endif
