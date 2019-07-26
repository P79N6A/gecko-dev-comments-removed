





#ifndef mozilla_RefCountType_h
#define mozilla_RefCountType_h

















#ifdef XP_WIN
typedef unsigned long MozRefCountType;
#else
typedef uint32_t MozRefCountType;
#endif

#endif
