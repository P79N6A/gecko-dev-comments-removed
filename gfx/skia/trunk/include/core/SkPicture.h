








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

    



    SkPicture();
    


    SkPicture(const SkPicture& src);

    










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
        







        kDisableRecordOptimizations_RecordingFlag = 0x04
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

protected:
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static const uint32_t PICTURE_VERSION = 20;

    
    
    
    SkPicturePlayback* fPlayback;
    SkPictureRecord* fRecord;
    int fWidth, fHeight;

    
    
    SkPicture(SkPicturePlayback*, int width, int height);

    
    
    virtual SkBBoxHierarchy* createBBoxHierarchy() const;

    
    
    
    
    static bool StreamIsSKP(SkStream*, SkPictInfo*);
    static bool BufferIsSKP(SkReadBuffer&, SkPictInfo*);
private:
    void createHeader(void* header) const;

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
