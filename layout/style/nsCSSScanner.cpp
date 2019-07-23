








































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

#define BUFFER_SIZE 256

static const PRUnichar CSS_ESCAPE = PRUnichar('\\');
const PRUint8 nsCSSScanner::IS_DIGIT = 0x01;
const PRUint8 nsCSSScanner::IS_HEX_DIGIT = 0x02;
const PRUint8 nsCSSScanner::START_IDENT = 0x04;
const PRUint8 nsCSSScanner::IS_IDENT = 0x08;
const PRUint8 nsCSSScanner::IS_WHITESPACE = 0x10;

static PRBool gLexTableSetup = PR_FALSE;
PRUint8 nsCSSScanner::gLexTable[256];

#ifdef CSS_REPORT_PARSE_ERRORS
static PRBool gReportErrors = PR_TRUE;
static nsIConsoleService *gConsoleService;
static nsIFactory *gScriptErrorFactory;
static nsIStringBundle *gStringBundle;
#endif


void
nsCSSScanner::BuildLexTable()
{
  gLexTableSetup = PR_TRUE;

  PRUint8* lt = gLexTable;
  int i;
  lt[CSS_ESCAPE] = START_IDENT;
  lt['-'] |= IS_IDENT;
  lt['_'] |= IS_IDENT | START_IDENT;
  
  lt[' '] |= IS_WHITESPACE;   
  lt['\t'] |= IS_WHITESPACE;  
  lt['\v'] |= IS_WHITESPACE;  
  lt['\r'] |= IS_WHITESPACE;  
  lt['\n'] |= IS_WHITESPACE;  
  lt['\f'] |= IS_WHITESPACE;  
  for (i = 161; i <= 255; i++) {
    lt[i] |= IS_IDENT | START_IDENT;
  }
  for (i = '0'; i <= '9'; i++) {
    lt[i] |= IS_DIGIT | IS_HEX_DIGIT | IS_IDENT;
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
#ifdef CSS_REPORT_PARSE_ERRORS
  , mError(mErrorBuf, NS_ARRAY_LENGTH(mErrorBuf), 0)
#endif
{
  MOZ_COUNT_CTOR(nsCSSScanner);
  if (!gLexTableSetup) {
    
    BuildLexTable();
  }
  mPushback = mLocalPushback;
  mPushbackSize = 4;
  
  
  
}

nsCSSScanner::~nsCSSScanner()
{
  MOZ_COUNT_DTOR(nsCSSScanner);
  Close();
  if (mLocalPushback != mPushback) {
    delete [] mPushback;
  }
}

#ifdef CSS_REPORT_PARSE_ERRORS
#define CSS_ERRORS_PREF "layout.css.report_errors"

PR_STATIC_CALLBACK(int) CSSErrorsPrefChanged(const char *aPref, void *aClosure)
{
  gReportErrors = nsContentUtils::GetBoolPref(CSS_ERRORS_PREF, PR_TRUE);
  return NS_OK;
}
#endif

 PRBool nsCSSScanner::InitGlobals()
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

 void nsCSSScanner::ReleaseGlobals()
{
#ifdef CSS_REPORT_PARSE_ERRORS
  nsContentUtils::UnregisterPrefCallback(CSS_ERRORS_PREF, CSSErrorsPrefChanged, nsnull);
  NS_IF_RELEASE(gConsoleService);
  NS_IF_RELEASE(gScriptErrorFactory);
  NS_IF_RELEASE(gStringBundle);
#endif
}

void nsCSSScanner::Init(nsIUnicharInputStream* aInput, 
                        const PRUnichar * aBuffer, PRInt32 aCount, 
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
  mLastRead = 0;

#ifdef CSS_REPORT_PARSE_ERRORS
  mColNumber = 0;
#endif
}

#ifdef CSS_REPORT_PARSE_ERRORS


#define REPORT_UNEXPECTED_EOF(lf_) \
  ReportUnexpectedEOF(#lf_)

void nsCSSScanner::AddToError(const nsSubstring& aErrorText)
{
  if (mError.IsEmpty()) {
    mErrorLineNumber = mLineNumber;
    mErrorColNumber = mColNumber;
    mError = aErrorText;
  } else {
    mError.Append(NS_LITERAL_STRING("  ") + aErrorText);
  }
}

void nsCSSScanner::ClearError()
{
  mError.Truncate();
}

void nsCSSScanner::OutputError()
{
  if (mError.IsEmpty()) return;
 
#ifdef DEBUG
  fprintf(stderr, "CSS Error (%s :%u.%u): %s\n",
                  mFileName.get(), mErrorLineNumber, mErrorColNumber,
                  NS_ConvertUTF16toUTF8(mError).get());
#endif

  

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

static PRBool InitStringBundle()
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
  
void nsCSSScanner::ReportUnexpectedParams(const char* aMessage,
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


void nsCSSScanner::ReportUnexpectedEOF(const char* aLookingFor)
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



void nsCSSScanner::ReportUnexpectedToken(nsCSSToken& tok,
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


void nsCSSScanner::ReportUnexpectedTokenParams(nsCSSToken& tok,
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

void nsCSSScanner::Close()
{
  mInputStream = nsnull;
  mReadPointer = nsnull;
}

#ifdef CSS_REPORT_PARSE_ERRORS
#define TAB_STOP_WIDTH 8
#endif


PRInt32 nsCSSScanner::Read(nsresult& aErrorCode)
{
  PRInt32 rv;
  if (0 < mPushbackCount) {
    rv = PRInt32(mPushback[--mPushbackCount]);
  } else {
    if (mCount < 0) {
      return -1;
    }
    if (mOffset == mCount) {
      mOffset = 0;
      if (!mInputStream) {
        mCount = 0;
        return -1;
      }
      aErrorCode = mInputStream->Read(mBuffer, CSS_BUFFER_SIZE, (PRUint32*)&mCount);
      if (NS_FAILED(aErrorCode) || mCount == 0) {
        mCount = 0;
        return -1;
      }
    }
    rv = PRInt32(mReadPointer[mOffset++]);
    if (((rv == '\n') && (mLastRead != '\r')) || (rv == '\r')) {
      
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
  mLastRead = rv;

  return rv;
}

PRInt32 nsCSSScanner::Peek(nsresult& aErrorCode)
{
  if (0 == mPushbackCount) {
    PRInt32 savedLastRead = mLastRead;
    PRInt32 ch = Read(aErrorCode);
    mLastRead = savedLastRead;
    if (ch < 0) {
      return -1;
    }
    mPushback[0] = PRUnichar(ch);
    mPushbackCount++;
  }

  return PRInt32(mPushback[mPushbackCount - 1]);
}

void nsCSSScanner::Unread()
{
  NS_PRECONDITION((mLastRead >= 0), "double pushback");
  Pushback(PRUnichar(mLastRead));
  mLastRead = -1;
}

void nsCSSScanner::Pushback(PRUnichar aChar)
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

PRBool nsCSSScanner::LookAhead(nsresult& aErrorCode, PRUnichar aChar)
{
  PRInt32 ch = Read(aErrorCode);
  if (ch < 0) {
    return PR_FALSE;
  }
  if (ch == aChar) {
    return PR_TRUE;
  }
  Unread();
  return PR_FALSE;
}

PRBool nsCSSScanner::EatWhiteSpace(nsresult& aErrorCode)
{
  PRBool eaten = PR_FALSE;
  for (;;) {
    PRInt32 ch = Read(aErrorCode);
    if (ch < 0) {
      break;
    }
    if ((ch == ' ') || (ch == '\n') || (ch == '\r') || (ch == '\t')) {
      eaten = PR_TRUE;
      continue;
    }
    Unread();
    break;
  }
  return eaten;
}

PRBool nsCSSScanner::EatNewline(nsresult& aErrorCode)
{
  PRInt32 ch = Read(aErrorCode);
  if (ch < 0) {
    return PR_FALSE;
  }
  PRBool eaten = PR_FALSE;
  if (ch == '\r') {
    eaten = PR_TRUE;
    ch = Peek(aErrorCode);
    if (ch == '\n') {
      (void) Read(aErrorCode);
    }
  } else if (ch == '\n') {
    eaten = PR_TRUE;
  } else {
    Unread();
  }
  return eaten;
}


PRBool
nsCSSScanner::CheckLexTable(PRInt32 aChar, PRUint8 aBit, PRUint8* aLexTable)
{
  NS_ASSERTION(!(aBit & (START_IDENT | IS_IDENT)),
               "can't use CheckLexTable with identifiers");
  return aChar >= 0 && aChar < 256 && (aLexTable[aChar] & aBit) != 0;
}

PRBool nsCSSScanner::Next(nsresult& aErrorCode, nsCSSToken& aToken)
{
  PRInt32 ch = Read(aErrorCode);
  if (ch < 0) {
    return PR_FALSE;
  }
  PRUint8* lexTable = gLexTable;

  
  if (StartsIdent(ch, Peek(aErrorCode), lexTable))
    return ParseIdent(aErrorCode, ch, aToken);

  
     
  
  if (ch == '@') {
    PRInt32 nextChar = Read(aErrorCode);
    PRInt32 followingChar = Peek(aErrorCode);
    Pushback(nextChar);
    if (StartsIdent(nextChar, followingChar, lexTable))
      return ParseAtKeyword(aErrorCode, ch, aToken);
  }

  
  if ((ch == '.') || (ch == '+') || (ch == '-')) {
    PRInt32 nextChar = Peek(aErrorCode);
    if (CheckLexTable(nextChar, IS_DIGIT, lexTable)) {
      return ParseNumber(aErrorCode, ch, aToken);
    }
    else if (('.' == nextChar) && ('.' != ch)) {
      nextChar = Read(aErrorCode);
      PRInt32 followingChar = Peek(aErrorCode);
      Pushback(nextChar);
      if (CheckLexTable(followingChar, IS_DIGIT, lexTable))
        return ParseNumber(aErrorCode, ch, aToken);
    }
  }
  if ((lexTable[ch] & IS_DIGIT) != 0) {
    return ParseNumber(aErrorCode, ch, aToken);
  }

  
  if (ch == '#') {
    return ParseRef(aErrorCode, ch, aToken);
  }

  
  if ((ch == '"') || (ch == '\'')) {
    return ParseString(aErrorCode, ch, aToken);
  }

  
  if ((lexTable[ch] & IS_WHITESPACE) != 0) {
    aToken.mType = eCSSToken_WhiteSpace;
    aToken.mIdent.Assign(PRUnichar(ch));
    (void) EatWhiteSpace(aErrorCode);
    return PR_TRUE;
  }
  if (ch == '/') {
    PRInt32 nextChar = Peek(aErrorCode);
    if (nextChar == '*') {
      (void) Read(aErrorCode);
#if 0
      
      
      
      
      aToken.mIdent.SetCapacity(2);
      aToken.mIdent.Assign(PRUnichar(ch));
      aToken.mIdent.Append(PRUnichar(nextChar));
      return ParseCComment(aErrorCode, aToken);
#endif
      return SkipCComment(aErrorCode) && Next(aErrorCode, aToken);
    }
  }
  if (ch == '<') {  
    if (LookAhead(aErrorCode, '!')) {
      if (LookAhead(aErrorCode, '-')) {
        if (LookAhead(aErrorCode, '-')) {
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
    if (LookAhead(aErrorCode, '-')) {
      if (LookAhead(aErrorCode, '>')) {
        aToken.mType = eCSSToken_HTMLComment;
        aToken.mIdent.AssignLiteral("-->");
        return PR_TRUE;
      }
      Pushback('-');
    }
  }

  
  if (( ch == '|' ) || ( ch == '~' ) || ( ch == '^' ) ||
      ( ch == '$' ) || ( ch == '*' )) {
    PRInt32 nextChar = Read(aErrorCode);
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
    } else {
      Pushback(nextChar);
    }
  }
  aToken.mType = eCSSToken_Symbol;
  aToken.mSymbol = ch;
  return PR_TRUE;
}

PRBool nsCSSScanner::NextURL(nsresult& aErrorCode, nsCSSToken& aToken)
{
  PRInt32 ch = Read(aErrorCode);
  if (ch < 0) {
    return PR_FALSE;
  }
  PRUint8* lexTable = gLexTable;

  
  if ((ch == '"') || (ch == '\'')) {
    return ParseString(aErrorCode, ch, aToken);
  }

  
  if (ch < 256 && (lexTable[ch] & IS_WHITESPACE) != 0) {
    aToken.mType = eCSSToken_WhiteSpace;
    aToken.mIdent.Assign(PRUnichar(ch));
    (void) EatWhiteSpace(aErrorCode);
    return PR_TRUE;
  }
  if (ch == '/') {
    PRInt32 nextChar = Peek(aErrorCode);
    if (nextChar == '*') {
      (void) Read(aErrorCode);
#if 0
      
      
      
      
      aToken.mIdent.SetCapacity(2);
      aToken.mIdent.Assign(PRUnichar(ch));
      aToken.mIdent.Append(PRUnichar(nextChar));
      return ParseCComment(aErrorCode, aToken);
#endif
      return SkipCComment(aErrorCode) && Next(aErrorCode, aToken);
    }
  }

  
  
  
  
  
  
  
  

  aToken.mType = eCSSToken_InvalidURL;
  nsString& ident = aToken.mIdent;
  ident.SetLength(0);

  if (ch == ')') {
    Pushback(ch);
    
    aToken.mType = eCSSToken_URL;
  } else {
    
    Pushback(ch);
    PRBool ok = PR_TRUE;
    for (;;) {
      ch = Read(aErrorCode);
      if (ch < 0) break;
      if (ch == CSS_ESCAPE) {
        ParseAndAppendEscape(aErrorCode, ident);
      } else if ((ch == '"') || (ch == '\'') || (ch == '(')) {
        
        ok = PR_FALSE;
      } else if ((256 > ch) && ((gLexTable[ch] & IS_WHITESPACE) != 0)) {
        
        (void) EatWhiteSpace(aErrorCode);
        if (LookAhead(aErrorCode, ')')) {
          Pushback(')');  
          
          break;
        }
        
        
        ok = PR_FALSE;
      } else if (ch == ')') {
        Unread();
        
        break;
      } else {
        
        ident.Append(PRUnichar(ch));
      }
    }

    
    
    if (ok) {
      aToken.mType = eCSSToken_URL;
    }
  }
  return PR_TRUE;
}


void
nsCSSScanner::ParseAndAppendEscape(nsresult& aErrorCode, nsString& aOutput)
{
  PRUint8* lexTable = gLexTable;
  PRInt32 ch = Peek(aErrorCode);
  if (ch < 0) {
    aOutput.Append(CSS_ESCAPE);
    return;
  }
  if ((ch <= 255) && ((lexTable[ch] & IS_HEX_DIGIT) != 0)) {
    PRInt32 rv = 0;
    int i;
    for (i = 0; i < 6; i++) { 
      ch = Read(aErrorCode);
      if (ch < 0) {
        
        break;
      }
      if (ch >= 256 || (lexTable[ch] & (IS_HEX_DIGIT | IS_WHITESPACE)) == 0) {
        Unread();
        break;
      } else if ((lexTable[ch] & IS_HEX_DIGIT) != 0) {
        if ((lexTable[ch] & IS_DIGIT) != 0) {
          rv = rv * 16 + (ch - '0');
        } else {
          
          
          
          rv = rv * 16 + ((ch & 0x7) + 9);
        }
      } else {
        NS_ASSERTION((lexTable[ch] & IS_WHITESPACE) != 0, "bad control flow");
        
        if (ch == '\r' && Peek(aErrorCode) == '\n') {
          
          Read(aErrorCode);
        }
        break;
      }
    }
    if (6 == i) { 
      ch = Peek(aErrorCode);
      if ((0 <= ch) && (ch <= 255) && 
          ((lexTable[ch] & IS_WHITESPACE) != 0)) {
        ch = Read(aErrorCode);
        
        if (ch == '\r') {
          ch = Peek(aErrorCode);
          if (ch == '\n') {
            ch = Read(aErrorCode);
          }
        }
      }
    }
    NS_ASSERTION(rv >= 0, "How did rv become negative?");
    if (rv > 0) {
      AppendUCS4ToUTF16(ENSURE_VALID_CHAR(rv), aOutput);
    }
    return;
  } else {
    
    
    
    if (!EatNewline(aErrorCode)) { 
      (void) Read(aErrorCode);
      if (ch > 0) {
        aOutput.Append(ch);
      }
    }
    return;
  }
}








PRBool nsCSSScanner::GatherIdent(nsresult& aErrorCode, PRInt32 aChar,
                                 nsString& aIdent)
{
  if (aChar == CSS_ESCAPE) {
    ParseAndAppendEscape(aErrorCode, aIdent);
  }
  else if (0 < aChar) {
    aIdent.Append(aChar);
  }
  for (;;) {
    aChar = Read(aErrorCode);
    if (aChar < 0) break;
    if (aChar == CSS_ESCAPE) {
      ParseAndAppendEscape(aErrorCode, aIdent);
    } else if ((aChar > 255) || ((gLexTable[aChar] & IS_IDENT) != 0)) {
      aIdent.Append(PRUnichar(aChar));
    } else {
      Unread();
      break;
    }
  }
  return PR_TRUE;
}

PRBool nsCSSScanner::ParseRef(nsresult& aErrorCode,
                              PRInt32 aChar,
                              nsCSSToken& aToken)
{
  aToken.mIdent.SetLength(0);
  aToken.mType = eCSSToken_Ref;
  PRInt32 ch = Read(aErrorCode);
  if (ch < 0) {
    return PR_FALSE;
  }
  if (ch > 255 || (gLexTable[ch] & IS_IDENT) || ch == CSS_ESCAPE) {
    
    
    if (StartsIdent(ch, Peek(aErrorCode), gLexTable)) {
      aToken.mType = eCSSToken_ID;
    }
    return GatherIdent(aErrorCode, ch, aToken.mIdent);
  }

  
  Unread();
  return PR_TRUE;
}

PRBool nsCSSScanner::ParseIdent(nsresult& aErrorCode,
                                PRInt32 aChar,
                                nsCSSToken& aToken)
{
  nsString& ident = aToken.mIdent;
  ident.SetLength(0);
  if (!GatherIdent(aErrorCode, aChar, ident)) {
    return PR_FALSE;
  }

  nsCSSTokenType tokenType = eCSSToken_Ident;
  
  if (PRUnichar('(') == PRUnichar(Peek(aErrorCode))) { 
    tokenType = eCSSToken_Function;
  }

  aToken.mType = tokenType;
  return PR_TRUE;
}

PRBool nsCSSScanner::ParseAtKeyword(nsresult& aErrorCode, PRInt32 aChar,
                                    nsCSSToken& aToken)
{
  aToken.mIdent.SetLength(0);
  aToken.mType = eCSSToken_AtKeyword;
  return GatherIdent(aErrorCode, 0, aToken.mIdent);
}

PRBool nsCSSScanner::ParseNumber(nsresult& aErrorCode, PRInt32 c,
                                 nsCSSToken& aToken)
{
  nsString& ident = aToken.mIdent;
  ident.SetLength(0);
  PRBool gotDot = (c == '.') ? PR_TRUE : PR_FALSE;
  if (c != '+') {
    ident.Append(PRUnichar(c));
  }

  
  PRUint8* lexTable = gLexTable;
  for (;;) {
    c = Read(aErrorCode);
    if (c < 0) break;
    if (!gotDot && (c == '.') &&
        CheckLexTable(Peek(aErrorCode), IS_DIGIT, lexTable)) {
      gotDot = PR_TRUE;
    } else if ((c > 255) || ((lexTable[c] & IS_DIGIT) == 0)) {
      break;
    }
    ident.Append(PRUnichar(c));
  }

  
  nsCSSTokenType type = eCSSToken_Number;
  PRInt32 ec;
  float value = ident.ToFloat(&ec);

  
  aToken.mIntegerValid = PR_FALSE;
  if (c >= 0) {
    if ((c <= 255) && ((lexTable[c] & START_IDENT) != 0)) {
      ident.SetLength(0);
      if (!GatherIdent(aErrorCode, c, ident)) {
        return PR_FALSE;
      }
      type = eCSSToken_Dimension;
    } else if ('%' == c) {
      type = eCSSToken_Percentage;
      value = value / 100.0f;
      ident.SetLength(0);
    } else {
      
      Unread();
      if (!gotDot) {
        aToken.mInteger = ident.ToInteger(&ec);
        aToken.mIntegerValid = PR_TRUE;
      }
      ident.SetLength(0);
    }
  }
  else {  
    if (!gotDot) {
      aToken.mInteger = ident.ToInteger(&ec);
      aToken.mIntegerValid = PR_TRUE;
    }
    ident.SetLength(0);
  }
  aToken.mNumber = value;
  aToken.mType = type;
  return PR_TRUE;
}

PRBool nsCSSScanner::SkipCComment(nsresult& aErrorCode)
{
  for (;;) {
    PRInt32 ch = Read(aErrorCode);
    if (ch < 0) break;
    if (ch == '*') {
      if (LookAhead(aErrorCode, '/')) {
        return PR_TRUE;
      }
    }
  }

  REPORT_UNEXPECTED_EOF(PECommentEOF);
  return PR_FALSE;
}

#if 0
PRBool nsCSSScanner::ParseCComment(nsresult& aErrorCode, nsCSSToken& aToken)
{
  nsString& ident = aToken.mIdent;
  for (;;) {
    PRInt32 ch = Read(aErrorCode);
    if (ch < 0) break;
    if (ch == '*') {
      if (LookAhead(aErrorCode, '/')) {
        ident.Append(PRUnichar(ch));
        ident.Append(PRUnichar('/'));
        break;
      }
    }
#ifdef COLLECT_WHITESPACE
    ident.Append(PRUnichar(ch));
#endif
  }
  aToken.mType = eCSSToken_WhiteSpace;
  return PR_TRUE;
}
#endif

#if 0
PRBool nsCSSScanner::ParseEOLComment(nsresult& aErrorCode, nsCSSToken& aToken)
{
  nsString& ident = aToken.mIdent;
  ident.SetLength(0);
  for (;;) {
    if (EatNewline(aErrorCode)) {
      break;
    }
    PRInt32 ch = Read(aErrorCode);
    if (ch < 0) {
      break;
    }
#ifdef COLLECT_WHITESPACE
    ident.Append(PRUnichar(ch));
#endif
  }
  aToken.mType = eCSSToken_WhiteSpace;
  return PR_TRUE;
}
#endif 

PRBool nsCSSScanner::ParseString(nsresult& aErrorCode, PRInt32 aStop,
                                 nsCSSToken& aToken)
{
  aToken.mIdent.SetLength(0);
  aToken.mType = eCSSToken_String;
  aToken.mSymbol = PRUnichar(aStop); 
  for (;;) {
    if (EatNewline(aErrorCode)) {
      aToken.mType = eCSSToken_Error;
#ifdef CSS_REPORT_PARSE_ERRORS
      ReportUnexpectedToken(aToken, "SEUnterminatedString");
#endif
      return PR_TRUE;
    }
    PRInt32 ch = Read(aErrorCode);
    if (ch < 0) {
      return PR_FALSE;
    }
    if (ch == aStop) {
      break;
    }
    if (ch == CSS_ESCAPE) {
      ParseAndAppendEscape(aErrorCode, aToken.mIdent);
    }
    else if (0 < ch) {
      aToken.mIdent.Append(ch);
    }
  }
  return PR_TRUE;
}
