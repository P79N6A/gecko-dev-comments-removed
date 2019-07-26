








#ifndef SkPicture_DEFINED
#define SkPicture_DEFINED

#include "SkBitmap.h"
#include "SkRefCnt.h"

class SkBBoxHierarchy;
class SkCanvas;
class SkPicturePlayback;
class SkPictureRecord;
class SkStream;
class SkWStream;






class SK_API SkPicture : public SkRefCnt {
public:
    SK_DECLARE_INST_COUNT(SkPicture)

    



    SkPicture();
    


    SkPicture(const SkPicture& src);

    




    explicit SkPicture(SkStream*);

    










    typedef bool (*InstallPixelRefProc)(const void* src, size_t length, SkBitmap* dst);

    







    SkPicture(SkStream*, bool* success, InstallPixelRefProc proc);

    virtual ~SkPicture();

    


    void swap(SkPicture& other);

    


    SkPicture* clone() const;

    




    void clone(SkPicture* pictures, int count) const;

    enum RecordingFlags {
        







        kUsePathBoundsForClip_RecordingFlag = 0x01,
        
















        kOptimizeForClippedPlayback_RecordingFlag = 0x02
    };

    







    SkCanvas* beginRecording(int width, int height, uint32_t recordFlags = 0);

    


    SkCanvas* getRecordingCanvas() const;
    




    void endRecording();

    



    void draw(SkCanvas* surface);

    




    int width() const { return fWidth; }

    




    int height() const { return fHeight; }

    






    typedef bool (*EncodeBitmap)(SkWStream*, const SkBitmap&);

    



    void serialize(SkWStream*, EncodeBitmap encoder = NULL) const;

#ifdef SK_BUILD_FOR_ANDROID
    




    void abortPlayback();
#endif

protected:
    
    
    
    
    
    
    
    
    
    
    static const uint32_t PICTURE_VERSION = 10;

    
    
    
    SkPicturePlayback* fPlayback;
    SkPictureRecord* fRecord;
    int fWidth, fHeight;

    
    
    virtual SkBBoxHierarchy* createBBoxHierarchy() const;

private:
    void initFromStream(SkStream*, bool* success, InstallPixelRefProc);

    friend class SkFlatPicture;
    friend class SkPicturePlayback;

    typedef SkRefCnt INHERITED;
};

class SkAutoPictureRecord : SkNoncopyable {
public:
    SkAutoPictureRecord(SkPicture* pict, int width, int height,
                        uint32_t recordingFlags = 0) {
        fPicture = pict;
        fCanvas = pict->beginRecording(width, height, recordingFlags);
    }
    ~SkAutoPictureRecord() {
        fPicture->endRecording();
    }

    

    SkCanvas* getRecordingCanvas() const { return fCanvas; }

private:
    SkPicture*  fPicture;
    SkCanvas*   fCanvas;
};


#endif
