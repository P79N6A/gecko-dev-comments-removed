






































#include "nsJISx4501LineBreaker.h"

#include "pratom.h"
#include "nsLWBRKDll.h"
#include "jisx4501class.h"
#include "nsComplexBreaker.h"
#include "nsTArray.h"
#include "nsUnicharUtils.h"




















































































































































#define MAX_CLASSES 10

static const PRUint16 gPair[MAX_CLASSES] = {
  0x03FF, 
  0x0002, 
  0x0006, 
  0x0042, 
  0x0002, 
  0x0002, 
  0x0152, 
  0x0182, 
  0x01C2,
  0x0202
};


static inline int
GETCLASSFROMTABLE(const PRUint32* t, PRUint16 l)
{
  return ((((t)[(l>>3)]) >> ((l & 0x0007)<<2)) & 0x000f);
}

#define CLASS_COMPLEX 9



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

static inline int
IS_COMPLEX(PRUnichar u)
{
  return (0x0e01 <= (u) && (u) <= 0x0e5b);
}

static inline int
IS_SPACE(PRUnichar u)
{
  return ((u) == 0x0020 || (u) == 0x0009 || (u) == 0x000a || (u) == 0x000d || (u)==0x200b);
}

static PRInt8 GetClass(PRUnichar u)
{
   PRUint16 h = u & 0xFF00;
   PRUint16 l = u & 0x00ff;
   PRInt8 c;
   
   
   if( 0x0000 == h)
   {
     c = GETCLASSFROMTABLE(gLBClass00, l);
   } 
   else if( 0x0E00 == h)
   {
     c = GETCLASSFROMTABLE(gLBClass0E, l);
   }
   else if( 0x2000 == h)
   {
     c = GETCLASSFROMTABLE(gLBClass20, l);
   } 
   else if( 0x2100 == h)
   {
     c = GETCLASSFROMTABLE(gLBClass21, l);
   } 
   else if( 0x3000 == h)
   {
     c = GETCLASSFROMTABLE(gLBClass30, l);
   } 
   else if (  ( ( 0x3200 <= u) && ( u <= 0xA4CF) ) || 
              ( ( 0xAC00 <= h) && ( h <= 0xD7FF) ) || 
              ( ( 0xf900 <= h) && ( h <= 0xfaff) )
             )
   { 
     c = 5; 
   } 
   else if( 0xff00 == h)
   {
     if( l < 0x0060) 
     {
       c = GETCLASSFROMTABLE(gLBClass00, (l+0x20));
     } else if (l < 0x00a0) {
       switch (l)
       {
         case 0x61: c = GetClass(0x3002); break;
         case 0x62: c = GetClass(0x300c); break;
         case 0x63: c = GetClass(0x300d); break;
         case 0x64: c = GetClass(0x3001); break;
         case 0x65: c = GetClass(0x30fb); break;
         case 0x9e: c = GetClass(0x309b); break;
         case 0x9f: c = GetClass(0x309c); break;
         default:
           if(IS_HALFWIDTH_IN_JISx4051_CLASS3(u))
              c = 1; 
           else
              c = 5; 
           break;
       }
       
     } else if( l < 0x00e0) {
       c = 8; 
     } else if( l < 0x00f0) {
       static PRUnichar NarrowFFEx[16] = 
       { 
         0x00A2, 0x00A3, 0x00AC, 0x00AF, 0x00A6, 0x00A5, 0x20A9, 0x0000,
         0x2502, 0x2190, 0x2191, 0x2192, 0x2193, 0x25A0, 0x25CB, 0x0000
       };
       c = GetClass(NarrowFFEx[l - 0x00e0]);
     } else {
       c = 8;
     }
   }
   else if( 0x3100 == h) { 
     if ( l <= 0xbf) {  
                        
                        
       c = 5;
     }
     else if ( l >= 0xf0)
     {            
       c = 1;
     }
     else   
     {
       c = 8;
     }
   } 
   else {
     c = 8; 
   }
   return c;
}

static PRBool GetPair(PRInt8 c1, PRInt8 c2)
{
  NS_ASSERTION( c1 < MAX_CLASSES ,"illegal classes 1");
  NS_ASSERTION( c2 < MAX_CLASSES ,"illegal classes 2");

  return (0 == ((gPair[c1] >> c2 ) & 0x0001));
}

nsJISx4051LineBreaker::nsJISx4051LineBreaker()
{
}

nsJISx4051LineBreaker::~nsJISx4051LineBreaker()
{
}

NS_IMPL_ISUPPORTS1(nsJISx4051LineBreaker, nsILineBreaker)

#define U_PERIOD    PRUnichar('.')
#define U_COMMA     PRUnichar(',')
#define U_SEMICOLON PRUnichar(';')
#define U_SLASH     PRUnichar('/')
#define U_SPACE     PRUnichar(' ')
#define U_HYPHEN    PRUnichar('-')
#define U_EQUAL     PRUnichar('=')
#define U_NULL      PRUnichar(0x0000)
#define U_RIGHT_SINGLE_QUOTATION_MARK PRUnichar(0x2019)
#define NEED_CONTEXTUAL_ANALYSIS(c) ((c) == U_PERIOD || \
                                     (c) == U_COMMA || \
                                     (c) == U_SEMICOLON || \
                                     (c) == U_SLASH || \
                                     (c) == U_HYPHEN || \
                                     (c) == U_EQUAL || \
                                     (c) == U_RIGHT_SINGLE_QUOTATION_MARK)
#define NUMERIC_CLASS  6 // JIS x4051 class 15 is now map to simplified class 6
#define CHARACTER_CLASS  8 // JIS x4051 class 18 is now map to simplified class 8
#define IS_ASCII_DIGIT(u) (0x0030 <= (u) && (u) <= 0x0039)

static PRInt8 ContextualAnalysis(
  PRUnichar prev, PRUnichar cur, PRUnichar next)
{
   if(U_COMMA == cur || U_SEMICOLON == cur)
   {
     if((IS_ASCII_DIGIT(prev) || prev == U_NULL) && IS_ASCII_DIGIT(next))
       return NUMERIC_CLASS;
   }
   else if(U_PERIOD == cur)
   {
     if((IS_ASCII_DIGIT(prev) || prev == U_SPACE || prev == U_NULL) &&
         IS_ASCII_DIGIT(next))
       return NUMERIC_CLASS;

     
     
     
     
     
     
     
     PRUint8 pc = prev != U_NULL ? GetClass(prev) : CHARACTER_CLASS;
     if((pc > 5 || pc == 0)  && GetClass(next) > 5)
       return CHARACTER_CLASS;
   }
   else if(U_SLASH == cur || U_HYPHEN == cur || U_EQUAL == cur)
   {
     
     if (U_SLASH == cur && prev == U_NULL)
       return CHARACTER_CLASS;
     if (IS_ASCII_DIGIT(next))
       return NUMERIC_CLASS;
   }
   else if(U_RIGHT_SINGLE_QUOTATION_MARK == cur)
   {
     
     if(U_SPACE != next)
       return CHARACTER_CLASS;
   }
   return GetClass(cur);
}


PRInt32 nsJISx4051LineBreaker::WordMove(
  const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos, PRInt8 aDirection)
{
  PRBool  textNeedsJISx4051 = PR_FALSE;
  PRInt32 begin, end;

  for (begin = aPos; begin > 0 && !IS_SPACE(aText[begin - 1]); --begin) {
    if (IS_CJK_CHAR(aText[begin]) || IS_COMPLEX(aText[begin])) {
      textNeedsJISx4051 = PR_TRUE;
    }
  }
  for (end = aPos + 1; end < PRInt32(aLen) && !IS_SPACE(aText[end]); ++end) {
    if (IS_CJK_CHAR(aText[end]) || IS_COMPLEX(aText[end])) {
      textNeedsJISx4051 = PR_TRUE;
    }
  }

  PRInt32 ret;
  nsAutoTArray<PRPackedBool, 2000> breakState;
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

PRInt32 nsJISx4051LineBreaker::Next(
  const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos) 
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  NS_ASSERTION(aLen > aPos, "Illegal value (length > position)");

  PRInt32 nextPos = WordMove(aText, aLen, aPos, 1);
  return nextPos < PRInt32(aLen) ? nextPos : NS_LINEBREAKER_NEED_MORE_TEXT;
}

PRInt32 nsJISx4051LineBreaker::Prev( 
  const PRUnichar* aText, PRUint32 aLen, PRUint32 aPos) 
{
  NS_ASSERTION(aText, "aText shouldn't be null");
  NS_ASSERTION(aLen >= aPos, "Illegal value (length >= position)");

  PRInt32 prevPos = WordMove(aText, aLen, aPos, -1);
  return prevPos > 0 ? prevPos : NS_LINEBREAKER_NEED_MORE_TEXT;
}

void
nsJISx4051LineBreaker::GetJISx4051Breaks(const PRUnichar* aChars, PRUint32 aLength,
                                         PRPackedBool* aBreakBefore)
{
  PRUint32 cur;
  PRInt8 lastClass = -1;

  for (cur = 0; cur < aLength; ++cur) {
    PRUnichar ch = aChars[cur];
    PRInt8 cl;

    if (NEED_CONTEXTUAL_ANALYSIS(ch)) {
      cl = ContextualAnalysis(cur > 0 ? aChars[cur - 1] : U_NULL,
                              ch,
                              cur + 1 < aLength ? aChars[cur + 1] : U_NULL);
    } else {
      cl = GetClass(ch);
    }

    PRBool allowBreak;
    if (cur > 0) {
      NS_ASSERTION(CLASS_COMPLEX != lastClass || CLASS_COMPLEX != cl,
                   "Loop should have prevented adjacent complex chars here");
      allowBreak = GetPair(lastClass, cl);
    } else {
      allowBreak = PR_FALSE;
    }
    aBreakBefore[cur] = allowBreak;
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
                                         PRPackedBool* aBreakBefore)
{
  PRUint32 cur;
  PRInt8 lastClass = -1;

  for (cur = 0; cur < aLength; ++cur) {
    PRUnichar ch = aChars[cur];
    PRInt8 cl;

    if (NEED_CONTEXTUAL_ANALYSIS(ch)) {
      cl = ContextualAnalysis(cur > 0 ? aChars[cur - 1] : U_NULL,
                              ch,
                              cur + 1 < aLength ? aChars[cur + 1] : U_NULL);
    } else {
      cl = GetClass(ch);
    }

    PRBool allowBreak;
    if (cur > 0) {
      allowBreak = GetPair(lastClass, cl);
    } else {
      allowBreak = PR_FALSE;
    }
    aBreakBefore[cur] = allowBreak;
    lastClass = cl;
  }
}
