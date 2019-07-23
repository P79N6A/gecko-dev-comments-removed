












































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






























#if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define ATTRIBUTE_USED __attribute__ ((__used__))
#else
#define ATTRIBUTE_USED
#endif

