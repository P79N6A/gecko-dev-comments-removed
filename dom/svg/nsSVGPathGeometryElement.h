




#ifndef __NS_SVGPATHGEOMETRYELEMENT_H__
#define __NS_SVGPATHGEOMETRYELEMENT_H__

#include "mozilla/gfx/2D.h"
#include "SVGGraphicsElement.h"

struct nsSVGMark {
  enum Type {
    eStart,
    eMid,
    eEnd,

    eTypeCount
  };

  float x, y, angle;
  Type type;
  nsSVGMark(float aX, float aY, float aAngle, Type aType) :
    x(aX), y(aY), angle(aAngle), type(aType) {}
};

typedef mozilla::dom::SVGGraphicsElement nsSVGPathGeometryElementBase;

class nsSVGPathGeometryElement : public nsSVGPathGeometryElementBase
{
protected:
  typedef mozilla::gfx::CapStyle CapStyle;
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::FillRule FillRule;
  typedef mozilla::gfx::Float Float;
  typedef mozilla::gfx::Matrix Matrix;
  typedef mozilla::gfx::Path Path;
  typedef mozilla::gfx::Point Point;
  typedef mozilla::gfx::PathBuilder PathBuilder;
  typedef mozilla::gfx::Rect Rect;
  typedef mozilla::gfx::StrokeOptions StrokeOptions;

public:
  explicit nsSVGPathGeometryElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) override;

  



  virtual void ClearAnyCachedPath() override final {
    mCachedPath = nullptr;
  }

  virtual bool AttributeDefinesGeometry(const nsIAtom *aName);

  








  bool GeometryDependsOnCoordCtx();

  virtual bool IsMarkable();
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);

  





  virtual bool GetGeometryBounds(Rect* aBounds, const StrokeOptions& aStrokeOptions,
                                 const Matrix& aTransform) {
    return false;
  }

  


  class SimplePath
  {
  public:
    SimplePath()
      : mType(NONE)
    {}
    bool IsPath() const {
      return mType != NONE;
    }
    void SetRect(Float x, Float y, Float width, Float height) {
      mX = x; mY = y, mWidthOrX2 = width, mHeightOrY2 = height;
      mType = RECT;
    }
    Rect AsRect() const {
      MOZ_ASSERT(mType == RECT);
      return Rect(mX, mY, mWidthOrX2, mHeightOrY2);
    }
    bool IsRect() const {
      return mType == RECT;
    }
    void SetLine(Float x1, Float y1, Float x2, Float y2) {
      mX = x1, mY = y1, mWidthOrX2 = x2, mHeightOrY2 = y2;
      mType = LINE;
    }
    Point Point1() const {
      MOZ_ASSERT(mType == LINE);
      return Point(mX, mY);
    }
    Point Point2() const {
      MOZ_ASSERT(mType == LINE);
      return Point(mWidthOrX2, mHeightOrY2);
    }
    bool IsLine() const {
      return mType == LINE;
    }
    void Reset() {
      mType = NONE;
    }
  private:
    enum Type {
      NONE, RECT, LINE
    };
    Float mX, mY, mWidthOrX2, mHeightOrY2;
    Type mType;
  };

  





  virtual void GetAsSimplePath(SimplePath* aSimplePath) {
    aSimplePath->Reset();
  }

  




  virtual mozilla::TemporaryRef<Path> GetOrBuildPath(const DrawTarget& aDrawTarget,
                                                     FillRule fillRule);

  




  virtual mozilla::TemporaryRef<Path> BuildPath(PathBuilder* aBuilder) = 0;

  














  virtual mozilla::TemporaryRef<Path> GetOrBuildPathForMeasuring();

  



  FillRule GetFillRule();

protected:
  mutable mozilla::RefPtr<Path> mCachedPath;
};

#endif
