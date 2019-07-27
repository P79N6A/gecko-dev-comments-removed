




#ifndef GFX_READBACKMANAGERD3D11_H
#define GFX_READBACKMANAGERD3D11_H

#include <windows.h>
#include <d3d10_1.h>

#include "nsTArray.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace layers {

class TextureReadbackSink;
struct ReadbackTask;

class ReadbackManagerD3D11 MOZ_FINAL
{
  NS_INLINE_DECL_REFCOUNTING(ReadbackManagerD3D11)
public:
  ReadbackManagerD3D11();

  







  void PostTask(ID3D10Texture2D* aTexture, TextureReadbackSink* aSink);

private:
  ~ReadbackManagerD3D11();

  friend DWORD WINAPI StartTaskThread(void *aManager);

  void ProcessTasks();

  
  
  
  
  
  
  HANDLE mTaskSemaphore;
  
  HANDLE mShutdownEvent;
  
  HANDLE mTaskThread;

  
  
  CRITICAL_SECTION mTaskMutex;
  nsTArray<nsAutoPtr<ReadbackTask>> mPendingReadbackTasks;
};

}
}

#endif 
