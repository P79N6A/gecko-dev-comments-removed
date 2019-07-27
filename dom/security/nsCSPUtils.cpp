




#include "nsCSPUtils.h"
#include "nsDebug.h"
#include "nsIConsoleService.h"
#include "nsICryptoHash.h"
#include "nsIScriptError.h"
#include "nsIServiceManager.h"
#include "nsIStringBundle.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"

#if defined(PR_LOGGING)
static PRLogModuleInfo*
GetCspUtilsLog()
{
  static PRLogModuleInfo* gCspUtilsPRLog;
  if (!gCspUtilsPRLog)
    gCspUtilsPRLog = PR_NewLogModule("CSPUtils");
  return gCspUtilsPRLog;
}
#endif

#define CSPUTILSLOG(args) PR_LOG(GetCspUtilsLog(), 4, args)

void
CSP_GetLocalizedStr(const char16_t* aName,
                    const char16_t** aParams,
                    uint32_t aLength,
                    char16_t** outResult)
{
  nsCOMPtr<nsIStringBundle> keyStringBundle;
  nsCOMPtr<nsIStringBundleService> stringBundleService =
    mozilla::services::GetStringBundleService();

  NS_ASSERTION(stringBundleService, "String bundle service must be present!");
  stringBundleService->CreateBundle("chrome://global/locale/security/csp.properties",
                                      getter_AddRefs(keyStringBundle));

  NS_ASSERTION(keyStringBundle, "Key string bundle must be available!");

  if (!keyStringBundle) {
    return;
  }
  keyStringBundle->FormatStringFromName(aName, aParams, aLength, outResult);
}

void
CSP_LogStrMessage(const nsAString& aMsg)
{
  nsCOMPtr<nsIConsoleService> console(do_GetService("@mozilla.org/consoleservice;1"));

  if (!console) {
    return;
  }
  nsString msg = PromiseFlatString(aMsg);
  console->LogStringMessage(msg.get());
}

void
CSP_LogMessage(const nsAString& aMessage,
               const nsAString& aSourceName,
               const nsAString& aSourceLine,
               uint32_t aLineNumber,
               uint32_t aColumnNumber,
               uint32_t aFlags,
               const char *aCategory,
               uint32_t aInnerWindowID)
{
  nsCOMPtr<nsIConsoleService> console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));

  nsCOMPtr<nsIScriptError> error(do_CreateInstance(NS_SCRIPTERROR_CONTRACTID));

  if (!console || !error) {
    return;
  }

  
  nsString cspMsg;
  cspMsg.Append(NS_LITERAL_STRING("Content Security Policy: "));
  cspMsg.Append(aMessage);

  nsresult rv;
  if (aInnerWindowID > 0) {
    nsCString catStr;
    catStr.AssignASCII(aCategory);
    rv = error->InitWithWindowID(cspMsg, aSourceName,
                                 aSourceLine, aLineNumber,
                                 aColumnNumber, aFlags,
                                 catStr, aInnerWindowID);
  }
  else {
    rv = error->Init(cspMsg, aSourceName,
                     aSourceLine, aLineNumber,
                     aColumnNumber, aFlags,
                     aCategory);
  }
  if (NS_FAILED(rv)) {
    return;
  }
  console->LogMessage(error);
}




void
CSP_LogLocalizedStr(const char16_t* aName,
                    const char16_t** aParams,
                    uint32_t aLength,
                    const nsAString& aSourceName,
                    const nsAString& aSourceLine,
                    uint32_t aLineNumber,
                    uint32_t aColumnNumber,
                    uint32_t aFlags,
                    const char* aCategory,
                    uint32_t aInnerWindowID)
{
  nsXPIDLString logMsg;
  CSP_GetLocalizedStr(aName, aParams, aLength, getter_Copies(logMsg));
  CSP_LogMessage(logMsg, aSourceName, aSourceLine,
                 aLineNumber, aColumnNumber, aFlags,
                 aCategory, aInnerWindowID);
}


CSPDirective
CSP_ContentTypeToDirective(nsContentPolicyType aType)
{
  switch (aType) {
    case nsIContentPolicy::TYPE_IMAGE:
    case nsIContentPolicy::TYPE_IMAGESET:
      return nsIContentSecurityPolicy::IMG_SRC_DIRECTIVE;

    
    case nsIContentPolicy::TYPE_XSLT:
    case nsIContentPolicy::TYPE_SCRIPT:
      return nsIContentSecurityPolicy::SCRIPT_SRC_DIRECTIVE;

    case nsIContentPolicy::TYPE_STYLESHEET:
      return nsIContentSecurityPolicy::STYLE_SRC_DIRECTIVE;

    case nsIContentPolicy::TYPE_FONT:
      return nsIContentSecurityPolicy::FONT_SRC_DIRECTIVE;

    case nsIContentPolicy::TYPE_MEDIA:
      return nsIContentSecurityPolicy::MEDIA_SRC_DIRECTIVE;

    case nsIContentPolicy::TYPE_SUBDOCUMENT:
      return nsIContentSecurityPolicy::FRAME_SRC_DIRECTIVE;

    case nsIContentPolicy::TYPE_WEBSOCKET:
    case nsIContentPolicy::TYPE_XMLHTTPREQUEST:
    case nsIContentPolicy::TYPE_BEACON:
    case nsIContentPolicy::TYPE_FETCH:
      return nsIContentSecurityPolicy::CONNECT_SRC_DIRECTIVE;

    case nsIContentPolicy::TYPE_OBJECT:
    case nsIContentPolicy::TYPE_OBJECT_SUBREQUEST:
      return nsIContentSecurityPolicy::OBJECT_SRC_DIRECTIVE;

    case nsIContentPolicy::TYPE_XBL:
    case nsIContentPolicy::TYPE_PING:
    case nsIContentPolicy::TYPE_DTD:
    case nsIContentPolicy::TYPE_OTHER:
      return nsIContentSecurityPolicy::DEFAULT_SRC_DIRECTIVE;

    
    
    case nsIContentPolicy::TYPE_DOCUMENT:
    
    case nsIContentPolicy::TYPE_CSP_REPORT:
      return nsIContentSecurityPolicy::NO_DIRECTIVE;

    
    default:
      MOZ_ASSERT(false, "Can not map nsContentPolicyType to CSPDirective");
  }
  return nsIContentSecurityPolicy::DEFAULT_SRC_DIRECTIVE;
}

nsCSPHostSrc*
CSP_CreateHostSrcFromURI(nsIURI* aURI)
{
  
  nsCString host;
  aURI->GetHost(host);
  nsCSPHostSrc *hostsrc = new nsCSPHostSrc(NS_ConvertUTF8toUTF16(host));

  
  nsCString scheme;
  aURI->GetScheme(scheme);
  hostsrc->setScheme(NS_ConvertUTF8toUTF16(scheme));

  int32_t port;
  aURI->GetPort(&port);
  
  if (port > 0) {
    nsAutoString portStr;
    portStr.AppendInt(port);
    hostsrc->setPort(portStr);
  }
  return hostsrc;
}

bool
CSP_IsValidDirective(const nsAString& aDir)
{
  uint32_t numDirs = (sizeof(CSPStrDirectives) / sizeof(CSPStrDirectives[0]));

  for (uint32_t i = 0; i < numDirs; i++) {
    if (aDir.LowerCaseEqualsASCII(CSPStrDirectives[i])) {
      return true;
    }
  }
  return false;
}
bool
CSP_IsDirective(const nsAString& aValue, CSPDirective aDir)
{
  return aValue.LowerCaseEqualsASCII(CSP_CSPDirectiveToString(aDir));
}

bool
CSP_IsKeyword(const nsAString& aValue, enum CSPKeyword aKey)
{
  return aValue.LowerCaseEqualsASCII(CSP_EnumToKeyword(aKey));
}

bool
CSP_IsQuotelessKeyword(const nsAString& aKey)
{
  nsString lowerKey = PromiseFlatString(aKey);
  ToLowerCase(lowerKey);

  static_assert(CSP_LAST_KEYWORD_VALUE ==
                (sizeof(CSPStrKeywords) / sizeof(CSPStrKeywords[0])),
                "CSP_LAST_KEYWORD_VALUE does not match length of CSPStrKeywords");

  nsAutoString keyword;
  for (uint32_t i = 0; i < CSP_LAST_KEYWORD_VALUE; i++) {
    
    keyword.AssignASCII(CSPStrKeywords[i] + 1);
    keyword.Trim("'", false, true);
    if (lowerKey.Equals(keyword)) {
      return true;
    }
  }
  return false;
}



nsCSPBaseSrc::nsCSPBaseSrc()
{
}

nsCSPBaseSrc::~nsCSPBaseSrc()
{
}




bool
nsCSPBaseSrc::permits(nsIURI* aUri, const nsAString& aNonce, bool aWasRedirected) const
{
#ifdef PR_LOGGING
  {
    nsAutoCString spec;
    aUri->GetSpec(spec);
    CSPUTILSLOG(("nsCSPBaseSrc::permits, aUri: %s", spec.get()));
  }
#endif
  return false;
}




bool
nsCSPBaseSrc::allows(enum CSPKeyword aKeyword, const nsAString& aHashOrNonce) const
{
  CSPUTILSLOG(("nsCSPBaseSrc::allows, aKeyWord: %s, a HashOrNonce: %s",
              aKeyword == CSP_HASH ? "hash" : CSP_EnumToKeyword(aKeyword),
              NS_ConvertUTF16toUTF8(aHashOrNonce).get()));
  return false;
}



nsCSPSchemeSrc::nsCSPSchemeSrc(const nsAString& aScheme)
  : mScheme(aScheme)
{
  ToLowerCase(mScheme);
}

nsCSPSchemeSrc::~nsCSPSchemeSrc()
{
}

bool
nsCSPSchemeSrc::permits(nsIURI* aUri, const nsAString& aNonce, bool aWasRedirected) const
{
#ifdef PR_LOGGING
  {
    nsAutoCString spec;
    aUri->GetSpec(spec);
    CSPUTILSLOG(("nsCSPSchemeSrc::permits, aUri: %s", spec.get()));
  }
#endif

  NS_ASSERTION((!mScheme.EqualsASCII("")), "scheme can not be the empty string");
  nsAutoCString scheme;
  nsresult rv = aUri->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, false);
  return mScheme.EqualsASCII(scheme.get());
}

void
nsCSPSchemeSrc::toString(nsAString& outStr) const
{
  outStr.Append(mScheme);
  outStr.AppendASCII(":");
}



nsCSPHostSrc::nsCSPHostSrc(const nsAString& aHost)
  : mHost(aHost)
  , mAllowHttps(false)
{
  ToLowerCase(mHost);
}

nsCSPHostSrc::~nsCSPHostSrc()
{
}

bool
nsCSPHostSrc::permits(nsIURI* aUri, const nsAString& aNonce, bool aWasRedirected) const
{
#ifdef PR_LOGGING
  {
    nsAutoCString spec;
    aUri->GetSpec(spec);
    CSPUTILSLOG(("nsCSPHostSrc::permits, aUri: %s", spec.get()));
  }
#endif

  
  

  
  nsAutoCString scheme;
  nsresult rv = aUri->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, false);
  if (!mScheme.IsEmpty() &&
      !mScheme.EqualsASCII(scheme.get())) {

    
    
    
    bool isHttpsScheme =
      (NS_SUCCEEDED(aUri->SchemeIs("https", &isHttpsScheme)) && isHttpsScheme);

    if (!(isHttpsScheme && mAllowHttps)) {
      return false;
    }
  }

  
  
  NS_ASSERTION((!mHost.IsEmpty()), "host can not be the empty string");

  
  if (mHost.EqualsASCII("*")) {
    
    
    
    
    
    bool isBlobScheme =
      (NS_SUCCEEDED(aUri->SchemeIs("blob", &isBlobScheme)) && isBlobScheme);
    bool isDataScheme =
      (NS_SUCCEEDED(aUri->SchemeIs("data", &isDataScheme)) && isDataScheme);
    bool isFileScheme =
      (NS_SUCCEEDED(aUri->SchemeIs("filesystem", &isFileScheme)) && isFileScheme);

    if (isBlobScheme || isDataScheme || isFileScheme) {
      return false;
    }
    return true;
  }

  
  
  nsAutoCString uriHost;
  rv = aUri->GetHost(uriHost);
  NS_ENSURE_SUCCESS(rv, false);

  
  if (mHost.First() == '*') {
    NS_ASSERTION(mHost[1] == '.', "Second character needs to be '.' whenever host starts with '*'");

    
    
    nsString wildCardHost = mHost;
    wildCardHost = Substring(wildCardHost, 1, wildCardHost.Length() - 1);
    if (!StringEndsWith(NS_ConvertUTF8toUTF16(uriHost), wildCardHost)) {
      return false;
    }
  }
  
  else if (!mHost.Equals(NS_ConvertUTF8toUTF16(uriHost))) {
    return false;
  }

  
  
  
  if (!aWasRedirected && !mPath.IsEmpty()) {
    
    
    
    nsCOMPtr<nsIURL> url = do_QueryInterface(aUri);
    if (!url) {
      NS_ASSERTION(false, "can't QI into nsIURI");
      return false;
    }
    nsAutoCString uriPath;
    rv = url->GetFilePath(uriPath);
    NS_ENSURE_SUCCESS(rv, false);
    
    
    
    if (mPath.Last() == '/') {
      if (!StringBeginsWith(NS_ConvertUTF8toUTF16(uriPath), mPath)) {
        return false;
      }
    }
    
    
    else {
      if (!mPath.Equals(NS_ConvertUTF8toUTF16(uriPath))) {
        return false;
      }
    }
  }

  
  if (mPort.EqualsASCII("*")) {
    return true;
  }

  
  
  int32_t uriPort;
  rv = aUri->GetPort(&uriPort);
  NS_ENSURE_SUCCESS(rv, false);
  uriPort = (uriPort > 0) ? uriPort : NS_GetDefaultPort(scheme.get());

  
  if (mPort.IsEmpty()) {
    int32_t port = NS_GetDefaultPort(NS_ConvertUTF16toUTF8(mScheme).get());
    if (port != uriPort) {
      
      
      
      
      if (!(uriPort == NS_GetDefaultPort("https") && mAllowHttps)) {
        return false;
      }
    }
  }
  
  else {
    nsString portStr;
    portStr.AppendInt(uriPort);
    if (!mPort.Equals(portStr)) {
      return false;
    }
  }

  
  return true;
}

void
nsCSPHostSrc::toString(nsAString& outStr) const
{
  
  if (mHost.EqualsASCII("*") &&
      mScheme.IsEmpty() &&
      mPort.IsEmpty()) {
    outStr.Append(mHost);
    return;
  }

  
  outStr.Append(mScheme);

  
  outStr.AppendASCII("://");
  outStr.Append(mHost);

  
  if (!mPort.IsEmpty()) {
    outStr.AppendASCII(":");
    outStr.Append(mPort);
  }

  
  outStr.Append(mPath);
}

void
nsCSPHostSrc::setScheme(const nsAString& aScheme, bool aAllowHttps)
{
  mScheme = aScheme;
  ToLowerCase(mScheme);
  mAllowHttps = aAllowHttps;
}

void
nsCSPHostSrc::setPort(const nsAString& aPort)
{
  mPort = aPort;
}

void
nsCSPHostSrc::appendPath(const nsAString& aPath)
{
  mPath.Append(aPath);
}



nsCSPKeywordSrc::nsCSPKeywordSrc(enum CSPKeyword aKeyword)
 : mKeyword(aKeyword)
 , mInvalidated(false)
{
  NS_ASSERTION((aKeyword != CSP_SELF),
               "'self' should have been replaced in the parser");
}

nsCSPKeywordSrc::~nsCSPKeywordSrc()
{
}

bool
nsCSPKeywordSrc::allows(enum CSPKeyword aKeyword, const nsAString& aHashOrNonce) const
{
  CSPUTILSLOG(("nsCSPKeywordSrc::allows, aKeyWord: %s, aHashOrNonce: %s, mInvalidated: %s",
              CSP_EnumToKeyword(aKeyword),
              NS_ConvertUTF16toUTF8(aHashOrNonce).get(),
              mInvalidated ? "yes" : "false"));
  
  if (mInvalidated) {
    NS_ASSERTION(mKeyword == CSP_UNSAFE_INLINE,
                 "should only invalidate unsafe-inline within script-src");
    return false;
  }
  return mKeyword == aKeyword;
}

void
nsCSPKeywordSrc::toString(nsAString& outStr) const
{
  outStr.AppendASCII(CSP_EnumToKeyword(mKeyword));
}

void
nsCSPKeywordSrc::invalidate()
{
  mInvalidated = true;
  NS_ASSERTION(mInvalidated == CSP_UNSAFE_INLINE,
               "invalidate 'unsafe-inline' only within script-src");
}



nsCSPNonceSrc::nsCSPNonceSrc(const nsAString& aNonce)
  : mNonce(aNonce)
{
}

nsCSPNonceSrc::~nsCSPNonceSrc()
{
}

bool
nsCSPNonceSrc::permits(nsIURI* aUri, const nsAString& aNonce, bool aWasRedirected) const
{
#ifdef PR_LOGGING
  {
    nsAutoCString spec;
    aUri->GetSpec(spec);
    CSPUTILSLOG(("nsCSPNonceSrc::permits, aUri: %s, aNonce: %s",
                spec.get(), NS_ConvertUTF16toUTF8(aNonce).get()));
  }
#endif

  return mNonce.Equals(aNonce);
}

bool
nsCSPNonceSrc::allows(enum CSPKeyword aKeyword, const nsAString& aHashOrNonce) const
{
  CSPUTILSLOG(("nsCSPNonceSrc::allows, aKeyWord: %s, a HashOrNonce: %s",
              CSP_EnumToKeyword(aKeyword), NS_ConvertUTF16toUTF8(aHashOrNonce).get()));

  if (aKeyword != CSP_NONCE) {
    return false;
  }
  return mNonce.Equals(aHashOrNonce);
}

void
nsCSPNonceSrc::toString(nsAString& outStr) const
{
  outStr.AppendASCII(CSP_EnumToKeyword(CSP_NONCE));
  outStr.Append(mNonce);
  outStr.AppendASCII("'");
}



nsCSPHashSrc::nsCSPHashSrc(const nsAString& aAlgo, const nsAString& aHash)
 : mAlgorithm(aAlgo)
 , mHash(aHash)
{
  
  ToLowerCase(mAlgorithm);
}

nsCSPHashSrc::~nsCSPHashSrc()
{
}

bool
nsCSPHashSrc::allows(enum CSPKeyword aKeyword, const nsAString& aHashOrNonce) const
{
  CSPUTILSLOG(("nsCSPHashSrc::allows, aKeyWord: %s, a HashOrNonce: %s",
              CSP_EnumToKeyword(aKeyword), NS_ConvertUTF16toUTF8(aHashOrNonce).get()));

  if (aKeyword != CSP_HASH) {
    return false;
  }

  
  NS_ConvertUTF16toUTF8 utf8_hash(aHashOrNonce);

  nsresult rv;
  nsCOMPtr<nsICryptoHash> hasher;
  hasher = do_CreateInstance("@mozilla.org/security/hash;1", &rv);
  NS_ENSURE_SUCCESS(rv, false);

  rv = hasher->InitWithString(NS_ConvertUTF16toUTF8(mAlgorithm));
  NS_ENSURE_SUCCESS(rv, false);

  rv = hasher->Update((uint8_t *)utf8_hash.get(), utf8_hash.Length());
  NS_ENSURE_SUCCESS(rv, false);

  nsAutoCString hash;
  rv = hasher->Finish(true, hash);
  NS_ENSURE_SUCCESS(rv, false);

  
  
  
  hash.StripChars("\r\n");
  return NS_ConvertUTF16toUTF8(mHash).Equals(hash);
}

void
nsCSPHashSrc::toString(nsAString& outStr) const
{
  outStr.AppendASCII("'");
  outStr.Append(mAlgorithm);
  outStr.AppendASCII("-");
  outStr.Append(mHash);
  outStr.AppendASCII("'");
}



nsCSPReportURI::nsCSPReportURI(nsIURI *aURI)
  :mReportURI(aURI)
{
}

nsCSPReportURI::~nsCSPReportURI()
{
}

void
nsCSPReportURI::toString(nsAString& outStr) const
{
  nsAutoCString spec;
  nsresult rv = mReportURI->GetSpec(spec);
  if (NS_FAILED(rv)) {
    return;
  }
  outStr.AppendASCII(spec.get());
}



nsCSPDirective::nsCSPDirective(CSPDirective aDirective)
{
  mDirective = aDirective;
}

nsCSPDirective::~nsCSPDirective()
{
  for (uint32_t i = 0; i < mSrcs.Length(); i++) {
    delete mSrcs[i];
  }
}

bool
nsCSPDirective::permits(nsIURI* aUri, const nsAString& aNonce, bool aWasRedirected) const
{
#ifdef PR_LOGGING
  {
    nsAutoCString spec;
    aUri->GetSpec(spec);
    CSPUTILSLOG(("nsCSPDirective::permits, aUri: %s", spec.get()));
  }
#endif

  for (uint32_t i = 0; i < mSrcs.Length(); i++) {
    if (mSrcs[i]->permits(aUri, aNonce, aWasRedirected)) {
      return true;
    }
  }
  return false;
}

bool
nsCSPDirective::permits(nsIURI* aUri) const
{
  nsString dummyNonce;
  return permits(aUri, dummyNonce, false);
}

bool
nsCSPDirective::allows(enum CSPKeyword aKeyword, const nsAString& aHashOrNonce) const
{
  CSPUTILSLOG(("nsCSPDirective::allows, aKeyWord: %s, a HashOrNonce: %s",
              CSP_EnumToKeyword(aKeyword), NS_ConvertUTF16toUTF8(aHashOrNonce).get()));

  for (uint32_t i = 0; i < mSrcs.Length(); i++) {
    if (mSrcs[i]->allows(aKeyword, aHashOrNonce)) {
      return true;
    }
  }
  return false;
}

void
nsCSPDirective::toString(nsAString& outStr) const
{
  
  outStr.AppendASCII(CSP_CSPDirectiveToString(mDirective));
  outStr.AppendASCII(" ");

  
  uint32_t length = mSrcs.Length();
  for (uint32_t i = 0; i < length; i++) {
    mSrcs[i]->toString(outStr);
    if (i != (length - 1)) {
      outStr.AppendASCII(" ");
    }
  }
}

bool
nsCSPDirective::restrictsContentType(nsContentPolicyType aContentType) const
{
  
  if (isDefaultDirective()) {
    return false;
  }
  return mDirective == CSP_ContentTypeToDirective(aContentType);
}

void
nsCSPDirective::getReportURIs(nsTArray<nsString> &outReportURIs) const
{
  NS_ASSERTION((mDirective == nsIContentSecurityPolicy::REPORT_URI_DIRECTIVE), "not a report-uri directive");

  
  nsString tmpReportURI;
  for (uint32_t i = 0; i < mSrcs.Length(); i++) {
    tmpReportURI.Truncate();
    mSrcs[i]->toString(tmpReportURI);
    outReportURIs.AppendElement(tmpReportURI);
  }
}



nsCSPPolicy::nsCSPPolicy()
  : mReportOnly(false)
{
  CSPUTILSLOG(("nsCSPPolicy::nsCSPPolicy"));
}

nsCSPPolicy::~nsCSPPolicy()
{
  CSPUTILSLOG(("nsCSPPolicy::~nsCSPPolicy"));

  for (uint32_t i = 0; i < mDirectives.Length(); i++) {
    delete mDirectives[i];
  }
}

bool
nsCSPPolicy::permits(CSPDirective aDir,
                     nsIURI* aUri,
                     bool aSpecific) const
{
  nsString outp;
  return this->permits(aDir, aUri, EmptyString(), false, aSpecific, outp);
}

bool
nsCSPPolicy::permits(CSPDirective aDir,
                     nsIURI* aUri,
                     const nsAString& aNonce,
                     bool aWasRedirected,
                     bool aSpecific,
                     nsAString& outViolatedDirective) const
{
#ifdef PR_LOGGING
  {
    nsAutoCString spec;
    aUri->GetSpec(spec);
    CSPUTILSLOG(("nsCSPPolicy::permits, aUri: %s, aDir: %d, aSpecific: %s",
                 spec.get(), aDir, aSpecific ? "true" : "false"));
  }
#endif

  NS_ASSERTION(aUri, "permits needs an uri to perform the check!");

  nsCSPDirective* defaultDir = nullptr;

  
  
  for (uint32_t i = 0; i < mDirectives.Length(); i++) {
    if (mDirectives[i]->equals(aDir)) {
      if (!mDirectives[i]->permits(aUri, aNonce, aWasRedirected)) {
        mDirectives[i]->toString(outViolatedDirective);
        return false;
      }
      return true;
    }
    if (mDirectives[i]->isDefaultDirective()) {
      defaultDir = mDirectives[i];
    }
  }

  
  
  if (!aSpecific && defaultDir) {
    if (!defaultDir->permits(aUri, aNonce, aWasRedirected)) {
      defaultDir->toString(outViolatedDirective);
      return false;
    }
    return true;
  }

  
  
  return true;
}

bool
nsCSPPolicy::allows(nsContentPolicyType aContentType,
                    enum CSPKeyword aKeyword,
                    const nsAString& aHashOrNonce) const
{
  CSPUTILSLOG(("nsCSPPolicy::allows, aKeyWord: %s, a HashOrNonce: %s",
              CSP_EnumToKeyword(aKeyword), NS_ConvertUTF16toUTF8(aHashOrNonce).get()));

  nsCSPDirective* defaultDir = nullptr;

  
  for (uint32_t i = 0; i < mDirectives.Length(); i++) {
    if (mDirectives[i]->restrictsContentType(aContentType)) {
      if (mDirectives[i]->allows(aKeyword, aHashOrNonce)) {
        return true;
      }
      return false;
    }
    if (mDirectives[i]->isDefaultDirective()) {
      defaultDir = mDirectives[i];
    }
  }

  
  if (aKeyword == CSP_NONCE || aKeyword == CSP_HASH) {
    return false;
  }

  
  
  if (defaultDir) {
    return defaultDir->allows(aKeyword, aHashOrNonce);
  }

  
  
  
  
  
  return true;
}

bool
nsCSPPolicy::allows(nsContentPolicyType aContentType,
                    enum CSPKeyword aKeyword) const
{
  return allows(aContentType, aKeyword, NS_LITERAL_STRING(""));
}

void
nsCSPPolicy::toString(nsAString& outStr) const
{
  uint32_t length = mDirectives.Length();
  for (uint32_t i = 0; i < length; ++i) {

    if (mDirectives[i]->equals(nsIContentSecurityPolicy::REFERRER_DIRECTIVE)) {
      outStr.AppendASCII(CSP_CSPDirectiveToString(nsIContentSecurityPolicy::REFERRER_DIRECTIVE));
      outStr.AppendASCII(" ");
      outStr.Append(mReferrerPolicy);
    } else {
      mDirectives[i]->toString(outStr);
    }
    if (i != (length - 1)) {
      outStr.AppendASCII("; ");
    }
  }
}

bool
nsCSPPolicy::hasDirective(CSPDirective aDir) const
{
  for (uint32_t i = 0; i < mDirectives.Length(); i++) {
    if (mDirectives[i]->equals(aDir)) {
      return true;
    }
  }
  return false;
}







void
nsCSPPolicy::getDirectiveStringForContentType(nsContentPolicyType aContentType,
                                              nsAString& outDirective) const
{
  nsCSPDirective* defaultDir = nullptr;
  for (uint32_t i = 0; i < mDirectives.Length(); i++) {
    if (mDirectives[i]->restrictsContentType(aContentType)) {
      mDirectives[i]->toString(outDirective);
      return;
    }
    if (mDirectives[i]->isDefaultDirective()) {
      defaultDir = mDirectives[i];
    }
  }
  
  
  if (defaultDir) {
    defaultDir->toString(outDirective);
    return;
  }
  NS_ASSERTION(false, "Can not query directive string for contentType!");
  outDirective.AppendASCII("couldNotQueryViolatedDirective");
}

void
nsCSPPolicy::getDirectiveAsString(CSPDirective aDir, nsAString& outDirective) const
{
  for (uint32_t i = 0; i < mDirectives.Length(); i++) {
    if (mDirectives[i]->equals(aDir)) {
      mDirectives[i]->toString(outDirective);
      return;
    }
  }
}

void
nsCSPPolicy::getReportURIs(nsTArray<nsString>& outReportURIs) const
{
  for (uint32_t i = 0; i < mDirectives.Length(); i++) {
    if (mDirectives[i]->equals(nsIContentSecurityPolicy::REPORT_URI_DIRECTIVE)) {
      mDirectives[i]->getReportURIs(outReportURIs);
      return;
    }
  }
}
