



#ifndef __nsmultimixedconv__h__
#define __nsmultimixedconv__h__

#include "nsIStreamConverter.h"
#include "nsIChannel.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIByteRangeRequest.h"
#include "nsILoadInfo.h"
#include "nsIMultiPartChannel.h"
#include "nsAutoPtr.h"
#include "mozilla/Attributes.h"
#include "nsIResponseHeadProvider.h"
#include "nsHttpResponseHead.h"

using mozilla::net::nsHttpResponseHead;

#define NS_MULTIMIXEDCONVERTER_CID                         \
{ /* 7584CE90-5B25-11d3-A175-0050041CAF44 */         \
    0x7584ce90,                                      \
    0x5b25,                                          \
    0x11d3,                                          \
    {0xa1, 0x75, 0x0, 0x50, 0x4, 0x1c, 0xaf, 0x44}       \
}








class nsPartChannel MOZ_FINAL : public nsIChannel,
                                public nsIByteRangeRequest,
                                public nsIResponseHeadProvider,
                                public nsIMultiPartChannel
{
public:
  nsPartChannel(nsIChannel *aMultipartChannel, uint32_t aPartID,
                nsIStreamListener* aListener);

  void InitializeByteRange(int64_t aStart, int64_t aEnd);
  void SetIsLastPart() { mIsLastPart = true; }
  nsresult SendOnStartRequest(nsISupports* aContext);
  nsresult SendOnDataAvailable(nsISupports* aContext, nsIInputStream* aStream,
                               uint64_t aOffset, uint32_t aLen);
  nsresult SendOnStopRequest(nsISupports* aContext, nsresult aStatus);
  

  void SetContentDisposition(const nsACString& aContentDispositionHeader);
  void SetResponseHead(nsHttpResponseHead * head) { mResponseHead = head; }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIBYTERANGEREQUEST
  NS_DECL_NSIRESPONSEHEADPROVIDER
  NS_DECL_NSIMULTIPARTCHANNEL

protected:
  ~nsPartChannel();

protected:
  nsCOMPtr<nsIChannel>    mMultipartChannel;
  nsCOMPtr<nsIStreamListener> mListener;
  nsAutoPtr<nsHttpResponseHead> mResponseHead;

  nsresult                mStatus;
  nsLoadFlags             mLoadFlags;

  nsCOMPtr<nsILoadGroup>  mLoadGroup;

  nsCString               mContentType;
  nsCString               mContentCharset;
  uint32_t                mContentDisposition;
  nsString                mContentDispositionFilename;
  nsCString               mContentDispositionHeader;
  uint64_t                mContentLength;

  bool                    mIsByteRangeRequest;
  int64_t                 mByteRangeStart;
  int64_t                 mByteRangeEnd;

  uint32_t                mPartID; 
                                   
  bool                    mIsLastPart;
};






































class nsMultiMixedConv : public nsIStreamConverter {
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMCONVERTER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER

    nsMultiMixedConv();

protected:
    virtual ~nsMultiMixedConv();

    nsresult SendStart(nsIChannel *aChannel);
    nsresult SendStop(nsresult aStatus);
    nsresult SendData(char *aBuffer, uint32_t aLen);
    nsresult ParseHeaders(nsIChannel *aChannel, char *&aPtr,
                          uint32_t &aLen, bool *_retval);
    int32_t  PushOverLine(char *&aPtr, uint32_t &aLen);
    char *FindToken(char *aCursor, uint32_t aLen);
    nsresult BufferData(char *aData, uint32_t aLen);

    
    bool                mNewPart;        
    bool                mProcessingHeaders;
    nsCOMPtr<nsIStreamListener> mFinalListener; 

    nsCString           mToken;
    uint32_t            mTokenLen;

    nsRefPtr<nsPartChannel> mPartChannel;   
                                        
    nsCOMPtr<nsISupports> mContext;
    nsCString           mContentType;
    nsCString           mContentDisposition;
    uint64_t            mContentLength;
    
    char                *mBuffer;
    uint32_t            mBufLen;
    uint64_t            mTotalSent;
    bool                mFirstOnData;   

    
    
    
    int64_t             mByteRangeStart;
    int64_t             mByteRangeEnd;
    bool                mIsByteRangeRequest;

    uint32_t            mCurrentPartID;

    
    
    
    bool                mPackagedApp;
    nsAutoPtr<nsHttpResponseHead> mResponseHead;
};

#endif 
