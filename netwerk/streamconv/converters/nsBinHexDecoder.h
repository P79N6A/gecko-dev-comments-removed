


















































#ifndef nsBinHexDecoder_h__
#define nsBinHexDecoder_h__

#include "nsIStreamConverter.h"
#include "nsIChannel.h"
#include "nsIOutputStream.h"
#include "nsIInputStream.h"

#include "nsCOMPtr.h"
#include "nsString.h"

#define NS_BINHEXDECODER_CID                         \
{ /* 301DEA42-6850-4cda-8945-81F7DBC2186B */         \
  0x301dea42, 0x6850, 0x4cda,                        \
  { 0x89, 0x45, 0x81, 0xf7, 0xdb, 0xc2, 0x18, 0x6b } \
}

typedef struct _binhex_header
{
  PRUint32 type, creator;
  PRUint16 flags;
  PRInt32 dlen, rlen;
} binhex_header;

typedef union
{
  unsigned char c[4];
  PRUint32      val;
} longbuf;

#define BINHEX_STATE_START    0
#define BINHEX_STATE_FNAME    1
#define BINHEX_STATE_HEADER   2
#define BINHEX_STATE_HCRC     3
#define BINHEX_STATE_DFORK    4
#define BINHEX_STATE_DCRC     5
#define BINHEX_STATE_RFORK    6
#define BINHEX_STATE_RCRC     7
#define BINHEX_STATE_FINISH   8
#define BINHEX_STATE_DONE     9


class nsBinHexDecoder : public nsIStreamConverter
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSISTREAMCONVERTER

  
  NS_DECL_NSISTREAMLISTENER

  
  NS_DECL_NSIREQUESTOBSERVER

  nsBinHexDecoder();

protected:
  virtual ~nsBinHexDecoder();

  PRInt16  GetNextChar(PRUint32 numBytesInBuffer);
  nsresult ProcessNextChunk(nsIRequest * aRequest, nsISupports * aContext, PRUint32 numBytesInBuffer);
  nsresult ProcessNextState(nsIRequest * aRequest, nsISupports * aContext);
  nsresult SetContentType(nsIRequest * aRequest, const char * fileName);

protected:
  nsCOMPtr<nsIStreamListener> mNextListener;

  
  nsCOMPtr<nsIOutputStream>     mOutputStream;     
  nsCOMPtr<nsIInputStream>      mInputStream;

  PRInt16   mState;      
  PRUint16  mCRC;        
  PRUint16  mFileCRC;    
  longbuf   mOctetBuf;   
  PRInt16   mOctetin;    
  PRInt16   mDonePos;    
  PRInt16   mInCRC;      

  
  binhex_header mHeader;
  char      mName[64];   

  
  
  char * mDataBuffer; 
  char * mOutgoingBuffer; 
  PRUint32 mPosInDataBuffer;

  unsigned char mRlebuf;  

  PRInt32 mCount;         
  PRInt16 mMarker;        

  PRInt32 mPosInbuff;     
  PRInt32 mPosOutputBuff; 
};

#endif 
