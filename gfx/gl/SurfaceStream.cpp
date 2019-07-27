




#include "SurfaceStream.h"

#include "gfxPoint.h"
#include "SharedSurface.h"
#include "SharedSurfaceGL.h"
#include "GeckoProfiler.h"
#include "mozilla/Move.h"

namespace mozilla {
namespace gl {

SurfaceStreamType
SurfaceStream::ChooseGLStreamType(SurfaceStream::OMTC omtc,
                                  bool preserveBuffer)
{
    if (omtc == SurfaceStream::OffMainThread) {
        if (preserveBuffer)
            return SurfaceStreamType::TripleBuffer_Copy;
        else
            return SurfaceStreamType::TripleBuffer_Async;
    } else {
        if (preserveBuffer)
            return SurfaceStreamType::SingleBuffer;
        else
            return SurfaceStreamType::TripleBuffer;
    }
}

TemporaryRef<SurfaceStream>
SurfaceStream::CreateForType(SurfaceStreamType type, mozilla::gl::GLContext* glContext, SurfaceStream* prevStream)
{
    RefPtr<SurfaceStream> result;

    switch (type) {
        case SurfaceStreamType::SingleBuffer:
            result = new SurfaceStream_SingleBuffer(prevStream);
            break;
        case SurfaceStreamType::TripleBuffer_Copy:
            result = new SurfaceStream_TripleBuffer_Copy(prevStream);
            break;
        case SurfaceStreamType::TripleBuffer_Async:
            result = new SurfaceStream_TripleBuffer_Async(prevStream);
            break;
        case SurfaceStreamType::TripleBuffer:
            result = new SurfaceStream_TripleBuffer(prevStream);
            break;
        default:
            MOZ_CRASH("Invalid Type.");
    }

    result->mGLContext = glContext;

    return result.forget();
}

bool
SurfaceStream_TripleBuffer::CopySurfaceToProducer(SharedSurface* src, SurfaceFactory* factory)
{
    if (!mProducer) {
        New(factory, src->mSize, &mProducer);
        if (!mProducer) {
            return false;
        }
    }

    MOZ_ASSERT(src->mSize == mProducer->mSize, "Size mismatch");

    SharedSurface::ProdCopy(src, mProducer.get(), factory);
    return true;
}

void
SurfaceStream::New(SurfaceFactory* factory, const gfx::IntSize& size,
                   UniquePtr<SharedSurface>* surfSlot)
{
    MOZ_ASSERT(surfSlot);
    UniquePtr<SharedSurface>& surf = *surfSlot;

    MOZ_ASSERT(!surf);
    surf = factory->NewSharedSurface(size);

    if (surf) {
        
        
        surf->WaitForBufferOwnership();
#ifdef DEBUG
        mSurfaces.insert(surf.get());
#endif
    }
}

void
SurfaceStream::MoveTo(UniquePtr<SharedSurface>* slotFrom,
                      UniquePtr<SharedSurface>* slotTo)
{
    MOZ_ASSERT(slotFrom);
    UniquePtr<SharedSurface>& from = *slotFrom;

    MOZ_ASSERT(slotTo);
    UniquePtr<SharedSurface>& to = *slotTo;

    MOZ_ASSERT(!to);
    to = Move(from);
    from = nullptr;
}

void
SurfaceStream::Recycle(SurfaceFactory* factory, UniquePtr<SharedSurface>* surfSlot)
{
    MOZ_ASSERT(surfSlot);
    UniquePtr<SharedSurface>& surf = *surfSlot;

    if (surf) {
#ifdef DEBUG
        mSurfaces.erase(surf.get());
#endif
        factory->Recycle(Move(surf));
        surf = nullptr;
    }
    MOZ_ASSERT(!surf);
}

void
SurfaceStream::Delete(UniquePtr<SharedSurface>* surfSlot)
{
    MOZ_ASSERT(surfSlot);
    UniquePtr<SharedSurface>& surf = *surfSlot;

    if (surf) {
#ifdef DEBUG
        mSurfaces.erase(surf.get());
#endif
        surf = nullptr;
    }
    MOZ_ASSERT(!surf);
}

UniquePtr<SharedSurface>
SurfaceStream::Surrender(UniquePtr<SharedSurface>* surfSlot)
{
    MOZ_ASSERT(surfSlot);
    UniquePtr<SharedSurface>& surf = *surfSlot;

#ifdef DEBUG
    if (surf) {
        mSurfaces.erase(surf.get());
    }
#endif

    UniquePtr<SharedSurface> ret = Move(surf);
    surf = nullptr;

    return Move(ret);
}



UniquePtr<SharedSurface>
SurfaceStream::Absorb(UniquePtr<SharedSurface>* surfSlot)
{
    MOZ_ASSERT(surfSlot);
    UniquePtr<SharedSurface>& surf = *surfSlot;

#ifdef DEBUG
    if (surf) {
        mSurfaces.insert(surf.get());
    }
#endif

    UniquePtr<SharedSurface> ret = Move(surf);
    surf = nullptr;

    return Move(ret);
}

void
SurfaceStream::Scrap(UniquePtr<SharedSurface>* surfSlot)
{
    MOZ_ASSERT(surfSlot);
    UniquePtr<SharedSurface>& surf = *surfSlot;

    if (surf) {
        mScraps.push(Move(surf));
        surf = nullptr;
    }
    MOZ_ASSERT(!surf);

}

void
SurfaceStream::RecycleScraps(SurfaceFactory* factory)
{
    while (!mScraps.empty()) {
        UniquePtr<SharedSurface> cur = Move(mScraps.top());
        mScraps.pop();

        Recycle(factory, &cur);
    }
}




SurfaceStream::SurfaceStream(SurfaceStreamType type,
                             SurfaceStream* prevStream)
    : mType(type)
    , mProducer(nullptr)
    , mMonitor("SurfaceStream monitor")
    , mIsAlive(true)
{
    MOZ_ASSERT(!prevStream || mType != prevStream->mType,
               "We should not need to create a SurfaceStream from another "
               "of the same type.");
}

SurfaceStream::~SurfaceStream()
{
    Delete(&mProducer);

    while (!mScraps.empty()) {
        UniquePtr<SharedSurface> cur = Move(mScraps.top());
        mScraps.pop();

        Delete(&cur);
    }

    MOZ_ASSERT(mSurfaces.empty());
}

SharedSurface*
SurfaceStream::SwapConsumer()
{
    MOZ_ASSERT(mIsAlive);

    SharedSurface* ret = SwapConsumer_NoWait();
    if (!ret)
        return nullptr;

    if (!ret->WaitSync()) {
        return nullptr;
    }

    return ret;
}

SharedSurface*
SurfaceStream::Resize(SurfaceFactory* factory, const gfx::IntSize& size)
{
    MonitorAutoLock lock(mMonitor);

    if (mProducer) {
        Scrap(&mProducer);
    }

    New(factory, size, &mProducer);
    return mProducer.get();
}




SurfaceStream_SingleBuffer::SurfaceStream_SingleBuffer(SurfaceStream* prevStream)
    : SurfaceStream(SurfaceStreamType::SingleBuffer, prevStream)
    , mConsumer(nullptr)
{
    if (!prevStream)
        return;

    UniquePtr<SharedSurface> prevProducer;
    UniquePtr<SharedSurface> prevConsumer;
    prevStream->SurrenderSurfaces(&prevProducer, &prevConsumer);

    mProducer = Absorb(&prevProducer);
    mConsumer = Absorb(&prevConsumer);
}

SurfaceStream_SingleBuffer::~SurfaceStream_SingleBuffer()
{
    Delete(&mConsumer);
}

void
SurfaceStream_SingleBuffer::SurrenderSurfaces(UniquePtr<SharedSurface>* out_producer,
                                              UniquePtr<SharedSurface>* out_consumer)
{
    MOZ_ASSERT(out_producer);
    MOZ_ASSERT(out_consumer);

    mIsAlive = false;

    *out_producer = Surrender(&mProducer);
    *out_consumer = Surrender(&mConsumer);
}

SharedSurface*
SurfaceStream_SingleBuffer::SwapProducer(SurfaceFactory* factory,
                                         const gfx::IntSize& size)
{
    MonitorAutoLock lock(mMonitor);
    if (mConsumer) {
        Recycle(factory, &mConsumer);
    }

    if (mProducer) {
        
        mProducer->Fence();

        
        
        bool needsNewBuffer = mProducer->mSize != size;

        
        
        if (mProducer->mType != factory->mType &&
            !factory->mCaps.preserve)
        {
            needsNewBuffer = true;
        }

        if (needsNewBuffer) {
            MoveTo(&mProducer, &mConsumer);
        }
    }

    
    
    if (!mProducer) {
        New(factory, size, &mProducer);
    }

    return mProducer.get();
}

SharedSurface*
SurfaceStream_SingleBuffer::SwapConsumer_NoWait()
{
    MonitorAutoLock lock(mMonitor);

    
    
    SharedSurface* toConsume = mConsumer.get();
    if (!toConsume)
        toConsume = mProducer.get();

    return toConsume;
}




SurfaceStream_TripleBuffer_Copy::SurfaceStream_TripleBuffer_Copy(SurfaceStream* prevStream)
    : SurfaceStream(SurfaceStreamType::TripleBuffer_Copy, prevStream)
    , mStaging(nullptr)
    , mConsumer(nullptr)
{
    if (!prevStream)
        return;

    UniquePtr<SharedSurface> prevProducer;
    UniquePtr<SharedSurface> prevConsumer;
    prevStream->SurrenderSurfaces(&prevProducer, &prevConsumer);

    mProducer = Absorb(&prevProducer);
    mConsumer = Absorb(&prevConsumer);
}

SurfaceStream_TripleBuffer_Copy::~SurfaceStream_TripleBuffer_Copy()
{
    Delete(&mStaging);
    Delete(&mConsumer);
}

void
SurfaceStream_TripleBuffer_Copy::SurrenderSurfaces(UniquePtr<SharedSurface>* out_producer,
                                                   UniquePtr<SharedSurface>* out_consumer)
{
    MOZ_ASSERT(out_producer);
    MOZ_ASSERT(out_consumer);

    mIsAlive = false;

    *out_producer = Surrender(&mProducer);
    *out_consumer = Surrender(&mConsumer);

    if (!*out_consumer)
        *out_consumer = Surrender(&mStaging);
}

SharedSurface*
SurfaceStream_TripleBuffer_Copy::SwapProducer(SurfaceFactory* factory,
                                              const gfx::IntSize& size)
{
    MonitorAutoLock lock(mMonitor);

    RecycleScraps(factory);
    if (mProducer) {
        if (mStaging) {
            
            
            Recycle(factory, &mStaging);
        }

        MoveTo(&mProducer, &mStaging);
        mStaging->Fence();

        New(factory, size, &mProducer);

        if (mProducer &&
            mStaging->mSize == mProducer->mSize)
        {
            SharedSurface::ProdCopy(mStaging.get(), mProducer.get(), factory);
        }
    } else {
        New(factory, size, &mProducer);
    }

    return mProducer.get();
}

SharedSurface*
SurfaceStream_TripleBuffer_Copy::SwapConsumer_NoWait()
{
    MonitorAutoLock lock(mMonitor);

    if (mStaging) {
        Scrap(&mConsumer);
        MoveTo(&mStaging, &mConsumer);
    }

    return mConsumer.get();
}




void SurfaceStream_TripleBuffer::Init(SurfaceStream* prevStream)
{
    if (!prevStream)
        return;

    UniquePtr<SharedSurface> prevProducer;
    UniquePtr<SharedSurface> prevConsumer;
    prevStream->SurrenderSurfaces(&prevProducer, &prevConsumer);

    mProducer = Absorb(&prevProducer);
    mConsumer = Absorb(&prevConsumer);
}

SurfaceStream_TripleBuffer::SurfaceStream_TripleBuffer(SurfaceStreamType type,
                                                       SurfaceStream* prevStream)
    : SurfaceStream(type, prevStream)
    , mStaging(nullptr)
    , mConsumer(nullptr)
{
    SurfaceStream_TripleBuffer::Init(prevStream);
}

SurfaceStream_TripleBuffer::SurfaceStream_TripleBuffer(SurfaceStream* prevStream)
    : SurfaceStream(SurfaceStreamType::TripleBuffer, prevStream)
    , mStaging(nullptr)
    , mConsumer(nullptr)
{
    SurfaceStream_TripleBuffer::Init(prevStream);
}

SurfaceStream_TripleBuffer::~SurfaceStream_TripleBuffer()
{
    Delete(&mStaging);
    Delete(&mConsumer);
}

void
SurfaceStream_TripleBuffer::SurrenderSurfaces(UniquePtr<SharedSurface>* out_producer,
                                              UniquePtr<SharedSurface>* out_consumer)
{
    MOZ_ASSERT(out_producer);
    MOZ_ASSERT(out_consumer);

    mIsAlive = false;

    *out_producer = Surrender(&mProducer);
    *out_consumer = Surrender(&mConsumer);

    if (!*out_consumer)
        *out_consumer = Surrender(&mStaging);
}

SharedSurface*
SurfaceStream_TripleBuffer::SwapProducer(SurfaceFactory* factory,
                                         const gfx::IntSize& size)
{
    PROFILER_LABEL("SurfaceStream_TripleBuffer", "SwapProducer",
                   js::ProfileEntry::Category::GRAPHICS);

    MonitorAutoLock lock(mMonitor);
    if (mProducer) {
        RecycleScraps(factory);

        
        
        if (mStaging) {
            WaitForCompositor();
        }
        if (mStaging) {
            Scrap(&mStaging);
        }

        MoveTo(&mProducer, &mStaging);
        mStaging->Fence();
    }

    MOZ_ASSERT(!mProducer);
    New(factory, size, &mProducer);

    return mProducer.get();
}

SharedSurface*
SurfaceStream_TripleBuffer::SwapConsumer_NoWait()
{
    MonitorAutoLock lock(mMonitor);
    if (mStaging) {
        Scrap(&mConsumer);
        MoveTo(&mStaging, &mConsumer);
        mMonitor.NotifyAll();
    }

    return mConsumer.get();
}




SurfaceStream_TripleBuffer_Async::SurfaceStream_TripleBuffer_Async(SurfaceStream* prevStream)
    : SurfaceStream_TripleBuffer(SurfaceStreamType::TripleBuffer_Async,
                                 prevStream)
{
}

SurfaceStream_TripleBuffer_Async::~SurfaceStream_TripleBuffer_Async()
{
}

void
SurfaceStream_TripleBuffer_Async::WaitForCompositor()
{
    PROFILER_LABEL("SurfaceStream_TripleBuffer_Async",
                   "WaitForCompositor",
                   js::ProfileEntry::Category::GRAPHICS);

    
    
    
    mMonitor.Wait(PR_MillisecondsToInterval(100));
}

} 
} 
