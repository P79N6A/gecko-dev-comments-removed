







#ifndef builtin_SelfHostingDefines_h
#define builtin_SelfHostingDefines_h


#define TO_INT32(x) ((x) | 0)
#define TO_UINT32(x) ((x) >>> 0)
#define IS_UINT32(x) ((x) >>> 0 === (x))


#define ARRAY_PUSH(ARRAY, ELEMENT) \
  callFunction(std_Array_push, ARRAY, ELEMENT);
#define ARRAY_SLICE(ARRAY, ELEMENT) \
  callFunction(std_Array_slice, ARRAY, ELEMENT);


#define ATTR_ENUMERABLE         0x01
#define ATTR_CONFIGURABLE       0x02
#define ATTR_WRITABLE           0x04

#define ATTR_NONENUMERABLE      0x08
#define ATTR_NONCONFIGURABLE    0x10
#define ATTR_NONWRITABLE        0x20


#define WEAKSET_MAP_SLOT 0

#endif
