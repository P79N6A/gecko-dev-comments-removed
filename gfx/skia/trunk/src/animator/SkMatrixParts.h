








#ifndef SkMatrixParts_DEFINED
#define SkMatrixParts_DEFINED

#include "SkDisplayable.h"
#include "SkMemberInfo.h"
#include "SkPathMeasure.h"

class SkDrawPath;
class SkDrawRect;
class SkPolygon;

class SkDrawMatrix;


class SkMatrixPart : public SkDisplayable {
public:
    SkMatrixPart();
    virtual bool add() = 0;
    virtual void dirty();
    virtual SkDisplayable* getParent() const;
    virtual bool setParent(SkDisplayable* parent);
#ifdef SK_DEBUG
    virtual bool isMatrixPart() const { return true; }
#endif
protected:
    SkDrawMatrix* fMatrix;
};

class SkRotate : public SkMatrixPart {
    DECLARE_MEMBER_INFO(Rotate);
    SkRotate();
protected:
    virtual bool add();
    SkScalar degrees;
    SkPoint center;
};

class SkScale : public SkMatrixPart {
    DECLARE_MEMBER_INFO(Scale);
    SkScale();
protected:
    virtual bool add();
    SkScalar x;
    SkScalar y;
    SkPoint center;
};

class SkSkew : public SkMatrixPart {
    DECLARE_MEMBER_INFO(Skew);
    SkSkew();
protected:
    virtual bool add();
    SkScalar x;
    SkScalar y;
    SkPoint center;
};

class SkTranslate : public SkMatrixPart {
    DECLARE_MEMBER_INFO(Translate);
    SkTranslate();
protected:
    virtual bool add();
    SkScalar x;
    SkScalar y;
};

class SkFromPath : public SkMatrixPart {
    DECLARE_MEMBER_INFO(FromPath);
    SkFromPath();
    virtual ~SkFromPath();
protected:
    virtual bool add();
    int32_t mode;
    SkScalar offset;
    SkDrawPath* path;
    SkPathMeasure fPathMeasure;
};

class SkRectToRect : public SkMatrixPart {
    DECLARE_MEMBER_INFO(RectToRect);
    SkRectToRect();
    virtual ~SkRectToRect();
#ifdef SK_DUMP_ENABLED
    virtual void dump(SkAnimateMaker* );
#endif
    virtual const SkMemberInfo* preferredChild(SkDisplayTypes type);
protected:
    virtual bool add();
    SkDrawRect* source;
    SkDrawRect* destination;
};

class SkPolyToPoly : public SkMatrixPart {
    DECLARE_MEMBER_INFO(PolyToPoly);
    SkPolyToPoly();
    virtual ~SkPolyToPoly();
#ifdef SK_DUMP_ENABLED
    virtual void dump(SkAnimateMaker* );
#endif
    virtual void onEndElement(SkAnimateMaker& );
    virtual const SkMemberInfo* preferredChild(SkDisplayTypes type);
protected:
    virtual bool add();
    SkPolygon* source;
    SkPolygon* destination;
};



#endif 
