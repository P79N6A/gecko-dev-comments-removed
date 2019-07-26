




#ifndef SURFACESTREAM_H_
#define SURFACESTREAM_H_

#include <stack>
#include <set>
#include "mozilla/Monitor.h"
#include "mozilla/Attributes.h"
#include "gfxPoint.h"
#include "SurfaceTypes.h"

namespace mozilla {
namespace gfx {
class SharedSurface;
class SurfaceFactory;


class SurfaceStream
{
public:
    typedef enum {
        MainThread,
        OffMainThread
    } OMTC;

    static SurfaceStreamType ChooseGLStreamType(OMTC omtc,
                                                bool preserveBuffer);

    static SurfaceStream* CreateForType(SurfaceStreamType type,
                                        SurfaceStream* prevStream = nullptr);

    SurfaceStreamHandle GetShareHandle() {
        return reinterpret_cast<SurfaceStreamHandle>(this);
    }

    static SurfaceStream* FromHandle(SurfaceStreamHandle handle) {
        return (SurfaceStream*)handle;
    }

    const SurfaceStreamType mType;
protected:
    
    
    SharedSurface* mProducer;
    std::set<SharedSurface*> mSurfaces;
    std::stack<SharedSurface*> mScraps;
    mutable Monitor mMonitor;
    bool mIsAlive;

    
    
    SurfaceStream(SurfaceStreamType type, SurfaceStream* prevStream)
        : mType(type)
        , mProducer(nullptr)
        , mMonitor("SurfaceStream monitor")
        , mIsAlive(true)
    {
        MOZ_ASSERT(!prevStream || mType != prevStream->mType,
                   "We should not need to create a SurfaceStream from another "
                   "of the same type.");
    }

public:
    virtual ~SurfaceStream();

protected:
    
    
    
    static void Move(SharedSurface*& from, SharedSurface*& to) {
        MOZ_ASSERT(!to);
        to = from;
        from = nullptr;
    }

    void New(SurfaceFactory* factory, const gfxIntSize& size,
             SharedSurface*& surf);
    void Delete(SharedSurface*& surf);
    void Recycle(SurfaceFactory* factory, SharedSurface*& surf);

    
    SharedSurface* Surrender(SharedSurface*& surf);
    
    SharedSurface* Absorb(SharedSurface*& surf);

    
    
    
    void Scrap(SharedSurface*& scrap);

    
    void RecycleScraps(SurfaceFactory* factory);

public:
    







    virtual SharedSurface* SwapProducer(SurfaceFactory* factory,
                                        const gfxIntSize& size) = 0;

    virtual SharedSurface* Resize(SurfaceFactory* factory, const gfxIntSize& size);

protected:
    
    
    virtual SharedSurface* SwapConsumer_NoWait() = 0;

public:
    virtual SharedSurface* SwapConsumer();

    virtual void SurrenderSurfaces(SharedSurface*& producer, SharedSurface*& consumer) = 0;
};


class SurfaceStream_SingleBuffer
    : public SurfaceStream
{
protected:
    SharedSurface* mConsumer; 

public:
    SurfaceStream_SingleBuffer(SurfaceStream* prevStream);
    virtual ~SurfaceStream_SingleBuffer();

    



    virtual SharedSurface* SwapProducer(SurfaceFactory* factory,
                                        const gfxIntSize& size);

    virtual SharedSurface* SwapConsumer_NoWait();

    virtual void SurrenderSurfaces(SharedSurface*& producer, SharedSurface*& consumer);
};


class SurfaceStream_TripleBuffer_Copy
    : public SurfaceStream
{
protected:
    SharedSurface* mStaging;
    SharedSurface* mConsumer;

public:
    SurfaceStream_TripleBuffer_Copy(SurfaceStream* prevStream);
    virtual ~SurfaceStream_TripleBuffer_Copy();

    virtual SharedSurface* SwapProducer(SurfaceFactory* factory,
                                        const gfxIntSize& size);

    virtual SharedSurface* SwapConsumer_NoWait();

    virtual void SurrenderSurfaces(SharedSurface*& producer, SharedSurface*& consumer);
};


class SurfaceStream_TripleBuffer
    : public SurfaceStream
{
protected:
    SharedSurface* mStaging;
    SharedSurface* mConsumer;

    
    virtual bool WaitForCompositor() { return false; }

public:
    SurfaceStream_TripleBuffer(SurfaceStream* prevStream);
    virtual ~SurfaceStream_TripleBuffer();

    
    virtual SharedSurface* SwapProducer(SurfaceFactory* factory,
                                        const gfxIntSize& size);

    virtual SharedSurface* SwapConsumer_NoWait();

    virtual void SurrenderSurfaces(SharedSurface*& producer, SharedSurface*& consumer);
};

class SurfaceStream_TripleBuffer_Async
    : public SurfaceStream_TripleBuffer
{
protected:
    virtual bool WaitForCompositor();

public:
    SurfaceStream_TripleBuffer_Async(SurfaceStream* prevStream);
    virtual ~SurfaceStream_TripleBuffer_Async();
};


} 
} 

#endif 
