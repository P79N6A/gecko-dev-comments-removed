








#ifndef SkSnapShot_DEFINED
#define SkSnapShot_DEFINED

#include "SkDrawable.h"
#include "SkImageDecoder.h"
#include "SkMemberInfo.h"
#include "SkString.h"

class SkSnapshot: public SkDrawable {
    DECLARE_MEMBER_INFO(Snapshot);
    SkSnapshot();
    virtual bool draw(SkAnimateMaker& );
    private:
    SkString filename;
    SkScalar quality;
    SkBool sequence;
    int     type;
    int fSeqVal;
};

#endif 

