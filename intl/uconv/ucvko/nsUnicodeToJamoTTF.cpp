
















































#include "nsUCvKODll.h"
#include "nsUnicodeToJamoTTF.h"
#include "prmem.h"
#include "nsXPIDLString.h"
#include "prtypes.h"
#include "nscore.h"
#include "nsISupportsUtils.h"
#include "nsCOMPtr.h"
#include "nsIUnicodeDecoder.h"
#include "nsServiceManagerUtils.h"
#include "nsICharsetConverterManager.h"
#include "nsICharRepresentable.h"
#include <string.h>

typedef struct { 
  PRUint8 seq[3];
  PRUint8 liga;
} JamoNormMap; 


#include "jamoclusters.h"




#define LBASE 0x1100
#define VBASE 0x1161
#define TBASE 0x11A7
#define TSTART 0x11A8
#define SBASE 0xAC00

#define LCOUNT 19
#define VCOUNT 21
#define TCOUNT 28
#define SCOUNT (LCOUNT * VCOUNT * TCOUNT)
#define SEND (SBASE + SCOUNT - 1)


#define LFILL 0x115F
#define VFILL 0x1160

#define IS_LC(wc) (LBASE <= (wc) && (wc) <  VFILL)
#define IS_VO(wc) (VFILL <= (wc) && (wc) <  TSTART)
#define IS_TC(wc) (TSTART <= (wc) && (wc) <= 0x11FF)
#define IS_JAMO(wc)   (IS_LC(wc) || IS_VO(wc) || IS_TC(wc))


#define IS_SYL_LC(wc) (LBASE <= (wc) && (wc) <  LBASE + LCOUNT)
#define IS_SYL_VO(wc) (VBASE <= (wc) && (wc) <  VBASE + VCOUNT)
#define IS_SYL_TC(wc) (TBASE <  (wc) && (wc) <= TBASE + TCOUNT)


#define IS_SYL(wc)   (SBASE <= (wc) && (wc) <= SEND)
#define IS_SYL_WO_TC(wc)  (((wc) - SBASE) % TCOUNT == 0)
#define IS_SYL_WITH_TC(wc)  (((wc) - SBASE) % TCOUNT)


#define SYL_FROM_LVT(l,v,t) (SBASE + \
                             (((l) - LBASE) * VCOUNT + (v) - VBASE) * TCOUNT + \
                             (t) - TBASE)


#define HTONE1 0x302E
#define HTONE2 0x302F

#define IS_TONE(wc) ((wc) == HTONE1 || (wc) == HTONE2)



#define LC_TMPPOS  0xF000 // temp. block for leading consonants
#define VO_TMPPOS  0xF100 // temp. block for vowels
#define TC_TMPPOS  0xF200 // temp. block for trailinng consonants
#define LC_OFFSET  (LC_TMPPOS-LBASE)
#define VO_OFFSET  (VO_TMPPOS-VFILL)
#define TC_OFFSET  (TC_TMPPOS-TSTART)


#define IS_LC_EXT(wc) ( ((wc) & 0xFF00) == LC_TMPPOS )
#define IS_VO_EXT(wc) ( ((wc) & 0xFF00) == VO_TMPPOS )
#define IS_TC_EXT(wc) ( ((wc) & 0xFF00) == TC_TMPPOS )


#define UP_LBASE 0xE000  // 0xE000 = Lfill, 0xE006 = Kiyeok 
#define UP_VBASE 0xE300  // 0xE300 = Vfill, 0xE302 = Ah  
#define UP_TBASE 0xE404  // 0xE400 = Tfill, 0xE404 = Kiyeok


static nsCOMPtr<nsIUnicodeDecoder> gDecoder = 0;
  
static inline void FillInfoRange     (PRUint32* aInfo, PRUint32 aStart, 
                                      PRUint32 aEnd);
static nsresult     JamoNormalize    (const PRUnichar* aInSeq, 
                                      PRUnichar** aOutSeq, PRInt32* aLength);
static void         JamosToExtJamos  (PRUnichar* aInSeq,  PRInt32* aLength);
static const JamoNormMap* JamoClusterSearch(JamoNormMap aKey, 
                                            const JamoNormMap* aClusters,
                                            PRInt16 aClustersSize);
static nsresult     FillInfoEUCKR    (PRUint32 *aInfo, PRUint16 aHigh1, 
                                      PRUint16 aHigh2);

static PRInt32      JamoNormMapComp  (const JamoNormMap& p1, 
                                      const JamoNormMap& p2);
static PRInt16      JamoSrchReplace  (const JamoNormMap* aCluster, 
                                      PRUint16 aSize, PRUnichar *aIn, 
                                      PRInt32* aLength, PRUint16 aOffset);
static nsresult     GetDecoder       (nsIUnicodeDecoder** aDecoder);
static nsresult     ScanDecomposeSyllable (PRUnichar *aIn, PRInt32* aLength, 
                                           const PRInt32 aMaxLen);



  
NS_IMPL_ISUPPORTS2(nsUnicodeToJamoTTF, nsIUnicodeEncoder, nsICharRepresentable)

NS_IMETHODIMP 
nsUnicodeToJamoTTF::SetOutputErrorBehavior(PRInt32 aBehavior, 
                                           nsIUnicharEncoder *aEncoder, 
                                           PRUnichar aChar)
{
  if (aBehavior == kOnError_CallBack && aEncoder == nsnull)
    return NS_ERROR_NULL_POINTER;
  mErrEncoder = aEncoder;
  mErrBehavior = aBehavior;
  mErrChar = aChar;
  return NS_OK;
}



nsUnicodeToJamoTTF::nsUnicodeToJamoTTF() 
{
  mJamos = nsnull;
  Reset();
}

nsUnicodeToJamoTTF::~nsUnicodeToJamoTTF()
{
  if (mJamos != nsnull && mJamos != mJamosStatic)
    PR_Free(mJamos);
}

enum KoCharClass {
  KO_CHAR_CLASS_LC,
  KO_CHAR_CLASS_VO,  
  KO_CHAR_CLASS_TC,  
  KO_CHAR_CLASS_SYL1,   
  KO_CHAR_CLASS_SYL2,   
  KO_CHAR_CLASS_TONE,   
  KO_CHAR_CLASS_NOHANGUL, 
  KO_CHAR_CLASS_NUM
} ;

#define CHAR_CLASS(ch) \
  (IS_LC(ch) ? KO_CHAR_CLASS_LC   :  \
   IS_VO(ch) ? KO_CHAR_CLASS_VO   :  \
   IS_TC(ch) ? KO_CHAR_CLASS_TC   :  \
   IS_SYL(ch) ?                      \
    (IS_SYL_WITH_TC(ch) ? KO_CHAR_CLASS_SYL2 : KO_CHAR_CLASS_SYL1) : \
   IS_TONE(ch) ? KO_CHAR_CLASS_TONE : \
   KO_CHAR_CLASS_NOHANGUL)



const static PRBool gIsBoundary[KO_CHAR_CLASS_NUM][KO_CHAR_CLASS_NUM] = 
{
  { 0, 0, 1, 0, 0, 0, 1 }, 
  { 1, 0, 0, 1, 1, 0, 1 }, 
  { 1, 1, 0, 1, 1, 0, 1 }, 
  { 1, 0, 0, 1, 1, 0, 1 }, 
  { 1, 1, 0, 1, 1, 0, 1 }, 
  { 1, 1, 1, 1, 1, 0, 1 }, 
  { 1, 1, 1, 1, 1, 0, 1 }  
};


NS_IMETHODIMP 
nsUnicodeToJamoTTF::Convert(const PRUnichar * aSrc, 
                            PRInt32 * aSrcLength, char * aDest, 
                            PRInt32 * aDestLength)
{
  nsresult rv = NS_OK;
  mByteOff = 0;

  
  if (mJamoCount > mJamosMaxLength) 
  {
    NS_WARNING("mJamoCount > mJamoMaxLength on entering Convert()");
    Reset();
  }

  for (PRInt32 charOff = 0; charOff < *aSrcLength; charOff++)
  {
    PRUnichar ch = aSrc[charOff];

    
    if (mJamoCount != 0 &&
        gIsBoundary[CHAR_CLASS(mJamos[mJamoCount - 1])][CHAR_CLASS(ch)])
    {
      composeHangul(aDest);
      mJamoCount = 0;
    }
    
    else if (mJamoCount != 0 && IS_TONE(mJamos[mJamoCount - 1]) && IS_TONE(ch))
    {
      --mJamoCount; 
      composeHangul(aDest);
      mJamoCount = 0;

      
      while (IS_TONE(ch) && ++charOff < *aSrcLength)
        ch = aSrc[charOff]; 

      if (!IS_TONE(ch)) 
      {
        mJamos[mJamoCount++] = ch; 
        continue;
      }
      else
        break;
    }

    if (mJamoCount == mJamosMaxLength)
    {
      mJamosMaxLength++;
      if (mJamos == mJamosStatic)
      {
        mJamos = (PRUnichar *) PR_Malloc(sizeof(PRUnichar) * mJamosMaxLength);
        if (!mJamos)
          return  NS_ERROR_OUT_OF_MEMORY;
        memcpy(mJamos, mJamosStatic, sizeof(PRUnichar) * mJamoCount);
      }
      else
      {
        mJamos = (PRUnichar *) PR_Realloc(mJamos, 
                               sizeof(PRUnichar) * mJamosMaxLength);
        if (!mJamos)
          return  NS_ERROR_OUT_OF_MEMORY;
      }
    }

    mJamos[mJamoCount++] = ch;
  }
    
  if (mJamoCount != 0)
    composeHangul(aDest);
  mJamoCount = 0;
  *aDestLength = mByteOff;

  return rv;
}

NS_IMETHODIMP 
nsUnicodeToJamoTTF::Finish(char* aDest, PRInt32* aDestLength)
{
  mByteOff = 0;
  if (mJamoCount != 0)
    composeHangul(aDest);

  *aDestLength = mByteOff;

  mByteOff = 0;
  mJamoCount = 0;
  return NS_OK;
}


NS_IMETHODIMP 
nsUnicodeToJamoTTF::Reset()
{

  if (mJamos != nsnull && mJamos != mJamosStatic)
    PR_Free(mJamos);
  mJamos = mJamosStatic;
  mJamosMaxLength = sizeof(mJamosStatic) / sizeof(PRUnichar);
  memset(mJamos, 0, sizeof(mJamosStatic));
  mJamoCount = 0;
  mByteOff = 0;

  return NS_OK;
}

NS_IMETHODIMP 
nsUnicodeToJamoTTF::GetMaxLength(const PRUnichar * aSrc, PRInt32 aSrcLength,
                                 PRInt32 * aDestLength)
{
  
  
  *aDestLength = aSrcLength *  6;
  return NS_OK;
}


NS_IMETHODIMP 
nsUnicodeToJamoTTF::FillInfo(PRUint32* aInfo)
{
  FillInfoRange(aInfo, SBASE, SEND);

  PRUnichar i;

  
  for(i = 0x1100; i<= 0x1159; i++)
     SET_REPRESENTABLE(aInfo, i);
  SET_REPRESENTABLE(aInfo, 0x115f);
  for(i = 0x1160; i <= 0x11a2; i++)
     SET_REPRESENTABLE(aInfo, i);
  for(i = 0x11a8; i <= 0x11f9; i++)
     SET_REPRESENTABLE(aInfo, i);

  
  SET_REPRESENTABLE(aInfo, HTONE1);
  SET_REPRESENTABLE(aInfo, HTONE2);

  
  for(i=0x20; i < 0x7f; i++)
     SET_REPRESENTABLE(aInfo, i);

  nsresult rv;

  
  
  
  
  
  
  
  
    
  
  rv = FillInfoEUCKR(aInfo, 0xA1, 0xAF); 
  NS_ENSURE_SUCCESS(rv, rv);

  
  return FillInfoEUCKR(aInfo, 0xCA, 0xFD); 
}


























const static PRUint8 gUnParkLcGlyphMap[130] = {
  1,  2,  4, 12, 14, 20, 36, 42, 46, 62, 70, 85,100,102,108,113,
114,116,120,  5,  6,  7,  8, 13, 23, 26, 34, 35, 39, 41, 43, 44,
 45, 47, 48, 49, 50, 51, 52, 54, 55, 57, 58, 60, 61, 63, 64, 65,
 66, 67, 68, 69, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,
 84, 86, 87, 89, 90, 91, 92, 93, 94, 95, 96, 97, 99,101,104,105,
106,107,109,110,111,112,117,119,122,123,  0,  0,  0,  0,  0,  0,
  3,  9, 10, 11, 15, 16, 17, 18, 19, 21, 22, 24, 25, 27, 28, 29,
 30, 31, 32, 33, 37, 38, 40, 53, 56, 59, 71, 88, 98,103,115,118,
121, 124
};







const static PRUint8 gUnParkVoGlyphMap[95] = {
   0,  1,  5,  6, 10, 11, 15, 16, 20, 21, 22, 23, 33, 34, 43, 46, 
  48, 52, 54, 64, 71, 73,  2,  3,  7,  8, 12, 13, 14, 18, 19, 26, 
  27, 29, 30, 32, 37, 38, 40, 41, 42, 44, 45, 47, 50, 51, 55, 57, 
  58, 59, 60, 62, 63, 69, 70, 72, 74, 75, 80, 83, 85, 87, 88, 90, 
  92, 93, 94,  4,  9, 17, 24, 25, 28, 31, 35, 36, 39, 49, 53, 56, 
  61, 65, 66, 67, 68, 76, 77, 78, 79, 81, 82, 84, 86, 89, 91
};








const static PRUint8 gUnParkTcGlyphMap[141] = {
   0,  1,  5, 10, 17, 20, 21, 32, 33, 42, 46, 52, 57, 58, 59, 63,
  78, 84, 91, 98,109,123,127,128,129,130,135,  3,  6, 11, 13, 15,
  16, 19, 22, 25, 35, 37, 38, 39, 40, 43, 44, 48, 50, 51, 53, 54,
  56, 60, 64, 67, 69, 71, 72, 73, 75, 76, 77, 80, 88, 89, 90, 92,
  93, 94, 96,106,110,111,114,115,117,119,120,131,134,136,137,138,
 139,140,  2,  4,  7,  8,  9, 12, 14, 18, 23, 24, 26, 27, 28, 29,
  30, 31, 34, 36, 41, 45, 47, 49, 55, 61, 62, 65, 66, 68, 70, 74,
  79, 81, 82, 83, 85, 86, 87, 95, 97, 99,100,101,102,103,104,105,
 107,108,112,113,116,118,121,122,124,125,126,132,133
};









 
const static PRUint8 gUnParkVo2LcMap[95] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 1, 2, 2, 1,
  1, 1, 2, 2, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 2, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 2, 1, 1,
  1, 1, 1, 2, 1, 2, 2, 1, 0, 0, 1, 1, 1, 0, 2, 1,
  2, 1, 2, 1, 1, 0, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1,
  2, 1, 1, 1, 2, 1, 0, 0, 0, 1, 1, 1, 0, 2, 2
};



const static PRUint8 gUnParkVo2LcMap2[95] = {
  3, 3, 3, 3, 3, 3, 3, 3, 3, 5, 4, 4, 4, 5, 5, 4,
  4, 4, 5, 5, 4, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 5, 5, 4, 4, 4, 5, 4, 4, 4, 4, 4, 5, 4, 4,
  4, 4, 4, 5, 4, 5, 5, 4, 3, 3, 4, 4, 4, 3, 5, 4,
  5, 4, 5, 4, 4, 3, 4, 4, 4, 5, 4, 4, 4, 4, 4, 4,
  5, 4, 4, 4, 5, 4, 3, 3, 3, 4, 4, 4, 3, 5, 5
};





const static PRUint8 gUnParkVo2TcMap[95] = {
  3, 0, 2, 0, 2, 1, 2, 1, 2, 3, 0, 2, 1, 3, 3, 1,
  2, 1, 3, 3, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1,
  2, 2, 3, 3, 0, 2, 1, 3, 1, 0, 2, 1, 2, 3, 0, 1,
  2, 1, 2, 3, 1, 3, 3, 1, 2, 2, 1, 1, 1, 1, 3, 1,
  3, 1, 3, 0, 1, 0, 0, 0, 2, 3, 0, 2, 1, 1, 2, 2,
  3, 0, 0, 0, 3, 0, 2, 2, 2, 1, 0, 1, 2, 1, 1
};

NS_IMETHODIMP 
nsUnicodeToJamoTTF::composeHangul(char* aResult)
{
  PRInt32 length = mJamoCount, i;
  nsresult rv = NS_OK;

  if (!length)
  {
    NS_WARNING("composeHangul() : zero length string comes in ! \n");
    return NS_ERROR_UNEXPECTED;
  }

  if (!aResult) 
    return NS_ERROR_NULL_POINTER;

  
  
  
  
  if (IS_TONE(mJamos[length - 1])) 
  {
    aResult[mByteOff++] = PRUint8(mJamos[length - 1] >> 8);
    aResult[mByteOff++] = PRUint8(mJamos[length - 1] & 0xff); 
    if (--length == 0)
      return rv;
  }

  
  if (length == 1 && IS_SYL(mJamos[0])) 
  {
    aResult[mByteOff++] = PRUint8(mJamos[0] >> 8);
    aResult[mByteOff++] = PRUint8(mJamos[0] & 0xff); 
    return rv;
  }

  if (CHAR_CLASS(mJamos[0]) == KO_CHAR_CLASS_NOHANGUL) 
  {
    NS_ASSERTION(length == 1, "A non-Hangul should come by itself !!\n");
    aResult[mByteOff++] = PRUint8(mJamos[0] >> 8);
    aResult[mByteOff++] = PRUint8(mJamos[0] & 0xff); 
    return rv;
  }

  nsXPIDLString buffer;

  rv =  JamoNormalize(mJamos, getter_Copies(buffer), &length);

  
  PRUnichar* text = buffer.BeginWriting();
  NS_ENSURE_SUCCESS(rv, rv);

  text += RenderAsPrecompSyllable(text, &length, aResult);

  if (!length)
    return rv;

  
  JamosToExtJamos(text, &length);


  
  if (length != 2 && length != 3 ||
      (!IS_LC_EXT(text[0]) || !IS_VO_EXT(text[1]) ||
       (length == 3 && !IS_TC_EXT(text[2]))))
    goto fallback;



  
  text[0] -= LC_OFFSET; 
  text[1] -= VO_OFFSET; 
  if (length == 3)
    text[2] -= TC_OFFSET;

  if (length != 3)
  {
    text[0] = gUnParkLcGlyphMap[text[0] - LBASE] * 6 + 
              gUnParkVo2LcMap[text[1] - VFILL] + UP_LBASE;
    text[1] = gUnParkVoGlyphMap[text[1] - VFILL] * 2 + UP_VBASE;
  }
  else 
  {
    text[0] = gUnParkLcGlyphMap[text[0] - LBASE] * 6 + 
              gUnParkVo2LcMap2[text[1] - VFILL] + UP_LBASE;
    text[2] = gUnParkTcGlyphMap[text[2] - TSTART] * 4 + 
              gUnParkVo2TcMap[text[1] - VFILL] + UP_TBASE; 
    text[1] = gUnParkVoGlyphMap[text[1] - VFILL] * 2 + UP_VBASE + 1; 
  }

  
  
  
    
  if (UP_LBASE <= text[0] && text[0] < UP_LBASE + 6)
    text[0] = LFILL;

  
  
  
  if (UP_VBASE <= text[1] && text[1] < UP_VBASE + 2)
  {
    --length;
    if (length == 2) 
      text[1] = text[2]; 
  }

  for (i = 0 ; i < length; i++)
  {
    aResult[mByteOff++] = PRUint8(text[i] >> 8);
    aResult[mByteOff++] = PRUint8(text[i] & 0xff);
  }

  return rv;


  









fallback: 
  for (i = 0; i < length; i++)
  {
    PRUnichar wc=0, wc2=0;
    
    if (length > 1 && 
         (text[i] - LC_OFFSET == LFILL || text[i] - VO_OFFSET == VFILL))
      continue;
    else if (IS_LC_EXT (text[i]))
       wc = gUnParkLcGlyphMap[text[i] - LC_OFFSET - LBASE] * 6 + UP_LBASE;
    else 
    {
  
      wc = LBASE;
   
      if (text[i] - VO_OFFSET != VFILL) 
        wc2 = IS_VO_EXT (text[i]) ? 
        gUnParkVoGlyphMap[text[i] - VO_OFFSET - VFILL] * 2 + UP_VBASE:
        gUnParkTcGlyphMap[text[i] - TC_OFFSET - TSTART] * 4 + UP_TBASE + 3;
    }
    aResult[mByteOff++] = PRUint8(wc >> 8);
    aResult[mByteOff++] = PRUint8(wc & 0xff);

    if (wc2) 
    {
      aResult[mByteOff++] = wc2 >> 8;
      aResult[mByteOff++] = wc2 & 0xff; 
    }
  }

  return rv;
}

int
nsUnicodeToJamoTTF::RenderAsPrecompSyllable (PRUnichar* aSrc, 
                                             PRInt32* aSrcLength, char* aResult)
{

  int composed = 0;

  if (*aSrcLength == 3 && IS_SYL_LC(aSrc[0]) && IS_SYL_VO(aSrc[1]) && 
      IS_SYL_TC(aSrc[2]))
    composed = 3;
  else if (*aSrcLength == 2 && IS_SYL_LC(aSrc[0]) && IS_SYL_VO(aSrc[1]))
    composed = 2;
  else
    composed = 0;

  if (composed)
  {
    PRUnichar wc;
    if (composed == 3)
      wc = SYL_FROM_LVT(aSrc[0], aSrc[1], aSrc[2]);
    else
      wc = SYL_FROM_LVT(aSrc[0], aSrc[1], TBASE);
    aResult[mByteOff++] = PRUint8(wc >> 8);
    aResult[mByteOff++] = PRUint8(wc & 0xff);
  }

  *aSrcLength -= composed;

  return composed;
}



inline void FillInfoRange(PRUint32* aInfo, PRUint32 aStart, PRUint32 aEnd)
{

  PRUint32 b = aStart >> 5; 
  PRUint32 e = aEnd >> 5;

  if (aStart & 0x1f)
    aInfo[b++] |= ~ (0xFFFFFFFFL >> (32 - ((aStart) & 0x1f)));

  for( ; b < e ; b++)
    aInfo[b] |= 0xFFFFFFFFL;

  aInfo[e] |= (0xFFFFFFFFL >> (31 - ((aEnd) & 0x1f)));
}


#define ROWLEN 94
#define IS_GR94(x) (0xA0 < (x) && (x) < 0xFF)





nsresult FillInfoEUCKR (PRUint32 *aInfo, PRUint16 aHigh1, PRUint16 aHigh2)
{
  char row[ROWLEN * 2];
  PRUnichar dest[ROWLEN];
  nsresult rv = NS_OK;

  NS_ENSURE_TRUE(aInfo, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(IS_GR94(aHigh1) && IS_GR94(aHigh2), NS_ERROR_INVALID_ARG);

  nsCOMPtr<nsIUnicodeDecoder> decoder;
  rv = GetDecoder(getter_AddRefs(decoder));
  NS_ENSURE_SUCCESS(rv,rv);

  for (PRUint16 i = aHigh1 ; i <= aHigh2; i++)
  {
    PRUint16 j;
    
    for (j = 0 ; j < ROWLEN; j++)
    {
      row[j * 2] = char(i);
      row[j * 2 + 1] = char(j + 0xa1);
    }
    PRInt32 srcLen = ROWLEN * 2;
    PRInt32 destLen = ROWLEN;
    rv = decoder->Convert(row, &srcLen, dest, &destLen);
    NS_ENSURE_SUCCESS(rv, rv);

    
    for (j = 0 ; j < ROWLEN; j++)
      if (dest[j] != 0xFFFD)
        SET_REPRESENTABLE(aInfo, dest[j]);
  }
  return rv;
}


nsresult GetDecoder(nsIUnicodeDecoder** aDecoder)
{
  nsresult rv; 

  if (gDecoder) {
    *aDecoder = gDecoder.get();
    NS_ADDREF(*aDecoder);
    return NS_OK;
  }

  nsCOMPtr<nsICharsetConverterManager> charsetConverterManager;
  charsetConverterManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  rv = charsetConverterManager->GetUnicodeDecoderRaw("EUC-KR", getter_AddRefs(gDecoder));
  NS_ENSURE_SUCCESS(rv,rv);

  *aDecoder = gDecoder.get();
  NS_ADDREF(*aDecoder);
  return NS_OK;
}



PRInt32 JamoNormMapComp (const JamoNormMap& p1, const JamoNormMap& p2)
{
  if (p1.seq[0] != p2.seq[0]) 
    return p1.seq[0] - p2.seq[0];
  if (p1.seq[1] != p2.seq[1]) 
    return p1.seq[1] - p2.seq[1];
  return p1.seq[2] - p2.seq[2];
}


const JamoNormMap* JamoClusterSearch (JamoNormMap aKey, 
                                const JamoNormMap* aClusters, 
                                PRInt16 aClustersSize)
{

  if (aClustersSize <= 0 || !aClusters)
  {
    NS_WARNING("aClustersSize <= 0 || !aClusters");
    return nsnull;
  }

  if (aClustersSize < 9) 
  {
    PRInt16 i;
    for (i = 0; i < aClustersSize; i++)
      if (JamoNormMapComp (aKey, aClusters[i]) == 0) 
        return aClusters + i; 
    return nsnull;
  }
   
  PRUint16 l = 0, u = aClustersSize - 1;
  PRUint16 h = (l + u) / 2;

  if (JamoNormMapComp (aKey, aClusters[h]) < 0) 
    return JamoClusterSearch(aKey, &(aClusters[l]), h - l);   
  else if (JamoNormMapComp (aKey, aClusters[h]) > 0) 
    return JamoClusterSearch(aKey, &(aClusters[h + 1]), u - h);   
  else
    return aClusters + h;

}










PRInt16 JamoSrchReplace (const JamoNormMap* aClusters, 
                         PRUint16 aClustersSize, PRUnichar* aIn, 
                         PRInt32* aLength, PRUint16 aOffset)
{
  PRInt32 origLen = *aLength; 

  
  PRUint16 clusterLen = aClusters[0].seq[2] ? 3 : 2; 

  PRInt32 start = 0, end;

  
  while (start < origLen && (aIn[start] & 0xff00) != aOffset)
    ++start;
  for (end=start; end < origLen && (aIn[end] & 0xff00) == aOffset; ++end);

  
  
  
  for (PRInt32 i = start; i <= end - clusterLen; i++)
  {
    const JamoNormMap *match;
    JamoNormMap key;

    
    
    key.seq[0] = aIn[i] - aOffset;
    key.seq[1] = aIn[i + 1] - aOffset;
    key.seq[2] = clusterLen == 3 ? (aIn[i + 2] - aOffset) : 0;

    match = JamoClusterSearch (key, aClusters, aClustersSize);

    if (match) 
    {
      aIn[i] = match->liga + aOffset; 

      
      for (PRInt32 j = i + clusterLen ; j < *aLength; j++)
        aIn[j - clusterLen + 1] = aIn[j];

      end -= (clusterLen - 1);
      *aLength -= (clusterLen - 1);
    }
  }

  return *aLength - origLen;
}


nsresult ScanDecomposeSyllable(PRUnichar* aIn, PRInt32 *aLength, 
                               const PRInt32 maxLength)
{
  nsresult rv = NS_OK;

  if (!aIn || *aLength < 1 || maxLength < *aLength + 2)
    return NS_ERROR_INVALID_ARG;

  PRInt32 i = 0;
  while (i < *aLength && !IS_SYL(aIn[i]))
    i++;

  
  if (i < *aLength && IS_SYL(aIn[i]))
  {
    PRUint16 j = IS_SYL_WITH_TC(aIn[i]) ? 1 : 0; 
    aIn[i] -= SBASE;
    memmove(aIn + i + 2 + j, aIn + i + 1, *aLength - i - 1);
    if (j)
      aIn[i + 2] = aIn[i] % TCOUNT + TBASE;
    aIn[i + 1] = (aIn[i] / TCOUNT) % VCOUNT + VBASE;
    aIn[i] = aIn[i] / (TCOUNT * VCOUNT) + LBASE;
    *aLength += 1 + j;
  }

  return rv;
}
























nsresult JamoNormalize(const PRUnichar* aInSeq, PRUnichar** aOutSeq, 
                       PRInt32* aLength) 
{
  if (!aInSeq || !aOutSeq || *aLength <= 0)
    return NS_ERROR_INVALID_ARG;

  
  
  *aOutSeq = new PRUnichar[*aLength + 4]; 
  if (!*aOutSeq)
    return NS_ERROR_OUT_OF_MEMORY;
  memcpy(*aOutSeq, aInSeq, *aLength * sizeof(PRUnichar));

  nsresult rv = ScanDecomposeSyllable(*aOutSeq, aLength, *aLength + 4);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if ((*aLength == 2 && IS_LC((*aOutSeq)[0]) && IS_VO((*aOutSeq)[1])) || 
      (*aLength == 3 && IS_LC((*aOutSeq)[0]) && IS_VO((*aOutSeq)[1]) && 
      IS_TC((*aOutSeq)[2])))
    return NS_OK;

  
  
  if ((*aOutSeq)[0] == LFILL && *aLength > 1 && IS_LC((*aOutSeq)[1]))
  {
    memmove (*aOutSeq, *aOutSeq + 1, (*aLength - 1) * sizeof(PRUnichar)); 
    (*aLength)--;
  }

  if (*aLength > 1)
  {
    JamoSrchReplace (gJamoClustersGroup1,
        sizeof(gJamoClustersGroup1) / sizeof(gJamoClustersGroup1[0]), 
        *aOutSeq, aLength, LBASE);
    JamoSrchReplace (gJamoClustersGroup234,
        sizeof(gJamoClustersGroup234) / sizeof(gJamoClustersGroup234[0]), 
        *aOutSeq, aLength, LBASE);
  }

  
  if (IS_VO((*aOutSeq)[0])) 
  {
     memmove(*aOutSeq + 1, *aOutSeq, *aLength * sizeof(PRUnichar));
    (*aOutSeq)[0] = LFILL;
    (*aLength)++;
  }
  
  else if (IS_TC((*aOutSeq)[0])) 
  {
    memmove (*aOutSeq + 2, *aOutSeq, *aLength * sizeof(PRUnichar));
    (*aOutSeq)[0] = LFILL;
    (*aOutSeq)[1] = VFILL;
    *aLength += 2;
  }
  return NS_OK;
}











void JamosToExtJamos (PRUnichar* aInSeq,  PRInt32* aLength)
{
  
  for (PRInt32 i = 0; i < *aLength; i++)
  {
    if (IS_LC(aInSeq[i]))
      aInSeq[i] += LC_OFFSET;
    else if (IS_VO(aInSeq[i]))
      aInSeq[i] += VO_OFFSET;
    else if (IS_TC(aInSeq[i]))
      aInSeq[i] += TC_OFFSET;
  }

  
  if ((*aLength == 2 && IS_LC_EXT(aInSeq[0]) && IS_VO_EXT(aInSeq[1])) || 
      (*aLength == 3 && IS_LC_EXT(aInSeq[0]) && IS_VO_EXT(aInSeq[1]) && 
       IS_TC_EXT(aInSeq[2])))
    return;

  
  
    
  JamoSrchReplace (gExtLcClustersGroup1, 
      sizeof (gExtLcClustersGroup1) / sizeof (gExtLcClustersGroup1[0]), 
      aInSeq, aLength, LC_TMPPOS); 
  JamoSrchReplace (gExtLcClustersGroup2,
       sizeof (gExtLcClustersGroup2) / sizeof (gExtLcClustersGroup2[0]), 
       aInSeq, aLength, LC_TMPPOS);
  JamoSrchReplace (gExtVoClustersGroup1,
       sizeof (gExtVoClustersGroup1) / sizeof (gExtVoClustersGroup1[0]), 
       aInSeq, aLength, VO_TMPPOS);
  JamoSrchReplace (gExtVoClustersGroup2, 
       sizeof (gExtVoClustersGroup2) / sizeof (gExtVoClustersGroup2[0]), 
       aInSeq, aLength, VO_TMPPOS);
  JamoSrchReplace (gExtTcClustersGroup1, 
       sizeof (gExtTcClustersGroup1) / sizeof (gExtTcClustersGroup1[0]), 
       aInSeq, aLength, TC_TMPPOS);
  JamoSrchReplace (gExtTcClustersGroup2, 
       sizeof (gExtTcClustersGroup2) / sizeof (gExtTcClustersGroup2[0]), 
       aInSeq, aLength, TC_TMPPOS);
    return;
}

