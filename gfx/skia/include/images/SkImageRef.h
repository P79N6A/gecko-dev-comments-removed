








#ifndef SkImageRef_DEFINED
#define SkImageRef_DEFINED

#include "SkPixelRef.h"
#include "SkBitmap.h"
#include "SkImageDecoder.h"
#include "SkString.h"

class SkImageRefPool;
class SkStream;




class SkImageRef : public SkPixelRef {
public:
    










    SkImageRef(SkStream*, SkBitmap::Config config, int sampleSize = 1);
    virtual ~SkImageRef();

    

    void setDitherImage(bool dither) { fDoDither = dither; }
    
    






    bool getInfo(SkBitmap* bm);
    
    



    bool isOpaque(SkBitmap* bm);
    
    SkImageDecoderFactory* getDecoderFactory() const { return fFactory; }
    
    SkImageDecoderFactory* setDecoderFactory(SkImageDecoderFactory*);

    
    virtual void flatten(SkFlattenableWriteBuffer&) const;

protected:
    


    virtual bool onDecode(SkImageDecoder* codec, SkStream*, SkBitmap*,
                          SkBitmap::Config, SkImageDecoder::Mode);

    



    virtual void* onLockPixels(SkColorTable**);
    
    virtual void onUnlockPixels();
    
    SkImageRef(SkFlattenableReadBuffer&);

    SkBitmap fBitmap;

private:    
    SkStream* setStream(SkStream*);
    
    
    bool prepareBitmap(SkImageDecoder::Mode);

    SkImageDecoderFactory*  fFactory;    
    SkStream*               fStream;
    SkBitmap::Config        fConfig;
    int                     fSampleSize;
    bool                    fDoDither;
    bool                    fErrorInDecoding;
    
    friend class SkImageRefPool;
    
    SkImageRef*  fPrev, *fNext;    
    size_t ramUsed() const;
    
    typedef SkPixelRef INHERITED;
};

#endif
