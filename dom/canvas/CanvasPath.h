



#ifndef CanvasPath_h
#define CanvasPath_h

#include "mozilla/Attributes.h"
#include "mozilla/RefPtr.h"
#include "nsWrapperCache.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/ErrorResult.h"

namespace mozilla {
namespace dom {

enum class CanvasWindingRule : uint32_t;
class SVGMatrix;

class CanvasPath final :
  public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(CanvasPath)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(CanvasPath)

  nsCOMPtr<nsISupports> GetParentObject() { return mParent; }

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<CanvasPath> Constructor(const GlobalObject& aGlobal,
                                                  ErrorResult& rv);
  static already_AddRefed<CanvasPath> Constructor(const GlobalObject& aGlobal,
                                                  CanvasPath& aCanvasPath,
                                                  ErrorResult& rv);
  static already_AddRefed<CanvasPath> Constructor(const GlobalObject& aGlobal,
                                                  const nsAString& aPathString,
                                                  ErrorResult& rv);

  void ClosePath();
  void MoveTo(double x, double y);
  void LineTo(double x, double y);
  void QuadraticCurveTo(double cpx, double cpy, double x, double y);
  void BezierCurveTo(double cp1x, double cp1y,
                     double cp2x, double cp2y,
                     double x, double y);
  void ArcTo(double x1, double y1, double x2, double y2, double radius,
             ErrorResult& error);
  void Rect(double x, double y, double w, double h);
  void Arc(double x, double y, double radius,
           double startAngle, double endAngle, bool anticlockwise,
           ErrorResult& error);

  void LineTo(const gfx::Point& aPoint);
  void BezierTo(const gfx::Point& aCP1,
                const gfx::Point& aCP2,
                const gfx::Point& aCP3);

  already_AddRefed<gfx::Path> GetPath(const CanvasWindingRule& aWinding,
                                  const gfx::DrawTarget* aTarget) const;

  explicit CanvasPath(nsISupports* aParent);
  
  
  CanvasPath(nsISupports* aParent,
             already_AddRefed<gfx::PathBuilder> aPathBuilder);

  void AddPath(CanvasPath& aCanvasPath,
               const Optional<NonNull<SVGMatrix>>& aMatrix);

private:
  virtual ~CanvasPath() {}

  nsCOMPtr<nsISupports> mParent;
  static gfx::Float ToFloat(double aValue) { return gfx::Float(aValue); }

  mutable RefPtr<gfx::Path> mPath;
  mutable RefPtr<gfx::PathBuilder> mPathBuilder;

  void EnsurePathBuilder() const;
};

} 
} 

#endif 

