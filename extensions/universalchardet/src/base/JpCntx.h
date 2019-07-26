




#ifndef __JPCNTX_H__
#define __JPCNTX_H__

#define NUM_OF_CATEGORY 6

#include "nscore.h" 

#define ENOUGH_REL_THRESHOLD  100
#define MAX_REL_THRESHOLD     1000


extern const uint8_t jp2CharContext[83][83];

class JapaneseContextAnalysis
{
public:
  JapaneseContextAnalysis() {Reset(false);}

  void HandleData(const char* aBuf, uint32_t aLen);

  void HandleOneChar(const char* aStr, uint32_t aCharLen)
  {
    int32_t order;

    
    if (mTotalRel > MAX_REL_THRESHOLD)   mDone = true;
    if (mDone)       return;
     
    
    order = (aCharLen == 2) ? GetOrder(aStr) : -1;
    if (order != -1 && mLastCharOrder != -1)
    {
      mTotalRel++;
      
      mRelSample[jp2CharContext[mLastCharOrder][order]]++;
    }
    mLastCharOrder = order;
  }

  float GetConfidence(void);
  void      Reset(bool aIsPreferredLanguage);
  bool GotEnoughData() {return mTotalRel > ENOUGH_REL_THRESHOLD;}

protected:
  virtual int32_t GetOrder(const char* str, uint32_t *charLen) = 0;
  virtual int32_t GetOrder(const char* str) = 0;

  
  uint32_t mRelSample[NUM_OF_CATEGORY];

  
  uint32_t mTotalRel;

  
  uint32_t mDataThreshold;
  
  
  int32_t  mLastCharOrder;

  
  
  uint32_t mNeedToSkipCharNum;

  
  bool     mDone;
};


class SJISContextAnalysis : public JapaneseContextAnalysis
{
  
protected:
  int32_t GetOrder(const char* str, uint32_t *charLen);

  int32_t GetOrder(const char* str)
  {
    
    if (*str == '\202' && 
          (unsigned char)*(str+1) >= (unsigned char)0x9f && 
          (unsigned char)*(str+1) <= (unsigned char)0xf1)
      return (unsigned char)*(str+1) - (unsigned char)0x9f;
    return -1;
  }
};

class EUCJPContextAnalysis : public JapaneseContextAnalysis
{
protected:
  int32_t GetOrder(const char* str, uint32_t *charLen);
  int32_t GetOrder(const char* str)
    
  {
    if (*str == '\244' &&
          (unsigned char)*(str+1) >= (unsigned char)0xa1 &&
          (unsigned char)*(str+1) <= (unsigned char)0xf3)
      return (unsigned char)*(str+1) - (unsigned char)0xa1;
    return -1;
  }
};

#endif 

