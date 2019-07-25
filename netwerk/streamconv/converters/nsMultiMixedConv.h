



#ifndef __nsmultimixedconv__h__
#define __nsmultimixedconv__h__

#include "nsIStreamConverter.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsIByteRangeRequest.h"
#include "nsIMultiPartChannel.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"

#define NS_MULTIMIXEDCONVERTER_CID                         \
{ /* 7584CE90-5B25-11d3-A175-0050041CAF44 */         \
    0x7584ce90,                                      \
    0x5b25,                                          \
    0x11d3,                                          \
    {0xa1, 0x75, 0x0, 0x50, 0x4, 0x1c, 0xaf, 0x44}       \
}








class nsPartChannel MOZ_FINAL : public nsIChannel,
                                public nsIByteRangeRequest,
                                public nsIMultiPartChannel
{
public:
  nsPartChannel(nsIChannel *aMultipartChannel, PRUint32 aPartID,
                nsIStreamListener* aListener);

  void InitializeByteRange(PRInt64 aStart, PRInt64 aEnd);
  void SetIsLastPart() { mIsLastPart = true; }
  nsresult SendOnStartRequest(nsISupports* aContext);
  nsresult SendOnDataAvailable(nsISupports* aContext, nsIInputStream* aStream,
                               PRUint32 aOffset, PRUint32 aLen);
  nsresult SendOnStopRequest(nsISupports* aContext, nsresult aStatus);
  

  void SetContentDisposition(const nsACString& aContentDispositionHeader);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIBYTERANGEREQUEST
  NS_DECL_NSIMULTIPARTCHANNEL

protected:
  ~nsPartChannel();

protected:
  nsCOMPtr<nsIChannel>    mMultipartChannel;
  nsCOMPtr<nsIStreamListener> mListener;
  
  nsresult                mStatus;
  nsLoadFlags             mLoadFlags;

  nsCOMPtr<nsILoadGroup>  mLoadGroup;

  nsCString               mContentType;
  nsCString               mContentCharset;
  PRUint32                mContentDisposition;
  nsString                mContentDispositionFilename;
  nsCString               mContentDispositionHeader;
  PRUint64                mContentLength;

  bool                    mIsByteRangeRequest;
  PRInt64                 mByteRangeStart;
  PRInt64                 mByteRangeEnd;

  PRUint32                mPartID; 
                                   
  bool                    mIsLastPart;
};






































class nsMultiMixedConv : public nsIStreamConverter {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMCONVERTER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER

    nsMultiMixedConv();
    virtual ~nsMultiMixedConv();

protected:
    nsresult SendStart(nsIChannel *aChannel);
    nsresult SendStop(nsresult aStatus);
    nsresult SendData(char *aBuffer, PRUint32 aLen);
    nsresult ParseHeaders(nsIChannel *aChannel, char *&aPtr,
                          PRUint32 &aLen, bool *_retval);
    PRInt32  PushOverLine(char *&aPtr, PRUint32 &aLen);
    char *FindToken(char *aCursor, PRUint32 aLen);
    nsresult BufferData(char *aData, PRUint32 aLen);

    
    bool                mNewPart;        
    bool                mProcessingHeaders;
    nsCOMPtr<nsIStreamListener> mFinalListener; 

    nsCString           mToken;
    PRUint32            mTokenLen;

    nsRefPtr<nsPartChannel> mPartChannel;   
                                        
    nsCOMPtr<nsISupports> mContext;
    nsCString           mContentType;
    nsCString           mContentDisposition;
    PRUint64            mContentLength;
    
    char                *mBuffer;
    PRUint32            mBufLen;
    PRUint64            mTotalSent;
    bool                mFirstOnData;   

    
    
    
    PRInt64             mByteRangeStart;
    PRInt64             mByteRangeEnd;
    bool                mIsByteRangeRequest;

    PRUint32            mCurrentPartID;
};

#endif 
