







































#include "base/basictypes.h"

#include "nsIDOMXULElement.h"

#include "prmem.h"
#include "prenv.h"

#include "nsIServiceManager.h"
#include "nsMathUtils.h"

#include "nsContentUtils.h"

#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsICanvasRenderingContextInternal.h"
#include "nsHTMLCanvasElement.h"
#include "nsSVGEffects.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIVariant.h"

#include "nsIInterfaceRequestorUtils.h"
#include "nsIFrame.h"
#include "nsDOMError.h"
#include "nsIScriptError.h"

#include "nsCSSParser.h"
#include "mozilla/css/StyleRule.h"
#include "mozilla/css/Declaration.h"
#include "nsComputedDOMStyle.h"
#include "nsStyleSet.h"

#include "nsPrintfCString.h"

#include "nsReadableUtils.h"

#include "nsColor.h"
#include "nsGfxCIID.h"
#include "nsIScriptSecurityManager.h"
#include "nsIDocShell.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIXPConnect.h"
#include "jsapi.h"
#include "nsDisplayList.h"

#include "nsTArray.h"

#include "imgIEncoder.h"

#include "gfxContext.h"
#include "gfxASurface.h"
#include "gfxImageSurface.h"
#include "gfxPlatform.h"
#include "gfxFont.h"
#include "gfxTextRunCache.h"
#include "gfxBlur.h"
#include "gfxUtils.h"

#include "nsFrameManager.h"
#include "nsFrameLoader.h"
#include "nsBidi.h"
#include "nsBidiPresUtils.h"
#include "Layers.h"
#include "CanvasUtils.h"
#include "nsIMemoryReporter.h"
#include "nsStyleUtil.h"
#include "CanvasImageCache.h"

#include <algorithm>
#include "mozilla/dom/ContentParent.h"
#include "mozilla/ipc/PDocumentRendererParent.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/ipc/DocumentRendererParent.h"


#undef DrawText

using namespace mozilla;
using namespace mozilla::CanvasUtils;
using namespace mozilla::dom;
using namespace mozilla::ipc;
using namespace mozilla::layers;

static float kDefaultFontSize = 10.0;
static NS_NAMED_LITERAL_STRING(kDefaultFontName, "sans-serif");
static NS_NAMED_LITERAL_STRING(kDefaultFontStyle, "10px sans-serif");


static nsIMemoryReporter *gCanvasMemoryReporter = nsnull;
static PRInt64 gCanvasMemoryUsed = 0;

static PRInt64 GetCanvasMemoryUsed() {
    return gCanvasMemoryUsed;
}




NS_MEMORY_REPORTER_IMPLEMENT(CanvasMemory,
    "canvas-2d-pixel-bytes",
    KIND_OTHER,
    UNITS_BYTES,
    GetCanvasMemoryUsed,
    "Memory used by 2D canvases. Each canvas requires (width * height * 4) "
    "bytes.")

static void
CopyContext(gfxContext* dest, gfxContext* src)
{
    dest->Multiply(src->CurrentMatrix());

    nsRefPtr<gfxPath> path = src->CopyPath();
    dest->NewPath();
    dest->AppendPath(path);

    nsRefPtr<gfxPattern> pattern = src->GetPattern();
    dest->SetPattern(pattern);

    dest->SetLineWidth(src->CurrentLineWidth());
    dest->SetLineCap(src->CurrentLineCap());
    dest->SetLineJoin(src->CurrentLineJoin());
    dest->SetMiterLimit(src->CurrentMiterLimit());
    dest->SetFillRule(src->CurrentFillRule());

    dest->SetAntialiasMode(src->CurrentAntialiasMode());

    AutoFallibleTArray<gfxFloat, 10> dashes;
    double dashOffset;
    if (src->CurrentDash(dashes, &dashOffset)) {
        dest->SetDash(dashes.Elements(), dashes.Length(), dashOffset);
    }
}




#define NS_CANVASGRADIENT_PRIVATE_IID \
    { 0x491d39d8, 0x4058, 0x42bd, { 0xac, 0x76, 0x70, 0xd5, 0x62, 0x7f, 0x02, 0x10 } }
class nsCanvasGradient : public nsIDOMCanvasGradient
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_CANVASGRADIENT_PRIVATE_IID)

    nsCanvasGradient(gfxPattern* pat)
        : mPattern(pat)
    {
    }

    gfxPattern* GetPattern() {
        return mPattern;
    }

    
    NS_IMETHOD AddColorStop (float offset,
                             const nsAString& colorstr)
    {
        if (!FloatValidate(offset) || offset < 0.0 || offset > 1.0)
            return NS_ERROR_DOM_INDEX_SIZE_ERR;

        nscolor color;
        nsCSSParser parser;
        nsresult rv = parser.ParseColorString(nsString(colorstr),
                                              nsnull, 0, &color);
        if (NS_FAILED(rv))
            return NS_ERROR_DOM_SYNTAX_ERR;

        mPattern->AddColorStop(offset, gfxRGBA(color));

        return NS_OK;
    }

    NS_DECL_ISUPPORTS

protected:
    nsRefPtr<gfxPattern> mPattern;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsCanvasGradient, NS_CANVASGRADIENT_PRIVATE_IID)

NS_IMPL_ADDREF(nsCanvasGradient)
NS_IMPL_RELEASE(nsCanvasGradient)

DOMCI_DATA(CanvasGradient, nsCanvasGradient)

NS_INTERFACE_MAP_BEGIN(nsCanvasGradient)
  NS_INTERFACE_MAP_ENTRY(nsCanvasGradient)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCanvasGradient)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CanvasGradient)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




#define NS_CANVASPATTERN_PRIVATE_IID \
    { 0xb85c6c8a, 0x0624, 0x4530, { 0xb8, 0xee, 0xff, 0xdf, 0x42, 0xe8, 0x21, 0x6d } }
class nsCanvasPattern : public nsIDOMCanvasPattern
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_CANVASPATTERN_PRIVATE_IID)

    nsCanvasPattern(gfxPattern* pat,
                    nsIPrincipal* principalForSecurityCheck,
                    bool forceWriteOnly,
                    bool CORSUsed)
        : mPattern(pat),
          mPrincipal(principalForSecurityCheck),
          mForceWriteOnly(forceWriteOnly),
          mCORSUsed(CORSUsed)
    {
    }

    gfxPattern* GetPattern() const {
        return mPattern;
    }

    nsIPrincipal* Principal() const { return mPrincipal; }
    bool GetForceWriteOnly() const { return mForceWriteOnly; }
    bool GetCORSUsed() const { return mCORSUsed; }

    NS_DECL_ISUPPORTS

protected:
    nsRefPtr<gfxPattern> mPattern;
    nsCOMPtr<nsIPrincipal> mPrincipal;
    const bool mForceWriteOnly;
    const bool mCORSUsed;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsCanvasPattern, NS_CANVASPATTERN_PRIVATE_IID)

NS_IMPL_ADDREF(nsCanvasPattern)
NS_IMPL_RELEASE(nsCanvasPattern)

DOMCI_DATA(CanvasPattern, nsCanvasPattern)

NS_INTERFACE_MAP_BEGIN(nsCanvasPattern)
  NS_INTERFACE_MAP_ENTRY(nsCanvasPattern)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCanvasPattern)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CanvasPattern)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END




#define NS_TEXTMETRICS_PRIVATE_IID \
    { 0xc5b1c2f9, 0xcb4f, 0x4394, { 0xaf, 0xe0, 0xc6, 0x59, 0x33, 0x80, 0x8b, 0xf3 } }
class nsTextMetrics : public nsIDOMTextMetrics
{
public:
    nsTextMetrics(float w) : width(w) { }

    virtual ~nsTextMetrics() { }

    NS_DECLARE_STATIC_IID_ACCESSOR(NS_TEXTMETRICS_PRIVATE_IID)

    NS_IMETHOD GetWidth(float* w) {
        *w = width;
        return NS_OK;
    }

    NS_DECL_ISUPPORTS

private:
    float width;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsTextMetrics, NS_TEXTMETRICS_PRIVATE_IID)

NS_IMPL_ADDREF(nsTextMetrics)
NS_IMPL_RELEASE(nsTextMetrics)

DOMCI_DATA(TextMetrics, nsTextMetrics)

NS_INTERFACE_MAP_BEGIN(nsTextMetrics)
  NS_INTERFACE_MAP_ENTRY(nsTextMetrics)
  NS_INTERFACE_MAP_ENTRY(nsIDOMTextMetrics)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(TextMetrics)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

struct nsCanvasBidiProcessor;




class nsCanvasRenderingContext2D :
    public nsIDOMCanvasRenderingContext2D,
    public nsICanvasRenderingContextInternal
{
public:
    nsCanvasRenderingContext2D();
    virtual ~nsCanvasRenderingContext2D();

    nsresult Redraw();

    
    NS_IMETHOD SetCanvasElement(nsHTMLCanvasElement* aParentCanvas);
    NS_IMETHOD SetDimensions(PRInt32 width, PRInt32 height);
    void Initialize(nsIDocShell *shell, PRInt32 width, PRInt32 height);
    NS_IMETHOD InitializeWithSurface(nsIDocShell *shell, gfxASurface *surface, PRInt32 width, PRInt32 height);
    bool EnsureSurface();
    NS_IMETHOD Render(gfxContext *ctx, gfxPattern::GraphicsFilter aFilter);
    NS_IMETHOD GetInputStream(const char* aMimeType,
                              const PRUnichar* aEncoderOptions,
                              nsIInputStream **aStream);
    NS_IMETHOD GetThebesSurface(gfxASurface **surface);
    mozilla::TemporaryRef<mozilla::gfx::SourceSurface> GetSurfaceSnapshot()
        { return nsnull; }

    NS_IMETHOD SetIsOpaque(bool isOpaque);
    NS_IMETHOD Reset();
    virtual already_AddRefed<CanvasLayer> GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                                         CanvasLayer *aOldLayer,
                                                         LayerManager *aManager);
    virtual bool ShouldForceInactiveLayer(LayerManager *aManager);
    virtual void MarkContextClean();
    NS_IMETHOD SetIsIPC(bool isIPC);
    
    NS_IMETHOD Redraw(const gfxRect &r);
    
    NS_IMETHOD RedrawUser(const gfxRect &r);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS

    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsCanvasRenderingContext2D, nsIDOMCanvasRenderingContext2D)

    
    NS_DECL_NSIDOMCANVASRENDERINGCONTEXT2D

    enum Style {
        STYLE_STROKE = 0,
        STYLE_FILL,
        STYLE_SHADOW,
        STYLE_MAX
    };

    class PathAutoSaveRestore
    {
    public:
        PathAutoSaveRestore(nsCanvasRenderingContext2D* aCtx) :
          mContext(aCtx->mThebes)
        {
            if (aCtx->mHasPath) {
                mPath = mContext->CopyPath();
            }
        }
        ~PathAutoSaveRestore()
        {
            mContext->NewPath();
            if (mPath) {
                mContext->AppendPath(mPath);
            }
        }
    private:
        gfxContext *mContext;
        nsRefPtr<gfxPath> mPath;
    };
    friend class PathAutoSaveRestore;

protected:
    



    static PRUint32 sNumLivingContexts;

    


    static PRUint8 (*sUnpremultiplyTable)[256];

    


    static PRUint8 (*sPremultiplyTable)[256];

    
    nsresult SetStyleFromStringOrInterface(const nsAString& aStr, nsISupports *aInterface, Style aWhichStyle);
    nsresult GetStyleAsStringOrInterface(nsAString& aStr, nsISupports **aInterface, PRInt32 *aType, Style aWhichStyle);

    void StyleColorToString(const nscolor& aColor, nsAString& aStr);

    void DirtyAllStyles();
    




    void ApplyStyle(Style aWhichStyle, bool aUseGlobalAlpha = true);

    


    void EnsureUnpremultiplyTable();

    


    void EnsurePremultiplyTable();

    



    gfxASurface::gfxImageFormat GetImageFormat() const;

    
    PRInt32 mWidth, mHeight;
    bool mValid;
    bool mZero;
    bool mOpaque;
    bool mResetLayer;
    bool mIPC;

    
    nsCOMPtr<nsIDOMHTMLCanvasElement> mCanvasElement;
    nsHTMLCanvasElement *HTMLCanvasElement() {
        return static_cast<nsHTMLCanvasElement*>(mCanvasElement.get());
    }

    
    void CreateThebes();

    
    nsCOMPtr<nsIDocShell> mDocShell;

    
    nsRefPtr<gfxContext> mThebes;
    nsRefPtr<gfxASurface> mSurface;
    bool mSurfaceCreated;

    PRUint32 mSaveCount;

    



    bool mIsEntireFrameInvalid;
    




    bool mPredictManyRedrawCalls;
    


    bool mHasPath;

    


    PRUint32 mInvalidateCount;
    static const PRUint32 kCanvasMaxInvalidateCount = 100;

    




    bool OperatorAffectsUncoveredAreas(gfxContext::GraphicsOperator op) const
    {
        return op == gfxContext::OPERATOR_IN ||
               op == gfxContext::OPERATOR_OUT ||
               op == gfxContext::OPERATOR_DEST_IN ||
               op == gfxContext::OPERATOR_DEST_ATOP;
    }

    



    bool NeedToDrawShadow()
    {
        ContextState& state = CurrentState();

        
        
        return state.StyleIsColor(STYLE_SHADOW) &&
               NS_GET_A(state.colorStyles[STYLE_SHADOW]) > 0 &&
               (state.shadowOffset != gfxPoint(0, 0) || state.shadowBlur != 0);
    }

    





    bool NeedToUseIntermediateSurface()
    {
        if (!mThebes) {
            
            return OperatorAffectsUncoveredAreas(gfxContext::OPERATOR_OVER);
        }
     
        
        
        return OperatorAffectsUncoveredAreas(mThebes->CurrentOperator());

        
        
    }

    



    void ClearSurfaceForUnboundedSource()
    {
        if (!mThebes) {
            
            return;
        }

        gfxContext::GraphicsOperator current = mThebes->CurrentOperator();
        if (current != gfxContext::OPERATOR_SOURCE)
            return;
        mThebes->SetOperator(gfxContext::OPERATOR_CLEAR);
        
        
        
        mThebes->Paint();
        mThebes->SetOperator(current);
    }

    



    bool NeedIntermediateSurfaceToHandleGlobalAlpha(Style aWhichStyle)
    {
        return CurrentState().globalAlpha != 1.0 && !CurrentState().StyleIsColor(aWhichStyle);
    }

    









    gfxContext* ShadowInitialize(const gfxRect& extents, gfxAlphaBoxBlur& blur);

    



    void ShadowFinalize(gfxAlphaBoxBlur& blur);

    






    nsresult DrawPath(Style style, gfxRect *dirtyRect = nsnull);

    


    nsresult DrawRect(const gfxRect& rect, Style style);

    


    nsIPresShell *GetPresShell() {
      nsCOMPtr<nsIContent> content = do_QueryObject(mCanvasElement);
      if (content) {
        return content->OwnerDoc()->GetShell();
      }
      if (mDocShell) {
        nsCOMPtr<nsIPresShell> shell;
        mDocShell->GetPresShell(getter_AddRefs(shell));
        return shell.get();
      }
      return nsnull;
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

    gfxFontGroup* GetCurrentFontStyle();
    gfxTextRun* MakeTextRun(const PRUnichar* aText,
                            PRUint32         aLength,
                            PRUint32         aAppUnitsPerDevUnit,
                            PRUint32         aFlags);

    enum TextDrawOperation {
        TEXT_DRAW_OPERATION_FILL,
        TEXT_DRAW_OPERATION_STROKE,
        TEXT_DRAW_OPERATION_MEASURE
    };

    



    nsresult DrawOrMeasureText(const nsAString& text,
                               float x,
                               float y,
                               float maxWidth,
                               TextDrawOperation op,
                               float* aWidth);

    
    



    Style mLastStyle;
    bool mDirtyStyle[STYLE_MAX];

    
    class ContextState {
    public:
        ContextState() : shadowOffset(0.0, 0.0),
                         globalAlpha(1.0),
                         shadowBlur(0.0),
                         textAlign(TEXT_ALIGN_START),
                         textBaseline(TEXT_BASELINE_ALPHABETIC),
                         imageSmoothingEnabled(true)
        { }

        ContextState(const ContextState& other)
            : shadowOffset(other.shadowOffset),
              globalAlpha(other.globalAlpha),
              shadowBlur(other.shadowBlur),
              font(other.font),
              fontGroup(other.fontGroup),
              textAlign(other.textAlign),
              textBaseline(other.textBaseline),
              imageSmoothingEnabled(other.imageSmoothingEnabled)
        {
            for (int i = 0; i < STYLE_MAX; i++) {
                colorStyles[i] = other.colorStyles[i];
                gradientStyles[i] = other.gradientStyles[i];
                patternStyles[i] = other.patternStyles[i];
            }
        }

        inline void SetColorStyle(Style whichStyle, nscolor color) {
            colorStyles[whichStyle] = color;
            gradientStyles[whichStyle] = nsnull;
            patternStyles[whichStyle] = nsnull;
        }

        inline void SetPatternStyle(Style whichStyle, nsCanvasPattern* pat) {
            gradientStyles[whichStyle] = nsnull;
            patternStyles[whichStyle] = pat;
        }

        inline void SetGradientStyle(Style whichStyle, nsCanvasGradient* grad) {
            gradientStyles[whichStyle] = grad;
            patternStyles[whichStyle] = nsnull;
        }

        


        inline bool StyleIsColor(Style whichStyle) const
        {
            return !(patternStyles[whichStyle] ||
                     gradientStyles[whichStyle]);
        }

        gfxPoint shadowOffset;
        float globalAlpha;
        float shadowBlur;

        nsString font;
        nsRefPtr<gfxFontGroup> fontGroup;
        TextAlign textAlign;
        TextBaseline textBaseline;

        nscolor colorStyles[STYLE_MAX];
        nsCOMPtr<nsCanvasGradient> gradientStyles[STYLE_MAX];
        nsCOMPtr<nsCanvasPattern> patternStyles[STYLE_MAX];

        bool imageSmoothingEnabled;
    };

    nsTArray<ContextState> mStyleStack;

    inline ContextState& CurrentState() {
        return mStyleStack[mSaveCount];
    }

    
    void GetAppUnitsValues(PRUint32 *perDevPixel, PRUint32 *perCSSPixel) {
        
        PRUint32 devPixel = 60;
        PRUint32 cssPixel = 60;

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

    friend struct nsCanvasBidiProcessor;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsCanvasRenderingContext2D)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsCanvasRenderingContext2D)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsCanvasRenderingContext2D)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsCanvasRenderingContext2D)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCanvasElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsCanvasRenderingContext2D)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCanvasElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

DOMCI_DATA(CanvasRenderingContext2D, nsCanvasRenderingContext2D)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsCanvasRenderingContext2D)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCanvasRenderingContext2D)
  NS_INTERFACE_MAP_ENTRY(nsICanvasRenderingContextInternal)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMCanvasRenderingContext2D)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(CanvasRenderingContext2D)
NS_INTERFACE_MAP_END







PRUint32 nsCanvasRenderingContext2D::sNumLivingContexts = 0;
PRUint8 (*nsCanvasRenderingContext2D::sUnpremultiplyTable)[256] = nsnull;
PRUint8 (*nsCanvasRenderingContext2D::sPremultiplyTable)[256] = nsnull;

nsresult
NS_NewCanvasRenderingContext2DThebes(nsIDOMCanvasRenderingContext2D** aResult)
{
    nsRefPtr<nsIDOMCanvasRenderingContext2D> ctx = new nsCanvasRenderingContext2D();
    if (!ctx)
        return NS_ERROR_OUT_OF_MEMORY;

    *aResult = ctx.forget().get();
    return NS_OK;
}

nsCanvasRenderingContext2D::nsCanvasRenderingContext2D()
    : mValid(false), mZero(false), mOpaque(false), mResetLayer(true)
    , mIPC(false)
    , mCanvasElement(nsnull)
    , mSaveCount(0), mIsEntireFrameInvalid(false)
    , mPredictManyRedrawCalls(false), mHasPath(false), mInvalidateCount(0)
    , mLastStyle(STYLE_MAX), mStyleStack(20)
{
    sNumLivingContexts++;
}

nsCanvasRenderingContext2D::~nsCanvasRenderingContext2D()
{
    Reset();
    sNumLivingContexts--;
    if (!sNumLivingContexts) {
        delete[] sUnpremultiplyTable;
        delete[] sPremultiplyTable;
        sUnpremultiplyTable = nsnull;
        sPremultiplyTable = nsnull;
    }
}

nsresult
nsCanvasRenderingContext2D::Reset()
{
    if (mCanvasElement) {
        HTMLCanvasElement()->InvalidateCanvas();
    }

    
    
    if (mValid && !mDocShell && mSurface)
        gCanvasMemoryUsed -= mWidth * mHeight * 4;

    mSurface = nsnull;
    mThebes = nsnull;
    mValid = false;
    mIsEntireFrameInvalid = false;
    mPredictManyRedrawCalls = false;
    return NS_OK;
}

nsresult
nsCanvasRenderingContext2D::SetStyleFromStringOrInterface(const nsAString& aStr,
                                                          nsISupports *aInterface,
                                                          Style aWhichStyle)
{
    nsresult rv;
    nscolor color;

    if (!aStr.IsVoid()) {
        nsIDocument* document = mCanvasElement ?
                                HTMLCanvasElement()->OwnerDoc() : nsnull;

        
        
        nsCSSParser parser(document ? document->CSSLoader() : nsnull);
        rv = parser.ParseColorString(aStr, nsnull, 0, &color);
        if (NS_FAILED(rv)) {
            
            return NS_OK;
        }

        CurrentState().SetColorStyle(aWhichStyle, color);

        mDirtyStyle[aWhichStyle] = true;
        return NS_OK;
    }

    if (aInterface) {
        nsCOMPtr<nsCanvasGradient> grad(do_QueryInterface(aInterface));
        if (grad) {
            CurrentState().SetGradientStyle(aWhichStyle, grad);
            mDirtyStyle[aWhichStyle] = true;
            return NS_OK;
        }

        nsCOMPtr<nsCanvasPattern> pattern(do_QueryInterface(aInterface));
        if (pattern) {
            CurrentState().SetPatternStyle(aWhichStyle, pattern);
            mDirtyStyle[aWhichStyle] = true;
            return NS_OK;
        }
    }

    nsContentUtils::ReportToConsole(
        nsContentUtils::eDOM_PROPERTIES,
        "UnexpectedCanvasVariantStyle",
        nsnull, 0,
        nsnull,
        EmptyString(), 0, 0,
        nsIScriptError::warningFlag,
        "Canvas",
        mCanvasElement ? HTMLCanvasElement()->OwnerDoc() : nsnull);

    return NS_OK;
}

nsresult
nsCanvasRenderingContext2D::GetStyleAsStringOrInterface(nsAString& aStr,
                                                        nsISupports **aInterface,
                                                        PRInt32 *aType,
                                                        Style aWhichStyle)
{
    if (CurrentState().patternStyles[aWhichStyle]) {
        aStr.SetIsVoid(true);
        NS_ADDREF(*aInterface = CurrentState().patternStyles[aWhichStyle]);
        *aType = CMG_STYLE_PATTERN;
    } else if (CurrentState().gradientStyles[aWhichStyle]) {
        aStr.SetIsVoid(true);
        NS_ADDREF(*aInterface = CurrentState().gradientStyles[aWhichStyle]);
        *aType = CMG_STYLE_GRADIENT;
    } else {
        StyleColorToString(CurrentState().colorStyles[aWhichStyle], aStr);
        *aInterface = nsnull;
        *aType = CMG_STYLE_STRING;
    }

    return NS_OK;
}

void
nsCanvasRenderingContext2D::StyleColorToString(const nscolor& aColor, nsAString& aStr)
{
    
    
    if (NS_GET_A(aColor) == 255) {
        CopyUTF8toUTF16(nsPrintfCString(100, "#%02x%02x%02x",
                                        NS_GET_R(aColor),
                                        NS_GET_G(aColor),
                                        NS_GET_B(aColor)),
                        aStr);
    } else {
        CopyUTF8toUTF16(nsPrintfCString(100, "rgba(%d, %d, %d, ",
                                        NS_GET_R(aColor),
                                        NS_GET_G(aColor),
                                        NS_GET_B(aColor)),
                        aStr);
        aStr.AppendFloat(nsStyleUtil::ColorComponentToFloat(NS_GET_A(aColor)));
        aStr.Append(')');
    }
}

void
nsCanvasRenderingContext2D::DirtyAllStyles()
{
    for (int i = 0; i < STYLE_MAX; i++) {
        mDirtyStyle[i] = true;
    }
}

void
nsCanvasRenderingContext2D::ApplyStyle(Style aWhichStyle,
                                       bool aUseGlobalAlpha)
{
    if (mLastStyle == aWhichStyle &&
        !mDirtyStyle[aWhichStyle] &&
        aUseGlobalAlpha)
    {
        
        return;
    }

    if (!EnsureSurface()) {
      return;
    }

    
    if (aUseGlobalAlpha)
        mDirtyStyle[aWhichStyle] = false;
    mLastStyle = aWhichStyle;

    nsCanvasPattern* pattern = CurrentState().patternStyles[aWhichStyle];
    if (pattern) {
        if (mCanvasElement)
            CanvasUtils::DoDrawImageSecurityCheck(HTMLCanvasElement(),
                                                  pattern->Principal(),
                                                  pattern->GetForceWriteOnly(),
                                                  pattern->GetCORSUsed());

        gfxPattern* gpat = pattern->GetPattern();

        if (CurrentState().imageSmoothingEnabled)
            gpat->SetFilter(gfxPattern::FILTER_GOOD);
        else
            gpat->SetFilter(gfxPattern::FILTER_NEAREST);

        mThebes->SetPattern(gpat);
        return;
    }

    if (CurrentState().gradientStyles[aWhichStyle]) {
        gfxPattern* gpat = CurrentState().gradientStyles[aWhichStyle]->GetPattern();
        mThebes->SetPattern(gpat);
        return;
    }

    gfxRGBA color(CurrentState().colorStyles[aWhichStyle]);
    if (aUseGlobalAlpha)
        color.a *= CurrentState().globalAlpha;

    mThebes->SetColor(color);
}

nsresult
nsCanvasRenderingContext2D::Redraw()
{
    if (mIsEntireFrameInvalid)
        return NS_OK;
    mIsEntireFrameInvalid = true;

    if (!mCanvasElement) {
        NS_ASSERTION(mDocShell, "Redraw with no canvas element or docshell!");
        return NS_OK;
    }

    nsSVGEffects::InvalidateDirectRenderingObservers(HTMLCanvasElement());

    HTMLCanvasElement()->InvalidateCanvasContent(nsnull);

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Redraw(const gfxRect& r)
{
    ++mInvalidateCount;

    if (mIsEntireFrameInvalid)
        return NS_OK;

    if (mPredictManyRedrawCalls ||
        mInvalidateCount > kCanvasMaxInvalidateCount) {
        return Redraw();
    }

    if (!mCanvasElement) {
        NS_ASSERTION(mDocShell, "Redraw with no canvas element or docshell!");
        return NS_OK;
    }

    nsSVGEffects::InvalidateDirectRenderingObservers(HTMLCanvasElement());

    HTMLCanvasElement()->InvalidateCanvasContent(&r);

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::RedrawUser(const gfxRect& r)
{
    if (mIsEntireFrameInvalid) {
        ++mInvalidateCount;
        return NS_OK;
    }

    return Redraw(mThebes->UserToDevice(r));
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetDimensions(PRInt32 width, PRInt32 height)
{
    Initialize(NULL, width, height);
    return NS_OK;
}

void
nsCanvasRenderingContext2D::Initialize(nsIDocShell *docShell, PRInt32 width, PRInt32 height) 
{
    Reset();

    NS_ASSERTION(!docShell ^ !mCanvasElement, "Cannot set both docshell and canvas element");
    mDocShell = docShell;

    mWidth = width;
    mHeight = height;

    mResetLayer = true;
    mValid = true;
    mSurfaceCreated = false;

    
    mStyleStack.Clear();
    mSaveCount = 0;

    ContextState *state = mStyleStack.AppendElement();
    state->globalAlpha = 1.0;

    state->colorStyles[STYLE_FILL] = NS_RGB(0,0,0);
    state->colorStyles[STYLE_STROKE] = NS_RGB(0,0,0);
    state->colorStyles[STYLE_SHADOW] = NS_RGBA(0,0,0,0);
    DirtyAllStyles();

    
    
    Redraw();

    return;
}

void
nsCanvasRenderingContext2D::CreateThebes()
{
    mThebes = new gfxContext(mSurface);
    mSurfaceCreated = true;
    
    mThebes->SetOperator(gfxContext::OPERATOR_CLEAR);
    mThebes->NewPath();
    mThebes->Rectangle(gfxRect(0, 0, mWidth, mHeight));
    mThebes->Fill();

    mThebes->SetLineWidth(1.0);
    mThebes->SetOperator(gfxContext::OPERATOR_OVER);
    mThebes->SetMiterLimit(10.0);
    mThebes->SetLineCap(gfxContext::LINE_CAP_BUTT);
    mThebes->SetLineJoin(gfxContext::LINE_JOIN_MITER);

    mThebes->NewPath();
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::InitializeWithSurface(nsIDocShell *docShell, 
                                                  gfxASurface *surface, 
                                                  PRInt32 width, 
                                                  PRInt32 height)
{
    Initialize(docShell, width, height);
    
    mSurface = surface;
    CreateThebes();
    return mValid ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

bool
nsCanvasRenderingContext2D::EnsureSurface()
{
    if (!mValid) {
        return false;
    }

    if (mSurface && mThebes && mSurfaceCreated) {
        if (mSurface->CairoStatus()) {
            return false;
        }
        return true;
    }
    
    nsRefPtr<gfxASurface> surface;

    
    if (gfxASurface::CheckSurfaceSize(gfxIntSize(mWidth, mHeight), 0xffff)) {
        
        if (mHeight == 0 || mWidth == 0) {
            mZero = true;
            mHeight = 1;
            mWidth = 1;
        } else {
            mZero = false;
        }

        gfxASurface::gfxImageFormat format = GetImageFormat();

        if (!PR_GetEnv("MOZ_CANVAS_IMAGE_SURFACE")) {
            nsCOMPtr<nsIContent> content = do_QueryObject(mCanvasElement);
            nsIDocument* ownerDoc = nsnull;
            if (content)
                ownerDoc = content->OwnerDoc();
            nsRefPtr<LayerManager> layerManager = nsnull;

            if (ownerDoc)
              layerManager =
                nsContentUtils::PersistentLayerManagerForDocument(ownerDoc);

            if (layerManager) {
              surface = layerManager->CreateOptimalSurface(gfxIntSize(mWidth, mHeight), format);
            } else {
              surface = gfxPlatform::GetPlatform()->
                CreateOffscreenSurface(gfxIntSize(mWidth, mHeight), gfxASurface::ContentFromFormat(format));
            }
        }

        if (!surface || surface->CairoStatus()) {
            
            
            
            surface = new gfxImageSurface(gfxIntSize(mWidth, mHeight), format);
            if (!surface || surface->CairoStatus()) {
                surface = nsnull;
            }
        }
    }
    if (surface) {
        if (gCanvasMemoryReporter == nsnull) {
            gCanvasMemoryReporter = new NS_MEMORY_REPORTER_NAME(CanvasMemory);
            NS_RegisterMemoryReporter(gCanvasMemoryReporter);
        }

        gCanvasMemoryUsed += mWidth * mHeight * 4;
        JSContext* context = nsContentUtils::GetCurrentJSContext();
        if (context) {
            JS_updateMallocCounter(context, mWidth * mHeight * 4);
        }
    } else {
        return false;
    }

    mSurface = surface;
    CreateThebes();

    if (mSurface->CairoStatus()) {
        return false;
    }
    return true;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetIsOpaque(bool isOpaque)
{
    if (isOpaque == mOpaque)
        return NS_OK;

    mOpaque = isOpaque;

    if (mValid) {
        


        return SetDimensions(mWidth, mHeight);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetIsIPC(bool isIPC)
{
    if (isIPC == mIPC)
        return NS_OK;

    mIPC = isIPC;

    if (mValid) {
        


        return SetDimensions(mWidth, mHeight);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Render(gfxContext *ctx, gfxPattern::GraphicsFilter aFilter)
{
    nsresult rv = NS_OK;

    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxPattern> pat = new gfxPattern(mSurface);

    pat->SetFilter(aFilter);
    pat->SetExtend(gfxPattern::EXTEND_PAD);

    gfxContext::GraphicsOperator op = ctx->CurrentOperator();
    if (mOpaque)
        ctx->SetOperator(gfxContext::OPERATOR_SOURCE);

    
    
    ctx->NewPath();
    ctx->PixelSnappedRectangleAndSetPattern(gfxRect(0, 0, mWidth, mHeight), pat);
    ctx->Fill();

    if (mOpaque)
        ctx->SetOperator(op);

    return rv;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetInputStream(const char *aMimeType,
                                           const PRUnichar *aEncoderOptions,
                                           nsIInputStream **aStream)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    nsresult rv;
    const char encoderPrefix[] = "@mozilla.org/image/encoder;2?type=";
    nsAutoArrayPtr<char> conid(new (std::nothrow) char[strlen(encoderPrefix) + strlen(aMimeType) + 1]);

    if (!conid)
        return NS_ERROR_OUT_OF_MEMORY;

    strcpy(conid, encoderPrefix);
    strcat(conid, aMimeType);

    nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(conid);
    if (!encoder)
        return NS_ERROR_FAILURE;

    nsAutoArrayPtr<PRUint8> imageBuffer(new (std::nothrow) PRUint8[mWidth * mHeight * 4]);
    if (!imageBuffer)
        return NS_ERROR_OUT_OF_MEMORY;

    nsRefPtr<gfxImageSurface> imgsurf = new gfxImageSurface(imageBuffer.get(),
                                                            gfxIntSize(mWidth, mHeight),
                                                            mWidth * 4,
                                                            gfxASurface::ImageFormatARGB32);

    if (!imgsurf || imgsurf->CairoStatus())
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxContext> ctx = new gfxContext(imgsurf);

    if (!ctx || ctx->HasError())
        return NS_ERROR_FAILURE;

    ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
    ctx->SetSource(mSurface, gfxPoint(0, 0));
    ctx->Paint();

    rv = encoder->InitFromData(imageBuffer.get(),
                               mWidth * mHeight * 4, mWidth, mHeight, mWidth * 4,
                               imgIEncoder::INPUT_FORMAT_HOSTARGB,
                               nsDependentString(aEncoderOptions));
    NS_ENSURE_SUCCESS(rv, rv);

    return CallQueryInterface(encoder, aStream);
}

gfxASurface::gfxImageFormat
nsCanvasRenderingContext2D::GetImageFormat() const
{
    gfxASurface::gfxImageFormat format = gfxASurface::ImageFormatARGB32;

    if (mOpaque)
        format = gfxASurface::ImageFormatRGB24;

    return format;
}





NS_IMETHODIMP
nsCanvasRenderingContext2D::SetCanvasElement(nsHTMLCanvasElement* aCanvasElement)
{
    mCanvasElement = aCanvasElement;

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetCanvas(nsIDOMHTMLCanvasElement **canvas)
{
    NS_IF_ADDREF(*canvas = mCanvasElement);

    return NS_OK;
}





NS_IMETHODIMP
nsCanvasRenderingContext2D::Save()
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    ContextState state = CurrentState();
    mStyleStack.AppendElement(state);
    mThebes->Save();
    mSaveCount++;
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Restore()
{
    if (!EnsureSurface()) 
        return NS_ERROR_FAILURE;

    if (mSaveCount == 0)
        return NS_OK;

    mStyleStack.RemoveElementAt(mSaveCount);
    mThebes->Restore();

    mLastStyle = STYLE_MAX;
    DirtyAllStyles();

    mSaveCount--;
    return NS_OK;
}





NS_IMETHODIMP
nsCanvasRenderingContext2D::Scale(float x, float y)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(x,y))
        return NS_OK;

    mThebes->Scale(x, y);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Rotate(float angle)
{
    if (!EnsureSurface()) 
        return NS_ERROR_FAILURE;

    if (!FloatValidate(angle))
        return NS_OK;

    mThebes->Rotate(angle);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Translate(float x, float y)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(x,y))
        return NS_OK;

    mThebes->Translate(gfxPoint(x, y));
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Transform(float m11, float m12, float m21, float m22, float dx, float dy)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(m11,m12,m21,m22,dx,dy))
        return NS_OK;

    gfxMatrix matrix(m11, m12, m21, m22, dx, dy);
    mThebes->Multiply(matrix);

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetTransform(float m11, float m12, float m21, float m22, float dx, float dy)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(m11,m12,m21,m22,dx,dy))
        return NS_OK;

    gfxMatrix matrix(m11, m12, m21, m22, dx, dy);
    mThebes->SetMatrix(matrix);

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetMozCurrentTransform(JSContext* cx,
                                                   const jsval& matrix)
{
    nsresult rv;
    gfxMatrix newCTM;
    
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!JSValToMatrix(cx, matrix, &newCTM, &rv)) {
        return rv;
    }

    mThebes->SetMatrix(newCTM);

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetMozCurrentTransform(JSContext* cx,
                                                   jsval* matrix)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    return MatrixToJSVal(mThebes->CurrentMatrix(), cx, matrix);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetMozCurrentTransformInverse(JSContext* cx,
                                                          const jsval& matrix)
{
    nsresult rv;
    gfxMatrix newCTMInverse;
    
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!JSValToMatrix(cx, matrix, &newCTMInverse, &rv)) {
        return rv;
    }

    
    if (!newCTMInverse.IsSingular()) {
        mThebes->SetMatrix(newCTMInverse.Invert());
    }

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetMozCurrentTransformInverse(JSContext* cx,
                                                          jsval* matrix)
{
    gfxMatrix ctm = mThebes->CurrentMatrix();

    if (!mThebes->CurrentMatrix().IsSingular()) {
        ctm.Invert();
    } else {
        double NaN = JSVAL_TO_DOUBLE(JS_GetNaNValue(cx));
        ctm = gfxMatrix(NaN, NaN, NaN, NaN, NaN, NaN);
    }

    return MatrixToJSVal(ctm, cx, matrix);
}





NS_IMETHODIMP
nsCanvasRenderingContext2D::SetGlobalAlpha(float aGlobalAlpha)
{
    if (!FloatValidate(aGlobalAlpha) || aGlobalAlpha < 0.0 || aGlobalAlpha > 1.0)
        return NS_OK;

    CurrentState().globalAlpha = aGlobalAlpha;
    DirtyAllStyles();

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetGlobalAlpha(float *aGlobalAlpha)
{
    *aGlobalAlpha = CurrentState().globalAlpha;
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetStrokeStyle(nsIVariant *aValue)
{
    if (!aValue)
        return NS_ERROR_FAILURE;

    nsString str;

    nsresult rv;
    PRUint16 vtype;
    rv = aValue->GetDataType(&vtype);
    NS_ENSURE_SUCCESS(rv, rv);

    if (vtype == nsIDataType::VTYPE_INTERFACE ||
        vtype == nsIDataType::VTYPE_INTERFACE_IS)
    {
        nsIID *iid;
        nsCOMPtr<nsISupports> sup;
        rv = aValue->GetAsInterface(&iid, getter_AddRefs(sup));
        NS_ENSURE_SUCCESS(rv, rv);
        if (iid)
            NS_Free(iid);

        str.SetIsVoid(true);
        return SetStrokeStyle_multi(str, sup);
    }

    rv = aValue->GetAsAString(str);
    NS_ENSURE_SUCCESS(rv, rv);

    return SetStrokeStyle_multi(str, nsnull);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetStrokeStyle(nsIVariant **aResult)
{
    nsCOMPtr<nsIWritableVariant> wv = do_CreateInstance(NS_VARIANT_CONTRACTID);

    nsCOMPtr<nsISupports> sup;
    nsString str;
    PRInt32 t;
    nsresult rv = GetStrokeStyle_multi(str, getter_AddRefs(sup), &t);
    NS_ENSURE_SUCCESS(rv, rv);

    if (t == CMG_STYLE_STRING) {
        rv = wv->SetAsAString(str);
    } else if (t == CMG_STYLE_PATTERN) {
        rv = wv->SetAsInterface(NS_GET_IID(nsIDOMCanvasPattern),
                                sup);
    } else if (t == CMG_STYLE_GRADIENT) {
        rv = wv->SetAsInterface(NS_GET_IID(nsIDOMCanvasGradient),
                                sup);
    } else {
        NS_ERROR("Unknown type from GetStroke/FillStyle_multi!");
        return NS_ERROR_FAILURE;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    NS_IF_ADDREF(*aResult = wv.get());
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetFillStyle(nsIVariant *aValue)
{
    if (!aValue)
        return NS_ERROR_FAILURE;

    nsString str;
    nsresult rv;
    PRUint16 vtype;
    rv = aValue->GetDataType(&vtype);
    NS_ENSURE_SUCCESS(rv, rv);

    if (vtype == nsIDataType::VTYPE_INTERFACE ||
        vtype == nsIDataType::VTYPE_INTERFACE_IS)
    {
        nsIID *iid;
        nsCOMPtr<nsISupports> sup;
        rv = aValue->GetAsInterface(&iid, getter_AddRefs(sup));
        NS_ENSURE_SUCCESS(rv, rv);

        str.SetIsVoid(true);
        return SetFillStyle_multi(str, sup);
    }

    rv = aValue->GetAsAString(str);
    NS_ENSURE_SUCCESS(rv, rv);

    return SetFillStyle_multi(str, nsnull);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetFillStyle(nsIVariant **aResult)
{
    nsCOMPtr<nsIWritableVariant> wv = do_CreateInstance(NS_VARIANT_CONTRACTID);

    nsCOMPtr<nsISupports> sup;
    nsString str;
    PRInt32 t;
    nsresult rv = GetFillStyle_multi(str, getter_AddRefs(sup), &t);
    NS_ENSURE_SUCCESS(rv, rv);

    if (t == CMG_STYLE_STRING) {
        rv = wv->SetAsAString(str);
    } else if (t == CMG_STYLE_PATTERN) {
        rv = wv->SetAsInterface(NS_GET_IID(nsIDOMCanvasPattern),
                                sup);
    } else if (t == CMG_STYLE_GRADIENT) {
        rv = wv->SetAsInterface(NS_GET_IID(nsIDOMCanvasGradient),
                                sup);
    } else {
        NS_ERROR("Unknown type from GetStroke/FillStyle_multi!");
        return NS_ERROR_FAILURE;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    NS_IF_ADDREF(*aResult = wv.get());
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetStrokeStyle_multi(const nsAString& aStr, nsISupports *aInterface)
{
    return SetStyleFromStringOrInterface(aStr, aInterface, STYLE_STROKE);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetStrokeStyle_multi(nsAString& aStr, nsISupports **aInterface, PRInt32 *aType)
{
    return GetStyleAsStringOrInterface(aStr, aInterface, aType, STYLE_STROKE);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetFillStyle_multi(const nsAString& aStr, nsISupports *aInterface)
{
    return SetStyleFromStringOrInterface(aStr, aInterface, STYLE_FILL);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetFillStyle_multi(nsAString& aStr, nsISupports **aInterface, PRInt32 *aType)
{
    return GetStyleAsStringOrInterface(aStr, aInterface, aType, STYLE_FILL);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetMozFillRule(const nsAString& aString)
{
    gfxContext::FillRule rule;
    
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (aString.EqualsLiteral("evenodd"))
        rule = gfxContext::FILL_RULE_EVEN_ODD;
    else if (aString.EqualsLiteral("nonzero"))
        rule = gfxContext::FILL_RULE_WINDING;
    else
        
        return NS_OK;

    mThebes->SetFillRule(rule);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetMozFillRule(nsAString& aString)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    switch (mThebes->CurrentFillRule()) {
    case gfxContext::FILL_RULE_WINDING:
        aString.AssignLiteral("nonzero"); break;
    case gfxContext::FILL_RULE_EVEN_ODD:
        aString.AssignLiteral("evenodd"); break;
    default:
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}




NS_IMETHODIMP
nsCanvasRenderingContext2D::CreateLinearGradient(float x0, float y0, float x1, float y1,
                                                 nsIDOMCanvasGradient **_retval)
{
    if (!FloatValidate(x0,y0,x1,y1))
        return NS_ERROR_DOM_NOT_SUPPORTED_ERR;

    nsRefPtr<gfxPattern> gradpat = new gfxPattern(x0, y0, x1, y1);
    if (!gradpat)
        return NS_ERROR_OUT_OF_MEMORY;

    nsRefPtr<nsIDOMCanvasGradient> grad = new nsCanvasGradient(gradpat);
    if (!grad)
        return NS_ERROR_OUT_OF_MEMORY;

    *_retval = grad.forget().get();
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::CreateRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1,
                                                 nsIDOMCanvasGradient **_retval)
{
    if (!FloatValidate(x0,y0,r0,x1,y1,r1))
        return NS_ERROR_DOM_NOT_SUPPORTED_ERR;

    if (r0 < 0.0 || r1 < 0.0)
        return NS_ERROR_DOM_INDEX_SIZE_ERR;

    nsRefPtr<gfxPattern> gradpat = new gfxPattern(x0, y0, r0, x1, y1, r1);
    if (!gradpat)
        return NS_ERROR_OUT_OF_MEMORY;

    nsRefPtr<nsIDOMCanvasGradient> grad = new nsCanvasGradient(gradpat);
    if (!grad)
        return NS_ERROR_OUT_OF_MEMORY;

    *_retval = grad.forget().get();
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::CreatePattern(nsIDOMHTMLElement *image,
                                          const nsAString& repeat,
                                          nsIDOMCanvasPattern **_retval)
{
    nsCOMPtr<nsIContent> content = do_QueryInterface(image);
    if (!content) {
        return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
    }

    gfxPattern::GraphicsExtend extend;
    if (repeat.IsEmpty() || repeat.EqualsLiteral("repeat")) {
        extend = gfxPattern::EXTEND_REPEAT;
    } else if (repeat.EqualsLiteral("repeat-x")) {
        
        extend = gfxPattern::EXTEND_REPEAT;
    } else if (repeat.EqualsLiteral("repeat-y")) {
        
        extend = gfxPattern::EXTEND_REPEAT;
    } else if (repeat.EqualsLiteral("no-repeat")) {
        extend = gfxPattern::EXTEND_NONE;
    } else {
        
        return NS_ERROR_DOM_SYNTAX_ERR;
    }

    nsHTMLCanvasElement* canvas = nsHTMLCanvasElement::FromContent(content);
    if (canvas) {
        nsIntSize size = canvas->GetSize();
        if (size.width == 0 || size.height == 0) {
            return NS_ERROR_DOM_INVALID_STATE_ERR;
        }
    }

    
    
    nsLayoutUtils::SurfaceFromElementResult res =
        nsLayoutUtils::SurfaceFromElement(content->AsElement(),
            nsLayoutUtils::SFE_WANT_FIRST_FRAME | nsLayoutUtils::SFE_WANT_NEW_SURFACE);
    if (!res.mSurface)
        return NS_ERROR_NOT_AVAILABLE;

    nsRefPtr<gfxPattern> thebespat = new gfxPattern(res.mSurface);

    thebespat->SetExtend(extend);

    nsRefPtr<nsCanvasPattern> pat = new nsCanvasPattern(thebespat, res.mPrincipal,
                                                        res.mIsWriteOnly,
                                                        res.mCORSUsed);
    *_retval = pat.forget().get();
    return NS_OK;
}




NS_IMETHODIMP
nsCanvasRenderingContext2D::SetShadowOffsetX(float x)
{
    if (!FloatValidate(x))
        return NS_OK;

    CurrentState().shadowOffset.x = x;
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetShadowOffsetX(float *x)
{
    *x = static_cast<float>(CurrentState().shadowOffset.x);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetShadowOffsetY(float y)
{
    if (!FloatValidate(y))
        return NS_OK;

    CurrentState().shadowOffset.y = y;
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetShadowOffsetY(float *y)
{
    *y = static_cast<float>(CurrentState().shadowOffset.y);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetShadowBlur(float blur)
{
    if (!FloatValidate(blur) || blur < 0.0)
        return NS_OK;

    CurrentState().shadowBlur = blur;
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetShadowBlur(float *blur)
{
    *blur = CurrentState().shadowBlur;
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetShadowColor(const nsAString& colorstr)
{
    nsIDocument* document = mCanvasElement ?
                            HTMLCanvasElement()->OwnerDoc() : nsnull;

    
    
    nsCSSParser parser(document ? document->CSSLoader() : nsnull);
    nscolor color;
    nsresult rv = parser.ParseColorString(colorstr, nsnull, 0, &color);
    if (NS_FAILED(rv)) {
        
        return NS_OK;
    }

    CurrentState().SetColorStyle(STYLE_SHADOW, color);

    mDirtyStyle[STYLE_SHADOW] = true;

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetShadowColor(nsAString& color)
{
    StyleColorToString(CurrentState().colorStyles[STYLE_SHADOW], color);

    return NS_OK;
}

static const gfxFloat SIGMA_MAX = 100;

gfxContext*
nsCanvasRenderingContext2D::ShadowInitialize(const gfxRect& extents, gfxAlphaBoxBlur& blur)
{
    gfxIntSize blurRadius;

    float shadowBlur = CurrentState().shadowBlur;
    gfxFloat sigma = shadowBlur / 2;
    
    if (sigma > SIGMA_MAX)
        sigma = SIGMA_MAX;
    blurRadius = gfxAlphaBoxBlur::CalculateBlurRadius(gfxPoint(sigma, sigma));

    
    gfxRect drawExtents = extents;

    
    gfxMatrix matrix = mThebes->CurrentMatrix();
    mThebes->IdentityMatrix();
    gfxRect clipExtents = mThebes->GetClipExtents();
    mThebes->SetMatrix(matrix);
    
    
    clipExtents.Inflate(blurRadius.width, blurRadius.height);
    drawExtents = drawExtents.Intersect(clipExtents - CurrentState().shadowOffset);

    gfxContext* ctx = blur.Init(drawExtents, gfxIntSize(0,0), blurRadius, nsnull, nsnull);

    if (!ctx)
        return nsnull;

    return ctx;
}

void
nsCanvasRenderingContext2D::ShadowFinalize(gfxAlphaBoxBlur& blur)
{
    if (!EnsureSurface())
        return;

    ApplyStyle(STYLE_SHADOW);
    
    
    gfxMatrix matrix = mThebes->CurrentMatrix();
    mThebes->IdentityMatrix();
    mThebes->Translate(CurrentState().shadowOffset);

    blur.Paint(mThebes);
    mThebes->SetMatrix(matrix);
}

nsresult
nsCanvasRenderingContext2D::DrawPath(Style style, gfxRect *dirtyRect)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    bool doUseIntermediateSurface = false;
    
    if (mSurface->GetType() == gfxASurface::SurfaceTypeD2D) {
      if (style != STYLE_FILL) {
        
        
        
        
        doUseIntermediateSurface = NeedIntermediateSurfaceToHandleGlobalAlpha(style);
      }
    } else {
      




      doUseIntermediateSurface = NeedToUseIntermediateSurface() ||
                                 NeedIntermediateSurfaceToHandleGlobalAlpha(style);
    }

    bool doDrawShadow = NeedToDrawShadow();

    
    ClearSurfaceForUnboundedSource();

    if (doDrawShadow) {
        gfxMatrix matrix = mThebes->CurrentMatrix();
        mThebes->IdentityMatrix();

        
        gfxRect drawExtents;
        if (style == STYLE_FILL)
            drawExtents = mThebes->GetUserFillExtent();
        else 
            drawExtents = mThebes->GetUserStrokeExtent();

        mThebes->SetMatrix(matrix);

        gfxAlphaBoxBlur blur;

        
        gfxContext* ctx = ShadowInitialize(drawExtents, blur);
        if (ctx) {
            ApplyStyle(style, false);
            CopyContext(ctx, mThebes);
            ctx->SetOperator(gfxContext::OPERATOR_SOURCE);

            if (style == STYLE_FILL)
                ctx->Fill();
            else
                ctx->Stroke();

            ShadowFinalize(blur);
        }
    }

    if (doUseIntermediateSurface) {
        nsRefPtr<gfxPath> path = mThebes->CopyPath();
        
        if (!path)
            return NS_ERROR_FAILURE;

        
        mThebes->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);

        
        
        mThebes->NewPath();
        mThebes->AppendPath(path);

        
        if (mSurface->GetType() != gfxASurface::SurfaceTypeD2D) {
            mThebes->SetOperator(gfxContext::OPERATOR_SOURCE);
        } else {
            
            
            
            
            mThebes->SetOperator(gfxContext::OPERATOR_OVER);
        }
    }

    ApplyStyle(style);

    if (style == STYLE_FILL) {
        if (!doUseIntermediateSurface &&
            CurrentState().globalAlpha != 1.0 &&
            !CurrentState().StyleIsColor(style))
        {
            mThebes->Clip();
            mThebes->Paint(CurrentState().globalAlpha);
        } else {
            mThebes->Fill();
        }
    } else
        mThebes->Stroke();

    
    
    if (dirtyRect && style == STYLE_FILL && !doDrawShadow) {
        *dirtyRect = mThebes->GetUserPathExtent();
    }

    if (doUseIntermediateSurface) {
        mThebes->PopGroupToSource();
        DirtyAllStyles();

        mThebes->Paint(CurrentState().StyleIsColor(style) ? 1.0 : CurrentState().globalAlpha);
    }

    if (dirtyRect) {
        if (style != STYLE_FILL || doDrawShadow) {
            
            *dirtyRect = mThebes->GetClipExtents();
        }
    }

    return NS_OK;
}





NS_IMETHODIMP
nsCanvasRenderingContext2D::ClearRect(float x, float y, float w, float h)
{
    if (!mSurfaceCreated)
        return NS_OK;

    if (!FloatValidate(x,y,w,h))
        return NS_OK;

    PathAutoSaveRestore pathSR(this);
    gfxContextAutoSaveRestore autoSR(mThebes);

    mThebes->SetOperator(gfxContext::OPERATOR_CLEAR);
    mThebes->NewPath();
    mThebes->Rectangle(gfxRect(x, y, w, h));
    mThebes->Fill();

    return RedrawUser(mThebes->GetUserPathExtent());
}

nsresult
nsCanvasRenderingContext2D::DrawRect(const gfxRect& rect, Style style)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(rect.X(), rect.Y(), rect.Width(), rect.Height()))
        return NS_OK;

    PathAutoSaveRestore pathSR(this);

    mThebes->NewPath();
    mThebes->Rectangle(rect);

    gfxRect dirty;
    nsresult rv = DrawPath(style, &dirty);
    if (NS_FAILED(rv))
        return rv;

    return RedrawUser(dirty);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::FillRect(float x, float y, float w, float h)
{
    return DrawRect(gfxRect(x, y, w, h), STYLE_FILL);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::StrokeRect(float x, float y, float w, float h)
{
    if (w == 0.f && h == 0.f) {
        return NS_OK;
    }
    return DrawRect(gfxRect(x, y, w, h), STYLE_STROKE);
}





NS_IMETHODIMP
nsCanvasRenderingContext2D::BeginPath()
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    mHasPath = false;
    mThebes->NewPath();
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::ClosePath()
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    mThebes->ClosePath();
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Fill()
{
    gfxRect dirty;
    nsresult rv = DrawPath(STYLE_FILL, &dirty);
    if (NS_FAILED(rv))
        return rv;
    return RedrawUser(dirty);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Stroke()
{
    gfxRect dirty;
    nsresult rv = DrawPath(STYLE_STROKE, &dirty);
    if (NS_FAILED(rv))
        return rv;
    return RedrawUser(dirty);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Clip()
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    mThebes->Clip();
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::MoveTo(float x, float y)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(x,y))
        return NS_OK;

    mHasPath = true;
    mThebes->MoveTo(gfxPoint(x, y));
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::LineTo(float x, float y)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(x,y))
        return NS_OK;

    mHasPath = true;
    mThebes->LineTo(gfxPoint(x, y));
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::QuadraticCurveTo(float cpx, float cpy, float x, float y)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(cpx,cpy,x,y))
        return NS_OK;

    
    
    gfxPoint c = mThebes->CurrentPoint();
    gfxPoint p(x,y);
    gfxPoint cp(cpx, cpy);

    mHasPath = true;
    mThebes->CurveTo((c+cp*2)/3.0, (p+cp*2)/3.0, p);

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::BezierCurveTo(float cp1x, float cp1y,
                                          float cp2x, float cp2y,
                                          float x, float y)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(cp1x,cp1y,cp2x,cp2y,x,y))
        return NS_OK;

    mHasPath = true;
    mThebes->CurveTo(gfxPoint(cp1x, cp1y),
                     gfxPoint(cp2x, cp2y),
                     gfxPoint(x, y));

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::ArcTo(float x1, float y1, float x2, float y2, float radius)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(x1,y1,x2,y2,radius))
        return NS_OK;

    if (radius < 0)
        return NS_ERROR_DOM_INDEX_SIZE_ERR;

    mHasPath = true;

    gfxPoint p0 = mThebes->CurrentPoint();

    double dir, a2, b2, c2, cosx, sinx, d, anx, any, bnx, bny, x3, y3, x4, y4, cx, cy, angle0, angle1;
    bool anticlockwise;

    if ((x1 == p0.x && y1 == p0.y) || (x1 == x2 && y1 == y2) || radius == 0) {
        mThebes->LineTo(gfxPoint(x1, y1));
        return NS_OK;
    }

    dir = (x2-x1)*(p0.y-y1) + (y2-y1)*(x1-p0.x);
    if (dir == 0) {
        mThebes->LineTo(gfxPoint(x1, y1));
        return NS_OK;
    }

    a2 = (p0.x-x1)*(p0.x-x1) + (p0.y-y1)*(p0.y-y1);
    b2 = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
    c2 = (p0.x-x2)*(p0.x-x2) + (p0.y-y2)*(p0.y-y2);
    cosx = (a2+b2-c2)/(2*sqrt(a2*b2));

    sinx = sqrt(1 - cosx*cosx);
    d = radius / ((1 - cosx) / sinx);

    anx = (x1-p0.x) / sqrt(a2);
    any = (y1-p0.y) / sqrt(a2);
    bnx = (x1-x2) / sqrt(b2);
    bny = (y1-y2) / sqrt(b2);
    x3 = x1 - anx*d;
    y3 = y1 - any*d;
    x4 = x1 - bnx*d;
    y4 = y1 - bny*d;
    anticlockwise = (dir < 0);
    cx = x3 + any*radius*(anticlockwise ? 1 : -1);
    cy = y3 - anx*radius*(anticlockwise ? 1 : -1);
    angle0 = atan2((y3-cy), (x3-cx));
    angle1 = atan2((y4-cy), (x4-cx));

    mThebes->LineTo(gfxPoint(x3, y3));

    if (anticlockwise)
        mThebes->NegativeArc(gfxPoint(cx, cy), radius, angle0, angle1);
    else
        mThebes->Arc(gfxPoint(cx, cy), radius, angle0, angle1);

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Arc(float x, float y, float r, float startAngle, float endAngle, bool ccw)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(x,y,r,startAngle,endAngle))
        return NS_OK;

    if (r < 0.0)
        return NS_ERROR_DOM_INDEX_SIZE_ERR;

    gfxPoint p(x,y);

    mHasPath = true;
    if (ccw)
        mThebes->NegativeArc(p, r, startAngle, endAngle);
    else
        mThebes->Arc(p, r, startAngle, endAngle);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::Rect(float x, float y, float w, float h)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(x,y,w,h))
        return NS_OK;

    mHasPath = true;
    mThebes->Rectangle(gfxRect(x, y, w, h));
    return NS_OK;
}












static nsresult
CreateFontStyleRule(const nsAString& aFont,
                    nsINode* aNode,
                    css::StyleRule** aResult)
{
    nsRefPtr<css::StyleRule> rule;
    bool changed;

    nsIPrincipal* principal = aNode->NodePrincipal();
    nsIDocument* document = aNode->OwnerDoc();

    nsIURI* docURL = document->GetDocumentURI();
    nsIURI* baseURL = document->GetDocBaseURI();

    
    
    nsCSSParser parser(document->CSSLoader());

    nsresult rv = parser.ParseStyleAttribute(EmptyString(), docURL, baseURL,
                                             principal, getter_AddRefs(rule));
    if (NS_FAILED(rv))
        return rv;

    rv = parser.ParseProperty(eCSSProperty_font, aFont, docURL, baseURL,
                              principal, rule->GetDeclaration(), &changed,
                              false);
    if (NS_FAILED(rv))
        return rv;

    rv = parser.ParseProperty(eCSSProperty_line_height,
                              NS_LITERAL_STRING("normal"), docURL, baseURL,
                              principal, rule->GetDeclaration(), &changed,
                              false);
    if (NS_FAILED(rv))
        return rv;

    rule->RuleMatched();

    rule.forget(aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetFont(const nsAString& font)
{
    nsresult rv;

    







    nsCOMPtr<nsIContent> content = do_QueryInterface(mCanvasElement);
    if (!content && !mDocShell) {
        NS_WARNING("Canvas element must be an nsIContent and non-null or a docshell must be provided");
        return NS_ERROR_FAILURE;
    }

    nsIPresShell* presShell = GetPresShell();
    if (!presShell)
      return NS_ERROR_FAILURE;
    nsIDocument* document = presShell->GetDocument();

    nsCOMArray<nsIStyleRule> rules;

    nsRefPtr<css::StyleRule> rule;
    rv = CreateFontStyleRule(font, document, getter_AddRefs(rule));
    if (NS_FAILED(rv))
        return rv;

    css::Declaration *declaration = rule->GetDeclaration();
    
    
    
    
    
    
    const nsCSSValue *fsaVal =
      declaration->GetNormalBlock()->ValueFor(eCSSProperty_font_size_adjust);
    if (!fsaVal || (fsaVal->GetUnit() != eCSSUnit_None &&
                    fsaVal->GetUnit() != eCSSUnit_System_Font)) {
        
        
        return NS_OK;
    }

    rules.AppendObject(rule);

    nsStyleSet* styleSet = presShell->StyleSet();

    
    
    nsRefPtr<nsStyleContext> parentContext;

    if (content && content->IsInDoc()) {
        
        parentContext = nsComputedDOMStyle::GetStyleContextForElement(
                content->AsElement(),
                nsnull,
                presShell);
    } else {
        
        nsRefPtr<css::StyleRule> parentRule;
        rv = CreateFontStyleRule(kDefaultFontStyle,
                                 document,
                                 getter_AddRefs(parentRule));
        if (NS_FAILED(rv))
            return rv;
        nsCOMArray<nsIStyleRule> parentRules;
        parentRules.AppendObject(parentRule);
        parentContext = styleSet->ResolveStyleForRules(nsnull, parentRules);
    }

    if (!parentContext)
        return NS_ERROR_FAILURE;

    nsRefPtr<nsStyleContext> sc =
        styleSet->ResolveStyleForRules(parentContext, rules);
    if (!sc)
        return NS_ERROR_FAILURE;
    const nsStyleFont* fontStyle = sc->GetStyleFont();

    NS_ASSERTION(fontStyle, "Could not obtain font style");

    nsIAtom* language = sc->GetStyleVisibility()->mLanguage;
    if (!language) {
        language = presShell->GetPresContext()->GetLanguageFromCharset();
    }

    
    const PRUint32 aupcp = nsPresContext::AppUnitsPerCSSPixel();
    
    const nscoord fontSize = nsStyleFont::UnZoomText(parentContext->PresContext(), fontStyle->mFont.size);

    bool printerFont = (presShell->GetPresContext()->Type() == nsPresContext::eContext_PrintPreview ||
                          presShell->GetPresContext()->Type() == nsPresContext::eContext_Print);

    gfxFontStyle style(fontStyle->mFont.style,
                       fontStyle->mFont.weight,
                       fontStyle->mFont.stretch,
                       NSAppUnitsToFloatPixels(fontSize, float(aupcp)),
                       language,
                       fontStyle->mFont.sizeAdjust,
                       fontStyle->mFont.systemFont,
                       printerFont,
                       fontStyle->mFont.featureSettings,
                       fontStyle->mFont.languageOverride);

    CurrentState().fontGroup =
        gfxPlatform::GetPlatform()->CreateFontGroup(fontStyle->mFont.name,
                                                    &style,
                                                    presShell->GetPresContext()->GetUserFontSet());
    NS_ASSERTION(CurrentState().fontGroup, "Could not get font group");

    
    
    
    
    declaration->GetValue(eCSSProperty_font, CurrentState().font);

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetFont(nsAString& font)
{
    
    GetCurrentFontStyle();

    font = CurrentState().font;
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetTextAlign(const nsAString& ta)
{
    if (ta.EqualsLiteral("start"))
        CurrentState().textAlign = TEXT_ALIGN_START;
    else if (ta.EqualsLiteral("end"))
        CurrentState().textAlign = TEXT_ALIGN_END;
    else if (ta.EqualsLiteral("left"))
        CurrentState().textAlign = TEXT_ALIGN_LEFT;
    else if (ta.EqualsLiteral("right"))
        CurrentState().textAlign = TEXT_ALIGN_RIGHT;
    else if (ta.EqualsLiteral("center"))
        CurrentState().textAlign = TEXT_ALIGN_CENTER;

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetTextAlign(nsAString& ta)
{
    switch (CurrentState().textAlign)
    {
    case TEXT_ALIGN_START:
        ta.AssignLiteral("start");
        break;
    case TEXT_ALIGN_END:
        ta.AssignLiteral("end");
        break;
    case TEXT_ALIGN_LEFT:
        ta.AssignLiteral("left");
        break;
    case TEXT_ALIGN_RIGHT:
        ta.AssignLiteral("right");
        break;
    case TEXT_ALIGN_CENTER:
        ta.AssignLiteral("center");
        break;
    default:
        NS_ERROR("textAlign holds invalid value");
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetTextBaseline(const nsAString& tb)
{
    if (tb.EqualsLiteral("top"))
        CurrentState().textBaseline = TEXT_BASELINE_TOP;
    else if (tb.EqualsLiteral("hanging"))
        CurrentState().textBaseline = TEXT_BASELINE_HANGING;
    else if (tb.EqualsLiteral("middle"))
        CurrentState().textBaseline = TEXT_BASELINE_MIDDLE;
    else if (tb.EqualsLiteral("alphabetic"))
        CurrentState().textBaseline = TEXT_BASELINE_ALPHABETIC;
    else if (tb.EqualsLiteral("ideographic"))
        CurrentState().textBaseline = TEXT_BASELINE_IDEOGRAPHIC;
    else if (tb.EqualsLiteral("bottom"))
        CurrentState().textBaseline = TEXT_BASELINE_BOTTOM;

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetTextBaseline(nsAString& tb)
{
    switch (CurrentState().textBaseline)
    {
    case TEXT_BASELINE_TOP:
        tb.AssignLiteral("top");
        break;
    case TEXT_BASELINE_HANGING:
        tb.AssignLiteral("hanging");
        break;
    case TEXT_BASELINE_MIDDLE:
        tb.AssignLiteral("middle");
        break;
    case TEXT_BASELINE_ALPHABETIC:
        tb.AssignLiteral("alphabetic");
        break;
    case TEXT_BASELINE_IDEOGRAPHIC:
        tb.AssignLiteral("ideographic");
        break;
    case TEXT_BASELINE_BOTTOM:
        tb.AssignLiteral("bottom");
        break;
    default:
        NS_ERROR("textBaseline holds invalid value");
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}








static inline void
TextReplaceWhitespaceCharacters(nsAutoString& str)
{
    str.ReplaceChar("\x09\x0A\x0B\x0C\x0D", PRUnichar(' '));
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::FillText(const nsAString& text, float x, float y, float maxWidth)
{
    return DrawOrMeasureText(text, x, y, maxWidth, TEXT_DRAW_OPERATION_FILL, nsnull);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::StrokeText(const nsAString& text, float x, float y, float maxWidth)
{
    return DrawOrMeasureText(text, x, y, maxWidth, TEXT_DRAW_OPERATION_STROKE, nsnull);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::MeasureText(const nsAString& rawText,
                                        nsIDOMTextMetrics** _retval)
{
    float width;

    nsresult rv = DrawOrMeasureText(rawText, 0, 0, 0, TEXT_DRAW_OPERATION_MEASURE, &width);

    if (NS_FAILED(rv))
        return rv;

    nsRefPtr<nsIDOMTextMetrics> textMetrics = new nsTextMetrics(width);
    if (!textMetrics.get())
        return NS_ERROR_OUT_OF_MEMORY;

    *_retval = textMetrics.forget().get();

    return NS_OK;
}




struct NS_STACK_CLASS nsCanvasBidiProcessor : public nsBidiPresUtils::BidiProcessor
{
    virtual void SetText(const PRUnichar* text, PRInt32 length, nsBidiDirection direction)
    {
        mTextRun = gfxTextRunCache::MakeTextRun(text,
                                                length,
                                                mFontgrp,
                                                mThebes,
                                                mAppUnitsPerDevPixel,
                                                direction==NSBIDI_RTL ? gfxTextRunFactory::TEXT_IS_RTL : 0);
    }

    virtual nscoord GetWidth()
    {
        gfxTextRun::Metrics textRunMetrics = mTextRun->MeasureText(0,
                                                                   mTextRun->GetLength(),
                                                                   mDoMeasureBoundingBox ?
                                                                       gfxFont::TIGHT_INK_EXTENTS :
                                                                       gfxFont::LOOSE_INK_EXTENTS,
                                                                   mThebes,
                                                                   nsnull);

        
        
        if (mDoMeasureBoundingBox) {
            textRunMetrics.mBoundingBox.Scale(1.0 / mAppUnitsPerDevPixel);
            mBoundingBox = mBoundingBox.Union(textRunMetrics.mBoundingBox);
        }

        return NSToCoordRound(textRunMetrics.mAdvanceWidth);
    }

    virtual void DrawText(nscoord xOffset, nscoord width)
    {
        gfxPoint point = mPt;
        point.x += xOffset;

        
        if (mTextRun->IsRightToLeft()) {
            
            
            
            
            
            gfxTextRun::Metrics textRunMetrics =
                mTextRun->MeasureText(0,
                                      mTextRun->GetLength(),
                                      mDoMeasureBoundingBox ?
                                          gfxFont::TIGHT_INK_EXTENTS :
                                          gfxFont::LOOSE_INK_EXTENTS,
                                      mThebes,
                                      nsnull);
            point.x += textRunMetrics.mAdvanceWidth;
            
            
            
            
        }

        
        if (mOp == nsCanvasRenderingContext2D::TEXT_DRAW_OPERATION_STROKE)
            mTextRun->DrawToPath(mThebes,
                                 point,
                                 0,
                                 mTextRun->GetLength(),
                                 nsnull,
                                 nsnull);
        else
            
            mTextRun->Draw(mThebes,
                           point,
                           0,
                           mTextRun->GetLength(),
                           nsnull,
                           nsnull);
    }

    
    gfxTextRunCache::AutoTextRun mTextRun;

    
    
    gfxContext* mThebes;

    
    gfxPoint mPt;

    
    gfxFontGroup* mFontgrp;

    
    PRUint32 mAppUnitsPerDevPixel;

    
    nsCanvasRenderingContext2D::TextDrawOperation mOp;

    
    gfxRect mBoundingBox;

    
    bool mDoMeasureBoundingBox;
};

nsresult
nsCanvasRenderingContext2D::DrawOrMeasureText(const nsAString& aRawText,
                                              float aX,
                                              float aY,
                                              float aMaxWidth,
                                              TextDrawOperation aOp,
                                              float* aWidth)
{
    nsresult rv;

    if (!FloatValidate(aX, aY, aMaxWidth))
        return NS_ERROR_DOM_SYNTAX_ERR;

    
    
    
    
    if (aMaxWidth < 0)
        return NS_ERROR_INVALID_ARG;

    nsCOMPtr<nsIContent> content = do_QueryInterface(mCanvasElement);
    if (!content && !mDocShell) {
        NS_WARNING("Canvas element must be an nsIContent and non-null or a docshell must be provided");
        return NS_ERROR_FAILURE;
    }

    nsIPresShell* presShell = GetPresShell();
    if (!presShell)
        return NS_ERROR_FAILURE;

    nsIDocument* document = presShell->GetDocument();

    
    nsAutoString textToDraw(aRawText);
    TextReplaceWhitespaceCharacters(textToDraw);

    
    bool isRTL = false;

    if (content && content->IsInDoc()) {
        
        nsRefPtr<nsStyleContext> canvasStyle =
            nsComputedDOMStyle::GetStyleContextForElement(content->AsElement(),
                                                          nsnull,
                                                          presShell);
        if (!canvasStyle)
            return NS_ERROR_FAILURE;
        isRTL = canvasStyle->GetStyleVisibility()->mDirection ==
            NS_STYLE_DIRECTION_RTL;
    } else {
      isRTL = GET_BIDI_OPTION_DIRECTION(document->GetBidiOptions()) == IBMBIDI_TEXTDIRECTION_RTL;
    }

    
    bool doDrawShadow = aOp == TEXT_DRAW_OPERATION_FILL && NeedToDrawShadow();
    bool doUseIntermediateSurface = aOp == TEXT_DRAW_OPERATION_FILL &&
        (NeedToUseIntermediateSurface() || NeedIntermediateSurfaceToHandleGlobalAlpha(STYLE_FILL));

    
    ClearSurfaceForUnboundedSource();

    nsCanvasBidiProcessor processor;

    GetAppUnitsValues(&processor.mAppUnitsPerDevPixel, NULL);
    processor.mPt = gfxPoint(aX, aY);
    nsRefPtr<nsRenderingContext> ctx;
    if (mThebes) {
        processor.mThebes = mThebes;
    } else {
        ctx = presShell->GetReferenceRenderingContext();
        processor.mThebes = ctx->ThebesContext(); 
    }
    processor.mOp = aOp;
    processor.mBoundingBox = gfxRect(0, 0, 0, 0);
    processor.mDoMeasureBoundingBox = doDrawShadow || !mIsEntireFrameInvalid;

    processor.mFontgrp = GetCurrentFontStyle();
    NS_ASSERTION(processor.mFontgrp, "font group is null");

    nscoord totalWidthCoord;

    
    
    nsBidi bidiEngine;
    rv = nsBidiPresUtils::ProcessText(textToDraw.get(),
                                      textToDraw.Length(),
                                      isRTL ? NSBIDI_RTL : NSBIDI_LTR,
                                      presShell->GetPresContext(),
                                      processor,
                                      nsBidiPresUtils::MODE_MEASURE,
                                      nsnull,
                                      0,
                                      &totalWidthCoord,
                                      &bidiEngine);
    if (NS_FAILED(rv))
        return rv;

    float totalWidth = float(totalWidthCoord) / processor.mAppUnitsPerDevPixel;
    if (aWidth)
        *aWidth = totalWidth;

    
    if (aOp==TEXT_DRAW_OPERATION_MEASURE)
        return NS_OK;

    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    processor.mThebes = mThebes;

    
    gfxFloat anchorX;

    if (CurrentState().textAlign == TEXT_ALIGN_CENTER)
        anchorX = .5;
    else if (CurrentState().textAlign == TEXT_ALIGN_LEFT ||
             (!isRTL && CurrentState().textAlign == TEXT_ALIGN_START) ||
             (isRTL && CurrentState().textAlign == TEXT_ALIGN_END))
        anchorX = 0;
    else
        anchorX = 1;

    processor.mPt.x -= anchorX * totalWidth;

    
    NS_ASSERTION(processor.mFontgrp->FontListLength()>0, "font group contains no fonts");
    const gfxFont::Metrics& fontMetrics = processor.mFontgrp->GetFontAt(0)->GetMetrics();

    gfxFloat anchorY;

    switch (CurrentState().textBaseline)
    {
    case TEXT_BASELINE_HANGING:
        
    case TEXT_BASELINE_TOP:
        anchorY = fontMetrics.emAscent;
        break;
        break;
    case TEXT_BASELINE_MIDDLE:
        anchorY = (fontMetrics.emAscent - fontMetrics.emDescent) * .5f;
        break;
    case TEXT_BASELINE_IDEOGRAPHIC:
        
    case TEXT_BASELINE_ALPHABETIC:
        anchorY = 0;
        break;
    case TEXT_BASELINE_BOTTOM:
        anchorY = -fontMetrics.emDescent;
        break;
    default:
        NS_ERROR("mTextBaseline holds invalid value");
        return NS_ERROR_FAILURE;
    }

    processor.mPt.y += anchorY;

    
    processor.mBoundingBox.width = totalWidth;
    processor.mBoundingBox.MoveBy(processor.mPt);

    processor.mPt.x *= processor.mAppUnitsPerDevPixel;
    processor.mPt.y *= processor.mAppUnitsPerDevPixel;

    
    
    gfxContextAutoSaveRestore autoSR;
    if (aMaxWidth > 0 && totalWidth > aMaxWidth) {
        autoSR.SetContext(mThebes);
        
        gfxPoint trans(aX, 0);
        mThebes->Translate(trans);
        mThebes->Scale(aMaxWidth/totalWidth, 1);
        mThebes->Translate(-trans);
    }

    
    gfxRect boundingBox = processor.mBoundingBox;

    
    processor.mDoMeasureBoundingBox = false;

    if (doDrawShadow) {
        
        processor.mBoundingBox.Inflate(2.0);

        
        
        gfxRect drawExtents = mThebes->UserToDevice(processor.mBoundingBox);
        gfxAlphaBoxBlur blur;

        gfxContext* ctx = ShadowInitialize(drawExtents, blur);

        if (ctx) {
            CopyContext(ctx, mThebes);
            ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
            processor.mThebes = ctx;

            rv = nsBidiPresUtils::ProcessText(textToDraw.get(),
                                              textToDraw.Length(),
                                              isRTL ? NSBIDI_RTL : NSBIDI_LTR,
                                              presShell->GetPresContext(),
                                              processor,
                                              nsBidiPresUtils::MODE_DRAW,
                                              nsnull,
                                              0,
                                              nsnull,
                                              &bidiEngine);
            if (NS_FAILED(rv))
                return rv;

            ShadowFinalize(blur);
        }

        processor.mThebes = mThebes;
    }

    gfxContextPathAutoSaveRestore pathSR(mThebes, false);

    
    if (aOp == nsCanvasRenderingContext2D::TEXT_DRAW_OPERATION_STROKE) {
        pathSR.Save();
        mThebes->NewPath();
    }
    
    else {
        if (doUseIntermediateSurface) {
            mThebes->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);

            
            mThebes->SetOperator(gfxContext::OPERATOR_SOURCE);
        }

        ApplyStyle(STYLE_FILL);
    }

    rv = nsBidiPresUtils::ProcessText(textToDraw.get(),
                                      textToDraw.Length(),
                                      isRTL ? NSBIDI_RTL : NSBIDI_LTR,
                                      presShell->GetPresContext(),
                                      processor,
                                      nsBidiPresUtils::MODE_DRAW,
                                      nsnull,
                                      0,
                                      nsnull,
                                      &bidiEngine);

    
    if (doUseIntermediateSurface) {
        mThebes->PopGroupToSource();
        DirtyAllStyles();
    }

    if (NS_FAILED(rv))
        return rv;

    if (aOp == nsCanvasRenderingContext2D::TEXT_DRAW_OPERATION_STROKE) {
        
        rv = DrawPath(STYLE_STROKE);
        if (NS_FAILED(rv))
            return rv;
    } else if (doUseIntermediateSurface)
        mThebes->Paint(CurrentState().StyleIsColor(STYLE_FILL) ? 1.0 : CurrentState().globalAlpha);

    if (aOp == nsCanvasRenderingContext2D::TEXT_DRAW_OPERATION_FILL && !doDrawShadow)
        return RedrawUser(boundingBox);

    return Redraw();
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetMozTextStyle(const nsAString& textStyle)
{
    
    return SetFont(textStyle);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetMozTextStyle(nsAString& textStyle)
{
    
    return GetFont(textStyle);
}

gfxFontGroup*
nsCanvasRenderingContext2D::GetCurrentFontStyle()
{
    
    if(!CurrentState().fontGroup) {
        nsresult rv = SetMozTextStyle(kDefaultFontStyle);
        if (NS_FAILED(rv)) {
            gfxFontStyle style;
            style.size = kDefaultFontSize;
            CurrentState().fontGroup =
                gfxPlatform::GetPlatform()->CreateFontGroup(kDefaultFontName,
                                                            &style,
                                                            nsnull);
            if (CurrentState().fontGroup) {
                CurrentState().font = kDefaultFontStyle;
                rv = NS_OK;
            } else {
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
        }
            
        NS_ASSERTION(NS_SUCCEEDED(rv), "Default canvas font is invalid");
    }

    return CurrentState().fontGroup;
}

gfxTextRun*
nsCanvasRenderingContext2D::MakeTextRun(const PRUnichar* aText,
                                        PRUint32         aLength,
                                        PRUint32         aAppUnitsPerDevUnit,
                                        PRUint32         aFlags)
{
    gfxFontGroup* currentFontStyle = GetCurrentFontStyle();
    if (!currentFontStyle)
        return nsnull;
    return gfxTextRunCache::MakeTextRun(aText, aLength, currentFontStyle,
                                        mThebes, aAppUnitsPerDevUnit, aFlags);
}





NS_IMETHODIMP
nsCanvasRenderingContext2D::SetLineWidth(float width)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(width) || width <= 0.0)
        return NS_OK;

    mThebes->SetLineWidth(width);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetLineWidth(float *width)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;
 
    gfxFloat d = mThebes->CurrentLineWidth();
    *width = static_cast<float>(d);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetLineCap(const nsAString& capstyle)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    gfxContext::GraphicsLineCap cap;

    if (capstyle.EqualsLiteral("butt"))
        cap = gfxContext::LINE_CAP_BUTT;
    else if (capstyle.EqualsLiteral("round"))
        cap = gfxContext::LINE_CAP_ROUND;
    else if (capstyle.EqualsLiteral("square"))
        cap = gfxContext::LINE_CAP_SQUARE;
    else
        
        return NS_OK;

    mThebes->SetLineCap(cap);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetLineCap(nsAString& capstyle)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    gfxContext::GraphicsLineCap cap = mThebes->CurrentLineCap();

    if (cap == gfxContext::LINE_CAP_BUTT)
        capstyle.AssignLiteral("butt");
    else if (cap == gfxContext::LINE_CAP_ROUND)
        capstyle.AssignLiteral("round");
    else if (cap == gfxContext::LINE_CAP_SQUARE)
        capstyle.AssignLiteral("square");
    else
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetLineJoin(const nsAString& joinstyle)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    gfxContext::GraphicsLineJoin j;

    if (joinstyle.EqualsLiteral("round"))
        j = gfxContext::LINE_JOIN_ROUND;
    else if (joinstyle.EqualsLiteral("bevel"))
        j = gfxContext::LINE_JOIN_BEVEL;
    else if (joinstyle.EqualsLiteral("miter"))
        j = gfxContext::LINE_JOIN_MITER;
    else
        
        return NS_OK;

    mThebes->SetLineJoin(j);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetLineJoin(nsAString& joinstyle)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    gfxContext::GraphicsLineJoin j = mThebes->CurrentLineJoin();

    if (j == gfxContext::LINE_JOIN_ROUND)
        joinstyle.AssignLiteral("round");
    else if (j == gfxContext::LINE_JOIN_BEVEL)
        joinstyle.AssignLiteral("bevel");
    else if (j == gfxContext::LINE_JOIN_MITER)
        joinstyle.AssignLiteral("miter");
    else
        return NS_ERROR_FAILURE;

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetMiterLimit(float miter)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(miter) || miter <= 0.0)
        return NS_OK;

    mThebes->SetMiterLimit(miter);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetMiterLimit(float *miter)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    gfxFloat d = mThebes->CurrentMiterLimit();
    *miter = static_cast<float>(d);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetMozDash(JSContext *cx, const jsval& patternArray)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    AutoFallibleTArray<gfxFloat, 10> dashes;
    nsresult rv = JSValToDashArray(cx, patternArray, dashes);
    if (NS_SUCCEEDED(rv)) {
        mThebes->SetDash(dashes.Elements(), dashes.Length(),
                         mThebes->CurrentDashOffset());
    }
    return rv;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetMozDash(JSContext* cx, jsval* dashArray)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    AutoFallibleTArray<gfxFloat, 10> dashes;
    if (!mThebes->CurrentDash(dashes, nsnull)) {
        dashes.SetLength(0);
    }
    return DashArrayToJSVal(dashes, cx, dashArray);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetMozDashOffset(float offset)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(offset)) {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    AutoFallibleTArray<gfxFloat, 10> dashes;
    if (!mThebes->CurrentDash(dashes, nsnull)) {
        
        

        
        return NS_OK;
    }
    NS_ABORT_IF_FALSE(dashes.Length() > 0,
                      "CurrentDash() should have returned false");

    mThebes->SetDash(dashes.Elements(), dashes.Length(),
                     gfxFloat(offset));

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetMozDashOffset(float* offset)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    *offset = float(mThebes->CurrentDashOffset());
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::IsPointInPath(float x, float y, bool *retVal)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!FloatValidate(x,y)) {
        *retVal = false;
        return NS_OK;
    }

    gfxPoint pt(x, y);
    *retVal = mThebes->PointInFill(mThebes->DeviceToUser(pt));
    return NS_OK;
}












NS_IMETHODIMP
nsCanvasRenderingContext2D::DrawImage(nsIDOMElement *imgElt, float a1,
                                      float a2, float a3, float a4, float a5,
                                      float a6, float a7, float a8,
                                      PRUint8 optional_argc)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIContent> content = do_QueryInterface(imgElt);
    if (!content) {
        return NS_ERROR_DOM_TYPE_MISMATCH_ERR;
    }

    nsHTMLCanvasElement* canvas = nsHTMLCanvasElement::FromContent(content);
    if (canvas) {
        nsIntSize size = canvas->GetSize();
        if (size.width == 0 || size.height == 0) {
            return NS_ERROR_DOM_INVALID_STATE_ERR;
        }
    }

    gfxMatrix matrix;
    nsRefPtr<gfxPattern> pattern;
    gfxIntSize imgSize;
    nsRefPtr<gfxASurface> imgsurf =
      CanvasImageCache::Lookup(imgElt, HTMLCanvasElement(), &imgSize);

    if (!imgsurf) {
        
        
        PRUint32 sfeFlags = nsLayoutUtils::SFE_WANT_FIRST_FRAME;
        nsLayoutUtils::SurfaceFromElementResult res =
            nsLayoutUtils::SurfaceFromElement(content->AsElement(), sfeFlags);
        if (!res.mSurface) {
            
            return res.mIsStillLoading ? NS_OK : NS_ERROR_NOT_AVAILABLE;
        }

        
        
        if (res.mSurface == mSurface) {
            sfeFlags |= nsLayoutUtils::SFE_WANT_NEW_SURFACE;
            res = nsLayoutUtils::SurfaceFromElement(content->AsElement(),
                                                    sfeFlags);
            if (!res.mSurface)
                return NS_ERROR_NOT_AVAILABLE;
        }

        imgsurf = res.mSurface.forget();
        imgSize = res.mSize;

        if (mCanvasElement) {
            CanvasUtils::DoDrawImageSecurityCheck(HTMLCanvasElement(),
                                                  res.mPrincipal,
                                                  res.mIsWriteOnly,
                                                  res.mCORSUsed);
        }

        if (res.mImageRequest) {
            CanvasImageCache::NotifyDrawImage(imgElt, HTMLCanvasElement(),
                                              res.mImageRequest, imgsurf, imgSize);
        }
    }

    double sx,sy,sw,sh;
    double dx,dy,dw,dh;
    if (optional_argc == 0) {
        dx = a1;
        dy = a2;
        sx = sy = 0.0;
        dw = sw = (double) imgSize.width;
        dh = sh = (double) imgSize.height;
    } else if (optional_argc == 2) {
        dx = a1;
        dy = a2;
        dw = a3;
        dh = a4;
        sx = sy = 0.0;
        sw = (double) imgSize.width;
        sh = (double) imgSize.height;
    } else if (optional_argc == 6) {
        sx = a1;
        sy = a2;
        sw = a3;
        sh = a4;
        dx = a5;
        dy = a6;
        dw = a7;
        dh = a8;
    } else {
        
        return NS_ERROR_INVALID_ARG;
    }

    if (dw == 0.0 || dh == 0.0) {
        
        
        return NS_OK;
    }

    if (!FloatValidate(sx, sy, sw, sh) || !FloatValidate(dx, dy, dw, dh)) {
        return NS_OK;
    }

    
    if (sx < 0.0 || sy < 0.0 ||
        sw < 0.0 || sw > (double) imgSize.width ||
        sh < 0.0 || sh > (double) imgSize.height ||
        dw < 0.0 || dh < 0.0)
    {
        
        return NS_ERROR_DOM_INDEX_SIZE_ERR;
    }

    matrix.Translate(gfxPoint(sx, sy));
    matrix.Scale(sw/dw, sh/dh);

    pattern = new gfxPattern(imgsurf);
    pattern->SetMatrix(matrix);
    pattern->SetExtend(gfxPattern::EXTEND_PAD);

    if (CurrentState().imageSmoothingEnabled)
        pattern->SetFilter(gfxPattern::FILTER_GOOD);
    else
        pattern->SetFilter(gfxPattern::FILTER_NEAREST);

    PathAutoSaveRestore pathSR(this);

    
    ClearSurfaceForUnboundedSource();

    {
        gfxContextMatrixAutoSaveRestore autoMatrixSR(mThebes);

        mThebes->Translate(gfxPoint(dx, dy));

        gfxRect clip(0, 0, dw, dh);

        if (NeedToDrawShadow()) {
            gfxRect drawExtents = mThebes->UserToDevice(clip);
            gfxAlphaBoxBlur blur;

            gfxContext* ctx = ShadowInitialize(drawExtents, blur);

            if (ctx) {
                CopyContext(ctx, mThebes);
                ctx->SetPattern(pattern);
                ctx->SetOperator(gfxContext::OPERATOR_SOURCE);
                ctx->Clip(clip);
                ctx->Paint();

                ShadowFinalize(blur);
            }
        }

        mThebes->SetPattern(pattern);
        DirtyAllStyles();

        bool doUseIntermediateSurface = NeedToUseIntermediateSurface();
        if (doUseIntermediateSurface) {
            gfxContextAutoSaveRestore autoSR(mThebes);

            
            mThebes->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
            mThebes->Clip(clip);

            
            mThebes->SetOperator(gfxContext::OPERATOR_SOURCE);

            mThebes->Paint();
            mThebes->PopGroupToSource();
            mThebes->Paint(CurrentState().globalAlpha);
        } else if (CurrentState().globalAlpha == 1.0f &&
                   mThebes->CurrentOperator() == gfxContext::OPERATOR_OVER) {
            
            mThebes->NewPath();
            mThebes->Rectangle(clip);
            mThebes->Fill();
        } else {
            gfxContextAutoSaveRestore autoSR(mThebes);

            
            mThebes->Clip(clip);
            mThebes->Paint(CurrentState().globalAlpha);
        }

        RedrawUser(clip);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetGlobalCompositeOperation(const nsAString& op)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    gfxContext::GraphicsOperator thebes_op;

#define CANVAS_OP_TO_THEBES_OP(cvsop,thebesop) \
    if (op.EqualsLiteral(cvsop))   \
        thebes_op = gfxContext::OPERATOR_##thebesop;

    CANVAS_OP_TO_THEBES_OP("copy", SOURCE)
    else CANVAS_OP_TO_THEBES_OP("destination-atop", DEST_ATOP)
    else CANVAS_OP_TO_THEBES_OP("destination-in", DEST_IN)
    else CANVAS_OP_TO_THEBES_OP("destination-out", DEST_OUT)
    else CANVAS_OP_TO_THEBES_OP("destination-over", DEST_OVER)
    else CANVAS_OP_TO_THEBES_OP("lighter", ADD)
    else CANVAS_OP_TO_THEBES_OP("source-atop", ATOP)
    else CANVAS_OP_TO_THEBES_OP("source-in", IN)
    else CANVAS_OP_TO_THEBES_OP("source-out", OUT)
    else CANVAS_OP_TO_THEBES_OP("source-over", OVER)
    else CANVAS_OP_TO_THEBES_OP("xor", XOR)
    
    else return NS_OK;

#undef CANVAS_OP_TO_THEBES_OP

    mThebes->SetOperator(thebes_op);
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetGlobalCompositeOperation(nsAString& op)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    gfxContext::GraphicsOperator thebes_op = mThebes->CurrentOperator();

#define CANVAS_OP_TO_THEBES_OP(cvsop,thebesop) \
    if (thebes_op == gfxContext::OPERATOR_##thebesop) \
        op.AssignLiteral(cvsop);

    CANVAS_OP_TO_THEBES_OP("copy", SOURCE)
    else CANVAS_OP_TO_THEBES_OP("destination-atop", DEST_ATOP)
    else CANVAS_OP_TO_THEBES_OP("destination-in", DEST_IN)
    else CANVAS_OP_TO_THEBES_OP("destination-out", DEST_OUT)
    else CANVAS_OP_TO_THEBES_OP("destination-over", DEST_OVER)
    else CANVAS_OP_TO_THEBES_OP("lighter", ADD)
    else CANVAS_OP_TO_THEBES_OP("source-atop", ATOP)
    else CANVAS_OP_TO_THEBES_OP("source-in", IN)
    else CANVAS_OP_TO_THEBES_OP("source-out", OUT)
    else CANVAS_OP_TO_THEBES_OP("source-over", OVER)
    else CANVAS_OP_TO_THEBES_OP("xor", XOR)
    else return NS_ERROR_FAILURE;

#undef CANVAS_OP_TO_THEBES_OP

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::DrawWindow(nsIDOMWindow* aWindow, float aX, float aY,
                                       float aW, float aH,
                                       const nsAString& aBGColor,
                                       PRUint32 flags)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    NS_ENSURE_ARG(aWindow != nsnull);

    
    
    if (!gfxASurface::CheckSurfaceSize(gfxIntSize(PRInt32(aW), PRInt32(aH)),
                                       0xffff))
        return NS_ERROR_FAILURE;

    
    
    
    
    
    
    if (!nsContentUtils::IsCallerTrustedForRead()) {
      
      
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    
    if (!(flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DO_NOT_FLUSH))
        nsContentUtils::FlushLayoutForTree(aWindow);

    nsRefPtr<nsPresContext> presContext;
    nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(aWindow);
    if (win) {
        nsIDocShell* docshell = win->GetDocShell();
        if (docshell) {
            docshell->GetPresContext(getter_AddRefs(presContext));
        }
    }
    if (!presContext)
        return NS_ERROR_FAILURE;

    nscolor bgColor;

    nsIDocument* elementDoc = mCanvasElement ?
                              HTMLCanvasElement()->OwnerDoc() : nsnull;

    
    
    nsCSSParser parser(elementDoc ? elementDoc->CSSLoader() : nsnull);
    nsresult rv = parser.ParseColorString(PromiseFlatString(aBGColor),
                                          nsnull, 0, &bgColor);
    NS_ENSURE_SUCCESS(rv, rv);

    nsIPresShell* presShell = presContext->PresShell();
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    nsRect r(nsPresContext::CSSPixelsToAppUnits(aX),
             nsPresContext::CSSPixelsToAppUnits(aY),
             nsPresContext::CSSPixelsToAppUnits(aW),
             nsPresContext::CSSPixelsToAppUnits(aH));
    PRUint32 renderDocFlags = (nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING |
                               nsIPresShell::RENDER_DOCUMENT_RELATIVE);
    if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DRAW_CARET) {
        renderDocFlags |= nsIPresShell::RENDER_CARET;
    }
    if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DRAW_VIEW) {
        renderDocFlags &= ~(nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING |
                            nsIPresShell::RENDER_DOCUMENT_RELATIVE);
    }
    if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_USE_WIDGET_LAYERS) {
        renderDocFlags |= nsIPresShell::RENDER_USE_WIDGET_LAYERS;
    }
    if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_ASYNC_DECODE_IMAGES) {
        renderDocFlags |= nsIPresShell::RENDER_ASYNC_DECODE_IMAGES;
    }

    rv = presShell->RenderDocument(r, renderDocFlags, bgColor, mThebes);

    
    mThebes->SetColor(gfxRGBA(1,1,1,1));
    DirtyAllStyles();

    
    
    
    RedrawUser(gfxRect(0, 0, aW, aH));

    return rv;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::AsyncDrawXULElement(nsIDOMXULElement* aElem, float aX, float aY,
                                                float aW, float aH,
                                                const nsAString& aBGColor,
                                                PRUint32 flags)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    NS_ENSURE_ARG(aElem != nsnull);

    
    
    
    
    
    
    if (!nsContentUtils::IsCallerTrustedForRead()) {
        
        
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    nsCOMPtr<nsIFrameLoaderOwner> loaderOwner = do_QueryInterface(aElem);
    if (!loaderOwner)
        return NS_ERROR_FAILURE;

    nsRefPtr<nsFrameLoader> frameloader = loaderOwner->GetFrameLoader();
    if (!frameloader)
        return NS_ERROR_FAILURE;

    PBrowserParent *child = frameloader->GetRemoteBrowser();
    if (!child) {
        nsCOMPtr<nsIDOMWindow> window =
            do_GetInterface(frameloader->GetExistingDocShell());
        if (!window)
            return NS_ERROR_FAILURE;

        return DrawWindow(window, aX, aY, aW, aH, aBGColor, flags);
    }

    
    
    if (!gfxASurface::CheckSurfaceSize(gfxIntSize(aW, aH), 0xffff))
        return NS_ERROR_FAILURE;

    bool flush =
        (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DO_NOT_FLUSH) == 0;

    PRUint32 renderDocFlags = nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING;
    if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DRAW_CARET) {
        renderDocFlags |= nsIPresShell::RENDER_CARET;
    }
    if (flags & nsIDOMCanvasRenderingContext2D::DRAWWINDOW_DRAW_VIEW) {
        renderDocFlags &= ~nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING;
    }

    nsRect rect(nsPresContext::CSSPixelsToAppUnits(aX),
                nsPresContext::CSSPixelsToAppUnits(aY),
                nsPresContext::CSSPixelsToAppUnits(aW),
                nsPresContext::CSSPixelsToAppUnits(aH));
    if (mIPC) {
        PDocumentRendererParent *pdocrender =
            child->SendPDocumentRendererConstructor(rect,
                                                    mThebes->CurrentMatrix(),
                                                    nsString(aBGColor),
                                                    renderDocFlags, flush,
                                                    nsIntSize(mWidth, mHeight));
        if (!pdocrender)
            return NS_ERROR_FAILURE;

        DocumentRendererParent *docrender =
            static_cast<DocumentRendererParent *>(pdocrender);

        docrender->SetCanvasContext(this, mThebes);
    }

    return NS_OK;
}





void
nsCanvasRenderingContext2D::EnsureUnpremultiplyTable() {
  if (sUnpremultiplyTable)
    return;

  
  sUnpremultiplyTable = new PRUint8[256][256];

  
  
  
  
  

  
  for (PRUint32 c = 0; c <= 255; c++) {
    sUnpremultiplyTable[0][c] = c;
  }

  for (int a = 1; a <= 255; a++) {
    for (int c = 0; c <= 255; c++) {
      sUnpremultiplyTable[a][c] = (PRUint8)((c * 255) / a);
    }
  }
}


NS_IMETHODIMP
nsCanvasRenderingContext2D::GetImageData()
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetImageData_explicit(PRInt32 x, PRInt32 y, PRUint32 w, PRUint32 h,
                                                  PRUint8 *aData, PRUint32 aDataLen)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (!mCanvasElement && !mDocShell) {
        NS_ERROR("No canvas element and no docshell in GetImageData!!!");
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    
    
    if (mCanvasElement &&
        HTMLCanvasElement()->IsWriteOnly() &&
        !nsContentUtils::IsCallerTrustedForRead())
    {
        
        return NS_ERROR_DOM_SECURITY_ERR;
    }

    if (w == 0 || h == 0 || aDataLen != w * h * 4)
        return NS_ERROR_DOM_SYNTAX_ERR;

    CheckedInt32 rightMost = CheckedInt32(x) + w;
    CheckedInt32 bottomMost = CheckedInt32(y) + h;

    if (!rightMost.valid() || !bottomMost.valid())
        return NS_ERROR_DOM_SYNTAX_ERR;

    
    nsRefPtr<gfxImageSurface> tmpsurf =
        new gfxImageSurface(aData,
                            gfxIntSize(w, h),
                            w * 4,
                            gfxASurface::ImageFormatARGB32);

    if (tmpsurf->CairoStatus())
        return NS_ERROR_FAILURE;

    nsRefPtr<gfxContext> tmpctx = new gfxContext(tmpsurf);

    if (tmpctx->HasError())
        return NS_ERROR_FAILURE;

    if (!mZero) {
        gfxRect srcRect(0, 0, mWidth, mHeight);
        gfxRect destRect(x, y, w, h);

        bool finishedPainting = false;
        
        if (!srcRect.Contains(destRect)) {
            
            gfxRect tmp = srcRect.Intersect(destRect);
            finishedPainting = tmp.IsEmpty();

            
            if (!finishedPainting) {
                tmpctx->Rectangle(tmp);
            }
        }

        if (!finishedPainting) {
            tmpctx->SetOperator(gfxContext::OPERATOR_SOURCE);
            tmpctx->SetSource(mSurface, gfxPoint(-x, -y));
            tmpctx->Paint();
        }
    }

    
    EnsureUnpremultiplyTable();

    
    
    PRUint8 *src = aData;
    PRUint8 *dst = aData;

    for (PRUint32 j = 0; j < h; j++) {
        for (PRUint32 i = 0; i < w; i++) {
            
#ifdef IS_LITTLE_ENDIAN
            PRUint8 b = *src++;
            PRUint8 g = *src++;
            PRUint8 r = *src++;
            PRUint8 a = *src++;
#else
            PRUint8 a = *src++;
            PRUint8 r = *src++;
            PRUint8 g = *src++;
            PRUint8 b = *src++;
#endif
            
            *dst++ = sUnpremultiplyTable[a][r];
            *dst++ = sUnpremultiplyTable[a][g];
            *dst++ = sUnpremultiplyTable[a][b];
            *dst++ = a;
        }
    }

    return NS_OK;
}

void
nsCanvasRenderingContext2D::EnsurePremultiplyTable() {
  if (sPremultiplyTable)
    return;

  
  sPremultiplyTable = new PRUint8[256][256];

  
  
  

  for (int a = 0; a <= 255; a++) {
    for (int c = 0; c <= 255; c++) {
      sPremultiplyTable[a][c] = (a * c + 254) / 255;
    }
  }
}


NS_IMETHODIMP
nsCanvasRenderingContext2D::PutImageData()
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::PutImageData_explicit(PRInt32 x, PRInt32 y, PRUint32 w, PRUint32 h,
                                                  unsigned char *aData, PRUint32 aDataLen,
                                                  bool hasDirtyRect, PRInt32 dirtyX, PRInt32 dirtyY,
                                                  PRInt32 dirtyWidth, PRInt32 dirtyHeight)
{
    if (!EnsureSurface())
        return NS_ERROR_FAILURE;

    if (w == 0 || h == 0)
        return NS_ERROR_DOM_SYNTAX_ERR;

    gfxRect dirtyRect;
    gfxRect imageDataRect(0, 0, w, h);

    if (hasDirtyRect) {
        
        if (dirtyWidth < 0) {
            NS_ENSURE_TRUE(dirtyWidth != INT_MIN, NS_ERROR_DOM_INDEX_SIZE_ERR);

            CheckedInt32 checkedDirtyX = CheckedInt32(dirtyX) + dirtyWidth;

            if (!checkedDirtyX.valid())
                return NS_ERROR_DOM_INDEX_SIZE_ERR;

            dirtyX = checkedDirtyX.value();
            dirtyWidth = -(int32)dirtyWidth;
        }

        if (dirtyHeight < 0) {
            NS_ENSURE_TRUE(dirtyHeight != INT_MIN, NS_ERROR_DOM_INDEX_SIZE_ERR);

            CheckedInt32 checkedDirtyY = CheckedInt32(dirtyY) + dirtyHeight;

            if (!checkedDirtyY.valid())
                return NS_ERROR_DOM_INDEX_SIZE_ERR;

            dirtyY = checkedDirtyY.value();
            dirtyHeight = -(int32)dirtyHeight;
        }

        
        dirtyRect = imageDataRect.Intersect(gfxRect(dirtyX, dirtyY, dirtyWidth, dirtyHeight));

        if (dirtyRect.Width() <= 0 || dirtyRect.Height() <= 0)
            return NS_OK;
    } else {
        dirtyRect = imageDataRect;
    }

    dirtyRect.MoveBy(gfxPoint(x, y));
    dirtyRect = gfxRect(0, 0, mWidth, mHeight).Intersect(dirtyRect);

    if (dirtyRect.Width() <= 0 || dirtyRect.Height() <= 0)
        return NS_OK;

    PRUint32 len = w * h * 4;
    if (aDataLen != len)
        return NS_ERROR_DOM_SYNTAX_ERR;

    nsRefPtr<gfxImageSurface> imgsurf = new gfxImageSurface(gfxIntSize(w, h),
                                                            gfxASurface::ImageFormatARGB32);
    if (!imgsurf || imgsurf->CairoStatus())
        return NS_ERROR_FAILURE;

    
    EnsurePremultiplyTable();

    PRUint8 *src = aData;
    PRUint8 *dst = imgsurf->Data();

    for (PRUint32 j = 0; j < h; j++) {
        for (PRUint32 i = 0; i < w; i++) {
            PRUint8 r = *src++;
            PRUint8 g = *src++;
            PRUint8 b = *src++;
            PRUint8 a = *src++;

            
#ifdef IS_LITTLE_ENDIAN
            *dst++ = sPremultiplyTable[a][b];
            *dst++ = sPremultiplyTable[a][g];
            *dst++ = sPremultiplyTable[a][r];
            *dst++ = a;
#else
            *dst++ = a;
            *dst++ = sPremultiplyTable[a][r];
            *dst++ = sPremultiplyTable[a][g];
            *dst++ = sPremultiplyTable[a][b];
#endif
        }
    }

    PathAutoSaveRestore pathSR(this);
    gfxContextAutoSaveRestore autoSR(mThebes);

    
    mThebes->ResetClip();

    mThebes->IdentityMatrix();
    mThebes->NewPath();
    mThebes->Rectangle(dirtyRect);
    mThebes->SetSource(imgsurf, gfxPoint(x, y));
    mThebes->SetOperator(gfxContext::OPERATOR_SOURCE);
    mThebes->Fill();

    return Redraw(dirtyRect);
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetThebesSurface(gfxASurface **surface)
{
    if (!EnsureSurface()) {
        *surface = nsnull;
        return NS_ERROR_NOT_AVAILABLE;
    }

    *surface = mSurface.get();
    NS_ADDREF(*surface);

    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::CreateImageData()
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::GetMozImageSmoothingEnabled(bool *retVal)
{
    *retVal = CurrentState().imageSmoothingEnabled;
    return NS_OK;
}

NS_IMETHODIMP
nsCanvasRenderingContext2D::SetMozImageSmoothingEnabled(bool val)
{
    if (val != CurrentState().imageSmoothingEnabled) {
        CurrentState().imageSmoothingEnabled = val;
        DirtyAllStyles();
    }

    return NS_OK;
}

static PRUint8 g2DContextLayerUserData;

class CanvasRenderingContext2DUserData : public LayerUserData {
public:
  CanvasRenderingContext2DUserData(nsHTMLCanvasElement *aContent)
    : mContent(aContent) {}
  static void DidTransactionCallback(void* aData)
  {
    static_cast<CanvasRenderingContext2DUserData*>(aData)->mContent->MarkContextClean();
  }

private:
  nsRefPtr<nsHTMLCanvasElement> mContent;
};

already_AddRefed<CanvasLayer>
nsCanvasRenderingContext2D::GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                           CanvasLayer *aOldLayer,
                                           LayerManager *aManager)
{
    if (!EnsureSurface()) 
        return nsnull;

    if (!mResetLayer && aOldLayer &&
        aOldLayer->HasUserData(&g2DContextLayerUserData)) {
        NS_ADDREF(aOldLayer);
        return aOldLayer;
    }

    nsRefPtr<CanvasLayer> canvasLayer = aManager->CreateCanvasLayer();
    if (!canvasLayer) {
        NS_WARNING("CreateCanvasLayer returned null!");
        return nsnull;
    }
    CanvasRenderingContext2DUserData *userData = nsnull;
    if (aBuilder->IsPaintingToWindow()) {
      
      
      
      
      
      

      
      
      
      
      
      userData = new CanvasRenderingContext2DUserData(HTMLCanvasElement());
      canvasLayer->SetDidTransactionCallback(
              CanvasRenderingContext2DUserData::DidTransactionCallback, userData);
    }
    canvasLayer->SetUserData(&g2DContextLayerUserData, userData);

    CanvasLayer::Data data;

    data.mSurface = mSurface.get();
    data.mSize = nsIntSize(mWidth, mHeight);

    canvasLayer->Initialize(data);
    PRUint32 flags = mOpaque ? Layer::CONTENT_OPAQUE : 0;
    canvasLayer->SetContentFlags(flags);
    canvasLayer->Updated();

    mResetLayer = false;

    return canvasLayer.forget();
}

bool
nsCanvasRenderingContext2D::ShouldForceInactiveLayer(LayerManager *aManager)
{
    return !aManager->CanUseCanvasLayerForSize(gfxIntSize(mWidth, mHeight));
}

void
nsCanvasRenderingContext2D::MarkContextClean()
{
    if (mInvalidateCount > 0) {
        mPredictManyRedrawCalls = mInvalidateCount > kCanvasMaxInvalidateCount;
    }
    mIsEntireFrameInvalid = false;
    mInvalidateCount = 0;
}

