







































#ifndef mozilla_mozalloc_oom_h
#define mozilla_mozalloc_oom_h

#if defined(MOZALLOC_EXPORT)


#elif defined(XP_WIN) || (defined(XP_OS2) && defined(__declspec))
#  define MOZALLOC_EXPORT __declspec(dllimport)
#elif defined(HAVE_VISIBILITY_ATTRIBUTE)


#  define MOZALLOC_EXPORT __attribute__ ((visibility ("default")))
#else
#  define MOZALLOC_EXPORT
#endif









MOZALLOC_EXPORT void mozalloc_handle_oom();






#endif 
