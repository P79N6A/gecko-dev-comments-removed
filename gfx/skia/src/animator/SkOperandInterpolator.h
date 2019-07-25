








#ifndef SkOperandInterpolator_DEFINED
#define SkOperandInterpolator_DEFINED

#include "SkDisplayType.h"
#include "SkInterpolator.h"
#include "SkOperand.h"

class SkOperandInterpolator : public SkInterpolatorBase {
public:
    SkOperandInterpolator();
    SkOperandInterpolator(int elemCount, int frameCount, SkDisplayTypes type);
    SkOperand* getValues() { return fValues; }
    int getValuesCount() { return fFrameCount * fElemCount; }
    void    reset(int elemCount, int frameCount, SkDisplayTypes type);

    









    bool    setKeyFrame(int index, SkMSec time, const SkOperand values[], SkScalar blend = SK_Scalar1);
    Result timeToValues(SkMSec time, SkOperand values[]) const;
    SkDEBUGCODE(static void UnitTest();)
private:
    SkDisplayTypes fType;
    SkOperand* fValues;     
#ifdef SK_DEBUG
    SkOperand(* fValuesArray)[10];
#endif
    typedef SkInterpolatorBase INHERITED;
};

#endif 

