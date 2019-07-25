




































#include "nsHebrewProber.h"
#include <stdio.h>


#define FINAL_KAF ('\xea')
#define NORMAL_KAF ('\xeb')
#define FINAL_MEM ('\xed')
#define NORMAL_MEM ('\xee')
#define FINAL_NUN ('\xef')
#define NORMAL_NUN ('\xf0')
#define FINAL_PE ('\xf3')
#define NORMAL_PE ('\xf4')
#define FINAL_TSADI ('\xf5')
#define NORMAL_TSADI ('\xf6')



#define MIN_FINAL_CHAR_DISTANCE (5)



#define MIN_MODEL_DISTANCE (0.01)

#define VISUAL_HEBREW_NAME ("ISO-8859-8")
#define LOGICAL_HEBREW_NAME ("windows-1255")

PRBool nsHebrewProber::isFinal(char c)
{
  return ((c == FINAL_KAF) || (c == FINAL_MEM) || (c == FINAL_NUN) || (c == FINAL_PE) || (c == FINAL_TSADI));
}

PRBool nsHebrewProber::isNonFinal(char c)
{
  return ((c == NORMAL_KAF) || (c == NORMAL_MEM) || (c == NORMAL_NUN) || (c == NORMAL_PE));
  
  
  
  
  
  
  
  
  
  
}


























nsProbingState nsHebrewProber::HandleData(const char* aBuf, PRUint32 aLen)
{
  
  if (GetState() == eNotMe)
    return eNotMe;

  const char *curPtr, *endPtr = aBuf+aLen;
  char cur;

  for (curPtr = (char*)aBuf; curPtr < endPtr; ++curPtr)
  {
    cur = *curPtr;
    if (cur == ' ') 
    {
      if (mBeforePrev != ' ') 
      {
        if (isFinal(mPrev)) 
          ++mFinalCharLogicalScore;
        else if (isNonFinal(mPrev)) 
          ++mFinalCharVisualScore;
      }
    }
    else  
    {
      if ((mBeforePrev == ' ') && (isFinal(mPrev)) && (cur != ' ')) 
        ++mFinalCharVisualScore;
    }
    mBeforePrev = mPrev;
    mPrev = cur;
  }

  
  return eDetecting;
}


const char* nsHebrewProber::GetCharSetName()
{
  
  PRInt32 finalsub = mFinalCharLogicalScore - mFinalCharVisualScore;
  if (finalsub >= MIN_FINAL_CHAR_DISTANCE) 
    return LOGICAL_HEBREW_NAME;
  if (finalsub <= -(MIN_FINAL_CHAR_DISTANCE))
    return VISUAL_HEBREW_NAME;

  
  float modelsub = mLogicalProb->GetConfidence() - mVisualProb->GetConfidence();
  if (modelsub > MIN_MODEL_DISTANCE)
    return LOGICAL_HEBREW_NAME;
  if (modelsub < -(MIN_MODEL_DISTANCE))
    return VISUAL_HEBREW_NAME;

  
  if (finalsub < 0) 
    return VISUAL_HEBREW_NAME;

  
  return LOGICAL_HEBREW_NAME;
}


void nsHebrewProber::Reset(void)
{
  mFinalCharLogicalScore = 0;
  mFinalCharVisualScore = 0;

  
  
  mPrev = ' ';
  mBeforePrev = ' ';
}

nsProbingState nsHebrewProber::GetState(void) 
{
  
  if ((mLogicalProb->GetState() == eNotMe) && (mVisualProb->GetState() == eNotMe))
    return eNotMe;
  return eDetecting;
}

#ifdef DEBUG_chardet
void  nsHebrewProber::DumpStatus()
{
  printf("  HEB: %d - %d [Logical-Visual score]\r\n", mFinalCharLogicalScore, mFinalCharVisualScore);
}
#endif
