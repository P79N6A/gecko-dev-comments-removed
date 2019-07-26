








#ifndef SkPicture_DEFINED
#define SkPicture_DEFINED

#include "SkBitmap.h"
#include "SkImageDecoder.h"
#include "SkRefCnt.h"

class SkBBoxHierarchy;
class SkCanvas;
class SkDrawPictureCallback;
class SkData;
class SkPicturePlayback;
class SkPictureRecord;
class SkStream;
class SkWStream;

struct SkPictInfo;






class SK_API SkPicture : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkPicture)

    
    
    
    class AccelData : public SkRefCnt {
    public:
        typedef uint8_t Domain;
        typedef uint32_t Key;

        AccelData(Key key) : fKey(key) { }

        const Key& getKey() const { return fKey; }

        
        
        static Domain GenerateDomain();
    private:
        Key fKey;

        typedef SkRefCnt INHERITED;
    };

    



    SkPicture();
    


    SkPicture(const SkPicture& src);

    
    void EXPERIMENTAL_addAccelData(const AccelData* data) {
        SkRefCnt_SafeAssign(fAccelData, data);
    }
    
    const AccelData* EXPERIMENTAL_getAccelData(AccelData::Key key) const {
        if (NULL != fAccelData && fAccelData->getKey() == key) {
            return fAccelData;
        }
        return NULL;
    }

    










    typedef bool (*InstallPixelRefProc)(const void* src, size_t length, SkBitmap* dst);

    







    static SkPicture* CreateFromStream(SkStream*,
                                       InstallPixelRefProc proc = &SkImageDecoder::DecodeMemory);

    







    static SkPicture* CreateFromBuffer(SkReadBuffer&);

    virtual ~SkPicture();

    


    void swap(SkPicture& other);

    


    SkPicture* clone() const;

    




    void clone(SkPicture* pictures, int count) const;

    enum RecordingFlags {
        







        kUsePathBoundsForClip_RecordingFlag = 0x01,
        
















        kOptimizeForClippedPlayback_RecordingFlag = 0x02,
    };

    







    SkCanvas* beginRecording(int width, int height, uint32_t recordFlags = 0);

    


    SkCanvas* getRecordingCanvas() const;
    




    void endRecording();

    



    void draw(SkCanvas* canvas, SkDrawPictureCallback* = NULL);

    




    int width() const { return fWidth; }

    




    int height() const { return fHeight; }

    









    typedef SkData* (*EncodeBitmap)(size_t* pixelRefOffset, const SkBitmap& bm);

    




    void serialize(SkWStream*, EncodeBitmap encoder = NULL) const;

    


    void flatten(SkWriteBuffer&) const;

    




    bool willPlayBackBitmaps() const;

#ifdef SK_BUILD_FOR_ANDROID
    




    void abortPlayback();
#endif

    







    static bool InternalOnly_StreamIsSKP(SkStream*, SkPictInfo*);
    static bool InternalOnly_BufferIsSKP(SkReadBuffer&, SkPictInfo*);

    





    void internalOnly_EnableOpts(bool enableOpts);

protected:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    

    
    static const uint32_t MIN_PICTURE_VERSION = 19;
    static const uint32_t CURRENT_PICTURE_VERSION = 22;

    
    
    
    SkPicturePlayback*    fPlayback;
    SkPictureRecord*      fRecord;
    int                   fWidth, fHeight;
    const AccelData*      fAccelData;

    
    
    SkPicture(SkPicturePlayback*, int width, int height);

    
    
    virtual SkBBoxHierarchy* createBBoxHierarchy() const;
private:
    void createHeader(SkPictInfo* info) const;
    static bool IsValidPictInfo(const SkPictInfo& info);

    friend class SkFlatPicture;
    friend class SkPicturePlayback;

    typedef SkRefCnt INHERITED;
};











class SK_API SkDrawPictureCallback {
public:
    SkDrawPictureCallback() {}
    virtual ~SkDrawPictureCallback() {}

    virtual bool abortDrawing() = 0;
};

#endif
