




#ifndef SURFACESTREAM_H_
#define SURFACESTREAM_H_

#include <stack>
#include <set>
#include "mozilla/Monitor.h"
#include "mozilla/Attributes.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/GenericRefCounted.h"
#include "mozilla/UniquePtr.h"
#include "SurfaceTypes.h"
#include "SharedSurface.h"

namespace mozilla {

namespace gl {
class GLContext;
class SharedSurface;
class SurfaceFactory;


class SurfaceStream : public GenericAtomicRefCounted
{
public:
    MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(SurfaceStream)
    typedef enum {
        MainThread,
        OffMainThread
    } OMTC;

    static SurfaceStreamType ChooseGLStreamType(OMTC omtc,
                                                bool preserveBuffer);

    static TemporaryRef<SurfaceStream> CreateForType(SurfaceStreamType type,
                                                     mozilla::gl::GLContext* glContext,
                                                     SurfaceStream* prevStream = nullptr);

    const SurfaceStreamType mType;

    mozilla::gl::GLContext* GLContext() const { return mGLContext; }


protected:
    
    
    UniquePtr<SharedSurface> mProducer;
#ifdef DEBUG
    std::set<SharedSurface*> mSurfaces;
#endif
    std::stack< UniquePtr<SharedSurface> > mScraps;
    mutable Monitor mMonitor;
    bool mIsAlive;

    
    
    mozilla::gl::GLContext* mGLContext;

    
    
    SurfaceStream(SurfaceStreamType type, SurfaceStream* prevStream);

public:
    virtual ~SurfaceStream();

protected:
    
    
    
    void MoveTo(UniquePtr<SharedSurface>* slotFrom,
                UniquePtr<SharedSurface>* slotTo);
    void New(SurfaceFactory* factory, const gfx::IntSize& size,
             UniquePtr<SharedSurface>* surfSlot);
    void Delete(UniquePtr<SharedSurface>* surfSlot);
    void Recycle(SurfaceFactory* factory,
                 UniquePtr<SharedSurface>* surfSlot);

    
    UniquePtr<SharedSurface> Surrender(UniquePtr<SharedSurface>* surfSlot);
    
    UniquePtr<SharedSurface> Absorb(UniquePtr<SharedSurface>* surfSlot);

    
    
    
    void Scrap(UniquePtr<SharedSurface>* surfSlot);

    
    void RecycleScraps(SurfaceFactory* factory);

public:
    







    virtual SharedSurface* SwapProducer(SurfaceFactory* factory,
                                        const gfx::IntSize& size) = 0;

    virtual SharedSurface* Resize(SurfaceFactory* factory, const gfx::IntSize& size);

    virtual bool CopySurfaceToProducer(SharedSurface* src, SurfaceFactory* factory) { MOZ_ASSERT(0); return false; }

protected:
    
    
    virtual SharedSurface* SwapConsumer_NoWait() = 0;

public:
    virtual SharedSurface* SwapConsumer();

    virtual void SurrenderSurfaces(UniquePtr<SharedSurface>* out_producer,
                                   UniquePtr<SharedSurface>* out_consumer) = 0;
};


class SurfaceStream_SingleBuffer
    : public SurfaceStream
{
protected:
    UniquePtr<SharedSurface> mConsumer; 

public:
    MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(SurfaceStream_SingleBuffer)

    explicit SurfaceStream_SingleBuffer(SurfaceStream* prevStream);
    virtual ~SurfaceStream_SingleBuffer();

    



    virtual SharedSurface* SwapProducer(SurfaceFactory* factory,
                                        const gfx::IntSize& size) MOZ_OVERRIDE;

    virtual SharedSurface* SwapConsumer_NoWait() MOZ_OVERRIDE;

    virtual void SurrenderSurfaces(UniquePtr<SharedSurface>* out_producer,
                                   UniquePtr<SharedSurface>* out_consumer) MOZ_OVERRIDE;
};


class SurfaceStream_TripleBuffer_Copy
    : public SurfaceStream
{
protected:
    UniquePtr<SharedSurface> mStaging;
    UniquePtr<SharedSurface> mConsumer;

public:
    MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(SurfaceStream_TripleBuffer_Copy)

    explicit SurfaceStream_TripleBuffer_Copy(SurfaceStream* prevStream);
    virtual ~SurfaceStream_TripleBuffer_Copy();

    virtual SharedSurface* SwapProducer(SurfaceFactory* factory,
                                        const gfx::IntSize& size) MOZ_OVERRIDE;

    virtual SharedSurface* SwapConsumer_NoWait() MOZ_OVERRIDE;

    virtual void SurrenderSurfaces(UniquePtr<SharedSurface>* out_producer,
                                   UniquePtr<SharedSurface>* out_consumer) MOZ_OVERRIDE;
};


class SurfaceStream_TripleBuffer
    : public SurfaceStream
{
protected:
    UniquePtr<SharedSurface> mStaging;
    UniquePtr<SharedSurface> mConsumer;

    
    virtual void WaitForCompositor() {}

    
    SurfaceStream_TripleBuffer(SurfaceStreamType type, SurfaceStream* prevStream);

public:
    MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(SurfaceStream_TripleBuffer)

    explicit SurfaceStream_TripleBuffer(SurfaceStream* prevStream);
    virtual ~SurfaceStream_TripleBuffer();

    virtual bool CopySurfaceToProducer(SharedSurface* src,
                                       SurfaceFactory* factory) MOZ_OVERRIDE;

private:
    
    void Init(SurfaceStream* prevStream);

public:
    
    virtual SharedSurface* SwapProducer(SurfaceFactory* factory,
                                        const gfx::IntSize& size) MOZ_OVERRIDE;

    virtual SharedSurface* SwapConsumer_NoWait() MOZ_OVERRIDE;

    virtual void SurrenderSurfaces(UniquePtr<SharedSurface>* out_producer,
                                   UniquePtr<SharedSurface>* out_consumer) MOZ_OVERRIDE;
};

class SurfaceStream_TripleBuffer_Async
    : public SurfaceStream_TripleBuffer
{
protected:
    virtual void WaitForCompositor() MOZ_OVERRIDE;

public:
    MOZ_DECLARE_REFCOUNTED_VIRTUAL_TYPENAME(SurfaceStream_TripleBuffer_Async)

    explicit SurfaceStream_TripleBuffer_Async(SurfaceStream* prevStream);
    virtual ~SurfaceStream_TripleBuffer_Async();
};


} 
} 

#endif 
