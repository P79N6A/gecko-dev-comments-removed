






































#ifndef nsBidi_h__
#define nsBidi_h__

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsBidiUtils.h"









































































typedef PRUint8 nsBidiLevel;




#define NSBIDI_DEFAULT_LTR 0xfe




#define NSBIDI_DEFAULT_RTL 0xff






#define NSBIDI_MAX_EXPLICIT_LEVEL 61




#define NSBIDI_LEVEL_OVERRIDE 0x80




enum nsBidiDirection {
  
  NSBIDI_LTR,
  
  NSBIDI_RTL,
  
  NSBIDI_MIXED
};

typedef enum nsBidiDirection nsBidiDirection;









#define NSBIDI_KEEP_BASE_COMBINING       1








#define NSBIDI_DO_MIRRORING              2







#define NSBIDI_REMOVE_BIDI_CONTROLS      8


#define GETDIRPROPSMEMORY(length) \
                                  GetMemory((void **)&mDirPropsMemory, &mDirPropsSize, \
                                  mMayAllocateText, (length))

#define GETLEVELSMEMORY(length) \
                                GetMemory((void **)&mLevelsMemory, &mLevelsSize, \
                                mMayAllocateText, (length))

#define GETRUNSMEMORY(length) \
                              GetMemory((void **)&mRunsMemory, &mRunsSize, \
                              mMayAllocateRuns, (length)*sizeof(Run))


#define GETINITIALDIRPROPSMEMORY(length) \
                                         GetMemory((void **)&mDirPropsMemory, &mDirPropsSize, \
                                         PR_TRUE, (length))

#define GETINITIALLEVELSMEMORY(length) \
                                       GetMemory((void **)&mLevelsMemory, &mLevelsSize, \
                                       PR_TRUE, (length))

#define GETINITIALRUNSMEMORY(length) \
                                     GetMemory((void **)&mRunsMemory, &mRunsSize, \
                                     PR_TRUE, (length)*sizeof(Run))







typedef PRUint8 DirProp;

#define DIRPROP_FLAG(dir) (1UL<<(dir))


#define DIRPROP_FLAG_MULTI_RUNS (1UL<<31)


#define MASK_LTR (DIRPROP_FLAG(L)|DIRPROP_FLAG(EN)|DIRPROP_FLAG(AN)|DIRPROP_FLAG(LRE)|DIRPROP_FLAG(LRO))
#define MASK_RTL (DIRPROP_FLAG(R)|DIRPROP_FLAG(AL)|DIRPROP_FLAG(RLE)|DIRPROP_FLAG(RLO))


#define MASK_LRX (DIRPROP_FLAG(LRE)|DIRPROP_FLAG(LRO))
#define MASK_RLX (DIRPROP_FLAG(RLE)|DIRPROP_FLAG(RLO))
#define MASK_OVERRIDE (DIRPROP_FLAG(LRO)|DIRPROP_FLAG(RLO))

#define MASK_EXPLICIT (MASK_LRX|MASK_RLX|DIRPROP_FLAG(PDF))
#define MASK_BN_EXPLICIT (DIRPROP_FLAG(BN)|MASK_EXPLICIT)


#define MASK_B_S (DIRPROP_FLAG(B)|DIRPROP_FLAG(S))


#define MASK_WS (MASK_B_S|DIRPROP_FLAG(WS)|MASK_BN_EXPLICIT)
#define MASK_N (DIRPROP_FLAG(O_N)|MASK_WS)


#define MASK_ET_NSM_BN (DIRPROP_FLAG(ET)|DIRPROP_FLAG(NSM)|MASK_BN_EXPLICIT)


#define MASK_POSSIBLE_N (DIRPROP_FLAG(CS)|DIRPROP_FLAG(ES)|DIRPROP_FLAG(ET)|MASK_N)






#define MASK_EMBEDDING (DIRPROP_FLAG(NSM)|MASK_POSSIBLE_N)


#define GET_LR_FROM_LEVEL(level) ((DirProp)((level)&1))

#define IS_DEFAULT_LEVEL(level) (((level)&0xfe)==0xfe)



#define IS_FIRST_SURROGATE(uchar) (((uchar)&0xfc00)==0xd800)
#define IS_SECOND_SURROGATE(uchar) (((uchar)&0xfc00)==0xdc00)


#define SURROGATE_OFFSET ((0xd800<<10UL)+0xdc00-0x10000)
#define GET_UTF_32(first, second) (((first)<<10UL)+(second)-SURROGATE_OFFSET)


#define UTF_ERROR_VALUE 0xffff










#define UTF16_APPEND_CHAR_UNSAFE(s, i, c){ \
                                         if((PRUint32)(c)<=0xffff) { \
                                         (s)[(i)++]=(PRUnichar)(c); \
                                         } else { \
                                         (s)[(i)++]=(PRUnichar)((c)>>10)+0xd7c0; \
                                         (s)[(i)++]=(PRUnichar)(c)&0x3ff|0xdc00; \
                                         } \
}



#define UTF16_APPEND_CHAR_SAFE(s, i, length, c) { \
                                                if((PRUInt32)(c)<=0xffff) { \
                                                (s)[(i)++]=(PRUnichar)(c); \
                                                } else if((PRUInt32)(c)<=0x10ffff) { \
                                                if((i)+1<(length)) { \
                                                (s)[(i)++]=(PRUnichar)((c)>>10)+0xd7c0; \
                                                (s)[(i)++]=(PRUnichar)(c)&0x3ff|0xdc00; \
                                                } else /* not enough space */ { \
                                                (s)[(i)++]=UTF_ERROR_VALUE; \
                                                } \
                                                } else /* c>0x10ffff, write error value */ { \
                                                (s)[(i)++]=UTF_ERROR_VALUE; \
                                                } \
}

















#define UTF16_PREV_CHAR_UNSAFE(s, i, c) { \
                                        (c)=(s)[--(i)]; \
                                        if(IS_SECOND_SURROGATE(c)) { \
                                        (c)=GET_UTF_32((s)[--(i)], (c)); \
                                        } \
}

#define UTF16_BACK_1_UNSAFE(s, i) { \
                                  if(IS_SECOND_SURROGATE((s)[--(i)])) { \
                                  --(i); \
                                  } \
}

#define UTF16_BACK_N_UNSAFE(s, i, n) { \
                                     PRInt32 __N=(n); \
                                     while(__N>0) { \
                                     UTF16_BACK_1_UNSAFE(s, i); \
                                     --__N; \
                                     } \
}



#define UTF16_PREV_CHAR_SAFE(s, start, i, c, strict) { \
                                                     (c)=(s)[--(i)]; \
                                                     if(IS_SECOND_SURROGATE(c)) { \
                                                     PRUnichar __c2; \
                                                     if((i)>(start) && IS_FIRST_SURROGATE(__c2=(s)[(i)-1])) { \
                                                     --(i); \
                                                     (c)=GET_UTF_32(__c2, (c)); \
      /* strict: ((c)&0xfffe)==0xfffe is caught by UTF_IS_ERROR() */ \
                                                     } else if(strict) {\
      /* unmatched second surrogate */ \
                                                     (c)=UTF_ERROR_VALUE; \
                                                     } \
                                                     } else if(strict && IS_FIRST_SURROGATE(c)) { \
      /* unmatched first surrogate */ \
                                                     (c)=UTF_ERROR_VALUE; \
  /* else strict: (c)==0xfffe is caught by UTF_IS_ERROR() */ \
                                                     } \
}

#define UTF16_BACK_1_SAFE(s, start, i) { \
                                       if(IS_SECOND_SURROGATE((s)[--(i)]) && (i)>(start) && IS_FIRST_SURROGATE((s)[(i)-1])) { \
                                       --(i); \
                                       } \
}

#define UTF16_BACK_N_SAFE(s, start, i, n) { \
                                          PRInt32 __N=(n); \
                                          while(__N>0 && (i)>(start)) { \
                                          UTF16_BACK_1_SAFE(s, start, i); \
                                          --__N; \
                                          } \
}

#define UTF_PREV_CHAR_UNSAFE(s, i, c)                UTF16_PREV_CHAR_UNSAFE(s, i, c)
#define UTF_PREV_CHAR_SAFE(s, start, i, c, strict)   UTF16_PREV_CHAR_SAFE(s, start, i, c, strict)
#define UTF_BACK_1_UNSAFE(s, i)                      UTF16_BACK_1_UNSAFE(s, i)
#define UTF_BACK_1_SAFE(s, start, i)                 UTF16_BACK_1_SAFE(s, start, i)
#define UTF_BACK_N_UNSAFE(s, i, n)                   UTF16_BACK_N_UNSAFE(s, i, n)
#define UTF_BACK_N_SAFE(s, start, i, n)              UTF16_BACK_N_SAFE(s, start, i, n)
#define UTF_APPEND_CHAR_UNSAFE(s, i, c)              UTF16_APPEND_CHAR_UNSAFE(s, i, c)
#define UTF_APPEND_CHAR_SAFE(s, i, length, c)        UTF16_APPEND_CHAR_SAFE(s, i, length, c)

#define UTF_PREV_CHAR(s, start, i, c)                UTF_PREV_CHAR_SAFE(s, start, i, c, PR_FALSE)
#define UTF_BACK_1(s, start, i)                      UTF_BACK_1_SAFE(s, start, i)
#define UTF_BACK_N(s, start, i, n)                   UTF_BACK_N_SAFE(s, start, i, n)
#define UTF_APPEND_CHAR(s, i, length, c)             UTF_APPEND_CHAR_SAFE(s, i, length, c)



typedef struct Run {
  PRInt32 logicalStart,  
  visualLimit;  
} Run;


#define INDEX_ODD_BIT (1UL<<31)

#define MAKE_INDEX_ODD_PAIR(index, level) (index|((PRUint32)level<<31))
#define ADD_ODD_BIT_FROM_LEVEL(x, level)  ((x)|=((PRUint32)level<<31))
#define REMOVE_ODD_BIT(x)          ((x)&=~INDEX_ODD_BIT)

#define GET_INDEX(x)   (x&~INDEX_ODD_BIT)
#define GET_ODD_BIT(x) ((PRUint32)x>>31)
#define IS_ODD_RUN(x)  ((x&INDEX_ODD_BIT)!=0)
#define IS_EVEN_RUN(x) ((x&INDEX_ODD_BIT)==0)

typedef PRUint32 Flags;

















class nsBidi
{
public: 
  










  nsBidi();

  
































  nsBidi(PRUint32 aMaxLength, PRUint32 aMaxRunCount);

  
  virtual ~nsBidi();


  




























































  nsresult SetPara(const PRUnichar *aText, PRInt32 aLength, nsBidiLevel aParaLevel, nsBidiLevel *aEmbeddingLevels);

#ifdef FULL_BIDI_ENGINE
  
































  nsresult SetLine(nsIBidi* aParaBidi, PRInt32 aStart, PRInt32 aLimit);  

  








  nsresult GetDirection(nsBidiDirection* aDirection);

  




  nsresult GetLength(PRInt32* aLength);

  






  nsresult GetParaLevel(nsBidiLevel* aParaLevel);

  








  nsresult GetLevelAt(PRInt32 aCharIndex,  nsBidiLevel* aLevel);

  










  nsresult GetLevels(nsBidiLevel** aLevels);
#endif 
  






  nsresult GetCharTypeAt(PRInt32 aCharIndex,  nsCharType* aType);

  


















  nsresult GetLogicalRun(PRInt32 aLogicalStart, PRInt32* aLogicalLimit, nsBidiLevel* aLevel);

  









  nsresult CountRuns(PRInt32* aRunCount);

  















































  nsresult GetVisualRun(PRInt32 aRunIndex, PRInt32* aLogicalStart, PRInt32* aLength, nsBidiDirection* aDirection);

#ifdef FULL_BIDI_ENGINE
  
















  nsresult GetVisualIndex(PRInt32 aLogicalIndex, PRInt32* aVisualIndex);

  














  nsresult GetLogicalIndex(PRInt32 aVisualIndex, PRInt32* aLogicalIndex);

  











  nsresult GetLogicalMap(PRInt32 *aIndexMap);

  











  nsresult GetVisualMap(PRInt32 *aIndexMap);

  


















  nsresult ReorderLogical(const nsBidiLevel *aLevels, PRInt32 aLength, PRInt32 *aIndexMap);
#endif 
  


















  nsresult ReorderVisual(const nsBidiLevel *aLevels, PRInt32 aLength, PRInt32 *aIndexMap);

#ifdef FULL_BIDI_ENGINE
  












  nsresult InvertMap(const PRInt32 *aSrcMap, PRInt32 *aDestMap, PRInt32 aLength);
#endif 
  


































  nsresult WriteReverse(const PRUnichar *aSrc, PRInt32 aSrcLength, PRUnichar *aDest, PRUint16 aOptions, PRInt32 *aDestSize);

protected:
  
  PRInt32 mLength;

  
  PRSize mDirPropsSize, mLevelsSize, mRunsSize;

  
  DirProp* mDirPropsMemory;
  nsBidiLevel* mLevelsMemory;
  Run* mRunsMemory;

  
  PRBool mMayAllocateText, mMayAllocateRuns;

  const DirProp* mDirProps;
  nsBidiLevel* mLevels;

  
  nsBidiLevel mParaLevel;

  
  Flags mFlags;

  
  nsBidiDirection mDirection;

  
  
  PRInt32 mTrailingWSStart;

  
  PRInt32 mRunCount;     
  Run* mRuns;

  
  Run mSimpleRuns[1];

private:

  void Init();

  PRBool GetMemory(void **aMemory, PRSize* aSize, PRBool aMayAllocate, PRSize aSizeNeeded);

  void Free();

  void GetDirProps(const PRUnichar *aText);

  nsBidiDirection ResolveExplicitLevels();

  nsresult CheckExplicitLevels(nsBidiDirection *aDirection);

  nsBidiDirection DirectionFromFlags(Flags aFlags);

  void ResolveImplicitLevels(PRInt32 aStart, PRInt32 aLimit, DirProp aSOR, DirProp aEOR);

  void AdjustWSLevels();

  void SetTrailingWSStart();

  PRBool GetRuns();

  void GetSingleRun(nsBidiLevel aLevel);

  void ReorderLine(nsBidiLevel aMinLevel, nsBidiLevel aMaxLevel);

  PRBool PrepareReorder(const nsBidiLevel *aLevels, PRInt32 aLength, PRInt32 *aIndexMap, nsBidiLevel *aMinLevel, nsBidiLevel *aMaxLevel);

  PRInt32 doWriteReverse(const PRUnichar *src, PRInt32 srcLength,
                         PRUnichar *dest, PRUint16 options);

};

#endif 
