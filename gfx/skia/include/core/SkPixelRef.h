








#ifndef SkPixelRef_DEFINED
#define SkPixelRef_DEFINED

#include "SkBitmap.h"
#include "SkRefCnt.h"
#include "SkString.h"
#include "SkFlattenable.h"

class SkColorTable;
struct SkIRect;
class SkMutex;


class SkGpuTexture;









class SK_API SkPixelRef : public SkFlattenable {
public:
    SK_DECLARE_INST_COUNT(SkPixelRef)

    explicit SkPixelRef(SkBaseMutex* mutex = NULL);

    


    void* pixels() const { return fPixels; }

    

    SkColorTable* colorTable() const { return fColorTable; }

    


    bool isLocked() const { return fLockCount > 0; }

    


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

#ifdef SK_BUILD_FOR_ANDROID
    




    virtual void globalRef(void* data=NULL);

    




    virtual void globalUnref();
#endif

protected:
    


    virtual void* onLockPixels(SkColorTable**) = 0;
    



    virtual void onUnlockPixels() = 0;

    
    virtual bool onLockPixelsAreWritable() const;

    





    virtual bool onReadPixels(SkBitmap* dst, const SkIRect* subsetOrNull);

    


    SkBaseMutex* mutex() const { return fMutex; }

    
    SkPixelRef(SkFlattenableReadBuffer&, SkBaseMutex*);
    virtual void flatten(SkFlattenableWriteBuffer&) const SK_OVERRIDE;

    
    
    
    void setPreLocked(void* pixels, SkColorTable* ctable);

    




    void useDefaultMutex() { this->setMutex(NULL); }

private:

    SkBaseMutex*    fMutex; 
    void*           fPixels;
    SkColorTable*   fColorTable;    
    int             fLockCount;

    mutable uint32_t fGenerationID;

    
    
    friend class SkBitmap;

    SkString    fURI;

    
    bool    fIsImmutable;
    
    bool    fPreLocked;

    void setMutex(SkBaseMutex* mutex);

    typedef SkFlattenable INHERITED;
};

#endif
