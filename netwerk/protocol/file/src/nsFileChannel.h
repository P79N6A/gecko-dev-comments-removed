






































#ifndef nsFileChannel_h__
#define nsFileChannel_h__

#include "nsBaseChannel.h"
#include "nsIFileChannel.h"
#include "nsIUploadChannel.h"

class nsFileChannel : public nsBaseChannel
                    , public nsIFileChannel
                    , public nsIUploadChannel
{
public: 
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIFILECHANNEL
  NS_DECL_NSIUPLOADCHANNEL

  nsFileChannel(nsIURI *uri) {
    SetURI(uri);
  }

protected:
  
  
  
  
  nsresult MakeFileInputStream(nsIFile *file, nsCOMPtr<nsIInputStream> &stream,
                               nsCString &contentType);

  virtual nsresult OpenContentStream(PRBool async, nsIInputStream **result);

private:
  nsCOMPtr<nsIInputStream> mUploadStream;
  PRInt64 mUploadLength;
};

#endif 
