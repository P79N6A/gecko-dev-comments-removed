








#ifndef SkImageDecoder_DEFINED
#define SkImageDecoder_DEFINED

#include "SkBitmap.h"
#include "SkBitmapFactory.h"
#include "SkImage.h"
#include "SkRect.h"
#include "SkRefCnt.h"

class SkStream;





class SkImageDecoder {
public:
    virtual ~SkImageDecoder();

    
    enum Format {
        kUnknown_Format,
        kBMP_Format,
        kGIF_Format,
        kICO_Format,
        kJPEG_Format,
        kPNG_Format,
        kWBMP_Format,
        kWEBP_Format,

        kLastKnownFormat = kWEBP_Format
    };

    

    virtual Format getFormat() const;

    

    const char* getFormatName() const;

    


    bool getDitherImage() const { return fDitherImage; }

    


    void setDitherImage(bool dither) { fDitherImage = dither; }

    



    bool getPreferQualityOverSpeed() const { return fPreferQualityOverSpeed; }

    



    void setPreferQualityOverSpeed(bool qualityOverSpeed) {
        fPreferQualityOverSpeed = qualityOverSpeed;
    }

    




    class Peeker : public SkRefCnt {
    public:
        SK_DECLARE_INST_COUNT(Peeker)

        


        virtual bool peek(const char tag[], const void* data, size_t length) = 0;
    private:
        typedef SkRefCnt INHERITED;
    };

    Peeker* getPeeker() const { return fPeeker; }
    Peeker* setPeeker(Peeker*);

    




    class Chooser : public SkRefCnt {
    public:
        SK_DECLARE_INST_COUNT(Chooser)

        virtual void begin(int count) {}
        virtual void inspect(int index, SkBitmap::Config config, int width, int height) {}
        

        virtual int choose() = 0;

    private:
        typedef SkRefCnt INHERITED;
    };

    Chooser* getChooser() const { return fChooser; }
    Chooser* setChooser(Chooser*);

    
























    void setPrefConfigTable(const SkBitmap::Config pref[6]);

    SkBitmap::Allocator* getAllocator() const { return fAllocator; }
    SkBitmap::Allocator* setAllocator(SkBitmap::Allocator*);

    
    
    
    
    
    
    int getSampleSize() const { return fSampleSize; }
    void setSampleSize(int size);

    

    void resetSampleSize() { this->setSampleSize(1); }

    










    void cancelDecode() {
        
        
        fShouldCancelDecode = true;
    }

    



    enum Mode {
        kDecodeBounds_Mode, 
        kDecodePixels_Mode  
    };

    












    bool decode(SkStream*, SkBitmap* bitmap, SkBitmap::Config pref, Mode, bool reuseBitmap = false);
    bool decode(SkStream* stream, SkBitmap* bitmap, Mode mode, bool reuseBitmap = false) {
        return this->decode(stream, bitmap, SkBitmap::kNo_Config, mode, reuseBitmap);
    }

    






    bool buildTileIndex(SkStream*, int *width, int *height);

    






    bool decodeRegion(SkBitmap* bitmap, const SkIRect& rect, SkBitmap::Config pref);

    


    static SkImageDecoder* Factory(SkStream*);

    












    static bool DecodeFile(const char file[], SkBitmap* bitmap,
                           SkBitmap::Config prefConfig, Mode,
                           Format* format = NULL);
    static bool DecodeFile(const char file[], SkBitmap* bitmap) {
        return DecodeFile(file, bitmap, SkBitmap::kNo_Config,
                          kDecodePixels_Mode, NULL);
    }
    












    static bool DecodeMemory(const void* buffer, size_t size, SkBitmap* bitmap,
                             SkBitmap::Config prefConfig, Mode,
                             Format* format = NULL);
    static bool DecodeMemory(const void* buffer, size_t size, SkBitmap* bitmap){
        return DecodeMemory(buffer, size, bitmap, SkBitmap::kNo_Config,
                            kDecodePixels_Mode, NULL);
    }

    























    static bool DecodeMemoryToTarget(const void* buffer, size_t size, SkImage::Info* info,
                                     const SkBitmapFactory::Target* target);

    












    static bool DecodeStream(SkStream* stream, SkBitmap* bitmap,
                             SkBitmap::Config prefConfig, Mode,
                             Format* format = NULL);
    static bool DecodeStream(SkStream* stream, SkBitmap* bitmap) {
        return DecodeStream(stream, bitmap, SkBitmap::kNo_Config,
                            kDecodePixels_Mode, NULL);
    }

    




    static SkBitmap::Config GetDeviceConfig();
    





    static void SetDeviceConfig(SkBitmap::Config);

protected:
    
    virtual bool onDecode(SkStream*, SkBitmap* bitmap, Mode) = 0;

    
    
    virtual bool onBuildTileIndex(SkStream*, int *width, int *height) {
        return false;
    }

    
    
    virtual bool onDecodeRegion(SkBitmap* bitmap, const SkIRect& rect) {
        return false;
    }

    













    void cropBitmap(SkBitmap *dst, SkBitmap *src, int sampleSize,
                    int dstX, int dstY, int width, int height,
                    int srcX, int srcY);



    








public:
    bool shouldCancelDecode() const { return fShouldCancelDecode; }

protected:
    SkImageDecoder();

    
    
    bool chooseFromOneChoice(SkBitmap::Config config, int width, int height) const;

    



    bool allocPixelRef(SkBitmap*, SkColorTable*) const;

    enum SrcDepth {
        kIndex_SrcDepth,
        k16Bit_SrcDepth,
        k32Bit_SrcDepth
    };
    







    SkBitmap::Config getPrefConfig(SrcDepth, bool hasAlpha) const;

private:
    Peeker*                 fPeeker;
    Chooser*                fChooser;
    SkBitmap::Allocator*    fAllocator;
    int                     fSampleSize;
    SkBitmap::Config        fDefaultPref;   
    SkBitmap::Config        fPrefTable[6];  
    bool                    fDitherImage;
    bool                    fUsePrefTable;
    mutable bool            fShouldCancelDecode;
    bool                    fPreferQualityOverSpeed;

    




    static const char* sFormatName[];

    
    SkImageDecoder(const SkImageDecoder&);
    SkImageDecoder& operator=(const SkImageDecoder&);
};







class SkImageDecoderFactory : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkImageDecoderFactory)

    virtual SkImageDecoder* newDecoder(SkStream*) = 0;

private:
    typedef SkRefCnt INHERITED;
};

class SkDefaultImageDecoderFactory : SkImageDecoderFactory {
public:
    
    virtual SkImageDecoder* newDecoder(SkStream* stream) {
        return SkImageDecoder::Factory(stream);
    }
};



#define DECLARE_DECODER_CREATOR(codec)          \
    SkImageDecoder *Create ## codec ();



#define DEFINE_DECODER_CREATOR(codec)           \
    SkImageDecoder *Create ## codec () {        \
        return SkNEW( Sk ## codec );            \
    }



DECLARE_DECODER_CREATOR(BMPImageDecoder);
DECLARE_DECODER_CREATOR(GIFImageDecoder);
DECLARE_DECODER_CREATOR(ICOImageDecoder);
DECLARE_DECODER_CREATOR(JPEGImageDecoder);
DECLARE_DECODER_CREATOR(PNGImageDecoder);
DECLARE_DECODER_CREATOR(WBMPImageDecoder);
DECLARE_DECODER_CREATOR(WEBPImageDecoder);

#endif
