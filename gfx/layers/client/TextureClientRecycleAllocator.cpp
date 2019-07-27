




#include <map>
#include <stack>

#include "gfxPlatform.h"
#include "mozilla/layers/GrallocTextureClient.h"
#include "mozilla/layers/ISurfaceAllocator.h"
#include "mozilla/Mutex.h"

#include "TextureClientRecycleAllocator.h"

namespace mozilla {
namespace layers {

class TextureClientRecycleAllocatorImp : public ISurfaceAllocator
{
  ~TextureClientRecycleAllocatorImp();

public:
  explicit TextureClientRecycleAllocatorImp(ISurfaceAllocator* aAllocator);

  void SetMaxPoolSize(uint32_t aMax)
  {
    if (aMax > 0) {
      mMaxPooledSize = aMax;
    }
  }

  
  TemporaryRef<TextureClient>
  CreateOrRecycleForDrawing(gfx::SurfaceFormat aFormat,
                            gfx::IntSize aSize,
                            gfx::BackendType aMoz2dBackend,
                            TextureFlags aTextureFlags,
                            TextureAllocationFlags flags);

  void Destroy();

  void RecycleCallbackImp(TextureClient* aClient);

  static void RecycleCallback(TextureClient* aClient, void* aClosure);

  
  virtual LayersBackend GetCompositorBackendType() const override
  {
    return mSurfaceAllocator->GetCompositorBackendType();
  }

  virtual bool AllocShmem(size_t aSize,
                          mozilla::ipc::SharedMemory::SharedMemoryType aType,
                          mozilla::ipc::Shmem* aShmem) override
  {
    return mSurfaceAllocator->AllocShmem(aSize, aType, aShmem);
  }

  virtual bool AllocUnsafeShmem(size_t aSize,
                                mozilla::ipc::SharedMemory::SharedMemoryType aType,
                                mozilla::ipc::Shmem* aShmem) override
  {
    return mSurfaceAllocator->AllocUnsafeShmem(aSize, aType, aShmem);
  }

  virtual void DeallocShmem(mozilla::ipc::Shmem& aShmem) override
  {
    mSurfaceAllocator->DeallocShmem(aShmem);
  }

  virtual bool IsSameProcess() const override
  {
    return mSurfaceAllocator->IsSameProcess();
  }

  virtual MessageLoop * GetMessageLoop() const override
  {
    return mSurfaceAllocator->GetMessageLoop();
  }

protected:
  
  virtual bool IsOnCompositorSide() const override
  {
    return false;
  }

private:
  static const uint32_t kMaxPooledSized = 2;

  
  class TextureClientHolder
  {
    ~TextureClientHolder() {}
  public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TextureClientHolder)

    explicit TextureClientHolder(TextureClient* aClient)
      : mTextureClient(aClient)
    {}

    TextureClient* GetTextureClient()
    {
      return mTextureClient;
    }
    void ClearTextureClient() { mTextureClient = nullptr; }
  protected:
    RefPtr<TextureClient> mTextureClient;
  };

  bool mDestroyed;
  uint32_t mMaxPooledSize;
  RefPtr<ISurfaceAllocator> mSurfaceAllocator;
  std::map<TextureClient*, RefPtr<TextureClientHolder> > mInUseClients;

  
  
  
  
  std::stack<RefPtr<TextureClientHolder> > mPooledClients;
  Mutex mLock;
};

TextureClientRecycleAllocatorImp::TextureClientRecycleAllocatorImp(ISurfaceAllocator *aAllocator)
  : mDestroyed(false)
  , mMaxPooledSize(kMaxPooledSized)
  , mSurfaceAllocator(aAllocator)
  , mLock("TextureClientRecycleAllocatorImp.mLock")
{
}

TextureClientRecycleAllocatorImp::~TextureClientRecycleAllocatorImp()
{
  MOZ_ASSERT(mDestroyed);
  MOZ_ASSERT(mPooledClients.empty());
  MOZ_ASSERT(mInUseClients.empty());
}

TemporaryRef<TextureClient>
TextureClientRecycleAllocatorImp::CreateOrRecycleForDrawing(
                                             gfx::SurfaceFormat aFormat,
                                             gfx::IntSize aSize,
                                             gfx::BackendType aMoz2DBackend,
                                             TextureFlags aTextureFlags,
                                             TextureAllocationFlags aAllocFlags)
{
  
  
  MOZ_ASSERT(aAllocFlags == TextureAllocationFlags::ALLOC_DEFAULT ||
             aAllocFlags == TextureAllocationFlags::ALLOC_DISALLOW_BUFFERTEXTURECLIENT);
  MOZ_ASSERT(!(aTextureFlags & TextureFlags::RECYCLE));
  aTextureFlags = aTextureFlags | TextureFlags::RECYCLE; 

  RefPtr<TextureClientHolder> textureHolder;

  if (aMoz2DBackend == gfx::BackendType::NONE) {
    aMoz2DBackend = gfxPlatform::GetPlatform()->GetContentBackend();
  }

  {
    MutexAutoLock lock(mLock);
    if (mDestroyed) {
      return nullptr;
    } else if (!mPooledClients.empty()) {
      textureHolder = mPooledClients.top();
      mPooledClients.pop();
      
      if (textureHolder->GetTextureClient()->GetFormat() != aFormat ||
          textureHolder->GetTextureClient()->GetSize() != aSize)
      {
        TextureClientReleaseTask* task = new TextureClientReleaseTask(textureHolder->GetTextureClient());
        textureHolder->ClearTextureClient();
        textureHolder = nullptr;
        
        mSurfaceAllocator->GetMessageLoop()->PostTask(FROM_HERE, task);
      } else {
        textureHolder->GetTextureClient()->RecycleTexture(aTextureFlags);
      }
    }
  }

  if (!textureHolder) {
    
    RefPtr<TextureClient> texture;
    texture = TextureClient::CreateForDrawing(this, aFormat, aSize, aMoz2DBackend,
                                              aTextureFlags, aAllocFlags);
    if (!texture) {
      return nullptr;
    }
    textureHolder = new TextureClientHolder(texture);
  }

  {
    MutexAutoLock lock(mLock);
    MOZ_ASSERT(mInUseClients.find(textureHolder->GetTextureClient()) == mInUseClients.end());
    
    mInUseClients[textureHolder->GetTextureClient()] = textureHolder;
  }
  textureHolder->GetTextureClient()->SetRecycleCallback(TextureClientRecycleAllocatorImp::RecycleCallback, this);
  return textureHolder->GetTextureClient();
}

void
TextureClientRecycleAllocatorImp::Destroy()
{
  MutexAutoLock lock(mLock);
  if (mDestroyed) {
    return;
  }
  mDestroyed = true;
  while (!mPooledClients.empty()) {
    mPooledClients.pop();
  }
}

void
TextureClientRecycleAllocatorImp::RecycleCallbackImp(TextureClient* aClient)
{
  RefPtr<TextureClientHolder> textureHolder;
  aClient->ClearRecycleCallback();
  {
    MutexAutoLock lock(mLock);
    if (mInUseClients.find(aClient) != mInUseClients.end()) {
      textureHolder = mInUseClients[aClient]; 
      if (!mDestroyed && mPooledClients.size() < mMaxPooledSize) {
        mPooledClients.push(textureHolder);
      }
      mInUseClients.erase(aClient);
    }
  }
}

 void
TextureClientRecycleAllocatorImp::RecycleCallback(TextureClient* aClient, void* aClosure)
{
  TextureClientRecycleAllocatorImp* recycleAllocator = static_cast<TextureClientRecycleAllocatorImp*>(aClosure);
  recycleAllocator->RecycleCallbackImp(aClient);
}

TextureClientRecycleAllocator::TextureClientRecycleAllocator(ISurfaceAllocator *aAllocator)
{
  mAllocator = new TextureClientRecycleAllocatorImp(aAllocator);
}

TextureClientRecycleAllocator::~TextureClientRecycleAllocator()
{
  mAllocator->Destroy();
  mAllocator = nullptr;
}

void
TextureClientRecycleAllocator::SetMaxPoolSize(uint32_t aMax)
{
  mAllocator->SetMaxPoolSize(aMax);
}

TemporaryRef<TextureClient>
TextureClientRecycleAllocator::CreateOrRecycleForDrawing(
                                            gfx::SurfaceFormat aFormat,
                                            gfx::IntSize aSize,
                                            gfx::BackendType aMoz2DBackend,
                                            TextureFlags aTextureFlags,
                                            TextureAllocationFlags aAllocFlags)
{
  return mAllocator->CreateOrRecycleForDrawing(aFormat,
                                               aSize,
                                               aMoz2DBackend,
                                               aTextureFlags,
                                               aAllocFlags);
}

}
}
