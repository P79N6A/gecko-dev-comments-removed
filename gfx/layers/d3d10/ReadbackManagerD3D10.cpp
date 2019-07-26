




#include "ReadbackManagerD3D10.h"
#include "ReadbackProcessor.h"
#include "ReadbackLayer.h"

#include "nsIThread.h"
#include "nsThreadUtils.h"
#include "gfxImageSurface.h"
#include "gfxContext.h"

namespace mozilla {
namespace layers {





struct ReadbackTask {
  
  nsRefPtr<ID3D10Texture2D> mReadbackTexture;
  
  
  
  nsRefPtr<ReadbackLayer> mLayer;
  ReadbackProcessor::Update mUpdate;
  
  gfxPoint mOrigin;
  
  
  
  nsIntPoint mBackgroundOffset;
};



class ReadbackResultWriter MOZ_FINAL : public nsIRunnable
{
  NS_DECL_THREADSAFE_ISUPPORTS
public:
  ReadbackResultWriter(ReadbackTask *aTask) : mTask(aTask) {}

  NS_IMETHODIMP Run()
  {
    ReadbackProcessor::Update *update = &mTask->mUpdate;

    if (!update->mLayer->GetSink()) {
      
      return NS_OK;
    }

    nsIntPoint offset = mTask->mBackgroundOffset;

    D3D10_TEXTURE2D_DESC desc;
    mTask->mReadbackTexture->GetDesc(&desc);

    D3D10_MAPPED_TEXTURE2D mappedTex;
    
    
    HRESULT hr = mTask->mReadbackTexture->Map(0, D3D10_MAP_READ, 0, &mappedTex);

    if (FAILED(hr)) {
      
      update->mLayer->GetSink()->SetUnknown(update->mSequenceCounter);
      return NS_OK;
    }

    nsRefPtr<gfxImageSurface> sourceSurface =
      new gfxImageSurface((unsigned char*)mappedTex.pData,
                          gfxIntSize(desc.Width, desc.Height),
                          mappedTex.RowPitch,
                          gfxImageFormat::RGB24);

    nsRefPtr<gfxContext> ctx =
      update->mLayer->GetSink()->BeginUpdate(update->mUpdateRect + offset,
                                             update->mSequenceCounter);

    if (ctx) {
      ctx->Translate(gfxPoint(offset.x, offset.y));
      ctx->SetSource(sourceSurface, gfxPoint(mTask->mOrigin.x,
                                             mTask->mOrigin.y));
      ctx->Paint();

      update->mLayer->GetSink()->EndUpdate(ctx, update->mUpdateRect + offset);
    }

    mTask->mReadbackTexture->Unmap(0);

    return NS_OK;
  }

private:
  nsAutoPtr<ReadbackTask> mTask;
};

NS_IMPL_ISUPPORTS(ReadbackResultWriter, nsIRunnable)

DWORD WINAPI StartTaskThread(void *aManager)
{
  static_cast<ReadbackManagerD3D10*>(aManager)->ProcessTasks();

  return 0;
}

ReadbackManagerD3D10::ReadbackManagerD3D10()
  : mRefCnt(0)
{
  ::InitializeCriticalSection(&mTaskMutex);
  mShutdownEvent = ::CreateEventA(nullptr, FALSE, FALSE, nullptr);
  mTaskSemaphore = ::CreateSemaphoreA(nullptr, 0, 1000000, nullptr);
  mTaskThread = ::CreateThread(nullptr, 0, StartTaskThread, this, 0, 0);
}

ReadbackManagerD3D10::~ReadbackManagerD3D10()
{
  ::SetEvent(mShutdownEvent);

  
  
  DWORD result = ::WaitForSingleObject(mTaskThread, 5000);
  if (result != WAIT_TIMEOUT) {
    ::DeleteCriticalSection(&mTaskMutex);
    ::CloseHandle(mShutdownEvent);
    ::CloseHandle(mTaskSemaphore);
    ::CloseHandle(mTaskThread);
  } else {
    NS_RUNTIMEABORT("ReadbackManager: Task thread did not shutdown in 5 seconds.");
  }
}

void
ReadbackManagerD3D10::PostTask(ID3D10Texture2D *aTexture, void *aUpdate, const gfxPoint &aOrigin)
{
  ReadbackTask *task = new ReadbackTask;
  task->mReadbackTexture = aTexture;
  task->mUpdate = *static_cast<ReadbackProcessor::Update*>(aUpdate);
  task->mOrigin = aOrigin;
  task->mLayer = task->mUpdate.mLayer;
  task->mBackgroundOffset = task->mLayer->GetBackgroundLayerOffset();

  ::EnterCriticalSection(&mTaskMutex);
  mPendingReadbackTasks.AppendElement(task);
  ::LeaveCriticalSection(&mTaskMutex);

  ::ReleaseSemaphore(mTaskSemaphore, 1, nullptr);
}

HRESULT
ReadbackManagerD3D10::QueryInterface(REFIID riid, void **ppvObject)
{
  if (!ppvObject) {
    return E_POINTER;
  }

  if (riid == IID_IUnknown) {
    *ppvObject = this;
  } else {
    return E_NOINTERFACE;
  }

  return S_OK;
}

ULONG
ReadbackManagerD3D10::AddRef()
{
  NS_ASSERTION(NS_IsMainThread(),
    "ReadbackManagerD3D10 should only be refcounted on main thread.");
  return ++mRefCnt;
}

ULONG
ReadbackManagerD3D10::Release()
{
  NS_ASSERTION(NS_IsMainThread(),
    "ReadbackManagerD3D10 should only be refcounted on main thread.");
  ULONG newRefCnt = --mRefCnt;
  if (!newRefCnt) {
    mRefCnt++;
    delete this;
  }
  return newRefCnt;
}

void
ReadbackManagerD3D10::ProcessTasks()
{
  HANDLE handles[] = { mTaskSemaphore, mShutdownEvent };
  
  while (true) {
    DWORD result = ::WaitForMultipleObjects(2, handles, FALSE, INFINITE);
    if (result != WAIT_OBJECT_0) {
      return;
    }

    ::EnterCriticalSection(&mTaskMutex);
    if (mPendingReadbackTasks.Length() == 0) {
      NS_RUNTIMEABORT("Trying to read from an empty array, bad bad bad");
    }
    ReadbackTask *nextReadbackTask = mPendingReadbackTasks[0].forget();
    mPendingReadbackTasks.RemoveElementAt(0);
    ::LeaveCriticalSection(&mTaskMutex);

    
    
    D3D10_MAPPED_TEXTURE2D mappedTex;
    nextReadbackTask->mReadbackTexture->Map(0, D3D10_MAP_READ, 0, &mappedTex);
    nextReadbackTask->mReadbackTexture->Unmap(0);

    
    
    
    nsCOMPtr<nsIThread> thread = do_GetMainThread();
    thread->Dispatch(new ReadbackResultWriter(nextReadbackTask),
                     nsIEventTarget::DISPATCH_NORMAL);
  }
}

}
}
