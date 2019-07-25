




#ifndef CharDistribution_h__
#define CharDistribution_h__

#include "nscore.h"

#define ENOUGH_DATA_THRESHOLD 1024
 
#define MINIMUM_DATA_THRESHOLD  4

class CharDistributionAnalysis
{
public:
  CharDistributionAnalysis() {Reset(false);}

  
  void HandleData(const char* aBuf, uint32_t aLen) {}
  
  
  void HandleOneChar(const char* aStr, uint32_t aCharLen)
  {
    int32_t order;

    
    order = (aCharLen == 2) ? GetOrder(aStr) : -1;

    if (order >= 0)
    {
      mTotalChars++;
      
      if ((uint32_t)order < mTableSize)
      {
        if (512 > mCharToFreqOrder[order])
          mFreqChars++;
      }
    }
  }

  
  float GetConfidence(void);

  
  void      Reset(bool aIsPreferredLanguage) 
  {
    mDone = false;
    mTotalChars = 0;
    mFreqChars = 0;
    mDataThreshold = aIsPreferredLanguage ? 0 : MINIMUM_DATA_THRESHOLD;
  }

  
  
  bool GotEnoughData() {return mTotalChars > ENOUGH_DATA_THRESHOLD;}

protected:
  
  
  
  virtual int32_t GetOrder(const char* str) {return -1;}
  
  
  bool     mDone;

  
  uint32_t mFreqChars;

  
  uint32_t mTotalChars;

  
  uint32_t mDataThreshold;

  
  const int16_t  *mCharToFreqOrder;

  
  uint32_t mTableSize;

  
  
  float    mTypicalDistributionRatio;
};


class EUCTWDistributionAnalysis: public CharDistributionAnalysis
{
public:
  EUCTWDistributionAnalysis();
protected:

  
  
  
  
  int32_t GetOrder(const char* str) 
  { if ((unsigned char)*str >= (unsigned char)0xc4)  
      return 94*((unsigned char)str[0]-(unsigned char)0xc4) + (unsigned char)str[1] - (unsigned char)0xa1;
    else
      return -1;
  }
};


class EUCKRDistributionAnalysis : public CharDistributionAnalysis
{
public:
  EUCKRDistributionAnalysis();
protected:
  
  
  
  
  int32_t GetOrder(const char* str) 
  { if ((unsigned char)*str >= (unsigned char)0xb0)  
      return 94*((unsigned char)str[0]-(unsigned char)0xb0) + (unsigned char)str[1] - (unsigned char)0xa1;
    else
      return -1;
  }
};

class GB2312DistributionAnalysis : public CharDistributionAnalysis
{
public:
  GB2312DistributionAnalysis();
protected:
  
  
  
  
  int32_t GetOrder(const char* str) 
  { if ((unsigned char)*str >= (unsigned char)0xb0 && (unsigned char)str[1] >= (unsigned char)0xa1)  
      return 94*((unsigned char)str[0]-(unsigned char)0xb0) + (unsigned char)str[1] - (unsigned char)0xa1;
    else
      return -1;
  }
};


class Big5DistributionAnalysis : public CharDistributionAnalysis
{
public:
  Big5DistributionAnalysis();
protected:
  
  
  
  
  int32_t GetOrder(const char* str) 
  { if ((unsigned char)*str >= (unsigned char)0xa4)  
      if ((unsigned char)str[1] >= (unsigned char)0xa1)
        return 157*((unsigned char)str[0]-(unsigned char)0xa4) + (unsigned char)str[1] - (unsigned char)0xa1 +63;
      else
        return 157*((unsigned char)str[0]-(unsigned char)0xa4) + (unsigned char)str[1] - (unsigned char)0x40;
    else
      return -1;
  }
};

class SJISDistributionAnalysis : public CharDistributionAnalysis
{
public:
  SJISDistributionAnalysis();
protected:
  
  
  
  
  int32_t GetOrder(const char* str) 
  { 
    int32_t order;
    if ((unsigned char)*str >= (unsigned char)0x81 && (unsigned char)*str <= (unsigned char)0x9f)  
      order = 188 * ((unsigned char)str[0]-(unsigned char)0x81);
    else if ((unsigned char)*str >= (unsigned char)0xe0 && (unsigned char)*str <= (unsigned char)0xef)  
      order = 188 * ((unsigned char)str[0]-(unsigned char)0xe0 + 31);
    else
      return -1;
    order += (unsigned char)*(str+1) - 0x40;
    if ((unsigned char)str[1] > (unsigned char)0x7f)
      order--;
    return order;
  }
};

class EUCJPDistributionAnalysis : public CharDistributionAnalysis
{
public:
  EUCJPDistributionAnalysis();
protected:
  
  
  
  
  int32_t GetOrder(const char* str) 
  { if ((unsigned char)*str >= (unsigned char)0xa0)  
      return 94*((unsigned char)str[0]-(unsigned char)0xa1) + (unsigned char)str[1] - (unsigned char)0xa1;
    else
      return -1;
  }
};

#endif 

