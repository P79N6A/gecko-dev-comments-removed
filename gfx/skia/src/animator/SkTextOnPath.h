








#ifndef SkTextOnPath_DEFINED
#define SkTextOnPath_DEFINED

#include "SkBoundable.h"
#include "SkMemberInfo.h"

class SkDrawPath;
class SkText;

class SkTextOnPath : public SkBoundable {
    DECLARE_MEMBER_INFO(TextOnPath);
    SkTextOnPath();
    virtual bool draw(SkAnimateMaker& );
private:
    SkScalar offset;
    SkDrawPath* path;
    SkText* text;
    typedef SkBoundable INHERITED;
};

#endif 
