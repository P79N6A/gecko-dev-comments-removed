












































#ifdef MOZ_USE_STDCALL
#define ATTRIBUTE_STDCALL __attribute__ ((__stdcall__))
#else
#define ATTRIBUTE_STDCALL
#endif

#ifdef MOZ_NEED_LEADING_UNDERSCORE
#define SYMBOL_UNDERSCORE "_"
#else
#define SYMBOL_UNDERSCORE
#endif


#define ATTRIBUTE_USED __attribute__ ((__used__))
