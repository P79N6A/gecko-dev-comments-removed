



#include "mozilla/dom/MediaDevices.h"
#include "mozilla/dom/MediaStreamBinding.h"
#include "mozilla/dom/MediaDevicesBinding.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/MediaManager.h"
#include "nsIEventTarget.h"
#include "nsIScriptGlobalObject.h"
#include "nsPIDOMWindow.h"

namespace mozilla {
namespace dom {

class MediaDevices::GumResolver : public nsIDOMGetUserMediaSuccessCallback
{
public:
  NS_DECL_ISUPPORTS

  explicit GumResolver(Promise* aPromise) : mPromise(aPromise) {}

  NS_IMETHOD
  OnSuccess(nsISupports* aStream) override
  {
    nsRefPtr<DOMLocalMediaStream> stream = do_QueryObject(aStream);
    if (!stream) {
      return NS_ERROR_FAILURE;
    }
    mPromise->MaybeResolve(stream);
    return NS_OK;
  }

private:
  virtual ~GumResolver() {}
  nsRefPtr<Promise> mPromise;
};

class MediaDevices::EnumDevResolver : public nsIGetUserMediaDevicesSuccessCallback
{
public:
  NS_DECL_ISUPPORTS

  explicit EnumDevResolver(Promise* aPromise) : mPromise(aPromise) {}

  NS_IMETHOD
  OnSuccess(nsIVariant* aDevices) override
  {
    
    nsIID elementIID;
    uint16_t elementType;

    
    nsTArray<nsCOMPtr<nsIMediaDevice>> devices;
    
    {
      void* rawArray;
      uint32_t arrayLen;
      nsresult rv;
      rv = aDevices->GetAsArray(&elementType, &elementIID, &arrayLen, &rawArray);
      NS_ENSURE_SUCCESS(rv, rv);

      if (elementType != nsIDataType::VTYPE_INTERFACE) {
        NS_Free(rawArray);
        return NS_ERROR_FAILURE;
      }

      nsISupports **supportsArray = reinterpret_cast<nsISupports **>(rawArray);
      for (uint32_t i = 0; i < arrayLen; ++i) {
        nsCOMPtr<nsIMediaDevice> device(do_QueryInterface(supportsArray[i]));
        devices.AppendElement(device);
        NS_IF_RELEASE(supportsArray[i]); 
      }
      NS_Free(rawArray); 
    }
    nsTArray<nsRefPtr<MediaDeviceInfo>> infos;
    for (auto& device : devices) {
      nsString type;
      device->GetType(type);
      bool isVideo = type.EqualsLiteral("video");
      bool isAudio = type.EqualsLiteral("audio");
      if (isVideo || isAudio) {
        MediaDeviceKind kind = isVideo ?
            MediaDeviceKind::Videoinput : MediaDeviceKind::Audioinput;
        
        nsRefPtr<MediaDeviceInfo> info = new MediaDeviceInfo(nsString(), kind,
                                                             nsString());
        infos.AppendElement(info);
      }
    }
    mPromise->MaybeResolve(infos);
    return NS_OK;
  }

private:
  virtual ~EnumDevResolver() {}
  nsRefPtr<Promise> mPromise;
};

class MediaDevices::GumRejecter : public nsIDOMGetUserMediaErrorCallback
{
public:
  NS_DECL_ISUPPORTS

  explicit GumRejecter(Promise* aPromise) : mPromise(aPromise) {}

  NS_IMETHOD
  OnError(nsISupports* aError) override
  {
    nsRefPtr<MediaStreamError> error = do_QueryObject(aError);
    if (!error) {
      return NS_ERROR_FAILURE;
    }
    mPromise->MaybeReject(error);
    return NS_OK;
  }

private:
  virtual ~GumRejecter() {}
  nsRefPtr<Promise> mPromise;
};

NS_IMPL_ISUPPORTS(MediaDevices::GumResolver, nsIDOMGetUserMediaSuccessCallback)
NS_IMPL_ISUPPORTS(MediaDevices::EnumDevResolver, nsIGetUserMediaDevicesSuccessCallback)
NS_IMPL_ISUPPORTS(MediaDevices::GumRejecter, nsIDOMGetUserMediaErrorCallback)

already_AddRefed<Promise>
MediaDevices::GetUserMedia(const MediaStreamConstraints& aConstraints,
                           ErrorResult &aRv)
{
  nsPIDOMWindow* window = GetOwner();
  nsCOMPtr<nsIGlobalObject> go = do_QueryInterface(window);
  nsRefPtr<Promise> p = Promise::Create(go, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  nsRefPtr<GumResolver> resolver = new GumResolver(p);
  nsRefPtr<GumRejecter> rejecter = new GumRejecter(p);

  aRv = MediaManager::Get()->GetUserMedia(window, aConstraints,
                                          resolver, rejecter);
  return p.forget();
}

already_AddRefed<Promise>
MediaDevices::EnumerateDevices(ErrorResult &aRv)
{
  nsPIDOMWindow* window = GetOwner();
  nsCOMPtr<nsIGlobalObject> go = do_QueryInterface(window);
  nsRefPtr<Promise> p = Promise::Create(go, aRv);
  NS_ENSURE_TRUE(!aRv.Failed(), nullptr);

  nsRefPtr<EnumDevResolver> resolver = new EnumDevResolver(p);
  nsRefPtr<GumRejecter> rejecter = new GumRejecter(p);

  aRv = MediaManager::Get()->EnumerateDevices(window, resolver, rejecter);
  return p.forget();
}

NS_IMPL_ADDREF_INHERITED(MediaDevices, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(MediaDevices, DOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN(MediaDevices)
  NS_INTERFACE_MAP_ENTRY(MediaDevices)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

JSObject*
MediaDevices::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return MediaDevicesBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
