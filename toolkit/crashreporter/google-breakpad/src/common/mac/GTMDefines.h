
















 


#include <AvailabilityMacros.h>
#include <TargetConditionals.h>


#ifndef MAC_OS_X_VERSION_10_5
  #define MAC_OS_X_VERSION_10_5 1050
#endif
#ifndef MAC_OS_X_VERSION_10_6
  #define MAC_OS_X_VERSION_10_6 1060
#endif











#ifndef GTM_CONTAINERS_VALIDATION_FAILED_ASSERT
  #define GTM_CONTAINERS_VALIDATION_FAILED_ASSERT 0
#endif




#if !defined(GTM_INLINE)
  #if defined (__GNUC__) && (__GNUC__ == 4)
    #define GTM_INLINE static __inline__ __attribute__((always_inline))
  #else
    #define GTM_INLINE static __inline__
  #endif
#endif



#if !defined (GTM_EXTERN)
  #if defined __cplusplus
    #define GTM_EXTERN extern "C"
  #else
    #define GTM_EXTERN extern
  #endif
#endif



#if !defined (GTM_EXPORT)
  #define GTM_EXPORT __attribute__((visibility("default")))
#endif





















#ifndef _GTMDevLog

#ifdef DEBUG
  #define _GTMDevLog(...) NSLog(__VA_ARGS__)
#else
  #define _GTMDevLog(...) do { } while (0)
#endif

#endif 



@class NSString;
GTM_EXTERN void _GTMUnitTestDevLog(NSString *format, ...);

#ifndef _GTMDevAssert


#if !defined(NS_BLOCK_ASSERTIONS)
  #define _GTMDevAssert(condition, ...)                                       \
    do {                                                                      \
      if (!(condition)) {                                                     \
        [[NSAssertionHandler currentHandler]                                  \
            handleFailureInFunction:[NSString stringWithUTF8String:__PRETTY_FUNCTION__] \
                               file:[NSString stringWithUTF8String:__FILE__]  \
                         lineNumber:__LINE__                                  \
                        description:__VA_ARGS__];                             \
      }                                                                       \
    } while(0)
#else 
  #define _GTMDevAssert(condition, ...) do { } while (0)
#endif 

#endif 











#ifndef _GTMCompileAssert
  
  

  #define _GTMCompileAssertSymbolInner(line, msg) _GTMCOMPILEASSERT ## line ## __ ## msg
  #define _GTMCompileAssertSymbol(line, msg) _GTMCompileAssertSymbolInner(line, msg)
  #define _GTMCompileAssert(test, msg) \
    typedef char _GTMCompileAssertSymbol(__LINE__, msg) [ ((test) ? 1 : -1) ]
#endif 





#ifndef GTM_FOREACH_OBJECT
  #if TARGET_OS_IPHONE || (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5)
    #define GTM_FOREACH_OBJECT(element, collection) \
      for (element in collection)
    #define GTM_FOREACH_KEY(element, collection) \
      for (element in collection)
  #else
    #define GTM_FOREACH_OBJECT(element, collection) \
      for (NSEnumerator * _ ## element ## _enum = [collection objectEnumerator]; \
           (element = [_ ## element ## _enum nextObject]) != nil; )
    #define GTM_FOREACH_KEY(element, collection) \
      for (NSEnumerator * _ ## element ## _enum = [collection keyEnumerator]; \
           (element = [_ ## element ## _enum nextObject]) != nil; )
  #endif
#endif











#if TARGET_OS_IPHONE 
  
  #define GTM_IPHONE_SDK 1
  #if TARGET_IPHONE_SIMULATOR
    #define GTM_IPHONE_SIMULATOR 1
  #else
    #define GTM_IPHONE_DEVICE 1
  #endif  
#else
  
  #define GTM_MACOS_SDK 1
#endif



#ifndef GTM_SUPPORT_GC
  #if GTM_IPHONE_SDK
    
    #define GTM_SUPPORT_GC 0
  #else
    
    
    #if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4
      #define GTM_SUPPORT_GC 0
    #else
      #define GTM_SUPPORT_GC 1
    #endif
  #endif
#endif



#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4
 
  #ifndef NSINTEGER_DEFINED
    #if __LP64__ || NS_BUILD_32_LIKE_64
      typedef long NSInteger;
      typedef unsigned long NSUInteger;
    #else
      typedef int NSInteger;
      typedef unsigned int NSUInteger;
    #endif
    #define NSIntegerMax    LONG_MAX
    #define NSIntegerMin    LONG_MIN
    #define NSUIntegerMax   ULONG_MAX
    #define NSINTEGER_DEFINED 1
  #endif  
  
  #ifndef CGFLOAT_DEFINED
    #if defined(__LP64__) && __LP64__
      
      typedef double CGFloat;
      #define CGFLOAT_MIN DBL_MIN
      #define CGFLOAT_MAX DBL_MAX
      #define CGFLOAT_IS_DOUBLE 1
    #else 
      typedef float CGFloat;
      #define CGFLOAT_MIN FLT_MIN
      #define CGFLOAT_MAX FLT_MAX
      #define CGFLOAT_IS_DOUBLE 0
    #endif 
    #define CGFLOAT_DEFINED 1
  #endif 
#endif  
