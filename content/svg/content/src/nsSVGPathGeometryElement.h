




#ifndef __NS_SVGPATHGEOMETRYELEMENT_H__
#define __NS_SVGPATHGEOMETRYELEMENT_H__

#include "mozilla/gfx/2D.h"
#include "SVGGraphicsElement.h"

class gfxMatrix;

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
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::FillRule FillRule;
  typedef mozilla::gfx::Float Float;
  typedef mozilla::gfx::Path Path;
  typedef mozilla::gfx::PathBuilder PathBuilder;

public:
  explicit nsSVGPathGeometryElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) MOZ_OVERRIDE;

  



  virtual void ClearAnyCachedPath() MOZ_OVERRIDE MOZ_FINAL {
    mCachedPath = nullptr;
  }

  virtual bool AttributeDefinesGeometry(const nsIAtom *aName);

  








  bool GeometryDependsOnCoordCtx();

  virtual bool IsMarkable();
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);

  




  virtual mozilla::TemporaryRef<Path> GetOrBuildPath(const DrawTarget& aDrawTarget,
                                                     FillRule fillRule);

  




  virtual mozilla::TemporaryRef<Path> BuildPath(PathBuilder* aBuilder) = 0;

  














  virtual mozilla::TemporaryRef<Path> GetOrBuildPathForMeasuring();

  



  FillRule GetFillRule();

protected:
  mutable mozilla::RefPtr<Path> mCachedPath;
};

#endif
