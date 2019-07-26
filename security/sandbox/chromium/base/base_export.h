



#ifndef BASE_BASE_EXPORT_H_
#define BASE_BASE_EXPORT_H_

#if defined(COMPONENT_BUILD)
#if defined(WIN32)

#if defined(BASE_IMPLEMENTATION)
#define BASE_EXPORT __declspec(dllexport)
#define BASE_EXPORT_PRIVATE __declspec(dllexport)
#else
#define BASE_EXPORT __declspec(dllimport)
#define BASE_EXPORT_PRIVATE __declspec(dllimport)
#endif  

#else  
#if defined(BASE_IMPLEMENTATION)
#define BASE_EXPORT __attribute__((visibility("default")))
#define BASE_EXPORT_PRIVATE __attribute__((visibility("default")))
#else
#define BASE_EXPORT
#define BASE_EXPORT_PRIVATE
#endif  
#endif

#else  
#define BASE_EXPORT
#define BASE_EXPORT_PRIVATE
#endif

#endif  
