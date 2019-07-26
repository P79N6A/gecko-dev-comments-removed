




#if !defined(DXVA2Manager_h_)
#define DXVA2Manager_h_

#include "WMF.h"
#include "nsAutoPtr.h"
#include "mozilla/Mutex.h"
#include "nsRect.h"

class nsIntSize;

namespace mozilla {

namespace layers {
class Image;
class ImageContainer;
}

class DXVA2Manager {
public:

  
  
  static DXVA2Manager* CreateD3D9DXVA();
  static DXVA2Manager* CreateD3D11DXVA();

  
  
  
  
  virtual IUnknown* GetDXVADeviceManager() = 0;

  
  virtual HRESULT CopyToImage(IMFSample* aVideoSample,
                              const nsIntRect& aRegion,
                              layers::ImageContainer* aContainer,
                              layers::Image** aOutImage) = 0;

  virtual ~DXVA2Manager();

protected:
  Mutex mLock;
  DXVA2Manager();
};

} 

#endif 
