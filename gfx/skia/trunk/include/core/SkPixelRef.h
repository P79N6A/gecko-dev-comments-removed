






#ifndef SkPixelRef_DEFINED
#define SkPixelRef_DEFINED

#include "SkBitmap.h"
#include "SkDynamicAnnotations.h"
#include "SkRefCnt.h"
#include "SkString.h"
#include "SkImageInfo.h"
#include "SkSize.h"
#include "SkTDArray.h"



#ifdef SK_DEBUG
    










#endif

class SkColorTable;
class SkData;
struct SkIRect;
class SkMutex;

class GrTexture;









class SK_API SkPixelRef : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkPixelRef)

    explicit SkPixelRef(const SkImageInfo&);
    SkPixelRef(const SkImageInfo&, SkBaseMutex* mutex);
    virtual ~SkPixelRef();

    const SkImageInfo& info() const {
        return fInfo;
    }

    


    void* pixels() const { return fRec.fPixels; }

    

    SkColorTable* colorTable() const { return fRec.fColorTable; }

    size_t rowBytes() const { return fRec.fRowBytes; }

    



    struct LockRec {
        void*           fPixels;
        SkColorTable*   fColorTable;
        size_t          fRowBytes;

        void zero() { sk_bzero(this, sizeof(*this)); }

        bool isZero() const {
            return NULL == fPixels && NULL == fColorTable && 0 == fRowBytes;
        }
    };

    


    bool isLocked() const { return fLockCount > 0; }

    SkDEBUGCODE(int getLockCount() const { return fLockCount; })

    



    bool lockPixels();

    




    bool lockPixels(LockRec* rec);

    




    void unlockPixels();

    





    bool lockPixelsAreWritable() const;

    



    uint32_t getGenerationID() const;

    




    void notifyPixelsChanged();

    




    void changeAlphaType(SkAlphaType at);

    


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

    







    SkData* refEncodedData() {
        return this->onRefEncodedData();
    }

    




    bool implementsDecodeInto() {
        return this->onImplementsDecodeInto();
    }

    
















    bool decodeInto(int pow2, SkBitmap* bitmap) {
        SkASSERT(pow2 >= 0);
        return this->onDecodeInto(pow2, bitmap);
    }

    

    virtual GrTexture* getTexture() { return NULL; }

    







    bool getYUV8Planes(SkISize sizes[3], void* planes[3], size_t rowBytes[3]) {
        return this->onGetYUV8Planes(sizes, planes, rowBytes);
    }

    bool readPixels(SkBitmap* dst, const SkIRect* subset = NULL);

    








    virtual SkPixelRef* deepCopy(SkColorType colortype, const SkIRect* subset) {
        return NULL;
    }

#ifdef SK_BUILD_FOR_ANDROID
    




    virtual void globalRef(void* data=NULL);

    




    virtual void globalUnref();
#endif

    
    
    
    
    
    
    
    
    struct GenIDChangeListener {
        virtual ~GenIDChangeListener() {}
        virtual void onChange() = 0;
    };

    
    void addGenIDChangeListener(GenIDChangeListener* listener);

protected:
    






    virtual bool onNewLockPixels(LockRec*) = 0;

    







    virtual void onUnlockPixels() = 0;

    
    virtual bool onLockPixelsAreWritable() const;

    
    virtual bool onImplementsDecodeInto();
    
    virtual bool onDecodeInto(int pow2, SkBitmap* bitmap);

    





    virtual bool onReadPixels(SkBitmap* dst, const SkIRect* subsetOrNull);

    
    virtual SkData* onRefEncodedData();

    
    virtual bool onGetYUV8Planes(SkISize sizes[3], void* planes[3], size_t rowBytes[3]);

    







    virtual size_t getAllocatedSizeInBytes() const;

    


    SkBaseMutex* mutex() const { return fMutex; }

    
    
    
    void setPreLocked(void*, size_t rowBytes, SkColorTable*);

private:
    SkBaseMutex*    fMutex; 

    
    const SkImageInfo fInfo;

    
    LockRec         fRec;
    int             fLockCount;

    mutable SkTRacy<uint32_t> fGenerationID;
    mutable SkTRacy<bool>     fUniqueGenerationID;

    SkTDArray<GenIDChangeListener*> fGenIDChangeListeners;  

    SkString    fURI;

    
    bool    fIsImmutable;
    
    bool    fPreLocked;

    void needsNewGenID();
    void callGenIDChangeListeners();

    void setMutex(SkBaseMutex* mutex);

    
    
    friend class SkBitmap;  
    void cloneGenID(const SkPixelRef&);

    typedef SkRefCnt INHERITED;
};

class SkPixelRefFactory : public SkRefCnt {
public:
    





    virtual SkPixelRef* create(const SkImageInfo&, size_t rowBytes, SkColorTable*) = 0;
};

#endif
