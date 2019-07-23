




































#ifndef nsUnknownDecoder_h__
#define nsUnknownDecoder_h__

#include "nsIStreamConverter.h"
#include "nsIChannel.h"
#include "nsIContentSniffer.h"

#include "nsCOMPtr.h"
#include "nsString.h"

#define NS_UNKNOWNDECODER_CID                        \
{ /* 7d7008a0-c49a-11d3-9b22-0080c7cb1080 */         \
    0x7d7008a0,                                      \
    0xc49a,                                          \
    0x11d3,                                          \
    {0x9b, 0x22, 0x00, 0x80, 0xc7, 0xcb, 0x10, 0x80}       \
}


class nsUnknownDecoder : public nsIStreamConverter, public nsIContentSniffer
{
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSISTREAMCONVERTER

  
  NS_DECL_NSISTREAMLISTENER

  
  NS_DECL_NSIREQUESTOBSERVER

  
  NS_DECL_NSICONTENTSNIFFER

  nsUnknownDecoder();

protected:
  virtual ~nsUnknownDecoder();

  virtual void DetermineContentType(nsIRequest* aRequest);
  nsresult FireListenerNotifications(nsIRequest* request, nsISupports *aCtxt);

protected:
  nsCOMPtr<nsIStreamListener> mNextListener;

  
  
  
  
  
  PRBool AllowSniffing(nsIRequest* aRequest);
  
  
  
  PRBool TryContentSniffers(nsIRequest* aRequest);
  PRBool SniffForHTML(nsIRequest* aRequest);
  PRBool SniffForXML(nsIRequest* aRequest);

  
  
  PRBool SniffURI(nsIRequest* aRequest);

  
  
  
  
  PRBool LastDitchSniff(nsIRequest* aRequest);

  







  struct nsSnifferEntry {
    typedef PRBool (nsUnknownDecoder::*TypeSniffFunc)(nsIRequest* aRequest);
    
    const char* mBytes;
    PRUint32 mByteLen;
    
    
    const char* mMimeType;
    TypeSniffFunc mContentTypeSniffer;
  };

#define SNIFFER_ENTRY(_bytes, _type) \
  { _bytes, sizeof(_bytes) - 1, _type, nsnull }

#define SNIFFER_ENTRY_WITH_FUNC(_bytes, _func) \
  { _bytes, sizeof(_bytes) - 1, nsnull, _func }

  static nsSnifferEntry sSnifferEntries[];
  static PRUint32 sSnifferEntryNum;
  
  char *mBuffer;
  PRUint32 mBufferLen;
  PRBool mRequireHTMLsuffix;

  nsCString mContentType;

};

#define NS_BINARYDETECTOR_CID                        \
{ /* a2027ec6-ba0d-4c72-805d-148233f5f33c */         \
    0xa2027ec6,                                      \
    0xba0d,                                          \
    0x4c72,                                          \
    {0x80, 0x5d, 0x14, 0x82, 0x33, 0xf5, 0xf3, 0x3c} \
}







class nsBinaryDetector : public nsUnknownDecoder
{
protected:
  virtual void DetermineContentType(nsIRequest* aRequest);
};

#endif 

