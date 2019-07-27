




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
#include "mozilla/net/ReferrerPolicy.h"

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

static const char16_t COLON        = ':';
static const char16_t SEMICOLON    = ';';
static const char16_t SLASH        = '/';
static const char16_t PLUS         = '+';
static const char16_t DASH         = '-';
static const char16_t DOT          = '.';
static const char16_t UNDERLINE    = '_';
static const char16_t TILDE        = '~';
static const char16_t WILDCARD     = '*';
static const char16_t WHITESPACE   = ' ';
static const char16_t SINGLEQUOTE  = '\'';
static const char16_t OPEN_CURL    = '{';
static const char16_t CLOSE_CURL   = '}';
static const char16_t NUMBER_SIGN  = '#';
static const char16_t QUESTIONMARK = '?';
static const char16_t PERCENT_SIGN = '%';
static const char16_t EXCLAMATION  = '!';
static const char16_t DOLLAR       = '$';
static const char16_t AMPERSAND    = '&';
static const char16_t OPENBRACE    = '(';
static const char16_t CLOSINGBRACE = ')';
static const char16_t EQUALS       = '=';
static const char16_t ATSYMBOL     = '@';

static const uint32_t kSubHostPathCharacterCutoff = 512;

static const char *const kHashSourceValidFns [] = { "sha256", "sha384", "sha512" };
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
 : mHasHashOrNonce(false)
 , mUnsafeInlineKeywordSrc(nullptr)
 , mTokens(aTokens)
 , mSelfURI(aSelfURI)
 , mInnerWindowID(aInnerWindowID)
{
  CSPPARSERLOG(("nsCSPParser::nsCSPParser"));
}

nsCSPParser::~nsCSPParser()
{
  CSPPARSERLOG(("nsCSPParser::~nsCSPParser"));
}

static bool
isCharacterToken(char16_t aSymbol)
{
  return (aSymbol >= 'a' && aSymbol <= 'z') ||
         (aSymbol >= 'A' && aSymbol <= 'Z');
}

static bool
isNumberToken(char16_t aSymbol)
{
  return (aSymbol >= '0' && aSymbol <= '9');
}

static bool
isValidHexDig(char16_t aHexDig)
{
  return (isNumberToken(aHexDig) ||
          (aHexDig >= 'A' && aHexDig <= 'F') ||
          (aHexDig >= 'a' && aHexDig <= 'f'));
}

void
nsCSPParser::resetCurChar(const nsAString& aToken)
{
  mCurChar = aToken.BeginReading();
  mEndChar = aToken.EndReading();
  resetCurValue();
}




bool
nsCSPParser::atEndOfPath()
{
  return (atEnd() || peek(QUESTIONMARK) || peek(NUMBER_SIGN));
}

void
nsCSPParser::percentDecodeStr(const nsAString& aEncStr, nsAString& outDecStr)
{
  outDecStr.Truncate();

  
  struct local {
    static inline char16_t convertHexDig(char16_t aHexDig) {
      if (isNumberToken(aHexDig)) {
        return aHexDig - '0';
      }
      if (aHexDig >= 'A' && aHexDig <= 'F') {
        return aHexDig - 'A' + 10;
      }
      
      
      return aHexDig - 'a' + 10;
    }
  };

  const char16_t *cur, *end, *hexDig1, *hexDig2;
  cur = aEncStr.BeginReading();
  end = aEncStr.EndReading();

  while (cur != end) {
    
    
    if (*cur != PERCENT_SIGN) {
      outDecStr.Append(*cur);
      cur++;
      continue;
    }

    
    hexDig1 = cur + 1;
    hexDig2 = cur + 2;

    
    
    if (hexDig1 == end || hexDig2 == end ||
        !isValidHexDig(*hexDig1) ||
        !isValidHexDig(*hexDig2)) {
      outDecStr.Append(PERCENT_SIGN);
      cur++;
      continue;
    }

    
    char16_t decChar = (local::convertHexDig(*hexDig1) << 4) +
                       local::convertHexDig(*hexDig2);
    outDecStr.Append(decChar);

    
    cur = ++hexDig2;
  }
}


bool
nsCSPParser::atValidUnreservedChar()
{
  return (peek(isCharacterToken) || peek(isNumberToken) ||
          peek(DASH) || peek(DOT) ||
          peek(UNDERLINE) || peek(TILDE));
}







bool
nsCSPParser::atValidSubDelimChar()
{
  return (peek(EXCLAMATION) || peek(DOLLAR) || peek(AMPERSAND) ||
          peek(SINGLEQUOTE) || peek(OPENBRACE) || peek(CLOSINGBRACE) ||
          peek(WILDCARD) || peek(PLUS) || peek(EQUALS));
}


bool
nsCSPParser::atValidPctEncodedChar()
{
  const char16_t* pctCurChar = mCurChar;

  if ((pctCurChar + 2) >= mEndChar) {
    
    return false;
  }

  
  
  if (PERCENT_SIGN != *pctCurChar ||
     !isValidHexDig(*(pctCurChar+1)) ||
     !isValidHexDig(*(pctCurChar+2))) {
    return false;
  }
  return true;
}



bool
nsCSPParser::atValidPathChar()
{
  return (atValidUnreservedChar() ||
          atValidSubDelimChar() ||
          atValidPctEncodedChar() ||
          peek(COLON) || peek(ATSYMBOL));
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
  nsString pctDecodedSubPath;

  while (!atEndOfPath()) {
    if (peek(SLASH)) {
      
      
      
      percentDecodeStr(mCurValue, pctDecodedSubPath);
      aCspHost->appendPath(pctDecodedSubPath);
      
      
      
      resetCurValue();
    }
    else if (!atValidPathChar()) {
      const char16_t* params[] = { mCurToken.get() };
      logWarningErrorToConsole(nsIScriptError::warningFlag,
                               "couldntParseInvalidSource",
                               params, ArrayLength(params));
      return false;
    }
    
    
    
    if (peek(PERCENT_SIGN)) {
      advance();
      advance();
    }
    advance();
    if (++charCounter > kSubHostPathCharacterCutoff) {
      return false;
    }
  }
  
  
  
  percentDecodeStr(mCurValue, pctDecodedSubPath);
  aCspHost->appendPath(pctDecodedSubPath);
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
  if (atEndOfPath()) {
    
    
    
    
    aCspHost->appendPath(NS_LITERAL_STRING("/"));
    return true;
  }
  
  
  if (peek(SLASH)) {
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

  while (!atEndOfPath() && !peek(COLON) && !peek(SLASH)) {
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

  if (CSP_IsKeyword(mCurToken, CSP_UNSAFE_INLINE)) {
    
    
    if (mUnsafeInlineKeywordSrc) {
      const char16_t* params[] = { mCurToken.get() };
      logWarningErrorToConsole(nsIScriptError::warningFlag, "ignoringDuplicateSrc",
                               params, ArrayLength(params));
      return nullptr;
    }
    
    
    mUnsafeInlineKeywordSrc = new nsCSPKeywordSrc(CSP_KeywordToEnum(mCurToken));
    return mUnsafeInlineKeywordSrc;
  }

  if (CSP_IsKeyword(mCurToken, CSP_UNSAFE_EVAL)) {
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

  if (atEndOfPath()) {
    return cspHost;
  }

  
  
  
  if (!path(cspHost)) {
    
    
    
    delete cspHost;
    return nullptr;
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
  
  mHasHashOrNonce = true;
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
      
      mHasHashOrNonce = true;
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

  
  
  
  
  bool allowHttps = false;
  if (parsedScheme.IsEmpty()) {
    
    
    resetCurChar(mCurToken);
    nsAutoCString selfScheme;
    mSelfURI->GetScheme(selfScheme);
    parsedScheme.AssignASCII(selfScheme.get());
    allowHttps = selfScheme.EqualsASCII("http");
  }

  
  
  if (nsCSPHostSrc *cspHost = hostSource()) {
    
    cspHost->setScheme(parsedScheme, allowHttps);
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
nsCSPParser::referrerDirectiveValue()
{
  
  
  
  CSPPARSERLOG(("nsCSPParser::referrerDirectiveValue"));

  if (mCurDir.Length() > 2) {
    CSPPARSERLOG(("Too many tokens in referrer directive, got %d expected 1",
                 mCurDir.Length() - 1));
    return;
  }

  if (!mozilla::net::IsValidReferrerPolicy(mCurDir[1])) {
    CSPPARSERLOG(("invalid value for referrer directive: %s",
                  NS_ConvertUTF16toUTF8(mCurDir[1]).get()));
    return;
  }

  
  mPolicy->setReferrerPolicy(&mCurDir[1]);
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

  
  
  
  if (CSP_IsDirective(mCurDir[0], nsIContentSecurityPolicy::REPORT_URI_DIRECTIVE)) {
    reportURIList(outSrcs);
    return;
  }

  
  
  if (CSP_IsDirective(mCurDir[0], nsIContentSecurityPolicy::REFERRER_DIRECTIVE)) {
    referrerDirectiveValue();
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

  
  
  
  
  if (CSP_IsDirective(mCurToken, nsIContentSecurityPolicy::REFLECTED_XSS_DIRECTIVE)) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "notSupportingDirective",
                             params, ArrayLength(params));
    return nullptr;
  }

  
  
  if (mPolicy->hasDirective(CSP_StringToCSPDirective(mCurToken))) {
    const char16_t* params[] = { mCurToken.get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "duplicateDirective",
                             params, ArrayLength(params));
    return nullptr;
  }
  return new nsCSPDirective(CSP_StringToCSPDirective(mCurToken));
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

  
  nsCSPDirective* cspDir = directiveName();
  if (!cspDir) {
    
    return;
  }

  
  
  mHasHashOrNonce = false;
  mUnsafeInlineKeywordSrc = nullptr;

  
  nsTArray<nsCSPBaseSrc*> srcs;
  directiveValue(srcs);

  
  
  if (srcs.Length() == 0) {
    nsCSPKeywordSrc *keyword = new nsCSPKeywordSrc(CSP_NONE);
    srcs.AppendElement(keyword);
  }

  
  
  
  if (cspDir->equals(nsIContentSecurityPolicy::SCRIPT_SRC_DIRECTIVE) &&
      mHasHashOrNonce && mUnsafeInlineKeywordSrc) {
    mUnsafeInlineKeywordSrc->invalidate();
    
    const char16_t* params[] = { NS_LITERAL_STRING("'unsafe-inline'").get() };
    logWarningErrorToConsole(nsIScriptError::warningFlag, "ignoringSrcWithinScriptSrc",
                             params, ArrayLength(params));
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
    if (!policy->hasDirective(nsIContentSecurityPolicy::REPORT_URI_DIRECTIVE)) {
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
