







#ifndef builtin_SelfHostingDefines_h
#define builtin_SelfHostingDefines_h


#define TO_INT32(x) ((x) | 0)
#define TO_UINT32(x) ((x) >>> 0)
#define IS_UINT32(x) ((x) >>> 0 === (x))


#ifdef DEBUG
#define assert(b, info) if (!(b)) AssertionFailed(info)
#else
#define assert(b, info)
#endif


#define ARRAY_PUSH(ARRAY, ELEMENT) \
  callFunction(std_Array_push, ARRAY, ELEMENT);
#define ARRAY_SLICE(ARRAY, ELEMENT) \
  callFunction(std_Array_slice, ARRAY, ELEMENT);

#endif
