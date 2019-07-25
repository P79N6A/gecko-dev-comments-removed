








#ifndef SkMovie_DEFINED
#define SkMovie_DEFINED

#include "SkRefCnt.h"
#include "SkCanvas.h"

class SkStream;

class SkMovie : public SkRefCnt {
public:
    


    static SkMovie* DecodeStream(SkStream*);
    




    static SkMovie* DecodeFile(const char path[]);
    




    static SkMovie* DecodeMemory(const void* data, size_t length);

    SkMSec  duration();
    int     width();
    int     height();
    int     isOpaque();
    
    




    bool setTime(SkMSec);

    
    const SkBitmap& bitmap();
    
protected:
    struct Info {
        SkMSec  fDuration;
        int     fWidth;
        int     fHeight;
        bool    fIsOpaque;
    };

    virtual bool onGetInfo(Info*) = 0;
    virtual bool onSetTime(SkMSec) = 0;
    virtual bool onGetBitmap(SkBitmap*) = 0;

    
    SkMovie();

private:
    Info        fInfo;
    SkMSec      fCurrTime;
    SkBitmap    fBitmap;
    bool        fNeedBitmap;
    
    void ensureInfo();
};

#endif
