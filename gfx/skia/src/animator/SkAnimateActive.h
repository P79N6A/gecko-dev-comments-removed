








#ifndef SkAnimateActive_DEFINED
#define SkAnimateActive_DEFINED

#include "SkDisplayApply.h"
#include "SkOperandInterpolator.h"
#include "SkIntArray.h"

class SkAnimateMaker;

class SkActive {
public:
    SkActive(SkApply& , SkAnimateMaker& );
    ~SkActive();
    void advance();
    void append(SkApply* );
    void calcDurations(int index);
    void create(SkDrawable* scope, SkMSec time);
    bool draw() { return immediate(false); }
    bool enable() { return immediate(true); }
    void init( );
    SkMSec getTime(SkMSec inTime, int animatorIndex);
    void pickUp(SkActive* existing);
    void reset() { fDrawIndex = 0; }
    void setInterpolator(int index, SkOperand* from);
    void start();
#ifdef SK_DEBUG
    void validate();
#endif
private:
    void appendSave(int oldCount);
    void fixInterpolator(SkBool save);
    bool immediate(bool enable);
    bool initializeSave();
    void initState(SkApply* , int offset);
    void resetInterpolators();
    void resetState();
    void restoreInterpolatorValues(int index);
    void saveInterpolatorValues(int index);
    void setSteps(int steps);
    struct SkState {

        SkMSec getRelativeTime(SkMSec time);
        SkApply::Mode fMode;
        SkApply::Transition fTransition;
        SkBool8 fPickup;
        SkBool8 fRestore;
        SkBool8 fStarted;
        SkBool8 fUnpostedEndEvent;
        int32_t fSteps;
        SkMSec fBegin;
        SkMSec fStartTime;
        SkMSec fDuration;
        SkMSec fSave;
        SkMSec fTicks;
    };
    SkActive& operator= (const SkActive& );
    SkTDArray<SkOperandInterpolator*> fInterpolators;
    SkApply& fApply;
    SkTDArray<SkState> fState;  
    SkTDOperandPtrArray fSaveRestore;   
    SkTDOperandPtrArray fSaveInterpolators;
    SkTDAnimateArray fAnimators;
    SkMSec fMaxTime;    
    SkAnimateMaker& fMaker;
    int fDrawIndex;
    int fDrawMax;
    friend class SkApply;
};

#endif 
