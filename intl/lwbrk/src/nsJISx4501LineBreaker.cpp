






































#include "nsJISx4501LineBreaker.h"

#include "pratom.h"
#include "nsLWBRKDll.h"
#include "jisx4501class.h"
#include "nsComplexBreaker.h"
#include "nsTArray.h"
#include "nsUnicharUtils.h"














































































































































































































#define MAX_CLASSES 12

static const PRUint16 gPair[MAX_CLASSES] = {
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












































static const PRUint16 gPairConservative[MAX_CLASSES] = {
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



































#define CLASS_NONE                             PR_INT8_MAX

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

#define U_NULL      PRUnichar(0x0000)
#define U_SLASH     PRUnichar('/')
#define U_SPACE     PRUnichar(' ')
#define U_HYPHEN    PRUnichar('-')
#define U_EQUAL     PRUnichar('=')
#define U_PERCENT   PRUnichar('%')
#define U_AMPERSAND PRUnichar('&')
#define U_SEMICOLON PRUnichar(';')
#define U_BACKSLASH PRUnichar('\\')
#define U_OPEN_SINGLE_QUOTE PRUnichar(0x2018)
#define U_OPEN_DOUBLE_QUOTE PRUnichar(0x201C)
#define U_OPEN_GUILLEMET    PRUnichar(0x00AB)

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
GETCLASSFROMTABLE(const PRUint32* t, PRUint16 l)
{
  return ((((t)[(l>>3)]) >> ((l & 0x0007)<<2)) & 0x000f);
}

static inline int
IS_HALFWIDTH_IN_JISx4051_CLASS3(PRUnichar u)
{
  return ((0xff66 <= (u)) && ((u) <= 0xff70));
}

static inline int
IS_CJK_CHAR(PRUnichar u)
{
  return ((0x1100 <= (u) && (u) <= 0x11ff) ||
          (0x2e80 <= (u) && (u) <= 0xd7ff) ||
          (0xf900 <= (u) && (u) <= 0xfaff) ||
          (0xff00 <= (u) && (u) <= 0xffef) );
}

static inline bool
IS_NONBREAKABLE_SPACE(PRUnichar u)
{
  return u == 0x00A0 || u == 0x2007; 
}

static inline bool
IS_HYPHEN(PRUnichar u)
{
  return (u == U_HYPHEN ||
          u == 0x058A || 
          u == 0x2010 || 
          u == 0x2012 || 
          u == 0x2013);  
}

static PRInt8
GetClass(PRUnichar u)
{
   PRUint16 h = u & 0xFF00;
   PRUint16 l = u & 0x00ff;
   PRInt8 c;

   
   if (0x0000 == h) {
     c = GETCLASSFROMTABLE(gLBClass00, l);
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
       static PRUnichar NarrowFFEx[16] = {
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
       c = GETCLASSFROMTABLE(gLBClass00, PRUint16(U_HYPHEN));
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
   } else {
     c = CLASS_CHARACTER; 
   }
   return c;
}

static bool
GetPair(PRInt8 c1, PRInt8 c2)
{
  NS_ASSERTION(c1 < MAX_CLASSES ,"illegal classes 1");
  NS_ASSERTION(c2 < MAX_CLASSES ,"illegal classes 2");

  return (0 == ((gPair[c1] >> c2) & 0x0001));
}

static bool
GetPairConservative(PRInt8 c1, PRInt8 c2)
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

NS_IMPL_ISUPPORTS1(nsJISx4051LineBreaker, nsILineBreaker)

class ContextState {
public:
  ContextState(const PRUnichar* aText, PRUint32 aLength) {
    mUniText = aText;
    mText = nsnull;
    mLength = aLength;
    Init();
  }

  ContextState(const PRUint8* aText, PRUint32 aLength) {
    mUniText = nsnull;
    mText = aText;
    mLength = aLength;
    Init();
  }

  PRUint32 Length() { return mLength; }
  PRUint32 Index() { return mIndex; }

  PRUnichar GetCharAt(PRUint32 aIndex) {
    NS_ASSERTION(0 <= aIndex && aIndex < mLength, "Out of range!");
    return mUniText ? mUniText[aIndex] : PRUnichar(mText[aIndex]);
  }

  void AdvanceIndex() {
    ++mIndex;
  }

  void NotifyBreakBefore() { mLastBreakIndex = mIndex; }









#define CONSERVATIVE_BREAK_RANGE 6

  bool UseConservativeBreaking(PRUint32 aOffset = 0) {
    if (mHasCJKChar)
      return PR_FALSE;
    PRUint32 index = mIndex + aOffset;
    bool result = (index < CONSERVATIVE_BREAK_RANGE ||
                     mLength - index < CONSERVATIVE_BREAK_RANGE ||
                     index - mLastBreakIndex < CONSERVATIVE_BREAK_RANGE);
    if (result || !mHasNonbreakableSpace)
      return result;

    
    

    
    for (PRUint32 i = index; index - CONSERVATIVE_BREAK_RANGE < i; --i) {
      if (IS_NONBREAKABLE_SPACE(GetCharAt(i - 1)))
        return PR_TRUE;
    }
    
    for (PRUint32 i = index + 1; i < index + CONSERVATIVE_BREAK_RANGE; ++i) {
      if (IS_NONBREAKABLE_SPACE(GetCharAt(i)))
        return PR_TRUE;
    }
    return PR_FALSE;
  }

  bool HasPreviousEqualsSign() const {
    return mHasPreviousEqualsSign;
  }
  void NotifySeenEqualsSign() {
    mHasPreviousEqualsSign = PR_TRUE;
  }

  bool HasPreviousSlash() const {
    return mHasPreviousSlash;
  }
  void NotifySeenSlash() {
    mHasPreviousSlash = PR_TRUE;
  }

  bool HasPreviousBackslash() const {
    return mHasPreviousBackslash;
  }
  void NotifySeenBackslash() {
    mHasPreviousBackslash = PR_TRUE;
  }

  PRUnichar GetPreviousNonHyphenCharacter() const {
    return mPreviousNonHyphenCharacter;
  }
  void NotifyNonHyphenCharacter(PRUnichar ch) {
    mPreviousNonHyphenCharacter = ch;
  }

private:
  void Init() {
    mIndex = 0;
    mLastBreakIndex = 0;
    mPreviousNonHyphenCharacter = U_NULL;
    mHasCJKChar = 0;
    mHasNonbreakableSpace = 0;
    mHasPreviousEqualsSign = PR_FALSE;
    mHasPreviousSlash = PR_FALSE;
    mHasPreviousBackslash = PR_FALSE;

    for (PRUint32 i = 0; i < mLength; ++i) {
      PRUnichar u = GetCharAt(i);
      if (!mHasNonbreakableSpace && IS_NONBREAKABLE_SPACE(u))
        mHasNonbreakableSpace = 1;
      else if (mUniText && !mHasCJKChar && IS_CJK_CHAR(u))
        mHasCJKChar = 1;
    }
  }

  const PRUnichar* mUniText;
  const PRUint8* mText;

  PRUint32 mIndex;
  PRUint32 mLength;         
  PRUint32 mLastBreakIndex;
  PRUnichar mPreviousNonHyphenCharacter; 
                                         
  bool mHasCJKChar; 
  bool mHasNonbreakableSpace; 
                                     
  bool mHasPreviousEqualsSign; 
  bool mHasPreviousSlash;      
  bool mHasPreviousBackslash;  
};

static PRInt8
ContextualAnalysis(PRUnichar prev, PRUnichar cur, PRUnichar next,
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
      PRUnichar prevOfHyphen = aState.GetPreviousNonHyphenCharacter();
      if (prevOfHyphen && next) {
        bool prevIsChar = !NEED_CONTEXTUAL_ANALYSIS(prevOfHyphen) &&
                            GetClass(prevOfHyphen) == CLASS_CHARACTER;
        bool nextIsChar = !NEED_CONTEXTUAL_ANALYSIS(next) &&
                            GetClass(next) == CLASS_CHARACTER;
        if ((prevIsNum || prevIsChar) && (nextIsNum || nextIsChar))
          return CLASS_CLOSE;
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


PRInt32
nsJISx4051LineBreaker::WordMove(const PRUnichar* aText, PRUint32 aLen,
                                PRUint32 aPos, PRInt8 aDirection)
{
  bool    textNeedsJISx4051 = false;
  PRInt32 begin, end;

  for (begin = aPos; begin > 0 && !NS_IsSpace(aText[begin - 1]); --begin) {
    if (IS_CJK_CHAR(aText[begin]) || NS_NeedsPlatformNativeHandling(aText[begin])) {
      textNeedsJISx4051 = PR_TRUE;
    }
  }
  for (end = aPos + 1; end < PRInt32(aLen) && !NS_IsSpace(aText[end]); ++end) {
    if (IS_CJK_CHAR(aText[end]) || NS_NeedsPlatformNativeHandling(aText[end])) {
      textNeedsJISx4051 = PR_TRUE;
    }
  }

  PRInt32 ret;
  nsAutoTArray<PRUint8, 2000> breakState;
  if (!textNeedsJISx4051 || !breakState.AppendElements(end - begin)) {
    
    
    
    if (aDirection < 0) {
      ret = (begin == PRInt32(aPos)) ? begin - 1 : begin;
    } else {
      ret = end;
    }
  } else {
    GetJISx4051Breaks(aText + begin, end - begin, breakState.Elements());

    ret = aPos;
    do {
      ret += aDirection;
    } while (begin < ret && ret < end && !breakState[ret - begin]);
  }

  return ret;
}

PRInt32
nsJISx4051LineBreaker::Next(const PRUnichar* aText, PRUint32 aLen,
                            PRUint32 aPos) 
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  NS_ASSERTION(aLen > aPos, "Bad position passed to nsJISx4051LineBreaker::Next");

  PRInt32 nextPos = WordMove(aText, aLen, aPos, 1);
  return nextPos < PRInt32(aLen) ? nextPos : NS_LINEBREAKER_NEED_MORE_TEXT;
}

PRInt32
nsJISx4051LineBreaker::Prev(const PRUnichar* aText, PRUint32 aLen,
                            PRUint32 aPos) 
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  NS_ASSERTION(aLen >= aPos && aPos > 0,
               "Bad position passed to nsJISx4051LineBreaker::Prev");

  PRInt32 prevPos = WordMove(aText, aLen, aPos, -1);
  return prevPos > 0 ? prevPos : NS_LINEBREAKER_NEED_MORE_TEXT;
}

void
nsJISx4051LineBreaker::GetJISx4051Breaks(const PRUnichar* aChars, PRUint32 aLength,
                                         PRUint8* aBreakBefore)
{
  PRUint32 cur;
  PRInt8 lastClass = CLASS_NONE;
  ContextState state(aChars, aLength);

  for (cur = 0; cur < aLength; ++cur, state.AdvanceIndex()) {
    PRUnichar ch = aChars[cur];
    PRInt8 cl;

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

    bool allowBreak;
    if (cur > 0) {
      NS_ASSERTION(CLASS_COMPLEX != lastClass || CLASS_COMPLEX != cl,
                   "Loop should have prevented adjacent complex chars here");
      if (state.UseConservativeBreaking())
        allowBreak = GetPairConservative(lastClass, cl);
      else
        allowBreak = GetPair(lastClass, cl);
    } else {
      allowBreak = PR_FALSE;
    }
    aBreakBefore[cur] = allowBreak;
    if (allowBreak)
      state.NotifyBreakBefore();
    lastClass = cl;
    if (CLASS_COMPLEX == cl) {
      PRUint32 end = cur + 1;

      while (end < aLength && CLASS_COMPLEX == GetClass(aChars[end])) {
        ++end;
      }

      NS_GetComplexLineBreaks(aChars + cur, end - cur, aBreakBefore + cur);

      
      
      aBreakBefore[cur] = allowBreak;

      cur = end - 1;
    }
  }
}

void
nsJISx4051LineBreaker::GetJISx4051Breaks(const PRUint8* aChars, PRUint32 aLength,
                                         PRUint8* aBreakBefore)
{
  PRUint32 cur;
  PRInt8 lastClass = CLASS_NONE;
  ContextState state(aChars, aLength);

  for (cur = 0; cur < aLength; ++cur, state.AdvanceIndex()) {
    PRUnichar ch = aChars[cur];
    PRInt8 cl;

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

    bool allowBreak;
    if (cur > 0) {
      if (state.UseConservativeBreaking())
        allowBreak = GetPairConservative(lastClass, cl);
      else
        allowBreak = GetPair(lastClass, cl);
    } else {
      allowBreak = PR_FALSE;
    }
    aBreakBefore[cur] = allowBreak;
    if (allowBreak)
      state.NotifyBreakBefore();
    lastClass = cl;
  }
}
