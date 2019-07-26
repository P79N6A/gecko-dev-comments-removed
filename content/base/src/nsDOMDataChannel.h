





#ifndef nsDOMDataChannel_h
#define nsDOMDataChannel_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/DataChannelBinding.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/net/DataChannelListener.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMDataChannel.h"
#include "nsIInputStream.h"


namespace mozilla {
class DataChannel;
};

class nsDOMDataChannel : public nsDOMEventTargetHelper,
                         public nsIDOMDataChannel,
                         public mozilla::DataChannelListener
{
public:
  nsDOMDataChannel(already_AddRefed<mozilla::DataChannel> aDataChannel);
  ~nsDOMDataChannel();

  nsresult Init(nsPIDOMWindow* aDOMWindow);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMDATACHANNEL

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsDOMDataChannel,
                                           nsDOMEventTargetHelper)

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
    MOZ_OVERRIDE;
  nsPIDOMWindow* GetParentObject() const
  {
    return GetOwner();
  }

  
  
  bool Reliable() const;
  mozilla::dom::RTCDataChannelState ReadyState() const;
  uint32_t BufferedAmount() const;
  IMPL_EVENT_HANDLER(open)
  IMPL_EVENT_HANDLER(error)
  IMPL_EVENT_HANDLER(close)
  
  IMPL_EVENT_HANDLER(message)
  mozilla::dom::RTCDataChannelType BinaryType() const
  {
    return static_cast<mozilla::dom::RTCDataChannelType>(
      static_cast<int>(mBinaryType));
  }
  void SetBinaryType(mozilla::dom::RTCDataChannelType aType)
  {
    mBinaryType = static_cast<DataChannelBinaryType>(
      static_cast<int>(aType));
  }
  void Send(const nsAString& aData, mozilla::ErrorResult& aRv);
  void Send(nsIDOMBlob* aData, mozilla::ErrorResult& aRv);
  void Send(const mozilla::dom::ArrayBuffer& aData, mozilla::ErrorResult& aRv);
  void Send(const mozilla::dom::ArrayBufferView& aData,
            mozilla::ErrorResult& aRv);

  
  bool Ordered() const;
  uint16_t Id() const;
  uint16_t Stream() const; 

  nsresult
  DoOnMessageAvailable(const nsACString& aMessage, bool aBinary);

  virtual nsresult
  OnMessageAvailable(nsISupports* aContext, const nsACString& aMessage) MOZ_OVERRIDE;

  virtual nsresult
  OnBinaryMessageAvailable(nsISupports* aContext, const nsACString& aMessage) MOZ_OVERRIDE;

  virtual nsresult OnSimpleEvent(nsISupports* aContext, const nsAString& aName);

  virtual nsresult
  OnChannelConnected(nsISupports* aContext) MOZ_OVERRIDE;

  virtual nsresult
  OnChannelClosed(nsISupports* aContext) MOZ_OVERRIDE;

  virtual void
  AppReady();

private:
  void Send(nsIInputStream* aMsgStream, const nsACString& aMsgString,
            uint32_t aMsgLength, bool aIsBinary, mozilla::ErrorResult& aRv);

  
  nsRefPtr<mozilla::DataChannel> mDataChannel;
  nsString  mOrigin;
  enum DataChannelBinaryType {
    DC_BINARY_TYPE_ARRAYBUFFER,
    DC_BINARY_TYPE_BLOB,
  };
  DataChannelBinaryType mBinaryType;
};

#endif 
