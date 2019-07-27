




#include "mozilla/ArrayUtils.h"
#include "nsCOMPtr.h"
#include "nsCSPParser.h"
#include "nsCSPUtils.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIStringBundle.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsUnicharUtils.h"

using namespace mozilla;

#if defined(PR_LOGGING)
static PRLogModuleInfo*
GetCspParserLog()
{
  static PRLogModuleInfo* gCspParserPRLog;
  if (!gCspParserPRLog)
    gCspParserPRLog = PR_NewLogModule("CSPParser");
  return gCspParserPRLog;
}
#endif

#define CSPPARSERLOG(args) PR_LOG(GetCspParserLog(), 4, args)

static const char16_t COLON       = ':';
static const char16_t SEMICOLON   = ';';
static const char16_t SLASH       = '/';
static const char16_t PLUS        = '+';
static const char16_t DASH        = '-';
static const char16_t DOT         = '.';
static const char16_t UNDERLINE   = '_';
static const char16_t WILDCARD    = '*';
static const char16_t WHITESPACE  = ' ';
static const char16_t SINGLEQUOTE = '\'';
static const char16_t OPEN_CURL   = '{';
static const char16_t CLOSE_CURL  = '}';

static uint32_t kSubHostPathCharacterCutoff = 512;

static const char* kHashSourceValidFns [] = { "sha256", "sha384", "sha512" };
static const uint32_t kHashSourceValidFnsLen = 3;



nsCSPTokenizer::nsCSPTokenizer(const char16_t* aStart,
                               const char16_t* aEnd)
  : mCurChar(aStart)
  , mEndChar(aEnd)
{
  CSPPARSERLOG(("nsCSPTokenizer::nsCSPTokenizer"));
}

nsCSPTokenizer::~nsCSPTokenizer()
{
  CSPPARSERLOG(("nsCSPTokenizer::~nsCSPTokenizer"));
}

void
nsCSPTokenizer::generateNextToken()
{
  skipWhiteSpaceAndSemicolon();
  while (!atEnd() &&
         *mCurChar != WHITESPACE &&
         *mCurChar != SEMICOLON) {
    mCurToken.Append(*mCurChar++);
  }
  CSPPARSERLOG(("nsCSPTokenizer::generateNextToken: %s", NS_ConvertUTF16toUTF8(mCurToken).get()));
}

void
nsCSPTokenizer::generateTokens(cspTokens& outTokens)
{
  CSPPARSERLOG(("nsCSPTokenizer::generateTokens"));

  
  nsTArray <nsString> dirAndSrcs;

  while (!atEnd()) {
    generateNextToken();
    dirAndSrcs.AppendElement(mCurToken);
    skipWhiteSpace();
    if (atEnd() || accept(SEMICOLON)) {
      outTokens.AppendElement(dirAndSrcs);
      dirAndSrcs.Clear();
    }
  }
}

void
nsCSPTokenizer::tokenizeCSPPolicy(const nsAString &aPolicyString,
                                  cspTokens& outTokens)
{
  CSPPARSERLOG(("nsCSPTokenizer::tokenizeCSPPolicy"));

  nsCSPTokenizer tokenizer(aPolicyString.BeginReading(),
                           aPolicyString.EndReading());

  tokenizer.generateTokens(outTokens);
}



nsCSPParser::nsCSPParser(cspTokens& aTokens,
                         nsIURI* aSelfURI,
                         uint64_t aInnerWindowID)
 : mTokens(aTokens)
 , mSelfURI(aSelfURI)
 , mInnerWindowID(aInnerWindowID)
{
  CSPPARSERLOG(("nsCSPParser::nsCSPParser"));
}

nsCSPParser::~nsCSPParser()
{
  CSPPARSERLOG(("nsCSPParser::~nsCSPParser"));
}


bool
isCharacterToken(char16_t aSymbol)
{
  return (aSymbol >= 'a' && aSymbol <= 'z') ||
         (aSymbol >= 'A' && aSymbol <= 'Z');
}

bool
isNumberToken(char16_t aSymbol)
{
  return (aSymbol >= '0' && aSymbol <= '9');
}

void
nsCSPParser::resetCurChar(const nsAString& aToken)
{
  mCurChar = aToken.BeginReading();
  mEndChar = aToken.EndReading();
  resetCurValue();
}

void
nsCSPParser::logWarningErrorToConsole(uint32_t aSeverityFlag,
                                      const char* aProperty,
                                      const char16_t* aParams[],
                                      uint32_t aParamsLength)
{
  CSPPARSERLOG(("nsCSPParser::logWarningErrorToConsole: %s", aProperty));

  nsXPIDLString logMsg;
  CSP_GetLocalizedStr(NS_ConvertUTF8toUTF16(aProperty).get(),
                      aParams,
                      aParamsLength,
                      getter_Copies(logMsg));

  CSP_LogMessage(logMsg, EmptyString(), EmptyString(),
                 0, 0, aSeverityFlag,
                 "CSP", mInnerWindowID);
}

bool
nsCSPParser::hostChar()
{
  if (atEnd()) {
    return false;
  }
  return accept(isCharacterToken) ||
         accept(isNumberToken) ||
         accept(DASH);
}


bool
nsCSPParser::schemeChar()
{
  if (atEnd()) {
    return false;
  }
  return accept(isCharacterToken) ||
         accept(isNumberToken) ||
         accept(PLUS) ||
         accept(DASH) ||
         accept(DOT);
}

bool
nsCSPParser::fileAndArguments()
{
  CSPPARSERLOG(("nsCSPParser::fileAndArguments, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  if (accept(DOT) && !accept(isCharacterToken)) {
    return false;
  }

  
  while (!atEnd()) {
    advance();
  }
  return true;
}


bool
nsCSPParser::port()
{
  CSPPARSERLOG(("nsCSPParser::port, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  accept(COLON);

  
  
  
  resetCurValue();

  
  if (accept(WILDCARD)) {
    return true;
  }

  
  if (!accept(isNumberToken)) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "couldntParsePort",
                             params, ArrayLength(params));
    return false;
  }
  
  while (accept(isNumberToken)) {  }
  return true;
}

bool
nsCSPParser::subPath(nsCSPHostSrc* aCspHost)
{
  CSPPARSERLOG(("nsCSPParser::subPath, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  
  
  uint32_t charCounter = 0;

  while (!atEnd() && !peek(DOT)) {
    ++charCounter;
    while (hostChar() || accept(UNDERLINE)) {
      
      ++charCounter;
    }
    if (accept(SLASH)) {
      ++charCounter;
      aCspHost->appendPath(mCurValue);
      
      
      
      resetCurValue();
    }
    if (atEnd()) {
      return true;
    }
    if (charCounter > kSubHostPathCharacterCutoff) {
      return false;
    }
  }
  aCspHost->appendPath(mCurValue);
  resetCurValue();
  return true;
}

bool
nsCSPParser::path(nsCSPHostSrc* aCspHost)
{
  CSPPARSERLOG(("nsCSPParser::path, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  
  
  
  resetCurValue();

  if (!accept(SLASH)) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "couldntParseInvalidSource",
                             params, ArrayLength(params));
    return false;
  }
  if (atEnd()) {
    return true;
  }
  
  
  if (!hostChar()) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "couldntParseInvalidSource",
                             params, ArrayLength(params));
    return false;
  }
  return subPath(aCspHost);
}

bool
nsCSPParser::subHost()
{
  CSPPARSERLOG(("nsCSPParser::subHost, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  
  
  uint32_t charCounter = 0;

  while (!atEnd() && !peek(COLON) && !peek(SLASH)) {
    ++charCounter;
    while (hostChar()) {
      
      ++charCounter;
    }
    if (accept(DOT) && !hostChar()) {
      return false;
    }
    if (charCounter > kSubHostPathCharacterCutoff) {
      return false;
    }
  }
  return true;
}


nsCSPHostSrc*
nsCSPParser::host()
{
  CSPPARSERLOG(("nsCSPParser::host, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  
  
  
  if (accept(WILDCARD)) {
    
    if (atEnd() || peek(COLON)) {
      return new nsCSPHostSrc(mCurValue);
    }
    
    if (!accept(DOT)) {
      const char16_t* params[] = { mCurToken.get() };
      logWarningErrorToConsole(nsIScriptError::warningFlag, "couldntParseInvalidHost",
                               params, ArrayLength(params));
      return nullptr;
    }
  }

  
  if (!hostChar()) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "couldntParseInvalidHost",
                             params, ArrayLength(params));
    return nullptr;
  }

  
  if (!subHost()) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "couldntParseInvalidHost",
                             params, ArrayLength(params));
    return nullptr;
  }

  
  if (CSP_IsQuotelessKeyword(mCurValue)) {
    nsString keyword = mCurValue;
    ToLowerCase(keyword);
    const char16_t* params[] = { mCurToken.get(), keyword.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "hostNameMightBeKeyword",
                             params, ArrayLength(params));
  }

  
  return new nsCSPHostSrc(mCurValue);
}


nsCSPHostSrc*
nsCSPParser::appHost()
{
  CSPPARSERLOG(("nsCSPParser::appHost, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  while (hostChar()) {  }

  
  if (!accept(CLOSE_CURL)) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "couldntParseInvalidSource",
                             params, ArrayLength(params));
    return nullptr;
  }
  return new nsCSPHostSrc(mCurValue);
}


nsCSPBaseSrc*
nsCSPParser::keywordSource()
{
  CSPPARSERLOG(("nsCSPParser::keywordSource, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  
  if (CSP_IsKeyword(mCurToken, CSP_SELF)) {
    return CSP_CreateHostSrcFromURI(mSelfURI);
  }

  if (CSP_IsKeyword(mCurToken, CSP_UNSAFE_INLINE) ||
      CSP_IsKeyword(mCurToken, CSP_UNSAFE_EVAL)) {
    return new nsCSPKeywordSrc(CSP_KeywordToEnum(mCurToken));
  }
  return nullptr;
}


nsCSPHostSrc*
nsCSPParser::hostSource()
{
  CSPPARSERLOG(("nsCSPParser::hostSource, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  if (accept(OPEN_CURL)) {
    
    
    return appHost();
  }

  nsCSPHostSrc* cspHost = host();
  if (!cspHost) {
    
    return nullptr;
  }

  
  
  
  if (peek(COLON)) {
    if (!port()) {
      delete cspHost;
      return nullptr;
    }
    cspHost->setPort(mCurValue);
  }

  if (atEnd()) {
    return cspHost;
  }

  
  
  
  if (!path(cspHost)) {
    
    
    
    delete cspHost;
    return nullptr;
  }

  
  
  
  if (fileAndArguments()) {
    cspHost->setFileAndArguments(mCurValue);
  }

  return cspHost;
}


nsCSPSchemeSrc*
nsCSPParser::schemeSource()
{
  CSPPARSERLOG(("nsCSPParser::schemeSource, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  if (!accept(isCharacterToken)) {
    return nullptr;
  }
  while (schemeChar()) {  }
  nsString scheme = mCurValue;

  
  if (!accept(COLON)) {
    return nullptr;
  }

  
  
  if (peek(isNumberToken) || peek(WILDCARD)) {
    return nullptr;
  }

  return new nsCSPSchemeSrc(scheme);
}


nsCSPNonceSrc*
nsCSPParser::nonceSource()
{
  CSPPARSERLOG(("nsCSPParser::nonceSource, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  if (!StringBeginsWith(mCurToken, NS_ConvertUTF8toUTF16(CSP_EnumToKeyword(CSP_NONCE)),
                        nsASCIICaseInsensitiveStringComparator()) ||
      mCurToken.Last() != SINGLEQUOTE) {
    return nullptr;
  }

  
  const nsAString& expr = Substring(mCurToken, 1, mCurToken.Length() - 2);

  int32_t dashIndex = expr.FindChar(DASH);
  if (dashIndex < 0) {
    return nullptr;
  }
  return new nsCSPNonceSrc(Substring(expr,
                                     dashIndex + 1,
                                     expr.Length() - dashIndex + 1));
}


nsCSPHashSrc*
nsCSPParser::hashSource()
{
  CSPPARSERLOG(("nsCSPParser::hashSource, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));


  
  if (mCurToken.First() != SINGLEQUOTE ||
      mCurToken.Last() != SINGLEQUOTE) {
    return nullptr;
  }

  
  const nsAString& expr = Substring(mCurToken, 1, mCurToken.Length() - 2);

  int32_t dashIndex = expr.FindChar(DASH);
  if (dashIndex < 0) {
    return nullptr;
  }

  nsAutoString algo(Substring(expr, 0, dashIndex));
  nsAutoString hash(Substring(expr, dashIndex + 1, expr.Length() - dashIndex + 1));

  for (uint32_t i = 0; i < kHashSourceValidFnsLen; i++) {
    if (algo.LowerCaseEqualsASCII(kHashSourceValidFns[i])) {
      return new nsCSPHashSrc(algo, hash);
    }
  }
  return nullptr;
}



nsCSPBaseSrc*
nsCSPParser::sourceExpression()
{
  CSPPARSERLOG(("nsCSPParser::sourceExpression, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  if (nsCSPBaseSrc *cspKeyword = keywordSource()) {
    return cspKeyword;
  }

  
  if (nsCSPNonceSrc* cspNonce = nonceSource()) {
    return cspNonce;
  }

  
  if (nsCSPHashSrc* cspHash = hashSource()) {
    return cspHash;
  }

  
  
  if (mCurToken.EqualsASCII("*")) {
    return new nsCSPHostSrc(NS_LITERAL_STRING("*"));
  }

  
  
  
  
  
  resetCurChar(mCurToken);

  
  nsAutoString parsedScheme;
  if (nsCSPSchemeSrc* cspScheme = schemeSource()) {
    
    if (atEnd()) {
      return cspScheme;
    }
    
    
    
    
    cspScheme->toString(parsedScheme);
    parsedScheme.Trim(":", false, true);
    delete cspScheme;

    
    
    if (!accept(SLASH) || !accept(SLASH)) {
      const char16_t* params[] = { mCurToken.get() };
      logWarningErrorToConsole(nsIScriptError::warningFlag, "failedToParseUnrecognizedSource",
                               params, ArrayLength(params));
      return nullptr;
    }
  }

  
  
  
  
  
  resetCurValue();

  
  if (parsedScheme.IsEmpty()) {
    
    
    resetCurChar(mCurToken);
    nsAutoCString scheme;
    mSelfURI->GetScheme(scheme);
    parsedScheme.AssignASCII(scheme.get());
  }

  
  
  if (nsCSPHostSrc *cspHost = hostSource()) {
    
    cspHost->setScheme(parsedScheme);
    return cspHost;
  }
  
  return nullptr;
}



void
nsCSPParser::sourceList(nsTArray<nsCSPBaseSrc*>& outSrcs)
{
  bool isNone = false;

  
  for (uint32_t i = 1; i < mCurDir.Length(); i++) {
    
    
    mCurToken = mCurDir[i];
    resetCurValue();

    CSPPARSERLOG(("nsCSPParser::sourceList, mCurToken: %s, mCurValue: %s",
                 NS_ConvertUTF16toUTF8(mCurToken).get(),
                 NS_ConvertUTF16toUTF8(mCurValue).get()));

    
    
    
    if (CSP_IsKeyword(mCurToken, CSP_NONE)) {
      isNone = true;
      continue;
    }
    
    nsCSPBaseSrc* src = sourceExpression();
    if (src) {
      outSrcs.AppendElement(src);
    }
  }

  
  if (isNone) {
    
    if (outSrcs.Length() == 0) {
      nsCSPKeywordSrc *keyword = new nsCSPKeywordSrc(CSP_NONE);
      outSrcs.AppendElement(keyword);
    }
    
    else {
      NS_ConvertUTF8toUTF16 unicodeNone(CSP_EnumToKeyword(CSP_NONE));
      const char16_t* params[] = { unicodeNone.get() };
      logWarningErrorToConsole(nsIScriptError::warningFlag, "ignoringUnknownOption",
                               params, ArrayLength(params));
    }
  }
}

void
nsCSPParser::reportURIList(nsTArray<nsCSPBaseSrc*>& outSrcs)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv;

  
  for (uint32_t i = 1; i < mCurDir.Length(); i++) {
    mCurToken = mCurDir[i];

    CSPPARSERLOG(("nsCSPParser::reportURIList, mCurToken: %s, mCurValue: %s",
                 NS_ConvertUTF16toUTF8(mCurToken).get(),
                 NS_ConvertUTF16toUTF8(mCurValue).get()));

    rv = NS_NewURI(getter_AddRefs(uri), mCurToken, "", mSelfURI);

    
    if (NS_FAILED(rv)) {
      const char16_t* params[] = { mCurToken.get() };
      logWarningErrorToConsole(nsIScriptError::warningFlag, "couldNotParseReportURI",
                               params, ArrayLength(params));
      continue;
    }

    
    nsCSPReportURI* reportURI = new nsCSPReportURI(uri);
    outSrcs.AppendElement(reportURI);
  }
}


void
nsCSPParser::directiveValue(nsTArray<nsCSPBaseSrc*>& outSrcs)
{
  CSPPARSERLOG(("nsCSPParser::directiveValue"));

  
  
  
  if (CSP_IsDirective(mCurDir[0], CSP_REPORT_URI)) {
    reportURIList(outSrcs);
    return;
  }
  
  sourceList(outSrcs);
}


nsCSPDirective*
nsCSPParser::directiveName()
{
  CSPPARSERLOG(("nsCSPParser::directiveName, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  if (!CSP_IsValidDirective(mCurToken)) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "couldNotProcessUnknownDirective",
                             params, ArrayLength(params));
    return nullptr;
  }

  
  
  
  
  if (CSP_IsDirective(mCurToken, CSP_REFLECTED_XSS)) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "notSupportingDirective",
                             params, ArrayLength(params));
    return nullptr;
  }

  
  
  if (mPolicy->directiveExists(CSP_DirectiveToEnum(mCurToken))) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "duplicateDirective",
                             params, ArrayLength(params));
    return nullptr;
  }
  return new nsCSPDirective(CSP_DirectiveToEnum(mCurToken));
}


void
nsCSPParser::directive()
{
  
  
  mCurToken = mCurDir[0];

  CSPPARSERLOG(("nsCSPParser::directive, mCurToken: %s, mCurValue: %s",
               NS_ConvertUTF16toUTF8(mCurToken).get(),
               NS_ConvertUTF16toUTF8(mCurValue).get()));

  
  
  if (mCurDir.Length() < 1) {
    const char16_t* params[] = { NS_LITERAL_STRING("directive missing").get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "failedToParseUnrecognizedSource",
                             params, ArrayLength(params));
    return;
  }

  if (mCurDir.Length() < 2) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "failedToParseUnrecognizedSource",
                             params, ArrayLength(params));
    return;
  }

  
  nsCSPDirective* cspDir = directiveName();
  if (!cspDir) {
    
    return;
  }

  
  nsTArray<nsCSPBaseSrc*> srcs;
  directiveValue(srcs);

  
  if (srcs.Length() == 0) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "failedToParseUnrecognizedSource",
                             params, ArrayLength(params));
    delete cspDir;
    return;
  }

  
  cspDir->addSrcs(srcs);
  mPolicy->addDirective(cspDir);
}


nsCSPPolicy*
nsCSPParser::policy()
{
  CSPPARSERLOG(("nsCSPParser::policy"));

  mPolicy = new nsCSPPolicy();
  for (uint32_t i = 0; i < mTokens.Length(); i++) {
    
    
    
    mCurDir = mTokens[i];
    directive();
  }
  return mPolicy;
}

nsCSPPolicy*
nsCSPParser::parseContentSecurityPolicy(const nsAString& aPolicyString,
                                        nsIURI *aSelfURI,
                                        bool aReportOnly,
                                        uint64_t aInnerWindowID)
{
#ifdef PR_LOGGING
  {
    CSPPARSERLOG(("nsCSPParser::parseContentSecurityPolicy, policy: %s",
                 NS_ConvertUTF16toUTF8(aPolicyString).get()));
    nsAutoCString spec;
    aSelfURI->GetSpec(spec);
    CSPPARSERLOG(("nsCSPParser::parseContentSecurityPolicy, selfURI: %s", spec.get()));
    CSPPARSERLOG(("nsCSPParser::parseContentSecurityPolicy, reportOnly: %s",
                 (aReportOnly ? "true" : "false")));
  }
#endif

  NS_ASSERTION(aSelfURI, "Can not parseContentSecurityPolicy without aSelfURI");

  
  
  
  

  nsTArray< nsTArray<nsString> > tokens;
  nsCSPTokenizer::tokenizeCSPPolicy(aPolicyString, tokens);

  nsCSPParser parser(tokens, aSelfURI, aInnerWindowID);

  
  nsCSPPolicy* policy = parser.policy();

  
  if (aReportOnly) {
    policy->setReportOnlyFlag(true);
    if (!policy->directiveExists(CSP_REPORT_URI)) {
      nsAutoCString prePath;
      nsresult rv = aSelfURI->GetPrePath(prePath);
      NS_ENSURE_SUCCESS(rv, policy);
      NS_ConvertUTF8toUTF16 unicodePrePath(prePath);
      const char16_t* params[] = { unicodePrePath.get() };
      parser.logWarningErrorToConsole(nsIScriptError::warningFlag, "reportURInotInReportOnlyHeader",
                                      params, ArrayLength(params));
    }
  }

  if (policy->getNumDirectives() == 0) {
    
    
    delete policy;
    return nullptr;
  }

#ifdef PR_LOGGING
  {
    nsString parsedPolicy;
    policy->toString(parsedPolicy);
    CSPPARSERLOG(("nsCSPParser::parseContentSecurityPolicy, parsedPolicy: %s",
                 NS_ConvertUTF16toUTF8(parsedPolicy).get()));
  }
#endif

  return policy;
}
