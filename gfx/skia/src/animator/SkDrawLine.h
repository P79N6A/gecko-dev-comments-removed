








#ifndef SkDrawLine_DEFINED
#define SkDrawLine_DEFINED

#include "SkBoundable.h"
#include "SkMemberInfo.h"

class SkLine : public SkBoundable {
    DECLARE_MEMBER_INFO(Line);
    SkLine();
    virtual bool draw(SkAnimateMaker& );
private:
    SkScalar x1;
    SkScalar x2;
    SkScalar y1;
    SkScalar y2;
    typedef SkBoundable INHERITED;
};

#endif 

