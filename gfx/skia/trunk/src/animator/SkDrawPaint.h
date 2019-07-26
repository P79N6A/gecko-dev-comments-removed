








#ifndef SkDrawPaint_DEFINED
#define SkDrawPaint_DEFINED

#include "SkDrawable.h"
#include "SkIntArray.h"
#include "SkMemberInfo.h"
#include "SkPaint.h"
#include "SkXfermode.h"

class SkDrawMaskFilter;
class SkDrawPathEffect;
class SkDrawShader;
class SkTransferMode;
class SkDrawTypeface;

class SkDrawPaint : public SkDrawable {
    DECLARE_DRAW_MEMBER_INFO(Paint);
    SkDrawPaint();
    virtual ~SkDrawPaint();
    virtual bool add(SkAnimateMaker* , SkDisplayable* child);
    virtual SkDisplayable* deepCopy(SkAnimateMaker* );
    virtual bool draw(SkAnimateMaker& );
#ifdef SK_DUMP_ENABLED
    virtual void dump(SkAnimateMaker* );
#endif
    virtual void executeFunction(SkDisplayable* target, int index,
        SkTDArray<SkScriptValue>& parameters, SkDisplayTypes type,
        SkScriptValue* );
    virtual const SkFunctionParamType* getFunctionsParameters();
    virtual bool getProperty(int index, SkScriptValue* value) const;
    virtual bool resolveIDs(SkAnimateMaker& maker, SkDisplayable* original, SkApply* apply);
protected:
    static const SkFunctionParamType fFunctionParameters[];
    void setupPaint(SkPaint* paint) const;
public:
    SkBool antiAlias;
    SkDrawColor* color;
    SkBool fakeBold;
    SkBool filterBitmap;
    SkBool linearText;
    SkDrawMaskFilter* maskFilter;
    SkDrawPathEffect* pathEffect;
    SkDrawShader* shader;
    SkBool strikeThru;
    SkBool stroke;
    int  strokeCap;
    int  strokeJoin;
    SkScalar strokeMiter;
    SkScalar strokeWidth;
    int  style;
    int  textAlign;
    SkScalar textScaleX;
    SkScalar textSize;
    SkScalar textSkewX;
    SkDrawTypeface* typeface;
    SkBool underline;
    int  xfermode;
    SkBool8 fOwnsColor;
    SkBool8 fOwnsMaskFilter;
    SkBool8 fOwnsPathEffect;
    SkBool8 fOwnsShader;
    SkBool8 fOwnsTransferMode;
    SkBool8 fOwnsTypeface;
private:
    typedef SkDrawable INHERITED;
    friend class SkTextToPath;
    friend class SkSaveLayer;
};

#endif 
