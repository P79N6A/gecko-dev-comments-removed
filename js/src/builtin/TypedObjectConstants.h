







#ifndef builtin_TypedObjectConstants_h
#define builtin_TypedObjectConstants_h










#define JS_TYPEOBJ_SLOT_TYPE_REPR          0  // Associated Type Representation


#define JS_TYPEOBJ_SCALAR_SLOTS            1  // Maximum number


#define JS_TYPEOBJ_REFERENCE_SLOTS         1  // Maximum number


#define JS_TYPEOBJ_X4_SLOTS                1  // Maximum number


#define JS_TYPEOBJ_SLOT_ARRAY_ELEM_TYPE    1
#define JS_TYPEOBJ_ARRAY_SLOTS             3  // Maximum number


#define JS_TYPEOBJ_SLOT_STRUCT_FIELD_TYPES 1
#define JS_TYPEOBJ_STRUCT_SLOTS            2  // Maximum number











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




#define JS_DATUM_SLOT_TYPE_OBJ 0  // Type object for a given typed object
#define JS_DATUM_SLOT_OWNER    1  // Owner of data (if null, this is owner)
#define JS_DATUM_SLOT_LENGTH   2  // Length of array (see (*) below)
#define JS_DATUM_SLOTS         3  // Number of slots for typed objs






#endif
