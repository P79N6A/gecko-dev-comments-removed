




#ifndef nsError_h__
#define nsError_h__

#include "mozilla/Likely.h"
#include "mozilla/TypedEnum.h"

#ifndef nscore_h___
#include "nscore.h"  
#endif
























#define NS_ERROR_MODULE_XPCOM      1
#define NS_ERROR_MODULE_BASE       2
#define NS_ERROR_MODULE_GFX        3
#define NS_ERROR_MODULE_WIDGET     4
#define NS_ERROR_MODULE_CALENDAR   5
#define NS_ERROR_MODULE_NETWORK    6
#define NS_ERROR_MODULE_PLUGINS    7
#define NS_ERROR_MODULE_LAYOUT     8
#define NS_ERROR_MODULE_HTMLPARSER 9
#define NS_ERROR_MODULE_RDF        10
#define NS_ERROR_MODULE_UCONV      11
#define NS_ERROR_MODULE_REG        12
#define NS_ERROR_MODULE_FILES      13
#define NS_ERROR_MODULE_DOM        14
#define NS_ERROR_MODULE_IMGLIB     15
#define NS_ERROR_MODULE_MAILNEWS   16
#define NS_ERROR_MODULE_EDITOR     17
#define NS_ERROR_MODULE_XPCONNECT  18
#define NS_ERROR_MODULE_PROFILE    19
#define NS_ERROR_MODULE_LDAP       20
#define NS_ERROR_MODULE_SECURITY   21
#define NS_ERROR_MODULE_DOM_XPATH  22

#define NS_ERROR_MODULE_URILOADER  24
#define NS_ERROR_MODULE_CONTENT    25
#define NS_ERROR_MODULE_PYXPCOM    26
#define NS_ERROR_MODULE_XSLT       27
#define NS_ERROR_MODULE_IPC        28
#define NS_ERROR_MODULE_SVG        29
#define NS_ERROR_MODULE_STORAGE    30
#define NS_ERROR_MODULE_SCHEMA     31
#define NS_ERROR_MODULE_DOM_FILE   32
#define NS_ERROR_MODULE_DOM_INDEXEDDB 33
#define NS_ERROR_MODULE_DOM_FILEHANDLE 34
#define NS_ERROR_MODULE_SIGNED_JAR 35








#define NS_ERROR_MODULE_GENERAL    51





#define NS_ERROR_SEVERITY_SUCCESS       0
#define NS_ERROR_SEVERITY_ERROR         1






#define NS_ERROR_MODULE_BASE_OFFSET 0x45


#define SUCCESS_OR_FAILURE(sev, module, code) \
  ((uint32_t)(sev) << 31) | \
  ((uint32_t)(module + NS_ERROR_MODULE_BASE_OFFSET) << 16) | \
  (uint32_t)(code)
#define SUCCESS(code) \
  SUCCESS_OR_FAILURE(NS_ERROR_SEVERITY_SUCCESS, MODULE, code)
#define FAILURE(code) \
  SUCCESS_OR_FAILURE(NS_ERROR_SEVERITY_ERROR, MODULE, code)
















#if defined(__cplusplus)
#if defined(MOZ_HAVE_CXX11_STRONG_ENUMS)
  typedef enum class tag_nsresult : uint32_t
#elif defined(MOZ_HAVE_CXX11_ENUM_TYPE)
  
  typedef enum tag_nsresult : uint32_t
#else
  
  typedef enum tag_nsresult
#endif
  {
    #undef ERROR
    #define ERROR(key, val) key = val
    #include "ErrorList.h"
    #undef ERROR
  } nsresult;

#if defined(MOZ_HAVE_CXX11_STRONG_ENUMS)
  

  #include "ErrorListCxxDefines.h"
#endif
#else 
  


  typedef uint32_t nsresult;
  #include "ErrorListCDefines.h"
#endif 

#undef SUCCESS_OR_FAILURE
#undef SUCCESS
#undef FAILURE






#ifdef __cplusplus
inline uint32_t NS_FAILED_impl(nsresult _nsresult) {
  return static_cast<uint32_t>(_nsresult) & 0x80000000;
}
#define NS_FAILED(_nsresult)    ((bool)MOZ_UNLIKELY(NS_FAILED_impl(_nsresult)))
#define NS_SUCCEEDED(_nsresult) ((bool)MOZ_LIKELY(!NS_FAILED_impl(_nsresult)))
#else
#define NS_FAILED_impl(_nsresult) ((_nsresult) & 0x80000000)
#define NS_FAILED(_nsresult)    (MOZ_UNLIKELY(NS_FAILED_impl(_nsresult)))
#define NS_SUCCEEDED(_nsresult) (MOZ_LIKELY(!NS_FAILED_impl(_nsresult)))
#endif





#define NS_ERROR_GENERATE(sev, module, code) \
    (nsresult)(((uint32_t)(sev) << 31) | \
               ((uint32_t)(module + NS_ERROR_MODULE_BASE_OFFSET) << 16) | \
               ((uint32_t)(code)))

#define NS_ERROR_GENERATE_SUCCESS(module, code) \
  NS_ERROR_GENERATE(NS_ERROR_SEVERITY_SUCCESS, module, code)

#define NS_ERROR_GENERATE_FAILURE(module, code) \
  NS_ERROR_GENERATE(NS_ERROR_SEVERITY_ERROR, module, code)

 







extern nsresult
NS_ErrorAccordingToNSPR();






#ifdef __cplusplus
inline uint16_t NS_ERROR_GET_CODE(nsresult err) {
  return uint32_t(err) & 0xffff;
}
inline uint16_t NS_ERROR_GET_MODULE(nsresult err) {
  return ((uint32_t(err) >> 16) - NS_ERROR_MODULE_BASE_OFFSET) & 0x1fff;
}
inline bool NS_ERROR_GET_SEVERITY(nsresult err) {
  return uint32_t(err) >> 31;
}
#else
#define NS_ERROR_GET_CODE(err)     ((err) & 0xffff)
#define NS_ERROR_GET_MODULE(err)   ((((err) >> 16) - NS_ERROR_MODULE_BASE_OFFSET) & 0x1fff)
#define NS_ERROR_GET_SEVERITY(err) (((err) >> 31) & 0x1)
#endif


#ifdef _MSC_VER
#pragma warning(disable: 4251) /* 'nsCOMPtr<class nsIInputStream>' needs to have dll-interface to be used by clients of class 'nsInputStream' */
#pragma warning(disable: 4275) /* non dll-interface class 'nsISupports' used as base for dll-interface class 'nsIRDFNode' */
#endif

#endif
