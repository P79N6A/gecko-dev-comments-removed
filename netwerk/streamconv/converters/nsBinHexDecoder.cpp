





































#include "nsBinHexDecoder.h"
#include "nsIServiceManager.h"
#include "nsIStreamConverterService.h"
#include "nsCRT.h"
#include "nsIPipe.h"
#include "nsMimeTypes.h"
#include "netCore.h"
#include "nsXPIDLString.h"
#include "prnetdb.h"
#include "nsIURI.h"
#include "nsIURL.h"

#include "nsIMIMEService.h"
#include "nsMimeTypes.h"


#define DATA_BUFFER_SIZE (4096*2)

#define NS_STREAM_CONVERTER_SEGMENT_SIZE   (4*1024)
#define NS_STREAM_CONVERTER_BUFFER_SIZE    (32*1024)


#define CR  '\015'
#define LF '\012'

nsBinHexDecoder::nsBinHexDecoder() :
  mState(0), mCRC(0), mFileCRC(0), mOctetin(26),
  mDonePos(3), mInCRC(0), mCount(0), mMarker(0), mPosInbuff(0),
  mPosOutputBuff(0)
{
  mDataBuffer = nsnull;
  mOutgoingBuffer = nsnull;

  mOctetBuf.val = 0;
  mHeader.type = 0;
  mHeader.creator = 0;
  mHeader.flags = 0;
  mHeader.dlen = 0;
  mHeader.rlen = 0;
}

nsBinHexDecoder::~nsBinHexDecoder()
{
  if (mDataBuffer)
    nsMemory::Free(mDataBuffer);
  if (mOutgoingBuffer)
    nsMemory::Free(mOutgoingBuffer);
}

NS_IMPL_ADDREF(nsBinHexDecoder)
NS_IMPL_RELEASE(nsBinHexDecoder)

NS_INTERFACE_MAP_BEGIN(nsBinHexDecoder)
  NS_INTERFACE_MAP_ENTRY(nsIStreamConverter)
  NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
  NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




static char binhex_decode[256] =
{
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, -1, -1,
  13, 14, 15, 16, 17, 18, 19, -1, 20, 21, -1, -1, -1, -1, -1, -1,
  22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, -1,
  37, 38, 39, 40, 41, 42, 43, -1, 44, 45, 46, 47, -1, -1, -1, -1,
  48, 49, 50, 51, 52, 53, 54, -1, 55, 56, 57, 58, 59, 60, -1, -1,
  61, 62, 63, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

#define BHEXVAL(c) (binhex_decode[(unsigned char) c])





NS_IMETHODIMP
nsBinHexDecoder::Convert(nsIInputStream *aFromStream,
                         const char *aFromType,
                         const char *aToType,
                         nsISupports *aCtxt,
                         nsIInputStream **aResultStream)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBinHexDecoder::AsyncConvertData(const char *aFromType,
                                  const char *aToType,
                                  nsIStreamListener *aListener,
                                  nsISupports *aCtxt)
{
  NS_ASSERTION(aListener && aFromType && aToType,
               "null pointer passed into bin hex converter");

  
  
  
  mNextListener = aListener;
  return (aListener) ? NS_OK : NS_ERROR_FAILURE;
}




NS_IMETHODIMP
nsBinHexDecoder::OnDataAvailable(nsIRequest* request,
                                 nsISupports *aCtxt,
                                 nsIInputStream *aStream,
                                 PRUint32 aSourceOffset,
                                 PRUint32 aCount)
{
  nsresult rv = NS_OK;

  if (mOutputStream && mDataBuffer && aCount > 0)
  {
    PRUint32 numBytesRead = 0;
    while (aCount > 0) 
    {
      aStream->Read(mDataBuffer, PR_MIN(aCount, DATA_BUFFER_SIZE - 1), &numBytesRead);
      if (aCount >= numBytesRead)
        aCount -= numBytesRead; 
      else
        aCount = 0;

      
      ProcessNextChunk(request, aCtxt, numBytesRead);
    }
  }

  return rv;
}

nsresult nsBinHexDecoder::ProcessNextState(nsIRequest * aRequest, nsISupports * aContext)
{
  nsresult status = NS_OK;
  PRUint16 tmpcrc, cval;
  unsigned char ctmp, c = mRlebuf;

  
  ctmp = mInCRC ? c : 0;
  cval = mCRC & 0xf000;
  tmpcrc = ((PRUint16) (mCRC << 4) | (ctmp >> 4)) ^ (cval | (cval >> 7) | (cval >> 12));
  cval = tmpcrc & 0xf000;
  mCRC = ((PRUint16) (tmpcrc << 4) | (ctmp & 0x0f)) ^ (cval | (cval >> 7) | (cval >> 12));

  
  switch (mState)
  {
    case BINHEX_STATE_START:
      mState = BINHEX_STATE_FNAME;
      mCount = 0;

      
      
      mName.SetLength(c & 63);
      if (mName.Length() != c & 63) {
        
        mState = BINHEX_STATE_DONE;
      }
      break;

    case BINHEX_STATE_FNAME:
      mName.BeginWriting()[mCount] = c;

      if (++mCount > mName.Length())
      {
        
        

        DetectContentType(aRequest, mName);
        
        mNextListener->OnStartRequest(aRequest, aContext);

        mState = BINHEX_STATE_HEADER;
        mCount = 0;
      }
      break;

    case BINHEX_STATE_HEADER:
      ((char *) &mHeader)[mCount] = c;
      if (++mCount == 18)
      {
        if (sizeof(binhex_header) != 18)  
        {
          char *p = (char *)&mHeader;
          p += 19;
          for (c = 0; c < 8; c++)
          {
            *p = *(p-2);
            --p;
          }
        }

        mState = BINHEX_STATE_HCRC;
        mInCRC = 1;
        mCount = 0;
      }
      break;

    case BINHEX_STATE_DFORK:
    case BINHEX_STATE_RFORK:
      mOutgoingBuffer[mPosOutputBuff++] = c;
      if (--mCount == 0)
      {
        
        if (mState == BINHEX_STATE_DFORK)
        {
          PRUint32 numBytesWritten = 0;
          mOutputStream->Write(mOutgoingBuffer, mPosOutputBuff, &numBytesWritten);
          if (PRInt32(numBytesWritten) != mPosOutputBuff)
            status = NS_ERROR_FAILURE;

          
          mNextListener->OnDataAvailable(aRequest, aContext, mInputStream, 0, numBytesWritten);
        }
        else
          status = NS_OK;        

        mPosOutputBuff = 0;

        if (status != NS_OK)
          mState = BINHEX_STATE_DONE;
        else
          ++mState;

        mInCRC = 1;
      }
      else if (mPosOutputBuff >= DATA_BUFFER_SIZE)
      {
        if (mState == BINHEX_STATE_DFORK)
        {
          PRUint32 numBytesWritten = 0;
          mOutputStream->Write(mOutgoingBuffer, mPosOutputBuff, &numBytesWritten);
          if (PRInt32(numBytesWritten) != mPosOutputBuff)
            status = NS_ERROR_FAILURE;

          mNextListener->OnDataAvailable(aRequest, aContext, mInputStream, 0, numBytesWritten);
          mPosOutputBuff = 0;
        }
      }
      break;

    case BINHEX_STATE_HCRC:
    case BINHEX_STATE_DCRC:
    case BINHEX_STATE_RCRC:
      if (!mCount++)
        mFileCRC = (unsigned short) c << 8;
      else
      {
        if ((mFileCRC | c) != mCRC)
        {
          mState = BINHEX_STATE_DONE;
          break;
        }

        
        mCRC = 0;
        if (++mState == BINHEX_STATE_FINISH)
        {
          
          mNextListener->OnStopRequest(aRequest, aContext, NS_OK);
          mNextListener = 0;

          
          ++mState;
          break;
        }

        if (mState == BINHEX_STATE_DFORK)
          mCount = PR_ntohl(mHeader.dlen);
        else
        {
          
          
          
          mCount = 0;
        }

        if (mCount) {
          mInCRC = 0;
        } else {
          
          ++mState;
        }
      }
      break;
  }

  return NS_OK;
}

nsresult nsBinHexDecoder::ProcessNextChunk(nsIRequest * aRequest, nsISupports * aContext, PRUint32 numBytesInBuffer)
{
  PRBool foundStart;
  PRInt16 octetpos, c = 0;
  PRUint32 val;
  mPosInDataBuffer = 0; 

  NS_ENSURE_TRUE(numBytesInBuffer > 0, NS_ERROR_FAILURE);

  
  if (mState == BINHEX_STATE_START)
  {
    foundStart = PR_FALSE;
    
    while (mPosInDataBuffer < numBytesInBuffer)
    {
      c = mDataBuffer[mPosInDataBuffer++];
      while (c == CR || c == LF)
      {
        if (mPosInDataBuffer >= numBytesInBuffer)
          break;

        c = mDataBuffer[mPosInDataBuffer++];
        if (c == ':')
        {
          foundStart = PR_TRUE;
          break;
        }
      }
      if (foundStart)  break;    
    }

    if (mPosInDataBuffer >= numBytesInBuffer)
      return NS_OK;      

    if (c != ':')
      return NS_ERROR_FAILURE;    
  }

  while (mState != BINHEX_STATE_DONE)
  {
    
    do
    {
      if (mPosInDataBuffer >= numBytesInBuffer)
        return NS_OK;      

      c = GetNextChar(numBytesInBuffer);
      if (c == 0)  return NS_OK;

      if ((val = BHEXVAL(c)) == PRUint32(-1))
      {
        
        if (c)
        {
          
          --mDonePos;
          if (mOctetin >= 14)
            --mDonePos;
          if (mOctetin >= 20)
            --mDonePos;
        }
        break;
      }
      mOctetBuf.val |= val << mOctetin;
    }
    while ((mOctetin -= 6) > 2);

    

    
    
    
    
    mOctetBuf.val = PR_htonl(mOctetBuf.val);

    for (octetpos = 0; octetpos < mDonePos; ++octetpos)
    {
      c = mOctetBuf.c[octetpos];

      if (c == 0x90 && !mMarker++)
        continue;

      if (mMarker)
      {
        if (c == 0)
        {
          mRlebuf = 0x90;
          ProcessNextState(aRequest, aContext);
        }
        else
        {
          
          while (--c > 0)
            ProcessNextState(aRequest, aContext);
        }
        mMarker = 0;
      }
      else
      {
        mRlebuf = (unsigned char) c;
        ProcessNextState(aRequest, aContext);
      }

      if (mState >= BINHEX_STATE_DONE)
        break;
    }

    
    if (mDonePos < 3 && mState < BINHEX_STATE_DONE)
      mState = BINHEX_STATE_DONE;

    mOctetin = 26;
    mOctetBuf.val = 0;
  }

  return   NS_OK;
}

PRInt16 nsBinHexDecoder::GetNextChar(PRUint32 numBytesInBuffer)
{
  char c = 0;

  while (mPosInDataBuffer < numBytesInBuffer)
  {
    c = mDataBuffer[mPosInDataBuffer++];
    if (c != LF && c != CR)
      break;
  }
  return (c == LF || c == CR) ? 0 : (int) c;
}





NS_IMETHODIMP
nsBinHexDecoder::OnStartRequest(nsIRequest* request, nsISupports *aCtxt)
{
  nsresult rv = NS_OK;

  NS_ENSURE_TRUE(mNextListener, NS_ERROR_FAILURE);

  mDataBuffer = (char *) nsMemory::Alloc((sizeof(char) * DATA_BUFFER_SIZE));
  mOutgoingBuffer = (char *) nsMemory::Alloc((sizeof(char) * DATA_BUFFER_SIZE));
  if (!mDataBuffer || !mOutgoingBuffer) return NS_ERROR_FAILURE; 

  
  rv = NS_NewPipe(getter_AddRefs(mInputStream), getter_AddRefs(mOutputStream),
                  NS_STREAM_CONVERTER_SEGMENT_SIZE,
                  NS_STREAM_CONVERTER_BUFFER_SIZE,
                  PR_TRUE, PR_TRUE);

  
  return rv;
}





nsresult nsBinHexDecoder::DetectContentType(nsIRequest* aRequest,
                                            const nsAFlatCString &aFilename)
{
  if (aFilename.IsEmpty()) {
    
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMIMEService> mimeService(do_GetService("@mozilla.org/mime;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString contentType;

  
  const char * fileExt = strrchr(aFilename.get(), '.');
  if (!fileExt) {
    return NS_OK;
  }

  mimeService->GetTypeFromExtension(nsDependentCString(fileExt), contentType);

  
  if (!contentType.IsEmpty() && !contentType.Equals(APPLICATION_BINHEX)) {
    channel->SetContentType(contentType);
  } else {
    channel->SetContentType(NS_LITERAL_CSTRING(UNKNOWN_CONTENT_TYPE));
  }

  return NS_OK;
}


NS_IMETHODIMP
nsBinHexDecoder::OnStopRequest(nsIRequest* request, nsISupports *aCtxt,
                                nsresult aStatus)
{
  nsresult rv = NS_OK;

  if (!mNextListener) return NS_ERROR_FAILURE;
  
  

  return rv;
}
