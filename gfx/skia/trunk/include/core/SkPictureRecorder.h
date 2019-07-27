






#ifndef SkPictureRecorder_DEFINED
#define SkPictureRecorder_DEFINED

#include "SkBBHFactory.h"
#include "SkPicture.h"
#include "SkRefCnt.h"

#ifdef SK_BUILD_FOR_ANDROID_FRAMEWORK
namespace android {
    class Picture;
};
#endif

class SkCanvas;
class SkPictureRecord;
class SkRecord;
class SkRecorder;

class SK_API SkPictureRecorder : SkNoncopyable {
public:
    SkPictureRecorder();
    ~SkPictureRecorder();

    








    SkCanvas* beginRecording(int width, int height,
                             SkBBHFactory* bbhFactory = NULL,
                             uint32_t recordFlags = 0);

    
    SkCanvas* EXPERIMENTAL_beginRecording(int width, int height,
                                          SkBBHFactory* bbhFactory = NULL);

    


    SkCanvas* getRecordingCanvas();

    




    SkPicture* endRecording();

    





    void internalOnly_EnableOpts(bool enableOpts);

private:
    void reset();

    


#ifdef SK_BUILD_FOR_ANDROID_FRAMEWORK
    friend class android::Picture;
#endif
    friend class SkPictureRecorderReplayTester; 
    void partialReplay(SkCanvas* canvas) const;

    int fWidth;
    int fHeight;

    
    SkAutoTUnref<SkPictureRecord> fPictureRecord;  
    SkAutoTUnref<SkRecorder>      fRecorder;       

    
    SkAutoTDelete<SkRecord> fRecord;

    typedef SkNoncopyable INHERITED;
};

#endif
