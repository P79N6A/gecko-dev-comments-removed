



#ifndef CanvasRenderingContext2D_h
#define CanvasRenderingContext2D_h

#include <vector>
#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsICanvasRenderingContextInternal.h"
#include "mozilla/RefPtr.h"
#include "nsColor.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "mozilla/dom/HTMLVideoElement.h"
#include "CanvasUtils.h"
#include "gfxFont.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/ImageData.h"
#include "mozilla/dom/UnionTypes.h"
#include "mozilla/dom/CanvasRenderingContext2DBinding.h"

#define NS_CANVASGRADIENTAZURE_PRIVATE_IID \
    {0x28425a6a, 0x90e0, 0x4d42, {0x9c, 0x75, 0xff, 0x60, 0x09, 0xb3, 0x10, 0xa8}}
#define NS_CANVASPATTERNAZURE_PRIVATE_IID \
    {0xc9bacc25, 0x28da, 0x421e, {0x9a, 0x4b, 0xbb, 0xd6, 0x93, 0x05, 0x12, 0xbc}}

class nsXULElement;

namespace mozilla {
namespace gfx {
struct Rect;
class SourceSurface;
}

namespace dom {
extern const mozilla::gfx::Float SIGMA_MAX;

template<typename T> class Optional;




class CanvasGradient : public nsIDOMCanvasGradient
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CANVASGRADIENTAZURE_PRIVATE_IID)

  enum Type
  {
    LINEAR = 0,
    RADIAL
  };

  Type GetType()
  {
    return mType;
  }


  mozilla::gfx::GradientStops *
  GetGradientStopsForTarget(mozilla::gfx::DrawTarget *aRT)
  {
    if (mStops && mStops->GetBackendType() == aRT->GetType()) {
      return mStops;
    }

    mStops = aRT->CreateGradientStops(mRawStops.Elements(), mRawStops.Length());

    return mStops;
  }

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD AddColorStop(float offset, const nsAString& colorstr);

protected:
  CanvasGradient(Type aType) : mType(aType)
  {}

  nsTArray<mozilla::gfx::GradientStop> mRawStops;
  mozilla::RefPtr<mozilla::gfx::GradientStops> mStops;
  Type mType;
  virtual ~CanvasGradient() {}
};




class CanvasPattern MOZ_FINAL : public nsIDOMCanvasPattern
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CANVASPATTERNAZURE_PRIVATE_IID)

  enum RepeatMode
  {
    REPEAT,
    REPEATX,
    REPEATY,
    NOREPEAT
  };

  CanvasPattern(mozilla::gfx::SourceSurface* aSurface,
                RepeatMode aRepeat,
                nsIPrincipal* principalForSecurityCheck,
                bool forceWriteOnly,
                bool CORSUsed)
    : mSurface(aSurface)
    , mRepeat(aRepeat)
    , mPrincipal(principalForSecurityCheck)
    , mForceWriteOnly(forceWriteOnly)
    , mCORSUsed(CORSUsed)
  {
  }

  NS_DECL_ISUPPORTS

  mozilla::RefPtr<mozilla::gfx::SourceSurface> mSurface;
  const RepeatMode mRepeat;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  const bool mForceWriteOnly;
  const bool mCORSUsed;
};

struct CanvasBidiProcessor;
class CanvasRenderingContext2DUserData;




class CanvasRenderingContext2D :
  public nsICanvasRenderingContextInternal,
  public nsWrapperCache
{
typedef mozilla::dom::HTMLImageElementOrHTMLCanvasElementOrHTMLVideoElement
  HTMLImageOrCanvasOrVideoElement;

public:
  CanvasRenderingContext2D();
  virtual ~CanvasRenderingContext2D();

  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

  HTMLCanvasElement* GetCanvas() const
  {
    
    return mCanvasElement->GetOriginalCanvas();
  }

  void Save();
  void Restore();
  void Scale(double x, double y, mozilla::ErrorResult& error);
  void Rotate(double angle, mozilla::ErrorResult& error);
  void Translate(double x, double y, mozilla::ErrorResult& error);
  void Transform(double m11, double m12, double m21, double m22, double dx,
                 double dy, mozilla::ErrorResult& error);
  void SetTransform(double m11, double m12, double m21, double m22, double dx,
                    double dy, mozilla::ErrorResult& error);

  double GlobalAlpha()
  {
    return CurrentState().globalAlpha;
  }

  
  static mozilla::gfx::Float ToFloat(double aValue) { return mozilla::gfx::Float(aValue); }

  void SetGlobalAlpha(double globalAlpha)
  {
    if (globalAlpha >= 0.0 && globalAlpha <= 1.0) {
      CurrentState().globalAlpha = ToFloat(globalAlpha);
    }
  }

  void GetGlobalCompositeOperation(nsAString& op, mozilla::ErrorResult& error);
  void SetGlobalCompositeOperation(const nsAString& op,
                                   mozilla::ErrorResult& error);
  JS::Value GetStrokeStyle(JSContext* cx, mozilla::ErrorResult& error);

  void SetStrokeStyle(JSContext* cx, JS::Value& value)
  {
    SetStyleFromJSValue(cx, value, STYLE_STROKE);
  }

  JS::Value GetFillStyle(JSContext* cx, mozilla::ErrorResult& error);

  void SetFillStyle(JSContext* cx, JS::Value& value)
  {
    SetStyleFromJSValue(cx, value, STYLE_FILL);
  }

  already_AddRefed<nsIDOMCanvasGradient>
    CreateLinearGradient(double x0, double y0, double x1, double y1,
                         mozilla::ErrorResult& aError);
  already_AddRefed<nsIDOMCanvasGradient>
    CreateRadialGradient(double x0, double y0, double r0, double x1, double y1,
                         double r1, mozilla::ErrorResult& aError);
  already_AddRefed<nsIDOMCanvasPattern>
    CreatePattern(const HTMLImageOrCanvasOrVideoElement& element,
                  const nsAString& repeat, mozilla::ErrorResult& error);

  double ShadowOffsetX()
  {
    return CurrentState().shadowOffset.x;
  }

  void SetShadowOffsetX(double shadowOffsetX)
  {
    CurrentState().shadowOffset.x = ToFloat(shadowOffsetX);
  }

  double ShadowOffsetY()
  {
    return CurrentState().shadowOffset.y;
  }

  void SetShadowOffsetY(double shadowOffsetY)
  {
    CurrentState().shadowOffset.y = ToFloat(shadowOffsetY);
  }

  double ShadowBlur()
  {
    return CurrentState().shadowBlur;
  }

  void SetShadowBlur(double shadowBlur)
  {
    if (shadowBlur >= 0.0) {
      CurrentState().shadowBlur = ToFloat(shadowBlur);
    }
  }

  void GetShadowColor(nsAString& shadowColor)
  {
    StyleColorToString(CurrentState().shadowColor, shadowColor);
  }

  void SetShadowColor(const nsAString& shadowColor);
  void ClearRect(double x, double y, double w, double h);
  void FillRect(double x, double y, double w, double h);
  void StrokeRect(double x, double y, double w, double h);
  void BeginPath();
  void Fill(const CanvasWindingRule& winding);
  void Stroke();
  void Clip(const CanvasWindingRule& winding);
  bool IsPointInPath(double x, double y, const CanvasWindingRule& winding);
  bool IsPointInStroke(double x, double y);
  void FillText(const nsAString& text, double x, double y,
                const mozilla::dom::Optional<double>& maxWidth,
                mozilla::ErrorResult& error);
  void StrokeText(const nsAString& text, double x, double y,
                  const mozilla::dom::Optional<double>& maxWidth,
                  mozilla::ErrorResult& error);
  already_AddRefed<nsIDOMTextMetrics>
    MeasureText(const nsAString& rawText, mozilla::ErrorResult& error);

  void DrawImage(const HTMLImageOrCanvasOrVideoElement& image,
                 double dx, double dy, mozilla::ErrorResult& error)
  {
    DrawImage(image, 0.0, 0.0, 0.0, 0.0, dx, dy, 0.0, 0.0, 0, error);
  }

  void DrawImage(const HTMLImageOrCanvasOrVideoElement& image,
                 double dx, double dy, double dw, double dh,
                 mozilla::ErrorResult& error)
  {
    DrawImage(image, 0.0, 0.0, 0.0, 0.0, dx, dy, dw, dh, 2, error);
  }

  void DrawImage(const HTMLImageOrCanvasOrVideoElement& image,
                 double sx, double sy, double sw, double sh, double dx,
                 double dy, double dw, double dh, mozilla::ErrorResult& error)
  {
    DrawImage(image, sx, sy, sw, sh, dx, dy, dw, dh, 6, error);
  }

  already_AddRefed<mozilla::dom::ImageData>
    CreateImageData(JSContext* cx, double sw, double sh,
                    mozilla::ErrorResult& error);
  already_AddRefed<mozilla::dom::ImageData>
    CreateImageData(JSContext* cx, mozilla::dom::ImageData& imagedata,
                    mozilla::ErrorResult& error);
  already_AddRefed<mozilla::dom::ImageData>
    GetImageData(JSContext* cx, double sx, double sy, double sw, double sh,
                 mozilla::ErrorResult& error);
  void PutImageData(mozilla::dom::ImageData& imageData,
                    double dx, double dy, mozilla::ErrorResult& error);
  void PutImageData(mozilla::dom::ImageData& imageData,
                    double dx, double dy, double dirtyX, double dirtyY,
                    double dirtyWidth, double dirtyHeight,
                    mozilla::ErrorResult& error);

  double LineWidth()
  {
    return CurrentState().lineWidth;
  }

  void SetLineWidth(double width)
  {
    if (width > 0.0) {
      CurrentState().lineWidth = ToFloat(width);
    }
  }
  void GetLineCap(nsAString& linecap);
  void SetLineCap(const nsAString& linecap);
  void GetLineJoin(nsAString& linejoin, mozilla::ErrorResult& error);
  void SetLineJoin(const nsAString& linejoin);

  double MiterLimit()
  {
    return CurrentState().miterLimit;
  }

  void SetMiterLimit(double miter)
  {
    if (miter > 0.0) {
      CurrentState().miterLimit = ToFloat(miter);
    }
  }

  void GetFont(nsAString& font)
  {
    font = GetFont();
  }

  void SetFont(const nsAString& font, mozilla::ErrorResult& error);
  void GetTextAlign(nsAString& textAlign);
  void SetTextAlign(const nsAString& textAlign);
  void GetTextBaseline(nsAString& textBaseline);
  void SetTextBaseline(const nsAString& textBaseline);

  void ClosePath()
  {
    EnsureWritablePath();

    if (mPathBuilder) {
      mPathBuilder->Close();
    } else {
      mDSPathBuilder->Close();
    }
  }

  void MoveTo(double x, double y)
  {
    EnsureWritablePath();

    if (mPathBuilder) {
      mPathBuilder->MoveTo(mozilla::gfx::Point(ToFloat(x), ToFloat(y)));
    } else {
      mDSPathBuilder->MoveTo(mTarget->GetTransform() *
                             mozilla::gfx::Point(ToFloat(x), ToFloat(y)));
    }
  }

  void LineTo(double x, double y)
  {
    EnsureWritablePath();
    
    LineTo(mozilla::gfx::Point(ToFloat(x), ToFloat(y)));
  }

  void QuadraticCurveTo(double cpx, double cpy, double x, double y)
  {
    EnsureWritablePath();

    if (mPathBuilder) {
      mPathBuilder->QuadraticBezierTo(mozilla::gfx::Point(ToFloat(cpx), ToFloat(cpy)),
                                      mozilla::gfx::Point(ToFloat(x), ToFloat(y)));
    } else {
      mozilla::gfx::Matrix transform = mTarget->GetTransform();
      mDSPathBuilder->QuadraticBezierTo(transform *
                                        mozilla::gfx::Point(ToFloat(cpx), ToFloat(cpy)),
                                        transform *
                                        mozilla::gfx::Point(ToFloat(x), ToFloat(y)));
    }
  }

  void BezierCurveTo(double cp1x, double cp1y, double cp2x, double cp2y, double x, double y)
  {
    EnsureWritablePath();

    BezierTo(mozilla::gfx::Point(ToFloat(cp1x), ToFloat(cp1y)),
             mozilla::gfx::Point(ToFloat(cp2x), ToFloat(cp2y)),
             mozilla::gfx::Point(ToFloat(x), ToFloat(y)));
  }

  void ArcTo(double x1, double y1, double x2, double y2, double radius,
             mozilla::ErrorResult& error);
  void Rect(double x, double y, double w, double h);
  void Arc(double x, double y, double radius, double startAngle,
           double endAngle, bool anticlockwise, mozilla::ErrorResult& error);

  JSObject* GetMozCurrentTransform(JSContext* cx,
                                   mozilla::ErrorResult& error) const;
  void SetMozCurrentTransform(JSContext* cx, JSObject& currentTransform,
                              mozilla::ErrorResult& error);
  JSObject* GetMozCurrentTransformInverse(JSContext* cx,
                                          mozilla::ErrorResult& error) const;
  void SetMozCurrentTransformInverse(JSContext* cx, JSObject& currentTransform, 
                                     mozilla::ErrorResult& error);
  void GetFillRule(nsAString& fillRule);
  void SetFillRule(const nsAString& fillRule);
  JS::Value GetMozDash(JSContext* cx, mozilla::ErrorResult& error);
  void SetMozDash(JSContext* cx, const JS::Value& mozDash,
                  mozilla::ErrorResult& error);

  double MozDashOffset()
  {
    return CurrentState().dashOffset;
  }
  void SetMozDashOffset(double mozDashOffset);

  void GetMozTextStyle(nsAString& mozTextStyle)
  {
    GetFont(mozTextStyle);
  }

  void SetMozTextStyle(const nsAString& mozTextStyle,
                       mozilla::ErrorResult& error)
  {
    SetFont(mozTextStyle, error);
  }

  bool ImageSmoothingEnabled()
  {
    return CurrentState().imageSmoothingEnabled;
  }

  void SetImageSmoothingEnabled(bool imageSmoothingEnabled)
  {
    if (imageSmoothingEnabled != CurrentState().imageSmoothingEnabled) {
      CurrentState().imageSmoothingEnabled = imageSmoothingEnabled;
    }
  }

  void DrawWindow(nsIDOMWindow* window, double x, double y, double w, double h,
                  const nsAString& bgColor, uint32_t flags,
                  mozilla::ErrorResult& error);
  void AsyncDrawXULElement(nsXULElement& elem, double x, double y, double w,
                           double h, const nsAString& bgColor, uint32_t flags,
                           mozilla::ErrorResult& error);

  nsresult Redraw();

  
  NS_IMETHOD SetDimensions(int32_t width, int32_t height);
  NS_IMETHOD InitializeWithSurface(nsIDocShell *shell, gfxASurface *surface, int32_t width, int32_t height);

  NS_IMETHOD Render(gfxContext *ctx,
                    gfxPattern::GraphicsFilter aFilter,
                    uint32_t aFlags = RenderFlagPremultAlpha);
  NS_IMETHOD GetInputStream(const char* aMimeType,
                            const PRUnichar* aEncoderOptions,
                            nsIInputStream **aStream);
  NS_IMETHOD GetThebesSurface(gfxASurface **surface);

  mozilla::TemporaryRef<mozilla::gfx::SourceSurface> GetSurfaceSnapshot()
  { EnsureTarget(); return mTarget->Snapshot(); }

  NS_IMETHOD SetIsOpaque(bool isOpaque);
  NS_IMETHOD Reset();
  already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                               CanvasLayer *aOldLayer,
                                               LayerManager *aManager);
  virtual bool ShouldForceInactiveLayer(LayerManager *aManager);
  void MarkContextClean();
  NS_IMETHOD SetIsIPC(bool isIPC);
  
  void Redraw(const mozilla::gfx::Rect &r);
  NS_IMETHOD Redraw(const gfxRect &r) { Redraw(ToRect(r)); return NS_OK; }

  
  void RedrawUser(const gfxRect &r);

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(CanvasRenderingContext2D)

  enum CanvasMultiGetterType {
    CMG_STYLE_STRING = 0,
    CMG_STYLE_PATTERN = 1,
    CMG_STYLE_GRADIENT = 2
  };

  enum Style {
    STYLE_STROKE = 0,
    STYLE_FILL,
    STYLE_MAX
  };

  nsINode* GetParentObject()
  {
    return mCanvasElement;
  }

  void LineTo(const mozilla::gfx::Point& aPoint)
  {
    if (mPathBuilder) {
      mPathBuilder->LineTo(aPoint);
    } else {
      mDSPathBuilder->LineTo(mTarget->GetTransform() * aPoint);
    }
  }

  void BezierTo(const mozilla::gfx::Point& aCP1,
                const mozilla::gfx::Point& aCP2,
                const mozilla::gfx::Point& aCP3)
  {
    if (mPathBuilder) {
      mPathBuilder->BezierTo(aCP1, aCP2, aCP3);
    } else {
      mozilla::gfx::Matrix transform = mTarget->GetTransform();
      mDSPathBuilder->BezierTo(transform * aCP1,
                                transform * aCP2,
                                transform * aCP3);
    }
  }

  friend class CanvasRenderingContext2DUserData;

protected:
  nsresult GetImageDataArray(JSContext* aCx, int32_t aX, int32_t aY,
                             uint32_t aWidth, uint32_t aHeight,
                             JSObject** aRetval);

  nsresult PutImageData_explicit(int32_t x, int32_t y, uint32_t w, uint32_t h,
                                 unsigned char *aData, uint32_t aDataLen,
                                 bool hasDirtyRect, int32_t dirtyX, int32_t dirtyY,
                                 int32_t dirtyWidth, int32_t dirtyHeight);

  


  nsresult Initialize(int32_t width, int32_t height);

  nsresult InitializeWithTarget(mozilla::gfx::DrawTarget *surface,
                                int32_t width, int32_t height);

  



  static uint32_t sNumLivingContexts;

  


  static uint8_t (*sUnpremultiplyTable)[256];

  


  static uint8_t (*sPremultiplyTable)[256];

  static mozilla::gfx::DrawTarget* sErrorTarget;

  
  void SetStyleFromJSValue(JSContext* cx, JS::Value& value, Style whichStyle);
  void SetStyleFromString(const nsAString& str, Style whichStyle);

  void SetStyleFromGradient(CanvasGradient *gradient, Style whichStyle)
  {
    CurrentState().SetGradientStyle(whichStyle, gradient);
  }

  void SetStyleFromPattern(CanvasPattern *pattern, Style whichStyle)
  {
    CurrentState().SetPatternStyle(whichStyle, pattern);
  }

  nsISupports* GetStyleAsStringOrInterface(nsAString& aStr, CanvasMultiGetterType& aType, Style aWhichStyle);

  
  bool ParseColor(const nsAString& aString, nscolor* aColor);

  static void StyleColorToString(const nscolor& aColor, nsAString& aStr);

  


  static void EnsureErrorTarget();

  




  void EnsureWritablePath();

  
  void EnsureUserSpacePath(const CanvasWindingRule& winding = CanvasWindingRuleValues::Nonzero);

  



  void TransformWillUpdate();

  
  void FillRuleChanged();

   





  void EnsureTarget();

  


  void ClearTarget();

  


  bool IsTargetValid() { return mTarget != sErrorTarget; }

  



  mozilla::gfx::SurfaceFormat GetSurfaceFormat() const;

  void DrawImage(const HTMLImageOrCanvasOrVideoElement &imgElt,
                 double sx, double sy, double sw, double sh,
                 double dx, double dy, double dw, double dh, 
                 uint8_t optional_argc, mozilla::ErrorResult& error);

  nsString& GetFont()
  {
    
    GetCurrentFontStyle();

    return CurrentState().font;
  }

  
  int32_t mWidth, mHeight;

  
  
  bool mZero;

  bool mOpaque;

  
  
  bool mResetLayer;
  
  bool mIPC;

  nsTArray<CanvasRenderingContext2DUserData*> mUserDatas;

  
  nsCOMPtr<nsIDocShell> mDocShell;

  
  
  
  mozilla::RefPtr<mozilla::gfx::DrawTarget> mTarget;

  



  bool mIsEntireFrameInvalid;
  




  bool mPredictManyRedrawCalls;

  
  
  nsRefPtr<gfxASurface> mThebesSurface;

  























  mozilla::RefPtr<mozilla::gfx::Path> mPath;
  mozilla::RefPtr<mozilla::gfx::PathBuilder> mDSPathBuilder;
  mozilla::RefPtr<mozilla::gfx::PathBuilder> mPathBuilder;
  bool mPathTransformWillUpdate;
  mozilla::gfx::Matrix mPathToDS;

  


  uint32_t mInvalidateCount;
  static const uint32_t kCanvasMaxInvalidateCount = 100;


#ifdef USE_SKIA_GPU
  nsRefPtr<gl::GLContext> mGLContext;
#endif

  



  bool NeedToDrawShadow()
  {
    const ContextState& state = CurrentState();

    
    
    return NS_GET_A(state.shadowColor) != 0 && 
      (state.shadowBlur != 0 || state.shadowOffset.x != 0 || state.shadowOffset.y != 0);
  }

  mozilla::gfx::CompositionOp UsedOperation()
  {
    if (NeedToDrawShadow()) {
      
      return mozilla::gfx::OP_OVER;
    }

    return CurrentState().op;
  }

  


  nsIPresShell *GetPresShell() {
    if (mCanvasElement) {
      return mCanvasElement->OwnerDoc()->GetShell();
    }
    if (mDocShell) {
      return mDocShell->GetPresShell();
    }
    return nullptr;
  }

  
  enum TextAlign {
    TEXT_ALIGN_START,
    TEXT_ALIGN_END,
    TEXT_ALIGN_LEFT,
    TEXT_ALIGN_RIGHT,
    TEXT_ALIGN_CENTER
  };

  enum TextBaseline {
    TEXT_BASELINE_TOP,
    TEXT_BASELINE_HANGING,
    TEXT_BASELINE_MIDDLE,
    TEXT_BASELINE_ALPHABETIC,
    TEXT_BASELINE_IDEOGRAPHIC,
    TEXT_BASELINE_BOTTOM
  };

  gfxFontGroup *GetCurrentFontStyle();

  enum TextDrawOperation {
    TEXT_DRAW_OPERATION_FILL,
    TEXT_DRAW_OPERATION_STROKE,
    TEXT_DRAW_OPERATION_MEASURE
  };

  



  nsresult DrawOrMeasureText(const nsAString& text,
                             float x,
                             float y,
                             const mozilla::dom::Optional<double>& maxWidth,
                             TextDrawOperation op,
                             float* aWidth);

  
  class ContextState {
  public:
    ContextState() : textAlign(TEXT_ALIGN_START),
                     textBaseline(TEXT_BASELINE_ALPHABETIC),
                     lineWidth(1.0f),
                     miterLimit(10.0f),
                     globalAlpha(1.0f),
                     shadowBlur(0.0),
                     dashOffset(0.0f),
                     op(mozilla::gfx::OP_OVER),
                     fillRule(mozilla::gfx::FILL_WINDING),
                     lineCap(mozilla::gfx::CAP_BUTT),
                     lineJoin(mozilla::gfx::JOIN_MITER_OR_BEVEL),
                     imageSmoothingEnabled(true)
    { }

    ContextState(const ContextState& other)
        : fontGroup(other.fontGroup),
          font(other.font),
          textAlign(other.textAlign),
          textBaseline(other.textBaseline),
          shadowColor(other.shadowColor),
          transform(other.transform),
          shadowOffset(other.shadowOffset),
          lineWidth(other.lineWidth),
          miterLimit(other.miterLimit),
          globalAlpha(other.globalAlpha),
          shadowBlur(other.shadowBlur),
          dash(other.dash),
          dashOffset(other.dashOffset),
          op(other.op),
          fillRule(other.fillRule),
          lineCap(other.lineCap),
          lineJoin(other.lineJoin),
          imageSmoothingEnabled(other.imageSmoothingEnabled)
    {
      for (int i = 0; i < STYLE_MAX; i++) {
        colorStyles[i] = other.colorStyles[i];
        gradientStyles[i] = other.gradientStyles[i];
        patternStyles[i] = other.patternStyles[i];
      }
    }

    void SetColorStyle(Style whichStyle, nscolor color)
    {
      colorStyles[whichStyle] = color;
      gradientStyles[whichStyle] = nullptr;
      patternStyles[whichStyle] = nullptr;
    }

    void SetPatternStyle(Style whichStyle, CanvasPattern* pat)
    {
      gradientStyles[whichStyle] = nullptr;
      patternStyles[whichStyle] = pat;
    }

    void SetGradientStyle(Style whichStyle, CanvasGradient* grad)
    {
      gradientStyles[whichStyle] = grad;
      patternStyles[whichStyle] = nullptr;
    }

    


    bool StyleIsColor(Style whichStyle) const
    {
      return !(patternStyles[whichStyle] || gradientStyles[whichStyle]);
    }


    std::vector<mozilla::RefPtr<mozilla::gfx::Path> > clipsPushed;

    nsRefPtr<gfxFontGroup> fontGroup;
    nsRefPtr<CanvasGradient> gradientStyles[STYLE_MAX];
    nsRefPtr<CanvasPattern> patternStyles[STYLE_MAX];

    nsString font;
    TextAlign textAlign;
    TextBaseline textBaseline;

    nscolor colorStyles[STYLE_MAX];
    nscolor shadowColor;

    mozilla::gfx::Matrix transform;
    mozilla::gfx::Point shadowOffset;
    mozilla::gfx::Float lineWidth;
    mozilla::gfx::Float miterLimit;
    mozilla::gfx::Float globalAlpha;
    mozilla::gfx::Float shadowBlur;
    FallibleTArray<mozilla::gfx::Float> dash;
    mozilla::gfx::Float dashOffset;

    mozilla::gfx::CompositionOp op;
    mozilla::gfx::FillRule fillRule;
    mozilla::gfx::CapStyle lineCap;
    mozilla::gfx::JoinStyle lineJoin;

    bool imageSmoothingEnabled;
  };

  nsAutoTArray<ContextState, 3> mStyleStack;

  inline ContextState& CurrentState() {
    return mStyleStack[mStyleStack.Length() - 1];
  }

  friend class CanvasGeneralPattern;
  friend class AdjustedTarget;

  
  void GetAppUnitsValues(int32_t *perDevPixel, int32_t *perCSSPixel)
  {
    
    int32_t devPixel = 60;
    int32_t cssPixel = 60;

    nsIPresShell *ps = GetPresShell();
    nsPresContext *pc;

    if (!ps) goto FINISH;
    pc = ps->GetPresContext();
    if (!pc) goto FINISH;
    devPixel = pc->AppUnitsPerDevPixel();
    cssPixel = pc->AppUnitsPerCSSPixel();

  FINISH:
    if (perDevPixel)
      *perDevPixel = devPixel;
    if (perCSSPixel)
      *perCSSPixel = cssPixel;
  }

  friend struct CanvasBidiProcessor;
};

}
}

#endif 
