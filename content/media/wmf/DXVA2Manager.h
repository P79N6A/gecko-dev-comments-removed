




#if !defined(DXVA2Manager_h_)
#define DXVA2Manager_h_

#include "WMF.h"
#include "nsAutoPtr.h"
#include "mozilla/Mutex.h"

struct nsIntSize;
struct nsIntRect;

namespace mozilla {

namespace layers {
class Image;
class ImageContainer;
}

class DXVA2Manager {
public:

  
  
  
  static DXVA2Manager* Create();

  
  
  
  
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
