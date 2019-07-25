








#ifndef SkImageDecoder_DEFINED
#define SkImageDecoder_DEFINED

#include "SkBitmap.h"
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

        kLastKnownFormat = kWBMP_Format
    };

    

    virtual Format getFormat() const;

    


    bool getDitherImage() const { return fDitherImage; }

    


    void setDitherImage(bool dither) { fDitherImage = dither; }

    




    class Peeker : public SkRefCnt {
    public:
        


        virtual bool peek(const char tag[], const void* data, size_t length) = 0;
    };

    Peeker* getPeeker() const { return fPeeker; }
    Peeker* setPeeker(Peeker*);

    




    class Chooser : public SkRefCnt {
    public:
        virtual void begin(int count) {}
        virtual void inspect(int index, SkBitmap::Config config, int width, int height) {}
        

        virtual int choose() = 0;
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

    












    bool decode(SkStream*, SkBitmap* bitmap, SkBitmap::Config pref, Mode);
    bool decode(SkStream* stream, SkBitmap* bitmap, Mode mode) {
        return this->decode(stream, bitmap, SkBitmap::kNo_Config, mode);
    }

    


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
    












    static bool DecodeStream(SkStream* stream, SkBitmap* bitmap,
                             SkBitmap::Config prefConfig, Mode,
                             Format* format = NULL);
    static bool DecodeStream(SkStream* stream, SkBitmap* bitmap) {
        return DecodeStream(stream, bitmap, SkBitmap::kNo_Config,
                            kDecodePixels_Mode, NULL);
    }

    




    static SkBitmap::Config GetDeviceConfig();
    





    static void SetDeviceConfig(SkBitmap::Config);

  
    SkDEBUGCODE(static void UnitTest();)
  

protected:
    
    virtual bool onDecode(SkStream*, SkBitmap* bitmap, Mode) = 0;

    








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

    
    SkImageDecoder(const SkImageDecoder&);
    SkImageDecoder& operator=(const SkImageDecoder&);
};







class SkImageDecoderFactory : public SkRefCnt {
public:
    virtual SkImageDecoder* newDecoder(SkStream*) = 0;
};

class SkDefaultImageDecoderFactory : SkImageDecoderFactory {
public:
    
    virtual SkImageDecoder* newDecoder(SkStream* stream) {
        return SkImageDecoder::Factory(stream);
    }
};


#endif
