





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
  mRunCount = -1;

  mDirProps=nullptr;
  mLevels=nullptr;
  mRuns=nullptr;

  mDirPropsMemory=nullptr;
  mLevelsMemory=nullptr;
  mRunsMemory=nullptr;

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

  
  mLength=aLength;
  mParaLevel=aParaLevel;
  mDirection=NSBIDI_LTR;
  mTrailingWSStart=aLength;  

  mDirProps=nullptr;
  mLevels=nullptr;
  mRuns=nullptr;

  if(aLength==0) {
    




    if(IS_DEFAULT_LEVEL(aParaLevel)) {
      mParaLevel&=1;
    }
    if(aParaLevel&1) {
      mFlags=DIRPROP_FLAG(R);
      mDirection=NSBIDI_RTL;
    } else {
      mFlags=DIRPROP_FLAG(L);
      mDirection=NSBIDI_LTR;
    }

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
      direction=ResolveExplicitLevels();
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
          }
        } while(limit<aLength);
      }

      
      AdjustWSLevels();
      break;
  }

  mDirection=direction;
  return NS_OK;
}








void nsBidi::GetDirProps(const char16_t *aText)
{
  DirProp *dirProps=mDirPropsMemory;    

  int32_t i=0, length=mLength;
  Flags flags=0;      
  char16_t uchar;
  DirProp dirProp;

  if(IS_DEFAULT_LEVEL(mParaLevel)) {
    
    for(;;) {
      uchar=aText[i];
      if(!IS_FIRST_SURROGATE(uchar) || i+1==length || !IS_SECOND_SURROGATE(aText[i+1])) {
        
        flags|=DIRPROP_FLAG(dirProps[i]=dirProp=GetBidiCat((uint32_t)uchar));
      } else {
        
        dirProps[i++]=BN;   
        flags|=DIRPROP_FLAG(dirProps[i]=dirProp=GetBidiCat(GET_UTF_32(uchar, aText[i])))|DIRPROP_FLAG(BN);
      }
      ++i;
      if(dirProp==L) {
        mParaLevel=0;
        break;
      } else if(dirProp==R || dirProp==AL) {
        mParaLevel=1;
        break;
      } else if(i==length) {
        




        mParaLevel&=1;
        break;
      }
    }
  }

  
  while(i<length) {
    uchar=aText[i];
    if(!IS_FIRST_SURROGATE(uchar) || i+1==length || !IS_SECOND_SURROGATE(aText[i+1])) {
      
      flags|=DIRPROP_FLAG(dirProps[i]=GetBidiCat((uint32_t)uchar));
    } else {
      
      dirProps[i++]=BN;   
      flags|=DIRPROP_FLAG(dirProps[i]=GetBidiCat(GET_UTF_32(uchar, aText[i])))|DIRPROP_FLAG(BN);
    }
    ++i;
  }
  if(flags&MASK_EMBEDDING) {
    flags|=DIRPROP_FLAG_LR(mParaLevel);
  }
  mFlags=flags;
}
























































nsBidiDirection nsBidi::ResolveExplicitLevels()
{
  const DirProp *dirProps=mDirProps;
  nsBidiLevel *levels=mLevels;

  int32_t i=0, length=mLength;
  Flags flags=mFlags;       
  DirProp dirProp;
  nsBidiLevel level=mParaLevel;

  nsBidiDirection direction;

  
  direction=DirectionFromFlags(flags);

  
  if(direction!=NSBIDI_MIXED) {
    
  } else if(!(flags&MASK_EXPLICIT)) {
    
    
    for(i=0; i<length; ++i) {
      levels[i]=level;
    }
  } else {
    

    
    
    nsBidiLevel embeddingLevel=level, newLevel, stackTop=0;

    nsBidiLevel stack[NSBIDI_MAX_EXPLICIT_LEVEL];        
    uint32_t countOver60=0, countOver61=0;  

    
    flags=0;

    
    for(i=0; i<length; ++i) {
      dirProp=dirProps[i];
      switch(dirProp) {
        case LRE:
        case LRO:
          
          newLevel=(embeddingLevel+2)&~(NSBIDI_LEVEL_OVERRIDE|1);    
          if(newLevel<=NSBIDI_MAX_EXPLICIT_LEVEL) {
            stack[stackTop]=embeddingLevel;
            ++stackTop;
            embeddingLevel=newLevel;
            if(dirProp==LRO) {
              embeddingLevel|=NSBIDI_LEVEL_OVERRIDE;
            } else {
              embeddingLevel&=~NSBIDI_LEVEL_OVERRIDE;
            }
          } else if((embeddingLevel&~NSBIDI_LEVEL_OVERRIDE)==NSBIDI_MAX_EXPLICIT_LEVEL) {
            ++countOver61;
          } else  {
            ++countOver60;
          }
          flags|=DIRPROP_FLAG(BN);
          break;
        case RLE:
        case RLO:
          
          newLevel=((embeddingLevel&~NSBIDI_LEVEL_OVERRIDE)+1)|1;    
          if(newLevel<=NSBIDI_MAX_EXPLICIT_LEVEL) {
            stack[stackTop]=embeddingLevel;
            ++stackTop;
            embeddingLevel=newLevel;
            if(dirProp==RLO) {
              embeddingLevel|=NSBIDI_LEVEL_OVERRIDE;
            } else {
              embeddingLevel&=~NSBIDI_LEVEL_OVERRIDE;
            }
          } else {
            ++countOver61;
          }
          flags|=DIRPROP_FLAG(BN);
          break;
        case PDF:
          
          
          if(countOver61>0) {
            --countOver61;
          } else if(countOver60>0 && (embeddingLevel&~NSBIDI_LEVEL_OVERRIDE)!=NSBIDI_MAX_EXPLICIT_LEVEL) {
            
            --countOver60;
          } else if(stackTop>0) {
            
            --stackTop;
            embeddingLevel=stack[stackTop];
            
          }
          flags|=DIRPROP_FLAG(BN);
          break;
        case B:
          




          stackTop=0;
          countOver60=countOver61=0;
          embeddingLevel=level=mParaLevel;
          flags|=DIRPROP_FLAG(B);
          break;
        case BN:
          
          
          flags|=DIRPROP_FLAG(BN);
          break;
        default:
          
          if(level!=embeddingLevel) {
            level=embeddingLevel;
            if(level&NSBIDI_LEVEL_OVERRIDE) {
              flags|=DIRPROP_FLAG_O(level)|DIRPROP_FLAG_MULTI_RUNS;
            } else {
              flags|=DIRPROP_FLAG_E(level)|DIRPROP_FLAG_MULTI_RUNS;
            }
          }
          if(!(level&NSBIDI_LEVEL_OVERRIDE)) {
            flags|=DIRPROP_FLAG(dirProp);
          }
          break;
      }

      



      levels[i]=level;
    }
    if(flags&MASK_EMBEDDING) {
      flags|=DIRPROP_FLAG_LR(mParaLevel);
    }

    

    
    mFlags=flags;
    direction=DirectionFromFlags(flags);
  }
  return direction;
}











nsresult nsBidi::CheckExplicitLevels(nsBidiDirection *aDirection)
{
  const DirProp *dirProps=mDirProps;
  nsBidiLevel *levels=mLevels;

  int32_t i, length=mLength;
  Flags flags=0;  
  nsBidiLevel level, paraLevel=mParaLevel;

  for(i=0; i<length; ++i) {
    level=levels[i];
    if(level&NSBIDI_LEVEL_OVERRIDE) {
      
      level&=~NSBIDI_LEVEL_OVERRIDE;     
      flags|=DIRPROP_FLAG_O(level);
    } else {
      
      flags|=DIRPROP_FLAG_E(level)|DIRPROP_FLAG(dirProps[i]);
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


























#define EN_SHIFT 2
#define EN_AFTER_W2 1
#define EN_AFTER_W4 2
#define EN_ALL 3
#define PREV_EN_AFTER_W2 4
#define PREV_EN_AFTER_W4 8

void nsBidi::ResolveImplicitLevels(int32_t aStart, int32_t aLimit,
                   DirProp aSOR, DirProp aEOR)
{
  const DirProp *dirProps=mDirProps;
  nsBidiLevel *levels=mLevels;

  int32_t i, next, neutralStart=-1;
  DirProp prevDirProp, dirProp, nextDirProp, lastStrong, beforeNeutral;
  uint8_t historyOfEN;

  
  next=aStart;
  beforeNeutral=dirProp=lastStrong=aSOR;
  nextDirProp=dirProps[next];
  historyOfEN=0;

  






  while(DIRPROP_FLAG(nextDirProp)&MASK_BN_EXPLICIT) {
    if(++next<aLimit) {
      nextDirProp=dirProps[next];
    } else {
      nextDirProp=aEOR;
      break;
    }
  }

  
  while(next<aLimit) {
    
    prevDirProp=dirProp;
    dirProp=nextDirProp;
    i=next;
    do {
      if(++next<aLimit) {
        nextDirProp=dirProps[next];
      } else {
        nextDirProp=aEOR;
        break;
      }
    } while(DIRPROP_FLAG(nextDirProp)&MASK_BN_EXPLICIT);
    historyOfEN<<=EN_SHIFT;

    
    switch(dirProp) {
      case L:
        lastStrong=L;
        break;
      case R:
        lastStrong=R;
        break;
      case AL:
        
        lastStrong=AL;
        dirProp=R;
        break;
      case EN:
        
        if(lastStrong==AL) {
          
          dirProp=AN;
        } else {
          if(lastStrong==L) {
            
            dirProp=L;
          }
          
          historyOfEN|=EN_ALL;
        }
        break;
      case ES:
        if( historyOfEN&PREV_EN_AFTER_W2 &&     
            nextDirProp==EN && lastStrong!=AL   
          ) {
          
          if(lastStrong!=L) {
            dirProp=EN;
          } else {
            
            dirProp=L;
          }
          historyOfEN|=EN_AFTER_W4;
        } else {
          
          dirProp=O_N;
        }
        break;
      case CS:
        if( historyOfEN&PREV_EN_AFTER_W2 &&     
            nextDirProp==EN && lastStrong!=AL   
          ) {
          
          if(lastStrong!=L) {
            dirProp=EN;
          } else {
            
            dirProp=L;
          }
          historyOfEN|=EN_AFTER_W4;
        } else if(prevDirProp==AN &&                    
              (nextDirProp==AN ||                   
               (nextDirProp==EN && lastStrong==AL))   
             ) {
          
          dirProp=AN;
        } else {
          
          dirProp=O_N;
        }
        break;
      case ET:
        
        while(next<aLimit && DIRPROP_FLAG(nextDirProp)&MASK_ET_NSM_BN ) {
          if(++next<aLimit) {
            nextDirProp=dirProps[next];
          } else {
            nextDirProp=aEOR;
            break;
          }
        }

        if( historyOfEN&PREV_EN_AFTER_W4 ||     
            (nextDirProp==EN && lastStrong!=AL)   
          ) {
          
          if(lastStrong!=L) {
            dirProp=EN;
          } else {
            
            dirProp=L;
          }
        } else {
          
          dirProp=O_N;
        }

        
        break;
      case NSM:
        
        dirProp=prevDirProp;
        
        historyOfEN>>=EN_SHIFT;
        









        break;
      default:
        break;
    }

    

    
    
    if(DIRPROP_FLAG(dirProp)&MASK_N) {
      if(neutralStart<0) {
        
        neutralStart=i;
        beforeNeutral=prevDirProp;
      }
    } else  {
      





      nsBidiLevel level=levels[i];

      if(neutralStart>=0) {
        nsBidiLevel final;
        
        if(beforeNeutral==L) {
          if(dirProp==L) {
            final=0;                
          } else {
            final=level;            
          }
        } else  {
          if(dirProp==L) {
            final=level;            
          } else {
            final=1;                
          }
        }
        
        if((level^final)&1) {
          
          do {
            ++levels[neutralStart];
          } while(++neutralStart<i);
        }
        neutralStart=-1;
      }

      
      





      if(dirProp==L) {
        if(level&1) {
          ++level;
        } else {
          i=next;     
        }
      } else if(dirProp==R) {
        if(!(level&1)) {
          ++level;
        } else {
          i=next;     
        }
      } else  {
        level=(level+2)&~1;     
      }

      
      while(i<next) {
        levels[i++]=level;
      }
    }
  }

  

  
  if(neutralStart>=0) {
    





    nsBidiLevel level=levels[neutralStart], final;

    
    if(beforeNeutral==L) {
      if(aEOR==L) {
        final=0;                
      } else {
        final=level;            
      }
    } else  {
      if(aEOR==L) {
        final=level;            
      } else {
        final=1;                
      }
    }
    
    if((level^final)&1) {
      
      do {
        ++levels[neutralStart];
      } while(++neutralStart<aLimit);
    }
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
      
      while(i>0 && DIRPROP_FLAG(dirProps[--i])&MASK_WS) {
        levels[i]=paraLevel;
      }

      
      
      while(i>0) {
        flag=DIRPROP_FLAG(dirProps[--i]);
        if(flag&MASK_BN_EXPLICIT) {
          levels[i]=levels[i+1];
        } else if(flag&MASK_B_S) {
          levels[i]=paraLevel;
          break;
        }
      }
    }
  }

  
  
  if(mFlags&MASK_OVERRIDE) {
    for(i=mTrailingWSStart; i>0;) {
      levels[--i]&=~NSBIDI_LEVEL_OVERRIDE;
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


















































nsresult nsBidi::SetLine(nsIBidi* aParaBidi, int32_t aStart, int32_t aLimit)
{
  nsBidi* pParent = (nsBidi*)aParaBidi;
  int32_t length;

  
  if(pParent==nullptr) {
    return NS_ERROR_INVALID_POINTER;
  } else if(aStart<0 || aStart>aLimit || aLimit>pParent->mLength) {
    return NS_ERROR_INVALID_ARG;
  }

  
  length=mLength=aLimit-aStart;
  mParaLevel=pParent->mParaLevel;

  mRuns=nullptr;
  mFlags=0;

  if(length>0) {
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
      Flags flags=0;

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
  } else {
    
    mDirection=mParaLevel&1 ? NSBIDI_RTL : NSBIDI_LTR;
    mTrailingWSStart=mRunCount=0;

    mDirProps=nullptr;
    mLevels=nullptr;
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

  if(mDirection!=NSBIDI_MIXED || aLogicalStart>=mTrailingWSStart) {
    if(aLogicalLimit!=nullptr) {
      *aLogicalLimit=length;
    }
    if(aLevel!=nullptr) {
      *aLevel=mParaLevel;
    }
  } else {
    nsBidiLevel *levels=mLevels;
    nsBidiLevel level=levels[aLogicalStart];

    
    length=mTrailingWSStart;
    while(++aLogicalStart<length && level==levels[aLogicalStart]) {}

    if(aLogicalLimit!=nullptr) {
      *aLogicalLimit=aLogicalStart;
    }
    if(aLevel!=nullptr) {
      *aLevel=level;
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
  if(mDirection!=NSBIDI_MIXED) {
    
    GetSingleRun(mParaLevel);
  } else  {
    
    int32_t length=mLength, limit=mTrailingWSStart;

    










    if(limit==0) {
      
      GetSingleRun(mParaLevel);
    } else {
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

        
        start=0;
        level=levels[0];
        if(level<minLevel) {
          minLevel=level;
        }
        if(level>maxLevel) {
          maxLevel=level;
        }

        
        for(i=1; i<limit; ++i) {
          if(levels[i]!=level) {
            
            runs[runIndex].logicalStart=start;
            runs[runIndex].visualLimit=i-start;
            start=i;

            level=levels[i];
            if(level<minLevel) {
              minLevel=level;
            }
            if(level>maxLevel) {
              maxLevel=level;
            }
            ++runIndex;
          }
        }

        
        runs[runIndex].logicalStart=start;
        runs[runIndex].visualLimit=limit-start;
        ++runIndex;

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

        
        ADD_ODD_BIT_FROM_LEVEL(runs[0].logicalStart, levels[runs[0].logicalStart]);
        limit=runs[0].visualLimit;
        for(i=1; i<runIndex; ++i) {
          ADD_ODD_BIT_FROM_LEVEL(runs[i].logicalStart, levels[runs[i].logicalStart]);
          limit=runs[i].visualLimit+=limit;
        }

        
        if(runIndex<runCount) {
          ADD_ODD_BIT_FROM_LEVEL(runs[i].logicalStart, mParaLevel);
          runs[runIndex].visualLimit+=limit;
        }
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
  Run *runs;
  nsBidiLevel *levels;
  int32_t firstRun, endRun, limitRun, runCount, temp;

  
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
        temp=runs[firstRun].logicalStart;
        runs[firstRun].logicalStart=runs[endRun].logicalStart;
        runs[endRun].logicalStart=temp;

        temp=runs[firstRun].visualLimit;
        runs[firstRun].visualLimit=runs[endRun].visualLimit;
        runs[endRun].visualLimit=temp;

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
      temp=runs[firstRun].logicalStart;
      runs[firstRun].logicalStart=runs[runCount].logicalStart;
      runs[runCount].logicalStart=temp;

      temp=runs[firstRun].visualLimit;
      runs[firstRun].visualLimit=runs[runCount].visualLimit;
      runs[runCount].visualLimit=temp;

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

          
          for(i=0;; ++i) {
            length=runs[i].visualLimit-visualStart;
            offset=aLogicalIndex-GET_INDEX(runs[i].logicalStart);
            if(offset>=0 && offset<length) {
              if(IS_EVEN_RUN(runs[i].logicalStart)) {
                
                *aVisualIndex = visualStart+offset;
                return NS_OK;
              } else {
                
                *aVisualIndex = visualStart+length-offset-1;
                return NS_OK;
              }
            }
            visualStart+=length;
          }
        }
    }
  }
}

nsresult nsBidi::GetLogicalIndex(int32_t aVisualIndex, int32_t *aLogicalIndex)
{
  if(aVisualIndex<0 || mLength<=aVisualIndex) {
    return NS_ERROR_INVALID_ARG;
  } else {
    
    switch(mDirection) {
      case NSBIDI_LTR:
        *aLogicalIndex = aVisualIndex;
        return NS_OK;
      case NSBIDI_RTL:
        *aLogicalIndex = mLength-aVisualIndex-1;
        return NS_OK;
      default:
        if(mRunCount<0 && !GetRuns()) {
          return NS_ERROR_OUT_OF_MEMORY;
        } else {
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
    }
  }
}

nsresult nsBidi::GetLogicalMap(int32_t *aIndexMap)
{
  nsBidiLevel *levels;
  nsresult rv;

  
  rv = GetLevels(&levels);
  if(NS_FAILED(rv)) {
    return rv;
  } else if(aIndexMap==nullptr) {
    return NS_ERROR_INVALID_ARG;
  } else {
    return ReorderLogical(levels, mLength, aIndexMap);
  }
}

nsresult nsBidi::GetVisualMap(int32_t *aIndexMap)
{
  int32_t* runCount=nullptr;
  nsresult rv;

  
  rv = CountRuns(runCount);
  if(NS_FAILED(rv)) {
    return rv;
  } else if(aIndexMap==nullptr) {
    return NS_ERROR_INVALID_ARG;
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
  if(aSrcMap!=nullptr && aDestMap!=nullptr) {
    aSrcMap+=aLength;
    while(aLength>0) {
      aDestMap[*--aSrcMap]=--aLength;
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
        } while(srcLength>0 && IsBidiCategory(c, eBidiCat_NSM));

      
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
        
          while(srcLength>0 && IsBidiCategory(c, eBidiCat_NSM)) {
            UTF_PREV_CHAR(src, 0, srcLength, c);
          }
        }

        if(options&NSBIDI_REMOVE_BIDI_CONTROLS && IsBidiControl(c)) {
        
          continue;
        }

      
        j=srcLength;
        if(options&NSBIDI_DO_MIRRORING) {
          
          c = SymmSwap(c);

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
