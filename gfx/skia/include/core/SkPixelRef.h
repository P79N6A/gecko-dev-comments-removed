








#ifndef SkPixelRef_DEFINED
#define SkPixelRef_DEFINED

#include "SkBitmap.h"
#include "SkRefCnt.h"
#include "SkString.h"

class SkColorTable;
struct SkIRect;
class SkMutex;
class SkFlattenableReadBuffer;
class SkFlattenableWriteBuffer;


class SkGpuTexture;

#if SK_ALLOW_STATIC_GLOBAL_INITIALIZERS

#define SK_DECLARE_PIXEL_REF_REGISTRAR() 

#define SK_DEFINE_PIXEL_REF_REGISTRAR(pixelRef) \
    static SkPixelRef::Registrar g##pixelRef##Reg(#pixelRef, \
                                                  pixelRef::Create);
                                                      
#else

#define SK_DECLARE_PIXEL_REF_REGISTRAR() static void Init();

#define SK_DEFINE_PIXEL_REF_REGISTRAR(pixelRef) \
    void pixelRef::Init() { \
        SkPixelRef::Registrar(#pixelRef, Create); \
    }

#endif









class SkPixelRef : public SkRefCnt {
public:
    explicit SkPixelRef(SkMutex* mutex = NULL);

    


    void* pixels() const { return fPixels; }

    

    SkColorTable* colorTable() const { return fColorTable; }

    

    int getLockCount() const { return fLockCount; }

    


    void lockPixels();
    




    void unlockPixels();

    





    bool lockPixelsAreWritable() const;

    



    uint32_t getGenerationID() const;

    



    void notifyPixelsChanged();

    


    bool isImmutable() const { return fIsImmutable; }

    



    void setImmutable();

    


    const char* getURI() const { return fURI.size() ? fURI.c_str() : NULL; }

    

    void setURI(const char uri[]) {
        fURI.set(uri);
    }

    

    void setURI(const char uri[], size_t len) {
        fURI.set(uri, len);
    }

    

    void setURI(const SkString& uri) { fURI = uri; }

    

    virtual SkGpuTexture* getTexture() { return NULL; }

    bool readPixels(SkBitmap* dst, const SkIRect* subset = NULL);

    



    virtual SkPixelRef* deepCopy(SkBitmap::Config config) { return NULL; }

    

    typedef SkPixelRef* (*Factory)(SkFlattenableReadBuffer&);

    virtual Factory getFactory() const { return NULL; }
    virtual void flatten(SkFlattenableWriteBuffer&) const;

#ifdef SK_BUILD_FOR_ANDROID
    




    virtual void globalRef(void* data=NULL);

    




    virtual void globalUnref();
#endif

    static Factory NameToFactory(const char name[]);
    static const char* FactoryToName(Factory);
    static void Register(const char name[], Factory);

    class Registrar {
    public:
        Registrar(const char name[], Factory factory) {
            SkPixelRef::Register(name, factory);
        }
    };

protected:
    


    virtual void* onLockPixels(SkColorTable**) = 0;
    



    virtual void onUnlockPixels() = 0;

    
    virtual bool onLockPixelsAreWritable() const;

    





    virtual bool onReadPixels(SkBitmap* dst, const SkIRect* subsetOrNull);

    


    SkMutex* mutex() const { return fMutex; }

    SkPixelRef(SkFlattenableReadBuffer&, SkMutex*);

private:
#if !SK_ALLOW_STATIC_GLOBAL_INITIALIZERS
    static void InitializeFlattenables();
#endif

    SkMutex*        fMutex; 
    void*           fPixels;
    SkColorTable*   fColorTable;    
    int             fLockCount;

    mutable uint32_t fGenerationID;

    SkString    fURI;

    
    bool    fIsImmutable;

    friend class SkGraphics;
};

#endif
