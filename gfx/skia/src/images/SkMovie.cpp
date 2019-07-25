






#include "SkMovie.h"
#include "SkCanvas.h"
#include "SkPaint.h"



#define UNINITIALIZED_MSEC ((SkMSec)-1)

SkMovie::SkMovie()
{
    fInfo.fDuration = UNINITIALIZED_MSEC;  
    fCurrTime = UNINITIALIZED_MSEC; 
    fNeedBitmap = true;
}

void SkMovie::ensureInfo()
{
    if (fInfo.fDuration == UNINITIALIZED_MSEC && !this->onGetInfo(&fInfo))
        memset(&fInfo, 0, sizeof(fInfo));   
}

SkMSec SkMovie::duration()
{
    this->ensureInfo();
    return fInfo.fDuration;
}

int SkMovie::width()
{
    this->ensureInfo();
    return fInfo.fWidth;
}

int SkMovie::height()
{
    this->ensureInfo();
    return fInfo.fHeight;
}

int SkMovie::isOpaque()
{
    this->ensureInfo();
    return fInfo.fIsOpaque;
}

bool SkMovie::setTime(SkMSec time)
{
    SkMSec dur = this->duration();
    if (time > dur)
        time = dur;
        
    bool changed = false;
    if (time != fCurrTime)
    {
        fCurrTime = time;
        changed = this->onSetTime(time);
        fNeedBitmap |= changed;
    }
    return changed;
}

const SkBitmap& SkMovie::bitmap()
{
    if (fCurrTime == UNINITIALIZED_MSEC)    
        this->setTime(0);

    if (fNeedBitmap)
    {
        if (!this->onGetBitmap(&fBitmap))   
            fBitmap.reset();
        fNeedBitmap = false;
    }
    return fBitmap;
}



#include "SkStream.h"

SkMovie* SkMovie::DecodeMemory(const void* data, size_t length) {
    SkMemoryStream stream(data, length, false);
    return SkMovie::DecodeStream(&stream);
}

SkMovie* SkMovie::DecodeFile(const char path[])
{
    SkMovie* movie = NULL;

    SkFILEStream stream(path);
    if (stream.isValid()) {
        movie = SkMovie::DecodeStream(&stream);
    }
#ifdef SK_DEBUG
    else {
        SkDebugf("Movie file not found <%s>\n", path);
    }
#endif

    return movie;
}

