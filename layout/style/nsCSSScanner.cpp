





































#include <math.h>



#include "nsCSSScanner.h"
#include "nsIFactory.h"
#include "nsIInputStream.h"
#include "nsIUnicharInputStream.h"
#include "nsString.h"
#include "nsCRT.h"


#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsReadableUtils.h"
#include "nsIURI.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIStringBundle.h"
#include "nsContentUtils.h"


#undef COLLECT_WHITESPACE

static const PRUnichar CSS_ESCAPE = PRUnichar('\\');
static const PRUint8 IS_HEX_DIGIT = 0x02;
static const PRUint8 START_IDENT = 0x04;
static const PRUint8 IS_IDENT = 0x08;
static const PRUint8 IS_WHITESPACE = 0x10;

static PRBool gLexTableSetup = PR_FALSE;
static PRUint8 gLexTable[256];

#ifdef CSS_REPORT_PARSE_ERRORS
static PRBool gReportErrors = PR_TRUE;
static nsIConsoleService *gConsoleService;
static nsIFactory *gScriptErrorFactory;
static nsIStringBundle *gStringBundle;
#endif

static void
BuildLexTable()
{
  gLexTableSetup = PR_TRUE;

  PRUint8* lt = gLexTable;
  int i;
  lt[CSS_ESCAPE] = START_IDENT;
  lt['-'] |= IS_IDENT;
  lt['_'] |= IS_IDENT | START_IDENT;
  lt[' '] |= IS_WHITESPACE;   
  lt['\t'] |= IS_WHITESPACE;  
  lt['\r'] |= IS_WHITESPACE;  
  lt['\n'] |= IS_WHITESPACE;  
  lt['\f'] |= IS_WHITESPACE;  
  for (i = 161; i <= 255; i++) {
    lt[i] |= IS_IDENT | START_IDENT;
  }
  for (i = '0'; i <= '9'; i++) {
    lt[i] |= IS_HEX_DIGIT | IS_IDENT;
  }
  for (i = 'A'; i <= 'Z'; i++) {
    if ((i >= 'A') && (i <= 'F')) {
      lt[i] |= IS_HEX_DIGIT;
      lt[i+32] |= IS_HEX_DIGIT;
    }
    lt[i] |= IS_IDENT | START_IDENT;
    lt[i+32] |= IS_IDENT | START_IDENT;
  }
}

static inline PRBool
IsIdentStart(PRInt32 aChar)
{
  return aChar >= 0 &&
    (aChar >= 256 || (gLexTable[aChar] & START_IDENT) != 0);
}

static inline PRBool
StartsIdent(PRInt32 aFirstChar, PRInt32 aSecondChar)
{
  return IsIdentStart(aFirstChar) ||
    (aFirstChar == '-' && IsIdentStart(aSecondChar));
}

static inline PRBool
IsWhitespace(PRInt32 ch) {
  return PRUint32(ch) < 256 && (gLexTable[ch] & IS_WHITESPACE) != 0;
}

static inline PRBool
IsDigit(PRInt32 ch) {
  return (ch >= '0') && (ch <= '9');
}

static inline PRBool
IsHexDigit(PRInt32 ch) {
  return PRUint32(ch) < 256 && (gLexTable[ch] & IS_HEX_DIGIT) != 0;
}

static inline PRBool
IsIdent(PRInt32 ch) {
  return ch >= 0 && (ch >= 256 || (gLexTable[ch] & IS_IDENT) != 0);
}

static inline PRUint32
DecimalDigitValue(PRInt32 ch)
{
  return ch - '0';
}

static inline PRUint32
HexDigitValue(PRInt32 ch)
{
  if (IsDigit(ch)) {
    return DecimalDigitValue(ch);
  } else {
    
    
    
    return (ch & 0x7) + 9;
  }
}

nsCSSToken::nsCSSToken()
{
  mType = eCSSToken_Symbol;
}

void
nsCSSToken::AppendToString(nsString& aBuffer)
{
  switch (mType) {
    case eCSSToken_AtKeyword:
      aBuffer.Append(PRUnichar('@')); 
    case eCSSToken_Ident:
    case eCSSToken_WhiteSpace:
    case eCSSToken_Function:
    case eCSSToken_URL:
    case eCSSToken_InvalidURL:
    case eCSSToken_HTMLComment:
    case eCSSToken_URange:
      aBuffer.Append(mIdent);
      break;
    case eCSSToken_Number:
      if (mIntegerValid) {
        aBuffer.AppendInt(mInteger, 10);
      }
      else {
        aBuffer.AppendFloat(mNumber);
      }
      break;
    case eCSSToken_Percentage:
      NS_ASSERTION(!mIntegerValid, "How did a percentage token get this set?");
      aBuffer.AppendFloat(mNumber * 100.0f);
      aBuffer.Append(PRUnichar('%')); 
      break;
    case eCSSToken_Dimension:
      if (mIntegerValid) {
        aBuffer.AppendInt(mInteger, 10);
      }
      else {
        aBuffer.AppendFloat(mNumber);
      }
      aBuffer.Append(mIdent);
      break;
    case eCSSToken_String:
      aBuffer.Append(mSymbol);
      aBuffer.Append(mIdent); 
    case eCSSToken_Symbol:
      aBuffer.Append(mSymbol);
      break;
    case eCSSToken_ID:
    case eCSSToken_Ref:
      aBuffer.Append(PRUnichar('#'));
      aBuffer.Append(mIdent);
      break;
    case eCSSToken_Includes:
      aBuffer.AppendLiteral("~=");
      break;
    case eCSSToken_Dashmatch:
      aBuffer.AppendLiteral("|=");
      break;
    case eCSSToken_Beginsmatch:
      aBuffer.AppendLiteral("^=");
      break;
    case eCSSToken_Endsmatch:
      aBuffer.AppendLiteral("$=");
      break;
    case eCSSToken_Containsmatch:
      aBuffer.AppendLiteral("*=");
      break;
    case eCSSToken_Error:
      aBuffer.Append(mSymbol);
      aBuffer.Append(mIdent);
      break;
    default:
      NS_ERROR("invalid token type");
      break;
  }
}

nsCSSScanner::nsCSSScanner()
  : mInputStream(nsnull)
  , mReadPointer(nsnull)
  , mLowLevelError(NS_OK)
#ifdef MOZ_SVG
  , mSVGMode(PR_FALSE)
#endif
#ifdef CSS_REPORT_PARSE_ERRORS
  , mError(mErrorBuf, NS_ARRAY_LENGTH(mErrorBuf), 0)
#endif
{
  MOZ_COUNT_CTOR(nsCSSScanner);
  if (!gLexTableSetup) {
    
    BuildLexTable();
  }
  mPushback = mLocalPushback;
  mPushbackSize = NS_ARRAY_LENGTH(mLocalPushback);
  
  
  
}

nsCSSScanner::~nsCSSScanner()
{
  MOZ_COUNT_DTOR(nsCSSScanner);
  Close();
  if (mLocalPushback != mPushback) {
    delete [] mPushback;
  }
}

nsresult
nsCSSScanner::GetLowLevelError()
{
  return mLowLevelError;
}

void
nsCSSScanner::SetLowLevelError(nsresult aErrorCode)
{
  NS_ASSERTION(aErrorCode != NS_OK, "SetLowLevelError() used to clear error");
  NS_ASSERTION(mLowLevelError == NS_OK, "there is already a low-level error");
  mLowLevelError = aErrorCode;
}

#ifdef CSS_REPORT_PARSE_ERRORS
#define CSS_ERRORS_PREF "layout.css.report_errors"

static int
CSSErrorsPrefChanged(const char *aPref, void *aClosure)
{
  gReportErrors = nsContentUtils::GetBoolPref(CSS_ERRORS_PREF, PR_TRUE);
  return NS_OK;
}
#endif

 PRBool
nsCSSScanner::InitGlobals()
{
#ifdef CSS_REPORT_PARSE_ERRORS
  if (gConsoleService && gScriptErrorFactory)
    return PR_TRUE;
  
  nsresult rv = CallGetService(NS_CONSOLESERVICE_CONTRACTID, &gConsoleService);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  rv = CallGetClassObject(NS_SCRIPTERROR_CONTRACTID, &gScriptErrorFactory);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  NS_ASSERTION(gConsoleService && gScriptErrorFactory,
               "unexpected null pointer without failure");

  nsContentUtils::RegisterPrefCallback(CSS_ERRORS_PREF, CSSErrorsPrefChanged, nsnull);
  CSSErrorsPrefChanged(CSS_ERRORS_PREF, nsnull);
#endif
  return PR_TRUE;
}

 void
nsCSSScanner::ReleaseGlobals()
{
#ifdef CSS_REPORT_PARSE_ERRORS
  nsContentUtils::UnregisterPrefCallback(CSS_ERRORS_PREF, CSSErrorsPrefChanged, nsnull);
  NS_IF_RELEASE(gConsoleService);
  NS_IF_RELEASE(gScriptErrorFactory);
  NS_IF_RELEASE(gStringBundle);
#endif
}

void
nsCSSScanner::Init(nsIUnicharInputStream* aInput, 
                   const PRUnichar * aBuffer, PRUint32 aCount, 
                   nsIURI* aURI, PRUint32 aLineNumber)
{
  NS_PRECONDITION(!mInputStream, "Should not have an existing input stream!");
  NS_PRECONDITION(!mReadPointer, "Should not have an existing input buffer!");

  
  if (aInput) {
    NS_PRECONDITION(!aBuffer, "Shouldn't have both input and buffer!");
    NS_PRECONDITION(aCount == 0, "Shouldn't have count with a stream");
    mInputStream = aInput;
    mReadPointer = mBuffer;
    mCount = 0;
  } else {
    NS_PRECONDITION(aBuffer, "Either aInput or aBuffer must be set");
    
    mInputStream = nsnull;
    mReadPointer = aBuffer;
    mCount = aCount;
  }

#ifdef CSS_REPORT_PARSE_ERRORS
  
  
  if (aURI != mURI) {
    mURI = aURI;
    if (aURI) {
      aURI->GetSpec(mFileName);
    } else {
      mFileName.Adopt(NS_strdup("from DOM"));
    }
  }
#endif 
  mLineNumber = aLineNumber;

  
  mOffset = 0;
  mPushbackCount = 0;
  mLowLevelError = NS_OK;

#ifdef CSS_REPORT_PARSE_ERRORS
  mColNumber = 0;
#endif
}

#ifdef CSS_REPORT_PARSE_ERRORS


#define REPORT_UNEXPECTED_EOF(lf_) \
  ReportUnexpectedEOF(#lf_)

void
nsCSSScanner::AddToError(const nsSubstring& aErrorText)
{
  if (mError.IsEmpty()) {
    mErrorLineNumber = mLineNumber;
    mErrorColNumber = mColNumber;
    mError = aErrorText;
  } else {
    mError.Append(NS_LITERAL_STRING("  ") + aErrorText);
  }
}

void
nsCSSScanner::ClearError()
{
  mError.Truncate();
}

void
nsCSSScanner::OutputError()
{
  if (mError.IsEmpty()) return;
 
  

  if (InitGlobals() && gReportErrors) {
    nsresult rv;
    nsCOMPtr<nsIScriptError> errorObject =
      do_CreateInstance(gScriptErrorFactory, &rv);
    if (NS_SUCCEEDED(rv)) {
      rv = errorObject->Init(mError.get(),
                             NS_ConvertUTF8toUTF16(mFileName).get(),
                             EmptyString().get(),
                             mErrorLineNumber,
                             mErrorColNumber,
                             nsIScriptError::warningFlag,
                             "CSS Parser");
      if (NS_SUCCEEDED(rv))
        gConsoleService->LogMessage(errorObject);
    }
  }
  ClearError();
}

static PRBool
InitStringBundle()
{
  if (gStringBundle)
    return PR_TRUE;

  nsCOMPtr<nsIStringBundleService> sbs =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID);
  if (!sbs)
    return PR_FALSE;

  nsresult rv = 
    sbs->CreateBundle("chrome://global/locale/css.properties", &gStringBundle);
  if (NS_FAILED(rv)) {
    gStringBundle = nsnull;
    return PR_FALSE;
  }

  return PR_TRUE;
}

#define ENSURE_STRINGBUNDLE \
  PR_BEGIN_MACRO if (!InitStringBundle()) return; PR_END_MACRO


void nsCSSScanner::ReportUnexpected(const char* aMessage)
{
  ENSURE_STRINGBUNDLE;

  nsXPIDLString str;
  gStringBundle->GetStringFromName(NS_ConvertASCIItoUTF16(aMessage).get(),
                                   getter_Copies(str));
  AddToError(str);
}
  
void
nsCSSScanner::ReportUnexpectedParams(const char* aMessage,
                                     const PRUnichar **aParams,
                                     PRUint32 aParamsLength)
{
  NS_PRECONDITION(aParamsLength > 0, "use the non-params version");
  ENSURE_STRINGBUNDLE;

  nsXPIDLString str;
  gStringBundle->FormatStringFromName(NS_ConvertASCIItoUTF16(aMessage).get(),
                                      aParams, aParamsLength,
                                      getter_Copies(str));
  AddToError(str);
}


void
nsCSSScanner::ReportUnexpectedEOF(const char* aLookingFor)
{
  ENSURE_STRINGBUNDLE;

  nsXPIDLString innerStr;
  gStringBundle->GetStringFromName(NS_ConvertASCIItoUTF16(aLookingFor).get(),
                                   getter_Copies(innerStr));

  const PRUnichar *params[] = {
    innerStr.get()
  };
  nsXPIDLString str;
  gStringBundle->FormatStringFromName(NS_LITERAL_STRING("PEUnexpEOF2").get(),
                                      params, NS_ARRAY_LENGTH(params),
                                      getter_Copies(str));
  AddToError(str);
}


void
nsCSSScanner::ReportUnexpectedEOF(PRUnichar aLookingFor)
{
  ENSURE_STRINGBUNDLE;

  const PRUnichar lookingForStr[] = {
    PRUnichar('\''), aLookingFor, PRUnichar('\''), PRUnichar(0)
  };
  const PRUnichar *params[] = { lookingForStr };
  nsXPIDLString str;
  gStringBundle->FormatStringFromName(NS_LITERAL_STRING("PEUnexpEOF2").get(),
                                      params, NS_ARRAY_LENGTH(params),
                                      getter_Copies(str));
  AddToError(str);
}



void
nsCSSScanner::ReportUnexpectedToken(nsCSSToken& tok,
                                    const char *aMessage)
{
  ENSURE_STRINGBUNDLE;
  
  nsAutoString tokenString;
  tok.AppendToString(tokenString);

  const PRUnichar *params[] = {
    tokenString.get()
  };

  ReportUnexpectedParams(aMessage, params, NS_ARRAY_LENGTH(params));
}


void
nsCSSScanner::ReportUnexpectedTokenParams(nsCSSToken& tok,
                                          const char* aMessage,
                                          const PRUnichar **aParams,
                                          PRUint32 aParamsLength)
{
  NS_PRECONDITION(aParamsLength > 1, "use the non-params version");
  NS_PRECONDITION(aParams[0] == nsnull, "first param should be empty");

  ENSURE_STRINGBUNDLE;
  
  nsAutoString tokenString;
  tok.AppendToString(tokenString);
  aParams[0] = tokenString.get();

  ReportUnexpectedParams(aMessage, aParams, aParamsLength);
}

#else

#define REPORT_UNEXPECTED_EOF(lf_)

#endif 

void
nsCSSScanner::Close()
{
  mInputStream = nsnull;
  mReadPointer = nsnull;

  
#ifdef CSS_REPORT_PARSE_ERRORS
  mFileName.Truncate();
  mURI = nsnull;
  mError.Truncate();
#endif
  if (mPushback != mLocalPushback) {
    delete [] mPushback;
    mPushback = mLocalPushback;
    mPushbackSize = NS_ARRAY_LENGTH(mLocalPushback);
  }
}

#ifdef CSS_REPORT_PARSE_ERRORS
#define TAB_STOP_WIDTH 8
#endif

PRBool
nsCSSScanner::EnsureData()
{
  if (mOffset < mCount)
    return PR_TRUE;

  if (!mInputStream)
    return PR_FALSE;

  mOffset = 0;
  nsresult rv = mInputStream->Read(mBuffer, CSS_BUFFER_SIZE, &mCount);

  if (NS_FAILED(rv)) {
    mCount = 0;
    SetLowLevelError(rv);
    return PR_FALSE;
  }

  return mCount > 0;
}


PRInt32
nsCSSScanner::Read()
{
  PRInt32 rv;
  if (0 < mPushbackCount) {
    rv = PRInt32(mPushback[--mPushbackCount]);
  } else {
    if (mOffset == mCount && !EnsureData()) {
      return -1;
    }
    rv = PRInt32(mReadPointer[mOffset++]);
    
    
    if (rv == '\r') {
      if (EnsureData() && mReadPointer[mOffset] == '\n') {
        mOffset++;
      }
      rv = '\n';
    } else if (rv == '\f') {
      rv = '\n';
    }
    if (rv == '\n') {
      
      if (mLineNumber != 0)
        ++mLineNumber;
#ifdef CSS_REPORT_PARSE_ERRORS
      mColNumber = 0;
#endif
    } 
#ifdef CSS_REPORT_PARSE_ERRORS
    else if (rv == '\t') {
      mColNumber = ((mColNumber - 1 + TAB_STOP_WIDTH) / TAB_STOP_WIDTH)
                   * TAB_STOP_WIDTH;
    } else if (rv != '\n') {
      mColNumber++;
    }
#endif
  }

  return rv;
}

PRInt32
nsCSSScanner::Peek()
{
  if (0 == mPushbackCount) {
    PRInt32 ch = Read();
    if (ch < 0) {
      return -1;
    }
    mPushback[0] = PRUnichar(ch);
    mPushbackCount++;
  }

  return PRInt32(mPushback[mPushbackCount - 1]);
}

void
nsCSSScanner::Pushback(PRUnichar aChar)
{
  if (mPushbackCount == mPushbackSize) { 
    PRUnichar*  newPushback = new PRUnichar[mPushbackSize + 4];
    if (nsnull == newPushback) {
      return;
    }
    mPushbackSize += 4;
    memcpy(newPushback, mPushback, sizeof(PRUnichar) * mPushbackCount);
    if (mPushback != mLocalPushback) {
      delete [] mPushback;
    }
    mPushback = newPushback;
  }
  mPushback[mPushbackCount++] = aChar;
}

PRBool
nsCSSScanner::LookAhead(PRUnichar aChar)
{
  PRInt32 ch = Read();
  if (ch < 0) {
    return PR_FALSE;
  }
  if (ch == aChar) {
    return PR_TRUE;
  }
  Pushback(ch);
  return PR_FALSE;
}

void
nsCSSScanner::EatWhiteSpace()
{
  for (;;) {
    PRInt32 ch = Read();
    if (ch < 0) {
      break;
    }
    if ((ch != ' ') && (ch != '\n') && (ch != '\t')) {
      Pushback(ch);
      break;
    }
  }
}

PRBool
nsCSSScanner::Next(nsCSSToken& aToken)
{
  for (;;) { 
    PRInt32 ch = Read();
    if (ch < 0) {
      return PR_FALSE;
    }

    
    if ((ch == 'u' || ch == 'U') && Peek() == '+')
      return ParseURange(ch, aToken);

    
    if (StartsIdent(ch, Peek()))
      return ParseIdent(ch, aToken);

    
    if (ch == '@') {
      PRInt32 nextChar = Read();
      if (nextChar >= 0) {
        PRInt32 followingChar = Peek();
        Pushback(nextChar);
        if (StartsIdent(nextChar, followingChar))
          return ParseAtKeyword(ch, aToken);
      }
    }

    
    if ((ch == '.') || (ch == '+') || (ch == '-')) {
      PRInt32 nextChar = Peek();
      if (IsDigit(nextChar)) {
        return ParseNumber(ch, aToken);
      }
      else if (('.' == nextChar) && ('.' != ch)) {
        nextChar = Read();
        PRInt32 followingChar = Peek();
        Pushback(nextChar);
        if (IsDigit(followingChar))
          return ParseNumber(ch, aToken);
      }
    }
    if (IsDigit(ch)) {
      return ParseNumber(ch, aToken);
    }

    
    if (ch == '#') {
      return ParseRef(ch, aToken);
    }

    
    if ((ch == '"') || (ch == '\'')) {
      return ParseString(ch, aToken);
    }

    
    if (IsWhitespace(ch)) {
      aToken.mType = eCSSToken_WhiteSpace;
      aToken.mIdent.Assign(PRUnichar(ch));
      EatWhiteSpace();
      return PR_TRUE;
    }
    if (ch == '/') {
      PRInt32 nextChar = Peek();
      if (nextChar == '*') {
        (void) Read();
#if 0
        
        
        
        
        aToken.mIdent.SetCapacity(2);
        aToken.mIdent.Assign(PRUnichar(ch));
        aToken.mIdent.Append(PRUnichar(nextChar));
        return ParseCComment(aToken);
#endif
        if (!SkipCComment()) {
          return PR_FALSE;
        }
        continue; 
      }
    }
    if (ch == '<') {  
      if (LookAhead('!')) {
        if (LookAhead('-')) {
          if (LookAhead('-')) {
            aToken.mType = eCSSToken_HTMLComment;
            aToken.mIdent.AssignLiteral("<!--");
            return PR_TRUE;
          }
          Pushback('-');
        }
        Pushback('!');
      }
    }
    if (ch == '-') {  
      if (LookAhead('-')) {
        if (LookAhead('>')) {
          aToken.mType = eCSSToken_HTMLComment;
          aToken.mIdent.AssignLiteral("-->");
          return PR_TRUE;
        }
        Pushback('-');
      }
    }

    
    if (( ch == '|' ) || ( ch == '~' ) || ( ch == '^' ) ||
        ( ch == '$' ) || ( ch == '*' )) {
      PRInt32 nextChar = Read();
      if ( nextChar == '=' ) {
        if (ch == '~') {
          aToken.mType = eCSSToken_Includes;
        }
        else if (ch == '|') {
          aToken.mType = eCSSToken_Dashmatch;
        }
        else if (ch == '^') {
          aToken.mType = eCSSToken_Beginsmatch;
        }
        else if (ch == '$') {
          aToken.mType = eCSSToken_Endsmatch;
        }
        else if (ch == '*') {
          aToken.mType = eCSSToken_Containsmatch;
        }
        return PR_TRUE;
      } else if (nextChar >= 0) {
        Pushback(nextChar);
      }
    }
    aToken.mType = eCSSToken_Symbol;
    aToken.mSymbol = ch;
    return PR_TRUE;
  }
}

PRBool
nsCSSScanner::NextURL(nsCSSToken& aToken)
{
  PRInt32 ch = Read();
  if (ch < 0) {
    return PR_FALSE;
  }

  
  if ((ch == '"') || (ch == '\'')) {
    return ParseString(ch, aToken);
  }

  
  if (IsWhitespace(ch)) {
    aToken.mType = eCSSToken_WhiteSpace;
    aToken.mIdent.Assign(PRUnichar(ch));
    EatWhiteSpace();
    return PR_TRUE;
  }

  
  
  
  
  
  
  
  
  

  aToken.mType = eCSSToken_InvalidURL;
  nsString& ident = aToken.mIdent;
  ident.SetLength(0);

  Pushback(ch);

  
  PRBool ok = PR_TRUE;
  for (;;) {
    ch = Read();
    if (ch < 0) break;
    if (ch == CSS_ESCAPE) {
      ParseAndAppendEscape(ident);
    } else if ((ch == '"') || (ch == '\'') || (ch == '(')) {
      
      ok = PR_FALSE;
      Pushback(ch); 
                    
      break;
    } else if (IsWhitespace(ch)) {
      
        EatWhiteSpace();
        if (LookAhead(')')) {
        Pushback(')');  
        
        break;
      }
      
      
      ok = PR_FALSE;
      break;
    } else if (ch == ')') {
      Pushback(ch);
      
      break;
    } else {
      
      ident.Append(PRUnichar(ch));
    }
  }

  
  
  if (ok) {
    aToken.mType = eCSSToken_URL;
  }
  return PR_TRUE;
}


void
nsCSSScanner::ParseAndAppendEscape(nsString& aOutput)
{
  PRInt32 ch = Peek();
  if (ch < 0) {
    aOutput.Append(CSS_ESCAPE);
    return;
  }
  if (IsHexDigit(ch)) {
    PRInt32 rv = 0;
    int i;
    for (i = 0; i < 6; i++) { 
      ch = Read();
      if (ch < 0) {
        
        break;
      }
      if (!IsHexDigit(ch) && !IsWhitespace(ch)) {
        Pushback(ch);
        break;
      } else if (IsHexDigit(ch)) {
        rv = rv * 16 + HexDigitValue(ch);
      } else {
        NS_ASSERTION(IsWhitespace(ch), "bad control flow");
        
        break;
      }
    }
    if (6 == i) { 
      ch = Peek();
      if (IsWhitespace(ch)) {
        (void) Read();
      }
    }
    NS_ASSERTION(rv >= 0, "How did rv become negative?");
    
    
    
    
    
    
    
    
    
    if (rv > 0) {
      AppendUCS4ToUTF16(ENSURE_VALID_CHAR(rv), aOutput);
    } else {
      while (i--)
        aOutput.Append('0');
      if (IsWhitespace(ch))
        Pushback(ch);
    }
    return;
  } 
  
  
  
  ch = Read();  
  if ((ch > 0) && (ch != '\n')) {
    aOutput.Append(ch);
  }
}








PRBool
nsCSSScanner::GatherIdent(PRInt32 aChar, nsString& aIdent)
{
  if (aChar == CSS_ESCAPE) {
    ParseAndAppendEscape(aIdent);
  }
  else if (0 < aChar) {
    aIdent.Append(aChar);
  }
  for (;;) {
    
    if (!mPushbackCount && EnsureData()) {
      
      PRUint32 n = mOffset;
      
      while (n < mCount && IsIdent(mReadPointer[n])) {
        ++n;
      }
      
      if (n > mOffset) {
#ifdef CSS_REPORT_PARSE_ERRORS
        mColNumber += n - mOffset;
#endif
        aIdent.Append(&mReadPointer[mOffset], n - mOffset);
        mOffset = n;
      }
    }

    aChar = Read();
    if (aChar < 0) break;
    if (aChar == CSS_ESCAPE) {
      ParseAndAppendEscape(aIdent);
    } else if (IsIdent(aChar)) {
      aIdent.Append(PRUnichar(aChar));
    } else {
      Pushback(aChar);
      break;
    }
  }
  return PR_TRUE;
}

PRBool
nsCSSScanner::ParseRef(PRInt32 aChar, nsCSSToken& aToken)
{
  aToken.mIdent.SetLength(0);
  aToken.mType = eCSSToken_Ref;
  PRInt32 ch = Read();
  if (ch < 0) {
    return PR_FALSE;
  }
  if (IsIdent(ch) || ch == CSS_ESCAPE) {
    
    
    if (StartsIdent(ch, Peek())) {
      aToken.mType = eCSSToken_ID;
    }
    return GatherIdent(ch, aToken.mIdent);
  }

  
  Pushback(ch);
  return PR_TRUE;
}

PRBool
nsCSSScanner::ParseIdent(PRInt32 aChar, nsCSSToken& aToken)
{
  nsString& ident = aToken.mIdent;
  ident.SetLength(0);
  if (!GatherIdent(aChar, ident)) {
    return PR_FALSE;
  }

  nsCSSTokenType tokenType = eCSSToken_Ident;
  
  if (PRUnichar('(') == PRUnichar(Peek())) { 
    tokenType = eCSSToken_Function;
  }

  aToken.mType = tokenType;
  return PR_TRUE;
}

PRBool
nsCSSScanner::ParseAtKeyword(PRInt32 aChar, nsCSSToken& aToken)
{
  aToken.mIdent.SetLength(0);
  aToken.mType = eCSSToken_AtKeyword;
  return GatherIdent(0, aToken.mIdent);
}

PRBool
nsCSSScanner::ParseNumber(PRInt32 c, nsCSSToken& aToken)
{
  NS_PRECONDITION(c == '.' || c == '+' || c == '-' || IsDigit(c),
                  "Why did we get called?");
  aToken.mHasSign = (c == '+' || c == '-');

  
  PRInt32 sign = c == '-' ? -1 : 1;
  
  
  
  
  double intPart = 0;
  
  
  
  
  double fracPart = 0;
  
  
  
  
  PRInt32 exponent = 0;
  
  PRInt32 expSign = 1;

  if (aToken.mHasSign) {
    NS_ASSERTION(c != '.', "How did that happen?");
    c = Read();
  }

  PRBool gotDot = (c == '.');

  if (!gotDot) {
    
    NS_ASSERTION(IsDigit(c), "Why did we get called?");
    do {
      intPart = 10*intPart + DecimalDigitValue(c);
      c = Read();
      
    } while (IsDigit(c));

    gotDot = (c == '.') && IsDigit(Peek());
  }

  if (gotDot) {
    
    c = Read();
    NS_ASSERTION(IsDigit(c), "How did we get here?");
    
    float divisor = 10;
    do {
      fracPart += DecimalDigitValue(c) / divisor;
      divisor *= 10;
      c = Read();
      
    } while (IsDigit(c));
  }

  PRBool gotE = PR_FALSE;
#ifdef MOZ_SVG
  if (IsSVGMode() && (c == 'e' || c == 'E')) {
    PRInt32 nextChar = Peek();
    PRInt32 expSignChar = 0;
    if (nextChar == '-' || nextChar == '+') {
      expSignChar = Read();
      nextChar = Peek();
    }
    if (IsDigit(nextChar)) {
      gotE = PR_TRUE;
      if (expSignChar == '-') {
        expSign = -1;
      }

      c = Read();
      NS_ASSERTION(IsDigit(c), "Peek() must have lied");
      do {
        exponent = 10*exponent + DecimalDigitValue(c);
        c = Read();
        
      } while (IsDigit(c));
    } else {
      if (expSignChar) {
        Pushback(expSignChar);
      }
    }
  }
#endif

  nsCSSTokenType type = eCSSToken_Number;

  
  
  aToken.mIntegerValid = PR_FALSE;

  
  float value = float(sign * (intPart + fracPart));
  if (gotE) {
    
    
    
    value *= pow(10.0, double(expSign * exponent));
  } else if (!gotDot) {
    if (intPart > PR_INT32_MAX) {
      
      intPart = PR_INT32_MAX;
    }
    aToken.mInteger = PRInt32(sign * intPart);
    aToken.mIntegerValid = PR_TRUE;
  }

  nsString& ident = aToken.mIdent;
  ident.Truncate();

  
  if (c >= 0) {
    if (StartsIdent(c, Peek())) {
      if (!GatherIdent(c, ident)) {
        return PR_FALSE;
      }
      type = eCSSToken_Dimension;
    } else if ('%' == c) {
      type = eCSSToken_Percentage;
      value = value / 100.0f;
      aToken.mIntegerValid = PR_FALSE;
    } else {
      
      Pushback(c);
    }
  }
  aToken.mNumber = value;
  aToken.mType = type;
  return PR_TRUE;
}

PRBool
nsCSSScanner::SkipCComment()
{
  for (;;) {
    PRInt32 ch = Read();
    if (ch < 0) break;
    if (ch == '*') {
      if (LookAhead('/')) {
        return PR_TRUE;
      }
    }
  }

  REPORT_UNEXPECTED_EOF(PECommentEOF);
  return PR_FALSE;
}

PRBool
nsCSSScanner::ParseString(PRInt32 aStop, nsCSSToken& aToken)
{
  aToken.mIdent.SetLength(0);
  aToken.mType = eCSSToken_String;
  aToken.mSymbol = PRUnichar(aStop); 
  for (;;) {
    
    if (!mPushbackCount && EnsureData()) {
      
      PRUint32 n = mOffset;
      
      for (;n < mCount; ++n) {
        PRUnichar nextChar = mReadPointer[n];
        if ((nextChar == aStop) || (nextChar == CSS_ESCAPE) ||
            (nextChar == '\n') || (nextChar == '\r') || (nextChar == '\f')) {
          break;
        }
#ifdef CSS_REPORT_PARSE_ERRORS
        if (nextChar == '\t') {
          mColNumber = ((mColNumber - 1 + TAB_STOP_WIDTH) / TAB_STOP_WIDTH)
                       * TAB_STOP_WIDTH;
        } else {
          ++mColNumber;
        }
#endif
      }
      
      if (n > mOffset) {
        aToken.mIdent.Append(&mReadPointer[mOffset], n - mOffset);
        mOffset = n;
      }
    }
    PRInt32 ch = Read();
    if (ch < 0 || ch == aStop) {
      break;
    }
    if (ch == '\n') {
      aToken.mType = eCSSToken_Error;
#ifdef CSS_REPORT_PARSE_ERRORS
      ReportUnexpectedToken(aToken, "SEUnterminatedString");
#endif
      break;
    }
    if (ch == CSS_ESCAPE) {
      ParseAndAppendEscape(aToken.mIdent);
    } else {
      aToken.mIdent.Append(ch);
    }
  }
  return PR_TRUE;
}














PRBool
nsCSSScanner::ParseURange(PRInt32 aChar, nsCSSToken& aResult)
{
  PRInt32 intro2 = Read();
  PRInt32 ch = Peek();

  
  NS_ASSERTION(aChar == 'u' || aChar == 'U',
               "unicode-range called with improper introducer (U)");
  NS_ASSERTION(intro2 == '+',
               "unicode-range called with improper introducer (+)");

  
  
  
  if (!IsHexDigit(ch) && ch != '?') {
    Pushback(intro2);
    Pushback(aChar);
    return ParseIdent(aChar, aResult);
  }

  aResult.mIdent.Truncate();
  aResult.mIdent.Append(aChar);
  aResult.mIdent.Append(intro2);

  PRBool valid = PR_TRUE;
  PRBool haveQues = PR_FALSE;
  PRUint32 low = 0;
  PRUint32 high = 0;
  int i = 0;

  for (;;) {
    ch = Read();
    i++;
    if (i == 7 || !(IsHexDigit(ch) || ch == '?')) {
      break;
    }

    aResult.mIdent.Append(ch);
    if (IsHexDigit(ch)) {
      if (haveQues) {
        valid = PR_FALSE; 
      }
      low = low*16 + HexDigitValue(ch);
      high = high*16 + HexDigitValue(ch);
    } else {
      haveQues = PR_TRUE;
      low = low*16 + 0x0;
      high = high*16 + 0xF;
    }
  }

  if (ch == '-' && IsHexDigit(Peek())) {
    if (haveQues) {
      valid = PR_FALSE;
    }

    aResult.mIdent.Append(ch);
    high = 0;
    i = 0;
    for (;;) {
      ch = Read();
      i++;
      if (i == 7 || !IsHexDigit(ch)) {
        break;
      }
      aResult.mIdent.Append(ch);
      high = high*16 + HexDigitValue(ch);
    }
  }
  Pushback(ch);

  aResult.mInteger = low;
  aResult.mInteger2 = high;
  aResult.mIntegerValid = valid;
  aResult.mType = eCSSToken_URange;
  return PR_TRUE;
}
