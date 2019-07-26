








#ifndef SkImageRef_DEFINED
#define SkImageRef_DEFINED

#include "SkPixelRef.h"
#include "SkBitmap.h"
#include "SkImageDecoder.h"
#include "SkString.h"

class SkImageRefPool;
class SkStreamRewindable;




class SkImageRef : public SkPixelRef {
public:
    










    SkImageRef(const SkImageInfo&, SkStreamRewindable*, int sampleSize = 1,
               SkBaseMutex* mutex = NULL);
    virtual ~SkImageRef();

    

    void setDitherImage(bool dither) { fDoDither = dither; }

    






    bool getInfo(SkBitmap* bm);

    



    bool isOpaque(SkBitmap* bm);

    SkImageDecoderFactory* getDecoderFactory() const { return fFactory; }
    
    SkImageDecoderFactory* setDecoderFactory(SkImageDecoderFactory*);

protected:
    


    virtual bool onDecode(SkImageDecoder* codec, SkStreamRewindable*, SkBitmap*,
                          SkBitmap::Config, SkImageDecoder::Mode);

    



    virtual bool onNewLockPixels(LockRec*) SK_OVERRIDE;
    
    virtual void onUnlockPixels() SK_OVERRIDE {}

    SkImageRef(SkReadBuffer&, SkBaseMutex* mutex = NULL);
    virtual void flatten(SkWriteBuffer&) const SK_OVERRIDE;

    SkBitmap fBitmap;

private:
    SkStreamRewindable* setStream(SkStreamRewindable*);
    
    
    bool prepareBitmap(SkImageDecoder::Mode);

    SkImageDecoderFactory*  fFactory;    
    SkStreamRewindable*     fStream;
    int                     fSampleSize;
    bool                    fDoDither;
    bool                    fErrorInDecoding;

    friend class SkImageRefPool;

    SkImageRef*  fPrev, *fNext;
    size_t ramUsed() const;

    typedef SkPixelRef INHERITED;
};

#endif
