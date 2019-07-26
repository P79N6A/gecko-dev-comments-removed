























#ifndef mozilla_pkix__nullptr_h
#define mozilla_pkix__nullptr_h


#if defined(__GNUC__) && !defined(__clang__)
#if __GNUC__ * 100 + __GNUC_MINOR__ < 406
#define nullptr __null
#endif
#endif

#endif 
