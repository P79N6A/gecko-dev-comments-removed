



































#ifndef nsPSMDetectors_h__
#define nsPSMDetectors_h__

#include "nsCOMPtr.h"
#include "nsIFactory.h"
#include "nsVerifier.h"

#include "nsSJISVerifier.h"
#include "nsEUCJPVerifier.h"
#include "nsCP1252Verifier.h"
#include "nsUTF8Verifier.h"
#include "nsISO2022JPVerifier.h"
#include "nsISO2022KRVerifier.h"
#include "nsISO2022CNVerifier.h"
#include "nsHZVerifier.h"
#include "nsUCS2BEVerifier.h"
#include "nsUCS2LEVerifier.h"
#include "nsBIG5Verifier.h"
#include "nsGB2312Verifier.h"
#include "nsGB18030Verifier.h"
#include "nsEUCTWVerifier.h"
#include "nsEUCKRVerifier.h"




#define MAX_VERIFIERS 16


#define NS_JA_PSMDETECTOR_CID \
{ 0x12bb8f1b, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_JA_STRING_PSMDETECTOR_CID \
{ 0x12bb8f1c, 0x2389, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_KO_PSMDETECTOR_CID \
{ 0xea06d4e1, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_ZHCN_PSMDETECTOR_CID \
{ 0xea06d4e2, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_ZHTW_PSMDETECTOR_CID \
{ 0xea06d4e3, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }



#define NS_KO_STRING_PSMDETECTOR_CID \
{ 0xea06d4e4, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_ZHCN_STRING_PSMDETECTOR_CID \
{ 0xea06d4e5, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_ZHTW_STRING_PSMDETECTOR_CID \
{ 0xea06d4e6, 0x2b3d, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }



#define NS_ZH_STRING_PSMDETECTOR_CID \
{ 0xfcacef21, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_CJK_STRING_PSMDETECTOR_CID \
{ 0xfcacef22, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }



#define NS_ZH_PSMDETECTOR_CID \
{ 0xfcacef23, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }


#define NS_CJK_PSMDETECTOR_CID \
{ 0xfcacef24, 0x2b40, 0x11d3, { 0xb3, 0xbf, 0x0, 0x80, 0x5f, 0x8a, 0x66, 0x70 } }

typedef struct {
  float mFirstByteFreq[94];
  float mFirstByteStdDev;
  float mFirstByteMean;
  float mFirstByteWeight;
  float mSecoundByteFreq[94];
  float mSecoundByteStdDev;
  float mSecoundByteMean;
  float mSecoundByteWeight;
} nsEUCStatisticsMutable;

typedef const nsEUCStatisticsMutable nsEUCStatistics;
































#define ZHTW_DETECTOR_NUM_VERIFIERS 7
extern nsVerifier* const gZhTwVerifierSet[];
extern nsEUCStatistics* const gZhTwStatisticsSet[];

#define KO_DETECTOR_NUM_VERIFIERS 6
extern nsVerifier* const gKoVerifierSet[];

#define ZHCN_DETECTOR_NUM_VERIFIERS 8
extern nsVerifier* const gZhCnVerifierSet[];

#define JA_DETECTOR_NUM_VERIFIERS 7
extern nsVerifier* const gJaVerifierSet[];

#define ZH_DETECTOR_NUM_VERIFIERS 10
extern nsVerifier* const gZhVerifierSet[];
extern nsEUCStatistics* const gZhStatisticsSet[];

#define CJK_DETECTOR_NUM_VERIFIERS 15
extern nsVerifier* const gCJKVerifierSet[];
extern nsEUCStatistics* const gCJKStatisticsSet[];

class nsEUCSampler {
  public:
    nsEUCSampler() {
      mTotal =0;
      mThreshold = 200;
	  mState = 0;
      PRInt32 i;
      for(i=0;i<94;i++)
          mFirstByteCnt[i] = mSecondByteCnt[i]=0;
    }
    PRBool EnoughData()  { return mTotal > mThreshold; }
    PRBool GetSomeData() { return mTotal > 1; }
    PRBool Sample(const char* aIn, PRUint32 aLen);
    void   CalFreq();
    float   GetScore(const float* aFirstByteFreq, float aFirstByteWeight,
                     const float* aSecondByteFreq, float aSecondByteWeight);
    float   GetScore(const float* array1, const float* array2);
  private:
    PRUint32 mTotal;
    PRUint32 mThreshold;
    PRInt8 mState;
    PRUint32 mFirstByteCnt[94];
    PRUint32 mSecondByteCnt[94];
    float mFirstByteFreq[94];
    float mSecondByteFreq[94];
   
};














class nsPSMDetector {
public :
   nsPSMDetector(PRUint8 aItems, nsVerifier* const * aVerifierSet, nsEUCStatistics* const * aStatisticsSet);
   virtual ~nsPSMDetector() {}

   virtual PRBool HandleData(const char* aBuf, PRUint32 aLen);
   virtual void   DataEnd();
 
protected:
   virtual void Report(const char* charset) = 0;

   PRUint8 mItems;
   PRUint8 mClassItems;
   PRUint8 mState[MAX_VERIFIERS];
   PRUint8 mItemIdx[MAX_VERIFIERS];
   nsVerifier* const * mVerifier;
   nsEUCStatistics* const * mStatisticsData;
   PRBool mDone;

   PRBool mRunSampler;
   PRBool mClassRunSampler;
protected:
   void Reset();
   void Sample(const char* aBuf, PRUint32 aLen, PRBool aLastChance=PR_FALSE);
private:
#ifdef DETECTOR_DEBUG
   PRUint32 mDbgTest;
   PRUint32 mDbgLen;
#endif
   nsEUCSampler mSampler;

};

class nsXPCOMDetector : 
      private nsPSMDetector,
      public nsICharsetDetector 
{
  NS_DECL_ISUPPORTS
public:
    nsXPCOMDetector(PRUint8 aItems, nsVerifier* const * aVer, nsEUCStatistics* const * aStatisticsSet);
    virtual ~nsXPCOMDetector();
  NS_IMETHOD Init(nsICharsetDetectionObserver* aObserver);
  NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, PRBool* oDontFeedMe);
  NS_IMETHOD Done();

protected:
  virtual void Report(const char* charset);

private:
  nsCOMPtr<nsICharsetDetectionObserver> mObserver;
};

class nsXPCOMStringDetector : 
      private nsPSMDetector,
      public nsIStringCharsetDetector 
{
  NS_DECL_ISUPPORTS
public:
    nsXPCOMStringDetector(PRUint8 aItems, nsVerifier* const * aVer, nsEUCStatistics* const * aStatisticsSet);
    virtual ~nsXPCOMStringDetector();
    NS_IMETHOD DoIt(const char* aBuf, PRUint32 aLen, 
                   const char** oCharset, 
                   nsDetectionConfident &oConfident);
protected:
  virtual void Report(const char* charset);
private:
  const char* mResult;
};

class nsJAPSMDetector : public nsXPCOMDetector
{
public:
  nsJAPSMDetector() 
    : nsXPCOMDetector(JA_DETECTOR_NUM_VERIFIERS, gJaVerifierSet, nsnull) {}
};

class nsJAStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsJAStringPSMDetector() 
    : nsXPCOMStringDetector(JA_DETECTOR_NUM_VERIFIERS - 3, gJaVerifierSet, nsnull) {}
};

class nsKOPSMDetector : public nsXPCOMDetector
{
public:
  nsKOPSMDetector() 
    : nsXPCOMDetector(KO_DETECTOR_NUM_VERIFIERS, gKoVerifierSet, nsnull){}
};

class nsKOStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsKOStringPSMDetector() 
    : nsXPCOMStringDetector(KO_DETECTOR_NUM_VERIFIERS - 3, gKoVerifierSet, nsnull) {}
};

class nsZHTWPSMDetector : public nsXPCOMDetector
{
public:
  nsZHTWPSMDetector() 
    : nsXPCOMDetector(ZHTW_DETECTOR_NUM_VERIFIERS, gZhTwVerifierSet, gZhTwStatisticsSet) {}
};

class nsZHTWStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsZHTWStringPSMDetector() 
    : nsXPCOMStringDetector(ZHTW_DETECTOR_NUM_VERIFIERS - 3, gZhTwVerifierSet, gZhTwStatisticsSet) {}
};

class nsZHCNPSMDetector : public nsXPCOMDetector
{
public:
  nsZHCNPSMDetector() 
    : nsXPCOMDetector(ZHCN_DETECTOR_NUM_VERIFIERS, gZhCnVerifierSet, nsnull) {}
};

class nsZHCNStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsZHCNStringPSMDetector() 
    : nsXPCOMStringDetector(ZHCN_DETECTOR_NUM_VERIFIERS - 3, gZhCnVerifierSet, nsnull) {}
};

class nsZHPSMDetector : public nsXPCOMDetector
{
public:
  nsZHPSMDetector() 
    : nsXPCOMDetector(ZH_DETECTOR_NUM_VERIFIERS, gZhVerifierSet, gZhStatisticsSet) {}
};

class nsZHStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsZHStringPSMDetector() 
    : nsXPCOMStringDetector(ZH_DETECTOR_NUM_VERIFIERS - 3, gZhVerifierSet, gZhStatisticsSet) {}
};

class nsCJKPSMDetector : public nsXPCOMDetector
{
public:
  nsCJKPSMDetector() 
    : nsXPCOMDetector(CJK_DETECTOR_NUM_VERIFIERS, gCJKVerifierSet, gCJKStatisticsSet) {}
};

class nsCJKStringPSMDetector : public nsXPCOMStringDetector
{
public:
  nsCJKStringPSMDetector() 
    : nsXPCOMStringDetector(CJK_DETECTOR_NUM_VERIFIERS - 3, gCJKVerifierSet, gCJKStatisticsSet) {}
};

#endif 
