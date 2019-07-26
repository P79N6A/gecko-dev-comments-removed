



























#ifndef mozilla_pkix__enumclass_h
#define mozilla_pkix__enumclass_h

#if defined(_MSC_VER) && (_MSC_VER < 1700)




#define MOZILLA_PKIX_ENUM_CLASS  __pragma(warning(disable: 4480)) enum
#else
#define MOZILLA_PKIX_ENUM_CLASS enum class
#endif

#endif 
