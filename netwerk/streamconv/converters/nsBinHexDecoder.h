

















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
  uint32_t type, creator;
  uint16_t flags;
  int32_t dlen, rlen;
} binhex_header;

typedef union
{
  unsigned char c[4];
  uint32_t      val;
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

  int16_t  GetNextChar(uint32_t numBytesInBuffer);
  nsresult ProcessNextChunk(nsIRequest * aRequest, nsISupports * aContext, uint32_t numBytesInBuffer);
  nsresult ProcessNextState(nsIRequest * aRequest, nsISupports * aContext);
  nsresult DetectContentType(nsIRequest * aRequest, const nsAFlatCString &aFilename);

protected:
  nsCOMPtr<nsIStreamListener> mNextListener;

  
  nsCOMPtr<nsIOutputStream>     mOutputStream;     
  nsCOMPtr<nsIInputStream>      mInputStream;

  int16_t   mState;      
  uint16_t  mCRC;        
  uint16_t  mFileCRC;    
  longbuf   mOctetBuf;   
  int16_t   mOctetin;    
  int16_t   mDonePos;    
  int16_t   mInCRC;      

  
  binhex_header mHeader;
  nsCString mName;       

  
  
  char * mDataBuffer; 
  char * mOutgoingBuffer; 
  uint32_t mPosInDataBuffer;

  unsigned char mRlebuf;  

  uint32_t mCount;        
  int16_t mMarker;        

  int32_t mPosInbuff;     
  int32_t mPosOutputBuff; 
};

#endif 
