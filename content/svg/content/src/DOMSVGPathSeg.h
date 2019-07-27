




#ifndef MOZILLA_DOMSVGPATHSEG_H__
#define MOZILLA_DOMSVGPATHSEG_H__

#include "DOMSVGPathSegList.h"
#include "nsAutoPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "SVGPathSegUtils.h"
#include "mozilla/dom/SVGPathSegBinding.h"

class nsSVGElement;

#define MOZ_SVG_LIST_INDEX_BIT_COUNT 31

namespace mozilla {

#define CHECK_ARG_COUNT_IN_SYNC(segType)                                      \
          NS_ABORT_IF_FALSE(ArrayLength(mArgs) ==                             \
            SVGPathSegUtils::ArgCountForType(uint32_t(segType)) ||            \
            uint32_t(segType) == PATHSEG_CLOSEPATH,                           \
            "Arg count/array size out of sync")

#define IMPL_SVGPATHSEG_SUBCLASS_COMMON(segName, segType)                     \
  explicit DOMSVGPathSeg##segName(const float *aArgs)                         \
    : DOMSVGPathSeg()                                                         \
  {                                                                           \
    CHECK_ARG_COUNT_IN_SYNC(segType);                                         \
    memcpy(mArgs, aArgs,                                                      \
        SVGPathSegUtils::ArgCountForType(uint32_t(segType)) * sizeof(float)); \
  }                                                                           \
  DOMSVGPathSeg##segName(DOMSVGPathSegList *aList,                            \
                         uint32_t aListIndex,                                 \
                         bool aIsAnimValItem)                                 \
    : DOMSVGPathSeg(aList, aListIndex, aIsAnimValItem)                        \
  {                                                                           \
    CHECK_ARG_COUNT_IN_SYNC(segType);                                         \
  }                                                                           \
  /* From DOMSVGPathSeg: */                                                   \
  virtual uint32_t                                                            \
  Type() const                                                                \
  {                                                                           \
    return segType;                                                           \
  }                                                                           \
  virtual DOMSVGPathSeg*                                                      \
  Clone()                                                                     \
  {                                                                           \
    /* InternalItem() + 1, because we're skipping the encoded seg type */     \
    float *args = IsInList() ? InternalItem() + 1 : mArgs;                    \
    return new DOMSVGPathSeg##segName(args);                                  \
  }                                                                           \
  virtual float*                                                              \
  PtrToMemberArgs()                                                           \
  {                                                                           \
    return mArgs;                                                             \
  }                                                                           \
                                                                              \
  virtual JSObject*                                                           \
  WrapObject(JSContext* aCx) MOZ_OVERRIDE       \
  {                                                                           \
    return dom::SVGPathSeg##segName##Binding::Wrap(aCx, this);        \
  }




















class DOMSVGPathSeg : public nsWrapperCache
{
  friend class AutoChangePathSegNotifier;

public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(DOMSVGPathSeg)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(DOMSVGPathSeg)

  




  static DOMSVGPathSeg *CreateFor(DOMSVGPathSegList *aList,
                                  uint32_t aListIndex,
                                  bool aIsAnimValItem);

  



  virtual DOMSVGPathSeg* Clone() = 0;

  bool IsInList() const {
    return !!mList;
  }

  



  bool HasOwner() const {
    return !!mList;
  }

  








  void InsertingIntoList(DOMSVGPathSegList *aList,
                         uint32_t aListIndex,
                         bool aIsAnimValItem);

  static uint32_t MaxListIndex() {
    return (1U << MOZ_SVG_LIST_INDEX_BIT_COUNT) - 1;
  }

  
  void UpdateListIndex(uint32_t aListIndex) {
    mListIndex = aListIndex;
  }

  





  void RemovingFromList();

  





  void ToSVGPathSegEncodedData(float *aData);

  


  virtual uint32_t Type() const = 0;

  
  DOMSVGPathSegList* GetParentObject() { return mList; }
  uint16_t PathSegType() const { return Type(); }
  void GetPathSegTypeAsLetter(nsAString &aPathSegTypeAsLetter)
    { aPathSegTypeAsLetter = SVGPathSegUtils::GetPathSegTypeAsLetter(Type()); }
  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE = 0;

protected:

  


  DOMSVGPathSeg(DOMSVGPathSegList *aList,
                uint32_t aListIndex,
                bool aIsAnimValItem);

  




  DOMSVGPathSeg();

  virtual ~DOMSVGPathSeg() {
    
    
    
    if (mList) {
      mList->ItemAt(mListIndex) = nullptr;
    }
  }

  nsSVGElement* Element() {
    return mList->Element();
  }

  








  float* InternalItem();

  void InvalidateCachedList() {
    mList->InternalList().mCachedPath = nullptr;
  }

  virtual float* PtrToMemberArgs() = 0;

#ifdef DEBUG
  bool IndexIsValid();
#endif

  nsRefPtr<DOMSVGPathSegList> mList;

  
  

  uint32_t mListIndex:MOZ_SVG_LIST_INDEX_BIT_COUNT;
  uint32_t mIsAnimValItem:1; 
};

class DOMSVGPathSegClosePath
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegClosePath()
    : DOMSVGPathSeg()
  {
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(ClosePath, PATHSEG_CLOSEPATH)

protected:
  
  
  
  float mArgs[1];
};

class DOMSVGPathSegMovetoAbs
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegMovetoAbs(float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x;
    mArgs[1] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(MovetoAbs, PATHSEG_MOVETO_ABS)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);

protected:
  float mArgs[2];
};

class DOMSVGPathSegMovetoRel
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegMovetoRel(float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x;
    mArgs[1] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(MovetoRel, PATHSEG_MOVETO_REL)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);

protected:
  float mArgs[2];
};

class DOMSVGPathSegLinetoAbs
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegLinetoAbs(float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x;
    mArgs[1] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(LinetoAbs, PATHSEG_LINETO_ABS)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);

protected:
  float mArgs[2];
};

class DOMSVGPathSegLinetoRel
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegLinetoRel(float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x;
    mArgs[1] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(LinetoRel, PATHSEG_LINETO_REL)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);

protected:
  float mArgs[2];
};

class DOMSVGPathSegCurvetoCubicAbs
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegCurvetoCubicAbs(float x1, float y1,
                               float x2, float y2,
                               float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x1;
    mArgs[1] = y1;
    mArgs[2] = x2;
    mArgs[3] = y2;
    mArgs[4] = x;
    mArgs[5] = y;
  }

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);
  float X1();
  void SetX1(float aX1, ErrorResult& rv);
  float Y1();
  void SetY1(float aY1, ErrorResult& rv);
  float X2();
  void SetX2(float aX2, ErrorResult& rv);
  float Y2();
  void SetY2(float aY2, ErrorResult& rv);

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(CurvetoCubicAbs, PATHSEG_CURVETO_CUBIC_ABS)

protected:
  float mArgs[6];
};

class DOMSVGPathSegCurvetoCubicRel
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegCurvetoCubicRel(float x1, float y1,
                               float x2, float y2,
                               float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x1;
    mArgs[1] = y1;
    mArgs[2] = x2;
    mArgs[3] = y2;
    mArgs[4] = x;
    mArgs[5] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(CurvetoCubicRel, PATHSEG_CURVETO_CUBIC_REL)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);
  float X1();
  void SetX1(float aX1, ErrorResult& rv);
  float Y1();
  void SetY1(float aY1, ErrorResult& rv);
  float X2();
  void SetX2(float aX2, ErrorResult& rv);
  float Y2();
  void SetY2(float aY2, ErrorResult& rv);

protected:
  float mArgs[6];
};

class DOMSVGPathSegCurvetoQuadraticAbs
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegCurvetoQuadraticAbs(float x1, float y1,
                                   float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x1;
    mArgs[1] = y1;
    mArgs[2] = x;
    mArgs[3] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(CurvetoQuadraticAbs, PATHSEG_CURVETO_QUADRATIC_ABS)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);
  float X1();
  void SetX1(float aX1, ErrorResult& rv);
  float Y1();
  void SetY1(float aY1, ErrorResult& rv);

protected:
  float mArgs[4];
};

class DOMSVGPathSegCurvetoQuadraticRel
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegCurvetoQuadraticRel(float x1, float y1,
                                   float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x1;
    mArgs[1] = y1;
    mArgs[2] = x;
    mArgs[3] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(CurvetoQuadraticRel, PATHSEG_CURVETO_QUADRATIC_REL)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);
  float X1();
  void SetX1(float aX1, ErrorResult& rv);
  float Y1();
  void SetY1(float aY1, ErrorResult& rv);

protected:
  float mArgs[4];
};

class DOMSVGPathSegArcAbs
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegArcAbs(float r1, float r2, float angle,
                      bool largeArcFlag, bool sweepFlag,
                      float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = r1;
    mArgs[1] = r2;
    mArgs[2] = angle;
    mArgs[3] = largeArcFlag;
    mArgs[4] = sweepFlag;
    mArgs[5] = x;
    mArgs[6] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(ArcAbs, PATHSEG_ARC_ABS)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);
  float R1();
  void SetR1(float aR1, ErrorResult& rv);
  float R2();
  void SetR2(float aR2, ErrorResult& rv);
  float Angle();
  void SetAngle(float aAngle, ErrorResult& rv);
  bool LargeArcFlag();
  void SetLargeArcFlag(bool aFlag, ErrorResult& rv);
  bool SweepFlag();
  void SetSweepFlag(bool aFlag, ErrorResult& rv);

protected:
  float mArgs[7];
};

class DOMSVGPathSegArcRel
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegArcRel(float r1, float r2, float angle,
                      bool largeArcFlag, bool sweepFlag,
                      float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = r1;
    mArgs[1] = r2;
    mArgs[2] = angle;
    mArgs[3] = largeArcFlag;
    mArgs[4] = sweepFlag;
    mArgs[5] = x;
    mArgs[6] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(ArcRel, PATHSEG_ARC_REL)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);
  float R1();
  void SetR1(float aR1, ErrorResult& rv);
  float R2();
  void SetR2(float aR2, ErrorResult& rv);
  float Angle();
  void SetAngle(float aAngle, ErrorResult& rv);
  bool LargeArcFlag();
  void SetLargeArcFlag(bool aFlag, ErrorResult& rv);
  bool SweepFlag();
  void SetSweepFlag(bool aFlag, ErrorResult& rv);

protected:
  float mArgs[7];
};

class DOMSVGPathSegLinetoHorizontalAbs
  : public DOMSVGPathSeg
{
public:
  explicit DOMSVGPathSegLinetoHorizontalAbs(float x)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(LinetoHorizontalAbs, PATHSEG_LINETO_HORIZONTAL_ABS)

  float X();
  void SetX(float aX, ErrorResult& rv);

protected:
  float mArgs[1];
};

class DOMSVGPathSegLinetoHorizontalRel
  : public DOMSVGPathSeg
{
public:
  explicit DOMSVGPathSegLinetoHorizontalRel(float x)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(LinetoHorizontalRel, PATHSEG_LINETO_HORIZONTAL_REL)

  float X();
  void SetX(float aX, ErrorResult& rv);

protected:
  float mArgs[1];
};

class DOMSVGPathSegLinetoVerticalAbs
  : public DOMSVGPathSeg
{
public:
  explicit DOMSVGPathSegLinetoVerticalAbs(float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(LinetoVerticalAbs, PATHSEG_LINETO_VERTICAL_ABS)

  float Y();
  void SetY(float aY, ErrorResult& rv);

protected:
  float mArgs[1];
};

class DOMSVGPathSegLinetoVerticalRel
  : public DOMSVGPathSeg
{
public:
  explicit DOMSVGPathSegLinetoVerticalRel(float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(LinetoVerticalRel, PATHSEG_LINETO_VERTICAL_REL)

  float Y();
  void SetY(float aY, ErrorResult& rv);

protected:
  float mArgs[1];
};

class DOMSVGPathSegCurvetoCubicSmoothAbs
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegCurvetoCubicSmoothAbs(float x2, float y2,
                                     float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x2;
    mArgs[1] = y2;
    mArgs[2] = x;
    mArgs[3] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(CurvetoCubicSmoothAbs, PATHSEG_CURVETO_CUBIC_SMOOTH_ABS)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);
  float X2();
  void SetX2(float aX2, ErrorResult& rv);
  float Y2();
  void SetY2(float aY2, ErrorResult& rv);

protected:
  float mArgs[4];
};

class DOMSVGPathSegCurvetoCubicSmoothRel
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegCurvetoCubicSmoothRel(float x2, float y2,
                                     float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x2;
    mArgs[1] = y2;
    mArgs[2] = x;
    mArgs[3] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(CurvetoCubicSmoothRel, PATHSEG_CURVETO_CUBIC_SMOOTH_REL)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);
  float X2();
  void SetX2(float aX2, ErrorResult& rv);
  float Y2();
  void SetY2(float aY2, ErrorResult& rv);

protected:
  float mArgs[4];
};

class DOMSVGPathSegCurvetoQuadraticSmoothAbs
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x;
    mArgs[1] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(CurvetoQuadraticSmoothAbs, PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);

protected:
  float mArgs[2];
};

class DOMSVGPathSegCurvetoQuadraticSmoothRel
  : public DOMSVGPathSeg
{
public:
  DOMSVGPathSegCurvetoQuadraticSmoothRel(float x, float y)
    : DOMSVGPathSeg()
  {
    mArgs[0] = x;
    mArgs[1] = y;
  }

  IMPL_SVGPATHSEG_SUBCLASS_COMMON(CurvetoQuadraticSmoothRel, PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL)

  float X();
  void SetX(float aX, ErrorResult& rv);
  float Y();
  void SetY(float aY, ErrorResult& rv);

protected:
  float mArgs[2];
};

} 

#undef MOZ_SVG_LIST_INDEX_BIT_COUNT

#endif 
