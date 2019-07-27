






#include "nsJISx4051LineBreaker.h"

#include "jisx4051class.h"
#include "nsComplexBreaker.h"
#include "nsTArray.h"














































































































































































































#define MAX_CLASSES 12

static const uint16_t gPair[MAX_CLASSES] = {
  0x0FFF,
  0x0C02,
  0x0806,
  0x0842,
  0x0802,
  0x0C02,
  0x0ED2,
  0x0EC2,
  0x0902,
  0x0FFF,
  0x0CC2,
  0x0FFF
};












































static const uint16_t gPairConservative[MAX_CLASSES] = {
  0x0FFF,
  0x0EC2,
  0x0EC6,
  0x0EC2,
  0x0EC2,
  0x0C02,
  0x0FDF,
  0x0FDF,
  0x0FC2,
  0x0FFF,
  0x0FDF,
  0x0FFF
};



































#define CLASS_NONE                             INT8_MAX

#define CLASS_OPEN                             0x00
#define CLASS_CLOSE                            0x01
#define CLASS_NON_BREAKABLE_BETWEEN_SAME_CLASS 0x02
#define CLASS_PREFIX                           0x03
#define CLASS_POSTFFIX                         0x04
#define CLASS_BREAKABLE                        0x05
#define CLASS_NUMERIC                          0x06
#define CLASS_CHARACTER                        0x07
#define CLASS_COMPLEX                          0x08
#define CLASS_OPEN_LIKE_CHARACTER              0x09
#define CLASS_CLOSE_LIKE_CHARACTER             0x0A
#define CLASS_NON_BREAKABLE                    0x0B

#define U_NULL      char16_t(0x0000)
#define U_SLASH     char16_t('/')
#define U_SPACE     char16_t(' ')
#define U_HYPHEN    char16_t('-')
#define U_EQUAL     char16_t('=')
#define U_PERCENT   char16_t('%')
#define U_AMPERSAND char16_t('&')
#define U_SEMICOLON char16_t(';')
#define U_BACKSLASH char16_t('\\')
#define U_OPEN_SINGLE_QUOTE char16_t(0x2018)
#define U_OPEN_DOUBLE_QUOTE char16_t(0x201C)
#define U_OPEN_GUILLEMET    char16_t(0x00AB)

#define NEED_CONTEXTUAL_ANALYSIS(c) (IS_HYPHEN(c) || \
                                     (c) == U_SLASH || \
                                     (c) == U_PERCENT || \
                                     (c) == U_AMPERSAND || \
                                     (c) == U_SEMICOLON || \
                                     (c) == U_BACKSLASH || \
                                     (c) == U_OPEN_SINGLE_QUOTE || \
                                     (c) == U_OPEN_DOUBLE_QUOTE || \
                                     (c) == U_OPEN_GUILLEMET)

#define IS_ASCII_DIGIT(u) (0x0030 <= (u) && (u) <= 0x0039)

static inline int
GETCLASSFROMTABLE(const uint32_t* t, uint16_t l)
{
  return ((((t)[(l>>3)]) >> ((l & 0x0007)<<2)) & 0x000f);
}

static inline int
IS_HALFWIDTH_IN_JISx4051_CLASS3(char16_t u)
{
  return ((0xff66 <= (u)) && ((u) <= 0xff70));
}

static inline int
IS_CJK_CHAR(char16_t u)
{
  return ((0x1100 <= (u) && (u) <= 0x11ff) ||
          (0x2e80 <= (u) && (u) <= 0xd7ff) ||
          (0xf900 <= (u) && (u) <= 0xfaff) ||
          (0xff00 <= (u) && (u) <= 0xffef) );
}

static inline bool
IS_NONBREAKABLE_SPACE(char16_t u)
{
  return u == 0x00A0 || u == 0x2007; 
}

static inline bool
IS_HYPHEN(char16_t u)
{
  return (u == U_HYPHEN ||
          u == 0x058A || 
          u == 0x2010 || 
          u == 0x2012 || 
          u == 0x2013);  
}

static int8_t
GetClass(char16_t u)
{
   uint16_t h = u & 0xFF00;
   uint16_t l = u & 0x00ff;
   int8_t c;

   
   if (0x0000 == h) {
     c = GETCLASSFROMTABLE(gLBClass00, l);
   } else if (0x1700 == h) {
     c = GETCLASSFROMTABLE(gLBClass17, l);
   } else if (NS_NeedsPlatformNativeHandling(u)) {
     c = CLASS_COMPLEX;
   } else if (0x0E00 == h) {
     c = GETCLASSFROMTABLE(gLBClass0E, l);
   } else if (0x2000 == h) {
     c = GETCLASSFROMTABLE(gLBClass20, l);
   } else if (0x2100 == h) {
     c = GETCLASSFROMTABLE(gLBClass21, l);
   } else if (0x3000 == h) {
     c = GETCLASSFROMTABLE(gLBClass30, l);
   } else if (((0x3200 <= u) && (u <= 0xA4CF)) || 
              ((0xAC00 <= h) && (h <= 0xD7FF)) || 
              ((0xf900 <= h) && (h <= 0xfaff))) {
     c = CLASS_BREAKABLE; 
   } else if (0xff00 == h) {
     if (l < 0x0060) { 
       c = GETCLASSFROMTABLE(gLBClass00, (l+0x20));
     } else if (l < 0x00a0) {
       switch (l) {
         case 0x61: c = GetClass(0x3002); break;
         case 0x62: c = GetClass(0x300c); break;
         case 0x63: c = GetClass(0x300d); break;
         case 0x64: c = GetClass(0x3001); break;
         case 0x65: c = GetClass(0x30fb); break;
         case 0x9e: c = GetClass(0x309b); break;
         case 0x9f: c = GetClass(0x309c); break;
         default:
           if (IS_HALFWIDTH_IN_JISx4051_CLASS3(u))
              c = CLASS_CLOSE; 
           else
              c = CLASS_BREAKABLE; 
           break;
       }
     
     } else if (l < 0x00e0) {
       c = CLASS_CHARACTER; 
     } else if (l < 0x00f0) {
       static char16_t NarrowFFEx[16] = {
         0x00A2, 0x00A3, 0x00AC, 0x00AF, 0x00A6, 0x00A5, 0x20A9, 0x0000,
         0x2502, 0x2190, 0x2191, 0x2192, 0x2193, 0x25A0, 0x25CB, 0x0000
       };
       c = GetClass(NarrowFFEx[l - 0x00e0]);
     } else {
       c = CLASS_CHARACTER;
     }
   } else if (0x3100 == h) { 
     if (l <= 0xbf) { 
                      
                      
       c = CLASS_BREAKABLE;
     } else if (l >= 0xf0) { 
       c = CLASS_CLOSE;
     } else { 
       c = CLASS_CHARACTER;
     }
   } else if (0x0300 == h) {
     if (0x4F == l || (0x5C <= l && l <= 0x62))
       c = CLASS_NON_BREAKABLE;
     else
       c = CLASS_CHARACTER;
   } else if (0x0500 == h) {
     
     if (l == 0x8A)
       c = GETCLASSFROMTABLE(gLBClass00, uint16_t(U_HYPHEN));
     else
       c = CLASS_CHARACTER;
   } else if (0x0F00 == h) {
     if (0x08 == l || 0x0C == l || 0x12 == l)
       c = CLASS_NON_BREAKABLE;
     else
       c = CLASS_CHARACTER;
   } else if (0x1800 == h) {
     if (0x0E == l)
       c = CLASS_NON_BREAKABLE;
     else
       c = CLASS_CHARACTER;
   } else if (0x1600 == h) {
     if (0x80 == l) { 
       c = CLASS_BREAKABLE;
     } else {
       c = CLASS_CHARACTER;
     }
   } else if (u == 0xfeff) {
     c = CLASS_NON_BREAKABLE;
   } else {
     c = CLASS_CHARACTER; 
   }
   return c;
}

static bool
GetPair(int8_t c1, int8_t c2)
{
  NS_ASSERTION(c1 < MAX_CLASSES ,"illegal classes 1");
  NS_ASSERTION(c2 < MAX_CLASSES ,"illegal classes 2");

  return (0 == ((gPair[c1] >> c2) & 0x0001));
}

static bool
GetPairConservative(int8_t c1, int8_t c2)
{
  NS_ASSERTION(c1 < MAX_CLASSES ,"illegal classes 1");
  NS_ASSERTION(c2 < MAX_CLASSES ,"illegal classes 2");

  return (0 == ((gPairConservative[c1] >> c2) & 0x0001));
}

nsJISx4051LineBreaker::nsJISx4051LineBreaker()
{
}

nsJISx4051LineBreaker::~nsJISx4051LineBreaker()
{
}

NS_IMPL_ISUPPORTS(nsJISx4051LineBreaker, nsILineBreaker)

class ContextState {
public:
  ContextState(const char16_t* aText, uint32_t aLength) {
    mUniText = aText;
    mText = nullptr;
    mLength = aLength;
    Init();
  }

  ContextState(const uint8_t* aText, uint32_t aLength) {
    mUniText = nullptr;
    mText = aText;
    mLength = aLength;
    Init();
  }

  uint32_t Length() { return mLength; }
  uint32_t Index() { return mIndex; }

  char16_t GetCharAt(uint32_t aIndex) {
    NS_ASSERTION(aIndex < mLength, "Out of range!");
    return mUniText ? mUniText[aIndex] : char16_t(mText[aIndex]);
  }

  void AdvanceIndex() {
    ++mIndex;
  }

  void NotifyBreakBefore() { mLastBreakIndex = mIndex; }









#define CONSERVATIVE_BREAK_RANGE 6

  bool UseConservativeBreaking(uint32_t aOffset = 0) {
    if (mHasCJKChar)
      return false;
    uint32_t index = mIndex + aOffset;
    bool result = (index < CONSERVATIVE_BREAK_RANGE ||
                     mLength - index < CONSERVATIVE_BREAK_RANGE ||
                     index - mLastBreakIndex < CONSERVATIVE_BREAK_RANGE);
    if (result || !mHasNonbreakableSpace)
      return result;

    
    

    
    for (uint32_t i = index; index - CONSERVATIVE_BREAK_RANGE < i; --i) {
      if (IS_NONBREAKABLE_SPACE(GetCharAt(i - 1)))
        return true;
    }
    
    for (uint32_t i = index + 1; i < index + CONSERVATIVE_BREAK_RANGE; ++i) {
      if (IS_NONBREAKABLE_SPACE(GetCharAt(i)))
        return true;
    }
    return false;
  }

  bool HasPreviousEqualsSign() const {
    return mHasPreviousEqualsSign;
  }
  void NotifySeenEqualsSign() {
    mHasPreviousEqualsSign = true;
  }

  bool HasPreviousSlash() const {
    return mHasPreviousSlash;
  }
  void NotifySeenSlash() {
    mHasPreviousSlash = true;
  }

  bool HasPreviousBackslash() const {
    return mHasPreviousBackslash;
  }
  void NotifySeenBackslash() {
    mHasPreviousBackslash = true;
  }

  char16_t GetPreviousNonHyphenCharacter() const {
    return mPreviousNonHyphenCharacter;
  }
  void NotifyNonHyphenCharacter(char16_t ch) {
    mPreviousNonHyphenCharacter = ch;
  }

private:
  void Init() {
    mIndex = 0;
    mLastBreakIndex = 0;
    mPreviousNonHyphenCharacter = U_NULL;
    mHasCJKChar = 0;
    mHasNonbreakableSpace = 0;
    mHasPreviousEqualsSign = false;
    mHasPreviousSlash = false;
    mHasPreviousBackslash = false;

    for (uint32_t i = 0; i < mLength; ++i) {
      char16_t u = GetCharAt(i);
      if (!mHasNonbreakableSpace && IS_NONBREAKABLE_SPACE(u))
        mHasNonbreakableSpace = 1;
      else if (mUniText && !mHasCJKChar && IS_CJK_CHAR(u))
        mHasCJKChar = 1;
    }
  }

  const char16_t* mUniText;
  const uint8_t* mText;

  uint32_t mIndex;
  uint32_t mLength;         
  uint32_t mLastBreakIndex;
  char16_t mPreviousNonHyphenCharacter; 
                                         
  bool mHasCJKChar; 
  bool mHasNonbreakableSpace; 
                                     
  bool mHasPreviousEqualsSign; 
  bool mHasPreviousSlash;      
  bool mHasPreviousBackslash;  
};

static int8_t
ContextualAnalysis(char16_t prev, char16_t cur, char16_t next,
                   ContextState &aState)
{
  

  if (IS_HYPHEN(cur)) {
    
    if (IS_HYPHEN(next))
      return CLASS_CHARACTER;
    
    
    bool prevIsNum = IS_ASCII_DIGIT(prev);
    bool nextIsNum = IS_ASCII_DIGIT(next);
    if (prevIsNum && nextIsNum)
      return CLASS_NUMERIC;
    
    
    if (!aState.UseConservativeBreaking(1)) {
      char16_t prevOfHyphen = aState.GetPreviousNonHyphenCharacter();
      if (prevOfHyphen && next) {
        int8_t prevClass = GetClass(prevOfHyphen);
        int8_t nextClass = GetClass(next);
        bool prevIsNumOrCharOrClose =
          prevIsNum ||
          (prevClass == CLASS_CHARACTER &&
            !NEED_CONTEXTUAL_ANALYSIS(prevOfHyphen)) ||
          prevClass == CLASS_CLOSE ||
          prevClass == CLASS_CLOSE_LIKE_CHARACTER;
        bool nextIsNumOrCharOrOpen =
          nextIsNum ||
          (nextClass == CLASS_CHARACTER && !NEED_CONTEXTUAL_ANALYSIS(next)) ||
          nextClass == CLASS_OPEN ||
          nextClass == CLASS_OPEN_LIKE_CHARACTER ||
          next == U_OPEN_SINGLE_QUOTE ||
          next == U_OPEN_DOUBLE_QUOTE ||
          next == U_OPEN_GUILLEMET;
        if (prevIsNumOrCharOrClose && nextIsNumOrCharOrOpen) {
          return CLASS_CLOSE;
        }
      }
    }
  } else {
    aState.NotifyNonHyphenCharacter(cur);
    if (cur == U_SLASH || cur == U_BACKSLASH) {
      
      if (prev == cur)
        return CLASS_CHARACTER;
      
      
      bool shouldReturn = !aState.UseConservativeBreaking() &&
        (cur == U_SLASH ?
         aState.HasPreviousSlash() : aState.HasPreviousBackslash());

      if (cur == U_SLASH) {
        aState.NotifySeenSlash();
      } else {
        aState.NotifySeenBackslash();
      }

      if (shouldReturn)
        return CLASS_OPEN;
    } else if (cur == U_PERCENT) {
      
      if (!aState.UseConservativeBreaking()) {
        if (aState.Index() >= 3 &&
            aState.GetCharAt(aState.Index() - 3) == U_PERCENT)
          return CLASS_OPEN;
        if (aState.Index() + 3 < aState.Length() &&
            aState.GetCharAt(aState.Index() + 3) == U_PERCENT)
          return CLASS_OPEN;
      }
    } else if (cur == U_AMPERSAND || cur == U_SEMICOLON) {
      
      if (!aState.UseConservativeBreaking(1) &&
          aState.HasPreviousEqualsSign())
        return CLASS_CLOSE;
    } else if (cur == U_OPEN_SINGLE_QUOTE ||
               cur == U_OPEN_DOUBLE_QUOTE ||
               cur == U_OPEN_GUILLEMET) {
      
      
      
      if (!aState.UseConservativeBreaking() && IS_CJK_CHAR(next))
        return CLASS_OPEN;
    } else {
      NS_ERROR("Forgot to handle the current character!");
    }
  }
  return GetClass(cur);
}


int32_t
nsJISx4051LineBreaker::WordMove(const char16_t* aText, uint32_t aLen,
                                uint32_t aPos, int8_t aDirection)
{
  bool    textNeedsJISx4051 = false;
  int32_t begin, end;

  for (begin = aPos; begin > 0 && !NS_IsSpace(aText[begin - 1]); --begin) {
    if (IS_CJK_CHAR(aText[begin]) || NS_NeedsPlatformNativeHandling(aText[begin])) {
      textNeedsJISx4051 = true;
    }
  }
  for (end = aPos + 1; end < int32_t(aLen) && !NS_IsSpace(aText[end]); ++end) {
    if (IS_CJK_CHAR(aText[end]) || NS_NeedsPlatformNativeHandling(aText[end])) {
      textNeedsJISx4051 = true;
    }
  }

  int32_t ret;
  nsAutoTArray<uint8_t, 2000> breakState;
  if (!textNeedsJISx4051 || !breakState.AppendElements(end - begin)) {
    
    
    
    if (aDirection < 0) {
      ret = (begin == int32_t(aPos)) ? begin - 1 : begin;
    } else {
      ret = end;
    }
  } else {
    GetJISx4051Breaks(aText + begin, end - begin, nsILineBreaker::kWordBreak_Normal,
                      breakState.Elements());

    ret = aPos;
    do {
      ret += aDirection;
    } while (begin < ret && ret < end && !breakState[ret - begin]);
  }

  return ret;
}

int32_t
nsJISx4051LineBreaker::Next(const char16_t* aText, uint32_t aLen,
                            uint32_t aPos) 
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  NS_ASSERTION(aLen > aPos, "Bad position passed to nsJISx4051LineBreaker::Next");

  int32_t nextPos = WordMove(aText, aLen, aPos, 1);
  return nextPos < int32_t(aLen) ? nextPos : NS_LINEBREAKER_NEED_MORE_TEXT;
}

int32_t
nsJISx4051LineBreaker::Prev(const char16_t* aText, uint32_t aLen,
                            uint32_t aPos) 
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  NS_ASSERTION(aLen >= aPos && aPos > 0,
               "Bad position passed to nsJISx4051LineBreaker::Prev");

  int32_t prevPos = WordMove(aText, aLen, aPos, -1);
  return prevPos > 0 ? prevPos : NS_LINEBREAKER_NEED_MORE_TEXT;
}

void
nsJISx4051LineBreaker::GetJISx4051Breaks(const char16_t* aChars, uint32_t aLength,
                                         uint8_t aWordBreak,
                                         uint8_t* aBreakBefore)
{
  uint32_t cur;
  int8_t lastClass = CLASS_NONE;
  ContextState state(aChars, aLength);

  for (cur = 0; cur < aLength; ++cur, state.AdvanceIndex()) {
    char16_t ch = aChars[cur];
    int8_t cl;

    if (NEED_CONTEXTUAL_ANALYSIS(ch)) {
      cl = ContextualAnalysis(cur > 0 ? aChars[cur - 1] : U_NULL,
                              ch,
                              cur + 1 < aLength ? aChars[cur + 1] : U_NULL,
                              state);
    } else {
      if (ch == U_EQUAL)
        state.NotifySeenEqualsSign();
      state.NotifyNonHyphenCharacter(ch);
      cl = GetClass(ch);
    }

    bool allowBreak = false;
    if (cur > 0) {
      NS_ASSERTION(CLASS_COMPLEX != lastClass || CLASS_COMPLEX != cl,
                   "Loop should have prevented adjacent complex chars here");
      if (aWordBreak == nsILineBreaker::kWordBreak_Normal) {
        allowBreak = (state.UseConservativeBreaking()) ?
          GetPairConservative(lastClass, cl) : GetPair(lastClass, cl);
      } else if (aWordBreak == nsILineBreaker::kWordBreak_BreakAll) {
        allowBreak = true;
      }
    }
    aBreakBefore[cur] = allowBreak;
    if (allowBreak)
      state.NotifyBreakBefore();
    lastClass = cl;
    if (CLASS_COMPLEX == cl) {
      uint32_t end = cur + 1;

      while (end < aLength && CLASS_COMPLEX == GetClass(aChars[end])) {
        ++end;
      }

      NS_GetComplexLineBreaks(aChars + cur, end - cur, aBreakBefore + cur);

      
      if (aWordBreak != nsILineBreaker::kWordBreak_Normal) {
        
        for (uint32_t i = cur; i < end; i++)
          aBreakBefore[i] = (aWordBreak == nsILineBreaker::kWordBreak_BreakAll);
      }

      
      
      aBreakBefore[cur] = allowBreak;

      cur = end - 1;
    }
  }
}

void
nsJISx4051LineBreaker::GetJISx4051Breaks(const uint8_t* aChars, uint32_t aLength,
                                         uint8_t aWordBreak,
                                         uint8_t* aBreakBefore)
{
  uint32_t cur;
  int8_t lastClass = CLASS_NONE;
  ContextState state(aChars, aLength);

  for (cur = 0; cur < aLength; ++cur, state.AdvanceIndex()) {
    char16_t ch = aChars[cur];
    int8_t cl;

    if (NEED_CONTEXTUAL_ANALYSIS(ch)) {
      cl = ContextualAnalysis(cur > 0 ? aChars[cur - 1] : U_NULL,
                              ch,
                              cur + 1 < aLength ? aChars[cur + 1] : U_NULL,
                              state);
    } else {
      if (ch == U_EQUAL)
        state.NotifySeenEqualsSign();
      state.NotifyNonHyphenCharacter(ch);
      cl = GetClass(ch);
    }

    bool allowBreak = false;
    if (cur > 0) {
      if (aWordBreak == nsILineBreaker::kWordBreak_Normal) {
        allowBreak = (state.UseConservativeBreaking()) ?
          GetPairConservative(lastClass, cl) : GetPair(lastClass, cl);
      } else if (aWordBreak == nsILineBreaker::kWordBreak_BreakAll) {
        allowBreak = true;
      }
    }
    aBreakBefore[cur] = allowBreak;
    if (allowBreak)
      state.NotifyBreakBefore();
    lastClass = cl;
  }
}
