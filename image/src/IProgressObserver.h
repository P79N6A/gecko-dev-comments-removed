




#ifndef mozilla_image_src_IProgressObserver_h
#define mozilla_image_src_IProgressObserver_h

#include "mozilla/WeakPtr.h"
#include "nsISupports.h"
#include "nsRect.h"

namespace mozilla {
namespace image {













class IProgressObserver : public SupportsWeakPtr<IProgressObserver>
{
public:
  MOZ_DECLARE_WEAKREFERENCE_TYPENAME(IProgressObserver)

  
  
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void) = 0;
  NS_IMETHOD_(MozExternalRefCountType) Release(void) = 0;

  
  virtual void Notify(int32_t aType, const nsIntRect* aRect = nullptr) = 0;
  virtual void OnLoadComplete(bool aLastPart) = 0;

  
  virtual void BlockOnload() = 0;
  virtual void UnblockOnload() = 0;

  
  virtual void SetHasImage() = 0;
  virtual void OnStartDecode() = 0;
  virtual bool NotificationsDeferred() const = 0;
  virtual void SetNotificationsDeferred(bool aDeferNotifications) = 0;

protected:
  virtual ~IProgressObserver() { }
};

} 
} 

#endif 
