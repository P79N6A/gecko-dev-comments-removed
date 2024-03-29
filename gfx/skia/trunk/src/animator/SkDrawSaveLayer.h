








#ifndef SkDrawSaveLayer_DEFINED
#define SkDrawSaveLayer_DEFINED

#include "SkDrawGroup.h"
#include "SkMemberInfo.h"

class SkDrawPaint;
class SkDrawRect;

class SkSaveLayer : public SkGroup {
    DECLARE_MEMBER_INFO(SaveLayer);
    SkSaveLayer();
    virtual ~SkSaveLayer();
    virtual bool draw(SkAnimateMaker& );
#ifdef SK_DUMP_ENABLED
    virtual void dump(SkAnimateMaker* );
#endif
    virtual void onEndElement(SkAnimateMaker& );
protected:
    SkDrawPaint* paint;
    SkDrawRect* bounds;
private:
    typedef SkGroup INHERITED;

};

#endif 
