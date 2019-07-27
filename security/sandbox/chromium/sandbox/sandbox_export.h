



#ifndef SANDBOX_SANDBOX_EXPORT_H_
#define SANDBOX_SANDBOX_EXPORT_H_

#if defined(WIN32)
#error "sandbox_export.h does not support WIN32."
#endif

#if defined(COMPONENT_BUILD)

#if defined(SANDBOX_IMPLEMENTATION)
#define SANDBOX_EXPORT __attribute__((visibility("default")))
#define SANDBOX_EXPORT_PRIVATE __attribute__((visibility("default")))
#else
#define SANDBOX_EXPORT
#define SANDBOX_EXPORT_PRIVATE
#endif  

#else  

#define SANDBOX_EXPORT
#define SANDBOX_EXPORT_PRIVATE

#endif  

#endif  
