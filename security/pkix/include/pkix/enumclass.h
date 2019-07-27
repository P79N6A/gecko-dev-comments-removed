























#ifndef mozilla_pkix__enumclass_h
#define mozilla_pkix__enumclass_h

#if defined(_MSC_VER) && (_MSC_VER < 1700)




#define MOZILLA_PKIX_ENUM_CLASS  __pragma(warning(suppress: 4480)) enum
#elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ < 407)



#define MOZILLA_PKIX_ENUM_CLASS enum
#else
#define MOZILLA_PKIX_ENUM_CLASS enum class
#define MOZILLA_PKIX_ENUM_CLASS_REALLY_IS_ENUM_CLASS
#endif

#endif 
