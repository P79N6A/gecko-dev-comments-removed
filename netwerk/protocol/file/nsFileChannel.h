





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

  explicit nsFileChannel(nsIURI *uri);

protected:
  ~nsFileChannel();

  
  
  
  
  nsresult MakeFileInputStream(nsIFile *file, nsCOMPtr<nsIInputStream> &stream,
                               nsCString &contentType, bool async);

  virtual nsresult OpenContentStream(bool async, nsIInputStream **result,
                                     nsIChannel** channel);

private:
  nsCOMPtr<nsIInputStream> mUploadStream;
  int64_t mUploadLength;
};

#endif 
