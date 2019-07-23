








































#include "nsBidiUtils.h"
#include "symmtable.h"
#include "bidicattable.h"

#define FE_TO_06_OFFSET 0xfe70

static const PRUnichar FE_TO_06 [][2] = {
    {0x064b,0x0000},{0x064b,0x0640},{0x064c,0x0000},
    {0x0000,0x0000},{0x064d,0x0000},{0x0000,0x0000},
    {0x064e,0x0000},{0x064e,0x0640},{0x064f,0x0000},
    {0x064f,0x0640},{0x0650,0x0000},{0x0650,0x0640},
    {0x0651,0x0000},{0x0651,0x0640},{0x0652,0x0000},
    {0x0652,0x0640},{0x0621,0x0000},{0x0622,0x0000},
    {0x0622,0x0000},{0x0623,0x0000},{0x0623,0x0000},
    {0x0624,0x0000},{0x0624,0x0000},{0x0625,0x0000},
    {0x0625,0x0000},{0x0626,0x0000},{0x0626,0x0000},
    {0x0626,0x0000},{0x0626,0x0000},{0x0627,0x0000},
    {0x0627,0x0000},{0x0628,0x0000},{0x0628,0x0000},
    {0x0628,0x0000},{0x0628,0x0000},{0x0629,0x0000},
    {0x0629,0x0000},{0x062a,0x0000},{0x062a,0x0000},
    {0x062a,0x0000},{0x062a,0x0000},{0x062b,0x0000},
    {0x062b,0x0000},{0x062b,0x0000},{0x062b,0x0000},
    {0x062c,0x0000},{0x062c,0x0000},{0x062c,0x0000},
    {0x062c,0x0000},{0x062d,0x0000},{0x062d,0x0000},
    {0x062d,0x0000},{0x062d,0x0000},{0x062e,0x0000},
    {0x062e,0x0000},{0x062e,0x0000},{0x062e,0x0000},
    {0x062f,0x0000},{0x062f,0x0000},{0x0630,0x0000},
    {0x0630,0x0000},{0x0631,0x0000},{0x0631,0x0000},
    {0x0632,0x0000},{0x0632,0x0000},{0x0633,0x0000},
    {0x0633,0x0000},{0x0633,0x0000},{0x0633,0x0000},
    {0x0634,0x0000},{0x0634,0x0000},{0x0634,0x0000},
    {0x0634,0x0000},{0x0635,0x0000},{0x0635,0x0000},
    {0x0635,0x0000},{0x0635,0x0000},{0x0636,0x0000},
    {0x0636,0x0000},{0x0636,0x0000},{0x0636,0x0000},
    {0x0637,0x0000},{0x0637,0x0000},{0x0637,0x0000},
    {0x0637,0x0000},{0x0638,0x0000},{0x0638,0x0000},
    {0x0638,0x0000},{0x0638,0x0000},{0x0639,0x0000},
    {0x0639,0x0000},{0x0639,0x0000},{0x0639,0x0000},
    {0x063a,0x0000},{0x063a,0x0000},{0x063a,0x0000},
    {0x063a,0x0000},{0x0641,0x0000},{0x0641,0x0000},
    {0x0641,0x0000},{0x0641,0x0000},{0x0642,0x0000},
    {0x0642,0x0000},{0x0642,0x0000},{0x0642,0x0000},
    {0x0643,0x0000},{0x0643,0x0000},{0x0643,0x0000},
    {0x0643,0x0000},{0x0644,0x0000},{0x0644,0x0000},
    {0x0644,0x0000},{0x0644,0x0000},{0x0645,0x0000},
    {0x0645,0x0000},{0x0645,0x0000},{0x0645,0x0000},
    {0x0646,0x0000},{0x0646,0x0000},{0x0646,0x0000},
    {0x0646,0x0000},{0x0647,0x0000},{0x0647,0x0000},
    {0x0647,0x0000},{0x0647,0x0000},{0x0648,0x0000},
    {0x0648,0x0000},{0x0649,0x0000},{0x0649,0x0000},
    {0x064a,0x0000},{0x064a,0x0000},{0x064a,0x0000},
    {0x064a,0x0000},{0x0644,0x0622},{0x0644,0x0622},
    {0x0644,0x0623},{0x0644,0x0623},{0x0644,0x0625},
    {0x0644,0x0625},{0x0644,0x0627},{0x0644,0x0627}
};

static const PRUnichar FB_TO_06 [] = {
    0x0671,0x0671,0x067B,0x067B,0x067B,0x067B,0x067E,0x067E, 
    0x067E,0x067E,0x0680,0x0680,0x0680,0x0680,0x067A,0x067A, 
    0x067A,0x067A,0x067F,0x067F,0x067F,0x067F,0x0679,0x0679, 
    0x0679,0x0679,0x06A4,0x06A4,0x06A4,0x06A4,0x06A6,0x06A6, 
    0x06A6,0x06A6,0x0684,0x0684,0x0684,0x0684,0x0683,0x0683, 
    0x0683,0x0683,0x0686,0x0686,0x0686,0x0686,0x0687,0x0687, 
    0x0687,0x0687,0x068D,0x068D,0x068C,0x068C,0x068E,0x068E, 
    0x0688,0x0688,0x0698,0x0698,0x0691,0x0691,0x06A9,0x06A9, 
    0x06A9,0x06A9,0x06AF,0x06AF,0x06AF,0x06AF,0x06B3,0x06B3, 
    0x06B3,0x06B3,0x06B1,0x06B1,0x06B1,0x06B1,0x06BA,0x06BA, 
    0x06BB,0x06BB,0x06BB,0x06BB,0x06C0,0x06C0,0x06C1,0x06C1, 
    0x06C1,0x06C1,0x06BE,0x06BE,0x06BE,0x06BE,0x06D2,0x06D2, 
    0x06D3,0x06D3,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, 
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, 
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, 
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, 
    0x0000,0x0000,0x0000,0x06AD,0x06AD,0x06AD,0x06AD,0x06C7, 
    0x06C7,0x06C6,0x06C6,0x06C8,0x06C8,0x0677,0x06CB,0x06CB, 
    0x06C5,0x06C5,0x06C9,0x06C9,0x06D0,0x06D0,0x06D0,0x06D0, 
    0x0649,0x0649,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, 
    0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000, 
    0x0000,0x0000,0x0000,0x0000,0x06CC,0x06CC,0x06CC,0x06CC  
};

#define PresentationToOriginal(c, order)                  \
    (((0xFE70 <= (c) && (c) <= 0xFEFC)) ?                 \
         FE_TO_06[(c)- FE_TO_06_OFFSET][order] :                    \
     (((0xFB50 <= (c) && (c) <= 0xFBFF) && (order) == 0) ? \
         FB_TO_06[(c)-0xFB50] : (PRUnichar) 0x0000))



static const PRUint8 gArabicMap1[] = {
            0x81, 0x83, 0x85, 0x87, 0x89, 0x8D, 
0x8F, 0x93, 0x95, 0x99, 0x9D, 0xA1, 0xA5, 0xA9, 
0xAB, 0xAD, 0xAF, 0xB1, 0xB5, 0xB9, 0xBD, 0xC1, 
0xC5, 0xC9, 0xCD                                
};

static const PRUint8 gArabicMap2[] = {
      0xD1, 0xD5, 0xD9, 0xDD, 0xE1, 0xE5, 0xE9, 
0xED, 0xEF, 0xF1                                
};

static const PRUint8 gArabicMapEx[] = {
      0x50, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x66, 0x5E, 0x52, 0x00, 0x00, 0x56, 0x62, 
0x5A, 0x00, 0x00, 0x76, 0x72, 0x00, 0x7A, 0x7E, 
0x88, 0x00, 0x00, 0x00, 0x84, 0x82, 0x86, 0x00, 
0x00, 0x8C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x8A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x6A, 0x00, 0x6E, 0x00, 
0x00, 0x8E, 0x00, 0x00, 0x00, 0xD3, 0x00, 0x92, 
0x00, 0x9A, 0x00, 0x96, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x9E, 0xA0, 0x00, 0x00, 0xAA, 0x00, 
0xA4, 0xA6, 0x00, 0x00, 0x00, 0xE0, 0xD9, 0xD7, 
0xDB, 0xE2, 0x00, 0xDE, 0xFC, 0x00, 0x00, 0x00, 
0xE4, 0x00, 0xAE, 0xB0                          
};

#define PresentationFormB(c, form)                                       \
    (((0x0622<=(c)) && ((c)<=0x063A)) ?                                  \
      (0xFE00|(gArabicMap1[(c)-0x0622] + (form))) :                      \
       (((0x0641<=(c)) && ((c)<=0x064A)) ?                               \
        (0xFE00|(gArabicMap2[(c)-0x0641] + (form))) :                    \
         (((0x0671<=(c)) && ((c))<=0x06D3) && gArabicMapEx[(c)-0x0671]) ? \
          (0xFB00|(gArabicMapEx[(c)-0x0671] + (form))) : (c)))

typedef enum {
   eIsolated,  
   eFinal,     
   eInitial,   
   eMedial     
} eArabicForm;

typedef enum {
   eTr = 0, 
   eRJ = 1, 
   eLJ = 2, 
   eDJ = 3, 
   eNJ = 4, 
   eJC = 7, 
   eRightJCMask = 2, 
   eLeftJCMask = 1   
} eArabicJoiningClass;

#define RightJCClass(j) (eRightJCMask&(j))
#define LeftJCClass(j)  (eLeftJCMask&(j))

#define DecideForm(jl,j,jr)                                 \
  (((eRJ == (j)) && RightJCClass(jr)) ? eFinal              \
                                      :                     \
   ((eDJ == (j)) ?                                          \
    ((RightJCClass(jr)) ?                                   \
     (((LeftJCClass(jl)) ? eMedial                          \
                         : eFinal))                         \
                        :                                   \
     (((LeftJCClass(jl)) ? eInitial                         \
                         : eIsolated))                      \
    )                     : eIsolated))                     \



static const PRInt8 gJoiningClass[] = {
eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eTr, eTr, eTr, eTr, eTr, eTr, eNJ, eNJ, 
eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eNJ, eNJ, eRJ, eRJ, eRJ, eRJ, eDJ, eRJ, 
eDJ, eRJ, eDJ, eDJ, eDJ, eDJ, eDJ, eRJ, 
eRJ, eRJ, eRJ, eDJ, eDJ, eDJ, eDJ, eDJ, 
eDJ, eDJ, eDJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eJC, eDJ, eDJ, eDJ, eDJ, eDJ, eDJ, eDJ, 
eRJ, eDJ, eDJ, eTr, eTr, eTr, eTr, eTr, 
eTr, eTr, eTr, eTr, eTr, eTr, eTr, eTr, 
eTr, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eTr, eRJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eNJ, eDJ, eDJ, eDJ, eNJ, eNJ, eDJ, eDJ, 
eDJ, eNJ, eNJ, eDJ, eDJ, eNJ, eDJ, eDJ, 
eRJ, eNJ, eNJ, eNJ, eRJ, eRJ, eRJ, eNJ, 
eNJ, eRJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eRJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eNJ, eNJ, eNJ, eNJ, eDJ, eNJ, eDJ, eNJ, 
eNJ, eDJ, eNJ, eNJ, eNJ, eDJ, eNJ, eDJ, 
eNJ, eDJ, eNJ, eDJ, eNJ, eNJ, eNJ, eNJ, 
eNJ, eNJ, eDJ, eDJ, eNJ, eNJ, eDJ, eNJ, 
eRJ, eDJ, eNJ, eNJ, eNJ, eRJ, eRJ, eRJ, 
eRJ, eRJ, eNJ, eRJ, eDJ, eNJ, eNJ, eNJ, 
eDJ, eNJ, eRJ, eRJ, eNJ, eNJ, eTr, eTr, 
eTr, eTr, eTr, eTr, eTr, eNJ, eNJ, eTr, 
eTr, eTr, eTr, eTr, eTr, eNJ, eNJ, eTr, 
eTr, eNJ, eTr, eTr, eTr, eTr, eNJ, eNJ, 
eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, 
eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ, eNJ  
};

#define GetJoiningClass(c)                   \
  ((IS_ARABIC_CHAR(c)) ?                     \
      (gJoiningClass[(c) - 0x0600]) :        \
      ((0x200D == (c)) ? eJC : eTr))

static const PRUint16 gArabicLigatureMap[] = 
{
0x82DF, 
0x82E0, 
0x84DF, 
0x84E0, 
0x88DF, 
0x88E0, 
0x8EDF, 
0x8EE0  
};

static nsCharType ebc2ucd[15] = {
  eCharType_OtherNeutral, 
  eCharType_LeftToRight,
  eCharType_RightToLeft,
  eCharType_RightToLeftArabic,
  eCharType_ArabicNumber,
  eCharType_EuropeanNumber,
  eCharType_EuropeanNumberSeparator,
  eCharType_EuropeanNumberTerminator,
  eCharType_CommonNumberSeparator,
  eCharType_OtherNeutral,
  eCharType_DirNonSpacingMark,
  eCharType_BoundaryNeutral,
  eCharType_BlockSeparator,
  eCharType_SegmentSeparator,
  eCharType_WhiteSpaceNeutral
};

static nsCharType cc2ucd[5] = {
  eCharType_LeftToRightEmbedding,
  eCharType_RightToLeftEmbedding,
  eCharType_PopDirectionalFormat,
  eCharType_LeftToRightOverride,
  eCharType_RightToLeftOverride
};

#define ARABIC_TO_HINDI_DIGIT_INCREMENT (START_HINDI_DIGITS - START_ARABIC_DIGITS)
#define NUM_TO_ARABIC(c) \
  ((((c)>=START_HINDI_DIGITS) && ((c)<=END_HINDI_DIGITS)) ? \
   ((c) - (PRUint16)ARABIC_TO_HINDI_DIGIT_INCREMENT) : \
   (c))
#define NUM_TO_HINDI(c) \
  ((((c)>=START_ARABIC_DIGITS) && ((c)<=END_ARABIC_DIGITS)) ? \
   ((c) + (PRUint16)ARABIC_TO_HINDI_DIGIT_INCREMENT): \
   (c))


static void ReverseString(PRUnichar* aBuffer, PRUint32 aLen)
{
  PRUnichar *start, *end;
  PRUnichar swapChar;

  for (start = aBuffer, end = aBuffer + aLen - 1; start < end; ++start, --end) {
    swapChar = *start;
    *start = *end;
    *end = swapChar;
  }
}

nsresult ArabicShaping(const PRUnichar* aString, PRUint32 aLen,
                       PRUnichar* aBuf, PRUint32 *aBufLen, 
                       PRBool aInputLogical, PRBool aOutputLogical)
{
  nsAutoString tempString(aString, aLen);
  if (tempString.Length() != aLen)
    return NS_ERROR_OUT_OF_MEMORY;
  PRUnichar *tempBuf = tempString.BeginWriting();
  if (aInputLogical) {
    ReverseString(tempBuf, aLen);
  }
  const PRUnichar* src = tempBuf;
  const PRUnichar* p;
  PRUnichar* dest = aBuf;
  PRUnichar formB;
  PRInt8 leftJ, thisJ, rightJ;
  PRInt8 leftNoTrJ, rightNoTrJ;
  thisJ = leftNoTrJ = eNJ;
  rightJ = GetJoiningClass(*(src));
  while(src<tempBuf+aLen-1) {
    leftJ = thisJ;

    if ((eTr != leftJ) || ((leftJ == eTr) && 
        ( ( (src-1) >= tempBuf) && !IS_ARABIC_CHAR(*(src-1)))))
      leftNoTrJ = thisJ;

    if(src-2 >= (tempBuf)){
      for(p=src-2; (p >= (tempBuf))&& (eTr == leftNoTrJ) && (IS_ARABIC_CHAR(*(p+1))) ; p--)  
        leftNoTrJ = GetJoiningClass(*(p)) ;
    }

    thisJ = rightJ;
    rightJ = rightNoTrJ = GetJoiningClass(*(src+1)) ;

    if(src+2 <= (tempBuf+aLen-1)){
      for(p=src+2; (p <= (tempBuf+aLen-1))&&(eTr == rightNoTrJ) && (IS_ARABIC_CHAR(*(src+1))); p++)
        rightNoTrJ = GetJoiningClass(*(p)) ;
    }

    formB = PresentationFormB(*src, DecideForm(leftNoTrJ, thisJ, rightNoTrJ));
    *dest++ = formB;
    src++;

  }
  if((eTr != thisJ) || 
     ((thisJ == eTr) && (((src-1)>=tempBuf) && (!IS_ARABIC_CHAR(*(src-1))))))
    leftNoTrJ = thisJ;

  if(src-2 >= (tempBuf)){
    for(p=src-2; (src-2 >= (tempBuf)) && (eTr == leftNoTrJ) && (IS_ARABIC_CHAR(*(p+1))); p--)
      leftNoTrJ = GetJoiningClass(*(p)) ;
  }

  formB = PresentationFormB(*src, DecideForm(leftNoTrJ, rightJ, eNJ));
  *dest++ = formB;
  src++;

  PRUnichar *lSrc = aBuf;
  PRUnichar *lDest = aBuf;
  while(lSrc < (dest-1)) {
    PRUnichar next = *(lSrc+1);
    if(((0xFEDF == next) || (0xFEE0 == next)) && 
       (0xFE80 == (0xFFF1 & *lSrc))) {
      PRBool done = PR_FALSE;
      PRUint16 key = ((*lSrc) << 8) | ( 0x00FF & next);
      PRUint16 i;
      for(i=0;i<8;i++) {
        if(key == gArabicLigatureMap[i]) {
          done = PR_TRUE;
          
          
          *lDest++ = 0xFEF5 + i;
          lSrc+=2;
          break;
        }
      }
      if(! done)
        *lDest++ = *lSrc++; 
    } else if (0x200C == *lSrc || 0x200D == *lSrc)
      
      lSrc++;
    else 
      *lDest++ = *lSrc++; 

  }
  if(lSrc < dest)
    *lDest++ = *lSrc++; 

  *aBufLen = lDest - aBuf;
  NS_ASSERTION(*aBufLen <= aLen, "ArabicShaping() likely did a buffer overflow!");

  if (aOutputLogical) {
    ReverseString(aBuf, *aBufLen);
  }
  return NS_OK;
}

nsresult Conv_FE_06(const nsString& aSrc, nsString& aDst)
{
  PRUnichar *aSrcUnichars = (PRUnichar *)aSrc.get();
  PRUint32 i, size = aSrc.Length();
  aDst.Truncate();
  for (i=0;i<size;i++) { 
    if (aSrcUnichars[i] == 0x0000) 
      break; 
    if (IS_FE_CHAR(aSrcUnichars[i])) {
      
      PRUnichar ch = (PresentationToOriginal(aSrcUnichars[i], 1));
      if(ch)
        aDst += ch;
      ch=(PresentationToOriginal(aSrcUnichars[i], 0));
      if(ch)
        aDst += ch;
      else 
        aDst += aSrcUnichars[i];
    } else {
      aDst += aSrcUnichars[i]; 
    }
  }
  return NS_OK;
}

nsresult Conv_FE_06_WithReverse(const nsString& aSrc, nsString& aDst)
{
  PRUnichar *aSrcUnichars = (PRUnichar *)aSrc.get();
  PRBool foundArabic = PR_FALSE;
  PRUint32 i, endArabic, beginArabic, size;
  beginArabic = 0;
  size = aSrc.Length();
  aDst.Truncate();
  for (endArabic=0;endArabic<size;endArabic++) {
    if (aSrcUnichars[endArabic] == 0x0000) 
      break; 

    while( (IS_FE_CHAR(aSrcUnichars[endArabic]))||
           (IS_ARABIC_CHAR(aSrcUnichars[endArabic]))||
           (IS_ARABIC_DIGIT(aSrcUnichars[endArabic]))||
           (aSrcUnichars[endArabic]==0x0020)) 
    {
      if(! foundArabic ) {
        beginArabic=endArabic;
        foundArabic= PR_TRUE;
      }
      endArabic++;
    }
    if(foundArabic) {
      endArabic--;
      for (i=endArabic; i>=beginArabic; i--) {
        if(IS_FE_CHAR(aSrcUnichars[i])) {
          
          aDst += PresentationToOriginal(aSrcUnichars[i], 0);
          if (PresentationToOriginal(aSrcUnichars[i], 1)) {
            
             aDst += PresentationToOriginal(aSrcUnichars[i], 1);
          } 
        } else {
          
          if((IS_ARABIC_CHAR(aSrcUnichars[i]))||
             (IS_ARABIC_DIGIT(aSrcUnichars[i]))||
             (aSrcUnichars[i]==0x0020))
            aDst += aSrcUnichars[i];
        }     
      }
    } else {
      aDst += aSrcUnichars[endArabic]; 
    }
    foundArabic=PR_FALSE;
  }
  return NS_OK;
}

nsresult Conv_06_FE_WithReverse(const nsString& aSrc,
                                nsString& aDst,
                                PRUint32 aDir)
{
  PRUnichar *aSrcUnichars = (PRUnichar *)aSrc.get();
  PRUint32 i, beginArabic, endArabic, size;
  beginArabic = 0;
  size = aSrc.Length();
  aDst.Truncate();
  PRBool foundArabic = PR_FALSE;
  for (endArabic=0;endArabic<size;endArabic++) {
    if (aSrcUnichars[endArabic] == 0x0000) 
      break; 

    while( (IS_06_CHAR(aSrcUnichars[endArabic])) || 
           (IS_ARABIC_CHAR(aSrcUnichars[endArabic])) || 
           (aSrcUnichars[endArabic]==0x0020) || 
           (IS_ARABIC_DIGIT(aSrcUnichars[endArabic]))  ) 
    {
      if(! foundArabic) {
        beginArabic=endArabic;
        foundArabic=PR_TRUE;
      }
      endArabic++;
    }
    if(foundArabic) {
      endArabic--;
      PRUnichar buf[8192];
      PRUint32 len=8192;

      ArabicShaping(&aSrcUnichars[beginArabic], endArabic-beginArabic+1,
                    buf, &len, 
                    PR_TRUE, PR_FALSE);
      
      PRUint32 endNumeral, beginNumeral = 0;
      for (endNumeral=0;endNumeral<=len-1;endNumeral++){
        PRBool foundNumeral = PR_FALSE;
        while((endNumeral < len) && (IS_ARABIC_DIGIT(buf[endNumeral]))  ) {
          if(!foundNumeral)
          {
            foundNumeral=PR_TRUE;
            beginNumeral=endNumeral;
          }
          endNumeral++;
        }
        if(foundNumeral){
          endNumeral--;
          PRUnichar numbuf[20];
          for(i=beginNumeral; i<=endNumeral; i++){
            numbuf[i-beginNumeral]=buf[endNumeral-i+beginNumeral];
          }
          for(i=0;i<=endNumeral-beginNumeral;i++){
            buf[i+beginNumeral]=numbuf[i];
          }
        }
      }
      if(aDir==1){
        for (i=0;i<=len-1;i++){
          aDst+= buf[i];
        } 
      }
      else if(aDir==2){
        for (i=0;i<=len-1;i++){
          aDst+= buf[len-1-i];
        } 
      }
    } else {
      aDst += aSrcUnichars[endArabic];
    }
    foundArabic=PR_FALSE;
  }
  return NS_OK;
}

nsresult HandleNumbers(PRUnichar* aBuffer, PRUint32 aSize, PRUint32 aNumFlag)
{
  PRUint32 i;
  
  
  
  
  

  switch (aNumFlag) {
    case IBMBIDI_NUMERAL_HINDI:
      for (i=0;i<aSize;i++)
        aBuffer[i] = NUM_TO_HINDI(aBuffer[i]);
      break;
    case IBMBIDI_NUMERAL_ARABIC:
      for (i=0;i<aSize;i++)
        aBuffer[i] = NUM_TO_ARABIC(aBuffer[i]);
      break;
    case IBMBIDI_NUMERAL_REGULAR:
    case IBMBIDI_NUMERAL_HINDICONTEXT:
        
        
      for (i=1;i<aSize;i++) {
        if (IS_ARABIC_CHAR(aBuffer[i-1])) 
          aBuffer[i] = NUM_TO_HINDI(aBuffer[i]);
        else 
          aBuffer[i] = NUM_TO_ARABIC(aBuffer[i]);
      }
    case IBMBIDI_NUMERAL_NOMINAL:
    default:
      break;
  }
  return NS_OK;
}

nsresult HandleNumbers(const nsString& aSrc, nsString& aDst)
{
  aDst = aSrc;
  return HandleNumbers((PRUnichar *)aDst.get(),aDst.Length(), IBMBIDI_NUMERAL_REGULAR);
}

PRUint32 SymmSwap(PRUint32 aChar)
{
  return Mirrored(aChar);
}

eBidiCategory GetBidiCategory(PRUint32 aChar)
{
  eBidiCategory oResult = GetBidiCat(aChar);
  if (eBidiCat_CC == oResult)
    oResult = (eBidiCategory)(aChar & 0xFF); 
  return oResult;
}

PRBool IsBidiCategory(PRUint32 aChar, eBidiCategory aBidiCategory)
{
  return (GetBidiCategory(aChar) == aBidiCategory);
}

#define LRM_CHAR 0x200e
PRBool IsBidiControl(PRUint32 aChar)
{
  
  
  
  return (eBidiCat_CC == GetBidiCat(aChar) || ((aChar)&0xfffffe)==LRM_CHAR);
}

nsCharType GetCharType(PRUint32 aChar)
{
  nsCharType oResult;
  eBidiCategory bCat = GetBidiCat(aChar);
  if (eBidiCat_CC != bCat) {
    NS_ASSERTION(bCat < (sizeof(ebc2ucd)/sizeof(nsCharType)), "size mismatch");
    if(bCat < (sizeof(ebc2ucd)/sizeof(nsCharType)))
      oResult = ebc2ucd[bCat];
    else 
      oResult = ebc2ucd[0]; 
  } else {
    NS_ASSERTION((aChar-0x202a) < (sizeof(cc2ucd)/sizeof(nsCharType)), "size mismatch");
    if((aChar-0x202a) < (sizeof(cc2ucd)/sizeof(nsCharType)))
      oResult = cc2ucd[aChar - 0x202a];
    else 
      oResult = ebc2ucd[0]; 
  }
  return oResult;
}
