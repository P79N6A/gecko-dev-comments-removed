







































#include "mozilla/layers/ShadowLayersParent.h"

#include "BasicLayers.h"
#include "LayerManagerOGL.h"
#include "RenderFrameParent.h"

#include "gfx3DMatrix.h"
#include "nsFrameLoader.h"
#include "nsViewportFrame.h"
#include "nsSubDocumentFrame.h"
#include "nsIObserver.h"

typedef nsContentView::ViewConfig ViewConfig;
using namespace mozilla::layers;

namespace mozilla {
namespace layout {

typedef FrameMetrics::ViewID ViewID;
typedef RenderFrameParent::ViewMap ViewMap;

nsRefPtr<ImageContainer> sCheckerboard = nsnull;

class CheckerBoardPatternDeleter : public nsIObserver
{
public:
  NS_DECL_NSIOBSERVER
  NS_DECL_ISUPPORTS
};

NS_IMPL_ISUPPORTS1(CheckerBoardPatternDeleter, nsIObserver)

NS_IMETHODIMP
CheckerBoardPatternDeleter::Observe(nsISupports* aSubject,
                                    const char* aTopic,
                                    const PRUnichar* aData)
{
  if (!strcmp(aTopic, "xpcom-shutdown")) {
    sCheckerboard = nsnull;
  }
  return NS_OK;
}


struct ViewTransform {
  ViewTransform(nsIntPoint aTranslation = nsIntPoint(0, 0), float aXScale = 1, float aYScale = 1)
    : mTranslation(aTranslation)
    , mXScale(aXScale)
    , mYScale(aYScale)
  {}

  operator gfx3DMatrix() const
  {
    return
      gfx3DMatrix::Scale(mXScale, mYScale, 1) *
      gfx3DMatrix::Translation(mTranslation.x, mTranslation.y, 0);
  }

  nsIntPoint mTranslation;
  float mXScale;
  float mYScale;
};







static double GetXScale(const gfx3DMatrix& aTransform)
{
  return aTransform._11;
}
 
static double GetYScale(const gfx3DMatrix& aTransform)
{
  return aTransform._22;
}

static void Scale(gfx3DMatrix& aTransform, double aXScale, double aYScale)
{
  aTransform._11 *= aXScale;
  aTransform._22 *= aYScale;
}

static void ReverseTranslate(gfx3DMatrix& aTransform, ViewTransform& aViewTransform)
{
  aTransform._41 -= aViewTransform.mTranslation.x / aViewTransform.mXScale;
  aTransform._42 -= aViewTransform.mTranslation.y / aViewTransform.mYScale;
}


static void ApplyTransform(nsRect& aRect,
                           gfx3DMatrix& aTransform,
                           nscoord auPerDevPixel)
{
  aRect.x = aRect.x * aTransform._11 + aTransform._41 * auPerDevPixel;
  aRect.y = aRect.y * aTransform._22 + aTransform._42 * auPerDevPixel;
  aRect.width = aRect.width * aTransform._11;
  aRect.height = aRect.height * aTransform._22;
}
 
static void
AssertInTopLevelChromeDoc(ContainerLayer* aContainer,
                          nsIFrame* aContainedFrame)
{
  NS_ASSERTION(
    (aContainer->Manager()->GetBackendType() != LayerManager::LAYERS_BASIC) ||
    (aContainedFrame->GetNearestWidget() ==
     static_cast<BasicLayerManager*>(aContainer->Manager())->GetRetainerWidget()),
    "Expected frame to be in top-level chrome document");
}


static nsContentView*
FindViewForId(const ViewMap& aMap, ViewID aId)
{
  ViewMap::const_iterator iter = aMap.find(aId);
  return iter != aMap.end() ? iter->second : NULL;
}

static const FrameMetrics*
GetFrameMetrics(Layer* aLayer)
{
  ContainerLayer* container = aLayer->AsContainerLayer();
  return container ? &container->GetFrameMetrics() : NULL;
}

static nsIntPoint
GetRootFrameOffset(nsIFrame* aContainerFrame, nsDisplayListBuilder* aBuilder)
{
  nscoord auPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();

  
  nsPoint frameOffset =
    (aBuilder->ToReferenceFrame(aContainerFrame->GetParent()) +
     aContainerFrame->GetContentRect().TopLeft());

  return frameOffset.ToNearestPixels(auPerDevPixel);
}






static ViewTransform
ComputeShadowTreeTransform(nsIFrame* aContainerFrame,
                           nsFrameLoader* aRootFrameLoader,
                           const FrameMetrics* aMetrics,
                           const ViewConfig& aConfig,
                           float aInverseScaleX,
                           float aInverseScaleY)
{
  
  
  
  
  
  
  
  
  
  nscoord auPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntPoint scrollOffset =
    aConfig.mScrollOffset.ToNearestPixels(auPerDevPixel);
  
  nsIntPoint metricsScrollOffset = aMetrics->mViewportScrollOffset;

  if (aRootFrameLoader->AsyncScrollEnabled() && !aMetrics->mDisplayPort.IsEmpty()) {
    
    
    
    
    nsIntPoint scrollCompensation(
      scrollOffset.x * aInverseScaleX - metricsScrollOffset.x * aConfig.mXScale,
      scrollOffset.y * aInverseScaleY - metricsScrollOffset.y * aConfig.mYScale);

    return ViewTransform(-scrollCompensation, aConfig.mXScale, aConfig.mYScale);
  } else {
    return ViewTransform(nsIntPoint(0, 0), 1, 1);
  }
}


static void
BuildListForLayer(Layer* aLayer,
                  nsFrameLoader* aRootFrameLoader,
                  const gfx3DMatrix& aTransform,
                  nsDisplayListBuilder* aBuilder,
                  nsDisplayList& aShadowTree,
                  nsIFrame* aSubdocFrame)
{
  const FrameMetrics* metrics = GetFrameMetrics(aLayer);

  gfx3DMatrix transform;

  if (metrics && metrics->IsScrollable()) {
    const ViewID scrollId = metrics->mScrollId;

    
    
    
    

    
    nsContentView* view =
      aRootFrameLoader->GetCurrentRemoteFrame()->GetContentView(scrollId);
    
    
    gfx3DMatrix applyTransform = ComputeShadowTreeTransform(
      aSubdocFrame, aRootFrameLoader, metrics, view->GetViewConfig(),
      1 / GetXScale(aTransform), 1 / GetYScale(aTransform));
    transform = applyTransform * aLayer->GetTransform() * aTransform;

    
    
    gfx3DMatrix tmpTransform = aTransform;
    Scale(tmpTransform, GetXScale(applyTransform), GetYScale(applyTransform));

    
    nsRect bounds;
    {
      nscoord auPerDevPixel = aSubdocFrame->PresContext()->AppUnitsPerDevPixel();
      bounds = metrics->mViewport.ToAppUnits(auPerDevPixel);
      ApplyTransform(bounds, tmpTransform, auPerDevPixel);

    }

    aShadowTree.AppendToTop(
      new (aBuilder) nsDisplayRemoteShadow(aBuilder, aSubdocFrame, bounds, scrollId));

  } else {
    transform = aLayer->GetTransform() * aTransform;
  }

  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    BuildListForLayer(child, aRootFrameLoader, transform,
                      aBuilder, aShadowTree, aSubdocFrame);
  }
}


static void
TransformShadowTree(nsDisplayListBuilder* aBuilder, nsFrameLoader* aFrameLoader,
                    nsIFrame* aFrame, Layer* aLayer,
                    const ViewTransform& aTransform)
{
  ShadowLayer* shadow = aLayer->AsShadowLayer();
  shadow->SetShadowClipRect(aLayer->GetClipRect());
  shadow->SetShadowVisibleRegion(aLayer->GetVisibleRegion());

  const FrameMetrics* metrics = GetFrameMetrics(aLayer);

  gfx3DMatrix shadowTransform;
  ViewTransform layerTransform = aTransform;

  if (metrics && metrics->IsScrollable()) {
    const ViewID scrollId = metrics->mScrollId;
    const nsContentView* view =
      aFrameLoader->GetCurrentRemoteFrame()->GetContentView(scrollId);
    NS_ABORT_IF_FALSE(view, "Array of views should be consistent with layer tree");
    const gfx3DMatrix& currentTransform = aLayer->GetTransform();

    ViewTransform viewTransform = ComputeShadowTreeTransform(
      aFrame, aFrameLoader, metrics, view->GetViewConfig(),
      1 / (GetXScale(currentTransform)*layerTransform.mXScale),
      1 / (GetYScale(currentTransform)*layerTransform.mYScale)
    );

    
    shadowTransform = gfx3DMatrix(viewTransform) * currentTransform;

    if (metrics->IsRootScrollable()) {
      layerTransform.mTranslation = viewTransform.mTranslation;
      
      nsIntPoint rootFrameOffset = GetRootFrameOffset(aFrame, aBuilder);
      shadowTransform = shadowTransform *
          gfx3DMatrix::Translation(float(rootFrameOffset.x), float(rootFrameOffset.y), 0.0);
    }
  } else {
    shadowTransform = aLayer->GetTransform();
  }

  if (aLayer->GetIsFixedPosition() &&
      !aLayer->GetParent()->GetIsFixedPosition()) {
    ReverseTranslate(shadowTransform, layerTransform);
    const nsIntRect* clipRect = shadow->GetShadowClipRect();
    if (clipRect) {
      nsIntRect transformedClipRect(*clipRect);
      transformedClipRect.MoveBy(shadowTransform._41, shadowTransform._42);
      shadow->SetShadowClipRect(&transformedClipRect);
    }
  }

  shadow->SetShadowTransform(shadowTransform);
  layerTransform.mXScale *= GetXScale(shadowTransform);
  layerTransform.mYScale *= GetYScale(shadowTransform);

  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    TransformShadowTree(aBuilder, aFrameLoader, aFrame, child, layerTransform);
  }
}

static void
ClearContainer(ContainerLayer* aContainer)
{
  while (Layer* layer = aContainer->GetFirstChild()) {
    aContainer->RemoveChild(layer);
  }
}





static PRBool
IsTempLayerManager(LayerManager* aManager)
{
  return (LayerManager::LAYERS_BASIC == aManager->GetBackendType() &&
          !static_cast<BasicLayerManager*>(aManager)->IsRetained());
}







static void
BuildViewMap(ViewMap& oldContentViews, ViewMap& newContentViews,
             nsFrameLoader* aFrameLoader, Layer* aLayer,
             float aXScale = 1, float aYScale = 1)
{
  ContainerLayer* container = aLayer->AsContainerLayer();
  if (!container)
    return;
  const FrameMetrics metrics = container->GetFrameMetrics();
  const ViewID scrollId = metrics.mScrollId;

  if (metrics.IsScrollable()) {
    nscoord auPerDevPixel = aFrameLoader->GetPrimaryFrameOfOwningContent()
                                        ->PresContext()->AppUnitsPerDevPixel();
    nsContentView* view = FindViewForId(oldContentViews, scrollId);
    if (view) {
      
      
      ViewConfig config = view->GetViewConfig();
      aXScale *= config.mXScale;
      aYScale *= config.mYScale;
      view->mOwnerContent = aFrameLoader->GetOwnerContent();
    } else {
      
      
      
      ViewConfig config;
      config.mScrollOffset = nsPoint(
        NSIntPixelsToAppUnits(metrics.mViewportScrollOffset.x, auPerDevPixel) * aXScale,
        NSIntPixelsToAppUnits(metrics.mViewportScrollOffset.y, auPerDevPixel) * aYScale);
      view = new nsContentView(aFrameLoader->GetOwnerContent(), scrollId, config);
    }

    view->mViewportSize = nsSize(
      NSIntPixelsToAppUnits(metrics.mViewport.width, auPerDevPixel) * aXScale,
      NSIntPixelsToAppUnits(metrics.mViewport.height, auPerDevPixel) * aYScale);
    view->mContentSize = nsSize(
      NSIntPixelsToAppUnits(metrics.mContentSize.width, auPerDevPixel) * aXScale,
      NSIntPixelsToAppUnits(metrics.mContentSize.height, auPerDevPixel) * aYScale);

    newContentViews[scrollId] = view;
  }

  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    const gfx3DMatrix transform = aLayer->GetTransform();
    aXScale *= GetXScale(transform);
    aYScale *= GetYScale(transform);
    BuildViewMap(oldContentViews, newContentViews, aFrameLoader, child,
                 aXScale, aYScale);
  }
}

#define BOARDSIZE 32
#define CHECKERSIZE 16
already_AddRefed<gfxASurface>
GetBackgroundImage()
{
  static unsigned int data[BOARDSIZE * BOARDSIZE];
  static bool initialized = false;
  if (!initialized) {
    initialized = true;
    for (unsigned int y = 0; y < BOARDSIZE; y++) {
      for (unsigned int x = 0; x < BOARDSIZE; x++) {
        bool col_odd = (x / CHECKERSIZE) & 1;
        bool row_odd = (y / CHECKERSIZE) & 1;
        if (col_odd ^ row_odd) { 
          data[y * BOARDSIZE + x] = 0xFFFFFFFF;
        }
        else {
          data[y * BOARDSIZE + x] = 0xFFDDDDDD;
        }
      }
    }
  }

  nsRefPtr<gfxASurface> s =
    new gfxImageSurface((unsigned char*) data,
                        gfxIntSize(BOARDSIZE, BOARDSIZE),
                        BOARDSIZE * sizeof(unsigned int),
                        gfxASurface::ImageFormatARGB32);
  return s.forget();
}

static void
BuildBackgroundPatternFor(ContainerLayer* aContainer,
                          ContainerLayer* aShadowRoot,
                          const FrameMetrics& aMetrics,
                          const ViewConfig& aConfig,
                          LayerManager* aManager,
                          nsIFrame* aFrame,
                          nsDisplayListBuilder* aBuilder)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  ShadowLayer* shadowRoot = aShadowRoot->AsShadowLayer();
  gfxMatrix t;
  if (!shadowRoot->GetShadowTransform().Is2D(&t)) {
    return;
  }

  
  
  nsIntRect contentBounds = shadowRoot->GetShadowVisibleRegion().GetBounds();
  gfxRect contentVis(contentBounds.x, contentBounds.y,
                     contentBounds.width, contentBounds.height);
  gfxRect localContentVis(t.Transform(contentVis));
  
  localContentVis.RoundIn();
  nsIntRect localIntContentVis(localContentVis.X(), localContentVis.Y(),
                               localContentVis.Width(), localContentVis.Height());

  
  nscoord auPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntRect frameRect = aFrame->GetRect().ToOutsidePixels(auPerDevPixel);

  
  
  if (localIntContentVis.Contains(frameRect)) {
    return;
  }

  nsRefPtr<gfxASurface> bgImage = GetBackgroundImage();
  gfxIntSize bgImageSize = bgImage->GetSize();

  
  if (!sCheckerboard) {
    sCheckerboard = aManager->CreateImageContainer().get();
    const Image::Format fmts[] = { Image::CAIRO_SURFACE };
    nsRefPtr<Image> img = sCheckerboard->CreateImage(fmts, 1);
    CairoImage::Data data = { bgImage.get(), bgImageSize };
    static_cast<CairoImage*>(img.get())->SetData(data);
    sCheckerboard->SetCurrentImage(img);
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (!observerService) {
      return;
    }
    nsresult rv = observerService->AddObserver(new CheckerBoardPatternDeleter, "xpcom-shutdown", PR_FALSE);
    if (NS_FAILED(rv)) {
      return;
    }
  }

  nsRefPtr<ImageLayer> layer = aManager->CreateImageLayer();
  layer->SetContainer(sCheckerboard);

  
  nsIntRect tileSource(0, 0, bgImageSize.width, bgImageSize.height);
  layer->SetTileSourceRect(&tileSource);

  
  
  
  nsIntPoint translation = frameRect.TopLeft();
  nsIntPoint panNudge = aConfig.mScrollOffset.ToNearestPixels(auPerDevPixel);
  
  
  
  panNudge.x = (panNudge.x % bgImageSize.width);
  if (panNudge.x < 0) panNudge.x += bgImageSize.width;
  panNudge.y = (panNudge.y % bgImageSize.height);
  if (panNudge.y < 0) panNudge.y += bgImageSize.height;

  translation -= panNudge;
  layer->SetTransform(gfx3DMatrix::Translation(translation.x, translation.y, 0));

  
  
  nsIntRegion bgRgn(frameRect);
  bgRgn.Sub(bgRgn, localIntContentVis);
  bgRgn.MoveBy(-translation);
  layer->SetVisibleRegion(bgRgn);
      
  aContainer->InsertAfter(layer, nsnull);
}

RenderFrameParent::RenderFrameParent(nsFrameLoader* aFrameLoader)
  : mFrameLoader(aFrameLoader)
{
  NS_ABORT_IF_FALSE(aFrameLoader, "Need a frameloader here");
  mContentViews[FrameMetrics::ROOT_SCROLL_ID] =
    new nsContentView(aFrameLoader->GetOwnerContent(),
                      FrameMetrics::ROOT_SCROLL_ID);
}

RenderFrameParent::~RenderFrameParent()
{}

void
RenderFrameParent::Destroy()
{
  size_t numChildren = ManagedPLayersParent().Length();
  NS_ABORT_IF_FALSE(0 == numChildren || 1 == numChildren,
                    "render frame must only have 0 or 1 layer manager");

  if (numChildren) {
    ShadowLayersParent* layers =
      static_cast<ShadowLayersParent*>(ManagedPLayersParent()[0]);
    layers->Destroy();
  }
}

nsContentView*
RenderFrameParent::GetContentView(ViewID aId)
{
  return FindViewForId(mContentViews, aId);
}

void
RenderFrameParent::ShadowLayersUpdated()
{
  mFrameLoader->SetCurrentRemoteFrame(this);

  
  
  
  BuildViewMap();

  nsIFrame* docFrame = mFrameLoader->GetPrimaryFrameOfOwningContent();
  if (!docFrame) {
    
    
    
    
    
    return;
  }

  
  
  
  
  
  
  
  
  
  nsRect rect = nsRect(nsPoint(0, 0), docFrame->GetRect().Size());
  docFrame->InvalidateWithFlags(rect, nsIFrame::INVALIDATE_NO_THEBES_LAYERS);
}

already_AddRefed<Layer>
RenderFrameParent::BuildLayer(nsDisplayListBuilder* aBuilder,
                              nsIFrame* aFrame,
                              LayerManager* aManager,
                              const nsIntRect& aVisibleRect)
{
  NS_ABORT_IF_FALSE(aFrame,
                    "makes no sense to have a shadow tree without a frame");
  NS_ABORT_IF_FALSE(!mContainer ||
                    IsTempLayerManager(aManager) ||
                    mContainer->Manager() == aManager,
                    "retaining manager changed out from under us ... HELP!");

  if (mContainer && mContainer->Manager() != aManager) {
    
    
    
    
    
    
    return nsnull;
  }

  if (mContainer) {
    ClearContainer(mContainer);
  }

  ContainerLayer* shadowRoot = GetRootLayer();
  if (!shadowRoot) {
    mContainer = nsnull;
    return nsnull;
  }

  NS_ABORT_IF_FALSE(!shadowRoot || shadowRoot->Manager() == aManager,
                    "retaining manager changed out from under us ... HELP!");

  
  if (!mContainer) {
    mContainer = aManager->CreateContainerLayer();
  }
  NS_ABORT_IF_FALSE(!mContainer->GetFirstChild(),
                    "container of shadow tree shouldn't have a 'root' here");

  mContainer->InsertAfter(shadowRoot, nsnull);

  AssertInTopLevelChromeDoc(mContainer, aFrame);
  ViewTransform transform;
  TransformShadowTree(aBuilder, mFrameLoader, aFrame, shadowRoot, transform);
  mContainer->SetClipRect(nsnull);

  if (mFrameLoader->AsyncScrollEnabled()) {
    const nsContentView* view = GetContentView(FrameMetrics::ROOT_SCROLL_ID);
    BuildBackgroundPatternFor(mContainer,
                              shadowRoot,
                              shadowRoot->GetFrameMetrics(),
                              view->GetViewConfig(),
                              aManager, aFrame, aBuilder);
  }
  mContainer->SetVisibleRegion(aVisibleRect);

  return nsRefPtr<Layer>(mContainer).forget();
}

void
RenderFrameParent::OwnerContentChanged(nsIContent* aContent)
{
  NS_ABORT_IF_FALSE(mFrameLoader->GetOwnerContent() == aContent,
                    "Don't build new map if owner is same!");
  BuildViewMap();
}

void
RenderFrameParent::ActorDestroy(ActorDestroyReason why)
{
  if (mFrameLoader->GetCurrentRemoteFrame() == this) {
    
    
    
    
    
    
    mFrameLoader->SetCurrentRemoteFrame(nsnull);
  }
  mFrameLoader = nsnull;
}

PLayersParent*
RenderFrameParent::AllocPLayers()
{
  LayerManager* lm = GetLayerManager();
  switch (lm->GetBackendType()) {
  case LayerManager::LAYERS_BASIC: {
    BasicShadowLayerManager* bslm = static_cast<BasicShadowLayerManager*>(lm);
    return new ShadowLayersParent(bslm);
  }
  case LayerManager::LAYERS_OPENGL: {
    LayerManagerOGL* lmo = static_cast<LayerManagerOGL*>(lm);
    return new ShadowLayersParent(lmo);
  }
  default: {
    NS_WARNING("shadow layers no sprechen D3D backend yet");
    return nsnull;
  }
  }
}

bool
RenderFrameParent::DeallocPLayers(PLayersParent* aLayers)
{
  delete aLayers;
  return true;
}

void
RenderFrameParent::BuildViewMap()
{
  ViewMap newContentViews;
  
  if (GetRootLayer() && mFrameLoader->GetPrimaryFrameOfOwningContent()) {
    
    
    
    
    
    

    for (ViewMap::const_iterator iter = mContentViews.begin();
         iter != mContentViews.end();
         ++iter) {
      iter->second->mOwnerContent = NULL;
    }

    mozilla::layout::BuildViewMap(mContentViews, newContentViews, mFrameLoader, GetRootLayer());
  }

  
  
  
  
  if (newContentViews.empty()) {
    newContentViews[FrameMetrics::ROOT_SCROLL_ID] =
      FindViewForId(mContentViews, FrameMetrics::ROOT_SCROLL_ID);
  }
  
  mContentViews = newContentViews;
}

LayerManager*
RenderFrameParent::GetLayerManager() const
{
  nsIDocument* doc = mFrameLoader->GetOwnerDoc();
  return doc->GetShell()->GetLayerManager();
}

ShadowLayersParent*
RenderFrameParent::GetShadowLayers() const
{
  const nsTArray<PLayersParent*>& shadowParents = ManagedPLayersParent();
  NS_ABORT_IF_FALSE(shadowParents.Length() <= 1,
                    "can only support at most 1 ShadowLayersParent");
  return (shadowParents.Length() == 1) ?
    static_cast<ShadowLayersParent*>(shadowParents[0]) : nsnull;
}

ContainerLayer*
RenderFrameParent::GetRootLayer() const
{
  ShadowLayersParent* shadowLayers = GetShadowLayers();
  return shadowLayers ? shadowLayers->GetRoot() : nsnull;
}

NS_IMETHODIMP
RenderFrameParent::BuildDisplayList(nsDisplayListBuilder* aBuilder,
                                    nsSubDocumentFrame* aFrame,
                                    const nsRect& aDirtyRect,
                                    const nsDisplayListSet& aLists)
{
  
  
  nsDisplayList shadowTree;
  if (aBuilder->IsForEventDelivery()) {
    nsRect bounds = aFrame->EnsureInnerView()->GetBounds();
    ViewTransform offset =
      ViewTransform(GetRootFrameOffset(aFrame, aBuilder), 1, 1);
    BuildListForLayer(GetRootLayer(), mFrameLoader, offset,
                      aBuilder, shadowTree, aFrame);
  } else {
    shadowTree.AppendToTop(
      new (aBuilder) nsDisplayRemote(aBuilder, aFrame, this));
  }

  
  nsPoint offset = aFrame->GetOffsetToCrossDoc(aBuilder->ReferenceFrame());
  nsRect bounds = aFrame->EnsureInnerView()->GetBounds() + offset;

  return aLists.Content()->AppendNewToTop(
    new (aBuilder) nsDisplayClip(aBuilder, aFrame, &shadowTree,
                                 bounds));
}

}  
}  

already_AddRefed<Layer>
nsDisplayRemote::BuildLayer(nsDisplayListBuilder* aBuilder,
                            LayerManager* aManager,
                            const ContainerParameters& aContainerParameters)
{
  PRInt32 appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntRect visibleRect = GetVisibleRect().ToNearestPixels(appUnitsPerDevPixel);
  nsRefPtr<Layer> layer = mRemoteFrame->BuildLayer(aBuilder, mFrame, aManager, visibleRect);
  return layer.forget();
}


void
nsDisplayRemoteShadow::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                         HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  
  
  
  
  
  if (aState->mShadows) {
    aState->mShadows->AppendElement(mId);
  }
}
