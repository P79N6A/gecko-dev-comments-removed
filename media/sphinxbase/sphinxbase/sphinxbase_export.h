#ifndef __SPHINXBASE_EXPORT_H__
#define __SPHINXBASE_EXPORT_H__


#if (defined(_WIN32) || defined(_WIN32_WCE)) && !defined(_WIN32_WP) && !defined(__MINGW32__) && !defined(__CYGWIN__) && !defined(__WINSCW__) && !defined(__SYMBIAN32__)
#if defined(SPHINXBASE_EXPORTS) 
#define SPHINXBASE_EXPORT __declspec(dllexport)
#else
#define SPHINXBASE_EXPORT __declspec(dllimport)
#endif
#else 
#define SPHINXBASE_EXPORT
#endif

#endif 
