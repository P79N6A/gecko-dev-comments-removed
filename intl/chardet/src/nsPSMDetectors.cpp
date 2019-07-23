






































#include <math.h>
#include <stdio.h>

#include "nsIFactory.h"
#include "nsIGenericFactory.h"
#include "nsISupports.h"
#include "nsCharDetDll.h"
#include "pratom.h"
#include "nsPSMDetectors.h"

nsEUCStatistics gBig5Statistics = 
#include "Big5Statistics.h"


nsEUCStatistics gEUCTWStatistics = 
#include "EUCTWStatistics.h"


nsEUCStatistics gGB2312Statistics = 
#include "GB2312Statistics.h"


nsEUCStatistics gEUCJPStatistics = 
#include "EUCJPStatistics.h"


nsEUCStatistics gEUCKRStatistics = 
#include "EUCKRStatistics.h"












nsVerifier* const gZhTwVerifierSet[ZHTW_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsBIG5Verifier,
      &nsISO2022CNVerifier,
      &nsEUCTWVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};

nsEUCStatistics* const gZhTwStatisticsSet[ZHTW_DETECTOR_NUM_VERIFIERS] = {
      nsnull,
      &gBig5Statistics,
      nsnull,
      &gEUCTWStatistics,
      nsnull,
      nsnull,
      nsnull
};



nsVerifier* const gKoVerifierSet[KO_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsEUCKRVerifier,
      &nsISO2022KRVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};



nsVerifier* const gZhCnVerifierSet[ZHCN_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsGB2312Verifier,
      &nsGB18030Verifier,
      &nsISO2022CNVerifier,
      &nsHZVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};



nsVerifier* const gJaVerifierSet[JA_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsSJISVerifier,
      &nsEUCJPVerifier,
      &nsISO2022JPVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};



nsVerifier* const gZhVerifierSet[ZH_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsGB2312Verifier,
      &nsGB18030Verifier,
      &nsBIG5Verifier,
      &nsISO2022CNVerifier,
      &nsHZVerifier,
      &nsEUCTWVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};

nsEUCStatistics* const gZhStatisticsSet[ZH_DETECTOR_NUM_VERIFIERS] = {
      nsnull,
      &gGB2312Statistics,
      &gBig5Statistics,
      nsnull,
      nsnull,
      &gEUCTWStatistics,
      nsnull,
      nsnull,
      nsnull
};



nsVerifier* const gCJKVerifierSet[CJK_DETECTOR_NUM_VERIFIERS] = {
      &nsUTF8Verifier,
      &nsSJISVerifier,
      &nsEUCJPVerifier,
      &nsISO2022JPVerifier,
      &nsEUCKRVerifier,
      &nsISO2022KRVerifier,
      &nsBIG5Verifier,
      &nsEUCTWVerifier,
      &nsGB2312Verifier,
      &nsGB18030Verifier,
      &nsISO2022CNVerifier,
      &nsHZVerifier,
      &nsCP1252Verifier,
      &nsUCS2BEVerifier,
      &nsUCS2LEVerifier
};

nsEUCStatistics* const gCJKStatisticsSet[CJK_DETECTOR_NUM_VERIFIERS] = {
      nsnull,
      nsnull,
      &gEUCJPStatistics,
      nsnull,
      &gEUCKRStatistics,
      nsnull,
      &gBig5Statistics,
      &gEUCTWStatistics,
      &gGB2312Statistics,
      nsnull,
      nsnull,
      nsnull,
      nsnull,
      nsnull
};

PRBool nsEUCSampler::Sample(const char* aIn, PRUint32 aLen)
{
    if(mState == 1)
        return PR_FALSE;
    const unsigned char* p = (const unsigned char*) aIn;
    if(aLen + mTotal > 0x80000000) 
       aLen = 0x80000000 - mTotal;

     PRUint32 i;
     for(i=0; (i<aLen) && (1 != mState) ;i++,p++)
     {
        switch(mState) {
           case 0:
             if( *p & 0x0080)  
             {
                if((0x00ff == *p) || ( 0x00a1 > *p)) {
                   mState = 1;
                } else {
                   mTotal++;
                   mFirstByteCnt[*p - 0x00a1]++;
                   mState = 2;
                }
             }
             break;
           case 1:
             break;
           case 2:
             if( *p & 0x0080)  
             {
                if((0x00ff == *p) || ( 0x00a1 > *p)) {
                   mState = 1;
                } else {
                   mTotal++;
                   mSecondByteCnt[*p - 0x00a1]++;
                   mState = 0;
                }
             } else {
                mState = 1;
             }
             break;
           default:
             mState = 1;
        }
     }
   return (1 != mState  );
}
float nsEUCSampler::GetScore(const float* aFirstByteFreq, float aFirstByteWeight,
                     const float* aSecondByteFreq, float aSecondByteWeight)
{
   return aFirstByteWeight * GetScore(aFirstByteFreq, mFirstByteFreq) +
          aSecondByteWeight * GetScore(aSecondByteFreq, mSecondByteFreq);
}

float nsEUCSampler::GetScore(const float* array1, const float* array2)
{
   float s;
   float sum=0.0;
   PRUint16 i;
   for(i=0;i<94;i++) {
     s = array1[i] - array2[i];
     sum += s * s;
   }
   return (float)sqrt((double)sum) / 94.0f;
}

void nsEUCSampler::CalFreq()
{
   PRUint32 i;
   for(i = 0 ; i < 94; i++) {
      mFirstByteFreq[i] = (float)mFirstByteCnt[i] / (float)mTotal;
      mSecondByteFreq[i] = (float)mSecondByteCnt[i] / (float)mTotal;
   }
}


NS_IMPL_ISUPPORTS1(nsXPCOMDetector, nsICharsetDetector)
NS_IMPL_ISUPPORTS1(nsXPCOMStringDetector, nsIStringCharsetDetector)
nsPSMDetector::nsPSMDetector(PRUint8 aItems, nsVerifier* const * aVerifierSet, nsEUCStatistics* const * aStatisticsSet)
{
  mClassRunSampler = (nsnull != aStatisticsSet);
  mStatisticsData = aStatisticsSet;
  mVerifier = aVerifierSet;

  mClassItems = aItems;
  Reset();
}
void nsPSMDetector::Reset()
{
  mRunSampler = mClassRunSampler;
  mDone= PR_FALSE;
  mItems = mClassItems;
  NS_ASSERTION(MAX_VERIFIERS >= mItems , "MAX_VERIFIERS is too small!");
  for(PRUint8 i = 0; i < mItems ; i++)
  {
     mState[i] = 0;
     mItemIdx[i] = i;
  }
#ifdef DETECTOR_DEBUG
  mDbgLen = mDbgTest = 0;
#endif   
}

void nsPSMDetector::DataEnd()
{
  
  
  
  
  if (mItems == 2) {
    if ((&nsGB18030Verifier) == mVerifier[mItemIdx[0]]) {
      Report( mVerifier[mItemIdx[1]]->charset);
      mDone = PR_TRUE;
    } else if ((&nsGB18030Verifier) == mVerifier[mItemIdx[1]]) {
      Report( mVerifier[mItemIdx[0]]->charset);
      mDone = PR_TRUE;
    }
  }
  if(mRunSampler)
     Sample(nsnull, 0, PR_TRUE);
}





PRBool nsPSMDetector::HandleData(const char* aBuf, PRUint32 aLen)
{
  PRUint32 i,j;
  PRUint32 st;
  for(i=0; i < aLen; i++)
  {
     char b = aBuf[i];
     for(j = 0; j < mItems; )
     {
#ifdef ftang_TRACE_STATE
       if(  mVerifier[mItemIdx[j]] == & TRACE_VERIFIER )
       {
           printf("%d = %d\n", i + mDbgLen, mState[j]);
       }
#endif
#ifdef DETECTOR_DEBUG
        mDbgTest++;
#endif 
        st = GETNEXTSTATE( mVerifier[mItemIdx[j]], b, mState[j] );
        if(eItsMe == st) 
        {
#ifdef DETECTOR_DEBUG
            printf("It's %s- byte %d(%x) test %d\n", 
                    mVerifier[mItemIdx[j]]->charset,
                    i+mDbgLen,
                    i+mDbgLen,
                    mDbgTest
                  );
#endif
            Report( mVerifier[mItemIdx[j]]->charset);
            mDone = PR_TRUE;
            return mDone;
        } else if (eError == st) 
        {
#ifdef DETECTOR_DEBUG
            printf("It's NOT %s- byte %d(%x)\n", 
                    mVerifier[mItemIdx[j]]->charset,
                    i+mDbgLen,
                    i+mDbgLen);
#endif
            mItems--;
            if(j < mItems )
            {
                mItemIdx[j] = mItemIdx[mItems];
                mState[j] = mState[mItems];
            } 
        } else {
            mState[j++] = st;
        } 
     }
     if( mItems <= 1) 
     {
         if( 1 == mItems) {
#ifdef DETECTOR_DEBUG
             printf("It's %s- byte %d (%x) Test %d. The only left\n", 
                       mVerifier[mItemIdx[0]]->charset,
                       i+mDbgLen,
                       i+mDbgLen,
                       mDbgTest);
#endif
             Report( mVerifier[mItemIdx[0]]->charset);
         }
         mDone = PR_TRUE;
         return mDone;
     } else {
        
        PRInt32 nonUCS2Num=0;
        PRInt32 nonUCS2Idx=0;
        for(j = 0; j < mItems; j++) {
             if(((&nsUCS2BEVerifier) != mVerifier[mItemIdx[j]]) &&
                ((&nsUCS2LEVerifier) != mVerifier[mItemIdx[j]])) {
                  nonUCS2Num++;
                  nonUCS2Idx = j;
             }
        }
        if(1 == nonUCS2Num) {
#ifdef DETECTOR_DEBUG
             printf("It's %s- byte %d (%x) Test %d. The only left except UCS2LE/BE\n", 
                       mVerifier[mItemIdx[nonUCS2Idx]]->charset,
                       i+mDbgLen,
                       i+mDbgLen,
                       mDbgTest);
#endif
            Report( mVerifier[mItemIdx[nonUCS2Idx]]->charset);
            mDone = PR_TRUE;
            return mDone;
        }
     }
  }
  if(mRunSampler)
     Sample(aBuf, aLen);

#ifdef DETECTOR_DEBUG
  mDbgLen += aLen;
#endif
  return PR_FALSE;
}

void nsPSMDetector::Sample(const char* aBuf, PRUint32 aLen, PRBool aLastChance)
{
     PRInt32 possibleCandidateNum=0;
     PRInt32 j;
     PRInt32 eucNum=0;
     for(j = 0; j < mItems; j++) {
        if(nsnull != mStatisticsData[mItemIdx[j]]) 
             eucNum++;
        if(((&nsUCS2BEVerifier) != mVerifier[mItemIdx[j]]) &&
                ((&nsUCS2LEVerifier) != mVerifier[mItemIdx[j]]) &&
                ((&nsGB18030Verifier) != mVerifier[mItemIdx[j]]) ) {
                  possibleCandidateNum++;
        }
     }
     mRunSampler = (eucNum > 1);
     if(mRunSampler) {
        mRunSampler = mSampler.Sample(aBuf, aLen);
        if(((aLastChance && mSampler.GetSomeData()) || 
            mSampler.EnoughData())
           && (eucNum == possibleCandidateNum)) {
          mSampler.CalFreq();
#ifdef DETECTOR_DEBUG
          printf("We cannot figure out charset from the encoding, "
                 "All EUC based charset share the same encoding structure.\n"
                 "Detect based on statistics"); 
          if(aLastChance) {
             printf(" after we receive all the data.\n"); 
          } else {
             printf(" after we receive enough data.\n");
          }
#endif
          PRInt32 bestIdx = -1;
          PRInt32 eucCnt=0;
          float bestScore = 0.0f;
          for(j = 0; j < mItems; j++) {
             if((nsnull != mStatisticsData[mItemIdx[j]])  &&
                (&gBig5Statistics != mStatisticsData[mItemIdx[j]]))
             {
                float score = mSampler.GetScore(
                   mStatisticsData[mItemIdx[j]]->mFirstByteFreq,
                   mStatisticsData[mItemIdx[j]]->mFirstByteWeight,
                   mStatisticsData[mItemIdx[j]]->mSecoundByteFreq,
                   mStatisticsData[mItemIdx[j]]->mSecoundByteWeight );
#ifdef DETECTOR_DEBUG
                printf("Differences between %s and this data is %2.8f\n",
                       mVerifier[mItemIdx[j]]->charset,
                       score);
#endif
                if(( 0 == eucCnt++) || (bestScore > score )) {
                   bestScore = score;
                   bestIdx = j;
                } 
            } 
         } 
         if (bestIdx >= 0)
         {
#ifdef DETECTOR_DEBUG
           printf("Based on the statistic, we decide it is %s",
            mVerifier[mItemIdx[bestIdx]]->charset);
#endif
           Report( mVerifier[mItemIdx[bestIdx]]->charset);
           mDone = PR_TRUE;
         }
       } 
     } 
}

nsXPCOMDetector::nsXPCOMDetector(PRUint8 aItems, nsVerifier * const *aVer, nsEUCStatistics* const * aStatisticsSet)
   : nsPSMDetector( aItems, aVer, aStatisticsSet)
{
  mObserver = nsnull;
}

nsXPCOMDetector::~nsXPCOMDetector()
{
}

NS_IMETHODIMP nsXPCOMDetector::Init(
  nsICharsetDetectionObserver* aObserver)
{
  NS_ASSERTION(mObserver == nsnull , "Init twice");
  if(nsnull == aObserver)
     return NS_ERROR_ILLEGAL_VALUE;

  mObserver = aObserver;
  return NS_OK;
}

NS_IMETHODIMP nsXPCOMDetector::DoIt(
  const char* aBuf, PRUint32 aLen, PRBool* oDontFeedMe)
{
  NS_ASSERTION(mObserver != nsnull , "have not init yet");

  if((nsnull == aBuf) || (nsnull == oDontFeedMe))
     return NS_ERROR_ILLEGAL_VALUE;

  this->HandleData(aBuf, aLen);
  *oDontFeedMe = mDone;
  return NS_OK;
}

NS_IMETHODIMP nsXPCOMDetector::Done()
{
  NS_ASSERTION(mObserver != nsnull , "have not init yet");
  this->DataEnd();
  return NS_OK;
}

void nsXPCOMDetector::Report(const char* charset)
{
  mObserver->Notify(charset, eSureAnswer);
}

nsXPCOMStringDetector::nsXPCOMStringDetector(PRUint8 aItems, nsVerifier* const * aVer, nsEUCStatistics* const * aStatisticsSet)
   : nsPSMDetector( aItems, aVer, aStatisticsSet)
{
}

nsXPCOMStringDetector::~nsXPCOMStringDetector()
{
}

void nsXPCOMStringDetector::Report(const char* charset)
{
  mResult = charset;
}

NS_IMETHODIMP nsXPCOMStringDetector::DoIt(const char* aBuf, PRUint32 aLen, 
                   const char** oCharset, 
                   nsDetectionConfident &oConfident)
{
  mResult = nsnull;
  this->HandleData(aBuf, aLen);

  if( nsnull == mResult) {
     
     if(mDone) 
     {
        *oCharset = nsnull;
        oConfident = eNoAnswerMatch;
     } else {
        
        
        this->DataEnd();
        *oCharset = mResult;
        oConfident = (mResult) ? eBestAnswer : eNoAnswerMatch ;
     }
  } else {
     
     *oCharset = mResult;
     oConfident = eSureAnswer;
  }
  this->Reset();
  return NS_OK;
}

