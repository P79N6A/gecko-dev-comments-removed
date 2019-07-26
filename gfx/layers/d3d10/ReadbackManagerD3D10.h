




#ifndef GFX_READBACKMANAGERD3D10_H
#define GFX_READBACKMANAGERD3D10_H

#include <windows.h>
#include <d3d10_1.h>

#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "gfxPoint.h"

namespace mozilla {
namespace layers {

DWORD WINAPI StartTaskThread(void *aManager);

struct ReadbackTask;

class ReadbackManagerD3D10 MOZ_FINAL : public IUnknown
{
public:
  ReadbackManagerD3D10();
  ~ReadbackManagerD3D10();

  










  void PostTask(ID3D10Texture2D *aTexture, void *aUpdate, const gfxPoint &aOrigin);

  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                                   void **ppvObject);
  virtual ULONG STDMETHODCALLTYPE AddRef(void);
  virtual ULONG STDMETHODCALLTYPE Release(void);

private:
  friend DWORD WINAPI StartTaskThread(void *aManager);

  void ProcessTasks();

  
  
  
  
  
  
  HANDLE mTaskSemaphore;
  
  HANDLE mShutdownEvent;
  
  HANDLE mTaskThread;

  
  
  CRITICAL_SECTION mTaskMutex;
  nsTArray<nsAutoPtr<ReadbackTask>> mPendingReadbackTasks;

  ULONG mRefCnt;
};

}
}

#endif 
