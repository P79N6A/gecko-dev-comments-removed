



#ifndef CanvasRenderingContext2D_h
#define CanvasRenderingContext2D_h

#include "mozilla/Attributes.h"
#include <vector>
#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsICanvasRenderingContextInternal.h"
#include "mozilla/RefPtr.h"
#include "nsColor.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "mozilla/dom/HTMLVideoElement.h"
#include "CanvasUtils.h"
#include "gfxTextRun.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/CanvasGradient.h"
#include "mozilla/dom/CanvasRenderingContext2DBinding.h"
#include "mozilla/dom/CanvasPattern.h"
#include "mozilla/gfx/Rect.h"
#include "mozilla/gfx/2D.h"
#include "gfx2DGlue.h"
#include "imgIEncoder.h"
#include "nsLayoutUtils.h"
#include "mozilla/EnumeratedArray.h"
#include "FilterSupport.h"
#include "nsSVGEffects.h"

class nsGlobalWindow;
class nsXULElement;

namespace mozilla {
namespace gl {
class SourceSurface;
}

namespace dom {
class HTMLImageElementOrHTMLCanvasElementOrHTMLVideoElement;
class ImageData;
class StringOrCanvasGradientOrCanvasPattern;
class OwningStringOrCanvasGradientOrCanvasPattern;
class TextMetrics;
class CanvasFilterChainObserver;
class CanvasPath;

extern const mozilla::gfx::Float SIGMA_MAX;

template<typename T> class Optional;

struct CanvasBidiProcessor;
class CanvasRenderingContext2DUserData;
class CanvasDrawObserver;




class CanvasRenderingContext2D final :
  public nsICanvasRenderingContextInternal,
  public nsWrapperCache
{
typedef HTMLImageElementOrHTMLCanvasElementOrHTMLVideoElement
  HTMLImageOrCanvasOrVideoElement;

  virtual ~CanvasRenderingContext2D();

public:
  CanvasRenderingContext2D();

  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

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
  void ResetTransform(mozilla::ErrorResult& error);

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

  void GetStrokeStyle(OwningStringOrCanvasGradientOrCanvasPattern& value)
  {
    GetStyleAsUnion(value, Style::STROKE);
  }

  void SetStrokeStyle(const StringOrCanvasGradientOrCanvasPattern& value)
  {
    SetStyleFromUnion(value, Style::STROKE);
  }

  void GetFillStyle(OwningStringOrCanvasGradientOrCanvasPattern& value)
  {
    GetStyleAsUnion(value, Style::FILL);
  }

  void SetFillStyle(const StringOrCanvasGradientOrCanvasPattern& value)
  {
    SetStyleFromUnion(value, Style::FILL);
  }

  already_AddRefed<CanvasGradient>
    CreateLinearGradient(double x0, double y0, double x1, double y1);
  already_AddRefed<CanvasGradient>
    CreateRadialGradient(double x0, double y0, double r0, double x1, double y1,
                         double r1, ErrorResult& aError);
  already_AddRefed<CanvasPattern>
    CreatePattern(const HTMLImageOrCanvasOrVideoElement& element,
                  const nsAString& repeat, ErrorResult& error);

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

  void GetFilter(nsAString& filter)
  {
    filter = CurrentState().filterString;
  }

  void SetShadowColor(const nsAString& shadowColor);
  void SetFilter(const nsAString& filter, mozilla::ErrorResult& error);
  void ClearRect(double x, double y, double w, double h);
  void FillRect(double x, double y, double w, double h);
  void StrokeRect(double x, double y, double w, double h);
  void BeginPath();
  void Fill(const CanvasWindingRule& winding);
  void Fill(const CanvasPath& path, const CanvasWindingRule& winding);
  void Stroke();
  void Stroke(const CanvasPath& path);
  void DrawFocusIfNeeded(mozilla::dom::Element& element);
  bool DrawCustomFocusRing(mozilla::dom::Element& element);
  void Clip(const CanvasWindingRule& winding);
  void Clip(const CanvasPath& path, const CanvasWindingRule& winding);
  bool IsPointInPath(double x, double y, const CanvasWindingRule& winding);
  bool IsPointInPath(const CanvasPath& path, double x, double y, const CanvasWindingRule& winding);
  bool IsPointInStroke(double x, double y);
  bool IsPointInStroke(const CanvasPath& path, double x, double y);
  void FillText(const nsAString& text, double x, double y,
                const Optional<double>& maxWidth,
                mozilla::ErrorResult& error);
  void StrokeText(const nsAString& text, double x, double y,
                  const Optional<double>& maxWidth,
                  mozilla::ErrorResult& error);
  TextMetrics*
    MeasureText(const nsAString& rawText, mozilla::ErrorResult& error);

  void AddHitRegion(const HitRegionOptions& options, mozilla::ErrorResult& error);
  void RemoveHitRegion(const nsAString& id);
  void ClearHitRegions();

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

  already_AddRefed<ImageData>
    CreateImageData(JSContext* cx, double sw, double sh,
                    mozilla::ErrorResult& error);
  already_AddRefed<ImageData>
    CreateImageData(JSContext* cx, ImageData& imagedata,
                    mozilla::ErrorResult& error);
  already_AddRefed<ImageData>
    GetImageData(JSContext* cx, double sx, double sy, double sw, double sh,
                 mozilla::ErrorResult& error);
  void PutImageData(ImageData& imageData,
                    double dx, double dy, mozilla::ErrorResult& error);
  void PutImageData(ImageData& imageData,
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

  void GetMozCurrentTransform(JSContext* cx,
                              JS::MutableHandle<JSObject*> result,
                              mozilla::ErrorResult& error) const;
  void SetMozCurrentTransform(JSContext* cx,
                              JS::Handle<JSObject*> currentTransform,
                              mozilla::ErrorResult& error);
  void GetMozCurrentTransformInverse(JSContext* cx,
                                     JS::MutableHandle<JSObject*> result,
                                     mozilla::ErrorResult& error) const;
  void SetMozCurrentTransformInverse(JSContext* cx,
                                     JS::Handle<JSObject*> currentTransform,
                                     mozilla::ErrorResult& error);
  void GetFillRule(nsAString& fillRule);
  void SetFillRule(const nsAString& fillRule);
  void GetMozDash(JSContext* cx, JS::MutableHandle<JS::Value> retval,
                  mozilla::ErrorResult& error);
  void SetMozDash(JSContext* cx, const JS::Value& mozDash,
                  mozilla::ErrorResult& error);

  void SetLineDash(const Sequence<double>& mSegments);
  void GetLineDash(nsTArray<double>& mSegments) const;

  void SetLineDashOffset(double mOffset);
  double LineDashOffset() const;

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

  void DrawWindow(nsGlobalWindow& window, double x, double y, double w, double h,
                  const nsAString& bgColor, uint32_t flags,
                  mozilla::ErrorResult& error);
  void AsyncDrawXULElement(nsXULElement& elem, double x, double y, double w,
                           double h, const nsAString& bgColor, uint32_t flags,
                           mozilla::ErrorResult& error);

  enum RenderingMode {
    SoftwareBackendMode,
    OpenGLBackendMode,
    DefaultBackendMode
  };

  bool SwitchRenderingMode(RenderingMode aRenderingMode);

  
  void Demote();

  nsresult Redraw();

#ifdef DEBUG
    virtual int32_t GetWidth() const override;
    virtual int32_t GetHeight() const override;
#endif
  
  


  virtual nsIPresShell *GetPresShell() override {
    if (mCanvasElement) {
      return mCanvasElement->OwnerDoc()->GetShell();
    }
    if (mDocShell) {
      return mDocShell->GetPresShell();
    }
    return nullptr;
  }
  NS_IMETHOD SetDimensions(int32_t width, int32_t height) override;
  NS_IMETHOD InitializeWithSurface(nsIDocShell *shell, gfxASurface *surface, int32_t width, int32_t height) override;

  NS_IMETHOD GetInputStream(const char* aMimeType,
                            const char16_t* aEncoderOptions,
                            nsIInputStream **aStream) override;

  mozilla::TemporaryRef<mozilla::gfx::SourceSurface> GetSurfaceSnapshot(bool* aPremultAlpha = nullptr) override
  {
    EnsureTarget();
    if (aPremultAlpha) {
      *aPremultAlpha = true;
    }
    return mTarget->Snapshot();
  }

  NS_IMETHOD SetIsOpaque(bool isOpaque) override;
  bool GetIsOpaque() override { return mOpaque; }
  NS_IMETHOD Reset() override;
  already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                               CanvasLayer *aOldLayer,
                                               LayerManager *aManager) override;
  virtual bool ShouldForceInactiveLayer(LayerManager *aManager) override;
  void MarkContextClean() override;
  NS_IMETHOD SetIsIPC(bool isIPC) override;
  
  void Redraw(const mozilla::gfx::Rect &r);
  NS_IMETHOD Redraw(const gfxRect &r) override { Redraw(ToRect(r)); return NS_OK; }
  NS_IMETHOD SetContextOptions(JSContext* aCx, JS::Handle<JS::Value> aOptions) override;

  




  virtual void DidRefresh() override;

  
  void RedrawUser(const gfxRect &r);

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(CanvasRenderingContext2D)

  enum class CanvasMultiGetterType : uint8_t {
    STRING = 0,
    PATTERN = 1,
    GRADIENT = 2
  };

  enum class Style : uint8_t {
    STROKE = 0,
    FILL,
    MAX
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

  virtual void GetImageBuffer(uint8_t** aImageBuffer, int32_t* aFormat) override;


  
  nsString GetHitRegion(const mozilla::gfx::Point& aPoint) override;


  
  bool GetHitRegionRect(Element* aElement, nsRect& aRect) override;

protected:
  nsresult GetImageDataArray(JSContext* aCx, int32_t aX, int32_t aY,
                             uint32_t aWidth, uint32_t aHeight,
                             JSObject** aRetval);

  nsresult PutImageData_explicit(int32_t x, int32_t y, uint32_t w, uint32_t h,
                                 dom::Uint8ClampedArray* aArray,
                                 bool hasDirtyRect, int32_t dirtyX, int32_t dirtyY,
                                 int32_t dirtyWidth, int32_t dirtyHeight);

  


  nsresult Initialize(int32_t width, int32_t height);

  nsresult InitializeWithTarget(mozilla::gfx::DrawTarget *surface,
                                int32_t width, int32_t height);

  



  static uint32_t sNumLivingContexts;

  


  static uint8_t (*sUnpremultiplyTable)[256];

  


  static uint8_t (*sPremultiplyTable)[256];

  static mozilla::gfx::DrawTarget* sErrorTarget;

  
  void SetStyleFromUnion(const StringOrCanvasGradientOrCanvasPattern& value,
                         Style whichStyle);
  void SetStyleFromString(const nsAString& str, Style whichStyle);

  void SetStyleFromGradient(CanvasGradient& gradient, Style whichStyle)
  {
    CurrentState().SetGradientStyle(whichStyle, &gradient);
  }

  void SetStyleFromPattern(CanvasPattern& pattern, Style whichStyle)
  {
    CurrentState().SetPatternStyle(whichStyle, &pattern);
  }

  void GetStyleAsUnion(OwningStringOrCanvasGradientOrCanvasPattern& aValue,
                       Style aWhichStyle);

  
  bool ParseColor(const nsAString& aString, nscolor* aColor);

  static void StyleColorToString(const nscolor& aColor, nsAString& aStr);

   
  bool ParseFilter(const nsAString& aString,
                   nsTArray<nsStyleFilter>& aFilterChain,
                   ErrorResult& error);

  


  static void EnsureErrorTarget();

  




  void EnsureWritablePath();

  
  void EnsureUserSpacePath(const CanvasWindingRule& winding = CanvasWindingRule::Nonzero);

  



  void TransformWillUpdate();

  
  void FillRuleChanged();

   







  RenderingMode EnsureTarget(RenderingMode aRenderMode = RenderingMode::DefaultBackendMode);

  


  void ClearTarget();

  


  bool IsTargetValid() const {
    return mTarget != sErrorTarget && mTarget != nullptr;
  }

  



  mozilla::gfx::SurfaceFormat GetSurfaceFormat() const;

  



  void UpdateFilter();

  nsLayoutUtils::SurfaceFromElementResult
    CachedSurfaceFromElement(Element* aElement);

  void DrawImage(const HTMLImageOrCanvasOrVideoElement &imgElt,
                 double sx, double sy, double sw, double sh,
                 double dx, double dy, double dw, double dh,
                 uint8_t optional_argc, mozilla::ErrorResult& error);

  void DrawDirectlyToCanvas(const nsLayoutUtils::DirectDrawInfo& image,
                            mozilla::gfx::Rect* bounds,
                            mozilla::gfx::Rect dest,
                            mozilla::gfx::Rect src,
                            gfx::IntSize imgSize);

  nsString& GetFont()
  {
    
    GetCurrentFontStyle();

    return CurrentState().font;
  }

  static std::vector<CanvasRenderingContext2D*>& DemotableContexts();
  static void DemoteOldestContextIfNecessary();

  static void AddDemotableContext(CanvasRenderingContext2D* context);
  static void RemoveDemotableContext(CanvasRenderingContext2D* context);

  RenderingMode mRenderingMode;

  
  unsigned int mVideoTexture;
  nsIntSize mCurrentVideoSize;

  
  int32_t mWidth, mHeight;

  
  
  bool mZero;

  bool mOpaque;

  
  
  bool mResetLayer;
  
  bool mIPC;

  nsTArray<CanvasRenderingContext2DUserData*> mUserDatas;

  
  nsCOMPtr<nsIDocShell> mDocShell;

  
  
  
  mozilla::RefPtr<mozilla::gfx::DrawTarget> mTarget;

  uint32_t SkiaGLTex() const;

  
  
  
  CanvasDrawObserver* mDrawObserver;
  void RemoveDrawObserver();

  



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

  


  struct RegionInfo
  {
    nsString          mId;
    
    nsRefPtr<Element> mElement;
    
    RefPtr<gfx::Path> mPath;
  };

  nsTArray<RegionInfo> mHitRegionsOptions;

  



  bool NeedToDrawShadow()
  {
    const ContextState& state = CurrentState();

    
    
    return NS_GET_A(state.shadowColor) != 0 &&
      (state.shadowBlur != 0.f || state.shadowOffset.x != 0.f || state.shadowOffset.y != 0.f);
  }

  



  bool NeedToApplyFilter()
  {
    const ContextState& state = CurrentState();
    return state.filter.mPrimitives.Length() > 0;
  }

  bool NeedToCalculateBounds()
  {
    return NeedToDrawShadow() || NeedToApplyFilter();
  }

  mozilla::gfx::CompositionOp UsedOperation()
  {
    if (NeedToDrawShadow() || NeedToApplyFilter()) {
      
      return mozilla::gfx::CompositionOp::OP_OVER;
    }

    return CurrentState().op;
  }

  

protected:
  enum class TextAlign : uint8_t {
    START,
    END,
    LEFT,
    RIGHT,
    CENTER
  };

  enum class TextBaseline : uint8_t {
    TOP,
    HANGING,
    MIDDLE,
    ALPHABETIC,
    IDEOGRAPHIC,
    BOTTOM
  };

  enum class TextDrawOperation : uint8_t {
    FILL,
    STROKE,
    MEASURE
  };

protected:
  gfxFontGroup *GetCurrentFontStyle();

  



  nsresult DrawOrMeasureText(const nsAString& text,
                             float x,
                             float y,
                             const Optional<double>& maxWidth,
                             TextDrawOperation op,
                             float* aWidth);

  bool CheckSizeForSkiaGL(mozilla::gfx::IntSize size);

  
  class ContextState {
  public:
    ContextState() : textAlign(TextAlign::START),
                     textBaseline(TextBaseline::ALPHABETIC),
                     lineWidth(1.0f),
                     miterLimit(10.0f),
                     globalAlpha(1.0f),
                     shadowBlur(0.0),
                     dashOffset(0.0f),
                     op(mozilla::gfx::CompositionOp::OP_OVER),
                     fillRule(mozilla::gfx::FillRule::FILL_WINDING),
                     lineCap(mozilla::gfx::CapStyle::BUTT),
                     lineJoin(mozilla::gfx::JoinStyle::MITER_OR_BEVEL),
                     imageSmoothingEnabled(true),
                     fontExplicitLanguage(false)
    { }

    ContextState(const ContextState& other)
        : fontGroup(other.fontGroup),
          fontLanguage(other.fontLanguage),
          fontFont(other.fontFont),
          gradientStyles(other.gradientStyles),
          patternStyles(other.patternStyles),
          colorStyles(other.colorStyles),
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
          filterString(other.filterString),
          filterChain(other.filterChain),
          filterChainObserver(other.filterChainObserver),
          filter(other.filter),
          filterAdditionalImages(other.filterAdditionalImages),
          imageSmoothingEnabled(other.imageSmoothingEnabled),
          fontExplicitLanguage(other.fontExplicitLanguage)
    { }

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

    int32_t ShadowBlurRadius() const
    {
      static const gfxFloat GAUSSIAN_SCALE_FACTOR = (3 * sqrt(2 * M_PI) / 4) * 1.5;
      return (int32_t)floor(ShadowBlurSigma() * GAUSSIAN_SCALE_FACTOR + 0.5);
    }

    mozilla::gfx::Float ShadowBlurSigma() const
    {
      return std::min(SIGMA_MAX, shadowBlur / 2.0f);
    }

    std::vector<mozilla::RefPtr<mozilla::gfx::Path> > clipsPushed;

    nsRefPtr<gfxFontGroup> fontGroup;
    nsCOMPtr<nsIAtom> fontLanguage;
    nsFont fontFont;

    EnumeratedArray<Style, Style::MAX, nsRefPtr<CanvasGradient>> gradientStyles;
    EnumeratedArray<Style, Style::MAX, nsRefPtr<CanvasPattern>> patternStyles;
    EnumeratedArray<Style, Style::MAX, nscolor> colorStyles;

    nsString font;
    TextAlign textAlign;
    TextBaseline textBaseline;

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

    nsString filterString;
    nsTArray<nsStyleFilter> filterChain;
    nsRefPtr<nsSVGFilterChainObserver> filterChainObserver;
    mozilla::gfx::FilterDescription filter;
    nsTArray<mozilla::RefPtr<mozilla::gfx::SourceSurface>> filterAdditionalImages;

    bool imageSmoothingEnabled;
    bool fontExplicitLanguage;
  };

  nsAutoTArray<ContextState, 3> mStyleStack;

  inline ContextState& CurrentState() {
    return mStyleStack[mStyleStack.Length() - 1];
  }

  inline const ContextState& CurrentState() const {
    return mStyleStack[mStyleStack.Length() - 1];
  }

  friend class CanvasGeneralPattern;
  friend class CanvasFilterChainObserver;
  friend class AdjustedTarget;
  friend class AdjustedTargetForShadow;
  friend class AdjustedTargetForFilter;

  
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
  friend class CanvasDrawObserver;
};

}
}

#endif
