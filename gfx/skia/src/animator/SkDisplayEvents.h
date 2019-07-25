








#ifndef SkDisplayEvents_DEFINED
#define SkDisplayEvents_DEFINED

#include "SkEvent.h"
#include "SkDisplayEvent.h"

struct SkEventState {
    SkEventState();
    int fCode;
    SkBool fDisable;
    SkDisplayable* fDisplayable;
    SkScalar fX;
    SkScalar fY;
};

class SkEvents {
public:
    SkEvents();
    ~SkEvents();
    void addEvent(SkDisplayEvent* evt) { *fEvents.append() = evt; }
    bool doEvent(SkAnimateMaker& , SkDisplayEvent::Kind , SkEventState* );
#ifdef SK_DUMP_ENABLED
    void dump(SkAnimateMaker& );
#endif
    void reset() { fEvents.reset(); }
    void removeEvent(SkDisplayEvent::Kind kind, SkEventState* );
private:
    SkTDDisplayEventArray fEvents;
    SkBool fError;
    friend class SkDisplayXMLParser;
};

#endif 

