






#ifndef SkImageDecoder_DEFINED
#define SkImageDecoder_DEFINED

#include "SkBitmap.h"
#include "SkImage.h"
#include "SkRect.h"
#include "SkRefCnt.h"
#include "SkTRegistry.h"
#include "SkTypes.h"

class SkStream;
class SkStreamRewindable;





class SkImageDecoder : SkNoncopyable {
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
        kPKM_Format,
        kKTX_Format,

        kLastKnownFormat = kKTX_Format,
    };

    


    virtual Format getFormat() const;

    


    static Format GetStreamFormat(SkStreamRewindable*);

    

    static const char* GetFormatName(Format);

    

    const char* getFormatName() const;

    

    bool getSkipWritingZeroes() const { return fSkipWritingZeroes; }

    










    void setSkipWritingZeroes(bool skip) { fSkipWritingZeroes = skip; }

    


    bool getDitherImage() const { return fDitherImage; }

    


    void setDitherImage(bool dither) { fDitherImage = dither; }

    



    bool getPreferQualityOverSpeed() const { return fPreferQualityOverSpeed; }

    



    void setPreferQualityOverSpeed(bool qualityOverSpeed) {
        fPreferQualityOverSpeed = qualityOverSpeed;
    }

    





    void setRequireUnpremultipliedColors(bool request) {
        fRequireUnpremultipliedColors = request;
    }

    


    bool getRequireUnpremultipliedColors() const { return fRequireUnpremultipliedColors; }

    




    class Peeker : public SkRefCnt {
    public:
        SK_DECLARE_INST_COUNT(Peeker)

        


        virtual bool peek(const char tag[], const void* data, size_t length) = 0;
    private:
        typedef SkRefCnt INHERITED;
    };

    Peeker* getPeeker() const { return fPeeker; }
    Peeker* setPeeker(Peeker*);

#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    




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
#endif

#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
    



















    struct PrefConfigTable {
        SkBitmap::Config fPrefFor_8Index_NoAlpha_src;
        SkBitmap::Config fPrefFor_8Index_YesAlpha_src;
        SkBitmap::Config fPrefFor_8Gray_src;
        SkBitmap::Config fPrefFor_8bpc_NoAlpha_src;
        SkBitmap::Config fPrefFor_8bpc_YesAlpha_src;
    };

    






    void setPrefConfigTable(const PrefConfigTable&);

    




    void resetPrefConfigTable() { fUsePrefTable = false; }
#endif

    









    void setPreserveSrcDepth(bool preserve) {
        fPreserveSrcDepth = preserve;
    }

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

    


















    bool decode(SkStream*, SkBitmap* bitmap, SkColorType pref, Mode);
    bool decode(SkStream* stream, SkBitmap* bitmap, Mode mode) {
        return this->decode(stream, bitmap, kUnknown_SkColorType, mode);
    }

    






    bool buildTileIndex(SkStreamRewindable*, int *width, int *height);

    






    bool decodeSubset(SkBitmap* bm, const SkIRect& subset, SkColorType pref);

    


    static SkImageDecoder* Factory(SkStreamRewindable*);

    








    static bool DecodeFile(const char file[], SkBitmap* bitmap, SkColorType pref, Mode,
                           Format* format = NULL);
    static bool DecodeFile(const char file[], SkBitmap* bitmap) {
        return DecodeFile(file, bitmap, kUnknown_SkColorType, kDecodePixels_Mode, NULL);
    }

    








    static bool DecodeMemory(const void* buffer, size_t size, SkBitmap* bitmap, SkColorType pref,
                             Mode, Format* format = NULL);
    static bool DecodeMemory(const void* buffer, size_t size, SkBitmap* bitmap){
        return DecodeMemory(buffer, size, bitmap, kUnknown_SkColorType, kDecodePixels_Mode, NULL);
    }

    


    struct Target {
        


        void*  fAddr;

        


        size_t fRowBytes;
    };

    








    static bool DecodeStream(SkStreamRewindable* stream, SkBitmap* bitmap, SkColorType pref, Mode,
                             Format* format = NULL);
    static bool DecodeStream(SkStreamRewindable* stream, SkBitmap* bitmap) {
        return DecodeStream(stream, bitmap, kUnknown_SkColorType, kDecodePixels_Mode, NULL);
    }

protected:
    
    virtual bool onDecode(SkStream*, SkBitmap* bitmap, Mode) = 0;

    
    
    virtual bool onBuildTileIndex(SkStreamRewindable*, int *width, int *height) {
        return false;
    }

    
    
    virtual bool onDecodeSubset(SkBitmap* bitmap, const SkIRect& rect) {
        return false;
    }

    














    bool cropBitmap(SkBitmap *dst, SkBitmap *src, int sampleSize,
                    int dstX, int dstY, int width, int height,
                    int srcX, int srcY);

    



    void copyFieldsToOther(SkImageDecoder* other);

    








public:
    bool shouldCancelDecode() const { return fShouldCancelDecode; }

protected:
    SkImageDecoder();

    


    SkColorType getDefaultPref() { return fDefaultPref; }
    
#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    
    
    bool chooseFromOneChoice(SkColorType, int width, int height) const;
#endif

    


    bool allocPixelRef(SkBitmap*, SkColorTable*) const;

    


    enum SrcDepth {
        
        kIndex_SrcDepth,
        
        k8BitGray_SrcDepth,
        
        k32Bit_SrcDepth,
    };
    




    SkColorType getPrefColorType(SrcDepth, bool hasAlpha) const;

private:
    Peeker*                 fPeeker;
#ifdef SK_SUPPORT_LEGACY_IMAGEDECODER_CHOOSER
    Chooser*                fChooser;
#endif
    SkBitmap::Allocator*    fAllocator;
    int                     fSampleSize;
    SkColorType             fDefaultPref;   
#ifdef SK_SUPPORT_LEGACY_BITMAP_CONFIG
    PrefConfigTable         fPrefTable;     
    bool                    fUsePrefTable;
#endif
    bool                    fPreserveSrcDepth;
    bool                    fDitherImage;
    bool                    fSkipWritingZeroes;
    mutable bool            fShouldCancelDecode;
    bool                    fPreferQualityOverSpeed;
    bool                    fRequireUnpremultipliedColors;
};







class SkImageDecoderFactory : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkImageDecoderFactory)

    virtual SkImageDecoder* newDecoder(SkStreamRewindable*) = 0;

private:
    typedef SkRefCnt INHERITED;
};

class SkDefaultImageDecoderFactory : SkImageDecoderFactory {
public:
    
    virtual SkImageDecoder* newDecoder(SkStreamRewindable* stream) {
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
DECLARE_DECODER_CREATOR(PKMImageDecoder);
DECLARE_DECODER_CREATOR(KTXImageDecoder);



typedef SkTRegistry<SkImageDecoder*(*)(SkStreamRewindable*)>        SkImageDecoder_DecodeReg;
typedef SkTRegistry<SkImageDecoder::Format(*)(SkStreamRewindable*)> SkImageDecoder_FormatReg;

#endif
