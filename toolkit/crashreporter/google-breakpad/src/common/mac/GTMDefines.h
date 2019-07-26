



















#include <AvailabilityMacros.h>
#include <TargetConditionals.h>

#ifdef __OBJC__
#include <Foundation/NSObjCRuntime.h>
#endif  

#if TARGET_OS_IPHONE
#include <Availability.h>
#endif  


#ifndef MAC_OS_X_VERSION_10_5
  #define MAC_OS_X_VERSION_10_5 1050
#endif
#ifndef MAC_OS_X_VERSION_10_6
  #define MAC_OS_X_VERSION_10_6 1060
#endif
#ifndef MAC_OS_X_VERSION_10_7
  #define MAC_OS_X_VERSION_10_7 1070
#endif


#ifndef __IPHONE_3_0
  #define __IPHONE_3_0 30000
#endif
#ifndef __IPHONE_3_1
  #define __IPHONE_3_1 30100
#endif
#ifndef __IPHONE_3_2
  #define __IPHONE_3_2 30200
#endif
#ifndef __IPHONE_4_0
  #define __IPHONE_4_0 40000
#endif
#ifndef __IPHONE_4_3
  #define __IPHONE_4_3 40300
#endif
#ifndef __IPHONE_5_0
  #define __IPHONE_5_0 50000
#endif











#ifndef GTM_CONTAINERS_VALIDATION_FAILED_ASSERT
  #define GTM_CONTAINERS_VALIDATION_FAILED_ASSERT 0
#endif




#if !defined(GTM_INLINE)
  #if (defined (__GNUC__) && (__GNUC__ == 4)) || defined (__clang__)
    #define GTM_INLINE static __inline__ __attribute__((always_inline))
  #else
    #define GTM_INLINE static __inline__
  #endif
#endif



#if !defined (GTM_EXTERN)
  #if defined __cplusplus
    #define GTM_EXTERN extern "C"
    #define GTM_EXTERN_C_BEGIN extern "C" {
    #define GTM_EXTERN_C_END }
  #else
    #define GTM_EXTERN extern
    #define GTM_EXTERN_C_BEGIN
    #define GTM_EXTERN_C_END
  #endif
#endif



#if !defined (GTM_EXPORT)
  #define GTM_EXPORT __attribute__((visibility("default")))
#endif



#if !defined (GTM_UNUSED)
#define GTM_UNUSED(x) ((void)(x))
#endif





















#ifndef _GTMDevLog

#ifdef DEBUG
  #define _GTMDevLog(...) NSLog(__VA_ARGS__)
#else
  #define _GTMDevLog(...) do { } while (0)
#endif

#endif 

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









#if TARGET_OS_IPHONE 
  
  #define GTM_IPHONE_SDK 1
  #if TARGET_IPHONE_SIMULATOR
    #define GTM_IPHONE_SIMULATOR 1
  #else
    #define GTM_IPHONE_DEVICE 1
  #endif  
  
  
  
  #ifndef GTM_IPHONE_USE_SENTEST
    #define GTM_IPHONE_USE_SENTEST 0
  #endif
#else
  
  #define GTM_MACOS_SDK 1
#endif


#if GTM_MACOS_SDK
#define GTM_AVAILABLE_ONLY_ON_IPHONE UNAVAILABLE_ATTRIBUTE
#define GTM_AVAILABLE_ONLY_ON_MACOS
#else
#define GTM_AVAILABLE_ONLY_ON_IPHONE
#define GTM_AVAILABLE_ONLY_ON_MACOS UNAVAILABLE_ATTRIBUTE
#endif



#ifndef GTM_SUPPORT_GC
  #if GTM_IPHONE_SDK
    
    #define GTM_SUPPORT_GC 0
  #else
    
    
    #if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
      #define GTM_SUPPORT_GC 0
    #else
      #define GTM_SUPPORT_GC 1
    #endif
  #endif
#endif



#if !(MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
 
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



#ifndef __has_feature      
  #define __has_feature(x) 0 // Compatibility with non-clang compilers.
#endif

#ifndef NS_RETURNS_RETAINED
  #if __has_feature(attribute_ns_returns_retained)
    #define NS_RETURNS_RETAINED __attribute__((ns_returns_retained))
  #else
    #define NS_RETURNS_RETAINED
  #endif
#endif

#ifndef NS_RETURNS_NOT_RETAINED
  #if __has_feature(attribute_ns_returns_not_retained)
    #define NS_RETURNS_NOT_RETAINED __attribute__((ns_returns_not_retained))
  #else
    #define NS_RETURNS_NOT_RETAINED
  #endif
#endif

#ifndef CF_RETURNS_RETAINED
  #if __has_feature(attribute_cf_returns_retained)
    #define CF_RETURNS_RETAINED __attribute__((cf_returns_retained))
  #else
    #define CF_RETURNS_RETAINED
  #endif
#endif

#ifndef CF_RETURNS_NOT_RETAINED
  #if __has_feature(attribute_cf_returns_not_retained)
    #define CF_RETURNS_NOT_RETAINED __attribute__((cf_returns_not_retained))
  #else
    #define CF_RETURNS_NOT_RETAINED
  #endif
#endif

#ifndef NS_CONSUMED
  #if __has_feature(attribute_ns_consumed)
    #define NS_CONSUMED __attribute__((ns_consumed))
  #else
    #define NS_CONSUMED
  #endif
#endif

#ifndef CF_CONSUMED
  #if __has_feature(attribute_cf_consumed)
    #define CF_CONSUMED __attribute__((cf_consumed))
  #else
    #define CF_CONSUMED
  #endif
#endif

#ifndef NS_CONSUMES_SELF
  #if __has_feature(attribute_ns_consumes_self)
    #define NS_CONSUMES_SELF __attribute__((ns_consumes_self))
  #else
    #define NS_CONSUMES_SELF
  #endif
#endif


#ifndef NS_FORMAT_ARGUMENT
  #define NS_FORMAT_ARGUMENT(A)
#endif


#ifndef NS_FORMAT_FUNCTION
  #define NS_FORMAT_FUNCTION(F,A)
#endif


#ifndef CF_FORMAT_ARGUMENT
  #define CF_FORMAT_ARGUMENT(A)
#endif


#ifndef CF_FORMAT_FUNCTION
  #define CF_FORMAT_FUNCTION(F,A)
#endif

#ifndef GTM_NONNULL
  #define GTM_NONNULL(x) __attribute__((nonnull(x)))
#endif


#ifndef GTMInvalidateInitializer
  #if __has_feature(objc_arc)
    #define GTMInvalidateInitializer() \
      do { \
        [self class]; /* Avoid warning of dead store to |self|. */ \
        _GTMDevAssert(NO, @"Invalid initializer."); \
        return nil; \
      } while (0)
  #else
    #define GTMInvalidateInitializer() \
      do { \
        [self release]; \
        _GTMDevAssert(NO, @"Invalid initializer."); \
        return nil; \
      } while (0)
  #endif
#endif

#ifdef __OBJC__



@class NSString;
GTM_EXTERN void _GTMUnitTestDevLog(NSString *format, ...) NS_FORMAT_FUNCTION(1, 2);




#if !defined (GTM_NSSTRINGIFY)
  #define GTM_NSSTRINGIFY_INNER(x) @#x
  #define GTM_NSSTRINGIFY(x) GTM_NSSTRINGIFY_INNER(x)
#endif





#ifndef GTM_FOREACH_OBJECT
  #if TARGET_OS_IPHONE || !(MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5)
    #define GTM_FOREACH_ENUMEREE(element, enumeration) \
      for (element in enumeration)
    #define GTM_FOREACH_OBJECT(element, collection) \
      for (element in collection)
    #define GTM_FOREACH_KEY(element, collection) \
      for (element in collection)
  #else
    #define GTM_FOREACH_ENUMEREE(element, enumeration) \
      for (NSEnumerator *_ ## element ## _enum = enumeration; \
           (element = [_ ## element ## _enum nextObject]) != nil; )
    #define GTM_FOREACH_OBJECT(element, collection) \
      GTM_FOREACH_ENUMEREE(element, [collection objectEnumerator])
    #define GTM_FOREACH_KEY(element, collection) \
      GTM_FOREACH_ENUMEREE(element, [collection keyEnumerator])
  #endif
#endif





#if !defined(GTM_10_6_PROTOCOLS_DEFINED) && !(MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6)
#define GTM_10_6_PROTOCOLS_DEFINED 1
@protocol NSConnectionDelegate
@end
@protocol NSAnimationDelegate
@end
@protocol NSImageDelegate
@end
@protocol NSTabViewDelegate
@end
#endif  






#ifndef GTM_SEL_STRING
  #ifdef DEBUG
    #define GTM_SEL_STRING(selName) NSStringFromSelector(@selector(selName))
  #else
    #define GTM_SEL_STRING(selName) @#selName
  #endif  
#endif  

#endif  
