





#include "nsBidi.h"
#include "nsUnicodeProperties.h"
#include "nsCRTGlue.h"

using namespace mozilla::unicode;


#undef CS
#undef ES




enum {
    L =   eCharType_LeftToRight,
    R =   eCharType_RightToLeft,
    EN =  eCharType_EuropeanNumber,
    ES =  eCharType_EuropeanNumberSeparator,
    ET =  eCharType_EuropeanNumberTerminator,
    AN =  eCharType_ArabicNumber,
    CS =  eCharType_CommonNumberSeparator,
    B =   eCharType_BlockSeparator,
    S =   eCharType_SegmentSeparator,
    WS =  eCharType_WhiteSpaceNeutral,
    O_N = eCharType_OtherNeutral,
    LRE = eCharType_LeftToRightEmbedding,
    LRO = eCharType_LeftToRightOverride,
    AL =  eCharType_RightToLeftArabic,
    RLE = eCharType_RightToLeftEmbedding,
    RLO = eCharType_RightToLeftOverride,
    PDF = eCharType_PopDirectionalFormat,
    NSM = eCharType_DirNonSpacingMark,
    BN =  eCharType_BoundaryNeutral,
    LRI = eCharType_LeftToRightIsolate,
    RLI = eCharType_RightToLeftIsolate,
    FSI = eCharType_FirstStrongIsolate,
    PDI = eCharType_PopDirectionalIsolate,
    dirPropCount
};


static Flags flagLR[2]={ DIRPROP_FLAG(L), DIRPROP_FLAG(R) };
static Flags flagE[2]={ DIRPROP_FLAG(LRE), DIRPROP_FLAG(RLE) };
static Flags flagO[2]={ DIRPROP_FLAG(LRO), DIRPROP_FLAG(RLO) };

#define DIRPROP_FLAG_LR(level) flagLR[(level)&1]
#define DIRPROP_FLAG_E(level) flagE[(level)&1]
#define DIRPROP_FLAG_O(level) flagO[(level)&1]



















































































nsBidi::nsBidi()
{
  Init();

  mMayAllocateText=true;
  mMayAllocateRuns=true;
}

nsBidi::~nsBidi()
{
  Free();
}

void nsBidi::Init()
{
  
  mLength = 0;
  mParaLevel = 0;
  mFlags = 0;
  mDirection = NSBIDI_LTR;
  mTrailingWSStart = 0;

  mDirPropsSize = 0;
  mLevelsSize = 0;
  mRunsSize = 0;
  mIsolatesSize = 0;

  mRunCount = -1;
  mIsolateCount = -1;

  mDirProps=nullptr;
  mLevels=nullptr;
  mRuns=nullptr;
  mIsolates=nullptr;

  mDirPropsMemory=nullptr;
  mLevelsMemory=nullptr;
  mRunsMemory=nullptr;
  mIsolatesMemory=nullptr;

  mMayAllocateText=false;
  mMayAllocateRuns=false;
}














bool nsBidi::GetMemory(void **aMemory, size_t *aSize, bool aMayAllocate, size_t aSizeNeeded)
{
  
  if(*aMemory==nullptr) {
    
    if(!aMayAllocate) {
      return false;
    } else {
      *aMemory=malloc(aSizeNeeded);
      if (*aMemory!=nullptr) {
        *aSize=aSizeNeeded;
        return true;
      } else {
        *aSize=0;
        return false;
      }
    }
  } else {
    
    if(aSizeNeeded>*aSize && !aMayAllocate) {
      
      return false;
    } else if(aSizeNeeded!=*aSize && aMayAllocate) {
      
      void *memory=realloc(*aMemory, aSizeNeeded);

      if(memory!=nullptr) {
        *aMemory=memory;
        *aSize=aSizeNeeded;
        return true;
      } else {
        
        return false;
      }
    } else {
      
      return true;
    }
  }
}

void nsBidi::Free()
{
  free(mDirPropsMemory);
  mDirPropsMemory = nullptr;
  free(mLevelsMemory);
  mLevelsMemory = nullptr;
  free(mRunsMemory);
  mRunsMemory = nullptr;
  free(mIsolatesMemory);
  mIsolatesMemory = nullptr;
}



nsresult nsBidi::SetPara(const char16_t *aText, int32_t aLength,
                         nsBidiLevel aParaLevel, nsBidiLevel *aEmbeddingLevels)
{
  nsBidiDirection direction;

  
  if(aText==nullptr ||
     ((NSBIDI_MAX_EXPLICIT_LEVEL<aParaLevel) && !IS_DEFAULT_LEVEL(aParaLevel)) ||
     aLength<-1
    ) {
    return NS_ERROR_INVALID_ARG;
  }

  if(aLength==-1) {
    aLength = NS_strlen(aText);
  }

  
  mLength = aLength;
  mParaLevel=aParaLevel;
  mDirection=aParaLevel & 1 ? NSBIDI_RTL : NSBIDI_LTR;
  mTrailingWSStart=aLength;  

  mDirProps=nullptr;
  mLevels=nullptr;
  mRuns=nullptr;

  if(aLength==0) {
    




    if(IS_DEFAULT_LEVEL(aParaLevel)) {
      mParaLevel&=1;
    }
    mFlags=DIRPROP_FLAG_LR(aParaLevel);
    mRunCount=0;
    return NS_OK;
  }

  mRunCount=-1;

  




  if(GETDIRPROPSMEMORY(aLength)) {
    mDirProps=mDirPropsMemory;
    GetDirProps(aText);
  } else {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  if(aEmbeddingLevels==nullptr) {
    \
    if(GETLEVELSMEMORY(aLength)) {
      mLevels=mLevelsMemory;
      ResolveExplicitLevels(&direction);
    } else {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  } else {
    
    mLevels=aEmbeddingLevels;
    nsresult rv = CheckExplicitLevels(&direction);
    if(NS_FAILED(rv)) {
      return rv;
    }
  }

  
  if (mIsolateCount <= SIMPLE_ISOLATES_SIZE) {
    mIsolates = mSimpleIsolates;
  } else {
    if (mIsolateCount <= (int32_t) mIsolatesSize) {
      mIsolates = mIsolatesMemory;
    } else {
      if (GETINITIALISOLATESMEMORY(mIsolateCount)) {
        mIsolates = mIsolatesMemory;
      } else {
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }
  mIsolateCount = -1;  

  



  mDirection = direction;
  switch(direction) {
    case NSBIDI_LTR:
      
      mParaLevel=(mParaLevel+1)&~1;

      
      mTrailingWSStart=0;
      break;
    case NSBIDI_RTL:
      
      mParaLevel|=1;

      
      mTrailingWSStart=0;
      break;
    default:
      










      if(aEmbeddingLevels==nullptr && !(mFlags&DIRPROP_FLAG_MULTI_RUNS)) {
        ResolveImplicitLevels(0, aLength,
                    GET_LR_FROM_LEVEL(mParaLevel),
                    GET_LR_FROM_LEVEL(mParaLevel));
      } else {
        
        nsBidiLevel *levels=mLevels;
        int32_t start, limit=0;
        nsBidiLevel level, nextLevel;
        DirProp sor, eor;

        
        level=mParaLevel;
        nextLevel=levels[0];
        if(level<nextLevel) {
          eor=GET_LR_FROM_LEVEL(nextLevel);
        } else {
          eor=GET_LR_FROM_LEVEL(level);
        }

        do {
          

          
          sor=eor;
          start=limit;
          level=nextLevel;

          
          while(++limit<aLength && levels[limit]==level) {}

          
          if(limit<aLength) {
            nextLevel=levels[limit];
          } else {
            nextLevel=mParaLevel;
          }

          
          if((level&~NSBIDI_LEVEL_OVERRIDE)<(nextLevel&~NSBIDI_LEVEL_OVERRIDE)) {
            eor=GET_LR_FROM_LEVEL(nextLevel);
          } else {
            eor=GET_LR_FROM_LEVEL(level);
          }

          

          if(!(level&NSBIDI_LEVEL_OVERRIDE)) {
            ResolveImplicitLevels(start, limit, sor, eor);
          } else {
            do {
              levels[start++] &= ~NSBIDI_LEVEL_OVERRIDE;
            } while (start < limit);
          }
        } while(limit<aLength);
      }

      
      AdjustWSLevels();
      break;
  }

  return NS_OK;
}








void nsBidi::GetDirProps(const char16_t *aText)
{
  DirProp *dirProps=mDirPropsMemory;    

  int32_t i=0, length=mLength;
  Flags flags=0;      
  char16_t uchar;
  DirProp dirProp;

  bool isDefaultLevel = IS_DEFAULT_LEVEL(mParaLevel);

  enum State {
    NOT_SEEKING_STRONG,       
    SEEKING_STRONG_FOR_PARA,  
    SEEKING_STRONG_FOR_FSI,   
    LOOKING_FOR_PDI           
  };
  State state;

  




  

  int32_t isolateStartStack[NSBIDI_MAX_EXPLICIT_LEVEL + 1];
  

  State previousStateStack[NSBIDI_MAX_EXPLICIT_LEVEL + 1];
  int32_t stackLast = -1;

  if(isDefaultLevel) {
    




    mParaLevel &= 1;
    state = SEEKING_STRONG_FOR_PARA;
  } else {
    state = NOT_SEEKING_STRONG;
  }

  
  for(; i < length;) {
    uchar=aText[i];
    if(!IS_FIRST_SURROGATE(uchar) || i+1==length || !IS_SECOND_SURROGATE(aText[i+1])) {
      
      flags|=DIRPROP_FLAG(dirProps[i]=dirProp=GetBidiCat((uint32_t)uchar));
    } else {
      
      dirProps[i++]=BN;   
      flags|=DIRPROP_FLAG(dirProps[i]=dirProp=GetBidiCat(GET_UTF_32(uchar, aText[i])))|DIRPROP_FLAG(BN);
    }
    ++i;

    switch (dirProp) {
    case L:
      if (state == SEEKING_STRONG_FOR_PARA) {
        mParaLevel = 0;
        state = NOT_SEEKING_STRONG;
      } else if  (state == SEEKING_STRONG_FOR_FSI) {
        if (stackLast <= NSBIDI_MAX_EXPLICIT_LEVEL) {
          dirProps[isolateStartStack[stackLast]] = LRI;
          flags |= LRI;
        }
        state = LOOKING_FOR_PDI;
      }
      break;

    case R: case AL:
      if (state == SEEKING_STRONG_FOR_PARA) {
        mParaLevel = 1;
        state = NOT_SEEKING_STRONG;
      } else if  (state == SEEKING_STRONG_FOR_FSI) {
        if (stackLast <= NSBIDI_MAX_EXPLICIT_LEVEL) {
          dirProps[isolateStartStack[stackLast]] = RLI;
          flags |= RLI;
        }
        state = LOOKING_FOR_PDI;
      }
      break;

    case FSI: case LRI: case RLI:
      stackLast++;
      if (stackLast <= NSBIDI_MAX_EXPLICIT_LEVEL) {
        isolateStartStack[stackLast] = i - 1;
        previousStateStack[stackLast] = state;
      }
      if (dirProp == FSI) {
        state = SEEKING_STRONG_FOR_FSI;
      } else {
        state = LOOKING_FOR_PDI;
      }
      break;

    case PDI:
      if (state == SEEKING_STRONG_FOR_FSI) {
        if (stackLast <= NSBIDI_MAX_EXPLICIT_LEVEL) {
          dirProps[isolateStartStack[stackLast]] = LRI;
          flags |= DIRPROP_FLAG(LRI);
        }
      }
      if (stackLast >= 0) {
        if (stackLast <= NSBIDI_MAX_EXPLICIT_LEVEL) {
          state = previousStateStack[stackLast];
        }
        stackLast--;
      }
      break;

    case B:
      
      NS_NOTREACHED("Unexpected paragraph separator");
      break;

    default:
      break;
    }
  }

  
  if (stackLast > NSBIDI_MAX_EXPLICIT_LEVEL) {
    stackLast = NSBIDI_MAX_EXPLICIT_LEVEL;
    if (dirProps[previousStateStack[NSBIDI_MAX_EXPLICIT_LEVEL]] != FSI) {
      state = LOOKING_FOR_PDI;
    }
  }

  
  while (stackLast >= 0) {
    if (state == SEEKING_STRONG_FOR_FSI) {
      dirProps[isolateStartStack[stackLast]] = LRI;
      flags |= DIRPROP_FLAG(LRI);
    }
    state = previousStateStack[stackLast];
    stackLast--;
  }

  flags|=DIRPROP_FLAG_LR(mParaLevel);

  mFlags = flags;
}



















































void nsBidi::ResolveExplicitLevels(nsBidiDirection *aDirection)
{
  DirProp *dirProps=mDirProps;
  nsBidiLevel *levels=mLevels;

  int32_t i=0, length=mLength;
  Flags flags=mFlags;       
  DirProp dirProp;
  nsBidiLevel level=mParaLevel;
  nsBidiDirection direction;

  mIsolateCount = 0;

  
  direction=DirectionFromFlags(flags);

  
  if(direction!=NSBIDI_MIXED) {
    
  } else if(!(flags&(MASK_EXPLICIT|MASK_ISO))) {
    
    for(i=0; i<length; ++i) {
      levels[i]=level;
    }
  } else {
    

    
    
    nsBidiLevel embeddingLevel = level, newLevel;
    nsBidiLevel previousLevel = level;     

    uint16_t stack[NSBIDI_MAX_EXPLICIT_LEVEL + 2];   

    uint32_t stackLast = 0;
    int32_t overflowIsolateCount = 0;
    int32_t overflowEmbeddingCount = 0;
    int32_t validIsolateCount = 0;

    stack[0] = level;

    
    flags=0;

    
    for(i=0; i<length; ++i) {
      dirProp=dirProps[i];
      switch(dirProp) {
        case LRE:
        case RLE:
        case LRO:
        case RLO:
          
          flags |= DIRPROP_FLAG(BN);
          if (dirProp == LRE || dirProp == LRO) {
            newLevel = (embeddingLevel + 2) & ~(NSBIDI_LEVEL_OVERRIDE | 1);    
          } else {
            newLevel = ((embeddingLevel & ~NSBIDI_LEVEL_OVERRIDE) + 1) | 1;    
          }
          if(newLevel <= NSBIDI_MAX_EXPLICIT_LEVEL && overflowIsolateCount == 0 && overflowEmbeddingCount == 0) {
            embeddingLevel = newLevel;
            if (dirProp == LRO || dirProp == RLO) {
              embeddingLevel |= NSBIDI_LEVEL_OVERRIDE;
            }
            stackLast++;
            stack[stackLast] = embeddingLevel;
            



          } else {
            dirProps[i] |= IGNORE_CC;
            if (overflowIsolateCount == 0) {
              overflowEmbeddingCount++;
            }
          }
          break;

        case PDF:
          
          flags |= DIRPROP_FLAG(BN);
          
          if (overflowIsolateCount) {
            dirProps[i] |= IGNORE_CC;
            break;
          }
          if (overflowEmbeddingCount) {
            dirProps[i] |= IGNORE_CC;
            overflowEmbeddingCount--;
            break;
          }
          if (stackLast > 0 && stack[stackLast] < ISOLATE) {   
            stackLast--;
            embeddingLevel = stack[stackLast];
          } else {
            dirProps[i] |= IGNORE_CC;
          }
          break;

        case LRI:
        case RLI:
          if (embeddingLevel != previousLevel) {
            previousLevel = embeddingLevel;
          }
          
          flags |= DIRPROP_FLAG(O_N) | DIRPROP_FLAG(BN) | DIRPROP_FLAG_LR(embeddingLevel);
          level = embeddingLevel;
          if (dirProp == LRI) {
            newLevel = (embeddingLevel + 2) & ~(NSBIDI_LEVEL_OVERRIDE | 1); 
          } else {
            newLevel = ((embeddingLevel & ~NSBIDI_LEVEL_OVERRIDE) + 1) | 1;  
          }
          if (newLevel <= NSBIDI_MAX_EXPLICIT_LEVEL && overflowIsolateCount == 0 && overflowEmbeddingCount == 0) {
            previousLevel = embeddingLevel;
            validIsolateCount++;
            if (validIsolateCount > mIsolateCount) {
              mIsolateCount = validIsolateCount;
            }
            embeddingLevel = newLevel;
            stackLast++;
            stack[stackLast] = embeddingLevel + ISOLATE;
          } else {
            dirProps[i] |= IGNORE_CC;
            overflowIsolateCount++;
          }
          break;

        case PDI:
          
          if (overflowIsolateCount) {
            dirProps[i] |= IGNORE_CC;
            overflowIsolateCount--;
          } else if (validIsolateCount) {
            overflowEmbeddingCount = 0;
            while (stack[stackLast] < ISOLATE) {
              
              
              stackLast--;
            }
            stackLast--;  
            validIsolateCount--;
          } else {
            dirProps[i] |= IGNORE_CC;
          }
          embeddingLevel = stack[stackLast] & ~ISOLATE;
          previousLevel = level = embeddingLevel;
          flags |= DIRPROP_FLAG(O_N) | DIRPROP_FLAG(BN) | DIRPROP_FLAG_LR(embeddingLevel);
          break;

        case B:
          


          NS_NOTREACHED("Unexpected paragraph separator");
          break;

        case BN:
          
          
          flags|=DIRPROP_FLAG(BN);
          break;

        default:
          
          level = embeddingLevel;
          if(embeddingLevel != previousLevel) {
            previousLevel = embeddingLevel;
          }

          if (level & NSBIDI_LEVEL_OVERRIDE) {
            flags |= DIRPROP_FLAG_LR(level);
          } else {
            flags |= DIRPROP_FLAG(dirProp);
          }
          break;
      }

      



      levels[i]=level;
      if (i > 0 && levels[i - 1] != level) {
        flags |= DIRPROP_FLAG_MULTI_RUNS;
        if (level & NSBIDI_LEVEL_OVERRIDE) {
          flags |= DIRPROP_FLAG_O(level);
        } else {
          flags |= DIRPROP_FLAG_E(level);
        }
      }
      if (DIRPROP_FLAG(dirProp) & MASK_ISO) {
        level = embeddingLevel;
      }
    }

    if(flags&MASK_EMBEDDING) {
      flags|=DIRPROP_FLAG_LR(mParaLevel);
    }

    

    
    mFlags=flags;
    direction=DirectionFromFlags(flags);
  }

  *aDirection = direction;
}











nsresult nsBidi::CheckExplicitLevels(nsBidiDirection *aDirection)
{
  const DirProp *dirProps=mDirProps;
  DirProp dirProp;
  nsBidiLevel *levels=mLevels;
  int32_t isolateCount = 0;

  int32_t i, length=mLength;
  Flags flags=0;  
  nsBidiLevel level, paraLevel=mParaLevel;
  mIsolateCount = 0;

  for(i=0; i<length; ++i) {
    level=levels[i];
    dirProp = dirProps[i];
    if (dirProp == LRI || dirProp == RLI) {
      isolateCount++;
      if (isolateCount > mIsolateCount) {
        mIsolateCount = isolateCount;
      }
    } else if (dirProp == PDI) {
      isolateCount--;
    }
    if(level&NSBIDI_LEVEL_OVERRIDE) {
      
      level&=~NSBIDI_LEVEL_OVERRIDE;     
      flags|=DIRPROP_FLAG_O(level);
    } else {
      
      flags|=DIRPROP_FLAG_E(level)|DIRPROP_FLAG(dirProp);
    }
    if(level<paraLevel || NSBIDI_MAX_EXPLICIT_LEVEL<level) {
      
      *aDirection = NSBIDI_LTR;
      return NS_ERROR_INVALID_ARG;
    }
  }
  if(flags&MASK_EMBEDDING) {
    flags|=DIRPROP_FLAG_LR(mParaLevel);
  }

  
  mFlags=flags;
  *aDirection = DirectionFromFlags(flags);
  return NS_OK;
}


nsBidiDirection nsBidi::DirectionFromFlags(Flags aFlags)
{
  
  if(!(aFlags&MASK_RTL || (aFlags&DIRPROP_FLAG(AN) && aFlags&MASK_POSSIBLE_N))) {
    return NSBIDI_LTR;
  } else if(!(aFlags&MASK_LTR)) {
    return NSBIDI_RTL;
  } else {
    return NSBIDI_MIXED;
  }
}


















#define IMPTABPROPS_COLUMNS 16
#define IMPTABPROPS_RES (IMPTABPROPS_COLUMNS - 1)
#define GET_STATEPROPS(cell) ((cell)&0x1f)
#define GET_ACTIONPROPS(cell) ((cell)>>5)
#undef s
#define s(action, newState) ((uint8_t)(newState+(action<<5)))

static const uint8_t groupProp[] =          
{

    0,  1,  2,  7,  8,  3,  9,  6,  5,  4,  4,  10, 10, 12, 10, 10, 10, 11, 10, 4,  4,  4,  4,  13, 14
};




































static const uint8_t impTabProps[][IMPTABPROPS_COLUMNS] =
{

 {     1 ,     2 ,     4 ,     5 ,     7 ,    15 ,    17 ,     7 ,     9 ,     7 ,     0 ,     7 ,     3 ,    18 ,    21 , DirProp_ON },
 {     1 , s(1,2), s(1,4), s(1,5), s(1,7),s(1,15),s(1,17), s(1,7), s(1,9), s(1,7),     1 ,     1 , s(1,3),s(1,18),s(1,21),  DirProp_L },
 { s(1,1),     2 , s(1,4), s(1,5), s(1,7),s(1,15),s(1,17), s(1,7), s(1,9), s(1,7),     2 ,     2 , s(1,3),s(1,18),s(1,21),  DirProp_R },
 { s(1,1), s(1,2), s(1,6), s(1,6), s(1,8),s(1,16),s(1,17), s(1,8), s(1,8), s(1,8),     3 ,     3 ,     3 ,s(1,18),s(1,21),  DirProp_R },
 { s(1,1), s(1,2),     4 , s(1,5), s(1,7),s(1,15),s(1,17),s(2,10),    11 ,s(2,10),     4 ,     4 , s(1,3),    18 ,    21 , DirProp_EN },
 { s(1,1), s(1,2), s(1,4),     5 , s(1,7),s(1,15),s(1,17), s(1,7), s(1,9),s(2,12),     5 ,     5 , s(1,3),s(1,18),s(1,21), DirProp_AN },
 { s(1,1), s(1,2),     6 ,     6 , s(1,8),s(1,16),s(1,17), s(1,8), s(1,8),s(2,13),     6 ,     6 , s(1,3),    18 ,    21 , DirProp_AN },
 { s(1,1), s(1,2), s(1,4), s(1,5),     7 ,s(1,15),s(1,17),     7 ,s(2,14),     7 ,     7 ,     7 , s(1,3),s(1,18),s(1,21), DirProp_ON },
 { s(1,1), s(1,2), s(1,6), s(1,6),     8 ,s(1,16),s(1,17),     8 ,     8 ,     8 ,     8 ,     8 , s(1,3),s(1,18),s(1,21), DirProp_ON },
 { s(1,1), s(1,2),     4 , s(1,5),     7 ,s(1,15),s(1,17),     7 ,     9 ,     7 ,     9 ,     9 , s(1,3),    18 ,    21 , DirProp_ON },
 { s(3,1), s(3,2),     4 , s(3,5), s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    10 , s(4,7), s(3,3),    18 ,    21 , DirProp_EN },
 { s(1,1), s(1,2),     4 , s(1,5), s(1,7),s(1,15),s(1,17), s(1,7),    11 , s(1,7),    11 ,    11 , s(1,3),    18 ,    21 , DirProp_EN },
 { s(3,1), s(3,2), s(3,4),     5 , s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    12 , s(4,7), s(3,3),s(3,18),s(3,21), DirProp_AN },
 { s(3,1), s(3,2),     6 ,     6 , s(4,8),s(3,16),s(3,17), s(4,8), s(4,8), s(4,8),    13 , s(4,8), s(3,3),    18 ,    21 , DirProp_AN },
 { s(1,1), s(1,2), s(4,4), s(1,5),     7 ,s(1,15),s(1,17),     7 ,    14 ,     7 ,    14 ,    14 , s(1,3),s(4,18),s(4,21), DirProp_ON },
 { s(1,1), s(1,2), s(1,4), s(1,5), s(1,7),    15 ,s(1,17), s(1,7), s(1,9), s(1,7),    15 , s(1,7), s(1,3),s(1,18),s(1,21),  DirProp_S },
 { s(1,1), s(1,2), s(1,6), s(1,6), s(1,8),    16 ,s(1,17), s(1,8), s(1,8), s(1,8),    16 , s(1,8), s(1,3),s(1,18),s(1,21),  DirProp_S },
 { s(1,1), s(1,2), s(1,4), s(1,5), s(1,7),s(1,15),    17 , s(1,7), s(1,9), s(1,7),    17 , s(1,7), s(1,3),s(1,18),s(1,21),  DirProp_B },
 { s(1,1), s(1,2),    18 , s(1,5), s(1,7),s(1,15),s(1,17),s(2,19),    20 ,s(2,19),    18 ,    18 , s(1,3),    18 ,    21 ,  DirProp_L },
 { s(3,1), s(3,2),    18 , s(3,5), s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    19 , s(4,7), s(3,3),    18 ,    21 ,  DirProp_L },
 { s(1,1), s(1,2),    18 , s(1,5), s(1,7),s(1,15),s(1,17), s(1,7),    20 , s(1,7),    20 ,    20 , s(1,3),    18 ,    21 ,  DirProp_L },
 { s(1,1), s(1,2),    21 , s(1,5), s(1,7),s(1,15),s(1,17),s(2,22),    23 ,s(2,22),    21 ,    21 , s(1,3),    18 ,    21 , DirProp_AN },
 { s(3,1), s(3,2),    21 , s(3,5), s(4,7),s(3,15),s(3,17), s(4,7),s(4,14), s(4,7),    22 , s(4,7), s(3,3),    18 ,    21 , DirProp_AN },
 { s(1,1), s(1,2),    21 , s(1,5), s(1,7),s(1,15),s(1,17), s(1,7),    23 , s(1,7),    23 ,    23 , s(1,3),    18 ,    21 , DirProp_AN }
};




#undef s




















#define IMPTABLEVELS_RES (IMPTABLEVELS_COLUMNS - 1)
#define GET_STATE(cell) ((cell)&0x0f)
#define GET_ACTION(cell) ((cell)>>4)
#define s(action, newState) ((uint8_t)(newState+(action<<4)))






































static const ImpTab impTabL =   



{

 {     0 ,     1 ,     0 ,     2 ,     0 ,     0 ,     0 ,  0 },
 {     0 ,     1 ,     3 ,     3 , s(1,4), s(1,4),     0 ,  1 },
 {     0 ,     1 ,     0 ,     2 , s(1,5), s(1,5),     0 ,  2 },
 {     0 ,     1 ,     3 ,     3 , s(1,4), s(1,4),     0 ,  2 },
 { s(2,0),     1 ,     3 ,     3 ,     4 ,     4 , s(2,0),  1 },
 { s(2,0),     1 , s(2,0),     2 ,     5 ,     5 , s(2,0),  1 }
};
static const ImpTab impTabR =   



{

 {     1 ,     0 ,     2 ,     2 ,     0 ,     0 ,     0 ,  0 },
 {     1 ,     0 ,     1 ,     3 , s(1,4), s(1,4),     0 ,  1 },
 {     1 ,     0 ,     2 ,     2 ,     0 ,     0 ,     0 ,  1 },
 {     1 ,     0 ,     1 ,     3 ,     5 ,     5 ,     0 ,  1 },
 { s(2,1),     0 , s(2,1),     3 ,     4 ,     4 ,     0 ,  0 },
 {     1 ,     0 ,     1 ,     3 ,     5 ,     5 ,     0 ,  0 }
};

#undef s

static ImpAct impAct0 = {0,1,2,3,4,5,6};
static PImpTab impTab[2] = {impTabL, impTabR};


















void nsBidi::ProcessPropertySeq(LevState *pLevState, uint8_t _prop, int32_t start, int32_t limit)
{
  uint8_t cell, oldStateSeq, actionSeq;
  PImpTab pImpTab = pLevState->pImpTab;
  PImpAct pImpAct = pLevState->pImpAct;
  nsBidiLevel* levels = mLevels;
  nsBidiLevel level, addLevel;
  int32_t start0, k;

  start0 = start;                         
  oldStateSeq = (uint8_t)pLevState->state;
  cell = pImpTab[oldStateSeq][_prop];
  pLevState->state = GET_STATE(cell);       
  actionSeq = pImpAct[GET_ACTION(cell)]; 
  addLevel = pImpTab[pLevState->state][IMPTABLEVELS_RES];

  if(actionSeq) {
    switch(actionSeq) {
    case 1:                         
      pLevState->startON = start0;
      break;

    case 2:                         
      start = pLevState->startON;
      break;

    default:                        
      MOZ_ASSERT(false);
      break;
    }
  }
  if(addLevel || (start < start0)) {
    level = pLevState->runLevel + addLevel;
    if (start >= pLevState->runStart) {
      for (k = start; k < limit; k++) {
        levels[k] = level;
      }
    } else {
      DirProp *dirProps = mDirProps, dirProp;
      int32_t isolateCount = 0;
      for (k = start; k < limit; k++) {
        dirProp = dirProps[k];
        if (dirProp == PDI) {
          isolateCount--;
        }
        if (isolateCount == 0) {
          levels[k]=level;
        }
        if (dirProp == LRI || dirProp == RLI) {
          isolateCount++;
        }
      }
    }
  }
}

void nsBidi::ResolveImplicitLevels(int32_t aStart, int32_t aLimit,
                   DirProp aSOR, DirProp aEOR)
{
  const DirProp *dirProps = mDirProps;
  DirProp dirProp;
  LevState levState;
  int32_t i, start1, start2;
  uint16_t oldStateImp, stateImp, actionImp;
  uint8_t gprop, resProp, cell;

  
  levState.startON = -1;
  levState.runStart = aStart;
  levState.runLevel = mLevels[aStart];
  levState.pImpTab = impTab[levState.runLevel & 1];
  levState.pImpAct = impAct0;

  


  if (dirProps[aStart] == PDI) {
    start1 = mIsolates[mIsolateCount].start1;
    stateImp = mIsolates[mIsolateCount].stateImp;
    levState.state = mIsolates[mIsolateCount].state;
    mIsolateCount--;
  } else {
    start1 = aStart;
    if (dirProps[aStart] == NSM) {
      stateImp = 1 + aSOR;
    } else {
      stateImp = 0;
    }
    levState.state = 0;
    ProcessPropertySeq(&levState, aSOR, aStart, aStart);
  }
  start2 = aStart;

  for (i = aStart; i <= aLimit; i++) {
    if (i >= aLimit) {
      if (aLimit > aStart) {
        dirProp = mDirProps[aLimit - 1];
        if (dirProp == LRI || dirProp == RLI) {
          break;  
        }
      }
      gprop = aEOR;
    } else {
      DirProp prop;
      prop = PURE_DIRPROP(dirProps[i]);
      gprop = groupProp[prop];
    }
    oldStateImp = stateImp;
    cell = impTabProps[oldStateImp][gprop];
    stateImp = GET_STATEPROPS(cell);      
    actionImp = GET_ACTIONPROPS(cell);    
    if ((i == aLimit) && (actionImp == 0)) {
      
      actionImp = 1;                      
    }
    if (actionImp) {
      resProp = impTabProps[oldStateImp][IMPTABPROPS_RES];
      switch (actionImp) {
      case 1:             
        ProcessPropertySeq(&levState, resProp, start1, i);
        start1 = i;
        break;
      case 2:             
        start2 = i;
        break;
      case 3:             
        ProcessPropertySeq(&levState, resProp, start1, start2);
        ProcessPropertySeq(&levState, DirProp_ON, start2, i);
        start1 = i;
        break;
      case 4:             
        ProcessPropertySeq(&levState, resProp, start1, start2);
        start1 = start2;
        start2 = i;
        break;
      default:            
        MOZ_ASSERT(false);
        break;
      }
    }
  }

  dirProp = dirProps[aLimit - 1];
  if ((dirProp == LRI || dirProp == RLI) && aLimit < mLength) {
    mIsolateCount++;
    mIsolates[mIsolateCount].stateImp = stateImp;
    mIsolates[mIsolateCount].state = levState.state;
    mIsolates[mIsolateCount].start1 = start1;
  } else {
    ProcessPropertySeq(&levState, aEOR, aLimit, aLimit);
  }
}










void nsBidi::AdjustWSLevels()
{
  const DirProp *dirProps=mDirProps;
  nsBidiLevel *levels=mLevels;
  int32_t i;

  if(mFlags&MASK_WS) {
    nsBidiLevel paraLevel=mParaLevel;
    Flags flag;

    i=mTrailingWSStart;
    while(i>0) {
      
      while (i > 0 && DIRPROP_FLAG(PURE_DIRPROP(dirProps[--i])) & MASK_WS) {
        levels[i]=paraLevel;
      }

      
      
      while(i>0) {
        flag = DIRPROP_FLAG(PURE_DIRPROP(dirProps[--i]));
        if(flag&MASK_BN_EXPLICIT) {
          levels[i]=levels[i+1];
        } else if(flag&MASK_B_S) {
          levels[i]=paraLevel;
          break;
        }
      }
    }
  }
}

nsresult nsBidi::GetDirection(nsBidiDirection* aDirection)
{
  *aDirection = mDirection;
  return NS_OK;
}

nsresult nsBidi::GetParaLevel(nsBidiLevel* aParaLevel)
{
  *aParaLevel = mParaLevel;
  return NS_OK;
}
#ifdef FULL_BIDI_ENGINE



nsresult nsBidi::GetLength(int32_t* aLength)
{
  *aLength = mLength;
  return NS_OK;
}


















































nsresult nsBidi::SetLine(const nsBidi* aParaBidi, int32_t aStart, int32_t aLimit)
{
  nsBidi* pParent = (nsBidi*)aParaBidi;
  int32_t length;

  
  if(pParent==nullptr) {
    return NS_ERROR_INVALID_POINTER;
  } else if(aStart < 0 || aStart >= aLimit || aLimit > pParent->mLength) {
    return NS_ERROR_INVALID_ARG;
  }

  
  length = mLength = aLimit - aStart;
  mParaLevel=pParent->mParaLevel;

  mRuns=nullptr;
  mFlags=0;

  mDirProps=pParent->mDirProps+aStart;
  mLevels=pParent->mLevels+aStart;
  mRunCount=-1;

  if(pParent->mDirection!=NSBIDI_MIXED) {
    
    mDirection=pParent->mDirection;

    




    if(pParent->mTrailingWSStart<=aStart) {
      mTrailingWSStart=0;
    } else if(pParent->mTrailingWSStart<aLimit) {
      mTrailingWSStart=pParent->mTrailingWSStart-aStart;
    } else {
      mTrailingWSStart=length;
    }
  } else {
    const nsBidiLevel *levels=mLevels;
    int32_t i, trailingWSStart;
    nsBidiLevel level;

    SetTrailingWSStart();
    trailingWSStart=mTrailingWSStart;

    
    if(trailingWSStart==0) {
      
      mDirection=(nsBidiDirection)(mParaLevel&1);
   } else {
      
      level=levels[0]&1;

      
      if(trailingWSStart<length && (mParaLevel&1)!=level) {
        
        mDirection=NSBIDI_MIXED;
      } else {
        
        i=1;
        for(;;) {
          if(i==trailingWSStart) {
            
            mDirection=(nsBidiDirection)level;
            break;
          } else if((levels[i]&1)!=level) {
            mDirection=NSBIDI_MIXED;
            break;
          }
          ++i;
        }
      }
    }

    switch(mDirection) {
      case NSBIDI_LTR:
        
        mParaLevel=(mParaLevel+1)&~1;

        
        mTrailingWSStart=0;
      break;
      case NSBIDI_RTL:
        
        mParaLevel|=1;

        
        mTrailingWSStart=0;
        break;
      default:
        break;
    }
  }
  return NS_OK;
}












void nsBidi::SetTrailingWSStart() {
  

  const DirProp *dirProps=mDirProps;
  nsBidiLevel *levels=mLevels;
  int32_t start=mLength;
  nsBidiLevel paraLevel=mParaLevel;

  
  while(start>0 && DIRPROP_FLAG(dirProps[start-1])&MASK_WS) {
    --start;
  }

  
  while(start>0 && levels[start-1]==paraLevel) {
    --start;
  }

  mTrailingWSStart=start;
}

nsresult nsBidi::GetLevelAt(int32_t aCharIndex, nsBidiLevel* aLevel)
{
  
  if(aCharIndex<0 || mLength<=aCharIndex) {
    *aLevel = 0;
  } else if(mDirection!=NSBIDI_MIXED || aCharIndex>=mTrailingWSStart) {
    *aLevel = mParaLevel;
  } else {
    *aLevel = mLevels[aCharIndex];
  }
  return NS_OK;
}

nsresult nsBidi::GetLevels(nsBidiLevel** aLevels)
{
  int32_t start, length;

  length = mLength;
  if(length<=0) {
    *aLevels = nullptr;
    return NS_ERROR_INVALID_ARG;
  }

  start = mTrailingWSStart;
  if(start==length) {
    
    *aLevels = mLevels;
    return NS_OK;
  }

  







  if(GETLEVELSMEMORY(length)) {
    nsBidiLevel *levels=mLevelsMemory;

    if(start>0 && levels!=mLevels) {
      memcpy(levels, mLevels, start);
    }
    memset(levels+start, mParaLevel, length-start);

    
    mTrailingWSStart=length;
    *aLevels=mLevels=levels;
    return NS_OK;
  } else {
    
    *aLevels = nullptr;
    return NS_ERROR_OUT_OF_MEMORY;
  }
}
#endif 

nsresult nsBidi::GetCharTypeAt(int32_t aCharIndex, nsCharType* pType)
{
  if(aCharIndex<0 || mLength<=aCharIndex) {
    return NS_ERROR_INVALID_ARG;
  }
  *pType = (nsCharType)mDirProps[aCharIndex];
  return NS_OK;
}

nsresult nsBidi::GetLogicalRun(int32_t aLogicalStart, int32_t *aLogicalLimit, nsBidiLevel *aLevel)
{
  int32_t length = mLength;

  if(aLogicalStart<0 || length<=aLogicalStart) {
    return NS_ERROR_INVALID_ARG;
  }

  int32_t runCount, visualStart, logicalLimit, logicalFirst, i;
  Run iRun;

  
  nsresult rv = CountRuns(&runCount);
  if (NS_FAILED(rv)) {
    return rv;
  }

  visualStart = logicalLimit = 0;
  iRun = mRuns[0];

  for (i = 0; i < runCount; i++) {
    iRun = mRuns[i];
    logicalFirst = GET_INDEX(iRun.logicalStart);
    logicalLimit = logicalFirst + iRun.visualLimit - visualStart;
    if ((aLogicalStart >= logicalFirst) && (aLogicalStart < logicalLimit)) {
       break;
    }
    visualStart = iRun.visualLimit;
  }
  if (aLogicalLimit) {
    *aLogicalLimit = logicalLimit;
  }
  if (aLevel) {
    if (mDirection != NSBIDI_MIXED || aLogicalStart >= mTrailingWSStart) {
      *aLevel = mParaLevel;
    } else {
      *aLevel = mLevels[aLogicalStart];
    }
  }
  return NS_OK;
}



nsresult nsBidi::CountRuns(int32_t* aRunCount)
{
  if(mRunCount<0 && !GetRuns()) {
    return NS_ERROR_OUT_OF_MEMORY;
  } else {
    if (aRunCount)
      *aRunCount = mRunCount;
    return NS_OK;
  }
}

nsresult nsBidi::GetVisualRun(int32_t aRunIndex, int32_t *aLogicalStart, int32_t *aLength, nsBidiDirection *aDirection)
{
  if( aRunIndex<0 ||
      (mRunCount==-1 && !GetRuns()) ||
      aRunIndex>=mRunCount
    ) {
    *aDirection = NSBIDI_LTR;
    return NS_OK;
  } else {
    int32_t start=mRuns[aRunIndex].logicalStart;
    if(aLogicalStart!=nullptr) {
      *aLogicalStart=GET_INDEX(start);
    }
    if(aLength!=nullptr) {
      if(aRunIndex>0) {
        *aLength=mRuns[aRunIndex].visualLimit-
             mRuns[aRunIndex-1].visualLimit;
      } else {
        *aLength=mRuns[0].visualLimit;
      }
    }
    *aDirection = (nsBidiDirection)GET_ODD_BIT(start);
    return NS_OK;
  }
}










bool nsBidi::GetRuns()
{
  



  if (mRunCount >= 0) {
    return true;
  }

  if(mDirection!=NSBIDI_MIXED) {
    
    GetSingleRun(mParaLevel);
  } else  {
    
    int32_t length=mLength, limit=mTrailingWSStart;

    










    nsBidiLevel *levels=mLevels;
    int32_t i, runCount;
    nsBidiLevel level=NSBIDI_DEFAULT_LTR;   

    
    runCount=0;
    for(i=0; i<limit; ++i) {
      
      if(levels[i]!=level) {
        ++runCount;
        level=levels[i];
      }
    }

    



    if(runCount==1 && limit==length) {
      
      GetSingleRun(levels[0]);
    } else  {
      
      Run *runs;
      int32_t runIndex, start;
      nsBidiLevel minLevel=NSBIDI_MAX_EXPLICIT_LEVEL+1, maxLevel=0;

      
      if(limit<length) {
        ++runCount;
      }

      
      if(GETRUNSMEMORY(runCount)) {
        runs=mRunsMemory;
      } else {
        return false;
      }

      
      
      
      runIndex=0;

      
      i = 0;
      do {
        
        start = i;
        level = levels[i];
        if(level<minLevel) {
          minLevel=level;
        }
        if(level>maxLevel) {
          maxLevel=level;
        }

        
        while (++i < limit && levels[i] == level) {
        }

        
        runs[runIndex].logicalStart = start;
        runs[runIndex].visualLimit = i - start;
        ++runIndex;
      } while (i < limit);

      if(limit<length) {
        
        runs[runIndex].logicalStart=limit;
        runs[runIndex].visualLimit=length-limit;
        if(mParaLevel<minLevel) {
          minLevel=mParaLevel;
        }
      }

      
      mRuns=runs;
      mRunCount=runCount;

      ReorderLine(minLevel, maxLevel);

      
      
      limit = 0;
      for (i = 0; i < runCount; ++i) {
        ADD_ODD_BIT_FROM_LEVEL(runs[i].logicalStart, levels[runs[i].logicalStart]);
        limit += runs[i].visualLimit;
        runs[i].visualLimit = limit;
      }

      
      
      if (runIndex < runCount) {
        int32_t trailingRun = (mParaLevel & 1) ? 0 : runIndex;
        ADD_ODD_BIT_FROM_LEVEL(runs[trailingRun].logicalStart, mParaLevel);
      }
    }
  }

  return true;
}


void nsBidi::GetSingleRun(nsBidiLevel aLevel)
{
  
  mRuns=mSimpleRuns;
  mRunCount=1;

  
  mRuns[0].logicalStart=MAKE_INDEX_ODD_PAIR(0, aLevel);
  mRuns[0].visualLimit=mLength;
}


































void nsBidi::ReorderLine(nsBidiLevel aMinLevel, nsBidiLevel aMaxLevel)
{
  Run *runs, tempRun;
  nsBidiLevel *levels;
  int32_t firstRun, endRun, limitRun, runCount;

  
  if(aMaxLevel<=(aMinLevel|1)) {
    return;
  }

  




  ++aMinLevel;

  runs=mRuns;
  levels=mLevels;
  runCount=mRunCount;

  
  if(mTrailingWSStart<mLength) {
    --runCount;
  }

  while(--aMaxLevel>=aMinLevel) {
    firstRun=0;

    
    for(;;) {
      
      
      while(firstRun<runCount && levels[runs[firstRun].logicalStart]<aMaxLevel) {
        ++firstRun;
      }
      if(firstRun>=runCount) {
        break;  
      }

      
      for(limitRun=firstRun; ++limitRun<runCount && levels[runs[limitRun].logicalStart]>=aMaxLevel;) {}

      
      endRun=limitRun-1;
      while(firstRun<endRun) {
        tempRun = runs[firstRun];
        runs[firstRun] = runs[endRun];
        runs[endRun] = tempRun;
        ++firstRun;
        --endRun;
      }

      if(limitRun==runCount) {
        break;  
      } else {
        firstRun=limitRun+1;
      }
    }
  }

  
  if(!(aMinLevel&1)) {
    firstRun=0;

    
    if(mTrailingWSStart==mLength) {
      --runCount;
    }

    
    while(firstRun<runCount) {
      tempRun = runs[firstRun];
      runs[firstRun] = runs[runCount];
      runs[runCount] = tempRun;
      ++firstRun;
      --runCount;
    }
  }
}

nsresult nsBidi::ReorderVisual(const nsBidiLevel *aLevels, int32_t aLength, int32_t *aIndexMap)
{
  int32_t start, end, limit, temp;
  nsBidiLevel minLevel, maxLevel;

  if(aIndexMap==nullptr ||
     !PrepareReorder(aLevels, aLength, aIndexMap, &minLevel, &maxLevel)) {
    return NS_OK;
  }

  
  if(minLevel==maxLevel && (minLevel&1)==0) {
    return NS_OK;
  }

  
  minLevel|=1;

  
  do {
    start=0;

    
    for(;;) {
      
      
      while(start<aLength && aLevels[start]<maxLevel) {
        ++start;
      }
      if(start>=aLength) {
        break;  
      }

      
      for(limit=start; ++limit<aLength && aLevels[limit]>=maxLevel;) {}

      





      end=limit-1;
      while(start<end) {
        temp=aIndexMap[start];
        aIndexMap[start]=aIndexMap[end];
        aIndexMap[end]=temp;

        ++start;
        --end;
      }

      if(limit==aLength) {
        break;  
      } else {
        start=limit+1;
      }
    }
  } while(--maxLevel>=minLevel);

  return NS_OK;
}

bool nsBidi::PrepareReorder(const nsBidiLevel *aLevels, int32_t aLength,
                int32_t *aIndexMap,
                nsBidiLevel *aMinLevel, nsBidiLevel *aMaxLevel)
{
  int32_t start;
  nsBidiLevel level, minLevel, maxLevel;

  if(aLevels==nullptr || aLength<=0) {
    return false;
  }

  
  minLevel=NSBIDI_MAX_EXPLICIT_LEVEL+1;
  maxLevel=0;
  for(start=aLength; start>0;) {
    level=aLevels[--start];
    if(level>NSBIDI_MAX_EXPLICIT_LEVEL+1) {
      return false;
    }
    if(level<minLevel) {
      minLevel=level;
    }
    if(level>maxLevel) {
      maxLevel=level;
    }
  }
  *aMinLevel=minLevel;
  *aMaxLevel=maxLevel;

  
  for(start=aLength; start>0;) {
    --start;
    aIndexMap[start]=start;
  }

  return true;
}

#ifdef FULL_BIDI_ENGINE


nsresult nsBidi::GetVisualIndex(int32_t aLogicalIndex, int32_t* aVisualIndex) {
  int32_t visualIndex = NSBIDI_MAP_NOWHERE;

  if(aLogicalIndex<0 || mLength<=aLogicalIndex) {
    return NS_ERROR_INVALID_ARG;
  } else {
    
    switch(mDirection) {
    case NSBIDI_LTR:
      *aVisualIndex = aLogicalIndex;
      return NS_OK;
    case NSBIDI_RTL:
      *aVisualIndex = mLength-aLogicalIndex-1;
      return NS_OK;
    default:
      if(mRunCount<0 && !GetRuns()) {
        return NS_ERROR_OUT_OF_MEMORY;
      } else {
        Run *runs=mRuns;
        int32_t i, visualStart=0, offset, length;

        
        for (i = 0; i < mRunCount; ++i) {
          length=runs[i].visualLimit-visualStart;
          offset=aLogicalIndex-GET_INDEX(runs[i].logicalStart);
          if(offset>=0 && offset<length) {
            if(IS_EVEN_RUN(runs[i].logicalStart)) {
              
              visualIndex = visualStart + offset;
            } else {
              
              visualIndex = visualStart + length - offset - 1;
            }
            break;
          }
          visualStart+=length;
        }
        if (i >= mRunCount) {
          *aVisualIndex = NSBIDI_MAP_NOWHERE;
          return NS_OK;
        }
      }
    }
  }

  *aVisualIndex = visualIndex;
  return NS_OK;
}

nsresult nsBidi::GetLogicalIndex(int32_t aVisualIndex, int32_t *aLogicalIndex)
{
  if(aVisualIndex<0 || mLength<=aVisualIndex) {
    return NS_ERROR_INVALID_ARG;
  }

  
  if (mDirection == NSBIDI_LTR) {
    *aLogicalIndex = aVisualIndex;
    return NS_OK;
  } else if (mDirection == NSBIDI_RTL) {
    *aLogicalIndex = mLength - aVisualIndex - 1;
    return NS_OK;
  }

  if(mRunCount<0 && !GetRuns()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  Run *runs=mRuns;
  int32_t i, runCount=mRunCount, start;

  if(runCount<=10) {
    
    for(i=0; aVisualIndex>=runs[i].visualLimit; ++i) {}
  } else {
    
    int32_t start=0, limit=runCount;

    
    for(;;) {
      i=(start+limit)/2;
      if(aVisualIndex>=runs[i].visualLimit) {
        start=i+1;
      } else if(i==0 || aVisualIndex>=runs[i-1].visualLimit) {
        break;
      } else {
        limit=i;
      }
    }
  }

  start=runs[i].logicalStart;
  if(IS_EVEN_RUN(start)) {
    
    
    if(i>0) {
      aVisualIndex-=runs[i-1].visualLimit;
    }
    *aLogicalIndex = GET_INDEX(start)+aVisualIndex;
    return NS_OK;
  } else {
    
    *aLogicalIndex = GET_INDEX(start)+runs[i].visualLimit-aVisualIndex-1;
    return NS_OK;
  }
}

nsresult nsBidi::GetLogicalMap(int32_t *aIndexMap)
{
  nsresult rv;

  
  rv = CountRuns(nullptr);
  if(NS_FAILED(rv)) {
    return rv;
  } else if(aIndexMap==nullptr) {
    return NS_ERROR_INVALID_ARG;
  } else {
    
    int32_t visualStart, visualLimit, j;
    int32_t logicalStart;
    Run *runs = mRuns;
    if (mLength <= 0) {
      return NS_OK;
    }

    visualStart = 0;
    for (j = 0; j < mRunCount; ++j) {
      logicalStart = GET_INDEX(runs[j].logicalStart);
      visualLimit = runs[j].visualLimit;
      if (IS_EVEN_RUN(runs[j].logicalStart)) {
        do { 
          aIndexMap[logicalStart++] = visualStart++;
        } while (visualStart < visualLimit);
      } else {
        logicalStart += visualLimit-visualStart;  
        do { 
          aIndexMap[--logicalStart] = visualStart++;
        } while (visualStart < visualLimit);
      }
      
    }
  }
  return NS_OK;
}

nsresult nsBidi::GetVisualMap(int32_t *aIndexMap)
{
  int32_t* runCount=nullptr;
  nsresult rv;

  if(aIndexMap==nullptr) {
    return NS_ERROR_INVALID_ARG;
  }

  
  rv = CountRuns(runCount);
  if(NS_FAILED(rv)) {
    return rv;
  } else {
    
    Run *runs=mRuns, *runsLimit=runs+mRunCount;
    int32_t logicalStart, visualStart, visualLimit;

    visualStart=0;
    for(; runs<runsLimit; ++runs) {
      logicalStart=runs->logicalStart;
      visualLimit=runs->visualLimit;
      if(IS_EVEN_RUN(logicalStart)) {
        do { 
          *aIndexMap++ = logicalStart++;
        } while(++visualStart<visualLimit);
      } else {
        REMOVE_ODD_BIT(logicalStart);
        logicalStart+=visualLimit-visualStart;  
        do { 
          *aIndexMap++ = --logicalStart;
        } while(++visualStart<visualLimit);
      }
      
    }
    return NS_OK;
  }
}



nsresult nsBidi::ReorderLogical(const nsBidiLevel *aLevels, int32_t aLength, int32_t *aIndexMap)
{
  int32_t start, limit, sumOfSosEos;
  nsBidiLevel minLevel, maxLevel;

  if(aIndexMap==nullptr ||
     !PrepareReorder(aLevels, aLength, aIndexMap, &minLevel, &maxLevel)) {
    return NS_OK;
  }

  
  if(minLevel==maxLevel && (minLevel&1)==0) {
    return NS_OK;
  }

  
  minLevel|=1;

  
  do {
    start=0;

    
    for(;;) {
      
      
      while(start<aLength && aLevels[start]<maxLevel) {
        ++start;
      }
      if(start>=aLength) {
        break;  
      }

      
      for(limit=start; ++limit<aLength && aLevels[limit]>=maxLevel;) {}

      










      sumOfSosEos=start+limit-1;

      
      do {
        aIndexMap[start]=sumOfSosEos-aIndexMap[start];
      } while(++start<limit);

      
      if(limit==aLength) {
        break;  
      } else {
        start=limit+1;
      }
    }
  } while(--maxLevel>=minLevel);

  return NS_OK;
}

nsresult nsBidi::InvertMap(const int32_t *aSrcMap, int32_t *aDestMap, int32_t aLength)
{
  if(aSrcMap!=nullptr && aDestMap!=nullptr && aLength > 0) {
    const int32_t *pi;
    int32_t destLength = -1, count = 0;
    
    pi = aSrcMap + aLength;
    while (pi > aSrcMap) {
      if (*--pi > destLength) {
        destLength = *pi;
      }
      if (*pi >= 0) {
        count++;
      }
    }
    destLength++;  
    if (count < destLength) {
      
      memset(aDestMap, 0xFF, destLength * sizeof(int32_t));
    }
    pi = aSrcMap + aLength;
    while (aLength > 0) {
      if (*--pi >= 0) {
        aDestMap[*pi] = --aLength;
      } else {
        --aLength;
      }
    }
  }
  return NS_OK;
}

int32_t nsBidi::doWriteReverse(const char16_t *src, int32_t srcLength,
                               char16_t *dest, uint16_t options) {
  

















  int32_t i, j, destSize;
  uint32_t c;

  
  switch(options&(NSBIDI_REMOVE_BIDI_CONTROLS|NSBIDI_DO_MIRRORING|NSBIDI_KEEP_BASE_COMBINING)) {
    case 0:
        





      destSize=srcLength;

    
      do {
      
        i=srcLength;

      
        UTF_BACK_1(src, 0, srcLength);

      
        j=srcLength;
        do {
          *dest++=src[j++];
        } while(j<i);
      } while(srcLength>0);
      break;
    case NSBIDI_KEEP_BASE_COMBINING:
    





      destSize=srcLength;

    
      do {
      
        i=srcLength;

      
        do {
          UTF_PREV_CHAR(src, 0, srcLength, c);
        } while(srcLength>0 && GetBidiCat(c) == eCharType_DirNonSpacingMark);

      
        j=srcLength;
        do {
          *dest++=src[j++];
        } while(j<i);
      } while(srcLength>0);
      break;
    default:
    






      if(!(options&NSBIDI_REMOVE_BIDI_CONTROLS)) {
        i=srcLength;
      } else {
      

        int32_t length=srcLength;
        char16_t ch;

        i=0;
        do {
          ch=*src++;
          if (!IsBidiControl((uint32_t)ch)) {
            ++i;
          }
        } while(--length>0);
        src-=srcLength;
      }
      destSize=i;

    
      do {
      
        i=srcLength;

      
        UTF_PREV_CHAR(src, 0, srcLength, c);
        if(options&NSBIDI_KEEP_BASE_COMBINING) {
        
          while(srcLength>0 && GetBidiCat(c) == eCharType_DirNonSpacingMark) {
            UTF_PREV_CHAR(src, 0, srcLength, c);
          }
        }

        if(options&NSBIDI_REMOVE_BIDI_CONTROLS && IsBidiControl(c)) {
        
          continue;
        }

      
        j=srcLength;
        if(options&NSBIDI_DO_MIRRORING) {
          
          c = GetMirroredChar(c);

          int32_t k=0;
          UTF_APPEND_CHAR_UNSAFE(dest, k, c);
          dest+=k;
          j+=k;
        }
        while(j<i) {
          *dest++=src[j++];
        }
      } while(srcLength>0);
      break;
  } 
  return destSize;
}

nsresult nsBidi::WriteReverse(const char16_t *aSrc, int32_t aSrcLength, char16_t *aDest, uint16_t aOptions, int32_t *aDestSize)
{
  if( aSrc==nullptr || aSrcLength<0 ||
      aDest==nullptr
    ) {
    return NS_ERROR_INVALID_ARG;
  }

  
  if( aSrc>=aDest && aSrc<aDest+aSrcLength ||
      aDest>=aSrc && aDest<aSrc+aSrcLength
    ) {
    return NS_ERROR_INVALID_ARG;
  }

  if(aSrcLength>0) {
    *aDestSize = doWriteReverse(aSrc, aSrcLength, aDest, aOptions);
  }
  return NS_OK;
}
#endif 
