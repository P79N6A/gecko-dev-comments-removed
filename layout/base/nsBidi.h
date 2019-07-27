





#ifndef nsBidi_h__
#define nsBidi_h__

#include "nsBidiUtils.h"









































































typedef uint8_t nsBidiLevel;




#define NSBIDI_DEFAULT_LTR 0xfe




#define NSBIDI_DEFAULT_RTL 0xff






#define NSBIDI_MAX_EXPLICIT_LEVEL 125




#define NSBIDI_LEVEL_OVERRIDE 0x80









#define NSBIDI_MAP_NOWHERE (-1)




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
                                         true, (length))

#define GETINITIALLEVELSMEMORY(length) \
                                       GetMemory((void **)&mLevelsMemory, &mLevelsSize, \
                                       true, (length))

#define GETINITIALRUNSMEMORY(length) \
                                     GetMemory((void **)&mRunsMemory, &mRunsSize, \
                                     true, (length)*sizeof(Run))

#define GETINITIALISOLATESMEMORY(length) \
                                     GetMemory((void **)&mIsolatesMemory, &mIsolatesSize, \
                                     true, (length)*sizeof(Isolate))







typedef uint8_t DirProp;

#define DIRPROP_FLAG(dir) (1UL<<(dir))


#define DIRPROP_FLAG_MULTI_RUNS (1UL<<31)


#define MASK_LTR (DIRPROP_FLAG(L)|DIRPROP_FLAG(EN)|DIRPROP_FLAG(AN)|DIRPROP_FLAG(LRE)|DIRPROP_FLAG(LRO)|DIRPROP_FLAG(LRI))
#define MASK_RTL (DIRPROP_FLAG(R)|DIRPROP_FLAG(AL)|DIRPROP_FLAG(RLE)|DIRPROP_FLAG(RLO)|DIRPROP_FLAG(RLI))
#define MASK_R_AL (DIRPROP_FLAG(R)|DIRPROP_FLAG(AL))


#define MASK_EXPLICIT (DIRPROP_FLAG(LRE)|DIRPROP_FLAG(LRO)|DIRPROP_FLAG(RLE)|DIRPROP_FLAG(RLO)|DIRPROP_FLAG(PDF))


#define MASK_ISO (DIRPROP_FLAG(LRI)|DIRPROP_FLAG(RLI)|DIRPROP_FLAG(FSI)|DIRPROP_FLAG(PDI))

#define MASK_BN_EXPLICIT (DIRPROP_FLAG(BN)|MASK_EXPLICIT)


#define MASK_B_S (DIRPROP_FLAG(B)|DIRPROP_FLAG(S))


#define MASK_WS (MASK_B_S|DIRPROP_FLAG(WS)|MASK_BN_EXPLICIT|MASK_ISO)


#define MASK_POSSIBLE_N (DIRPROP_FLAG(O_N)|DIRPROP_FLAG(CS)|DIRPROP_FLAG(ES)|DIRPROP_FLAG(ET)|MASK_WS)






#define MASK_EMBEDDING (DIRPROP_FLAG(NSM)|MASK_POSSIBLE_N)


#define GET_LR_FROM_LEVEL(level) ((DirProp)((level)&1))

#define IS_DEFAULT_LEVEL(level) (((level)&0xfe)==0xfe)






#define IGNORE_CC 0x40

#define PURE_DIRPROP(prop) ((prop)&~IGNORE_CC)





#define ISOLATE 0x0100


#define SIMPLE_ISOLATES_SIZE 5



#define IS_FIRST_SURROGATE(uchar) (((uchar)&0xfc00)==0xd800)
#define IS_SECOND_SURROGATE(uchar) (((uchar)&0xfc00)==0xdc00)


#define SURROGATE_OFFSET ((0xd800<<10UL)+0xdc00-0x10000)
#define GET_UTF_32(first, second) (((first)<<10UL)+(second)-SURROGATE_OFFSET)


#define UTF_ERROR_VALUE 0xffff










#define UTF16_APPEND_CHAR_UNSAFE(s, i, c){ \
                                         if((uint32_t)(c)<=0xffff) { \
                                         (s)[(i)++]=(char16_t)(c); \
                                         } else { \
                                         (s)[(i)++]=(char16_t)((c)>>10)+0xd7c0; \
                                         (s)[(i)++]=(char16_t)(c)&0x3ff|0xdc00; \
                                         } \
}



#define UTF16_APPEND_CHAR_SAFE(s, i, length, c) { \
                                                if((PRUInt32)(c)<=0xffff) { \
                                                (s)[(i)++]=(char16_t)(c); \
                                                } else if((PRUInt32)(c)<=0x10ffff) { \
                                                if((i)+1<(length)) { \
                                                (s)[(i)++]=(char16_t)((c)>>10)+0xd7c0; \
                                                (s)[(i)++]=(char16_t)(c)&0x3ff|0xdc00; \
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
                                     int32_t __N=(n); \
                                     while(__N>0) { \
                                     UTF16_BACK_1_UNSAFE(s, i); \
                                     --__N; \
                                     } \
}



#define UTF16_PREV_CHAR_SAFE(s, start, i, c, strict) { \
                                                     (c)=(s)[--(i)]; \
                                                     if(IS_SECOND_SURROGATE(c)) { \
                                                     char16_t __c2; \
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
                                          int32_t __N=(n); \
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

#define UTF_PREV_CHAR(s, start, i, c)                UTF_PREV_CHAR_SAFE(s, start, i, c, false)
#define UTF_BACK_1(s, start, i)                      UTF_BACK_1_SAFE(s, start, i)
#define UTF_BACK_N(s, start, i, n)                   UTF_BACK_N_SAFE(s, start, i, n)
#define UTF_APPEND_CHAR(s, i, length, c)             UTF_APPEND_CHAR_SAFE(s, i, length, c)

struct Isolate {
  int32_t start1;
  int16_t stateImp;
  int16_t state;
};



typedef struct Run {
  int32_t logicalStart;  
  int32_t visualLimit;   
} Run;


#define INDEX_ODD_BIT (1UL<<31)

#define MAKE_INDEX_ODD_PAIR(index, level) (index|((uint32_t)level<<31))
#define ADD_ODD_BIT_FROM_LEVEL(x, level)  ((x)|=((uint32_t)level<<31))
#define REMOVE_ODD_BIT(x)          ((x)&=~INDEX_ODD_BIT)

#define GET_INDEX(x)   ((x)&~INDEX_ODD_BIT)
#define GET_ODD_BIT(x) ((uint32_t)(x)>>31)
#define IS_ODD_RUN(x)  (((x)&INDEX_ODD_BIT)!=0)
#define IS_EVEN_RUN(x) (((x)&INDEX_ODD_BIT)==0)

typedef uint32_t Flags;

enum { DirProp_L=0, DirProp_R=1, DirProp_EN=2, DirProp_AN=3, DirProp_ON=4, DirProp_S=5, DirProp_B=6 }; 

#define IMPTABLEVELS_COLUMNS (DirProp_B + 2)
typedef const uint8_t ImpTab[][IMPTABLEVELS_COLUMNS];
typedef const uint8_t (*PImpTab)[IMPTABLEVELS_COLUMNS];

typedef const uint8_t ImpAct[];
typedef const uint8_t *PImpAct;

struct LevState {
    PImpTab pImpTab;                    
    PImpAct pImpAct;                    
    int32_t startON;                    
    int32_t state;                      
    int32_t runStart;                   
    nsBidiLevel runLevel;               
};

















class nsBidi
{
public:
  










  nsBidi();

  
  virtual ~nsBidi();


  




























































  nsresult SetPara(const char16_t *aText, int32_t aLength, nsBidiLevel aParaLevel, nsBidiLevel *aEmbeddingLevels);

  








  nsresult GetDirection(nsBidiDirection* aDirection);

  






  nsresult GetParaLevel(nsBidiLevel* aParaLevel);

#ifdef FULL_BIDI_ENGINE
  
































  nsresult SetLine(const nsBidi* aParaBidi, int32_t aStart, int32_t aLimit);

  




  nsresult GetLength(int32_t* aLength);

  








  nsresult GetLevelAt(int32_t aCharIndex,  nsBidiLevel* aLevel);

  










  nsresult GetLevels(nsBidiLevel** aLevels);
#endif 
  






  nsresult GetCharTypeAt(int32_t aCharIndex,  nsCharType* aType);

  


















  nsresult GetLogicalRun(int32_t aLogicalStart, int32_t* aLogicalLimit, nsBidiLevel* aLevel);

  









  nsresult CountRuns(int32_t* aRunCount);

  















































  nsresult GetVisualRun(int32_t aRunIndex, int32_t* aLogicalStart, int32_t* aLength, nsBidiDirection* aDirection);

#ifdef FULL_BIDI_ENGINE
  
















  nsresult GetVisualIndex(int32_t aLogicalIndex, int32_t* aVisualIndex);

  














  nsresult GetLogicalIndex(int32_t aVisualIndex, int32_t* aLogicalIndex);

  











  nsresult GetLogicalMap(int32_t *aIndexMap);

  











  nsresult GetVisualMap(int32_t *aIndexMap);

  


















  static nsresult ReorderLogical(const nsBidiLevel *aLevels, int32_t aLength, int32_t *aIndexMap);
#endif 
  


















  static nsresult ReorderVisual(const nsBidiLevel *aLevels, int32_t aLength, int32_t *aIndexMap);

#ifdef FULL_BIDI_ENGINE
  












  nsresult InvertMap(const int32_t *aSrcMap, int32_t *aDestMap, int32_t aLength);
#endif 
  


































  nsresult WriteReverse(const char16_t *aSrc, int32_t aSrcLength, char16_t *aDest, uint16_t aOptions, int32_t *aDestSize);

protected:
  friend class nsBidiPresUtils;

  
  int32_t mLength;

  
  size_t mDirPropsSize, mLevelsSize, mRunsSize;
  size_t mIsolatesSize;

  
  DirProp* mDirPropsMemory;
  nsBidiLevel* mLevelsMemory;
  Run* mRunsMemory;
  Isolate* mIsolatesMemory;

  
  bool mMayAllocateText, mMayAllocateRuns;

  DirProp* mDirProps;
  nsBidiLevel* mLevels;

  
  nsBidiLevel mParaLevel;

  
  Flags mFlags;

  
  nsBidiDirection mDirection;

  
  
  int32_t mTrailingWSStart;

  
  int32_t mRunCount;     
  Run* mRuns;

  
  Run mSimpleRuns[1];

  
  



  int32_t mIsolateCount;
  Isolate* mIsolates;

  
  Isolate mSimpleIsolates[SIMPLE_ISOLATES_SIZE];

private:

  void Init();

  bool GetMemory(void **aMemory, size_t* aSize, bool aMayAllocate, size_t aSizeNeeded);

  void Free();

  void GetDirProps(const char16_t *aText);

  void ResolveExplicitLevels(nsBidiDirection *aDirection);

  nsresult CheckExplicitLevels(nsBidiDirection *aDirection);

  nsBidiDirection DirectionFromFlags(Flags aFlags);

  void ProcessPropertySeq(LevState *pLevState, uint8_t _prop, int32_t start, int32_t limit);

  void ResolveImplicitLevels(int32_t aStart, int32_t aLimit, DirProp aSOR, DirProp aEOR);

  void AdjustWSLevels();

  void SetTrailingWSStart();

  bool GetRuns();

  void GetSingleRun(nsBidiLevel aLevel);

  void ReorderLine(nsBidiLevel aMinLevel, nsBidiLevel aMaxLevel);

  static bool PrepareReorder(const nsBidiLevel *aLevels, int32_t aLength, int32_t *aIndexMap, nsBidiLevel *aMinLevel, nsBidiLevel *aMaxLevel);

  int32_t doWriteReverse(const char16_t *src, int32_t srcLength,
                         char16_t *dest, uint16_t options);

};

#endif 
