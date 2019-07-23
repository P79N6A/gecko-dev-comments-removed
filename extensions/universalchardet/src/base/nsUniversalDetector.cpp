





































#include "nscore.h"

#include "nsUniversalDetector.h"

#include "nsMBCSGroupProber.h"
#include "nsSBCSGroupProber.h"
#include "nsEscCharsetProber.h"
#include "nsLatin1Prober.h"

nsUniversalDetector::nsUniversalDetector()
{
  mDone = PR_FALSE;
  mBestGuess = -1;   
  mInTag = PR_FALSE;
  mEscCharSetProber = nsnull;

  mStart = PR_TRUE;
  mDetectedCharset = nsnull;
  mGotData = PR_FALSE;
  mInputState = ePureAscii;
  mLastChar = '\0';

  PRUint32 i;
  for (i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
    mCharSetProbers[i] = nsnull;
}

nsUniversalDetector::~nsUniversalDetector() 
{
  for (PRInt32 i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
    if (mCharSetProbers[i])      
      delete mCharSetProbers[i];
  if (mEscCharSetProber)
    delete mEscCharSetProber;
}

void 
nsUniversalDetector::Reset()
{
  mDone = PR_FALSE;
  mBestGuess = -1;   
  mInTag = PR_FALSE;

  mStart = PR_TRUE;
  mDetectedCharset = nsnull;
  mGotData = PR_FALSE;
  mInputState = ePureAscii;
  mLastChar = '\0';

  if (mEscCharSetProber)
    mEscCharSetProber->Reset();

  PRUint32 i;
  for (i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
    if (mCharSetProbers[i])
      mCharSetProbers[i]->Reset();
}


#define SHORTCUT_THRESHOLD      (float)0.95
#define MINIMUM_THRESHOLD      (float)0.20

nsresult nsUniversalDetector::HandleData(const char* aBuf, PRUint32 aLen)
{
  if(mDone) 
    return NS_OK;

  if (aLen > 0)
    mGotData = PR_TRUE;

  
  if (mStart)
  {
    mStart = PR_FALSE;
    if (aLen > 3)
      switch (aBuf[0])
        {
        case '\xEF':
          if (('\xBB' == aBuf[1]) && ('\xBF' == aBuf[2]))
            
            mDetectedCharset = "UTF-8";
        break;
        case '\xFE':
          if (('\xFF' == aBuf[1]) && ('\x00' == aBuf[2]) && ('\x00' == aBuf[3]))
            
            mDetectedCharset = "X-ISO-10646-UCS-4-3412";
          else if ('\xFF' == aBuf[1])
            
            mDetectedCharset = "UTF-16BE";
        break;
        case '\x00':
          if (('\x00' == aBuf[1]) && ('\xFE' == aBuf[2]) && ('\xFF' == aBuf[3]))
            
            mDetectedCharset = "UTF-32BE";
          else if (('\x00' == aBuf[1]) && ('\xFF' == aBuf[2]) && ('\xFE' == aBuf[3]))
            
            mDetectedCharset = "X-ISO-10646-UCS-4-2143";
        break;
        case '\xFF':
          if (('\xFE' == aBuf[1]) && ('\x00' == aBuf[2]) && ('\x00' == aBuf[3]))
            
            mDetectedCharset = "UTF-32LE";
          else if ('\xFE' == aBuf[1])
            
            mDetectedCharset = "UTF-16LE";
        break;
      }  

      if (mDetectedCharset)
      {
        mDone = PR_TRUE;
        return NS_OK;
      }
  }
  
  PRUint32 i;
  for (i = 0; i < aLen; i++)
  {
    
    if (aBuf[i] & '\x80' && aBuf[i] != '\xA0')  
    {
      
      if (mInputState != eHighbyte)
      {
        
        mInputState = eHighbyte;

        
        if (mEscCharSetProber) {
          delete mEscCharSetProber;
          mEscCharSetProber = nsnull;
        }

        
        if (nsnull == mCharSetProbers[0])
          mCharSetProbers[0] = new nsMBCSGroupProber;
        if (nsnull == mCharSetProbers[1])
          mCharSetProbers[1] = new nsSBCSGroupProber;
        if (nsnull == mCharSetProbers[2])
          mCharSetProbers[2] = new nsLatin1Prober; 

        if ((nsnull == mCharSetProbers[0]) ||
            (nsnull == mCharSetProbers[1]) ||
            (nsnull == mCharSetProbers[2]))
            return NS_ERROR_OUT_OF_MEMORY;
      }
    }
    else
    {
      
      if ( ePureAscii == mInputState &&
        (aBuf[i] == '\033' || (aBuf[i] == '{' && mLastChar == '~')) )
      {
        
        mInputState = eEscAscii;
      }
      mLastChar = aBuf[i];
    }
  }

  nsProbingState st;
  switch (mInputState)
  {
  case eEscAscii:
    if (nsnull == mEscCharSetProber) {
      mEscCharSetProber = new nsEscCharSetProber;
      if (nsnull == mEscCharSetProber)
        return NS_ERROR_OUT_OF_MEMORY;
    }
    st = mEscCharSetProber->HandleData(aBuf, aLen);
    if (st == eFoundIt)
    {
      mDone = PR_TRUE;
      mDetectedCharset = mEscCharSetProber->GetCharSetName();
    }
    break;
  case eHighbyte:
    for (i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
    {
      st = mCharSetProbers[i]->HandleData(aBuf, aLen);
      if (st == eFoundIt) 
      {
        mDone = PR_TRUE;
        mDetectedCharset = mCharSetProbers[i]->GetCharSetName();
        return NS_OK;
      } 
    }
    break;

  default:  
    ;
  }
  return NS_OK;
}



void nsUniversalDetector::DataEnd()
{
  if (!mGotData)
  {
    
    
    return;
  }

  if (mDetectedCharset)
  {
    mDone = PR_TRUE;
    Report(mDetectedCharset);
    return;
  }
  
  switch (mInputState)
  {
  case eHighbyte:
    {
      float proberConfidence;
      float maxProberConfidence = (float)0.0;
      PRInt32 maxProber = 0;

      for (PRInt32 i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
      {
        proberConfidence = mCharSetProbers[i]->GetConfidence();
        if (proberConfidence > maxProberConfidence)
        {
          maxProberConfidence = proberConfidence;
          maxProber = i;
        }
      }
      
      if (maxProberConfidence > MINIMUM_THRESHOLD)
        Report(mCharSetProbers[maxProber]->GetCharSetName());
    }
    break;
  case eEscAscii:
    break;
  default:
    ;
  }
  return;
}
