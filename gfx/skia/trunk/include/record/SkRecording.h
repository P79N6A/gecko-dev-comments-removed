






#ifndef SkRecording_DEFINED
#define SkRecording_DEFINED

#include "SkCanvas.h"     
#include "SkRefCnt.h"     
#include "SkTemplates.h"  
#include "SkTypes.h"      


class SkRecord;
class SkRecorder;

namespace EXPERIMENTAL {


















class SK_API SkPlayback : SkNoncopyable {
public:
    
    ~SkPlayback();

    
    void draw(SkCanvas*) const;

private:
    explicit SkPlayback(const SkRecord*);

    SkAutoTDelete<const SkRecord> fRecord;

    friend class SkRecording;
};

class SK_API SkRecording : SkNoncopyable {
public:
    SkRecording(int width, int height);
    ~SkRecording();

    
    
    SkCanvas* canvas();

    
    
    SkPlayback* releasePlayback();

private:
    SkAutoTDelete<SkRecord> fRecord;
    SkAutoTUnref<SkRecorder> fRecorder;
};

}  

#endif
