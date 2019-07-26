







#include <math.h> 

#include "nsCSSScanner.h"
#include "nsStyleUtil.h"
#include "mozilla/css/ErrorReporter.h"
#include "mozilla/Likely.h"
#include "mozilla/Util.h"
#include <algorithm>

using mozilla::ArrayLength;

static const uint8_t IS_HEX_DIGIT  = 0x01;
static const uint8_t IS_IDSTART    = 0x02;
static const uint8_t IS_IDCHAR     = 0x04;
static const uint8_t IS_URL_CHAR   = 0x08;
static const uint8_t IS_HSPACE     = 0x10;
static const uint8_t IS_VSPACE     = 0x20;
static const uint8_t IS_SPACE      = IS_HSPACE|IS_VSPACE;

#define H    IS_HSPACE
#define V    IS_VSPACE
#define I    IS_IDCHAR
#define U                                      IS_URL_CHAR
#define S              IS_IDSTART
#define UI   IS_IDCHAR                        |IS_URL_CHAR
#define USI  IS_IDCHAR|IS_IDSTART             |IS_URL_CHAR
#define UXI  IS_IDCHAR           |IS_HEX_DIGIT|IS_URL_CHAR
#define UXSI IS_IDCHAR|IS_IDSTART|IS_HEX_DIGIT|IS_URL_CHAR

static const uint8_t gLexTable[] = {

   0,  0,  0,  0,  0,  0,  0,  0,  0,  H,  V,  0,  V,  V,  0,  0,

   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,

   H,  U,  0,  U,  U,  U,  U,  0,  0,  0,  U,  U,  U,  UI, U,  U,

   UXI,UXI,UXI,UXI,UXI,UXI,UXI,UXI,UXI,UXI,U,  U,  U,  U,  U,  U,

   U,UXSI,UXSI,UXSI,UXSI,UXSI,UXSI,USI,USI,USI,USI,USI,USI,USI,USI,USI,

   USI,USI,USI,USI,USI,USI,USI,USI,USI,USI,USI,U,  S,  U,  U,  USI,

   U,UXSI,UXSI,UXSI,UXSI,UXSI,UXSI,USI,USI,USI,USI,USI,USI,USI,USI,USI,

   USI,USI,USI,USI,USI,USI,USI,USI,USI,USI,USI,U,  U,  U,  U,  0
};

MOZ_STATIC_ASSERT(MOZ_ARRAY_LENGTH(gLexTable) == 128,
                  "gLexTable expected to cover all 128 ASCII characters");

#undef H
#undef V
#undef S
#undef I
#undef U
#undef UI
#undef USI
#undef UXI
#undef UXSI

static inline bool
IsHorzSpace(int32_t ch) {
  return uint32_t(ch) < 128 && (gLexTable[ch] & IS_HSPACE) != 0;
}

static inline bool
IsVertSpace(int32_t ch) {
  return uint32_t(ch) < 128 && (gLexTable[ch] & IS_VSPACE) != 0;
}

static inline bool
IsWhitespace(int32_t ch) {
  return uint32_t(ch) < 128 && (gLexTable[ch] & IS_SPACE) != 0;
}

static inline bool
IsIdentChar(int32_t ch) {
  return ch >= 0 && (ch >= 128 || (gLexTable[ch] & IS_IDCHAR) != 0);
}

static inline bool
IsIdentStart(int32_t ch) {
  return ch >= 0 && (ch >= 128 || (gLexTable[ch] & IS_IDSTART) != 0);
}

static inline bool
StartsIdent(int32_t aFirstChar, int32_t aSecondChar)
{
  return IsIdentStart(aFirstChar) ||
    (aFirstChar == '-' && IsIdentStart(aSecondChar));
}

static inline bool
IsURLChar(int32_t ch) {
  return ch >= 0 && (ch >= 128 || (gLexTable[ch] & IS_URL_CHAR) != 0);
}

static inline bool
IsDigit(int32_t ch) {
  return (ch >= '0') && (ch <= '9');
}

static inline bool
IsHexDigit(int32_t ch) {
  return uint32_t(ch) < 128 && (gLexTable[ch] & IS_HEX_DIGIT) != 0;
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
}

void
nsCSSScanner::StartRecording()
{
  MOZ_ASSERT(!mRecording, "already started recording");
  mRecording = true;
  mRecordStartOffset = mOffset;
}

void
nsCSSScanner::StopRecording()
{
  MOZ_ASSERT(mRecording, "haven't started recording");
  mRecording = false;
}

void
nsCSSScanner::StopRecording(nsString& aBuffer)
{
  MOZ_ASSERT(mRecording, "haven't started recording");
  mRecording = false;
  aBuffer.Append(mBuffer + mRecordStartOffset,
                 mOffset - mRecordStartOffset);
}

nsDependentSubstring
nsCSSScanner::GetCurrentLine() const
{
  uint32_t end = mTokenOffset;
  while (end < mCount && !IsVertSpace(mBuffer[end])) {
    end++;
  }
  return nsDependentSubstring(mBuffer + mTokenLineOffset,
                              mBuffer + end);
}






inline int32_t
nsCSSScanner::Peek(uint32_t n)
{
  if (mOffset + n >= mCount) {
    return -1;
  }
  return mBuffer[mOffset + n];
}







inline void
nsCSSScanner::Advance(uint32_t n)
{
#ifdef DEBUG
  while (mOffset < mCount && n > 0) {
    MOZ_ASSERT(!IsVertSpace(mBuffer[mOffset]),
               "may not Advance() over a line boundary");
    mOffset++;
    n--;
  }
#else
  if (mOffset + n >= mCount || mOffset + n < mOffset)
    mOffset = mCount;
  else
    mOffset += n;
#endif
}




void
nsCSSScanner::AdvanceLine()
{
  MOZ_ASSERT(IsVertSpace(mBuffer[mOffset]),
             "may not AdvanceLine() over a horizontal character");
  
  if (mBuffer[mOffset]   == '\r' && mOffset + 1 < mCount &&
      mBuffer[mOffset+1] == '\n')
    mOffset += 2;
  else
    mOffset += 1;
  
  if (mLineNumber != 0)
    mLineNumber++;
  mLineOffset = mOffset;
}







void
nsCSSScanner::Backup(uint32_t n)
{
#if 1
  
  
  
  while (mOffset > 0 && n > 0) {
    if (IsVertSpace(mBuffer[mOffset-1])) {
      if (mBuffer[mOffset-1] == '\n' && mOffset > 1 &&
          mBuffer[mOffset-2] == '\r') {
        mOffset -= 2;
      } else {
        mOffset -= 1;
      }
      n--;
      mLineNumber--;
    } else {
      mOffset--;
      n--;
    }
  }
#else
#ifdef DEBUG
  while (mOffset > 0 && n > 0) {
    MOZ_ASSERT(!IsVertSpace(mBuffer[mOffset-1]),
               "may not Backup() over a line boundary");
    mOffset--;
    n--;
  }
#else
  if (mOffset < n)
    mOffset = 0;
  else
    mOffset -= n;
#endif
#endif
}


int32_t
nsCSSScanner::Read()
{
  int32_t rv = Peek();

  
  
  if (IsVertSpace(rv)) {
    AdvanceLine();
    rv = '\n';
  } else if (rv >= 0) {
    Advance();
  }
  return rv;
}

void
nsCSSScanner::Pushback(PRUnichar aChar)
{
  MOZ_ASSERT(mOffset > 0 && aChar == mBuffer[mOffset-1],
             "may only push back exactly what was read");
  Backup(1);
}

bool
nsCSSScanner::LookAhead(PRUnichar aChar)
{
  if (Peek() == aChar) {
    if (IsVertSpace(aChar)) {
      AdvanceLine();
    } else {
      Advance();
    }
    return true;
  }
  return false;
}

bool
nsCSSScanner::LookAheadOrEOF(PRUnichar aChar)
{
  int32_t ch = Peek();
  if (ch == -1) {
    return true;
  }
  if (ch == aChar) {
    if (IsVertSpace(aChar)) {
      AdvanceLine();
    } else {
      Advance();
    }
    return true;
  }
  return false;
}

void
nsCSSScanner::SkipWhitespace()
{
  for (;;) {
    int32_t ch = Peek();
    if (!IsWhitespace(ch)) { 
      break;
    }
    if (IsVertSpace(ch)) {
      AdvanceLine();
    } else {
      Advance();
    }
  }
}

void
nsCSSScanner::SkipComment()
{
  for (;;) {
    int32_t ch = Peek();
    if (ch < 0) {
      mReporter->ReportUnexpectedEOF("PECommentEOF");
      return;
    }
    if (ch == '*' && Peek(1) == '/') {
      Advance(2);
      return;
    }
    if (IsVertSpace(ch)) {
      AdvanceLine();
    } else {
      Advance();
    }
  }
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
    if (mOffset < mCount) {
      
      uint32_t n = mOffset;
      
      while (n < mCount && IsIdentChar(mBuffer[n])) {
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
    } else if (IsIdentChar(aChar)) {
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
  if (IsIdentChar(ch) || ch == '\\') {
    
    
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
    if (mOffset < mCount) {
      
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
