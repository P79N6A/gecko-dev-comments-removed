







#include <math.h> 

#include "nsCSSScanner.h"
#include "nsStyleUtil.h"
#include "mozilla/css/ErrorReporter.h"
#include "mozilla/Likely.h"
#include "mozilla/Util.h"
#include <algorithm>

using mozilla::ArrayLength;

static const uint8_t IS_HEX_DIGIT  = 0x01;
static const uint8_t START_IDENT   = 0x02;
static const uint8_t IS_IDENT      = 0x04;
static const uint8_t IS_WHITESPACE = 0x08;
static const uint8_t IS_URL_CHAR   = 0x10;

#define W    IS_WHITESPACE
#define I    IS_IDENT
#define U                                      IS_URL_CHAR
#define S             START_IDENT
#define UI   IS_IDENT                         |IS_URL_CHAR
#define USI  IS_IDENT|START_IDENT             |IS_URL_CHAR
#define UXI  IS_IDENT            |IS_HEX_DIGIT|IS_URL_CHAR
#define UXSI IS_IDENT|START_IDENT|IS_HEX_DIGIT|IS_URL_CHAR

static const uint8_t gLexTable[] = {

   0,  0,  0,  0,  0,  0,  0,  0,  0,  W,  W,  0,  W,  W,  0,  0,

   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

   W,  U,  0,  U,  U,  U,  U,  0,  0,  0,  U,  U,  U,  UI, U,  U,

   UXI,UXI,UXI,UXI,UXI,UXI,UXI,UXI,UXI,UXI,U,  U,  U,  U,  U,  U,

   U,UXSI,UXSI,UXSI,UXSI,UXSI,UXSI,USI,USI,USI,USI,USI,USI,USI,USI,USI,

   USI,USI,USI,USI,USI,USI,USI,USI,USI,USI,USI,U,  S,  U,  U,  USI,

   U,UXSI,UXSI,UXSI,UXSI,UXSI,UXSI,USI,USI,USI,USI,USI,USI,USI,USI,USI,

   USI,USI,USI,USI,USI,USI,USI,USI,USI,USI,USI,U,  U,  U,  U,  0
};

MOZ_STATIC_ASSERT(NS_ARRAY_LENGTH(gLexTable) == 128,
                  "gLexTable expected to cover all 128 ASCII characters");

#undef W
#undef S
#undef I
#undef U
#undef UI
#undef USI
#undef UXI
#undef UXSI

static inline bool
IsIdentStart(int32_t aChar)
{
  return aChar >= 0 &&
    (aChar >= 128 || (gLexTable[aChar] & START_IDENT) != 0);
}

static inline bool
StartsIdent(int32_t aFirstChar, int32_t aSecondChar)
{
  return IsIdentStart(aFirstChar) ||
    (aFirstChar == '-' && IsIdentStart(aSecondChar));
}

static inline bool
IsWhitespace(int32_t ch) {
  return uint32_t(ch) < 128 && (gLexTable[ch] & IS_WHITESPACE) != 0;
}

static inline bool
IsDigit(int32_t ch) {
  return (ch >= '0') && (ch <= '9');
}

static inline bool
IsHexDigit(int32_t ch) {
  return uint32_t(ch) < 128 && (gLexTable[ch] & IS_HEX_DIGIT) != 0;
}

static inline bool
IsIdent(int32_t ch) {
  return ch >= 0 && (ch >= 128 || (gLexTable[ch] & IS_IDENT) != 0);
}

static inline bool
IsURLChar(int32_t ch) {
  return ch >= 0 && (ch >= 128 || (gLexTable[ch] & IS_URL_CHAR) != 0);
}

static inline uint32_t
DecimalDigitValue(int32_t ch)
{
  return ch - '0';
}

static inline uint32_t
HexDigitValue(int32_t ch)
{
  if (IsDigit(ch)) {
    return DecimalDigitValue(ch);
  } else {
    
    
    
    return (ch & 0x7) + 9;
  }
}

void
nsCSSToken::AppendToString(nsString& aBuffer) const
{
  switch (mType) {
    case eCSSToken_Ident:
      nsStyleUtil::AppendEscapedCSSIdent(mIdent, aBuffer);
      break;

    case eCSSToken_AtKeyword:
      aBuffer.Append('@');
      nsStyleUtil::AppendEscapedCSSIdent(mIdent, aBuffer);
      break;

    case eCSSToken_ID:
    case eCSSToken_Hash:
      aBuffer.Append('#');
      nsStyleUtil::AppendEscapedCSSIdent(mIdent, aBuffer);
      break;

    case eCSSToken_Function:
      nsStyleUtil::AppendEscapedCSSIdent(mIdent, aBuffer);
      aBuffer.Append('(');
      break;

    case eCSSToken_URL:
    case eCSSToken_Bad_URL:
      aBuffer.AppendLiteral("url(");
      if (mSymbol != PRUnichar(0)) {
        nsStyleUtil::AppendEscapedCSSString(mIdent, aBuffer, mSymbol);
      } else {
        aBuffer.Append(mIdent);
      }
      if (mType == eCSSToken_URL) {
        aBuffer.Append(PRUnichar(')'));
      }
      break;

    case eCSSToken_Number:
      if (mIntegerValid) {
        aBuffer.AppendInt(mInteger, 10);
      } else {
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
      } else {
        aBuffer.AppendFloat(mNumber);
      }
      nsStyleUtil::AppendEscapedCSSIdent(mIdent, aBuffer);
      break;

    case eCSSToken_Bad_String:
      nsStyleUtil::AppendEscapedCSSString(mIdent, aBuffer, mSymbol);
      
      aBuffer.Truncate(aBuffer.Length() - 1);
      break;

    case eCSSToken_String:
      nsStyleUtil::AppendEscapedCSSString(mIdent, aBuffer, mSymbol);
      break;

    case eCSSToken_Symbol:
      aBuffer.Append(mSymbol);
      break;

    case eCSSToken_Whitespace:
      aBuffer.Append(' ');
      break;

    case eCSSToken_HTMLComment:
    case eCSSToken_URange:
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

    default:
      NS_ERROR("invalid token type");
      break;
  }
}

nsCSSScanner::nsCSSScanner(const nsAString& aBuffer, uint32_t aLineNumber)
  : mBuffer(aBuffer.BeginReading())
  , mOffset(0)
  , mCount(aBuffer.Length())
  , mPushback(mLocalPushback)
  , mPushbackCount(0)
  , mPushbackSize(ArrayLength(mLocalPushback))
  , mLineNumber(aLineNumber)
  , mLineOffset(0)
  , mTokenLineNumber(aLineNumber)
  , mTokenLineOffset(0)
  , mTokenOffset(0)
  , mRecordStartOffset(0)
  , mReporter(nullptr)
  , mSVGMode(false)
  , mRecording(false)
{
  MOZ_COUNT_CTOR(nsCSSScanner);
}

nsCSSScanner::~nsCSSScanner()
{
  MOZ_COUNT_DTOR(nsCSSScanner);
  if (mLocalPushback != mPushback) {
    delete [] mPushback;
  }
}

void
nsCSSScanner::StartRecording()
{
  NS_ASSERTION(!mRecording, "already started recording");
  mRecording = true;
  mRecordStartOffset = mOffset - mPushbackCount;
}

void
nsCSSScanner::StopRecording()
{
  NS_ASSERTION(mRecording, "haven't started recording");
  mRecording = false;
}

void
nsCSSScanner::StopRecording(nsString& aBuffer)
{
  NS_ASSERTION(mRecording, "haven't started recording");
  mRecording = false;
  aBuffer.Append(mBuffer + mRecordStartOffset,
                 mOffset - mPushbackCount - mRecordStartOffset);
}

nsDependentSubstring
nsCSSScanner::GetCurrentLine() const
{
  uint32_t end = mTokenOffset;
  while (end < mCount &&
         mBuffer[end] != '\n' && mBuffer[end] != '\r' &&
         mBuffer[end] != '\f') {
    end++;
  }
  return nsDependentSubstring(mBuffer + mTokenLineOffset,
                              mBuffer + end);
}


int32_t
nsCSSScanner::Read()
{
  int32_t rv;
  if (0 < mPushbackCount) {
    rv = int32_t(mPushback[--mPushbackCount]);
  } else {
    if (mOffset == mCount) {
      return -1;
    }
    rv = int32_t(mBuffer[mOffset++]);
    
    
    if (rv == '\r') {
      if (mOffset < mCount && mBuffer[mOffset] == '\n') {
        mOffset++;
      }
      rv = '\n';
    } else if (rv == '\f') {
      rv = '\n';
    }
    if (rv == '\n') {
      
      if (mLineNumber != 0)
        ++mLineNumber;
      mLineOffset = mOffset;
    }
  }
  return rv;
}

int32_t
nsCSSScanner::Peek()
{
  if (0 == mPushbackCount) {
    int32_t ch = Read();
    if (ch < 0) {
      return -1;
    }
    mPushback[0] = PRUnichar(ch);
    mPushbackCount++;
  }
  return int32_t(mPushback[mPushbackCount - 1]);
}

void
nsCSSScanner::Pushback(PRUnichar aChar)
{
  if (mPushbackCount == mPushbackSize) { 
    PRUnichar*  newPushback = new PRUnichar[mPushbackSize + 4];
    if (nullptr == newPushback) {
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

bool
nsCSSScanner::LookAhead(PRUnichar aChar)
{
  int32_t ch = Read();
  if (ch < 0) {
    return false;
  }
  if (ch == aChar) {
    return true;
  }
  Pushback(ch);
  return false;
}

bool
nsCSSScanner::LookAheadOrEOF(PRUnichar aChar)
{
  int32_t ch = Read();
  if (ch < 0) {
    return true;
  }
  if (ch == aChar) {
    return true;
  }
  Pushback(ch);
  return false;
}

void
nsCSSScanner::SkipWhitespace()
{
  for (;;) {
    int32_t ch = Read();
    if (ch < 0) {
      break;
    }
    if ((ch != ' ') && (ch != '\n') && (ch != '\t')) {
      Pushback(ch);
      break;
    }
  }
}

void
nsCSSScanner::SkipComment()
{
  for (;;) {
    int32_t ch = Read();
    if (ch < 0) break;
    if (ch == '*') {
      if (LookAhead('/')) {
        return;
      }
    }
  }

  mReporter->ReportUnexpectedEOF("PECommentEOF");
}





bool
nsCSSScanner::GatherEscape(nsString& aOutput, bool aInString)
{
  int32_t ch = Read();
  if (ch < 0) {
    return false;
  }
  if (IsHexDigit(ch)) {
    int32_t rv = 0;
    int i;
    Pushback(ch);
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
    return true;
  }
  
  
  
  if (ch == '\n') {
    if (!aInString) {
      
      
      
      Pushback(ch);
      return false;
    }
    
    
  } else {
    aOutput.Append(ch);
  }

  return true;
}












bool
nsCSSScanner::GatherIdent(int32_t aChar, nsString& aIdent)
{
  if (aChar == '\\') {
    if (!GatherEscape(aIdent, false)) {
      return false;
    }
  } else {
    MOZ_ASSERT(aChar > 0);
    aIdent.Append(aChar);
  }
  for (;;) {
    
    if (!mPushbackCount && mOffset < mCount) {
      
      uint32_t n = mOffset;
      
      while (n < mCount && IsIdent(mBuffer[n])) {
        ++n;
      }
      
      if (n > mOffset) {
        aIdent.Append(&mBuffer[mOffset], n - mOffset);
        mOffset = n;
      }
    }

    aChar = Read();
    if (aChar < 0) break;
    if (aChar == '\\') {
      if (!GatherEscape(aIdent, false)) {
        Pushback(aChar);
        break;
      }
    } else if (IsIdent(aChar)) {
      aIdent.Append(PRUnichar(aChar));
    } else {
      Pushback(aChar);
      break;
    }
  }
  MOZ_ASSERT(aIdent.Length() > 0);
  return true;
}

bool
nsCSSScanner::ScanIdent(int32_t aChar, nsCSSToken& aToken)
{
  nsString& ident = aToken.mIdent;
  ident.SetLength(0);
  if (!GatherIdent(aChar, ident)) {
    aToken.mType = eCSSToken_Symbol;
    aToken.mSymbol = aChar;
    return true;
  }

  nsCSSTokenType tokenType = eCSSToken_Ident;
  
  if (Peek() == PRUnichar('(')) {
    Read();
    tokenType = eCSSToken_Function;

    if (ident.LowerCaseEqualsLiteral("url")) {
      NextURL(aToken); 
      return true;
    }
  }

  aToken.mType = tokenType;
  return true;
}

bool
nsCSSScanner::ScanAtKeyword(nsCSSToken& aToken)
{
  int32_t ch = Read();
  if (StartsIdent(ch, Peek())) {
    aToken.mIdent.SetLength(0);
    aToken.mType = eCSSToken_AtKeyword;
    if (GatherIdent(ch, aToken.mIdent)) {
      return true;
    }
  }
  if (ch >= 0) {
    Pushback(ch);
  }
  aToken.mType = eCSSToken_Symbol;
  aToken.mSymbol = PRUnichar('@');
  return true;
}

bool
nsCSSScanner::ScanHash(int32_t aChar, nsCSSToken& aToken)
{
  
  aToken.mType = eCSSToken_Symbol;
  aToken.mSymbol = aChar;

  int32_t ch = Read();
  if (ch < 0) {
    return true;
  }
  if (IsIdent(ch) || ch == '\\') {
    
    
    nsCSSTokenType type =
      StartsIdent(ch, Peek()) ? eCSSToken_ID : eCSSToken_Hash;
    aToken.mIdent.SetLength(0);
    if (GatherIdent(ch, aToken.mIdent)) {
      aToken.mType = type;
      return true;
    }
  }

  
  Pushback(ch);
  return true;
}

bool
nsCSSScanner::ScanNumber(int32_t c, nsCSSToken& aToken)
{
  NS_PRECONDITION(c == '.' || c == '+' || c == '-' || IsDigit(c),
                  "Why did we get called?");
  aToken.mHasSign = (c == '+' || c == '-');

  
  int32_t sign = c == '-' ? -1 : 1;
  
  
  
  
  double intPart = 0;
  
  
  
  
  double fracPart = 0;
  
  
  
  
  int32_t exponent = 0;
  
  int32_t expSign = 1;

  if (aToken.mHasSign) {
    NS_ASSERTION(c != '.', "How did that happen?");
    c = Read();
  }

  bool gotDot = (c == '.');

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

  bool gotE = false;
  if (IsSVGMode() && (c == 'e' || c == 'E')) {
    int32_t nextChar = Peek();
    int32_t expSignChar = 0;
    if (nextChar == '-' || nextChar == '+') {
      expSignChar = Read();
      nextChar = Peek();
    }
    if (IsDigit(nextChar)) {
      gotE = true;
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

  nsCSSTokenType type = eCSSToken_Number;

  
  
  aToken.mIntegerValid = false;

  
  float value = float(sign * (intPart + fracPart));
  if (gotE) {
    
    
    
    value *= pow(10.0, double(expSign * exponent));
  } else if (!gotDot) {
    
    if (sign > 0) {
      aToken.mInteger = int32_t(std::min(intPart, double(INT32_MAX)));
    } else {
      aToken.mInteger = int32_t(std::max(-intPart, double(INT32_MIN)));
    }
    aToken.mIntegerValid = true;
  }

  nsString& ident = aToken.mIdent;
  ident.Truncate();

  
  if (c >= 0) {
    if (StartsIdent(c, Peek())) {
      if (GatherIdent(c, ident)) {
        type = eCSSToken_Dimension;
      }
    } else if ('%' == c) {
      type = eCSSToken_Percentage;
      value = value / 100.0f;
      aToken.mIntegerValid = false;
    } else {
      
      Pushback(c);
    }
  }
  aToken.mNumber = value;
  aToken.mType = type;
  return true;
}

bool
nsCSSScanner::ScanString(int32_t aStop, nsCSSToken& aToken)
{
  aToken.mIdent.SetLength(0);
  aToken.mType = eCSSToken_String;
  aToken.mSymbol = PRUnichar(aStop); 
  for (;;) {
    
    if (!mPushbackCount && mOffset < mCount) {
      
      uint32_t n = mOffset;
      
      for (;n < mCount; ++n) {
        PRUnichar nextChar = mBuffer[n];
        if ((nextChar == aStop) || (nextChar == '\\') ||
            (nextChar == '\n') || (nextChar == '\r') || (nextChar == '\f')) {
          break;
        }
      }
      
      if (n > mOffset) {
        aToken.mIdent.Append(&mBuffer[mOffset], n - mOffset);
        mOffset = n;
      }
    }
    int32_t ch = Read();
    if (ch < 0 || ch == aStop) {
      break;
    }
    if (ch == '\n') {
      aToken.mType = eCSSToken_Bad_String;
      mReporter->ReportUnexpected("SEUnterminatedString", aToken);
      break;
    }
    if (ch == '\\') {
      if (!GatherEscape(aToken.mIdent, true)) {
        aToken.mType = eCSSToken_Bad_String;
        Pushback(ch);
        
        
        
        
        
        
        
        mReporter->ReportUnexpected("SEUnterminatedString", aToken);
        break;
      }
    } else {
      aToken.mIdent.Append(ch);
    }
  }
  return true;
}














bool
nsCSSScanner::ScanURange(int32_t aChar, nsCSSToken& aResult)
{
  int32_t intro2 = Read();
  int32_t ch = Peek();

  
  NS_ASSERTION(aChar == 'u' || aChar == 'U',
               "unicode-range called with improper introducer (U)");
  NS_ASSERTION(intro2 == '+',
               "unicode-range called with improper introducer (+)");

  
  
  
  if (!IsHexDigit(ch) && ch != '?') {
    Pushback(intro2);
    Pushback(aChar);
    return ScanIdent(aChar, aResult);
  }

  aResult.mIdent.Truncate();
  aResult.mIdent.Append(aChar);
  aResult.mIdent.Append(intro2);

  bool valid = true;
  bool haveQues = false;
  uint32_t low = 0;
  uint32_t high = 0;
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
        valid = false; 
      }
      low = low*16 + HexDigitValue(ch);
      high = high*16 + HexDigitValue(ch);
    } else {
      haveQues = true;
      low = low*16 + 0x0;
      high = high*16 + 0xF;
    }
  }

  if (ch == '-' && IsHexDigit(Peek())) {
    if (haveQues) {
      valid = false;
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
  return true;
}

bool
nsCSSScanner::NextURL(nsCSSToken& aToken)
{
  SkipWhitespace();

  int32_t ch = Read();
  if (ch < 0) {
    return false;
  }

  
  if ((ch == '"') || (ch == '\'')) {
#ifdef DEBUG
    bool ok =
#endif
      ScanString(ch, aToken);
    NS_ABORT_IF_FALSE(ok, "ScanString should never fail, "
                          "since there's always something read");

    NS_ABORT_IF_FALSE(aToken.mType == eCSSToken_String ||
                      aToken.mType == eCSSToken_Bad_String,
                      "unexpected token type");
    if (MOZ_LIKELY(aToken.mType == eCSSToken_String)) {
      SkipWhitespace();
      if (LookAheadOrEOF(')')) {
        aToken.mType = eCSSToken_URL;
      } else {
        aToken.mType = eCSSToken_Bad_URL;
      }
    } else {
      aToken.mType = eCSSToken_Bad_URL;
    }
    return true;
  }

  
  
  
  
  
  
  
  
  

  aToken.mType = eCSSToken_Bad_URL;
  aToken.mSymbol = PRUnichar(0);
  nsString& ident = aToken.mIdent;
  ident.SetLength(0);

  
  bool ok = true;
  for (;;) {
    if (IsURLChar(ch)) {
      
      ident.Append(PRUnichar(ch));
    } else if (ch == ')') {
      
      break;
    } else if (IsWhitespace(ch)) {
      
      SkipWhitespace();
      
      ok = LookAheadOrEOF(')');
      break;
    } else if (ch == '\\') {
      if (!GatherEscape(ident, false)) {
        ok = false;
        Pushback(ch);
        break;
      }
    } else {
      
      ok = false;
      Pushback(ch); 
                    
      break;
    }

    ch = Read();
    if (ch < 0) {
      break;
    }
  }

  
  
  if (ok) {
    aToken.mType = eCSSToken_URL;
  }
  return true;
}

bool
nsCSSScanner::Next(nsCSSToken& aToken, bool aSkipWS)
{
  for (;;) { 
    mTokenOffset = mOffset;
    mTokenLineOffset = mLineOffset;
    mTokenLineNumber = mLineNumber;

    int32_t ch = Read();
    if (ch < 0) {
      return false;
    }

    
    if ((ch == 'u' || ch == 'U') && Peek() == '+')
      return ScanURange(ch, aToken);

    
    if (StartsIdent(ch, Peek()))
      return ScanIdent(ch, aToken);

    
    if (ch == '@') {
      return ScanAtKeyword(aToken);
    }

    
    if ((ch == '.') || (ch == '+') || (ch == '-')) {
      int32_t nextChar = Peek();
      if (IsDigit(nextChar)) {
        return ScanNumber(ch, aToken);
      }
      else if (('.' == nextChar) && ('.' != ch)) {
        nextChar = Read();
        int32_t followingChar = Peek();
        Pushback(nextChar);
        if (IsDigit(followingChar))
          return ScanNumber(ch, aToken);
      }
    }
    if (IsDigit(ch)) {
      return ScanNumber(ch, aToken);
    }

    
    if (ch == '#') {
      return ScanHash(ch, aToken);
    }

    
    if ((ch == '"') || (ch == '\'')) {
      return ScanString(ch, aToken);
    }

    
    if (IsWhitespace(ch)) {
      SkipWhitespace();
      if (!aSkipWS) {
        aToken.mType = eCSSToken_Whitespace;
        return true;
      }
      continue; 
    }
    if (ch == '/' && !IsSVGMode()) {
      int32_t nextChar = Peek();
      if (nextChar == '*') {
        Read();
        
        SkipComment();
        continue; 
      }
    }
    if (ch == '<') {  
      if (LookAhead('!')) {
        if (LookAhead('-')) {
          if (LookAhead('-')) {
            aToken.mType = eCSSToken_HTMLComment;
            aToken.mIdent.AssignLiteral("<!--");
            return true;
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
          return true;
        }
        Pushback('-');
      }
    }

    
    if (( ch == '|' ) || ( ch == '~' ) || ( ch == '^' ) ||
        ( ch == '$' ) || ( ch == '*' )) {
      int32_t nextChar = Read();
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
        return true;
      } else if (nextChar >= 0) {
        Pushback(nextChar);
      }
    }
    aToken.mType = eCSSToken_Symbol;
    aToken.mSymbol = ch;
    return true;
  }
}
