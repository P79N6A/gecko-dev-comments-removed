






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
  
  void ReportUnexpected(const char *aMessage, const nsString& aParam);
  
  void ReportUnexpected(const char *aMessage, const nsCSSToken& aToken);
  
  void ReportUnexpected(const char *aMessage, const nsCSSToken& aToken,
                        PRUnichar aChar);

  
  
  
  void ReportUnexpectedEOF(const char *aExpected);
  void ReportUnexpectedEOF(PRUnichar aExpected);

private:
  void AddToError(const nsString &aErrorText);

#ifdef CSS_REPORT_PARSE_ERRORS
  nsAutoString mError;
  nsString mErrorLine;
  nsString mFileName;
  const nsCSSScanner *mScanner;
  const nsCSSStyleSheet *mSheet;
  const Loader *mLoader;
  nsIURI *mURI;
  uint64_t mInnerWindowID;
  uint32_t mErrorLineNumber;
  uint32_t mPrevErrorLineNumber;
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
inline void ErrorReporter::ReportUnexpected(const char *, const nsString &) {}
inline void ErrorReporter::ReportUnexpected(const char *, const nsCSSToken &) {}
inline void ErrorReporter::ReportUnexpected(const char *, const nsCSSToken &,
                                            PRUnichar) {}

inline void ErrorReporter::ReportUnexpectedEOF(const char *) {}
inline void ErrorReporter::ReportUnexpectedEOF(PRUnichar) {}

inline void ErrorReporter::AddToError(const nsString &) {}
#endif

} 
} 

#endif 
