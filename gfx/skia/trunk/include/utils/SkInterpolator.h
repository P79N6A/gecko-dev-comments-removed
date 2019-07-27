








#ifndef SkInterpolator_DEFINED
#define SkInterpolator_DEFINED

#include "SkScalar.h"

class SkInterpolatorBase : SkNoncopyable {
public:
    enum Result {
        kNormal_Result,
        kFreezeStart_Result,
        kFreezeEnd_Result
    };
protected:
    SkInterpolatorBase();
    ~SkInterpolatorBase();
public:
    void    reset(int elemCount, int frameCount);

    









    bool    getDuration(SkMSec* startTime, SkMSec* endTime) const;


    



    void setMirror(bool mirror) {
        fFlags = SkToU8((fFlags & ~kMirror) | (int)mirror);
    }

    


    void    setRepeatCount(SkScalar repeatCount) { fRepeat = repeatCount; }

    



    void setReset(bool reset) {
        fFlags = SkToU8((fFlags & ~kReset) | (int)reset);
    }

    Result  timeToT(SkMSec time, SkScalar* T, int* index, SkBool* exact) const;

protected:
    enum Flags {
        kMirror = 1,
        kReset = 2,
        kHasBlend = 4
    };
    static SkScalar ComputeRelativeT(SkMSec time, SkMSec prevTime,
                             SkMSec nextTime, const SkScalar blend[4] = NULL);
    int16_t fFrameCount;
    uint8_t fElemCount;
    uint8_t fFlags;
    SkScalar fRepeat;
    struct SkTimeCode {
        SkMSec  fTime;
        SkScalar fBlend[4];
    };
    SkTimeCode* fTimes;     
    void* fStorage;
#ifdef SK_DEBUG
    SkTimeCode(* fTimesArray)[10];
#endif
};

class SkInterpolator : public SkInterpolatorBase {
public:
    SkInterpolator();
    SkInterpolator(int elemCount, int frameCount);
    void    reset(int elemCount, int frameCount);

    










    bool setKeyFrame(int index, SkMSec time, const SkScalar values[],
                     const SkScalar blend[4] = NULL);

    






    Result timeToValues(SkMSec time, SkScalar values[] = NULL) const;

private:
    SkScalar* fValues;  
#ifdef SK_DEBUG
    SkScalar(* fScalarsArray)[10];
#endif
    typedef SkInterpolatorBase INHERITED;
};




SkScalar SkUnitCubicInterp(SkScalar value, SkScalar bx, SkScalar by,
                           SkScalar cx, SkScalar cy);

#endif
