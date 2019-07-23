



































#ifndef __nsmultimixedconv__h__
#define __nsmultimixedconv__h__

#include "nsIStreamConverter.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsXPIDLString.h"
#include "nsCOMPtr.h"
#include "nsInt64.h"
#include "nsIByteRangeRequest.h"
#include "nsIMultiPartChannel.h"
#include "nsAutoPtr.h"

#define NS_MULTIMIXEDCONVERTER_CID                         \
{ /* 7584CE90-5B25-11d3-A175-0050041CAF44 */         \
    0x7584ce90,                                      \
    0x5b25,                                          \
    0x11d3,                                          \
    {0xa1, 0x75, 0x0, 0x50, 0x4, 0x1c, 0xaf, 0x44}       \
}








class nsPartChannel : public nsIChannel,
                      public nsIByteRangeRequest,
                      public nsIMultiPartChannel
{
public:
  nsPartChannel(nsIChannel *aMultipartChannel, PRUint32 aPartID);

  void InitializeByteRange(PRInt64 aStart, PRInt64 aEnd);
  void SetIsLastPart() { mIsLastPart = PR_TRUE; }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIBYTERANGEREQUEST
  NS_DECL_NSIMULTIPARTCHANNEL

protected:
  ~nsPartChannel();

protected:
  nsCOMPtr<nsIChannel>    mMultipartChannel;
  
  nsresult                mStatus;
  nsLoadFlags             mLoadFlags;

  nsCOMPtr<nsILoadGroup>  mLoadGroup;

  nsCString               mContentType;
  nsCString               mContentCharset;
  nsCString               mContentDisposition;
  PRUint64                mContentLength;

  PRBool                  mIsByteRangeRequest;
  nsInt64                 mByteRangeStart;
  nsInt64                 mByteRangeEnd;

  PRUint32                mPartID; 
                                   
  PRBool                  mIsLastPart;
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
                          PRUint32 &aLen, PRBool *_retval);
    PRInt32  PushOverLine(char *&aPtr, PRUint32 &aLen);
    char *FindToken(char *aCursor, PRUint32 aLen);
    nsresult BufferData(char *aData, PRUint32 aLen);

    
    PRBool              mNewPart;        
    PRBool              mProcessingHeaders;
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
    PRBool              mFirstOnData;   

    
    
    
    nsInt64             mByteRangeStart;
    nsInt64             mByteRangeEnd;
    PRBool              mIsByteRangeRequest;

    PRUint32            mCurrentPartID;
};

#endif 
