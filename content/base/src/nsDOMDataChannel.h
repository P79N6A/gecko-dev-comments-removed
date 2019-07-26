





#ifndef nsDOMDataChannel_h
#define nsDOMDataChannel_h

#include "mozilla/net/DataChannel.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMDataChannel.h"

class nsDOMDataChannel : public nsDOMEventTargetHelper,
                         public nsIDOMDataChannel,
                         public mozilla::DataChannelListener
{
public:
  nsDOMDataChannel(already_AddRefed<mozilla::DataChannel> aDataChannel)
    : mDataChannel(aDataChannel)
    , mBinaryType(DC_BINARY_TYPE_BLOB)
  {}

  ~nsDOMDataChannel();

  nsresult Init(nsPIDOMWindow* aDOMWindow);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDATACHANNEL

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMDataChannel,
                                           nsDOMEventTargetHelper)

  nsresult
  DoOnMessageAvailable(const nsACString& aMessage, bool aBinary);

  virtual nsresult
  OnMessageAvailable(nsISupports* aContext, const nsACString& aMessage);

  virtual nsresult
  OnBinaryMessageAvailable(nsISupports* aContext, const nsACString& aMessage);

  virtual nsresult OnSimpleEvent(nsISupports* aContext, const nsAString& aName);

  virtual nsresult
  OnChannelConnected(nsISupports* aContext);

  virtual nsresult
  OnChannelClosed(nsISupports* aContext);

  virtual void
  AppReady();

private:
  
  nsresult GetSendParams(nsIVariant *aData, nsCString &aStringOut,
                         nsCOMPtr<nsIInputStream> &aStreamOut,
                         bool &aIsBinary, uint32_t &aOutgoingLength);

  
  nsRefPtr<mozilla::DataChannel> mDataChannel;
  nsString  mOrigin;
  enum
  {
    DC_BINARY_TYPE_ARRAYBUFFER,
    DC_BINARY_TYPE_BLOB,
  } mBinaryType;
};

#endif 
