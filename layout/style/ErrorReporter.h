






#ifndef mozilla_css_ErrorReporter_h_
#define mozilla_css_ErrorReporter_h_


#define CSS_REPORT_PARSE_ERRORS

#include "nsString.h"

struct nsCSSToken;
class nsCSSStyleSheet;
class nsCSSScanner;
class nsIURI;

namespace mozilla {
namespace css {

class Loader;



class NS_STACK_CLASS ErrorReporter {
public:
  ErrorReporter(const nsCSSScanner &aScanner,
                const nsCSSStyleSheet *aSheet,
                const Loader *aLoader,
                nsIURI *aURI);
  ~ErrorReporter();

  static void ReleaseGlobals();

  void OutputError();
  void ClearError();

  
  void ReportUnexpected(const char *aMessage);
  
  void ReportUnexpectedEOF(const char *aLookingFor);
  
  void ReportUnexpectedEOF(PRUnichar aLookingFor);

  
  
  void ReportUnexpectedToken(const char *aMessage, const nsCSSToken &aToken);

  
  template<uint32_t N>
  void ReportUnexpectedParams(const char* aMessage,
                              const PRUnichar* (&aParams)[N])
  {
    MOZ_STATIC_ASSERT(N > 0, "use ReportUnexpected instead");
    ReportUnexpectedParams(aMessage, aParams, N);
  }

  
  
  template<uint32_t N>
  void ReportUnexpectedTokenParams(const char* aMessage,
                                   const nsCSSToken &aToken,
                                   const PRUnichar* (&aParams)[N])
  {
    MOZ_STATIC_ASSERT(N > 1, "use ReportUnexpectedToken instead");
    ReportUnexpectedTokenParams(aMessage, aToken, aParams, N);
  }

private:
  void ReportUnexpectedParams(const char *aMessage,
                              const PRUnichar **aParams,
                              uint32_t aParamsLength);
  void ReportUnexpectedTokenParams(const char *aMessage,
                                   const nsCSSToken &aToken,
                                   const PRUnichar **aParams,
                                   uint32_t aParamsLength);

  void AddToError(const nsAString &aErrorText);

#ifdef CSS_REPORT_PARSE_ERRORS
  nsAutoString mError;
  nsString mFileName;
  const nsCSSScanner *mScanner;
  const nsCSSStyleSheet *mSheet;
  const Loader *mLoader;
  nsIURI *mURI;
  uint64_t mInnerWindowID;
  uint32_t mErrorLineNumber;
  uint32_t mErrorColNumber;
#endif
};

#ifndef CSS_REPORT_PARSE_ERRORS
inline ErrorReporter::ErrorReporter(const nsCSSScanner&,
                                    const nsCSSStyleSheet*,
                                    const Loader*,
                                    nsIURI*) {}
inline ErrorReporter::~ErrorReporter() {}

inline void ErrorReporter::ReleaseGlobals() {}

inline void ErrorReporter::OutputError() {}
inline void ErrorReporter::ClearError() {}

inline void ErrorReporter::ReportUnexpected(const char *) {}
inline void ErrorReporter::ReportUnexpectedParams(const char *,
                                                  const PRUnichar **,
                                                  uint32_t) {}
inline void ErrorReporter::ReportUnexpectedEOF(const char *) {}
inline void ErrorReporter::ReportUnexpectedEOF(PRUnichar) {}
inline void ErrorReporter::ReportUnexpectedToken(const char *,
                                                 const nsCSSToken &) {}
inline void ErrorReporter::ReportUnexpectedTokenParams(const char *,
                                                       const nsCSSToken &,
                                                       const PRUnichar **,
                                                       uint32_t) {}

inline void ErrorReporter::AddToError(const nsAString &) {}
#endif

} 
} 

#endif 
