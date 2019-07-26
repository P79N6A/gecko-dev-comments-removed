






#include "mozilla/css/ErrorReporter.h"
#include "mozilla/css/Loader.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "nsCSSScanner.h"
#include "nsCSSStyleSheet.h"
#include "nsIConsoleService.h"
#include "nsIDocument.h"
#include "nsIFactory.h"
#include "nsIScriptError.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsThreadUtils.h"

#ifdef CSS_REPORT_PARSE_ERRORS

using mozilla::Preferences;
namespace services = mozilla::services;

namespace {
class ShortTermURISpecCache : public nsRunnable {
public:
  ShortTermURISpecCache() : mPending(false) {}

  nsString const& GetSpec(nsIURI* aURI) {
    if (mURI != aURI) {
      mURI = aURI;

      nsAutoCString cSpec;
      mURI->GetSpec(cSpec);
      CopyUTF8toUTF16(cSpec, mSpec);
    }
    return mSpec;
  }

  bool IsInUse() const { return mURI != nullptr; }
  bool IsPending() const { return mPending; }
  void SetPending() { mPending = true; }

  
  NS_IMETHOD Run() {
    mURI = nullptr;
    mSpec.Truncate();
    mPending = false;
    return NS_OK;
  }

private:
  nsCOMPtr<nsIURI> mURI;
  nsString mSpec;
  bool mPending;
};
}

static bool sReportErrors;
static nsIConsoleService *sConsoleService;
static nsIFactory *sScriptErrorFactory;
static nsIStringBundle *sStringBundle;
static ShortTermURISpecCache *sSpecCache;

#define CSS_ERRORS_PREF "layout.css.report_errors"

static bool
InitGlobals()
{
  NS_ABORT_IF_FALSE(!sConsoleService && !sScriptErrorFactory && !sStringBundle,
                    "should not have been called");

  if (NS_FAILED(Preferences::AddBoolVarCache(&sReportErrors, CSS_ERRORS_PREF,
                                             true))) {
    return false;
  }

  nsCOMPtr<nsIConsoleService> cs = do_GetService(NS_CONSOLESERVICE_CONTRACTID);
  if (!cs) {
    return false;
  }

  nsCOMPtr<nsIFactory> sf = do_GetClassObject(NS_SCRIPTERROR_CONTRACTID);
  if (!sf) {
    return false;
  }

  nsCOMPtr<nsIStringBundleService> sbs = services::GetStringBundleService();
  if (!sbs) {
    return false;
  }

  nsCOMPtr<nsIStringBundle> sb;
  nsresult rv = sbs->CreateBundle("chrome://global/locale/css.properties",
                                  getter_AddRefs(sb));
  if (NS_FAILED(rv) || !sb) {
    return false;
  }

  sConsoleService = cs.forget().get();
  sScriptErrorFactory = sf.forget().get();
  sStringBundle = sb.forget().get();

  return true;
}

static inline bool
ShouldReportErrors()
{
  if (!sConsoleService) {
    if (!InitGlobals()) {
      return false;
    }
  }
  return sReportErrors;
}

namespace mozilla {
namespace css {

 void
ErrorReporter::ReleaseGlobals()
{
  NS_IF_RELEASE(sConsoleService);
  NS_IF_RELEASE(sScriptErrorFactory);
  NS_IF_RELEASE(sStringBundle);
  NS_IF_RELEASE(sSpecCache);
}

ErrorReporter::ErrorReporter(const nsCSSScanner& aScanner,
                             const nsCSSStyleSheet* aSheet,
                             const Loader* aLoader,
                             nsIURI* aURI)
  : mScanner(&aScanner), mSheet(aSheet), mLoader(aLoader), mURI(aURI),
    mInnerWindowID(0), mErrorLineNumber(0), mErrorColNumber(0)
{
}

ErrorReporter::~ErrorReporter()
{
  
  
  
  if (sSpecCache && sSpecCache->IsInUse() && !sSpecCache->IsPending()) {
    if (NS_FAILED(NS_DispatchToCurrentThread(sSpecCache))) {
      
      sSpecCache->Run();
    } else {
      sSpecCache->SetPending();
    }
  }
}

void
ErrorReporter::OutputError()
{
  if (mError.IsEmpty()) {
    return;
  }
  if (!ShouldReportErrors()) {
    ClearError();
    return;
  }

  if (mInnerWindowID == 0 && (mSheet || mLoader)) {
    if (mSheet) {
      mInnerWindowID = mSheet->FindOwningWindowInnerID();
    }
    if (mInnerWindowID == 0 && mLoader) {
      nsIDocument* doc = mLoader->GetDocument();
      if (doc) {
        mInnerWindowID = doc->InnerWindowID();
      }
    }
    
    mSheet = nullptr;
    mLoader = nullptr;
  }

  if (mFileName.IsEmpty()) {
    if (mURI) {
      if (!sSpecCache) {
        sSpecCache = new ShortTermURISpecCache;
        NS_ADDREF(sSpecCache);
      }
      mFileName = sSpecCache->GetSpec(mURI);
      mURI = nullptr;
    } else {
      mFileName.AssignLiteral("from DOM");
    }
  }

  nsresult rv;
  nsCOMPtr<nsIScriptError> errorObject =
    do_CreateInstance(sScriptErrorFactory, &rv);

  if (NS_SUCCEEDED(rv)) {
    rv = errorObject->InitWithWindowID(mError,
                                       mFileName,
                                       EmptyString(),
                                       mErrorLineNumber,
                                       mErrorColNumber,
                                       nsIScriptError::warningFlag,
                                       "CSS Parser",
                                       mInnerWindowID);
    if (NS_SUCCEEDED(rv)) {
      sConsoleService->LogMessage(errorObject);
    }
  }

  ClearError();
}

void
ErrorReporter::ClearError()
{
  mError.Truncate();
}

void
ErrorReporter::AddToError(const nsString &aErrorText)
{
  if (!ShouldReportErrors()) return;

  if (mError.IsEmpty()) {
    mErrorLineNumber = mScanner->GetLineNumber();
    mErrorColNumber = mScanner->GetColumnNumber();
    mError = aErrorText;
  } else {
    mError.AppendLiteral("  ");
    mError.Append(aErrorText);
  }
}

void
ErrorReporter::ReportUnexpected(const char *aMessage)
{
  if (!ShouldReportErrors()) return;

  nsAutoString str;
  sStringBundle->GetStringFromName(NS_ConvertASCIItoUTF16(aMessage).get(),
                                   getter_Copies(str));
  AddToError(str);
}

void
ErrorReporter::ReportUnexpected(const char *aMessage,
                                const nsString &aParam)
{
  if (!ShouldReportErrors()) return;

  const PRUnichar *params[1] = { aParam.get() };
  nsAutoString str;
  sStringBundle->FormatStringFromName(NS_ConvertASCIItoUTF16(aMessage).get(),
                                      params, ArrayLength(params),
                                      getter_Copies(str));
  AddToError(str);
}

void
ErrorReporter::ReportUnexpected(const char *aMessage,
                                const nsCSSToken &aToken)
{
  if (!ShouldReportErrors()) return;

  nsAutoString tokenString;
  aToken.AppendToString(tokenString);
  const PRUnichar *params[1] = { tokenString.get() };

  nsAutoString str;
  sStringBundle->FormatStringFromName(NS_ConvertASCIItoUTF16(aMessage).get(),
                                      params, ArrayLength(params),
                                      getter_Copies(str));
  AddToError(str);
}

void
ErrorReporter::ReportUnexpected(const char *aMessage,
                                const nsCSSToken &aToken,
                                PRUnichar aChar)
{
  if (!ShouldReportErrors()) return;

  nsAutoString tokenString;
  aToken.AppendToString(tokenString);
  const PRUnichar charStr[2] = { aChar, 0 };
  const PRUnichar *params[2] = { tokenString.get(), charStr };

  nsAutoString str;
  sStringBundle->FormatStringFromName(NS_ConvertASCIItoUTF16(aMessage).get(),
                                      params, ArrayLength(params),
                                      getter_Copies(str));
  AddToError(str);
}

void
ErrorReporter::ReportUnexpectedEOF(const char *aMessage)
{
  if (!ShouldReportErrors()) return;

  nsAutoString innerStr;
  sStringBundle->GetStringFromName(NS_ConvertASCIItoUTF16(aMessage).get(),
                                   getter_Copies(innerStr));
  const PRUnichar *params[1] = { innerStr.get() };

  nsAutoString str;
  sStringBundle->FormatStringFromName(NS_LITERAL_STRING("PEUnexpEOF2").get(),
                                      params, ArrayLength(params),
                                      getter_Copies(str));
  AddToError(str);
}

void
ErrorReporter::ReportUnexpectedEOF(PRUnichar aExpected)
{
  if (!ShouldReportErrors()) return;

  const PRUnichar expectedStr[] = {
    PRUnichar('\''), aExpected, PRUnichar('\''), PRUnichar(0)
  };
  const PRUnichar *params[1] = { expectedStr };

  nsAutoString str;
  sStringBundle->FormatStringFromName(NS_LITERAL_STRING("PEUnexpEOF2").get(),
                                      params, ArrayLength(params),
                                      getter_Copies(str));
  AddToError(str);
}

} 
} 

#endif
