







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
static const uint8_t IS_STRING     = 0x40;

#define H    IS_HSPACE
#define V    IS_VSPACE
#define I    IS_IDCHAR
#define J    IS_IDSTART
#define U    IS_URL_CHAR
#define S    IS_STRING
#define X    IS_HEX_DIGIT

#define SH    S|H
#define SU    S|U
#define SUI   S|U|I
#define SUIJ  S|U|I|J
#define SUIX  S|U|I|X
#define SUIJX S|U|I|J|X

static const uint8_t gLexTable[] = {

    0,    S,    S,    S,    S,    S,    S,    S,

    S,   SH,    V,    S,    V,    V,    S,    S,

    S,    S,    S,    S,    S,    S,    S,    S,

    S,    S,    S,    S,    S,    S,    S,    S,

   SH,   SU,    0,   SU,   SU,   SU,   SU,    0,

    S,    S,   SU,   SU,   SU,  SUI,   SU,   SU,

 SUIX, SUIX, SUIX, SUIX, SUIX, SUIX, SUIX, SUIX,

 SUIX, SUIX,   SU,   SU,   SU,   SU,   SU,   SU,

   SU,SUIJX,SUIJX,SUIJX,SUIJX,SUIJX,SUIJX, SUIJ,

 SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ,

 SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ,

 SUIJ, SUIJ, SUIJ,   SU,    J,   SU,   SU, SUIJ,

   SU,SUIJX,SUIJX,SUIJX,SUIJX,SUIJX,SUIJX, SUIJ,

 SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ,

 SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ, SUIJ,

 SUIJ, SUIJ, SUIJ,   SU,   SU,   SU,   SU,    S,
};

MOZ_STATIC_ASSERT(MOZ_ARRAY_LENGTH(gLexTable) == 128,
                  "gLexTable expected to cover all 128 ASCII characters");

#undef I
#undef J
#undef U
#undef S
#undef X
#undef SH
#undef SU
#undef SUI
#undef SUIJ
#undef SUIX
#undef SUIJX






static inline bool
IsOpenCharClass(int32_t ch, uint8_t cls) {
  return ch >= 0 && (ch >= 128 || (gLexTable[ch] & cls) != 0);
}






static inline bool
IsClosedCharClass(int32_t ch, uint8_t cls) {
  return uint32_t(ch) < 128 && (gLexTable[ch] & cls) != 0;
}





static inline bool
IsWhitespace(int32_t ch) {
  return IsClosedCharClass(ch, IS_SPACE);
}




static inline bool
IsHorzSpace(int32_t ch) {
  return IsClosedCharClass(ch, IS_HSPACE);
}





static inline bool
IsVertSpace(int32_t ch) {
  return IsClosedCharClass(ch, IS_VSPACE);
}





static inline bool
IsIdentChar(int32_t ch) {
  return IsOpenCharClass(ch, IS_IDCHAR);
}





static inline bool
IsIdentStart(int32_t ch) {
  return IsOpenCharClass(ch, IS_IDSTART);
}





static inline bool
StartsIdent(int32_t aFirstChar, int32_t aSecondChar)
{
  return IsIdentStart(aFirstChar) ||
    (aFirstChar == '-' && IsIdentStart(aSecondChar));
}




static inline bool
IsDigit(int32_t ch) {
  return (ch >= '0') && (ch <= '9');
}




static inline bool
IsHexDigit(int32_t ch) {
  return IsClosedCharClass(ch, IS_HEX_DIGIT);
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






static inline nsCSSTokenType
MatchOperatorType(int32_t ch)
{
  switch (ch) {
  case '~': return eCSSToken_Includes;
  case '|': return eCSSToken_Dashmatch;
  case '^': return eCSSToken_Beginsmatch;
  case '$': return eCSSToken_Endsmatch;
  case '*': return eCSSToken_Containsmatch;
  default:  return eCSSToken_Symbol;
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
  MOZ_ASSERT(Peek() == '/' && Peek(1) == '*', "should not have been called");
  Advance(2);
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
  MOZ_ASSERT(Peek() == '\\', "should not have been called");
  int32_t ch = Peek(1);
  if (ch < 0) {
    
    return false;
  }
  if (IsVertSpace(ch)) {
    if (aInString) {
      
      
      
      Advance();
      AdvanceLine();
      return true;
    }
    
    return false;
  }

  if (!IsHexDigit(ch)) {
    
    
    
    Advance(2);
    aOutput.Append(ch);
    return true;
  }

  
  
  
  
  

  
  
  
  Advance();
  uint32_t val = 0;
  int i = 0;
  do {
    val = val * 16 + HexDigitValue(ch);
    i++;
    Advance();
    ch = Peek();
  } while (i < 6 && IsHexDigit(ch));

  
  
  
  if (MOZ_UNLIKELY(val == 0)) {
    do {
      aOutput.Append('0');
    } while (--i);
  } else {
    AppendUCS4ToUTF16(ENSURE_VALID_CHAR(val), aOutput);
    
    
    if (IsVertSpace(ch)) {
      AdvanceLine();
    } else if (IsHorzSpace(ch)) {
      Advance();
    }
  }
  return true;
}










bool
nsCSSScanner::GatherText(uint8_t aClass, nsString& aText)
{
  
  
  
  MOZ_ASSERT(aClass == IS_STRING ||
             aClass == IS_IDCHAR ||
             aClass == IS_URL_CHAR,
             "possibly-inappropriate character class");

  uint32_t start = mOffset;
  bool inString = aClass == IS_STRING;

  for (;;) {
    
    uint32_t n = mOffset;
    while (n < mCount && IsOpenCharClass(mBuffer[n], aClass)) {
      n++;
    }
    if (n > mOffset) {
      aText.Append(&mBuffer[mOffset], n - mOffset);
      mOffset = n;
    }
    if (n == mCount) {
      break;
    }

    int32_t ch = Peek();
    MOZ_ASSERT(!IsOpenCharClass(ch, aClass),
               "should not have exited the inner loop");

    if (ch != '\\') {
      break;
    }
    if (!GatherEscape(aText, inString)) {
      break;
    }
  }

  return mOffset > start;
}







bool
nsCSSScanner::ScanIdent(nsCSSToken& aToken)
{
  if (MOZ_UNLIKELY(!GatherText(IS_IDCHAR, aToken.mIdent))) {
    aToken.mSymbol = Peek();
    Advance();
    return true;
  }

  if (MOZ_LIKELY(Peek() != '(')) {
    aToken.mType = eCSSToken_Ident;
    return true;
  }

  Advance();
  aToken.mType = eCSSToken_Function;
  if (aToken.mIdent.LowerCaseEqualsLiteral("url")) {
    NextURL(aToken);
  }
  return true;
}





bool
nsCSSScanner::ScanAtKeyword(nsCSSToken& aToken)
{
  MOZ_ASSERT(Peek() == '@', "should not have been called");

  
  aToken.mSymbol = '@';
  Advance();

  int32_t ch = Peek();
  if (StartsIdent(ch, Peek(1))) {
    if (GatherText(IS_IDCHAR, aToken.mIdent)) {
       aToken.mType = eCSSToken_AtKeyword;
     }
  }
  return true;
}






bool
nsCSSScanner::ScanHash(nsCSSToken& aToken)
{
  MOZ_ASSERT(Peek() == '#', "should not have been called");

  
  aToken.mSymbol = '#';
  Advance();

  int32_t ch = Peek();
  if (IsIdentChar(ch) || ch == '\\') {
    nsCSSTokenType type =
      StartsIdent(ch, Peek(1)) ? eCSSToken_ID : eCSSToken_Hash;
    aToken.mIdent.SetLength(0);
    if (GatherText(IS_IDCHAR, aToken.mIdent)) {
      aToken.mType = type;
    }
  }

  return true;
}








bool
nsCSSScanner::ScanNumber(nsCSSToken& aToken)
{
  int32_t c = Peek();
#ifdef DEBUG
  {
    int32_t c2 = Peek(1);
    int32_t c3 = Peek(2);
    MOZ_ASSERT(IsDigit(c) ||
               (IsDigit(c2) && (c == '.' || c == '+' || c == '-')) ||
               (IsDigit(c3) && (c == '+' || c == '-') && c2 == '.'),
               "should not have been called");
  }
#endif

  
  int32_t sign = c == '-' ? -1 : 1;
  
  
  
  
  double intPart = 0;
  
  
  
  
  double fracPart = 0;
  
  
  
  
  int32_t exponent = 0;
  
  int32_t expSign = 1;

  aToken.mHasSign = (c == '+' || c == '-');
  if (aToken.mHasSign) {
    Advance();
    c = Peek();
  }

  bool gotDot = (c == '.');

  if (!gotDot) {
    
    MOZ_ASSERT(IsDigit(c), "should have been excluded by logic above");
    do {
      intPart = 10*intPart + DecimalDigitValue(c);
      Advance();
      c = Peek();
    } while (IsDigit(c));

    gotDot = (c == '.') && IsDigit(Peek(1));
  }

  if (gotDot) {
    
    Advance();
    c = Peek();
    MOZ_ASSERT(IsDigit(c), "should have been excluded by logic above");
    
    double divisor = 10;
    do {
      fracPart += DecimalDigitValue(c) / divisor;
      divisor *= 10;
      Advance();
      c = Peek();
    } while (IsDigit(c));
  }

  bool gotE = false;
  if (IsSVGMode() && (c == 'e' || c == 'E')) {
    int32_t expSignChar = Peek(1);
    int32_t nextChar = Peek(2);
    if (IsDigit(expSignChar) ||
        ((expSignChar == '-' || expSignChar == '+') && IsDigit(nextChar))) {
      gotE = true;
      if (expSignChar == '-') {
        expSign = -1;
      }
      Advance(); 
      if (expSignChar == '-' || expSignChar == '+') {
        Advance();
        c = nextChar;
      } else {
        c = expSignChar;
      }
      MOZ_ASSERT(IsDigit(c), "should have been excluded by logic above");
      do {
        exponent = 10*exponent + DecimalDigitValue(c);
        Advance();
        c = Peek();
      } while (IsDigit(c));
    }
  }

  nsCSSTokenType type = eCSSToken_Number;

  
  
  aToken.mIntegerValid = false;

  
  
  double value = sign * (intPart + fracPart);
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

  
  if (c >= 0) {
    if (StartsIdent(c, Peek(1))) {
      if (GatherText(IS_IDCHAR, ident)) {
        type = eCSSToken_Dimension;
      }
    } else if (c == '%') {
      Advance();
      type = eCSSToken_Percentage;
      value = value / 100.0f;
      aToken.mIntegerValid = false;
    }
  }
  aToken.mNumber = value;
  aToken.mType = type;
  return true;
}






bool
nsCSSScanner::ScanString(nsCSSToken& aToken)
{
  int32_t aStop = Peek();
  MOZ_ASSERT(aStop == '"' || aStop == '\'', "should not have been called");
  aToken.mType = eCSSToken_String;
  aToken.mSymbol = PRUnichar(aStop); 
  Advance();

  for (;;) {
    GatherText(IS_STRING, aToken.mIdent);

    int32_t ch = Peek();
    if (ch == -1) {
      break; 
    }
    if (ch == aStop) {
      Advance();
      break;
    }
    
    if (ch == '"' || ch == '\'') {
      aToken.mIdent.Append(ch);
      Advance();
      continue;
    }

    aToken.mType = eCSSToken_Bad_String;
    mReporter->ReportUnexpected("SEUnterminatedString", aToken);
    break;
  }
  return true;
}

















bool
nsCSSScanner::ScanURange(nsCSSToken& aResult)
{
  int32_t intro1 = Peek();
  int32_t intro2 = Peek(1);
  int32_t ch = Peek(2);

  MOZ_ASSERT((intro1 == 'u' || intro1 == 'U') &&
             intro2 == '+' &&
             (IsHexDigit(ch) || ch == '?'),
             "should not have been called");

  aResult.mIdent.Append(intro1);
  aResult.mIdent.Append(intro2);
  Advance(2);

  bool valid = true;
  bool haveQues = false;
  uint32_t low = 0;
  uint32_t high = 0;
  int i = 0;

  do {
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

    i++;
    Advance();
    ch = Peek();
  } while (i < 6 && (IsHexDigit(ch) || ch == '?'));

  if (ch == '-' && IsHexDigit(Peek(1))) {
    if (haveQues) {
      valid = false;
    }

    aResult.mIdent.Append(ch);
    Advance();
    ch = Peek();
    high = 0;
    i = 0;
    do {
      aResult.mIdent.Append(ch);
      high = high*16 + HexDigitValue(ch);

      i++;
      Advance();
      ch = Peek();
    } while (i < 6 && IsHexDigit(ch));
  }

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

  int32_t ch = Peek();
  if (ch < 0) {
    return false;
  }

  
  aToken.mIdent.Truncate();

  
  if (ch == '"' || ch == '\'') {
    ScanString(aToken);
    if (MOZ_UNLIKELY(aToken.mType == eCSSToken_Bad_String)) {
      aToken.mType = eCSSToken_Bad_URL;
      return true;
    }
    MOZ_ASSERT(aToken.mType == eCSSToken_String, "unexpected token type");

  } else {
    
    aToken.mSymbol = PRUnichar(0);
    GatherText(IS_URL_CHAR, aToken.mIdent);
  }

  
  SkipWhitespace();
  ch = Peek();
  if (MOZ_LIKELY(ch < 0 || ch == ')')) {
    Advance();
    aToken.mType = eCSSToken_URL;
  } else {
    aToken.mType = eCSSToken_Bad_URL;
  }
  return true;
}











bool
nsCSSScanner::Next(nsCSSToken& aToken, bool aSkipWS)
{
  int32_t ch;

  
  aToken.mIdent.Truncate();
  aToken.mType = eCSSToken_Symbol;

  for (;;) {
    
    
    mTokenOffset = mOffset;
    mTokenLineOffset = mLineOffset;
    mTokenLineNumber = mLineNumber;

    ch = Peek();
    if (IsWhitespace(ch)) {
      SkipWhitespace();
      if (!aSkipWS) {
        aToken.mType = eCSSToken_Whitespace;
        return true;
      }
      continue; 
    }
    if (ch == '/' && !IsSVGMode() && Peek(1) == '*') {
      
      SkipComment();
      continue; 
    }
    break;
  }

  
  if (ch < 0) {
    return false;
  }

  
  if (ch == 'u' || ch == 'U') {
    int32_t c2 = Peek(1);
    int32_t c3 = Peek(2);
    if (c2 == '+' && (IsHexDigit(c3) || c3 == '?')) {
      return ScanURange(aToken);
    }
    return ScanIdent(aToken);
  }

  
  if (IsIdentStart(ch)) {
    return ScanIdent(aToken);
  }

  
  if (IsDigit(ch)) {
    return ScanNumber(aToken);
  }

  if (ch == '.' && IsDigit(Peek(1))) {
    return ScanNumber(aToken);
  }

  if (ch == '+') {
    int32_t c2 = Peek(1);
    if (IsDigit(c2) || (c2 == '.' && IsDigit(Peek(2)))) {
      return ScanNumber(aToken);
    }
  }

  
  
  if (ch == '-') {
    int32_t c2 = Peek(1);
    int32_t c3 = Peek(2);
    if (IsIdentStart(c2)) {
      return ScanIdent(aToken);
    }
    if (IsDigit(c2) || (c2 == '.' && IsDigit(c3))) {
      return ScanNumber(aToken);
    }
    if (c2 == '-' && c3 == '>') {
      Advance(3);
      aToken.mType = eCSSToken_HTMLComment;
      aToken.mIdent.AssignLiteral("-->");
      return true;
    }
  }

  
  if (ch == '<' && Peek(1) == '!' && Peek(2) == '-' && Peek(3) == '-') {
    Advance(4);
    aToken.mType = eCSSToken_HTMLComment;
    aToken.mIdent.AssignLiteral("<!--");
    return true;
  }

  
  if (ch == '@') {
    return ScanAtKeyword(aToken);
  }

  
  if (ch == '#') {
    return ScanHash(aToken);
  }

  
  if (ch == '"' || ch == '\'') {
    return ScanString(aToken);
  }

  
  nsCSSTokenType opType = MatchOperatorType(ch);
  if (opType != eCSSToken_Symbol && Peek(1) == '=') {
    aToken.mType = opType;
    Advance(2);
    return true;
  }

  
  aToken.mSymbol = ch;
  Advance();
  return true;
}
