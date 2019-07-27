





#include "nsSVGPatternFrame.h"


#include "gfx2DGlue.h"
#include "gfxContext.h"
#include "gfxMatrix.h"
#include "gfxPattern.h"
#include "gfxPlatform.h"
#include "mozilla/gfx/2D.h"
#include "nsContentUtils.h"
#include "nsGkAtoms.h"
#include "nsISVGChildFrame.h"
#include "nsStyleContext.h"
#include "nsSVGEffects.h"
#include "nsSVGPathGeometryFrame.h"
#include "mozilla/dom/SVGPatternElement.h"
#include "nsSVGUtils.h"
#include "nsSVGAnimatedTransformList.h"
#include "SVGContentUtils.h"
#include "gfxColor.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;




class MOZ_STACK_CLASS nsSVGPatternFrame::AutoPatternReferencer
{
public:
  explicit AutoPatternReferencer(nsSVGPatternFrame *aFrame
                                 MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mFrame(aFrame)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    
    
    MOZ_ASSERT(!mFrame->mLoopFlag, "Undetected reference loop!");
    mFrame->mLoopFlag = true;
  }
  ~AutoPatternReferencer() {
    mFrame->mLoopFlag = false;
  }
private:
  nsSVGPatternFrame *mFrame;
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};




nsSVGPatternFrame::nsSVGPatternFrame(nsStyleContext* aContext) :
  nsSVGPatternFrameBase(aContext),
  mLoopFlag(false),
  mNoHRefURI(false)
{
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGPatternFrame)




nsresult
nsSVGPatternFrame::AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::patternUnits ||
       aAttribute == nsGkAtoms::patternContentUnits ||
       aAttribute == nsGkAtoms::patternTransform ||
       aAttribute == nsGkAtoms::x ||
       aAttribute == nsGkAtoms::y ||
       aAttribute == nsGkAtoms::width ||
       aAttribute == nsGkAtoms::height ||
       aAttribute == nsGkAtoms::preserveAspectRatio ||
       aAttribute == nsGkAtoms::viewBox)) {
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  }

  if (aNameSpaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href) {
    
    Properties().Delete(nsSVGEffects::HrefProperty());
    mNoHRefURI = false;
    
    nsSVGEffects::InvalidateDirectRenderingObservers(this);
  }

  return nsSVGPatternFrameBase::AttributeChanged(aNameSpaceID,
                                                 aAttribute, aModType);
}

#ifdef DEBUG
void
nsSVGPatternFrame::Init(nsIContent*       aContent,
                        nsContainerFrame* aParent,
                        nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVGElement(nsGkAtoms::pattern), "Content is not an SVG pattern");

  nsSVGPatternFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom*
nsSVGPatternFrame::GetType() const
{
  return nsGkAtoms::svgPatternFrame;
}








gfxMatrix
nsSVGPatternFrame::GetCanvasTM()
{
  if (mCTM) {
    return *mCTM;
  }

  
  if (mSource) {
    
    return mSource->GetCanvasTM();
  }

  
  return gfxMatrix();
}






static float
MaxExpansion(const Matrix &aMatrix)
{
  
  
  
  double a = aMatrix._11;
  double b = aMatrix._12;
  double c = aMatrix._21;
  double d = aMatrix._22;
  double f = (a * a + b * b + c * c + d * d) / 2;
  double g = (a * a + b * b - c * c - d * d) / 2;
  double h = a * c + b * d;
  return sqrt(f + sqrt(g * g + h * h));
}





static bool
IncludeBBoxScale(const nsSVGViewBox& aViewBox,
                 uint32_t aPatternContentUnits, uint32_t aPatternUnits)
{
  return (!aViewBox.IsExplicitlySet() &&
          aPatternContentUnits == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) ||
         (aViewBox.IsExplicitlySet() &&
          aPatternUnits == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX);
}



static Matrix
GetPatternMatrix(uint16_t aPatternUnits,
                 const Matrix &patternTransform,
                 const gfxRect &bbox,
                 const gfxRect &callerBBox,
                 const Matrix &callerCTM)
{
  
  gfxFloat minx = bbox.X();
  gfxFloat miny = bbox.Y();

  if (aPatternUnits == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    minx += callerBBox.X();
    miny += callerBBox.Y();
  }

  float scale = 1.0f / MaxExpansion(callerCTM);
  Matrix patternMatrix = patternTransform;
  patternMatrix.PreScale(scale, scale);
  patternMatrix.PreTranslate(minx, miny);

  return patternMatrix;
}

static nsresult
GetTargetGeometry(gfxRect *aBBox,
                  const nsSVGViewBox &aViewBox,
                  uint16_t aPatternContentUnits,
                  uint16_t aPatternUnits,
                  nsIFrame *aTarget,
                  const Matrix &aContextMatrix,
                  const gfxRect *aOverrideBounds)
{
  *aBBox = aOverrideBounds ? *aOverrideBounds : nsSVGUtils::GetBBox(aTarget);

  
  if (IncludeBBoxScale(aViewBox, aPatternContentUnits, aPatternUnits) &&
      (aBBox->Width() <= 0 || aBBox->Height() <= 0)) {
    return NS_ERROR_FAILURE;
  }

  
  
  float scale = MaxExpansion(aContextMatrix);
  if (scale <= 0) {
    return NS_ERROR_FAILURE;
  }
  aBBox->Scale(scale);
  return NS_OK;
}

TemporaryRef<SourceSurface>
nsSVGPatternFrame::PaintPattern(const DrawTarget* aDrawTarget,
                                Matrix* patternMatrix,
                                const Matrix &aContextMatrix,
                                nsIFrame *aSource,
                                nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                float aGraphicOpacity,
                                const gfxRect *aOverrideBounds)
{
  











  nsSVGPatternFrame* patternWithChildren = GetPatternWithChildren();
  if (!patternWithChildren) {
    return nullptr; 
  }
  nsIFrame* firstKid = patternWithChildren->mFrames.FirstChild();

  const nsSVGViewBox& viewBox = GetViewBox();

  uint16_t patternContentUnits =
    GetEnumValue(SVGPatternElement::PATTERNCONTENTUNITS);
  uint16_t patternUnits =
    GetEnumValue(SVGPatternElement::PATTERNUNITS);

  


















  
  
  gfxRect callerBBox;
  if (NS_FAILED(GetTargetGeometry(&callerBBox,
                                  viewBox,
                                  patternContentUnits, patternUnits,
                                  aSource,
                                  aContextMatrix,
                                  aOverrideBounds))) {
    return nullptr;
  }

  
  
  gfxMatrix ctm = ConstructCTM(viewBox, patternContentUnits, patternUnits,
                               callerBBox, aContextMatrix, aSource);
  if (ctm.IsSingular()) {
    return nullptr;
  }

  if (patternWithChildren->mCTM) {
    *patternWithChildren->mCTM = ctm;
  } else {
    patternWithChildren->mCTM = new gfxMatrix(ctm);
  }

  
  
  
  gfxRect bbox = GetPatternRect(patternUnits, callerBBox, aContextMatrix, aSource);
  if (bbox.Width() <= 0.0 || bbox.Height() <= 0.0) {
    return nullptr;
  }

  
  Matrix patternTransform = ToMatrix(GetPatternTransform());

  
  if (aFillOrStroke == &nsStyleSVG::mStroke) {
    gfxMatrix userToOuterSVG;
    if (nsSVGUtils::GetNonScalingStrokeTransform(aSource, &userToOuterSVG)) {
      patternTransform *= ToMatrix(userToOuterSVG);
      if (patternTransform.IsSingular()) {
        NS_WARNING("Singular matrix painting non-scaling-stroke");
        return nullptr;
      }
    }
  }

  
  
  *patternMatrix = GetPatternMatrix(patternUnits, patternTransform,
                                    bbox, callerBBox, aContextMatrix);
  if (patternMatrix->IsSingular()) {
    return nullptr;
  }

  
  
  gfxRect transformedBBox = ThebesRect(patternTransform.TransformBounds(ToRect(bbox)));

  bool resultOverflows;
  IntSize surfaceSize =
    nsSVGUtils::ConvertToSurfaceSize(
      transformedBBox.Size(), &resultOverflows);

  
  if (surfaceSize.width <= 0 || surfaceSize.height <= 0) {
    return nullptr;
  }

  gfxFloat patternWidth = bbox.Width();
  gfxFloat patternHeight = bbox.Height();

  if (resultOverflows ||
      patternWidth != surfaceSize.width ||
      patternHeight != surfaceSize.height) {
    
    gfxMatrix tempTM =
      gfxMatrix(surfaceSize.width / patternWidth, 0.0,
                0.0, surfaceSize.height / patternHeight,
                0.0, 0.0);
    patternWithChildren->mCTM->PreMultiply(tempTM);

    
    patternMatrix->PreScale(patternWidth / surfaceSize.width,
                            patternHeight / surfaceSize.height);
  }

  RefPtr<DrawTarget> dt =
    aDrawTarget->CreateSimilarDrawTarget(surfaceSize, SurfaceFormat::B8G8R8A8);
  if (!dt) {
    return nullptr;
  }
  dt->ClearRect(Rect(0, 0, surfaceSize.width, surfaceSize.height));

  nsRefPtr<gfxContext> gfx = new gfxContext(dt);

  if (aGraphicOpacity != 1.0f) {
    gfx->Save();
    gfx->PushGroup(gfxContentType::COLOR_ALPHA);
  }

  
  
  

  if (aSource->IsFrameOfType(nsIFrame::eSVGGeometry)) {
    
    patternWithChildren->mSource = static_cast<nsSVGPathGeometryFrame*>(aSource);
  }

  
  
  if (!(patternWithChildren->GetStateBits() & NS_FRAME_DRAWING_AS_PAINTSERVER)) {
    patternWithChildren->AddStateBits(NS_FRAME_DRAWING_AS_PAINTSERVER);
    for (nsIFrame* kid = firstKid; kid;
         kid = kid->GetNextSibling()) {
      
      nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
      if (SVGFrame) {
        SVGFrame->NotifySVGChanged(nsISVGChildFrame::TRANSFORM_CHANGED);
      }
      gfxMatrix tm = *(patternWithChildren->mCTM);
      if (kid->GetContent()->IsSVGElement()) {
        tm = static_cast<nsSVGElement*>(kid->GetContent())->
              PrependLocalTransformsTo(tm, nsSVGElement::eUserSpaceToParent);
      }
      nsSVGUtils::PaintFrameWithEffects(kid, *gfx, tm);
    }
    patternWithChildren->RemoveStateBits(NS_FRAME_DRAWING_AS_PAINTSERVER);
  }

  patternWithChildren->mSource = nullptr;

  if (aGraphicOpacity != 1.0f) {
    gfx->PopGroupToSource();
    gfx->Paint(aGraphicOpacity);
    gfx->Restore();
  }

  
  return dt->Snapshot();
}





nsSVGPatternFrame*
nsSVGPatternFrame::GetPatternWithChildren()
{
  
  if (!mFrames.IsEmpty())
    return this;

  
  AutoPatternReferencer patternRef(this);

  nsSVGPatternFrame* next = GetReferencedPatternIfNotInUse();
  if (!next)
    return nullptr;

  return next->GetPatternWithChildren();
}

uint16_t
nsSVGPatternFrame::GetEnumValue(uint32_t aIndex, nsIContent *aDefault)
{
  nsSVGEnum& thisEnum =
    static_cast<SVGPatternElement *>(mContent)->mEnumAttributes[aIndex];

  if (thisEnum.IsExplicitlySet())
    return thisEnum.GetAnimValue();

  AutoPatternReferencer patternRef(this);

  nsSVGPatternFrame *next = GetReferencedPatternIfNotInUse();
  return next ? next->GetEnumValue(aIndex, aDefault) :
    static_cast<SVGPatternElement *>(aDefault)->
      mEnumAttributes[aIndex].GetAnimValue();
}

nsSVGAnimatedTransformList*
nsSVGPatternFrame::GetPatternTransformList(nsIContent* aDefault)
{
  nsSVGAnimatedTransformList *thisTransformList =
    static_cast<SVGPatternElement *>(mContent)->GetAnimatedTransformList();

  if (thisTransformList && thisTransformList->IsExplicitlySet())
    return thisTransformList;

  AutoPatternReferencer patternRef(this);

  nsSVGPatternFrame *next = GetReferencedPatternIfNotInUse();
  return next ? next->GetPatternTransformList(aDefault) :
    static_cast<SVGPatternElement *>(aDefault)->mPatternTransform.get();
}

gfxMatrix
nsSVGPatternFrame::GetPatternTransform()
{
  nsSVGAnimatedTransformList* animTransformList =
    GetPatternTransformList(mContent);
  if (!animTransformList)
    return gfxMatrix();

  return animTransformList->GetAnimValue().GetConsolidationMatrix();
}

const nsSVGViewBox &
nsSVGPatternFrame::GetViewBox(nsIContent* aDefault)
{
  const nsSVGViewBox &thisViewBox =
    static_cast<SVGPatternElement *>(mContent)->mViewBox;

  if (thisViewBox.IsExplicitlySet())
    return thisViewBox;

  AutoPatternReferencer patternRef(this);

  nsSVGPatternFrame *next = GetReferencedPatternIfNotInUse();
  return next ? next->GetViewBox(aDefault) :
    static_cast<SVGPatternElement *>(aDefault)->mViewBox;
}

const SVGAnimatedPreserveAspectRatio &
nsSVGPatternFrame::GetPreserveAspectRatio(nsIContent *aDefault)
{
  const SVGAnimatedPreserveAspectRatio &thisPar =
    static_cast<SVGPatternElement *>(mContent)->mPreserveAspectRatio;

  if (thisPar.IsExplicitlySet())
    return thisPar;

  AutoPatternReferencer patternRef(this);

  nsSVGPatternFrame *next = GetReferencedPatternIfNotInUse();
  return next ? next->GetPreserveAspectRatio(aDefault) :
    static_cast<SVGPatternElement *>(aDefault)->mPreserveAspectRatio;
}

const nsSVGLength2 *
nsSVGPatternFrame::GetLengthValue(uint32_t aIndex, nsIContent *aDefault)
{
  const nsSVGLength2 *thisLength =
    &static_cast<SVGPatternElement *>(mContent)->mLengthAttributes[aIndex];

  if (thisLength->IsExplicitlySet())
    return thisLength;

  AutoPatternReferencer patternRef(this);

  nsSVGPatternFrame *next = GetReferencedPatternIfNotInUse();
  return next ? next->GetLengthValue(aIndex, aDefault) :
    &static_cast<SVGPatternElement *>(aDefault)->mLengthAttributes[aIndex];
}


nsSVGPatternFrame *
nsSVGPatternFrame::GetReferencedPattern()
{
  if (mNoHRefURI)
    return nullptr;

  nsSVGPaintingProperty *property = static_cast<nsSVGPaintingProperty*>
    (Properties().Get(nsSVGEffects::HrefProperty()));

  if (!property) {
    
    SVGPatternElement *pattern = static_cast<SVGPatternElement *>(mContent);
    nsAutoString href;
    pattern->mStringAttributes[SVGPatternElement::HREF].GetAnimValue(href, pattern);
    if (href.IsEmpty()) {
      mNoHRefURI = true;
      return nullptr; 
    }

    
    nsCOMPtr<nsIURI> targetURI;
    nsCOMPtr<nsIURI> base = mContent->GetBaseURI();
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                              mContent->GetCurrentDoc(), base);

    property =
      nsSVGEffects::GetPaintingProperty(targetURI, this, nsSVGEffects::HrefProperty());
    if (!property)
      return nullptr;
  }

  nsIFrame *result = property->GetReferencedFrame();
  if (!result)
    return nullptr;

  nsIAtom* frameType = result->GetType();
  if (frameType != nsGkAtoms::svgPatternFrame)
    return nullptr;

  return static_cast<nsSVGPatternFrame*>(result);
}

nsSVGPatternFrame *
nsSVGPatternFrame::GetReferencedPatternIfNotInUse()
{
  nsSVGPatternFrame *referenced = GetReferencedPattern();
  if (!referenced)
    return nullptr;

  if (referenced->mLoopFlag) {
    
    NS_WARNING("pattern reference loop detected while inheriting attribute!");
    return nullptr;
  }

  return referenced;
}

gfxRect
nsSVGPatternFrame::GetPatternRect(uint16_t aPatternUnits,
                                  const gfxRect &aTargetBBox,
                                  const Matrix &aTargetCTM,
                                  nsIFrame *aTarget)
{
  
  float x,y,width,height;

  
  const nsSVGLength2 *tmpX, *tmpY, *tmpHeight, *tmpWidth;
  tmpX = GetLengthValue(SVGPatternElement::ATTR_X);
  tmpY = GetLengthValue(SVGPatternElement::ATTR_Y);
  tmpHeight = GetLengthValue(SVGPatternElement::ATTR_HEIGHT);
  tmpWidth = GetLengthValue(SVGPatternElement::ATTR_WIDTH);

  if (aPatternUnits == SVG_UNIT_TYPE_OBJECTBOUNDINGBOX) {
    x = nsSVGUtils::ObjectSpace(aTargetBBox, tmpX);
    y = nsSVGUtils::ObjectSpace(aTargetBBox, tmpY);
    width = nsSVGUtils::ObjectSpace(aTargetBBox, tmpWidth);
    height = nsSVGUtils::ObjectSpace(aTargetBBox, tmpHeight);
  } else {
    float scale = MaxExpansion(aTargetCTM);
    x = nsSVGUtils::UserSpace(aTarget, tmpX) * scale;
    y = nsSVGUtils::UserSpace(aTarget, tmpY) * scale;
    width = nsSVGUtils::UserSpace(aTarget, tmpWidth) * scale;
    height = nsSVGUtils::UserSpace(aTarget, tmpHeight) * scale;
  }

  return gfxRect(x, y, width, height);
}

gfxMatrix
nsSVGPatternFrame::ConstructCTM(const nsSVGViewBox& aViewBox,
                                uint16_t aPatternContentUnits,
                                uint16_t aPatternUnits,
                                const gfxRect &callerBBox,
                                const Matrix &callerCTM,
                                nsIFrame *aTarget)
{
  SVGSVGElement *ctx = nullptr;
  nsIContent* targetContent = aTarget->GetContent();
  gfxFloat scaleX, scaleY;

  
  if (IncludeBBoxScale(aViewBox, aPatternContentUnits, aPatternUnits)) {
    scaleX = callerBBox.Width();
    scaleY = callerBBox.Height();
  } else {
    if (targetContent->IsSVGElement()) {
      ctx = static_cast<nsSVGElement*>(targetContent)->GetCtx();
    }
    scaleX = scaleY = MaxExpansion(callerCTM);
  }

  if (!aViewBox.IsExplicitlySet()) {
    return gfxMatrix(scaleX, 0.0, 0.0, scaleY, 0.0, 0.0);
  }
  const nsSVGViewBoxRect viewBoxRect = aViewBox.GetAnimValue();

  if (viewBoxRect.height <= 0.0f || viewBoxRect.width <= 0.0f) {
    return gfxMatrix(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); 
  }

  float viewportWidth, viewportHeight;
  if (targetContent->IsSVGElement()) {
    
    
    
    viewportWidth =
      GetLengthValue(SVGPatternElement::ATTR_WIDTH)->GetAnimValue(ctx);
    viewportHeight =
      GetLengthValue(SVGPatternElement::ATTR_HEIGHT)->GetAnimValue(ctx);
  } else {
    
    viewportWidth =
      GetLengthValue(SVGPatternElement::ATTR_WIDTH)->GetAnimValue(aTarget);
    viewportHeight =
      GetLengthValue(SVGPatternElement::ATTR_HEIGHT)->GetAnimValue(aTarget);
  }

  if (viewportWidth <= 0.0f || viewportHeight <= 0.0f) {
    return gfxMatrix(0.0, 0.0, 0.0, 0.0, 0.0, 0.0); 
  }

  Matrix tm = SVGContentUtils::GetViewBoxTransform(
    viewportWidth * scaleX, viewportHeight * scaleY,
    viewBoxRect.x, viewBoxRect.y,
    viewBoxRect.width, viewBoxRect.height,
    GetPreserveAspectRatio());

  return ThebesMatrix(tm);
}




already_AddRefed<gfxPattern>
nsSVGPatternFrame::GetPaintServerPattern(nsIFrame *aSource,
                                         const DrawTarget* aDrawTarget,
                                         const gfxMatrix& aContextMatrix,
                                         nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                         float aGraphicOpacity,
                                         const gfxRect *aOverrideBounds)
{
  if (aGraphicOpacity == 0.0f) {
    nsRefPtr<gfxPattern> pattern = new gfxPattern(gfxRGBA(0, 0, 0, 0));
    return pattern.forget();
  }

  
  Matrix pMatrix;
  RefPtr<SourceSurface> surface =
    PaintPattern(aDrawTarget, &pMatrix, ToMatrix(aContextMatrix), aSource,
                 aFillOrStroke, aGraphicOpacity, aOverrideBounds);

  if (!surface) {
    return nullptr;
  }

  nsRefPtr<gfxPattern> pattern = new gfxPattern(surface, pMatrix);

  if (!pattern || pattern->CairoStatus())
    return nullptr;

  pattern->SetExtend(gfxPattern::EXTEND_REPEAT);
  return pattern.forget();
}





nsIFrame* NS_NewSVGPatternFrame(nsIPresShell*   aPresShell,
                                nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGPatternFrame(aContext);
}

