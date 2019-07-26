








#ifndef SkPicture_DEFINED
#define SkPicture_DEFINED

#include "SkRefCnt.h"

class SkBitmap;
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

    


    bool hasRecorded() const;

    



    void draw(SkCanvas* surface);

    




    int width() const { return fWidth; }

    




    int height() const { return fHeight; }

    void serialize(SkWStream*) const;

    



    void abortPlayback();

private:
    int fWidth, fHeight;
    SkPictureRecord* fRecord;
    SkPicturePlayback* fPlayback;

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
