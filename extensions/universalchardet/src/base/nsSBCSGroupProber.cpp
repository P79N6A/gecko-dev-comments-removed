





































#include <stdio.h>
#include "prmem.h"

#include "nsSBCharSetProber.h"
#include "nsSBCSGroupProber.h"

#include "nsHebrewProber.h"

nsSBCSGroupProber::nsSBCSGroupProber()
{
  mProbers[0] = new nsSingleByteCharSetProber(&Win1251Model);
  mProbers[1] = new nsSingleByteCharSetProber(&Koi8rModel);
  mProbers[2] = new nsSingleByteCharSetProber(&Latin5Model);
  mProbers[3] = new nsSingleByteCharSetProber(&MacCyrillicModel);
  mProbers[4] = new nsSingleByteCharSetProber(&Ibm866Model);
  mProbers[5] = new nsSingleByteCharSetProber(&Ibm855Model);
  mProbers[6] = new nsSingleByteCharSetProber(&Latin7Model);
  mProbers[7] = new nsSingleByteCharSetProber(&Win1253Model);
  mProbers[8] = new nsSingleByteCharSetProber(&Latin5BulgarianModel);
  mProbers[9] = new nsSingleByteCharSetProber(&Win1251BulgarianModel);
  mProbers[10] = new nsSingleByteCharSetProber(&TIS620ThaiModel);

  nsHebrewProber *hebprober = new nsHebrewProber();
  
  
  mProbers[11] = hebprober;
  mProbers[12] = new nsSingleByteCharSetProber(&Win1255Model, false, hebprober); 
  mProbers[13] = new nsSingleByteCharSetProber(&Win1255Model, true, hebprober); 
  
  if (mProbers[11] && mProbers[12] && mProbers[13]) 
  {
    hebprober->SetModelProbers(mProbers[12], mProbers[13]);
  }
  else 
  {
    for (PRUint32 i = 11; i <= 13; ++i)
    { 
      delete mProbers[i]; 
      mProbers[i] = 0; 
    }
  }

  
  
  
  

  Reset();
}

nsSBCSGroupProber::~nsSBCSGroupProber()
{
  for (PRUint32 i = 0; i < NUM_OF_SBCS_PROBERS; i++)
  {
    delete mProbers[i];
  }
}


const char* nsSBCSGroupProber::GetCharSetName()
{
  
  if (mBestGuess == -1)
  {
    GetConfidence();
    
    if (mBestGuess == -1)
      
      mBestGuess = 0;
  }
  return mProbers[mBestGuess]->GetCharSetName();
}

void  nsSBCSGroupProber::Reset(void)
{
  mActiveNum = 0;
  for (PRUint32 i = 0; i < NUM_OF_SBCS_PROBERS; i++)
  {
    if (mProbers[i]) 
    {
      mProbers[i]->Reset();
      mIsActive[i] = true;
      ++mActiveNum;
    }
    else
      mIsActive[i] = false;
  }
  mBestGuess = -1;
  mState = eDetecting;
}


nsProbingState nsSBCSGroupProber::HandleData(const char* aBuf, PRUint32 aLen)
{
  nsProbingState st;
  PRUint32 i;
  char *newBuf1 = 0;
  PRUint32 newLen1 = 0;

  
  
  
  
  
  
  if (!FilterWithoutEnglishLetters(aBuf, aLen, &newBuf1, newLen1))
    goto done;
  
  if (newLen1 == 0)
    goto done; 

  for (i = 0; i < NUM_OF_SBCS_PROBERS; i++)
  {
     if (!mIsActive[i])
       continue;
     st = mProbers[i]->HandleData(newBuf1, newLen1);
     if (st == eFoundIt)
     {
       mBestGuess = i;
       mState = eFoundIt;
       break;
     }
     else if (st == eNotMe)
     {
       mIsActive[i] = false;
       mActiveNum--;
       if (mActiveNum <= 0)
       {
         mState = eNotMe;
         break;
       }
     }
  }

done:
  PR_FREEIF(newBuf1);

  return mState;
}

float nsSBCSGroupProber::GetConfidence(void)
{
  PRUint32 i;
  float bestConf = 0.0, cf;

  switch (mState)
  {
  case eFoundIt:
    return (float)0.99; 
  case eNotMe:
    return (float)0.01;  
  default:
    for (i = 0; i < NUM_OF_SBCS_PROBERS; i++)
    {
      if (!mIsActive[i])
        continue;
      cf = mProbers[i]->GetConfidence();
      if (bestConf < cf)
      {
        bestConf = cf;
        mBestGuess = i;
      }
    }
  }
  return bestConf;
}

#ifdef DEBUG_chardet
void nsSBCSGroupProber::DumpStatus()
{
  PRUint32 i;
  float cf;
  
  cf = GetConfidence();
  printf(" SBCS Group Prober --------begin status \r\n");
  for (i = 0; i < NUM_OF_SBCS_PROBERS; i++)
  {
    if (!mIsActive[i])
      printf("  inactive: [%s] (i.e. confidence is too low).\r\n", mProbers[i]->GetCharSetName());
    else
      mProbers[i]->DumpStatus();
  }
  printf(" SBCS Group found best match [%s] confidence %f.\r\n",  
         mProbers[mBestGuess]->GetCharSetName(), cf);
}
#endif
