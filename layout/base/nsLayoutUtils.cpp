





#include "nsLayoutUtils.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/gfx/PathHelpers.h"
#include "mozilla/Likely.h"
#include "mozilla/Maybe.h"
#include "mozilla/MemoryReporting.h"
#include "nsCharTraits.h"
#include "nsFontMetrics.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsFrameList.h"
#include "nsGkAtoms.h"
#include "nsHtml5Atoms.h"
#include "nsIAtom.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSColorUtils.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsPlaceholderFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIDOMEvent.h"
#include "nsDisplayList.h"
#include "nsRegion.h"
#include "nsFrameManager.h"
#include "nsBlockFrame.h"
#include "nsBidiPresUtils.h"
#include "imgIContainer.h"
#include "ImageOps.h"
#include "ImageRegion.h"
#include "gfxRect.h"
#include "gfxContext.h"
#include "nsRenderingContext.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsCSSRendering.h"
#include "nsThemeConstants.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIWidget.h"
#include "gfxMatrix.h"
#include "gfxPrefs.h"
#include "gfxTypes.h"
#include "nsTArray.h"
#include "mozilla/dom/HTMLCanvasElement.h"
#include "nsICanvasRenderingContextInternal.h"
#include "gfxPlatform.h"
#include <algorithm>
#include <limits>
#include "mozilla/dom/HTMLVideoElement.h"
#include "mozilla/dom/HTMLImageElement.h"
#include "mozilla/dom/DOMRect.h"
#include "mozilla/dom/KeyframeEffect.h"
#include "imgIRequest.h"
#include "nsIImageLoadingContent.h"
#include "nsCOMPtr.h"
#include "nsCSSProps.h"
#include "nsListControlFrame.h"
#include "mozilla/dom/Element.h"
#include "nsCanvasFrame.h"
#include "gfxDrawable.h"
#include "gfxUtils.h"
#include "nsDataHashtable.h"
#include "nsTextFrame.h"
#include "nsFontFaceList.h"
#include "nsFontInflationData.h"
#include "nsSVGUtils.h"
#include "SVGImageContext.h"
#include "SVGTextFrame.h"
#include "nsStyleStructInlines.h"
#include "nsStyleTransformMatrix.h"
#include "nsIFrameInlines.h"
#include "ImageContainer.h"
#include "nsComputedDOMStyle.h"
#include "ActiveLayerTracker.h"
#include "mozilla/gfx/2D.h"
#include "gfx2DGlue.h"
#include "mozilla/LookAndFeel.h"
#include "UnitTransforms.h"
#include "TiledLayerBuffer.h" 
#include "ClientLayerManager.h"
#include "nsRefreshDriver.h"
#include "nsIContentViewer.h"
#include "LayersLogging.h"
#include "mozilla/Preferences.h"
#include "nsFrameSelection.h"
#include "FrameLayerBuilder.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/Telemetry.h"
#include "mozilla/EventDispatcher.h"

#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#endif

#include "GeckoProfiler.h"
#include "nsAnimationManager.h"
#include "nsTransitionManager.h"
#include "RestyleManager.h"
#include "mozilla/EventDispatcher.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::image;
using namespace mozilla::layers;
using namespace mozilla::layout;
using namespace mozilla::gfx;

#define GRID_ENABLED_PREF_NAME "layout.css.grid.enabled"
#define RUBY_ENABLED_PREF_NAME "layout.css.ruby.enabled"
#define STICKY_ENABLED_PREF_NAME "layout.css.sticky.enabled"
#define DISPLAY_CONTENTS_ENABLED_PREF_NAME "layout.css.display-contents.enabled"
#define TEXT_ALIGN_TRUE_ENABLED_PREF_NAME "layout.css.text-align-true-value.enabled"

#ifdef DEBUG

bool nsLayoutUtils::gPreventAssertInCompareTreePosition = false;
#endif 

typedef FrameMetrics::ViewID ViewID;

 uint32_t nsLayoutUtils::sFontSizeInflationEmPerLine;
 uint32_t nsLayoutUtils::sFontSizeInflationMinTwips;
 uint32_t nsLayoutUtils::sFontSizeInflationLineThreshold;
 int32_t  nsLayoutUtils::sFontSizeInflationMappingIntercept;
 uint32_t nsLayoutUtils::sFontSizeInflationMaxRatio;
 bool nsLayoutUtils::sFontSizeInflationForceEnabled;
 bool nsLayoutUtils::sFontSizeInflationDisabledInMasterProcess;
 bool nsLayoutUtils::sInvalidationDebuggingIsEnabled;
 bool nsLayoutUtils::sCSSVariablesEnabled;
 bool nsLayoutUtils::sInterruptibleReflowEnabled;

static ViewID sScrollIdCounter = FrameMetrics::START_SCROLL_ID;

typedef nsDataHashtable<nsUint64HashKey, nsIContent*> ContentMap;
static ContentMap* sContentMap = nullptr;
static ContentMap& GetContentMap() {
  if (!sContentMap) {
    sContentMap = new ContentMap();
  }
  return *sContentMap;
}




static void
GridEnabledPrefChangeCallback(const char* aPrefName, void* aClosure)
{
  MOZ_ASSERT(strncmp(aPrefName, GRID_ENABLED_PREF_NAME,
                     ArrayLength(GRID_ENABLED_PREF_NAME)) == 0,
             "We only registered this callback for a single pref, so it "
             "should only be called for that pref");

  static int32_t sIndexOfGridInDisplayTable;
  static int32_t sIndexOfInlineGridInDisplayTable;
  static bool sAreGridKeywordIndicesInitialized; 

  bool isGridEnabled =
    Preferences::GetBool(GRID_ENABLED_PREF_NAME, false);
  if (!sAreGridKeywordIndicesInitialized) {
    
    
    sIndexOfGridInDisplayTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_grid,
                                     nsCSSProps::kDisplayKTable);
    MOZ_ASSERT(sIndexOfGridInDisplayTable >= 0,
               "Couldn't find grid in kDisplayKTable");
    sIndexOfInlineGridInDisplayTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_inline_grid,
                                     nsCSSProps::kDisplayKTable);
    MOZ_ASSERT(sIndexOfInlineGridInDisplayTable >= 0,
               "Couldn't find inline-grid in kDisplayKTable");
    sAreGridKeywordIndicesInitialized = true;
  }

  
  
  if (sIndexOfGridInDisplayTable >= 0) {
    nsCSSProps::kDisplayKTable[sIndexOfGridInDisplayTable] =
      isGridEnabled ? eCSSKeyword_grid : eCSSKeyword_UNKNOWN;
  }
  if (sIndexOfInlineGridInDisplayTable >= 0) {
    nsCSSProps::kDisplayKTable[sIndexOfInlineGridInDisplayTable] =
      isGridEnabled ? eCSSKeyword_inline_grid : eCSSKeyword_UNKNOWN;
  }
}

static void
RubyEnabledPrefChangeCallback(const char* aPrefName, void* aClosure)
{
  MOZ_ASSERT(strncmp(aPrefName, RUBY_ENABLED_PREF_NAME,
                     ArrayLength(RUBY_ENABLED_PREF_NAME)) == 0,
             "We only registered this callback for a single pref, so it "
             "should only be called for that pref");

  static int32_t sIndexOfRubyInDisplayTable;
  static int32_t sIndexOfRubyBaseInDisplayTable;
  static int32_t sIndexOfRubyBaseContainerInDisplayTable;
  static int32_t sIndexOfRubyTextInDisplayTable;
  static int32_t sIndexOfRubyTextContainerInDisplayTable;
  static bool sAreRubyKeywordIndicesInitialized; 

  bool isRubyEnabled =
    Preferences::GetBool(RUBY_ENABLED_PREF_NAME, false);
  if (!sAreRubyKeywordIndicesInitialized) {
    
    
    sIndexOfRubyInDisplayTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_ruby,
                                     nsCSSProps::kDisplayKTable);
    MOZ_ASSERT(sIndexOfRubyInDisplayTable >= 0,
               "Couldn't find ruby in kDisplayKTable");
    sIndexOfRubyBaseInDisplayTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_ruby_base,
                                     nsCSSProps::kDisplayKTable);
    MOZ_ASSERT(sIndexOfRubyBaseInDisplayTable >= 0,
               "Couldn't find ruby-base in kDisplayKTable");
    sIndexOfRubyBaseContainerInDisplayTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_ruby_base_container,
                                     nsCSSProps::kDisplayKTable);
    MOZ_ASSERT(sIndexOfRubyBaseContainerInDisplayTable >= 0,
               "Couldn't find ruby-base-container in kDisplayKTable");
    sIndexOfRubyTextInDisplayTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_ruby_text,
                                     nsCSSProps::kDisplayKTable);
    MOZ_ASSERT(sIndexOfRubyTextInDisplayTable >= 0,
               "Couldn't find ruby-text in kDisplayKTable");
    sIndexOfRubyTextContainerInDisplayTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_ruby_text_container,
                                     nsCSSProps::kDisplayKTable);
    MOZ_ASSERT(sIndexOfRubyTextContainerInDisplayTable >= 0,
               "Couldn't find ruby-text-container in kDisplayKTable");
    sAreRubyKeywordIndicesInitialized = true;
  }

  
  
  if (sIndexOfRubyInDisplayTable >= 0) {
    nsCSSProps::kDisplayKTable[sIndexOfRubyInDisplayTable] =
      isRubyEnabled ? eCSSKeyword_ruby : eCSSKeyword_UNKNOWN;
  }
  if (sIndexOfRubyBaseInDisplayTable >= 0) {
    nsCSSProps::kDisplayKTable[sIndexOfRubyBaseInDisplayTable] =
      isRubyEnabled ? eCSSKeyword_ruby_base : eCSSKeyword_UNKNOWN;
  }
  if (sIndexOfRubyBaseContainerInDisplayTable >= 0) {
    nsCSSProps::kDisplayKTable[sIndexOfRubyBaseContainerInDisplayTable] =
      isRubyEnabled ? eCSSKeyword_ruby_base_container : eCSSKeyword_UNKNOWN;
  }
  if (sIndexOfRubyTextInDisplayTable >= 0) {
    nsCSSProps::kDisplayKTable[sIndexOfRubyTextInDisplayTable] =
      isRubyEnabled ? eCSSKeyword_ruby_text : eCSSKeyword_UNKNOWN;
  }
  if (sIndexOfRubyTextContainerInDisplayTable >= 0) {
    nsCSSProps::kDisplayKTable[sIndexOfRubyTextContainerInDisplayTable] =
      isRubyEnabled ? eCSSKeyword_ruby_text_container : eCSSKeyword_UNKNOWN;
  }
}




static void
StickyEnabledPrefChangeCallback(const char* aPrefName, void* aClosure)
{
  MOZ_ASSERT(strncmp(aPrefName, STICKY_ENABLED_PREF_NAME,
                     ArrayLength(STICKY_ENABLED_PREF_NAME)) == 0,
             "We only registered this callback for a single pref, so it "
             "should only be called for that pref");

  static int32_t sIndexOfStickyInPositionTable;
  static bool sIsStickyKeywordIndexInitialized; 

  bool isStickyEnabled =
    Preferences::GetBool(STICKY_ENABLED_PREF_NAME, false);

  if (!sIsStickyKeywordIndexInitialized) {
    
    sIndexOfStickyInPositionTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_sticky,
                                     nsCSSProps::kPositionKTable);
    MOZ_ASSERT(sIndexOfStickyInPositionTable >= 0,
               "Couldn't find sticky in kPositionKTable");
    sIsStickyKeywordIndexInitialized = true;
  }

  
  
  nsCSSProps::kPositionKTable[sIndexOfStickyInPositionTable] =
    isStickyEnabled ? eCSSKeyword_sticky : eCSSKeyword_UNKNOWN;
}




static void
DisplayContentsEnabledPrefChangeCallback(const char* aPrefName, void* aClosure)
{
  NS_ASSERTION(strcmp(aPrefName, DISPLAY_CONTENTS_ENABLED_PREF_NAME) == 0,
               "Did you misspell " DISPLAY_CONTENTS_ENABLED_PREF_NAME " ?");

  static bool sIsDisplayContentsKeywordIndexInitialized;
  static int32_t sIndexOfContentsInDisplayTable;
  bool isDisplayContentsEnabled =
    Preferences::GetBool(DISPLAY_CONTENTS_ENABLED_PREF_NAME, false);

  if (!sIsDisplayContentsKeywordIndexInitialized) {
    
    sIndexOfContentsInDisplayTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_contents,
                                     nsCSSProps::kDisplayKTable);
    sIsDisplayContentsKeywordIndexInitialized = true;
  }

  
  
  if (sIndexOfContentsInDisplayTable >= 0) {
    nsCSSProps::kDisplayKTable[sIndexOfContentsInDisplayTable] =
      isDisplayContentsEnabled ? eCSSKeyword_contents : eCSSKeyword_UNKNOWN;
  }
}




static void
TextAlignTrueEnabledPrefChangeCallback(const char* aPrefName, void* aClosure)
{
  NS_ASSERTION(strcmp(aPrefName, TEXT_ALIGN_TRUE_ENABLED_PREF_NAME) == 0,
               "Did you misspell " TEXT_ALIGN_TRUE_ENABLED_PREF_NAME " ?");

  static bool sIsInitialized;
  static int32_t sIndexOfTrueInTextAlignTable;
  static int32_t sIndexOfTrueInTextAlignLastTable;
  bool isTextAlignTrueEnabled =
    Preferences::GetBool(TEXT_ALIGN_TRUE_ENABLED_PREF_NAME, false);

  if (!sIsInitialized) {
    
    sIndexOfTrueInTextAlignTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_true,
                                     nsCSSProps::kTextAlignKTable);
    
    sIndexOfTrueInTextAlignLastTable =
      nsCSSProps::FindIndexOfKeyword(eCSSKeyword_true,
                                     nsCSSProps::kTextAlignLastKTable);
    sIsInitialized = true;
  }

  
  
  MOZ_ASSERT(sIndexOfTrueInTextAlignTable >= 0);
  nsCSSProps::kTextAlignKTable[sIndexOfTrueInTextAlignTable] =
    isTextAlignTrueEnabled ? eCSSKeyword_true : eCSSKeyword_UNKNOWN;
  MOZ_ASSERT(sIndexOfTrueInTextAlignLastTable >= 0);
  nsCSSProps::kTextAlignLastKTable[sIndexOfTrueInTextAlignLastTable] =
    isTextAlignTrueEnabled ? eCSSKeyword_true : eCSSKeyword_UNKNOWN;
}

bool
nsLayoutUtils::HasAnimationsForCompositor(nsIContent* aContent,
                                          nsCSSProperty aProperty)
{
  return nsAnimationManager::GetAnimationsForCompositor(aContent, aProperty) ||
         nsTransitionManager::GetAnimationsForCompositor(aContent, aProperty);
}

static AnimationPlayerCollection*
GetAnimationsOrTransitions(nsIContent* aContent,
                           nsIAtom* aAnimationProperty,
                           nsCSSProperty aProperty)
{
  AnimationPlayerCollection* collection =
    static_cast<AnimationPlayerCollection*>(aContent->GetProperty(
        aAnimationProperty));
  if (collection) {
    bool propertyMatches = collection->HasAnimationOfProperty(aProperty);
    if (propertyMatches) {
      return collection;
    }
  }
  return nullptr;
}

bool
nsLayoutUtils::HasAnimations(nsIContent* aContent,
                             nsCSSProperty aProperty)
{
  if (!aContent->MayHaveAnimations())
    return false;
  return GetAnimationsOrTransitions(aContent, nsGkAtoms::animationsProperty,
                                    aProperty) ||
         GetAnimationsOrTransitions(aContent, nsGkAtoms::transitionsProperty,
                                    aProperty);
}

bool
nsLayoutUtils::HasCurrentAnimations(nsIContent* aContent,
                                    nsIAtom* aAnimationProperty)
{
  if (!aContent->MayHaveAnimations())
    return false;

  AnimationPlayerCollection* collection =
    static_cast<AnimationPlayerCollection*>(
      aContent->GetProperty(aAnimationProperty));
  return (collection && collection->HasCurrentAnimations());
}

bool
nsLayoutUtils::HasCurrentAnimationsForProperties(nsIContent* aContent,
                                                 const nsCSSProperty* aProperties,
                                                 size_t aPropertyCount)
{
  if (!aContent->MayHaveAnimations())
    return false;

  static nsIAtom* const sAnimProps[] = { nsGkAtoms::transitionsProperty,
                                         nsGkAtoms::animationsProperty,
                                         nullptr };
  for (nsIAtom* const* animProp = sAnimProps; *animProp; animProp++) {
    AnimationPlayerCollection* collection =
      static_cast<AnimationPlayerCollection*>(aContent->GetProperty(*animProp));
    if (collection &&
        collection->HasCurrentAnimationsForProperties(aProperties,
                                                      aPropertyCount)) {
      return true;
    }
  }

  return false;
}

static gfxSize
GetScaleForValue(const StyleAnimationValue& aValue, nsIFrame* aFrame)
{
  if (!aFrame) {
    NS_WARNING("No frame.");
    return gfxSize();
  }
  if (aValue.GetUnit() != StyleAnimationValue::eUnit_Transform) {
    NS_WARNING("Expected a transform.");
    return gfxSize();
  }

  nsCSSValueSharedList* list = aValue.GetCSSValueSharedListValue();
  MOZ_ASSERT(list->mHead);

  if (list->mHead->mValue.GetUnit() == eCSSUnit_None) {
    
    return gfxSize();
  }

  nsRect frameBounds = aFrame->GetRect();
  bool dontCare;
  gfx3DMatrix transform = nsStyleTransformMatrix::ReadTransforms(
                            list->mHead,
                            aFrame->StyleContext(),
                            aFrame->PresContext(), dontCare, frameBounds,
                            aFrame->PresContext()->AppUnitsPerDevPixel());

  gfxMatrix transform2d;
  bool canDraw2D = transform.CanDraw2D(&transform2d);
  if (!canDraw2D) {
    return gfxSize();
  }

  return transform2d.ScaleFactors(true);
}

static float
GetSuitableScale(float aMaxScale, float aMinScale)
{
  
  
  if (aMinScale >= 1.0f) {
    return aMinScale;
  }
  else if (aMaxScale <= 1.0f) {
    return aMaxScale;
  }

  return 1.0f;
}

static void
GetMinAndMaxScaleForAnimationProperty(nsIContent* aContent,
                                      AnimationPlayerCollection* aPlayers,
                                      gfxSize& aMaxScale,
                                      gfxSize& aMinScale)
{
  for (size_t playerIdx = aPlayers->mPlayers.Length(); playerIdx-- != 0; ) {
    AnimationPlayer* player = aPlayers->mPlayers[playerIdx];
    if (!player->GetEffect() || player->GetEffect()->IsFinishedTransition()) {
      continue;
    }
    dom::KeyframeEffectReadonly* effect = player->GetEffect();
    for (size_t propIdx = effect->Properties().Length(); propIdx-- != 0; ) {
      AnimationProperty& prop = effect->Properties()[propIdx];
      if (prop.mProperty == eCSSProperty_transform) {
        for (uint32_t segIdx = prop.mSegments.Length(); segIdx-- != 0; ) {
          AnimationPropertySegment& segment = prop.mSegments[segIdx];
          gfxSize from = GetScaleForValue(segment.mFromValue,
                                          aContent->GetPrimaryFrame());
          aMaxScale.width = std::max<float>(aMaxScale.width, from.width);
          aMaxScale.height = std::max<float>(aMaxScale.height, from.height);
          aMinScale.width = std::min<float>(aMinScale.width, from.width);
          aMinScale.height = std::min<float>(aMinScale.height, from.height);
          gfxSize to = GetScaleForValue(segment.mToValue,
                                        aContent->GetPrimaryFrame());
          aMaxScale.width = std::max<float>(aMaxScale.width, to.width);
          aMaxScale.height = std::max<float>(aMaxScale.height, to.height);
          aMinScale.width = std::min<float>(aMinScale.width, to.width);
          aMinScale.height = std::min<float>(aMinScale.height, to.height);
        }
      }
    }
  }
}

gfxSize
nsLayoutUtils::ComputeSuitableScaleForAnimation(nsIContent* aContent)
{
  gfxSize maxScale(std::numeric_limits<gfxFloat>::min(),
                   std::numeric_limits<gfxFloat>::min());
  gfxSize minScale(std::numeric_limits<gfxFloat>::max(),
                   std::numeric_limits<gfxFloat>::max());

  AnimationPlayerCollection* animations =
    nsAnimationManager::GetAnimationsForCompositor(aContent,
                                                   eCSSProperty_transform);
  if (animations) {
    GetMinAndMaxScaleForAnimationProperty(aContent, animations,
                                          maxScale, minScale);
  }

  animations =
    nsTransitionManager::GetAnimationsForCompositor(aContent,
                                                    eCSSProperty_transform);
  if (animations) {
    GetMinAndMaxScaleForAnimationProperty(aContent, animations,
                                          maxScale, minScale);
  }

  if (maxScale.width == std::numeric_limits<gfxFloat>::min()) {
    
    maxScale = minScale = gfxSize(1.0, 1.0);
  }

  return gfxSize(GetSuitableScale(maxScale.width, minScale.width),
                 GetSuitableScale(maxScale.height, minScale.height));
}

bool
nsLayoutUtils::AreAsyncAnimationsEnabled()
{
  static bool sAreAsyncAnimationsEnabled;
  static bool sAsyncPrefCached = false;

  if (!sAsyncPrefCached) {
    sAsyncPrefCached = true;
    Preferences::AddBoolVarCache(&sAreAsyncAnimationsEnabled,
                                 "layers.offmainthreadcomposition.async-animations");
  }

  return sAreAsyncAnimationsEnabled &&
    gfxPlatform::OffMainThreadCompositingEnabled();
}

bool
nsLayoutUtils::IsAnimationLoggingEnabled()
{
  static bool sShouldLog;
  static bool sShouldLogPrefCached;

  if (!sShouldLogPrefCached) {
    sShouldLogPrefCached = true;
    Preferences::AddBoolVarCache(&sShouldLog,
                                 "layers.offmainthreadcomposition.log-animations");
  }

  return sShouldLog;
}

bool
nsLayoutUtils::UseBackgroundNearestFiltering()
{
  static bool sUseBackgroundNearestFilteringEnabled;
  static bool sUseBackgroundNearestFilteringPrefInitialised = false;

  if (!sUseBackgroundNearestFilteringPrefInitialised) {
    sUseBackgroundNearestFilteringPrefInitialised = true;
    sUseBackgroundNearestFilteringEnabled =
      Preferences::GetBool("gfx.filter.nearest.force-enabled", false);
  }

  return sUseBackgroundNearestFilteringEnabled;
}

bool
nsLayoutUtils::GPUImageScalingEnabled()
{
  static bool sGPUImageScalingEnabled;
  static bool sGPUImageScalingPrefInitialised = false;

  if (!sGPUImageScalingPrefInitialised) {
    sGPUImageScalingPrefInitialised = true;
    sGPUImageScalingEnabled =
      Preferences::GetBool("layout.gpu-image-scaling.enabled", false);
  }

  return sGPUImageScalingEnabled;
}

bool
nsLayoutUtils::AnimatedImageLayersEnabled()
{
  static bool sAnimatedImageLayersEnabled;
  static bool sAnimatedImageLayersPrefCached = false;

  if (!sAnimatedImageLayersPrefCached) {
    sAnimatedImageLayersPrefCached = true;
    Preferences::AddBoolVarCache(&sAnimatedImageLayersEnabled,
                                 "layout.animated-image-layers.enabled",
                                 false);
  }

  return sAnimatedImageLayersEnabled;
}

bool
nsLayoutUtils::CSSFiltersEnabled()
{
  static bool sCSSFiltersEnabled;
  static bool sCSSFiltersPrefCached = false;

  if (!sCSSFiltersPrefCached) {
    sCSSFiltersPrefCached = true;
    Preferences::AddBoolVarCache(&sCSSFiltersEnabled,
                                 "layout.css.filters.enabled",
                                 false);
  }

  return sCSSFiltersEnabled;
}

bool
nsLayoutUtils::CSSClipPathShapesEnabled()
{
  static bool sCSSClipPathShapesEnabled;
  static bool sCSSClipPathShapesPrefCached = false;

  if (!sCSSClipPathShapesPrefCached) {
   sCSSClipPathShapesPrefCached = true;
   Preferences::AddBoolVarCache(&sCSSClipPathShapesEnabled,
                                "layout.css.clip-path-shapes.enabled",
                                false);
  }

  return sCSSClipPathShapesEnabled;
}

bool
nsLayoutUtils::UnsetValueEnabled()
{
  static bool sUnsetValueEnabled;
  static bool sUnsetValuePrefCached = false;

  if (!sUnsetValuePrefCached) {
    sUnsetValuePrefCached = true;
    Preferences::AddBoolVarCache(&sUnsetValueEnabled,
                                 "layout.css.unset-value.enabled",
                                 false);
  }

  return sUnsetValueEnabled;
}

bool
nsLayoutUtils::IsTextAlignTrueValueEnabled()
{
  static bool sTextAlignTrueValueEnabled;
  static bool sTextAlignTrueValueEnabledPrefCached = false;

  if (!sTextAlignTrueValueEnabledPrefCached) {
    sTextAlignTrueValueEnabledPrefCached = true;
    Preferences::AddBoolVarCache(&sTextAlignTrueValueEnabled,
                                 TEXT_ALIGN_TRUE_ENABLED_PREF_NAME,
                                 false);
  }

  return sTextAlignTrueValueEnabled;
}

void
nsLayoutUtils::UnionChildOverflow(nsIFrame* aFrame,
                                  nsOverflowAreas& aOverflowAreas,
                                  FrameChildListIDs aSkipChildLists)
{
  
  FrameChildListIDs skip = aSkipChildLists |
      nsIFrame::kSelectPopupList | nsIFrame::kPopupList;
  for (nsIFrame::ChildListIterator childLists(aFrame);
       !childLists.IsDone(); childLists.Next()) {
    if (skip.Contains(childLists.CurrentID())) {
      continue;
    }

    nsFrameList children = childLists.CurrentList();
    for (nsFrameList::Enumerator e(children); !e.AtEnd(); e.Next()) {
      nsIFrame* child = e.get();
      nsOverflowAreas childOverflow =
        child->GetOverflowAreas() + child->GetPosition();
      aOverflowAreas.UnionWith(childOverflow);
    }
  }
}

static void DestroyViewID(void* aObject, nsIAtom* aPropertyName,
                          void* aPropertyValue, void* aData)
{
  ViewID* id = static_cast<ViewID*>(aPropertyValue);
  GetContentMap().Remove(*id);
  delete id;
}





bool
nsLayoutUtils::FindIDFor(const nsIContent* aContent, ViewID* aOutViewId)
{
  void* scrollIdProperty = aContent->GetProperty(nsGkAtoms::RemoteId);
  if (scrollIdProperty) {
    *aOutViewId = *static_cast<ViewID*>(scrollIdProperty);
    return true;
  }
  return false;
}

ViewID
nsLayoutUtils::FindOrCreateIDFor(nsIContent* aContent)
{
  ViewID scrollId;

  if (!FindIDFor(aContent, &scrollId)) {
    scrollId = sScrollIdCounter++;
    aContent->SetProperty(nsGkAtoms::RemoteId, new ViewID(scrollId),
                          DestroyViewID);
    GetContentMap().Put(scrollId, aContent);
  }

  return scrollId;
}

nsIContent*
nsLayoutUtils::FindContentFor(ViewID aId)
{
  MOZ_ASSERT(aId != FrameMetrics::NULL_SCROLL_ID,
             "Cannot find a content element in map for null IDs.");
  nsIContent* content;
  bool exists = GetContentMap().Get(aId, &content);

  if (exists) {
    return content;
  } else {
    return nullptr;
  }
}

nsIScrollableFrame*
nsLayoutUtils::FindScrollableFrameFor(ViewID aId)
{
  nsIContent* content = FindContentFor(aId);
  if (!content) {
    return nullptr;
  }

  nsIFrame* scrolledFrame = content->GetPrimaryFrame();
  if (scrolledFrame && content->OwnerDoc()->GetRootElement() == content) {
    
    
    scrolledFrame = scrolledFrame->PresContext()->PresShell()->GetRootScrollFrame();
  }
  return scrolledFrame ? scrolledFrame->GetScrollTargetFrame() : nullptr;
}

static nsRect
ApplyRectMultiplier(nsRect aRect, float aMultiplier)
{
  if (aMultiplier == 1.0f) {
    return aRect;
  }
  float newWidth = aRect.width * aMultiplier;
  float newHeight = aRect.height * aMultiplier;
  float newX = aRect.x - ((newWidth - aRect.width) / 2.0f);
  float newY = aRect.y - ((newHeight - aRect.height) / 2.0f);
  
  return nsRect(ceil(newX), ceil(newY), floor(newWidth), floor(newHeight));
}



static nscoord
GetMaxDisplayPortSize(nsIContent* aContent)
{
  MOZ_ASSERT(!gfxPrefs::LayersTilesEnabled(), "Do not clamp displayports if tiling is enabled");

  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (!frame) {
    return nscoord_MAX;
  }
  frame = nsLayoutUtils::GetDisplayRootFrame(frame);

  nsIWidget* widget = frame->GetNearestWidget();
  if (!widget) {
    return nscoord_MAX;
  }
  LayerManager* lm = widget->GetLayerManager();
  if (!lm) {
    return nscoord_MAX;
  }
  nsPresContext* presContext = frame->PresContext();

  int32_t maxSizeInDevPixels = lm->GetMaxTextureSize();
  if (maxSizeInDevPixels < 0 || maxSizeInDevPixels == INT_MAX) {
    return nscoord_MAX;
  }
  return presContext->DevPixelsToAppUnits(maxSizeInDevPixels);
}

static nsRect
GetDisplayPortFromRectData(nsIContent* aContent,
                           DisplayPortPropertyData* aRectData,
                           float aMultiplier)
{
  
  
  
  
  
  return ApplyRectMultiplier(aRectData->mRect, aMultiplier);
}

static nsRect
GetDisplayPortFromMarginsData(nsIContent* aContent,
                              DisplayPortMarginsPropertyData* aMarginsData,
                              float aMultiplier)
{
  
  
  
  

  nsRect base;
  if (nsRect* baseData = static_cast<nsRect*>(aContent->GetProperty(nsGkAtoms::DisplayPortBase))) {
    base = *baseData;
  } else {
    NS_WARNING("Attempting to get a margins-based displayport with no base data!");
  }

  nsIFrame* frame = aContent->GetPrimaryFrame();
  if (!frame) {
    
    
    
    NS_WARNING("Attempting to get a displayport from a content with no primary frame!");
    return ApplyRectMultiplier(base, aMultiplier);
  }

  bool isRoot = false;
  if (aContent->OwnerDoc()->GetRootElement() == aContent) {
    
    
    frame = frame->PresContext()->PresShell()->GetRootScrollFrame();
    if (!frame) {
      
      return ApplyRectMultiplier(base, aMultiplier);
    }

    isRoot = true;
  }

  nsPoint scrollPos;
  if (nsIScrollableFrame* scrollableFrame = frame->GetScrollTargetFrame()) {
    scrollPos = scrollableFrame->GetScrollPosition();
  }

  nsPresContext* presContext = frame->PresContext();
  int32_t auPerDevPixel = presContext->AppUnitsPerDevPixel();

  LayoutDeviceToScreenScale2D res(presContext->PresShell()->GetCumulativeResolution()
                                * nsLayoutUtils::GetTransformToAncestorScale(frame));

  
  LayoutDeviceToScreenScale2D parentRes = res;
  if (isRoot) {
    
    
    float localRes = presContext->PresShell()->GetResolution();
    parentRes.xScale /= localRes;
    parentRes.yScale /= localRes;
  }
  ScreenRect screenRect = LayoutDeviceRect::FromAppUnits(base, auPerDevPixel)
                        * parentRes;

  if (gfxPrefs::LayersTilesEnabled()) {
    
    
    
    
    
    
    
    
    
    
    int alignmentX = gfxPlatform::GetPlatform()->GetTileWidth();
    int alignmentY = gfxPlatform::GetPlatform()->GetTileHeight();

    
    screenRect.Inflate(aMarginsData->mMargins);

    
    
    
    
    screenRect.Inflate(1);

    
    if (alignmentX == 0) {
      alignmentX = 1;
    }
    if (alignmentY == 0) {
      alignmentY = 1;
    }

    ScreenPoint scrollPosScreen = LayoutDevicePoint::FromAppUnits(scrollPos, auPerDevPixel)
                                * res;

    screenRect += scrollPosScreen;
    float x = alignmentX * floor(screenRect.x / alignmentX);
    float y = alignmentY * floor(screenRect.y / alignmentY);
    float w = alignmentX * ceil(screenRect.XMost() / alignmentX) - x;
    float h = alignmentY * ceil(screenRect.YMost() / alignmentY) - y;
    screenRect = ScreenRect(x, y, w, h);
    screenRect -= scrollPosScreen;
  } else {
    nscoord maxSizeInAppUnits = GetMaxDisplayPortSize(aContent);
    if (maxSizeInAppUnits == nscoord_MAX) {
      
      
      maxSizeInAppUnits = presContext->DevPixelsToAppUnits(8192);
    }

    
    int32_t maxSizeInDevPixels = presContext->AppUnitsToDevPixels(maxSizeInAppUnits);
    int32_t maxWidthInScreenPixels = floor(double(maxSizeInDevPixels) * res.xScale);
    int32_t maxHeightInScreenPixels = floor(double(maxSizeInDevPixels) * res.yScale);

    
    const ScreenMargin& margins = aMarginsData->mMargins;
    if (screenRect.height < maxHeightInScreenPixels) {
      int32_t budget = maxHeightInScreenPixels - screenRect.height;

      int32_t top = std::min(int32_t(margins.top), budget);
      screenRect.y -= top;
      screenRect.height += top + std::min(int32_t(margins.bottom), budget - top);
    }
    if (screenRect.width < maxWidthInScreenPixels) {
      int32_t budget = maxWidthInScreenPixels - screenRect.width;

      int32_t left = std::min(int32_t(margins.left), budget);
      screenRect.x -= left;
      screenRect.width += left + std::min(int32_t(margins.right), budget - left);
    }
  }

  
  nsRect result = LayoutDeviceRect::ToAppUnits(screenRect / res, auPerDevPixel);

  
  result = ApplyRectMultiplier(result, aMultiplier);

  
  nsRect expandedScrollableRect = nsLayoutUtils::CalculateExpandedScrollableRect(frame);
  result = expandedScrollableRect.Intersect(result + scrollPos) - scrollPos;

  return result;
}

static bool
GetDisplayPortImpl(nsIContent* aContent, nsRect *aResult, float aMultiplier)
{
  DisplayPortPropertyData* rectData =
    static_cast<DisplayPortPropertyData*>(aContent->GetProperty(nsGkAtoms::DisplayPort));
  DisplayPortMarginsPropertyData* marginsData =
    static_cast<DisplayPortMarginsPropertyData*>(aContent->GetProperty(nsGkAtoms::DisplayPortMargins));

  if (!rectData && !marginsData) {
    
    return false;
  }

  if (!aResult) {
    
    
    return true;
  }

  if (rectData && marginsData) {
    
    if (rectData->mPriority > marginsData->mPriority) {
      marginsData = nullptr;
    } else {
      rectData = nullptr;
    }
  }

  NS_ASSERTION((rectData == nullptr) != (marginsData == nullptr),
               "Only one of rectData or marginsData should be set!");

  nsRect result;
  if (rectData) {
    result = GetDisplayPortFromRectData(aContent, rectData, aMultiplier);
  } else {
    result = GetDisplayPortFromMarginsData(aContent, marginsData, aMultiplier);
  }

  if (!gfxPrefs::LayersTilesEnabled()) {
    
    
    NS_ASSERTION(result.width <= GetMaxDisplayPortSize(aContent),
                 "Displayport must be a valid texture size");
    NS_ASSERTION(result.height <= GetMaxDisplayPortSize(aContent),
                 "Displayport must be a valid texture size");
  }

  *aResult = result;
  return true;
}

bool
nsLayoutUtils::GetDisplayPort(nsIContent* aContent, nsRect *aResult)
{
  if (gfxPrefs::UseLowPrecisionBuffer()) {
    return GetDisplayPortImpl(aContent, aResult, 1.0f / gfxPrefs::LowPrecisionResolution());
  }
  return GetDisplayPortImpl(aContent, aResult, 1.0f);
}

bool
nsLayoutUtils::SetDisplayPortMargins(nsIContent* aContent,
                                     nsIPresShell* aPresShell,
                                     const ScreenMargin& aMargins,
                                     uint32_t aPriority,
                                     RepaintMode aRepaintMode)
{
  MOZ_ASSERT(aContent);
  MOZ_ASSERT(aContent->GetCurrentDoc() == aPresShell->GetDocument());

  DisplayPortMarginsPropertyData* currentData =
    static_cast<DisplayPortMarginsPropertyData*>(aContent->GetProperty(nsGkAtoms::DisplayPortMargins));
  if (currentData && currentData->mPriority > aPriority) {
    return false;
  }

  aContent->SetProperty(nsGkAtoms::DisplayPortMargins,
                        new DisplayPortMarginsPropertyData(
                            aMargins, aPriority),
                        nsINode::DeleteProperty<DisplayPortMarginsPropertyData>);

  if (nsLayoutUtils::UsesAsyncScrolling() && gfxPrefs::LayoutUseContainersForRootFrames()) {
    nsIFrame* rootScrollFrame = aPresShell->GetRootScrollFrame();
    if (rootScrollFrame && aContent == rootScrollFrame->GetContent()) {
      
      
      
      aPresShell->SetIgnoreViewportScrolling(true);
    }
  }

  if (aRepaintMode == RepaintMode::Repaint) {
    nsIFrame* rootFrame = aPresShell->FrameManager()->GetRootFrame();
    if (rootFrame) {
      rootFrame->SchedulePaint();
    }
  }

  return true;
}

void
nsLayoutUtils::SetDisplayPortBase(nsIContent* aContent, const nsRect& aBase)
{
  aContent->SetProperty(nsGkAtoms::DisplayPortBase, new nsRect(aBase),
                        nsINode::DeleteProperty<nsRect>);
}

void
nsLayoutUtils::SetDisplayPortBaseIfNotSet(nsIContent* aContent, const nsRect& aBase)
{
  if (!aContent->GetProperty(nsGkAtoms::DisplayPortBase)) {
    SetDisplayPortBase(aContent, aBase);
  }
}

bool
nsLayoutUtils::GetCriticalDisplayPort(nsIContent* aContent, nsRect* aResult)
{
  if (gfxPrefs::UseLowPrecisionBuffer()) {
    return GetDisplayPortImpl(aContent, aResult, 1.0f);
  }
  return false;
}

nsContainerFrame*
nsLayoutUtils::LastContinuationWithChild(nsContainerFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");
  nsIFrame* f = aFrame->LastContinuation();
  while (!f->GetFirstPrincipalChild() && f->GetPrevContinuation()) {
    f = f->GetPrevContinuation();
  }
  return static_cast<nsContainerFrame*>(f);
}


FrameChildListID
nsLayoutUtils::GetChildListNameFor(nsIFrame* aChildFrame)
{
  nsIFrame::ChildListID id = nsIFrame::kPrincipalList;

  if (aChildFrame->GetStateBits() & NS_FRAME_IS_OVERFLOW_CONTAINER) {
    nsIFrame* pif = aChildFrame->GetPrevInFlow();
    if (pif->GetParent() == aChildFrame->GetParent()) {
      id = nsIFrame::kExcessOverflowContainersList;
    }
    else {
      id = nsIFrame::kOverflowContainersList;
    }
  }
  
  else if (aChildFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
    
    const nsStyleDisplay* disp = aChildFrame->StyleDisplay();

    if (NS_STYLE_POSITION_ABSOLUTE == disp->mPosition) {
      id = nsIFrame::kAbsoluteList;
    } else if (NS_STYLE_POSITION_FIXED == disp->mPosition) {
      if (nsLayoutUtils::IsReallyFixedPos(aChildFrame)) {
        id = nsIFrame::kFixedList;
      } else {
        id = nsIFrame::kAbsoluteList;
      }
#ifdef MOZ_XUL
    } else if (NS_STYLE_DISPLAY_POPUP == disp->mDisplay) {
      
#ifdef DEBUG
      nsIFrame* parent = aChildFrame->GetParent();
      NS_ASSERTION(parent && parent->GetType() == nsGkAtoms::popupSetFrame,
                   "Unexpected parent");
#endif 

      id = nsIFrame::kPopupList;
#endif
    } else {
      NS_ASSERTION(aChildFrame->IsFloating(), "not a floated frame");
      id = nsIFrame::kFloatList;
    }

  } else {
    nsIAtom* childType = aChildFrame->GetType();
    if (nsGkAtoms::menuPopupFrame == childType) {
      nsIFrame* parent = aChildFrame->GetParent();
      MOZ_ASSERT(parent, "nsMenuPopupFrame can't be the root frame");
      if (parent) {
        if (parent->GetType() == nsGkAtoms::popupSetFrame) {
          id = nsIFrame::kPopupList;
        } else {
          nsIFrame* firstPopup = parent->GetFirstChild(nsIFrame::kPopupList);
          MOZ_ASSERT(!firstPopup || !firstPopup->GetNextSibling(),
                     "We assume popupList only has one child, but it has more.");
          id = firstPopup == aChildFrame
                 ? nsIFrame::kPopupList
                 : nsIFrame::kPrincipalList;
        }
      } else {
        id = nsIFrame::kPrincipalList;
      }
    } else if (nsGkAtoms::tableColGroupFrame == childType) {
      id = nsIFrame::kColGroupList;
    } else if (aChildFrame->IsTableCaption()) {
      id = nsIFrame::kCaptionList;
    } else {
      id = nsIFrame::kPrincipalList;
    }
  }

#ifdef DEBUG
  
  
  nsContainerFrame* parent = aChildFrame->GetParent();
  bool found = parent->GetChildList(id).ContainsFrame(aChildFrame);
  if (!found) {
    if (!(aChildFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
      found = parent->GetChildList(nsIFrame::kOverflowList)
                .ContainsFrame(aChildFrame);
    }
    else if (aChildFrame->IsFloating()) {
      found = parent->GetChildList(nsIFrame::kOverflowOutOfFlowList)
                .ContainsFrame(aChildFrame);
      if (!found) {
        found = parent->GetChildList(nsIFrame::kPushedFloatsList)
                  .ContainsFrame(aChildFrame);
      }
    }
    
    NS_POSTCONDITION(found, "not in child list");
  }
#endif

  return id;
}

 nsIFrame*
nsLayoutUtils::GetBeforeFrameForContent(nsIFrame* aFrame,
                                        nsIContent* aContent)
{
  
  
  nsContainerFrame* genConParentFrame =
    FirstContinuationOrIBSplitSibling(aFrame)->GetContentInsertionFrame();
  if (!genConParentFrame) {
    return nullptr;
  }
  nsTArray<nsIContent*>* prop = genConParentFrame->GetGenConPseudos();
  if (prop) {
    const nsTArray<nsIContent*>& pseudos(*prop);
    for (uint32_t i = 0; i < pseudos.Length(); ++i) {
      if (pseudos[i]->GetParent() == aContent &&
          pseudos[i]->NodeInfo()->NameAtom() == nsGkAtoms::mozgeneratedcontentbefore) {
        return pseudos[i]->GetPrimaryFrame();
      }
    }
  }
  
  
  
  nsIFrame* childFrame = genConParentFrame->GetFirstPrincipalChild();
  if (childFrame &&
      childFrame->IsPseudoFrame(aContent) &&
      !childFrame->IsGeneratedContentFrame()) {
    return GetBeforeFrameForContent(childFrame, aContent);
  }
  return nullptr;
}

 nsIFrame*
nsLayoutUtils::GetBeforeFrame(nsIFrame* aFrame)
{
  return GetBeforeFrameForContent(aFrame, aFrame->GetContent());
}

 nsIFrame*
nsLayoutUtils::GetAfterFrameForContent(nsIFrame* aFrame,
                                       nsIContent* aContent)
{
  
  
  nsContainerFrame* genConParentFrame =
    FirstContinuationOrIBSplitSibling(aFrame)->GetContentInsertionFrame();
  if (!genConParentFrame) {
    return nullptr;
  }
  nsTArray<nsIContent*>* prop = genConParentFrame->GetGenConPseudos();
  if (prop) {
    const nsTArray<nsIContent*>& pseudos(*prop);
    for (uint32_t i = 0; i < pseudos.Length(); ++i) {
      if (pseudos[i]->GetParent() == aContent &&
          pseudos[i]->NodeInfo()->NameAtom() == nsGkAtoms::mozgeneratedcontentafter) {
        return pseudos[i]->GetPrimaryFrame();
      }
    }
  }
  
  
  
  genConParentFrame = aFrame->GetContentInsertionFrame();
  if (!genConParentFrame) {
    return nullptr;
  }
  nsIFrame* lastParentContinuation =
    LastContinuationWithChild(static_cast<nsContainerFrame*>(
      LastContinuationOrIBSplitSibling(genConParentFrame)));
  nsIFrame* childFrame =
    lastParentContinuation->GetLastChild(nsIFrame::kPrincipalList);
  if (childFrame &&
      childFrame->IsPseudoFrame(aContent) &&
      !childFrame->IsGeneratedContentFrame()) {
    return GetAfterFrameForContent(childFrame->FirstContinuation(), aContent);
  }
  return nullptr;
}

 nsIFrame*
nsLayoutUtils::GetAfterFrame(nsIFrame* aFrame)
{
  return GetAfterFrameForContent(aFrame, aFrame->GetContent());
}


nsIFrame*
nsLayoutUtils::GetClosestFrameOfType(nsIFrame* aFrame, nsIAtom* aFrameType)
{
  for (nsIFrame* frame = aFrame; frame; frame = frame->GetParent()) {
    if (frame->GetType() == aFrameType) {
      return frame;
    }
  }
  return nullptr;
}


nsIFrame*
nsLayoutUtils::GetStyleFrame(nsIFrame* aFrame)
{
  if (aFrame->GetType() == nsGkAtoms::tableOuterFrame) {
    nsIFrame* inner = aFrame->GetFirstPrincipalChild();
    NS_ASSERTION(inner, "Outer table must have an inner");
    return inner;
  }

  return aFrame;
}

nsIFrame*
nsLayoutUtils::GetStyleFrame(const nsIContent* aContent)
{
  nsIFrame *frame = aContent->GetPrimaryFrame();
  if (!frame) {
    return nullptr;
  }

  return nsLayoutUtils::GetStyleFrame(frame);
}

nsIFrame*
nsLayoutUtils::GetFloatFromPlaceholder(nsIFrame* aFrame) {
  NS_ASSERTION(nsGkAtoms::placeholderFrame == aFrame->GetType(),
               "Must have a placeholder here");
  if (aFrame->GetStateBits() & PLACEHOLDER_FOR_FLOAT) {
    nsIFrame *outOfFlowFrame =
      nsPlaceholderFrame::GetRealFrameForPlaceholder(aFrame);
    NS_ASSERTION(outOfFlowFrame->IsFloating(),
                 "How did that happen?");
    return outOfFlowFrame;
  }

  return nullptr;
}


bool
nsLayoutUtils::IsGeneratedContentFor(nsIContent* aContent,
                                     nsIFrame* aFrame,
                                     nsIAtom* aPseudoElement)
{
  NS_PRECONDITION(aFrame, "Must have a frame");
  NS_PRECONDITION(aPseudoElement, "Must have a pseudo name");

  if (!aFrame->IsGeneratedContentFrame()) {
    return false;
  }
  nsIFrame* parent = aFrame->GetParent();
  NS_ASSERTION(parent, "Generated content can't be root frame");
  if (parent->IsGeneratedContentFrame()) {
    
    return false;
  }

  if (aContent && parent->GetContent() != aContent) {
    return false;
  }

  return (aFrame->GetContent()->NodeInfo()->NameAtom() == nsGkAtoms::mozgeneratedcontentbefore) ==
    (aPseudoElement == nsCSSPseudoElements::before);
}


nsIFrame*
nsLayoutUtils::GetCrossDocParentFrame(const nsIFrame* aFrame,
                                      nsPoint* aExtraOffset)
{
  nsIFrame* p = aFrame->GetParent();
  if (p)
    return p;

  nsView* v = aFrame->GetView();
  if (!v)
    return nullptr;
  v = v->GetParent(); 
  if (!v)
    return nullptr;
  if (aExtraOffset) {
    *aExtraOffset += v->GetPosition();
  }
  v = v->GetParent(); 
  return v ? v->GetFrame() : nullptr;
}


bool
nsLayoutUtils::IsProperAncestorFrameCrossDoc(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                             nsIFrame* aCommonAncestor)
{
  if (aFrame == aAncestorFrame)
    return false;
  return IsAncestorFrameCrossDoc(aAncestorFrame, aFrame, aCommonAncestor);
}


bool
nsLayoutUtils::IsAncestorFrameCrossDoc(const nsIFrame* aAncestorFrame, const nsIFrame* aFrame,
                                       const nsIFrame* aCommonAncestor)
{
  for (const nsIFrame* f = aFrame; f != aCommonAncestor;
       f = GetCrossDocParentFrame(f)) {
    if (f == aAncestorFrame)
      return true;
  }
  return aCommonAncestor == aAncestorFrame;
}


bool
nsLayoutUtils::IsProperAncestorFrame(nsIFrame* aAncestorFrame, nsIFrame* aFrame,
                                     nsIFrame* aCommonAncestor)
{
  if (aFrame == aAncestorFrame)
    return false;
  for (nsIFrame* f = aFrame; f != aCommonAncestor; f = f->GetParent()) {
    if (f == aAncestorFrame)
      return true;
  }
  return aCommonAncestor == aAncestorFrame;
}


int32_t
nsLayoutUtils::DoCompareTreePosition(nsIContent* aContent1,
                                     nsIContent* aContent2,
                                     int32_t aIf1Ancestor,
                                     int32_t aIf2Ancestor,
                                     const nsIContent* aCommonAncestor)
{
  NS_PRECONDITION(aContent1, "aContent1 must not be null");
  NS_PRECONDITION(aContent2, "aContent2 must not be null");

  nsAutoTArray<nsINode*, 32> content1Ancestors;
  nsINode* c1;
  for (c1 = aContent1; c1 && c1 != aCommonAncestor; c1 = c1->GetParentNode()) {
    content1Ancestors.AppendElement(c1);
  }
  if (!c1 && aCommonAncestor) {
    
    
    aCommonAncestor = nullptr;
  }

  nsAutoTArray<nsINode*, 32> content2Ancestors;
  nsINode* c2;
  for (c2 = aContent2; c2 && c2 != aCommonAncestor; c2 = c2->GetParentNode()) {
    content2Ancestors.AppendElement(c2);
  }
  if (!c2 && aCommonAncestor) {
    
    
    return DoCompareTreePosition(aContent1, aContent2,
                                 aIf1Ancestor, aIf2Ancestor, nullptr);
  }

  int last1 = content1Ancestors.Length() - 1;
  int last2 = content2Ancestors.Length() - 1;
  nsINode* content1Ancestor = nullptr;
  nsINode* content2Ancestor = nullptr;
  while (last1 >= 0 && last2 >= 0
         && ((content1Ancestor = content1Ancestors.ElementAt(last1)) ==
             (content2Ancestor = content2Ancestors.ElementAt(last2)))) {
    last1--;
    last2--;
  }

  if (last1 < 0) {
    if (last2 < 0) {
      NS_ASSERTION(aContent1 == aContent2, "internal error?");
      return 0;
    }
    
    return aIf1Ancestor;
  }

  if (last2 < 0) {
    
    return aIf2Ancestor;
  }

  
  nsINode* parent = content1Ancestor->GetParentNode();
#ifdef DEBUG
  
  NS_ASSERTION(gPreventAssertInCompareTreePosition || parent,
               "no common ancestor at all???");
#endif 
  if (!parent) { 
    return 0;
  }

  int32_t index1 = parent->IndexOf(content1Ancestor);
  int32_t index2 = parent->IndexOf(content2Ancestor);
  if (index1 < 0 || index2 < 0) {
    
    return 0;
  }

  return index1 - index2;
}


nsIFrame*
nsLayoutUtils::FillAncestors(nsIFrame* aFrame,
                             nsIFrame* aStopAtAncestor,
                             nsTArray<nsIFrame*>* aAncestors)
{
  while (aFrame && aFrame != aStopAtAncestor) {
    aAncestors->AppendElement(aFrame);
    aFrame = nsLayoutUtils::GetParentOrPlaceholderFor(aFrame);
  }
  return aFrame;
}


static bool IsFrameAfter(nsIFrame* aFrame1, nsIFrame* aFrame2)
{
  nsIFrame* f = aFrame2;
  do {
    f = f->GetNextSibling();
    if (f == aFrame1)
      return true;
  } while (f);
  return false;
}


int32_t
nsLayoutUtils::DoCompareTreePosition(nsIFrame* aFrame1,
                                     nsIFrame* aFrame2,
                                     int32_t aIf1Ancestor,
                                     int32_t aIf2Ancestor,
                                     nsIFrame* aCommonAncestor)
{
  NS_PRECONDITION(aFrame1, "aFrame1 must not be null");
  NS_PRECONDITION(aFrame2, "aFrame2 must not be null");

  nsAutoTArray<nsIFrame*,20> frame2Ancestors;
  nsIFrame* nonCommonAncestor =
    FillAncestors(aFrame2, aCommonAncestor, &frame2Ancestors);

  return DoCompareTreePosition(aFrame1, aFrame2, frame2Ancestors,
                               aIf1Ancestor, aIf2Ancestor,
                               nonCommonAncestor ? aCommonAncestor : nullptr);
}


int32_t
nsLayoutUtils::DoCompareTreePosition(nsIFrame* aFrame1,
                                     nsIFrame* aFrame2,
                                     nsTArray<nsIFrame*>& aFrame2Ancestors,
                                     int32_t aIf1Ancestor,
                                     int32_t aIf2Ancestor,
                                     nsIFrame* aCommonAncestor)
{
  NS_PRECONDITION(aFrame1, "aFrame1 must not be null");
  NS_PRECONDITION(aFrame2, "aFrame2 must not be null");

  nsPresContext* presContext = aFrame1->PresContext();
  if (presContext != aFrame2->PresContext()) {
    NS_ERROR("no common ancestor at all, different documents");
    return 0;
  }

  nsAutoTArray<nsIFrame*,20> frame1Ancestors;
  if (aCommonAncestor &&
      !FillAncestors(aFrame1, aCommonAncestor, &frame1Ancestors)) {
    
    
    return DoCompareTreePosition(aFrame1, aFrame2,
                                 aIf1Ancestor, aIf2Ancestor, nullptr);
  }

  int32_t last1 = int32_t(frame1Ancestors.Length()) - 1;
  int32_t last2 = int32_t(aFrame2Ancestors.Length()) - 1;
  while (last1 >= 0 && last2 >= 0 &&
         frame1Ancestors[last1] == aFrame2Ancestors[last2]) {
    last1--;
    last2--;
  }

  if (last1 < 0) {
    if (last2 < 0) {
      NS_ASSERTION(aFrame1 == aFrame2, "internal error?");
      return 0;
    }
    
    return aIf1Ancestor;
  }

  if (last2 < 0) {
    
    return aIf2Ancestor;
  }

  nsIFrame* ancestor1 = frame1Ancestors[last1];
  nsIFrame* ancestor2 = aFrame2Ancestors[last2];
  
  if (IsFrameAfter(ancestor2, ancestor1))
    return -1;
  if (IsFrameAfter(ancestor1, ancestor2))
    return 1;
  NS_WARNING("Frames were in different child lists???");
  return 0;
}


nsIFrame* nsLayoutUtils::GetLastSibling(nsIFrame* aFrame) {
  if (!aFrame) {
    return nullptr;
  }

  nsIFrame* next;
  while ((next = aFrame->GetNextSibling()) != nullptr) {
    aFrame = next;
  }
  return aFrame;
}


nsView*
nsLayoutUtils::FindSiblingViewFor(nsView* aParentView, nsIFrame* aFrame) {
  nsIFrame* parentViewFrame = aParentView->GetFrame();
  nsIContent* parentViewContent = parentViewFrame ? parentViewFrame->GetContent() : nullptr;
  for (nsView* insertBefore = aParentView->GetFirstChild(); insertBefore;
       insertBefore = insertBefore->GetNextSibling()) {
    nsIFrame* f = insertBefore->GetFrame();
    if (!f) {
      
      for (nsView* searchView = insertBefore->GetParent(); searchView;
           searchView = searchView->GetParent()) {
        f = searchView->GetFrame();
        if (f) {
          break;
        }
      }
      NS_ASSERTION(f, "Can't find a frame anywhere!");
    }
    if (!f || !aFrame->GetContent() || !f->GetContent() ||
        CompareTreePosition(aFrame->GetContent(), f->GetContent(), parentViewContent) > 0) {
      
      
      return insertBefore;
    }
  }
  return nullptr;
}


nsIScrollableFrame*
nsLayoutUtils::GetScrollableFrameFor(const nsIFrame *aScrolledFrame)
{
  nsIFrame *frame = aScrolledFrame->GetParent();
  nsIScrollableFrame *sf = do_QueryFrame(frame);
  return (sf && sf->GetScrolledFrame() == aScrolledFrame) ? sf : nullptr;
}

 void
nsLayoutUtils::SetFixedPositionLayerData(Layer* aLayer,
                                         const nsIFrame* aViewportFrame,
                                         const nsRect& aAnchorRect,
                                         const nsIFrame* aFixedPosFrame,
                                         nsPresContext* aPresContext,
                                         const ContainerLayerParameters& aContainerParameters) {
  
  
  
  float factor = aPresContext->AppUnitsPerDevPixel();
  Rect anchorRect(NSAppUnitsToFloatPixels(aAnchorRect.x, factor) *
                    aContainerParameters.mXScale,
                  NSAppUnitsToFloatPixels(aAnchorRect.y, factor) *
                    aContainerParameters.mYScale,
                  NSAppUnitsToFloatPixels(aAnchorRect.width, factor) *
                    aContainerParameters.mXScale,
                  NSAppUnitsToFloatPixels(aAnchorRect.height, factor) *
                    aContainerParameters.mYScale);
  
  
  Matrix transform2d;
  if (aLayer->GetTransform().Is2D(&transform2d)) {
    transform2d.Invert();
    anchorRect = transform2d.TransformBounds(anchorRect);
  } else {
    NS_ERROR("3D transform found between fixedpos content and its viewport (should never happen)");
    anchorRect = Rect(0,0,0,0);
  }

  
  
  
  
  LayerPoint anchor(anchorRect.x, anchorRect.y);
  
  
  nsMargin fixedMargins = aPresContext->PresShell()->GetContentDocumentFixedPositionMargins();
  LayerMargin fixedLayerMargins(NSAppUnitsToFloatPixels(fixedMargins.top, factor) *
                                  aContainerParameters.mYScale,
                                NSAppUnitsToFloatPixels(fixedMargins.right, factor) *
                                  aContainerParameters.mXScale,
                                NSAppUnitsToFloatPixels(fixedMargins.bottom, factor) *
                                  aContainerParameters.mYScale,
                                NSAppUnitsToFloatPixels(fixedMargins.left, factor) *
                                  aContainerParameters.mXScale);

  if (aFixedPosFrame != aViewportFrame) {
    const nsStylePosition* position = aFixedPosFrame->StylePosition();
    if (position->mOffset.GetRightUnit() != eStyleUnit_Auto) {
      if (position->mOffset.GetLeftUnit() != eStyleUnit_Auto) {
        anchor.x = anchorRect.x + anchorRect.width / 2.f;
      } else {
        anchor.x = anchorRect.XMost();
      }
    }
    if (position->mOffset.GetBottomUnit() != eStyleUnit_Auto) {
      if (position->mOffset.GetTopUnit() != eStyleUnit_Auto) {
        anchor.y = anchorRect.y + anchorRect.height / 2.f;
      } else {
        anchor.y = anchorRect.YMost();
      }
    }

    
    
    
    if (position->mOffset.GetLeftUnit() == eStyleUnit_Auto &&
        position->mOffset.GetRightUnit() == eStyleUnit_Auto) {
      fixedLayerMargins.left = -1;
    }
    if (position->mOffset.GetTopUnit() == eStyleUnit_Auto &&
        position->mOffset.GetBottomUnit() == eStyleUnit_Auto) {
      fixedLayerMargins.top = -1;
    }
  }

  aLayer->SetFixedPositionAnchor(anchor);
  aLayer->SetFixedPositionMargins(fixedLayerMargins);
}

bool
nsLayoutUtils::ViewportHasDisplayPort(nsPresContext* aPresContext, nsRect* aDisplayPort)
{
  nsIFrame* rootScrollFrame =
    aPresContext->PresShell()->GetRootScrollFrame();
  return rootScrollFrame &&
    nsLayoutUtils::GetDisplayPort(rootScrollFrame->GetContent(), aDisplayPort);
}

bool
nsLayoutUtils::IsFixedPosFrameInDisplayPort(const nsIFrame* aFrame, nsRect* aDisplayPort)
{
  
  
  
  nsIFrame* parent = aFrame->GetParent();
  if (!parent || parent->GetParent() ||
      aFrame->StyleDisplay()->mPosition != NS_STYLE_POSITION_FIXED) {
    return false;
  }
  return ViewportHasDisplayPort(aFrame->PresContext(), aDisplayPort);
}

NS_DECLARE_FRAME_PROPERTY(ScrollbarThumbLayerized, nullptr)

 void
nsLayoutUtils::SetScrollbarThumbLayerization(nsIFrame* aThumbFrame, bool aLayerize)
{
  aThumbFrame->Properties().Set(ScrollbarThumbLayerized(),
    reinterpret_cast<void*>(intptr_t(aLayerize)));
}

bool
nsLayoutUtils::IsScrollbarThumbLayerized(nsIFrame* aThumbFrame)
{
  return reinterpret_cast<intptr_t>(aThumbFrame->Properties().Get(ScrollbarThumbLayerized()));
}

nsIFrame*
nsLayoutUtils::GetAnimatedGeometryRootForFrame(nsDisplayListBuilder* aBuilder,
                                               nsIFrame* aFrame,
                                               const nsIFrame* aStopAtAncestor)
{
  return aBuilder->FindAnimatedGeometryRootFor(aFrame, aStopAtAncestor);
}

nsIFrame*
nsLayoutUtils::GetAnimatedGeometryRootFor(nsDisplayItem* aItem,
                                          nsDisplayListBuilder* aBuilder,
                                          LayerManager* aManager)
{
  nsIFrame* f = aItem->Frame();
  if (aItem->GetType() == nsDisplayItem::TYPE_SCROLL_LAYER) {
    nsDisplayScrollLayer* scrollLayerItem =
      static_cast<nsDisplayScrollLayer*>(aItem);
    nsIFrame* scrolledFrame = scrollLayerItem->GetScrolledFrame();
    return GetAnimatedGeometryRootForFrame(aBuilder, scrolledFrame,
        aBuilder->FindReferenceFrameFor(scrolledFrame));
  }
  if (aItem->ShouldFixToViewport(aManager)) {
    
    
    
    
    nsIFrame* viewportFrame =
      nsLayoutUtils::GetClosestFrameOfType(f, nsGkAtoms::viewportFrame);
    NS_ASSERTION(viewportFrame, "no viewport???");
    return GetAnimatedGeometryRootForFrame(aBuilder, viewportFrame,
        aBuilder->FindReferenceFrameFor(viewportFrame));
  }
  return GetAnimatedGeometryRootForFrame(aBuilder, f, aItem->ReferenceFrame());
}


nsIScrollableFrame*
nsLayoutUtils::GetNearestScrollableFrameForDirection(nsIFrame* aFrame,
                                                     Direction aDirection)
{
  NS_ASSERTION(aFrame, "GetNearestScrollableFrameForDirection expects a non-null frame");
  for (nsIFrame* f = aFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    nsIScrollableFrame* scrollableFrame = do_QueryFrame(f);
    if (scrollableFrame) {
      ScrollbarStyles ss = scrollableFrame->GetScrollbarStyles();
      uint32_t directions = scrollableFrame->GetPerceivedScrollingDirections();
      if (aDirection == eVertical ?
          (ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN &&
           (directions & nsIScrollableFrame::VERTICAL)) :
          (ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN &&
           (directions & nsIScrollableFrame::HORIZONTAL)))
        return scrollableFrame;
    }
  }
  return nullptr;
}


nsIScrollableFrame*
nsLayoutUtils::GetNearestScrollableFrame(nsIFrame* aFrame, uint32_t aFlags)
{
  NS_ASSERTION(aFrame, "GetNearestScrollableFrame expects a non-null frame");
  for (nsIFrame* f = aFrame; f; f = (aFlags & SCROLLABLE_SAME_DOC) ?
       f->GetParent() : nsLayoutUtils::GetCrossDocParentFrame(f)) {
    nsIScrollableFrame* scrollableFrame = do_QueryFrame(f);
    if (scrollableFrame) {
      if (aFlags & SCROLLABLE_ONLY_ASYNC_SCROLLABLE) {
        if (scrollableFrame->WantAsyncScroll()) {
          return scrollableFrame;
        }
        continue;
      }
      ScrollbarStyles ss = scrollableFrame->GetScrollbarStyles();
      if ((aFlags & SCROLLABLE_INCLUDE_HIDDEN) ||
          ss.mVertical != NS_STYLE_OVERFLOW_HIDDEN ||
          ss.mHorizontal != NS_STYLE_OVERFLOW_HIDDEN)
        return scrollableFrame;
    }
    if (aFlags & SCROLLABLE_ALWAYS_MATCH_ROOT) {
      nsIPresShell* ps = f->PresContext()->PresShell();
      if (ps->GetDocument() && ps->GetDocument()->IsRootDisplayDocument() &&
          ps->GetRootFrame() == f) {
        return ps->GetRootScrollFrameAsScrollable();
      }
    }
  }
  return nullptr;
}


nsRect
nsLayoutUtils::GetScrolledRect(nsIFrame* aScrolledFrame,
                               const nsRect& aScrolledFrameOverflowArea,
                               const nsSize& aScrollPortSize,
                               uint8_t aDirection)
{
  nscoord x1 = aScrolledFrameOverflowArea.x,
          x2 = aScrolledFrameOverflowArea.XMost(),
          y1 = aScrolledFrameOverflowArea.y,
          y2 = aScrolledFrameOverflowArea.YMost();
  if (y1 < 0) {
    y1 = 0;
  }
  if (aDirection != NS_STYLE_DIRECTION_RTL) {
    if (x1 < 0) {
      x1 = 0;
    }
  } else {
    if (x2 > aScrollPortSize.width) {
      x2 = aScrollPortSize.width;
    }
    
    
    
    
    
    
    nscoord extraWidth = std::max(0, aScrolledFrame->GetSize().width - aScrollPortSize.width);
    x2 += extraWidth;
  }
  return nsRect(x1, y1, x2 - x1, y2 - y1);
}


bool
nsLayoutUtils::HasPseudoStyle(nsIContent* aContent,
                              nsStyleContext* aStyleContext,
                              nsCSSPseudoElements::Type aPseudoElement,
                              nsPresContext* aPresContext)
{
  NS_PRECONDITION(aPresContext, "Must have a prescontext");

  nsRefPtr<nsStyleContext> pseudoContext;
  if (aContent) {
    pseudoContext = aPresContext->StyleSet()->
      ProbePseudoElementStyle(aContent->AsElement(), aPseudoElement,
                              aStyleContext);
  }
  return pseudoContext != nullptr;
}

nsPoint
nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(nsIDOMEvent* aDOMEvent, nsIFrame* aFrame)
{
  if (!aDOMEvent)
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  WidgetEvent* event = aDOMEvent->GetInternalNSEvent();
  if (!event)
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  return GetEventCoordinatesRelativeTo(event, aFrame);
}

nsPoint
nsLayoutUtils::GetEventCoordinatesRelativeTo(const WidgetEvent* aEvent,
                                             nsIFrame* aFrame)
{
  if (!aEvent || (aEvent->mClass != eMouseEventClass &&
                  aEvent->mClass != eMouseScrollEventClass &&
                  aEvent->mClass != eWheelEventClass &&
                  aEvent->mClass != eDragEventClass &&
                  aEvent->mClass != eSimpleGestureEventClass &&
                  aEvent->mClass != ePointerEventClass &&
                  aEvent->mClass != eGestureNotifyEventClass &&
                  aEvent->mClass != eTouchEventClass &&
                  aEvent->mClass != eQueryContentEventClass))
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);

  return GetEventCoordinatesRelativeTo(aEvent,
           aEvent->AsGUIEvent()->refPoint,
           aFrame);
}

nsPoint
nsLayoutUtils::GetEventCoordinatesRelativeTo(const WidgetEvent* aEvent,
                                             const LayoutDeviceIntPoint& aPoint,
                                             nsIFrame* aFrame)
{
  if (!aFrame) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  nsIWidget* widget = aEvent->AsGUIEvent()->widget;
  if (!widget) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  return GetEventCoordinatesRelativeTo(widget, aPoint, aFrame);
}

nsPoint
nsLayoutUtils::GetEventCoordinatesRelativeTo(nsIWidget* aWidget,
                                             const LayoutDeviceIntPoint& aPoint,
                                             nsIFrame* aFrame)
{
  if (!aFrame || !aWidget) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  nsView* view = aFrame->GetView();
  if (view) {
    nsIWidget* frameWidget = view->GetWidget();
    if (frameWidget && frameWidget == aWidget) {
      
      
      
      nsPresContext* presContext = aFrame->PresContext();
      nsPoint pt(presContext->DevPixelsToAppUnits(aPoint.x),
                 presContext->DevPixelsToAppUnits(aPoint.y));
      return pt - view->ViewToWidgetOffset();
    }
  }

  



  nsIFrame* rootFrame = aFrame;
  bool transformFound = false;
  for (nsIFrame* f = aFrame; f; f = GetCrossDocParentFrame(f)) {
    if (f->IsTransformed()) {
      transformFound = true;
    }

    rootFrame = f;
  }

  nsView* rootView = rootFrame->GetView();
  if (!rootView) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  nsPoint widgetToView = TranslateWidgetToView(rootFrame->PresContext(),
                                               aWidget, aPoint, rootView);

  if (widgetToView == nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE)) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  
  
  int32_t rootAPD = rootFrame->PresContext()->AppUnitsPerDevPixel();
  int32_t localAPD = aFrame->PresContext()->AppUnitsPerDevPixel();
  widgetToView = widgetToView.ScaleToOtherAppUnits(rootAPD, localAPD);

  


  if (transformFound || aFrame->IsSVGText()) {
    return TransformRootPointToFrame(aFrame, widgetToView);
  }

  


  return widgetToView - aFrame->GetOffsetToCrossDoc(rootFrame);
}

nsIFrame*
nsLayoutUtils::GetPopupFrameForEventCoordinates(nsPresContext* aPresContext,
                                                const WidgetEvent* aEvent)
{
#ifdef MOZ_XUL
  nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
  if (!pm) {
    return nullptr;
  }
  nsTArray<nsIFrame*> popups;
  pm->GetVisiblePopups(popups);
  uint32_t i;
  
  for (i = 0; i < popups.Length(); i++) {
    nsIFrame* popup = popups[i];
    if (popup->PresContext()->GetRootPresContext() == aPresContext &&
        popup->GetScrollableOverflowRect().Contains(
          GetEventCoordinatesRelativeTo(aEvent, popup))) {
      return popup;
    }
  }
#endif
  return nullptr;
}

static void ConstrainToCoordValues(float& aStart, float& aSize)
{
  MOZ_ASSERT(aSize >= 0);

  
  

  
  
  
  float end = aStart + aSize;
  aStart = clamped(aStart, float(nscoord_MIN), float(nscoord_MAX));
  end = clamped(end, float(nscoord_MIN), float(nscoord_MAX));

  aSize = end - aStart;

  
  
  
  
  if (aSize > nscoord_MAX) {
    float excess = aSize - nscoord_MAX;
    excess /= 2;
    aStart += excess;
    aSize = nscoord_MAX;
  }
}






static void ConstrainToCoordValues(gfxFloat& aVal)
{
  if (aVal <= nscoord_MIN)
    aVal = nscoord_MIN;
  else if (aVal >= nscoord_MAX)
    aVal = nscoord_MAX;
}

static void ConstrainToCoordValues(gfxFloat& aStart, gfxFloat& aSize)
{
  gfxFloat max = aStart + aSize;

  
  ConstrainToCoordValues(aStart);
  ConstrainToCoordValues(max);

  aSize = max - aStart;
  
  
  if (aSize > nscoord_MAX) {
    gfxFloat excess = aSize - nscoord_MAX;
    excess /= 2;

    aStart += excess;
    aSize = nscoord_MAX;
  } else if (aSize < nscoord_MIN) {
    gfxFloat excess = aSize - nscoord_MIN;
    excess /= 2;

    aStart -= excess;
    aSize = nscoord_MIN;
  }
}

nsRect
nsLayoutUtils::RoundGfxRectToAppRect(const Rect &aRect, float aFactor)
{
  
  Rect scaledRect = aRect;
  scaledRect.ScaleRoundOut(aFactor);

  
  ConstrainToCoordValues(scaledRect.x, scaledRect.width);
  ConstrainToCoordValues(scaledRect.y, scaledRect.height);

  
  return nsRect(nscoord(scaledRect.X()), nscoord(scaledRect.Y()),
                nscoord(scaledRect.Width()), nscoord(scaledRect.Height()));
}

nsRect
nsLayoutUtils::RoundGfxRectToAppRect(const gfxRect &aRect, float aFactor)
{
  
  gfxRect scaledRect = aRect;
  scaledRect.ScaleRoundOut(aFactor);

  
  ConstrainToCoordValues(scaledRect.x, scaledRect.width);
  ConstrainToCoordValues(scaledRect.y, scaledRect.height);

  
  return nsRect(nscoord(scaledRect.X()), nscoord(scaledRect.Y()),
                nscoord(scaledRect.Width()), nscoord(scaledRect.Height()));
}


nsRegion
nsLayoutUtils::RoundedRectIntersectRect(const nsRect& aRoundedRect,
                                        const nscoord aRadii[8],
                                        const nsRect& aContainedRect)
{
  
  
  nsRect rectFullHeight = aRoundedRect;
  nscoord xDiff = std::max(aRadii[NS_CORNER_TOP_LEFT_X], aRadii[NS_CORNER_BOTTOM_LEFT_X]);
  rectFullHeight.x += xDiff;
  rectFullHeight.width -= std::max(aRadii[NS_CORNER_TOP_RIGHT_X],
                                 aRadii[NS_CORNER_BOTTOM_RIGHT_X]) + xDiff;
  nsRect r1;
  r1.IntersectRect(rectFullHeight, aContainedRect);

  nsRect rectFullWidth = aRoundedRect;
  nscoord yDiff = std::max(aRadii[NS_CORNER_TOP_LEFT_Y], aRadii[NS_CORNER_TOP_RIGHT_Y]);
  rectFullWidth.y += yDiff;
  rectFullWidth.height -= std::max(aRadii[NS_CORNER_BOTTOM_LEFT_Y],
                                 aRadii[NS_CORNER_BOTTOM_RIGHT_Y]) + yDiff;
  nsRect r2;
  r2.IntersectRect(rectFullWidth, aContainedRect);

  nsRegion result;
  result.Or(r1, r2);
  return result;
}

nsIntRegion
nsLayoutUtils::RoundedRectIntersectIntRect(const nsIntRect& aRoundedRect,
                                           const RectCornerRadii& aCornerRadii,
                                           const nsIntRect& aContainedRect)
{
  
  
  nsIntRect rectFullHeight = aRoundedRect;
  uint32_t xDiff = std::max(aCornerRadii.TopLeft().width,
                            aCornerRadii.BottomLeft().width);
  rectFullHeight.x += xDiff;
  rectFullHeight.width -= std::max(aCornerRadii.TopRight().width,
                                   aCornerRadii.BottomRight().width) + xDiff;
  nsIntRect r1;
  r1.IntersectRect(rectFullHeight, aContainedRect);

  nsIntRect rectFullWidth = aRoundedRect;
  uint32_t yDiff = std::max(aCornerRadii.TopLeft().height,
                            aCornerRadii.TopRight().height);
  rectFullWidth.y += yDiff;
  rectFullWidth.height -= std::max(aCornerRadii.BottomLeft().height,
                                   aCornerRadii.BottomRight().height) + yDiff;
  nsIntRect r2;
  r2.IntersectRect(rectFullWidth, aContainedRect);

  nsIntRegion result;
  result.Or(r1, r2);
  return result;
}


static bool
CheckCorner(nscoord aXOffset, nscoord aYOffset,
            nscoord aXRadius, nscoord aYRadius)
{
  MOZ_ASSERT(aXOffset > 0 && aYOffset > 0,
             "must not pass nonpositives to CheckCorner");
  MOZ_ASSERT(aXRadius >= 0 && aYRadius >= 0,
             "must not pass negatives to CheckCorner");

  
  
  
  if (aXOffset >= aXRadius || aYOffset >= aYRadius)
    return true;

  
  
  float scaledX = float(aXRadius - aXOffset) / float(aXRadius);
  float scaledY = float(aYRadius - aYOffset) / float(aYRadius);
  return scaledX * scaledX + scaledY * scaledY < 1.0f;
}

bool
nsLayoutUtils::RoundedRectIntersectsRect(const nsRect& aRoundedRect,
                                         const nscoord aRadii[8],
                                         const nsRect& aTestRect)
{
  if (!aTestRect.Intersects(aRoundedRect))
    return false;

  
  
  nsMargin insets;
  insets.top = aTestRect.YMost() - aRoundedRect.y;
  insets.right = aRoundedRect.XMost() - aTestRect.x;
  insets.bottom = aRoundedRect.YMost() - aTestRect.y;
  insets.left = aTestRect.XMost() - aRoundedRect.x;

  
  
  
  return CheckCorner(insets.left, insets.top,
                     aRadii[NS_CORNER_TOP_LEFT_X],
                     aRadii[NS_CORNER_TOP_LEFT_Y]) &&
         CheckCorner(insets.right, insets.top,
                     aRadii[NS_CORNER_TOP_RIGHT_X],
                     aRadii[NS_CORNER_TOP_RIGHT_Y]) &&
         CheckCorner(insets.right, insets.bottom,
                     aRadii[NS_CORNER_BOTTOM_RIGHT_X],
                     aRadii[NS_CORNER_BOTTOM_RIGHT_Y]) &&
         CheckCorner(insets.left, insets.bottom,
                     aRadii[NS_CORNER_BOTTOM_LEFT_X],
                     aRadii[NS_CORNER_BOTTOM_LEFT_Y]);
}

nsRect
nsLayoutUtils::MatrixTransformRectOut(const nsRect &aBounds,
                                      const gfx3DMatrix &aMatrix, float aFactor)
{
  nsRect outside = aBounds;
  outside.ScaleRoundOut(1/aFactor);
  gfxRect image = aMatrix.TransformBounds(gfxRect(outside.x,
                                                  outside.y,
                                                  outside.width,
                                                  outside.height));
  return RoundGfxRectToAppRect(image, aFactor);
}

nsRect
nsLayoutUtils::MatrixTransformRect(const nsRect &aBounds,
                                   const gfx3DMatrix &aMatrix, float aFactor)
{
  gfxRect image = aMatrix.TransformBounds(gfxRect(NSAppUnitsToDoublePixels(aBounds.x, aFactor),
                                                  NSAppUnitsToDoublePixels(aBounds.y, aFactor),
                                                  NSAppUnitsToDoublePixels(aBounds.width, aFactor),
                                                  NSAppUnitsToDoublePixels(aBounds.height, aFactor)));

  return RoundGfxRectToAppRect(image, aFactor);
}

nsPoint
nsLayoutUtils::MatrixTransformPoint(const nsPoint &aPoint,
                                    const gfx3DMatrix &aMatrix, float aFactor)
{
  gfxPoint image = aMatrix.Transform(gfxPoint(NSAppUnitsToFloatPixels(aPoint.x, aFactor),
                                              NSAppUnitsToFloatPixels(aPoint.y, aFactor)));
  return nsPoint(NSFloatPixelsToAppUnits(float(image.x), aFactor),
                 NSFloatPixelsToAppUnits(float(image.y), aFactor));
}

Matrix4x4
nsLayoutUtils::GetTransformToAncestor(nsIFrame *aFrame, const nsIFrame *aAncestor)
{
  nsIFrame* parent;
  Matrix4x4 ctm;
  if (aFrame == aAncestor) {
    return ctm;
  }
  ctm = aFrame->GetTransformMatrix(aAncestor, &parent);
  while (parent && parent != aAncestor) {
    if (!parent->Preserves3DChildren()) {
      ctm.ProjectTo2D();
    }
    ctm = ctm * parent->GetTransformMatrix(aAncestor, &parent);
  }
  return ctm;
}

gfxSize
nsLayoutUtils::GetTransformToAncestorScale(nsIFrame* aFrame)
{
  Matrix4x4 transform = GetTransformToAncestor(aFrame,
      nsLayoutUtils::GetDisplayRootFrame(aFrame));
  Matrix transform2D;
  if (transform.Is2D(&transform2D)) {
    return ThebesMatrix(transform2D).ScaleFactors(true);
  }
  return gfxSize(1, 1);
}


nsIFrame*
nsLayoutUtils::FindNearestCommonAncestorFrame(nsIFrame* aFrame1, nsIFrame* aFrame2)
{
  nsAutoTArray<nsIFrame*,100> ancestors1;
  nsAutoTArray<nsIFrame*,100> ancestors2;
  nsIFrame* commonAncestor = nullptr;
  if (aFrame1->PresContext() == aFrame2->PresContext()) {
    commonAncestor = aFrame1->PresContext()->PresShell()->GetRootFrame();
  }
  for (nsIFrame* f = aFrame1; f != commonAncestor;
       f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    ancestors1.AppendElement(f);
  }
  for (nsIFrame* f = aFrame2; f != commonAncestor;
       f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    ancestors2.AppendElement(f);
  }
  uint32_t minLengths = std::min(ancestors1.Length(), ancestors2.Length());
  for (uint32_t i = 1; i <= minLengths; ++i) {
    if (ancestors1[ancestors1.Length() - i] == ancestors2[ancestors2.Length() - i]) {
      commonAncestor = ancestors1[ancestors1.Length() - i];
    } else {
      break;
    }
  }
  return commonAncestor;
}

nsLayoutUtils::TransformResult
nsLayoutUtils::TransformPoints(nsIFrame* aFromFrame, nsIFrame* aToFrame,
                               uint32_t aPointCount, CSSPoint* aPoints)
{
  nsIFrame* nearestCommonAncestor = FindNearestCommonAncestorFrame(aFromFrame, aToFrame);
  if (!nearestCommonAncestor) {
    return NO_COMMON_ANCESTOR;
  }
  Matrix4x4 downToDest = GetTransformToAncestor(aToFrame, nearestCommonAncestor);
  if (downToDest.IsSingular()) {
    return NONINVERTIBLE_TRANSFORM;
  }
  downToDest.Invert();
  Matrix4x4 upToAncestor = GetTransformToAncestor(aFromFrame, nearestCommonAncestor);
  CSSToLayoutDeviceScale devPixelsPerCSSPixelFromFrame(
    double(nsPresContext::AppUnitsPerCSSPixel())/
      aFromFrame->PresContext()->AppUnitsPerDevPixel());
  CSSToLayoutDeviceScale devPixelsPerCSSPixelToFrame(
    double(nsPresContext::AppUnitsPerCSSPixel())/
      aToFrame->PresContext()->AppUnitsPerDevPixel());
  for (uint32_t i = 0; i < aPointCount; ++i) {
    LayoutDevicePoint devPixels = aPoints[i] * devPixelsPerCSSPixelFromFrame;
    
    
    Point toDevPixels = downToDest.ProjectPoint(
        (upToAncestor * Point(devPixels.x, devPixels.y))).As2DPoint();
    
    
    aPoints[i] = LayoutDevicePoint(toDevPixels.x, toDevPixels.y) /
        devPixelsPerCSSPixelToFrame;
  }
  return TRANSFORM_SUCCEEDED;
}

nsLayoutUtils::TransformResult
nsLayoutUtils::TransformPoint(nsIFrame* aFromFrame, nsIFrame* aToFrame,
                              nsPoint& aPoint)
{
  nsIFrame* nearestCommonAncestor = FindNearestCommonAncestorFrame(aFromFrame, aToFrame);
  if (!nearestCommonAncestor) {
    return NO_COMMON_ANCESTOR;
  }
  Matrix4x4 downToDest = GetTransformToAncestor(aToFrame, nearestCommonAncestor);
  if (downToDest.IsSingular()) {
    return NONINVERTIBLE_TRANSFORM;
  }
  downToDest.Invert();
  Matrix4x4 upToAncestor = GetTransformToAncestor(aFromFrame, nearestCommonAncestor);

  float devPixelsPerAppUnitFromFrame =
    1.0f / aFromFrame->PresContext()->AppUnitsPerDevPixel();
  float devPixelsPerAppUnitToFrame =
    1.0f / aToFrame->PresContext()->AppUnitsPerDevPixel();
  Point4D toDevPixels = downToDest.ProjectPoint(
      upToAncestor * Point(aPoint.x * devPixelsPerAppUnitFromFrame,
                           aPoint.y * devPixelsPerAppUnitFromFrame));
  if (!toDevPixels.HasPositiveWCoord()) {
    
    
    return NONINVERTIBLE_TRANSFORM;
  }
  aPoint.x = NSToCoordRound(toDevPixels.x / devPixelsPerAppUnitToFrame);
  aPoint.y = NSToCoordRound(toDevPixels.y / devPixelsPerAppUnitToFrame);
  return TRANSFORM_SUCCEEDED;
}

nsLayoutUtils::TransformResult
nsLayoutUtils::TransformRect(nsIFrame* aFromFrame, nsIFrame* aToFrame,
                             nsRect& aRect)
{
  nsIFrame* nearestCommonAncestor = FindNearestCommonAncestorFrame(aFromFrame, aToFrame);
  if (!nearestCommonAncestor) {
    return NO_COMMON_ANCESTOR;
  }
  Matrix4x4 downToDest = GetTransformToAncestor(aToFrame, nearestCommonAncestor);
  if (downToDest.IsSingular()) {
    return NONINVERTIBLE_TRANSFORM;
  }
  downToDest.Invert();
  Matrix4x4 upToAncestor = GetTransformToAncestor(aFromFrame, nearestCommonAncestor);

  float devPixelsPerAppUnitFromFrame =
    1.0f / aFromFrame->PresContext()->AppUnitsPerDevPixel();
  float devPixelsPerAppUnitToFrame =
    1.0f / aToFrame->PresContext()->AppUnitsPerDevPixel();
  gfx::Rect toDevPixels = downToDest.ProjectRectBounds(
    upToAncestor.ProjectRectBounds(
      gfx::Rect(aRect.x * devPixelsPerAppUnitFromFrame,
                aRect.y * devPixelsPerAppUnitFromFrame,
                aRect.width * devPixelsPerAppUnitFromFrame,
                aRect.height * devPixelsPerAppUnitFromFrame),
      Rect(-std::numeric_limits<Float>::max() * 0.5f,
           -std::numeric_limits<Float>::max() * 0.5f,
           std::numeric_limits<Float>::max(),
           std::numeric_limits<Float>::max())),
    Rect(-std::numeric_limits<Float>::max() * devPixelsPerAppUnitFromFrame * 0.5f,
         -std::numeric_limits<Float>::max() * devPixelsPerAppUnitFromFrame * 0.5f,
         std::numeric_limits<Float>::max() * devPixelsPerAppUnitFromFrame,
         std::numeric_limits<Float>::max() * devPixelsPerAppUnitFromFrame));
  aRect.x = toDevPixels.x / devPixelsPerAppUnitToFrame;
  aRect.y = toDevPixels.y / devPixelsPerAppUnitToFrame;
  aRect.width = toDevPixels.width / devPixelsPerAppUnitToFrame;
  aRect.height = toDevPixels.height / devPixelsPerAppUnitToFrame;
  return TRANSFORM_SUCCEEDED;
}

nsRect
nsLayoutUtils::GetRectRelativeToFrame(Element* aElement, nsIFrame* aFrame)
{
  if (!aElement || !aFrame) {
    return nsRect();
  }

  nsIFrame* frame = aElement->GetPrimaryFrame();
  if (!frame) {
    return nsRect();
  }

  nsRect rect = frame->GetRectRelativeToSelf();
  nsLayoutUtils::TransformResult rv =
    nsLayoutUtils::TransformRect(frame, aFrame, rect);
  if (rv != nsLayoutUtils::TRANSFORM_SUCCEEDED) {
    return nsRect();
  }

  return rect;
}

bool
nsLayoutUtils::ContainsPoint(const nsRect& aRect, const nsPoint& aPoint,
                             nscoord aInflateSize)
{
  nsRect rect = aRect;
  rect.Inflate(aInflateSize);
  return rect.Contains(aPoint);
}

bool
nsLayoutUtils::IsRectVisibleInScrollFrames(nsIFrame* aFrame, const nsRect& aRect)
{
  nsIFrame* closestScrollFrame =
    nsLayoutUtils::GetClosestFrameOfType(aFrame, nsGkAtoms::scrollFrame);

  while (closestScrollFrame) {
    nsIScrollableFrame* sf = do_QueryFrame(closestScrollFrame);
    nsRect scrollPortRect = sf->GetScrollPortRect();

    nsRect rectRelativeToScrollFrame = aRect;
    nsLayoutUtils::TransformRect(aFrame, closestScrollFrame,
                                 rectRelativeToScrollFrame);

    
    if (!scrollPortRect.Intersects(rectRelativeToScrollFrame)) {
      return false;
    }

    
    closestScrollFrame =
      nsLayoutUtils::GetClosestFrameOfType(closestScrollFrame->GetParent(),
                                           nsGkAtoms::scrollFrame);
  }

  return true;
}

bool
nsLayoutUtils::GetLayerTransformForFrame(nsIFrame* aFrame,
                                         Matrix4x4* aTransform)
{
  
  
  if (aFrame->Preserves3DChildren() || aFrame->HasTransformGetter()) {
    return false;
  }

  nsIFrame* root = nsLayoutUtils::GetDisplayRootFrame(aFrame);
  if (root->HasAnyStateBits(NS_FRAME_UPDATE_LAYER_TREE)) {
    
    
    return false;
  }
  
  
  if (!aTransform) {
    return true;
  }

  nsDisplayListBuilder builder(root, nsDisplayListBuilder::OTHER,
                               false);
  nsDisplayList list;  
  nsDisplayTransform* item =
    new (&builder) nsDisplayTransform(&builder, aFrame, &list, nsRect());

  *aTransform = item->GetTransform();
  item->~nsDisplayTransform();

  return true;
}

static bool
TransformGfxPointFromAncestor(nsIFrame *aFrame,
                              const Point &aPoint,
                              nsIFrame *aAncestor,
                              Point* aOut)
{
  Matrix4x4 ctm = nsLayoutUtils::GetTransformToAncestor(aFrame, aAncestor);
  ctm.Invert();
  Point4D point = ctm.ProjectPoint(aPoint);
  if (!point.HasPositiveWCoord()) {
    return false;
  }
  *aOut = point.As2DPoint();
  return true;
}

static Rect
TransformGfxRectToAncestor(nsIFrame *aFrame,
                           const Rect &aRect,
                           const nsIFrame *aAncestor,
                           bool* aPreservesAxisAlignedRectangles = nullptr)
{
  Matrix4x4 ctm = nsLayoutUtils::GetTransformToAncestor(aFrame, aAncestor);
  if (aPreservesAxisAlignedRectangles) {
    Matrix matrix2d;
    *aPreservesAxisAlignedRectangles =
      ctm.Is2D(&matrix2d) && matrix2d.PreservesAxisAlignedRectangles();
  }
  return ctm.TransformBounds(aRect);
}

static SVGTextFrame*
GetContainingSVGTextFrame(nsIFrame* aFrame)
{
  if (!aFrame->IsSVGText()) {
    return nullptr;
  }

  return static_cast<SVGTextFrame*>
    (nsLayoutUtils::GetClosestFrameOfType(aFrame->GetParent(),
                                          nsGkAtoms::svgTextFrame));
}

nsPoint
nsLayoutUtils::TransformAncestorPointToFrame(nsIFrame* aFrame,
                                             const nsPoint& aPoint,
                                             nsIFrame* aAncestor)
{
    SVGTextFrame* text = GetContainingSVGTextFrame(aFrame);

    float factor = aFrame->PresContext()->AppUnitsPerDevPixel();
    Point result(NSAppUnitsToFloatPixels(aPoint.x, factor),
                 NSAppUnitsToFloatPixels(aPoint.y, factor));

    if (text) {
        if (!TransformGfxPointFromAncestor(text, result, aAncestor, &result)) {
            return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
        }
        result = text->TransformFramePointToTextChild(result, aFrame);
    } else {
        if (!TransformGfxPointFromAncestor(aFrame, result, nullptr, &result)) {
            return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
        }
    }

    return nsPoint(NSFloatPixelsToAppUnits(float(result.x), factor),
                   NSFloatPixelsToAppUnits(float(result.y), factor));
}

nsRect
nsLayoutUtils::TransformFrameRectToAncestor(nsIFrame* aFrame,
                                            const nsRect& aRect,
                                            const nsIFrame* aAncestor,
                                            bool* aPreservesAxisAlignedRectangles )
{
  SVGTextFrame* text = GetContainingSVGTextFrame(aFrame);

  float srcAppUnitsPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  Rect result;

  if (text) {
    result = ToRect(text->TransformFrameRectFromTextChild(aRect, aFrame));
    result = TransformGfxRectToAncestor(text, result, aAncestor);
    
    
    if (aPreservesAxisAlignedRectangles)
      *aPreservesAxisAlignedRectangles = false;
  } else {
    result = Rect(NSAppUnitsToFloatPixels(aRect.x, srcAppUnitsPerDevPixel),
                  NSAppUnitsToFloatPixels(aRect.y, srcAppUnitsPerDevPixel),
                  NSAppUnitsToFloatPixels(aRect.width, srcAppUnitsPerDevPixel),
                  NSAppUnitsToFloatPixels(aRect.height, srcAppUnitsPerDevPixel));
    result = TransformGfxRectToAncestor(aFrame, result, aAncestor, aPreservesAxisAlignedRectangles);
  }

  float destAppUnitsPerDevPixel = aAncestor->PresContext()->AppUnitsPerDevPixel();
  return nsRect(NSFloatPixelsToAppUnits(float(result.x), destAppUnitsPerDevPixel),
                NSFloatPixelsToAppUnits(float(result.y), destAppUnitsPerDevPixel),
                NSFloatPixelsToAppUnits(float(result.width), destAppUnitsPerDevPixel),
                NSFloatPixelsToAppUnits(float(result.height), destAppUnitsPerDevPixel));
}

static LayoutDeviceIntPoint GetWidgetOffset(nsIWidget* aWidget, nsIWidget*& aRootWidget) {
  LayoutDeviceIntPoint offset(0, 0);
  while ((aWidget->WindowType() == eWindowType_child ||
          aWidget->IsPlugin())) {
    nsIWidget* parent = aWidget->GetParent();
    if (!parent) {
      break;
    }
    nsIntRect bounds;
    aWidget->GetBounds(bounds);
    offset += LayoutDeviceIntPoint::FromUntyped(bounds.TopLeft());
    aWidget = parent;
  }
  aRootWidget = aWidget;
  return offset;
}

static LayoutDeviceIntPoint
WidgetToWidgetOffset(nsIWidget* aFrom, nsIWidget* aTo) {
  nsIWidget* fromRoot;
  LayoutDeviceIntPoint fromOffset = GetWidgetOffset(aFrom, fromRoot);
  nsIWidget* toRoot;
  LayoutDeviceIntPoint toOffset = GetWidgetOffset(aTo, toRoot);

  if (fromRoot == toRoot) {
    return fromOffset - toOffset;
  }
  return aFrom->WidgetToScreenOffset() - aTo->WidgetToScreenOffset();
}

nsPoint
nsLayoutUtils::TranslateWidgetToView(nsPresContext* aPresContext,
                                     nsIWidget* aWidget, const LayoutDeviceIntPoint& aPt,
                                     nsView* aView)
{
  nsPoint viewOffset;
  nsIWidget* viewWidget = aView->GetNearestWidget(&viewOffset);
  if (!viewWidget) {
    return nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  LayoutDeviceIntPoint widgetPoint = aPt + WidgetToWidgetOffset(aWidget, viewWidget);
  nsPoint widgetAppUnits(aPresContext->DevPixelsToAppUnits(widgetPoint.x),
                         aPresContext->DevPixelsToAppUnits(widgetPoint.y));
  return widgetAppUnits - viewOffset;
}

LayoutDeviceIntPoint
nsLayoutUtils::TranslateViewToWidget(nsPresContext* aPresContext,
                                     nsView* aView, nsPoint aPt,
                                     nsIWidget* aWidget)
{
  nsPoint viewOffset;
  nsIWidget* viewWidget = aView->GetNearestWidget(&viewOffset);
  if (!viewWidget) {
    return LayoutDeviceIntPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }

  LayoutDeviceIntPoint relativeToViewWidget(aPresContext->AppUnitsToDevPixels(aPt.x + viewOffset.x),
                                            aPresContext->AppUnitsToDevPixels(aPt.y + viewOffset.y));
  return relativeToViewWidget + WidgetToWidgetOffset(viewWidget, aWidget);
}



uint8_t
nsLayoutUtils::CombineBreakType(uint8_t aOrigBreakType,
                                uint8_t aNewBreakType)
{
  uint8_t breakType = aOrigBreakType;
  switch(breakType) {
  case NS_STYLE_CLEAR_LEFT:
    if (NS_STYLE_CLEAR_RIGHT == aNewBreakType ||
        NS_STYLE_CLEAR_BOTH == aNewBreakType) {
      breakType = NS_STYLE_CLEAR_BOTH;
    }
    break;
  case NS_STYLE_CLEAR_RIGHT:
    if (NS_STYLE_CLEAR_LEFT == aNewBreakType ||
        NS_STYLE_CLEAR_BOTH == aNewBreakType) {
      breakType = NS_STYLE_CLEAR_BOTH;
    }
    break;
  case NS_STYLE_CLEAR_NONE:
    if (NS_STYLE_CLEAR_LEFT == aNewBreakType ||
        NS_STYLE_CLEAR_RIGHT == aNewBreakType ||
        NS_STYLE_CLEAR_BOTH == aNewBreakType) {
      breakType = aNewBreakType;
    }
  }
  return breakType;
}

#ifdef MOZ_DUMP_PAINTING
#include <stdio.h>

static bool gDumpEventList = false;
int gPaintCount = 0;
#endif

nsIFrame*
nsLayoutUtils::GetFrameForPoint(nsIFrame* aFrame, nsPoint aPt, uint32_t aFlags)
{
  PROFILER_LABEL("nsLayoutUtils", "GetFrameForPoint",
    js::ProfileEntry::Category::GRAPHICS);

  nsresult rv;
  nsAutoTArray<nsIFrame*,8> outFrames;
  rv = GetFramesForArea(aFrame, nsRect(aPt, nsSize(1, 1)), outFrames, aFlags);
  NS_ENSURE_SUCCESS(rv, nullptr);
  return outFrames.Length() ? outFrames.ElementAt(0) : nullptr;
}

nsresult
nsLayoutUtils::GetFramesForArea(nsIFrame* aFrame, const nsRect& aRect,
                                nsTArray<nsIFrame*> &aOutFrames,
                                uint32_t aFlags)
{
  PROFILER_LABEL("nsLayoutUtils", "GetFramesForArea",
    js::ProfileEntry::Category::GRAPHICS);

  nsDisplayListBuilder builder(aFrame, nsDisplayListBuilder::EVENT_DELIVERY,
                               false);
  nsDisplayList list;

  if (aFlags & IGNORE_PAINT_SUPPRESSION) {
    builder.IgnorePaintSuppression();
  }

  if (aFlags & IGNORE_ROOT_SCROLL_FRAME) {
    nsIFrame* rootScrollFrame =
      aFrame->PresContext()->PresShell()->GetRootScrollFrame();
    if (rootScrollFrame) {
      builder.SetIgnoreScrollFrame(rootScrollFrame);
    }
  }
  if (aFlags & IGNORE_CROSS_DOC) {
    builder.SetDescendIntoSubdocuments(false);
  }

  builder.EnterPresShell(aFrame);
  aFrame->BuildDisplayListForStackingContext(&builder, aRect, &list);
  builder.LeavePresShell(aFrame);

#ifdef MOZ_DUMP_PAINTING
  if (gDumpEventList) {
    fprintf_stderr(stderr, "Event handling --- (%d,%d):\n", aRect.x, aRect.y);

    std::stringstream ss;
    nsFrame::PrintDisplayList(&builder, list, ss);
    print_stderr(ss);
  }
#endif

  nsDisplayItem::HitTestState hitTestState;
  list.HitTest(&builder, aRect, &hitTestState, &aOutFrames);
  list.DeleteAll();
  return NS_OK;
}


static FrameMetrics
CalculateFrameMetricsForDisplayPort(nsIScrollableFrame* aScrollFrame) {
  nsIFrame* frame = do_QueryFrame(aScrollFrame);
  MOZ_ASSERT(frame);

  
  
  
  FrameMetrics metrics;
  nsPresContext* presContext = frame->PresContext();
  nsIPresShell* presShell = presContext->PresShell();
  CSSToLayoutDeviceScale deviceScale(float(nsPresContext::AppUnitsPerCSSPixel())
                                     / presContext->AppUnitsPerDevPixel());
  float resolution = 1.0f;
  if (frame == presShell->GetRootScrollFrame()) {
    
    
    resolution = presShell->GetResolution();
  }
  
  
  
  
  
  
  LayoutDeviceToLayerScale2D cumulativeResolution(
      presShell->GetCumulativeResolution()
    * nsLayoutUtils::GetTransformToAncestorScale(frame));

  LayerToParentLayerScale layerToParentLayerScale(1.0f);
  metrics.SetDevPixelsPerCSSPixel(deviceScale);
  metrics.SetPresShellResolution(resolution);
  metrics.SetCumulativeResolution(cumulativeResolution);
  metrics.SetZoom(deviceScale * cumulativeResolution * layerToParentLayerScale);

  
  
  nsSize compositionSize = nsLayoutUtils::CalculateCompositionSizeForFrame(frame);
  LayoutDeviceToParentLayerScale2D compBoundsScale;
  if (frame == presShell->GetRootScrollFrame() && presContext->IsRootContentDocument()) {
    if (presContext->GetParentPresContext()) {
      float res = presContext->GetParentPresContext()->PresShell()->GetCumulativeResolution();
      compBoundsScale = LayoutDeviceToParentLayerScale2D(
          LayoutDeviceToParentLayerScale(res));
    }
  } else {
    compBoundsScale = cumulativeResolution * layerToParentLayerScale;
  }
  metrics.mCompositionBounds
      = LayoutDeviceRect::FromAppUnits(nsRect(nsPoint(0, 0), compositionSize),
                                       presContext->AppUnitsPerDevPixel())
      * compBoundsScale;

  metrics.SetRootCompositionSize(
      nsLayoutUtils::CalculateRootCompositionSize(frame, false, metrics));

  metrics.SetScrollOffset(CSSPoint::FromAppUnits(
      aScrollFrame->GetScrollPosition()));

  metrics.SetScrollableRect(CSSRect::FromAppUnits(
      nsLayoutUtils::CalculateScrollableRectForFrame(aScrollFrame, nullptr)));

  return metrics;
}

bool
nsLayoutUtils::CalculateAndSetDisplayPortMargins(nsIScrollableFrame* aScrollFrame,
                                                 RepaintMode aRepaintMode) {
  nsIFrame* frame = do_QueryFrame(aScrollFrame);
  MOZ_ASSERT(frame);
  nsIContent* content = frame->GetContent();
  MOZ_ASSERT(content);

  FrameMetrics metrics = CalculateFrameMetricsForDisplayPort(aScrollFrame);
  ScreenMargin displayportMargins = APZCTreeManager::CalculatePendingDisplayPort(
      metrics, ParentLayerPoint(0.0f, 0.0f), 0.0);
  nsIPresShell* presShell = frame->PresContext()->GetPresShell();
  return nsLayoutUtils::SetDisplayPortMargins(
      content, presShell, displayportMargins, 0, aRepaintMode);
}

bool
nsLayoutUtils::GetOrMaybeCreateDisplayPort(nsDisplayListBuilder& aBuilder,
                                           nsIFrame* aScrollFrame,
                                           nsRect aDisplayPortBase,
                                           nsRect* aOutDisplayport) {
  nsIContent* content = aScrollFrame->GetContent();
  nsIScrollableFrame* scrollableFrame = do_QueryFrame(aScrollFrame);
  if (!content || !scrollableFrame) {
    return false;
  }

  
  
  
  SetDisplayPortBase(content, aDisplayPortBase);

  bool haveDisplayPort = GetDisplayPort(content, aOutDisplayport);

  
  
  
  
  
  
  if (aBuilder.IsPaintingToWindow() &&
      gfxPrefs::AsyncPanZoomEnabled() &&
      !aBuilder.HaveScrollableDisplayPort() &&
      scrollableFrame->WantAsyncScroll()) {

    
    if (!haveDisplayPort) {
      CalculateAndSetDisplayPortMargins(scrollableFrame, nsLayoutUtils::RepaintMode::DoNotRepaint);
      haveDisplayPort = GetDisplayPort(content, aOutDisplayport);
      NS_ASSERTION(haveDisplayPort, "should have a displayport after having just set it");
    }

    
    aBuilder.SetHaveScrollableDisplayPort();
  }

  return haveDisplayPort;
}

nsresult
nsLayoutUtils::PaintFrame(nsRenderingContext* aRenderingContext, nsIFrame* aFrame,
                          const nsRegion& aDirtyRegion, nscolor aBackstop,
                          uint32_t aFlags)
{
  PROFILER_LABEL("nsLayoutUtils", "PaintFrame",
    js::ProfileEntry::Category::GRAPHICS);

  if (aFlags & PAINT_WIDGET_LAYERS) {
    nsView* view = aFrame->GetView();
    if (!(view && view->GetWidget() && GetDisplayRootFrame(aFrame) == aFrame)) {
      aFlags &= ~PAINT_WIDGET_LAYERS;
      NS_ASSERTION(aRenderingContext, "need a rendering context");
    }
  }

  nsPresContext* presContext = aFrame->PresContext();
  nsIPresShell* presShell = presContext->PresShell();
  nsRootPresContext* rootPresContext = presContext->GetRootPresContext();
  if (!rootPresContext) {
    return NS_OK;
  }

  TimeStamp startBuildDisplayList = TimeStamp::Now();
  nsDisplayListBuilder builder(aFrame, nsDisplayListBuilder::PAINTING,
                           !(aFlags & PAINT_HIDE_CARET));

  nsIFrame* rootScrollFrame = presShell->GetRootScrollFrame();
  bool usingDisplayPort = false;
  nsRect displayport;
  if (rootScrollFrame && !aFrame->GetParent() &&
      (aFlags & (PAINT_WIDGET_LAYERS | PAINT_TO_WINDOW))) {
    nsRect displayportBase(
        nsPoint(0,0),
        nsLayoutUtils::CalculateCompositionSizeForFrame(rootScrollFrame));
    usingDisplayPort = nsLayoutUtils::GetOrMaybeCreateDisplayPort(
        builder, rootScrollFrame, displayportBase, &displayport);
  }

  nsRegion visibleRegion;
  if (aFlags & PAINT_WIDGET_LAYERS) {
    
    
    
    
    
    
    if (!usingDisplayPort) {
      visibleRegion = aFrame->GetVisualOverflowRectRelativeToSelf();
    } else {
      visibleRegion = displayport;
    }
  } else {
    visibleRegion = aDirtyRegion;
  }

  
  
  
  bool willFlushRetainedLayers = (aFlags & PAINT_HIDE_CARET) != 0;

  nsDisplayList list;
  if (aFlags & PAINT_IN_TRANSFORM) {
    builder.SetInTransform(true);
  }
  if (aFlags & PAINT_SYNC_DECODE_IMAGES) {
    builder.SetSyncDecodeImages(true);
  }
  if (aFlags & (PAINT_WIDGET_LAYERS | PAINT_TO_WINDOW)) {
    builder.SetPaintingToWindow(true);
  }
  if (aFlags & PAINT_IGNORE_SUPPRESSION) {
    builder.IgnorePaintSuppression();
  }

  
  
  if ((aFlags & PAINT_WIDGET_LAYERS) &&
      !willFlushRetainedLayers &&
      !(aFlags & PAINT_DOCUMENT_RELATIVE) &&
      rootPresContext->NeedToComputePluginGeometryUpdates()) {
    builder.SetWillComputePluginGeometry(true);
  }

  nsRect canvasArea(nsPoint(0, 0), aFrame->GetSize());
  bool ignoreViewportScrolling =
    aFrame->GetParent() ? false : presShell->IgnoringViewportScrolling();
  if (ignoreViewportScrolling && rootScrollFrame) {
    nsIScrollableFrame* rootScrollableFrame =
      presShell->GetRootScrollFrameAsScrollable();
    if (aFlags & PAINT_DOCUMENT_RELATIVE) {
      
      
      nsPoint pos = rootScrollableFrame->GetScrollPosition();
      visibleRegion.MoveBy(-pos);
      if (aRenderingContext) {
        gfxPoint devPixelOffset =
          nsLayoutUtils::PointToGfxPoint(pos,
                                         presContext->AppUnitsPerDevPixel());
        aRenderingContext->ThebesContext()->SetMatrix(
          aRenderingContext->ThebesContext()->CurrentMatrix().Translate(devPixelOffset));
      }
    }
    builder.SetIgnoreScrollFrame(rootScrollFrame);

    nsCanvasFrame* canvasFrame =
      do_QueryFrame(rootScrollableFrame->GetScrolledFrame());
    if (canvasFrame) {
      
      
      canvasArea.UnionRect(canvasArea,
        canvasFrame->CanvasArea() + builder.ToReferenceFrame(canvasFrame));
    }
  }

  builder.EnterPresShell(aFrame);
  nsRect dirtyRect = visibleRegion.GetBounds();
  {
    
    
    
    
    
    ViewID id = FrameMetrics::NULL_SCROLL_ID;
    if (ignoreViewportScrolling && presContext->IsRootContentDocument()) {
      if (nsIFrame* rootScrollFrame = presShell->GetRootScrollFrame()) {
        if (nsIContent* content = rootScrollFrame->GetContent()) {
          id = nsLayoutUtils::FindOrCreateIDFor(content);
        }
      }
    }
    nsDisplayListBuilder::AutoCurrentScrollParentIdSetter idSetter(&builder, id);

    PROFILER_LABEL("nsLayoutUtils", "PaintFrame::BuildDisplayList",
      js::ProfileEntry::Category::GRAPHICS);

    aFrame->BuildDisplayListForStackingContext(&builder, dirtyRect, &list);
  }
  const bool paintAllContinuations = aFlags & PAINT_ALL_CONTINUATIONS;
  NS_ASSERTION(!paintAllContinuations || !aFrame->GetPrevContinuation(),
               "If painting all continuations, the frame must be "
               "first-continuation");

  nsIAtom* frameType = aFrame->GetType();

  if (paintAllContinuations) {
    nsIFrame* currentFrame = aFrame;
    while ((currentFrame = currentFrame->GetNextContinuation()) != nullptr) {
      PROFILER_LABEL("nsLayoutUtils", "PaintFrame::ContinuationsBuildDisplayList",
        js::ProfileEntry::Category::GRAPHICS);

      nsRect frameDirty = dirtyRect - builder.ToReferenceFrame(currentFrame);
      currentFrame->BuildDisplayListForStackingContext(&builder,
                                                       frameDirty, &list);
    }
  }

  
  
  if (frameType == nsGkAtoms::viewportFrame && 
      nsLayoutUtils::NeedsPrintPreviewBackground(presContext)) {
    nsRect bounds = nsRect(builder.ToReferenceFrame(aFrame),
                           aFrame->GetSize());
    nsDisplayListBuilder::AutoBuildingDisplayList
      buildingDisplayList(&builder, aFrame, bounds, false);
    presShell->AddPrintPreviewBackgroundItem(builder, list, aFrame, bounds);
  } else if (frameType != nsGkAtoms::pageFrame) {
    
    
    
    
    

    
    
    
    canvasArea.IntersectRect(canvasArea, visibleRegion.GetBounds());
    nsDisplayListBuilder::AutoBuildingDisplayList
      buildingDisplayList(&builder, aFrame, canvasArea, false);
    presShell->AddCanvasBackgroundColorItem(
           builder, list, aFrame, canvasArea, aBackstop);

    
    
    if ((aFlags & PAINT_WIDGET_LAYERS) && !willFlushRetainedLayers) {
      nsView* view = aFrame->GetView();
      if (view) {
        nscolor backstop = presShell->ComputeBackstopColor(view);
        
        
        nscolor canvasColor = presShell->GetCanvasBackground();
        if (NS_ComposeColors(aBackstop, canvasColor) !=
            NS_ComposeColors(backstop, canvasColor)) {
          willFlushRetainedLayers = true;
        }
      }
    }
  }

  builder.LeavePresShell(aFrame);
  Telemetry::AccumulateTimeDelta(Telemetry::PAINT_BUILD_DISPLAYLIST_TIME,
                                 startBuildDisplayList);

  if (builder.GetHadToIgnorePaintSuppression()) {
    willFlushRetainedLayers = true;
  }


  bool profilerNeedsDisplayList = profiler_feature_active("displaylistdump");
  bool consoleNeedsDisplayList = gfxUtils::DumpDisplayList() || gfxUtils::sDumpPainting;
#ifdef MOZ_DUMP_PAINTING
  FILE* savedDumpFile = gfxUtils::sDumpPaintFile;
#endif

  UniquePtr<std::stringstream> ss;
  if (consoleNeedsDisplayList || profilerNeedsDisplayList) {
    ss = MakeUnique<std::stringstream>();
#ifdef MOZ_DUMP_PAINTING
    if (gfxUtils::sDumpPaintingToFile) {
      nsCString string("dump-");
      string.AppendInt(gPaintCount);
      string.AppendLiteral(".html");
      gfxUtils::sDumpPaintFile = fopen(string.BeginReading(), "w");
    } else {
      gfxUtils::sDumpPaintFile = stderr;
    }
    if (gfxUtils::sDumpPaintingToFile) {
      *ss << "<html><head><script>var array = {}; function ViewImage(index) { window.location = array[index]; }</script></head><body>";
    }
#endif
    *ss << nsPrintfCString("Painting --- before optimization (dirty %d,%d,%d,%d):\n",
            dirtyRect.x, dirtyRect.y, dirtyRect.width, dirtyRect.height).get();
    nsFrame::PrintDisplayList(&builder, list, *ss, gfxUtils::sDumpPaintingToFile);
    if (gfxUtils::sDumpPaintingToFile) {
      *ss << "<script>";
    } else {
      
      
      if (profilerNeedsDisplayList && !consoleNeedsDisplayList) {
        profiler_log(ss->str().c_str());
      } else {
        
        fprint_stderr(gfxUtils::sDumpPaintFile, *ss);
      }
      ss = MakeUnique<std::stringstream>();
    }
  }

  uint32_t flags = nsDisplayList::PAINT_DEFAULT;
  if (aFlags & PAINT_WIDGET_LAYERS) {
    flags |= nsDisplayList::PAINT_USE_WIDGET_LAYERS;
    if (willFlushRetainedLayers) {
      
      
      
      
      
      
      NS_WARNING("Flushing retained layers!");
      flags |= nsDisplayList::PAINT_FLUSH_LAYERS;
    } else if (!(aFlags & PAINT_DOCUMENT_RELATIVE)) {
      nsIWidget *widget = aFrame->GetNearestWidget();
      if (widget) {
        
        
        widget->UpdateThemeGeometries(builder.GetThemeGeometries());
      }
    }
  }
  if (aFlags & PAINT_EXISTING_TRANSACTION) {
    flags |= nsDisplayList::PAINT_EXISTING_TRANSACTION;
  }
  if (aFlags & PAINT_NO_COMPOSITE) {
    flags |= nsDisplayList::PAINT_NO_COMPOSITE;
  }
  if (aFlags & PAINT_COMPRESSED) {
    flags |= nsDisplayList::PAINT_COMPRESSED;
  }

  TimeStamp paintStart = TimeStamp::Now();
  nsRefPtr<LayerManager> layerManager =
    list.PaintRoot(&builder, aRenderingContext, flags);
  Telemetry::AccumulateTimeDelta(Telemetry::PAINT_RASTERIZE_TIME,
                                 paintStart);

  if (consoleNeedsDisplayList || profilerNeedsDisplayList) {
#ifdef MOZ_DUMP_PAINTING
    if (gfxUtils::sDumpPaintingToFile) {
      *ss << "</script>";
    }
#endif
    *ss << "Painting --- after optimization:\n";
    nsFrame::PrintDisplayList(&builder, list, *ss, gfxUtils::sDumpPaintingToFile);

    *ss << "Painting --- layer tree:\n";
    if (layerManager) {
      FrameLayerBuilder::DumpRetainedLayerTree(layerManager, *ss,
                                               gfxUtils::sDumpPaintingToFile);
    }

    if (profilerNeedsDisplayList && !consoleNeedsDisplayList) {
      profiler_log(ss->str().c_str());
    } else {
      
      fprint_stderr(gfxUtils::sDumpPaintFile, *ss);
    }

#ifdef MOZ_DUMP_PAINTING
    if (gfxUtils::sDumpPaintingToFile) {
      *ss << "</body></html>";
    }
    if (gfxUtils::sDumpPaintingToFile) {
      fclose(gfxUtils::sDumpPaintFile);
    }
    gfxUtils::sDumpPaintFile = savedDumpFile;
    gPaintCount++;
#endif
  }

#ifdef MOZ_DUMP_PAINTING
  if (gfxPrefs::DumpClientLayers()) {
    std::stringstream ss;
    FrameLayerBuilder::DumpRetainedLayerTree(layerManager, ss, false);
    print_stderr(ss);
  }
#endif

  
  
  
  if ((aFlags & PAINT_WIDGET_LAYERS) &&
      !willFlushRetainedLayers &&
      !(aFlags & PAINT_DOCUMENT_RELATIVE)) {
    nsIWidget *widget = aFrame->GetNearestWidget();
    if (widget) {
      nsRegion opaqueRegion;
      opaqueRegion.And(builder.GetWindowExcludeGlassRegion(), builder.GetWindowOpaqueRegion());
      widget->UpdateOpaqueRegion(
        opaqueRegion.ToNearestPixels(presContext->AppUnitsPerDevPixel()));

      const nsIntRegion& draggingRegion = builder.GetWindowDraggingRegion();
      widget->UpdateWindowDraggingRegion(draggingRegion);
    }
  }

  if (builder.WillComputePluginGeometry()) {
    
    
    
    
    
    if (XRE_GetProcessType() == GeckoProcessType_Default) {
      rootPresContext->ComputePluginGeometryUpdates(aFrame, &builder, &list);
      
      
      
      
      if (layerManager && !layerManager->NeedsWidgetInvalidation()) {
        rootPresContext->ApplyPluginGeometryUpdates();
      }
    }

    
    
    
    if (layerManager) {
      layerManager->Composite();
    }
  }


  
  list.DeleteAll();
  return NS_OK;
}













bool
nsLayoutUtils::BinarySearchForPosition(nsRenderingContext* aRendContext,
                                       nsFontMetrics& aFontMetrics,
                        const char16_t* aText,
                        int32_t    aBaseWidth,
                        int32_t    aBaseInx,
                        int32_t    aStartInx,
                        int32_t    aEndInx,
                        int32_t    aCursorPos,
                        int32_t&   aIndex,
                        int32_t&   aTextWidth)
{
  int32_t range = aEndInx - aStartInx;
  if ((range == 1) || (range == 2 && NS_IS_HIGH_SURROGATE(aText[aStartInx]))) {
    aIndex   = aStartInx + aBaseInx;
    aTextWidth = nsLayoutUtils::AppUnitWidthOfString(aText, aIndex,
                                                     aFontMetrics,
                                                     *aRendContext);
    return true;
  }

  int32_t inx = aStartInx + (range / 2);

  
  if (NS_IS_HIGH_SURROGATE(aText[inx-1]))
    inx++;

  int32_t textWidth = nsLayoutUtils::AppUnitWidthOfString(aText, inx,
                                                          aFontMetrics,
                                                          *aRendContext);

  int32_t fullWidth = aBaseWidth + textWidth;
  if (fullWidth == aCursorPos) {
    aTextWidth = textWidth;
    aIndex = inx;
    return true;
  } else if (aCursorPos < fullWidth) {
    aTextWidth = aBaseWidth;
    if (BinarySearchForPosition(aRendContext, aFontMetrics, aText, aBaseWidth,
                                aBaseInx, aStartInx, inx, aCursorPos, aIndex,
                                aTextWidth)) {
      return true;
    }
  } else {
    aTextWidth = fullWidth;
    if (BinarySearchForPosition(aRendContext, aFontMetrics, aText, aBaseWidth,
                                aBaseInx, inx, aEndInx, aCursorPos, aIndex,
                                aTextWidth)) {
      return true;
    }
  }
  return false;
}

static void
AddBoxesForFrame(nsIFrame* aFrame,
                 nsLayoutUtils::BoxCallback* aCallback)
{
  nsIAtom* pseudoType = aFrame->StyleContext()->GetPseudo();

  if (pseudoType == nsCSSAnonBoxes::tableOuter) {
    AddBoxesForFrame(aFrame->GetFirstPrincipalChild(), aCallback);
    nsIFrame* kid = aFrame->GetFirstChild(nsIFrame::kCaptionList);
    if (kid) {
      AddBoxesForFrame(kid, aCallback);
    }
  } else if (pseudoType == nsCSSAnonBoxes::mozAnonymousBlock ||
             pseudoType == nsCSSAnonBoxes::mozAnonymousPositionedBlock ||
             pseudoType == nsCSSAnonBoxes::mozMathMLAnonymousBlock ||
             pseudoType == nsCSSAnonBoxes::mozXULAnonymousBlock) {
    for (nsIFrame* kid = aFrame->GetFirstPrincipalChild(); kid; kid = kid->GetNextSibling()) {
      AddBoxesForFrame(kid, aCallback);
    }
  } else {
    aCallback->AddBox(aFrame);
  }
}

void
nsLayoutUtils::GetAllInFlowBoxes(nsIFrame* aFrame, BoxCallback* aCallback)
{
  while (aFrame) {
    AddBoxesForFrame(aFrame, aCallback);
    aFrame = nsLayoutUtils::GetNextContinuationOrIBSplitSibling(aFrame);
  }
}

nsIFrame*
nsLayoutUtils::GetFirstNonAnonymousFrame(nsIFrame* aFrame)
{
  while (aFrame) {
    nsIAtom* pseudoType = aFrame->StyleContext()->GetPseudo();

    if (pseudoType == nsCSSAnonBoxes::tableOuter) {
      nsIFrame* f = GetFirstNonAnonymousFrame(aFrame->GetFirstPrincipalChild());
      if (f) {
        return f;
      }
      nsIFrame* kid = aFrame->GetFirstChild(nsIFrame::kCaptionList);
      if (kid) {
        f = GetFirstNonAnonymousFrame(kid);
        if (f) {
          return f;
        }
      }
    } else if (pseudoType == nsCSSAnonBoxes::mozAnonymousBlock ||
               pseudoType == nsCSSAnonBoxes::mozAnonymousPositionedBlock ||
               pseudoType == nsCSSAnonBoxes::mozMathMLAnonymousBlock ||
               pseudoType == nsCSSAnonBoxes::mozXULAnonymousBlock) {
      for (nsIFrame* kid = aFrame->GetFirstPrincipalChild(); kid; kid = kid->GetNextSibling()) {
        nsIFrame* f = GetFirstNonAnonymousFrame(kid);
        if (f) {
          return f;
        }
      }
    } else {
      return aFrame;
    }

    aFrame = nsLayoutUtils::GetNextContinuationOrIBSplitSibling(aFrame);
  }
  return nullptr;
}

struct BoxToRect : public nsLayoutUtils::BoxCallback {
  nsIFrame* mRelativeTo;
  nsLayoutUtils::RectCallback* mCallback;
  uint32_t mFlags;

  BoxToRect(nsIFrame* aRelativeTo, nsLayoutUtils::RectCallback* aCallback,
            uint32_t aFlags)
    : mRelativeTo(aRelativeTo), mCallback(aCallback), mFlags(aFlags) {}

  virtual void AddBox(nsIFrame* aFrame) override {
    nsRect r;
    nsIFrame* outer = nsSVGUtils::GetOuterSVGFrameAndCoveredRegion(aFrame, &r);
    if (!outer) {
      outer = aFrame;
      switch (mFlags & nsLayoutUtils::RECTS_WHICH_BOX_MASK) {
        case nsLayoutUtils::RECTS_USE_CONTENT_BOX:
          r = aFrame->GetContentRectRelativeToSelf();
          break;
        case nsLayoutUtils::RECTS_USE_PADDING_BOX:
          r = aFrame->GetPaddingRectRelativeToSelf();
          break;
        case nsLayoutUtils::RECTS_USE_MARGIN_BOX:
          r = aFrame->GetMarginRectRelativeToSelf();
          break;
        default: 
          r = aFrame->GetRectRelativeToSelf();
      }
    }
    if (mFlags & nsLayoutUtils::RECTS_ACCOUNT_FOR_TRANSFORMS) {
      r = nsLayoutUtils::TransformFrameRectToAncestor(outer, r, mRelativeTo);
    } else {
      r += outer->GetOffsetTo(mRelativeTo);
    }
    mCallback->AddRect(r);
  }
};

void
nsLayoutUtils::GetAllInFlowRects(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                 RectCallback* aCallback, uint32_t aFlags)
{
  BoxToRect converter(aRelativeTo, aCallback, aFlags);
  GetAllInFlowBoxes(aFrame, &converter);
}

nsLayoutUtils::RectAccumulator::RectAccumulator() : mSeenFirstRect(false) {}

void nsLayoutUtils::RectAccumulator::AddRect(const nsRect& aRect) {
  mResultRect.UnionRect(mResultRect, aRect);
  if (!mSeenFirstRect) {
    mSeenFirstRect = true;
    mFirstRect = aRect;
  }
}

nsLayoutUtils::RectListBuilder::RectListBuilder(DOMRectList* aList)
  : mRectList(aList)
{
}

void nsLayoutUtils::RectListBuilder::AddRect(const nsRect& aRect) {
  nsRefPtr<DOMRect> rect = new DOMRect(mRectList);

  rect->SetLayoutRect(aRect);
  mRectList->Append(rect);
}

nsIFrame* nsLayoutUtils::GetContainingBlockForClientRect(nsIFrame* aFrame)
{
  return aFrame->PresContext()->PresShell()->GetRootFrame();
}

nsRect
nsLayoutUtils::GetAllInFlowRectsUnion(nsIFrame* aFrame, nsIFrame* aRelativeTo,
                                      uint32_t aFlags) {
  RectAccumulator accumulator;
  GetAllInFlowRects(aFrame, aRelativeTo, &accumulator, aFlags);
  return accumulator.mResultRect.IsEmpty() ? accumulator.mFirstRect
          : accumulator.mResultRect;
}

nsRect
nsLayoutUtils::GetTextShadowRectsUnion(const nsRect& aTextAndDecorationsRect,
                                       nsIFrame* aFrame,
                                       uint32_t aFlags)
{
  const nsStyleText* textStyle = aFrame->StyleText();
  if (!textStyle->HasTextShadow())
    return aTextAndDecorationsRect;

  nsRect resultRect = aTextAndDecorationsRect;
  int32_t A2D = aFrame->PresContext()->AppUnitsPerDevPixel();
  for (uint32_t i = 0; i < textStyle->mTextShadow->Length(); ++i) {
    nsCSSShadowItem* shadow = textStyle->mTextShadow->ShadowAt(i);
    nsMargin blur = nsContextBoxBlur::GetBlurRadiusMargin(shadow->mRadius, A2D);
    if ((aFlags & EXCLUDE_BLUR_SHADOWS) && blur != nsMargin(0, 0, 0, 0))
      continue;

    nsRect tmpRect(aTextAndDecorationsRect);

    tmpRect.MoveBy(nsPoint(shadow->mXOffset, shadow->mYOffset));
    tmpRect.Inflate(blur);

    resultRect.UnionRect(resultRect, tmpRect);
  }
  return resultRect;
}

enum ObjectDimensionType { eWidth, eHeight };
static nscoord
ComputeMissingDimension(const nsSize& aDefaultObjectSize,
                        const nsSize& aIntrinsicRatio,
                        const Maybe<nscoord>& aSpecifiedWidth,
                        const Maybe<nscoord>& aSpecifiedHeight,
                        ObjectDimensionType aDimensionToCompute)
{
  
  

  
  
  
  if (aIntrinsicRatio.width > 0 && aIntrinsicRatio.height > 0) {
    
    nscoord knownDimensionSize;
    float ratio;
    if (aDimensionToCompute == eWidth) {
      knownDimensionSize = *aSpecifiedHeight;
      ratio = aIntrinsicRatio.width / aIntrinsicRatio.height;
    } else {
      knownDimensionSize = *aSpecifiedWidth;
      ratio = aIntrinsicRatio.height / aIntrinsicRatio.width;
    }
    return NSCoordSaturatingNonnegativeMultiply(knownDimensionSize, ratio);
  }

  
  
  
  
  

  
  
  return (aDimensionToCompute == eWidth) ?
    aDefaultObjectSize.width : aDefaultObjectSize.height;
}
























static Maybe<nsSize>
MaybeComputeObjectFitNoneSize(const nsSize& aDefaultObjectSize,
                              const IntrinsicSize& aIntrinsicSize,
                              const nsSize& aIntrinsicRatio)
{
  
  
  
  
  Maybe<nscoord> specifiedWidth;
  if (aIntrinsicSize.width.GetUnit() == eStyleUnit_Coord) {
    specifiedWidth.emplace(aIntrinsicSize.width.GetCoordValue());
  }

  Maybe<nscoord> specifiedHeight;
  if (aIntrinsicSize.height.GetUnit() == eStyleUnit_Coord) {
    specifiedHeight.emplace(aIntrinsicSize.height.GetCoordValue());
  }

  Maybe<nsSize> noneSize; 
  if (specifiedWidth || specifiedHeight) {
    
    
    
    noneSize.emplace();

    noneSize->width = specifiedWidth ?
      *specifiedWidth :
      ComputeMissingDimension(aDefaultObjectSize, aIntrinsicRatio,
                              specifiedWidth, specifiedHeight,
                              eWidth);

    noneSize->height = specifiedHeight ?
      *specifiedHeight :
      ComputeMissingDimension(aDefaultObjectSize, aIntrinsicRatio,
                              specifiedWidth, specifiedHeight,
                              eHeight);
  }
  
  
  
  
  return noneSize;
}



static nsSize
ComputeConcreteObjectSize(const nsSize& aConstraintSize,
                          const IntrinsicSize& aIntrinsicSize,
                          const nsSize& aIntrinsicRatio,
                          uint8_t aObjectFit)
{
  
  
  
  if (MOZ_LIKELY(aObjectFit == NS_STYLE_OBJECT_FIT_FILL) ||
      aIntrinsicRatio.width == 0 ||
      aIntrinsicRatio.height == 0) {
    return aConstraintSize;
  }

  
  Maybe<nsImageRenderer::FitType> fitType;

  Maybe<nsSize> noneSize;
  if (aObjectFit == NS_STYLE_OBJECT_FIT_NONE ||
      aObjectFit == NS_STYLE_OBJECT_FIT_SCALE_DOWN) {
    noneSize = MaybeComputeObjectFitNoneSize(aConstraintSize, aIntrinsicSize,
                                             aIntrinsicRatio);
    if (!noneSize || aObjectFit == NS_STYLE_OBJECT_FIT_SCALE_DOWN) {
      
      
      fitType.emplace(nsImageRenderer::CONTAIN);
    }
  } else if (aObjectFit == NS_STYLE_OBJECT_FIT_COVER) {
    fitType.emplace(nsImageRenderer::COVER);
  } else if (aObjectFit == NS_STYLE_OBJECT_FIT_CONTAIN) {
    fitType.emplace(nsImageRenderer::CONTAIN);
  }

  Maybe<nsSize> constrainedSize;
  if (fitType) {
    constrainedSize.emplace(
      nsImageRenderer::ComputeConstrainedSize(aConstraintSize,
                                              aIntrinsicRatio,
                                              *fitType));
  }

  
  switch (aObjectFit) {
    
    case NS_STYLE_OBJECT_FIT_CONTAIN:
    case NS_STYLE_OBJECT_FIT_COVER:
      MOZ_ASSERT(constrainedSize);
      return *constrainedSize;

    case NS_STYLE_OBJECT_FIT_NONE:
      if (noneSize) {
        return *noneSize;
      }
      MOZ_ASSERT(constrainedSize);
      return *constrainedSize;

    case NS_STYLE_OBJECT_FIT_SCALE_DOWN:
      MOZ_ASSERT(constrainedSize);
      if (noneSize) {
        constrainedSize->width =
          std::min(constrainedSize->width, noneSize->width);
        constrainedSize->height =
          std::min(constrainedSize->height, noneSize->height);
      }
      return *constrainedSize;

    default:
      MOZ_ASSERT_UNREACHABLE("Unexpected enum value for 'object-fit'");
      return aConstraintSize; 
  }
}



typedef nsStyleBackground::Position::PositionCoord PositionCoord;
static bool
IsCoord50Pct(const PositionCoord& aCoord)
{
  return (aCoord.mLength == 0 &&
          aCoord.mHasPercent &&
          aCoord.mPercent == 0.5f);
}



static bool
HasInitialObjectFitAndPosition(const nsStylePosition* aStylePos)
{
  const nsStyleBackground::Position& objectPos = aStylePos->mObjectPosition;

  return aStylePos->mObjectFit == NS_STYLE_OBJECT_FIT_FILL &&
    IsCoord50Pct(objectPos.mXPosition) &&
    IsCoord50Pct(objectPos.mYPosition);
}

 nsRect
nsLayoutUtils::ComputeObjectDestRect(const nsRect& aConstraintRect,
                                     const IntrinsicSize& aIntrinsicSize,
                                     const nsSize& aIntrinsicRatio,
                                     const nsStylePosition* aStylePos,
                                     nsPoint* aAnchorPoint)
{
  
  
  nsSize concreteObjectSize =
    ComputeConcreteObjectSize(aConstraintRect.Size(), aIntrinsicSize,
                              aIntrinsicRatio, aStylePos->mObjectFit);

  
  nsPoint imageTopLeftPt, imageAnchorPt;
  nsImageRenderer::ComputeObjectAnchorPoint(aStylePos->mObjectPosition,
                                            aConstraintRect.Size(),
                                            concreteObjectSize,
                                            &imageTopLeftPt, &imageAnchorPt);
  
  
  
  imageTopLeftPt += aConstraintRect.TopLeft();
  imageAnchorPt += aConstraintRect.TopLeft();

  if (aAnchorPoint) {
    
    
    
    
    
    
    
    
    
    
    if (HasInitialObjectFitAndPosition(aStylePos)) {
      *aAnchorPoint = imageTopLeftPt;
    } else {
      *aAnchorPoint = imageAnchorPt;
    }
  }
  return nsRect(imageTopLeftPt, concreteObjectSize);
}

nsresult
nsLayoutUtils::GetFontMetricsForFrame(const nsIFrame* aFrame,
                                      nsFontMetrics** aFontMetrics,
                                      float aInflation)
{
  return nsLayoutUtils::GetFontMetricsForStyleContext(aFrame->StyleContext(),
                                                      aFontMetrics,
                                                      aInflation);
}

nsresult
nsLayoutUtils::GetFontMetricsForStyleContext(nsStyleContext* aStyleContext,
                                             nsFontMetrics** aFontMetrics,
                                             float aInflation)
{
  
  nsPresContext* pc = aStyleContext->PresContext();
  gfxUserFontSet* fs = pc->GetUserFontSet();
  gfxTextPerfMetrics* tp = pc->GetTextPerfMetrics();

  WritingMode wm(aStyleContext);
  gfxFont::Orientation orientation =
    wm.IsVertical() && !wm.IsSideways() ? gfxFont::eVertical
                                        : gfxFont::eHorizontal;

  const nsStyleFont* styleFont = aStyleContext->StyleFont();

  
  
  
  
  if (aInflation == 1.0f) {
    return pc->DeviceContext()->GetMetricsFor(styleFont->mFont,
                                              styleFont->mLanguage,
                                              styleFont->mExplicitLanguage,
                                              orientation, fs, tp,
                                              *aFontMetrics);
  }

  nsFont font = styleFont->mFont;
  font.size = NSToCoordRound(font.size * aInflation);
  return pc->DeviceContext()->GetMetricsFor(font, styleFont->mLanguage,
                                            styleFont->mExplicitLanguage,
                                            orientation, fs, tp,
                                            *aFontMetrics);
}

nsIFrame*
nsLayoutUtils::FindChildContainingDescendant(nsIFrame* aParent, nsIFrame* aDescendantFrame)
{
  nsIFrame* result = aDescendantFrame;

  while (result) {
    nsIFrame* parent = result->GetParent();
    if (parent == aParent) {
      break;
    }

    
    result = parent;
  }

  return result;
}

nsBlockFrame*
nsLayoutUtils::GetAsBlock(nsIFrame* aFrame)
{
  nsBlockFrame* block = do_QueryFrame(aFrame);
  return block;
}

nsBlockFrame*
nsLayoutUtils::FindNearestBlockAncestor(nsIFrame* aFrame)
{
  nsIFrame* nextAncestor;
  for (nextAncestor = aFrame->GetParent(); nextAncestor;
       nextAncestor = nextAncestor->GetParent()) {
    nsBlockFrame* block = GetAsBlock(nextAncestor);
    if (block)
      return block;
  }
  return nullptr;
}

nsIFrame*
nsLayoutUtils::GetNonGeneratedAncestor(nsIFrame* aFrame)
{
  if (!(aFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT))
    return aFrame;

  nsIFrame* f = aFrame;
  do {
    f = GetParentOrPlaceholderFor(f);
  } while (f->GetStateBits() & NS_FRAME_GENERATED_CONTENT);
  return f;
}

nsIFrame*
nsLayoutUtils::GetParentOrPlaceholderFor(nsIFrame* aFrame)
{
  if ((aFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)
      && !aFrame->GetPrevInFlow()) {
    return aFrame->PresContext()->PresShell()->FrameManager()->
      GetPlaceholderFrameFor(aFrame);
  }
  return aFrame->GetParent();
}

nsIFrame*
nsLayoutUtils::GetParentOrPlaceholderForCrossDoc(nsIFrame* aFrame)
{
  nsIFrame* f = GetParentOrPlaceholderFor(aFrame);
  if (f)
    return f;
  return GetCrossDocParentFrame(aFrame);
}

nsIFrame*
nsLayoutUtils::GetNextContinuationOrIBSplitSibling(nsIFrame *aFrame)
{
  nsIFrame *result = aFrame->GetNextContinuation();
  if (result)
    return result;

  if ((aFrame->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT) != 0) {
    
    
    aFrame = aFrame->FirstContinuation();

    void* value = aFrame->Properties().Get(nsIFrame::IBSplitSibling());
    return static_cast<nsIFrame*>(value);
  }

  return nullptr;
}

nsIFrame*
nsLayoutUtils::FirstContinuationOrIBSplitSibling(nsIFrame *aFrame)
{
  nsIFrame *result = aFrame->FirstContinuation();
  if (result->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT) {
    while (true) {
      nsIFrame *f = static_cast<nsIFrame*>
        (result->Properties().Get(nsIFrame::IBSplitPrevSibling()));
      if (!f)
        break;
      result = f;
    }
  }

  return result;
}

nsIFrame*
nsLayoutUtils::LastContinuationOrIBSplitSibling(nsIFrame *aFrame)
{
  nsIFrame *result = aFrame->FirstContinuation();
  if (result->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT) {
    while (true) {
      nsIFrame *f = static_cast<nsIFrame*>
        (result->Properties().Get(nsIFrame::IBSplitSibling()));
      if (!f)
        break;
      result = f;
    }
  }

  result = result->LastContinuation();

  return result;
}

bool
nsLayoutUtils::IsFirstContinuationOrIBSplitSibling(nsIFrame *aFrame)
{
  if (aFrame->GetPrevContinuation()) {
    return false;
  }
  if ((aFrame->GetStateBits() & NS_FRAME_PART_OF_IBSPLIT) &&
      aFrame->Properties().Get(nsIFrame::IBSplitPrevSibling())) {
    return false;
  }

  return true;
}

bool
nsLayoutUtils::IsViewportScrollbarFrame(nsIFrame* aFrame)
{
  if (!aFrame)
    return false;

  nsIFrame* rootScrollFrame =
    aFrame->PresContext()->PresShell()->GetRootScrollFrame();
  if (!rootScrollFrame)
    return false;

  nsIScrollableFrame* rootScrollableFrame = do_QueryFrame(rootScrollFrame);
  NS_ASSERTION(rootScrollableFrame, "The root scorollable frame is null");

  if (!IsProperAncestorFrame(rootScrollFrame, aFrame))
    return false;

  nsIFrame* rootScrolledFrame = rootScrollableFrame->GetScrolledFrame();
  return !(rootScrolledFrame == aFrame ||
           IsProperAncestorFrame(rootScrolledFrame, aFrame));
}

static nscoord AddPercents(nsLayoutUtils::IntrinsicISizeType aType,
                           nscoord aCurrent, float aPercent)
{
  nscoord result = aCurrent;
  if (aPercent > 0.0f && aType == nsLayoutUtils::PREF_ISIZE) {
    
    
    if (aPercent >= 1.0f)
      result = nscoord_MAX;
    else
      result = NSToCoordRound(float(result) / (1.0f - aPercent));
  }
  return result;
}



static bool GetAbsoluteCoord(const nsStyleCoord& aStyle, nscoord& aResult)
{
  if (aStyle.IsCalcUnit()) {
    if (aStyle.CalcHasPercent()) {
      return false;
    }
    
    aResult = nsRuleNode::ComputeComputedCalc(aStyle, 0);
    if (aResult < 0)
      aResult = 0;
    return true;
  }

  if (eStyleUnit_Coord != aStyle.GetUnit())
    return false;

  aResult = aStyle.GetCoordValue();
  NS_ASSERTION(aResult >= 0, "negative widths not allowed");
  return true;
}


static bool
GetPercentBSize(const nsStyleCoord& aStyle,
                 nsIFrame* aFrame,
                 nscoord& aResult)
{
  if (eStyleUnit_Percent != aStyle.GetUnit() &&
      !aStyle.IsCalcUnit())
    return false;

  MOZ_ASSERT(!aStyle.IsCalcUnit() || aStyle.CalcHasPercent(),
             "GetAbsoluteCoord should have handled this");

  nsIFrame *f = aFrame->GetContainingBlock();
  if (!f) {
    NS_NOTREACHED("top of frame tree not a containing block");
    return false;
  }

  
  
  
  
  
  if (f->StyleContext()->GetPseudo() == nsCSSAnonBoxes::scrolledContent) {
    f = f->GetParent();
  }

  bool isVertical = f->GetWritingMode().IsVertical();

  const nsStylePosition *pos = f->StylePosition();
  const nsStyleCoord bSizeCoord = isVertical ? pos->mWidth : pos->mHeight;
  nscoord h;
  if (!GetAbsoluteCoord(bSizeCoord, h) &&
      !GetPercentBSize(bSizeCoord, f, h)) {
    NS_ASSERTION(bSizeCoord.GetUnit() == eStyleUnit_Auto ||
                 bSizeCoord.HasPercent(),
                 "unknown block-size unit");
    nsIAtom* fType = f->GetType();
    if (fType != nsGkAtoms::viewportFrame && fType != nsGkAtoms::canvasFrame &&
        fType != nsGkAtoms::pageContentFrame) {
      
      
      
      
      
      return false;
    }

    NS_ASSERTION(bSizeCoord.GetUnit() == eStyleUnit_Auto,
                 "Unexpected block-size unit for viewport or canvas or page-content");
    
    
    h = isVertical ? f->GetSize().width : f->GetSize().height;
    if (h == NS_UNCONSTRAINEDSIZE) {
      
      return false;
    }
  }

  const nsStyleCoord& maxBSizeCoord =
    isVertical ? pos->mMaxWidth : pos->mMaxHeight;

  nscoord maxh;
  if (GetAbsoluteCoord(maxBSizeCoord, maxh) ||
      GetPercentBSize(maxBSizeCoord, f, maxh)) {
    if (maxh < h)
      h = maxh;
  } else {
    NS_ASSERTION(maxBSizeCoord.GetUnit() == eStyleUnit_None ||
                 maxBSizeCoord.HasPercent(),
                 "unknown max block-size unit");
  }

  const nsStyleCoord& minBSizeCoord =
    isVertical ? pos->mMinWidth : pos->mMinHeight;

  nscoord minh;
  if (GetAbsoluteCoord(minBSizeCoord, minh) ||
      GetPercentBSize(minBSizeCoord, f, minh)) {
    if (minh > h)
      h = minh;
  } else {
    NS_ASSERTION(minBSizeCoord.HasPercent() ||
                 minBSizeCoord.GetUnit() == eStyleUnit_Auto,
                 "unknown min block-size unit");
  }

  if (aStyle.IsCalcUnit()) {
    aResult = std::max(nsRuleNode::ComputeComputedCalc(aStyle, h), 0);
    return true;
  }

  aResult = NSToCoordRound(aStyle.GetPercentValue() * h);
  return true;
}





enum eWidthProperty { PROP_WIDTH, PROP_MAX_WIDTH, PROP_MIN_WIDTH };
static bool
GetIntrinsicCoord(const nsStyleCoord& aStyle,
                  nsRenderingContext* aRenderingContext,
                  nsIFrame* aFrame,
                  eWidthProperty aProperty,
                  nscoord& aResult)
{
  NS_PRECONDITION(aProperty == PROP_WIDTH || aProperty == PROP_MAX_WIDTH ||
                  aProperty == PROP_MIN_WIDTH, "unexpected property");
  if (aStyle.GetUnit() != eStyleUnit_Enumerated)
    return false;
  int32_t val = aStyle.GetIntValue();
  NS_ASSERTION(val == NS_STYLE_WIDTH_MAX_CONTENT ||
               val == NS_STYLE_WIDTH_MIN_CONTENT ||
               val == NS_STYLE_WIDTH_FIT_CONTENT ||
               val == NS_STYLE_WIDTH_AVAILABLE,
               "unexpected enumerated value for width property");
  if (val == NS_STYLE_WIDTH_AVAILABLE)
    return false;
  if (val == NS_STYLE_WIDTH_FIT_CONTENT) {
    if (aProperty == PROP_WIDTH)
      return false; 
    if (aProperty == PROP_MAX_WIDTH)
      
      val = NS_STYLE_WIDTH_MAX_CONTENT;
    else
      
      val = NS_STYLE_WIDTH_MIN_CONTENT;
  }

  NS_ASSERTION(val == NS_STYLE_WIDTH_MAX_CONTENT ||
               val == NS_STYLE_WIDTH_MIN_CONTENT,
               "should have reduced everything remaining to one of these");

  
  
  AutoMaybeDisableFontInflation an(aFrame);

  if (val == NS_STYLE_WIDTH_MAX_CONTENT)
    aResult = aFrame->GetPrefISize(aRenderingContext);
  else
    aResult = aFrame->GetMinISize(aRenderingContext);
  return true;
}

#undef  DEBUG_INTRINSIC_WIDTH

#ifdef DEBUG_INTRINSIC_WIDTH
static int32_t gNoiseIndent = 0;
#endif

 nscoord
nsLayoutUtils::IntrinsicForContainer(nsRenderingContext *aRenderingContext,
                                     nsIFrame *aFrame,
                                     IntrinsicISizeType aType,
                                     uint32_t aFlags)
{
  NS_PRECONDITION(aFrame, "null frame");
  NS_PRECONDITION(aFrame->GetParent(),
                  "IntrinsicForContainer called on frame not in tree");
  NS_PRECONDITION(aType == MIN_ISIZE || aType == PREF_ISIZE, "bad type");

#ifdef DEBUG_INTRINSIC_WIDTH
  nsFrame::IndentBy(stderr, gNoiseIndent);
  static_cast<nsFrame*>(aFrame)->ListTag(stderr);
  printf_stderr(" %s intrinsic inline-size for container:\n",
         aType == MIN_ISIZE ? "min" : "pref");
#endif

  
  
  AutoMaybeDisableFontInflation an(aFrame);

  nsIFrame::IntrinsicISizeOffsetData offsets =
    aFrame->IntrinsicISizeOffsets(aRenderingContext);

  
  
  
  WritingMode wm = aFrame->GetParent()->GetWritingMode();
  WritingMode ourWM = aFrame->GetWritingMode();
  bool isOrthogonal = ourWM.IsOrthogonalTo(wm);
  bool isVertical = wm.IsVertical();

  const nsStylePosition *stylePos = aFrame->StylePosition();
  uint8_t boxSizing = stylePos->mBoxSizing;

  const nsStyleCoord &styleISize =
    isVertical ? stylePos->mHeight : stylePos->mWidth;
  const nsStyleCoord &styleMinISize =
    isVertical ? stylePos->mMinHeight : stylePos->mMinWidth;
  const nsStyleCoord &styleMaxISize =
    isVertical ? stylePos->mMaxHeight : stylePos->mMaxWidth;

  
  
  
  
  
  
  
  
  
  
  nscoord result = 0, min = 0;

  nscoord maxISize;
  bool haveFixedMaxISize = GetAbsoluteCoord(styleMaxISize, maxISize);
  nscoord minISize;

  
  bool haveFixedMinISize;
  if (eStyleUnit_Auto == styleMinISize.GetUnit()) {
    
    
    
    
    minISize = 0;
    haveFixedMinISize = true;
  } else {
    haveFixedMinISize = GetAbsoluteCoord(styleMinISize, minISize);
  }

  
  
  
  
  
  if (styleISize.GetUnit() == eStyleUnit_Enumerated &&
      (styleISize.GetIntValue() == NS_STYLE_WIDTH_MAX_CONTENT ||
       styleISize.GetIntValue() == NS_STYLE_WIDTH_MIN_CONTENT)) {
    
    
    
    
    boxSizing = NS_STYLE_BOX_SIZING_CONTENT;
  } else if (!styleISize.ConvertsToLength() &&
             !(haveFixedMinISize && haveFixedMaxISize && maxISize <= minISize)) {
#ifdef DEBUG_INTRINSIC_WIDTH
    ++gNoiseIndent;
#endif
    if (isOrthogonal) {
      
      
      
      
      
      
      
      
      
      result = aFrame->BSize();
    } else {
      result = aType == MIN_ISIZE
               ? aFrame->GetMinISize(aRenderingContext)
               : aFrame->GetPrefISize(aRenderingContext);
    }
#ifdef DEBUG_INTRINSIC_WIDTH
    --gNoiseIndent;
    nsFrame::IndentBy(stderr, gNoiseIndent);
    static_cast<nsFrame*>(aFrame)->ListTag(stderr);
    printf_stderr(" %s intrinsic inline-size from frame is %d.\n",
           aType == MIN_ISIZE ? "min" : "pref", result);
#endif

    
    
    
    
    
    
    const nsStyleCoord &styleBSize =
      isVertical ? stylePos->mWidth : stylePos->mHeight;
    const nsStyleCoord &styleMinBSize =
      isVertical ? stylePos->mMinWidth : stylePos->mMinHeight;
    const nsStyleCoord &styleMaxBSize =
      isVertical ? stylePos->mMaxWidth : stylePos->mMaxHeight;

    if (styleBSize.GetUnit() != eStyleUnit_Auto ||
        !(styleMinBSize.GetUnit() == eStyleUnit_Auto ||
          (styleMinBSize.GetUnit() == eStyleUnit_Coord &&
           styleMinBSize.GetCoordValue() == 0)) ||
        styleMaxBSize.GetUnit() != eStyleUnit_None) {

      LogicalSize ratio(wm, aFrame->GetIntrinsicRatio());

      if (ratio.BSize(wm) != 0) {
        nscoord bSizeTakenByBoxSizing = 0;
        switch (boxSizing) {
        case NS_STYLE_BOX_SIZING_BORDER: {
          const nsStyleBorder* styleBorder = aFrame->StyleBorder();
          bSizeTakenByBoxSizing +=
            isVertical ? styleBorder->GetComputedBorder().LeftRight()
                       : styleBorder->GetComputedBorder().TopBottom();
          
        }
        case NS_STYLE_BOX_SIZING_PADDING: {
          if (!(aFlags & IGNORE_PADDING)) {
            const nsStylePadding* stylePadding = aFrame->StylePadding();
            const nsStyleCoord& paddingStart =
              isVertical ? wm.IsVerticalRL()
                           ? stylePadding->mPadding.GetRight()
                           : stylePadding->mPadding.GetLeft()
                         : stylePadding->mPadding.GetTop();
            const nsStyleCoord& paddingEnd =
              isVertical ? wm.IsVerticalRL()
                           ? stylePadding->mPadding.GetLeft()
                           : stylePadding->mPadding.GetRight()
                         : stylePadding->mPadding.GetBottom();
            nscoord pad;
            if (GetAbsoluteCoord(paddingStart, pad) ||
                GetPercentBSize(paddingStart, aFrame, pad)) {
              bSizeTakenByBoxSizing += pad;
            }
            if (GetAbsoluteCoord(paddingEnd, pad) ||
                GetPercentBSize(paddingEnd, aFrame, pad)) {
              bSizeTakenByBoxSizing += pad;
            }
          }
          
        }
        case NS_STYLE_BOX_SIZING_CONTENT:
        default:
          break;
        }

        nscoord h;
        if (GetAbsoluteCoord(styleBSize, h) ||
            GetPercentBSize(styleBSize, aFrame, h)) {
          h = std::max(0, h - bSizeTakenByBoxSizing);
          result = NSCoordMulDiv(h, ratio.ISize(wm), ratio.BSize(wm));
        }

        if (GetAbsoluteCoord(styleMaxBSize, h) ||
            GetPercentBSize(styleMaxBSize, aFrame, h)) {
          h = std::max(0, h - bSizeTakenByBoxSizing);
          nscoord maxISize = NSCoordMulDiv(h, ratio.ISize(wm), ratio.BSize(wm));
          if (maxISize < result)
            result = maxISize;
        }

        if (GetAbsoluteCoord(styleMinBSize, h) ||
            GetPercentBSize(styleMinBSize, aFrame, h)) {
          h = std::max(0, h - bSizeTakenByBoxSizing);
          nscoord minISize = NSCoordMulDiv(h, ratio.ISize(wm), ratio.BSize(wm));
          if (minISize > result)
            result = minISize;
        }
      }
    }
  }

  if (aFrame->GetType() == nsGkAtoms::tableFrame) {
    
    
    min = aFrame->GetMinISize(aRenderingContext);
  }

  
  
  
  
  
  
  
  nscoord coordOutsideISize = 0;
  float pctOutsideISize = 0;
  float pctTotal = 0.0f;

  if (!(aFlags & IGNORE_PADDING)) {
    coordOutsideISize += offsets.hPadding;
    pctOutsideISize += offsets.hPctPadding;

    if (boxSizing == NS_STYLE_BOX_SIZING_PADDING) {
      min += coordOutsideISize;
      result = NSCoordSaturatingAdd(result, coordOutsideISize);
      pctTotal += pctOutsideISize;

      coordOutsideISize = 0;
      pctOutsideISize = 0.0f;
    }
  }

  coordOutsideISize += offsets.hBorder;

  if (boxSizing == NS_STYLE_BOX_SIZING_BORDER) {
    min += coordOutsideISize;
    result = NSCoordSaturatingAdd(result, coordOutsideISize);
    pctTotal += pctOutsideISize;

    coordOutsideISize = 0;
    pctOutsideISize = 0.0f;
  }

  coordOutsideISize += offsets.hMargin;
  pctOutsideISize += offsets.hPctMargin;

  min += coordOutsideISize;
  result = NSCoordSaturatingAdd(result, coordOutsideISize);
  pctTotal += pctOutsideISize;

  nscoord w;
  if (GetAbsoluteCoord(styleISize, w) ||
      GetIntrinsicCoord(styleISize, aRenderingContext, aFrame,
                        PROP_WIDTH, w)) {
    result = AddPercents(aType, w + coordOutsideISize, pctOutsideISize);
  }
  else if (aType == MIN_ISIZE &&
           
           
           
           styleISize.IsCoordPercentCalcUnit() &&
           aFrame->IsFrameOfType(nsIFrame::eReplaced)) {
    
    result = 0; 
  }
  else {
    
    
    
    
    
    result = AddPercents(aType, result, pctTotal);
  }

  if (haveFixedMaxISize ||
      GetIntrinsicCoord(styleMaxISize, aRenderingContext, aFrame,
                        PROP_MAX_WIDTH, maxISize)) {
    maxISize = AddPercents(aType, maxISize + coordOutsideISize, pctOutsideISize);
    if (result > maxISize)
      result = maxISize;
  }

  if (haveFixedMinISize ||
      GetIntrinsicCoord(styleMinISize, aRenderingContext, aFrame,
                        PROP_MIN_WIDTH, minISize)) {
    minISize = AddPercents(aType, minISize + coordOutsideISize, pctOutsideISize);
    if (result < minISize)
      result = minISize;
  }

  min = AddPercents(aType, min, pctTotal);
  if (result < min)
    result = min;

  const nsStyleDisplay *disp = aFrame->StyleDisplay();
  if (aFrame->IsThemed(disp)) {
    LayoutDeviceIntSize size;
    bool canOverride = true;
    nsPresContext *presContext = aFrame->PresContext();
    presContext->GetTheme()->
      GetMinimumWidgetSize(presContext, aFrame, disp->mAppearance,
                           &size, &canOverride);

    nscoord themeISize =
      presContext->DevPixelsToAppUnits(isVertical ? size.height : size.width);

    
    themeISize += offsets.hMargin;
    themeISize = AddPercents(aType, themeISize, offsets.hPctMargin);

    if (themeISize > result || !canOverride) {
      result = themeISize;
    }
  }

#ifdef DEBUG_INTRINSIC_WIDTH
  nsFrame::IndentBy(stderr, gNoiseIndent);
  static_cast<nsFrame*>(aFrame)->ListTag(stderr);
  printf_stderr(" %s intrinsic inline-size for container is %d twips.\n",
         aType == MIN_ISIZE ? "min" : "pref", result);
#endif

  return result;
}

 nscoord
nsLayoutUtils::ComputeCBDependentValue(nscoord aPercentBasis,
                                       const nsStyleCoord& aCoord)
{
  NS_WARN_IF_FALSE(aPercentBasis != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width or height; this should only "
                   "result from very large sizes, not attempts at intrinsic "
                   "size calculation");

  if (aCoord.IsCoordPercentCalcUnit()) {
    return nsRuleNode::ComputeCoordPercentCalc(aCoord, aPercentBasis);
  }
  NS_ASSERTION(aCoord.GetUnit() == eStyleUnit_None ||
               aCoord.GetUnit() == eStyleUnit_Auto,
               "unexpected width value");
  return 0;
}

 nscoord
nsLayoutUtils::ComputeISizeValue(
                 nsRenderingContext* aRenderingContext,
                 nsIFrame*            aFrame,
                 nscoord              aContainingBlockISize,
                 nscoord              aContentEdgeToBoxSizing,
                 nscoord              aBoxSizingToMarginEdge,
                 const nsStyleCoord&  aCoord)
{
  NS_PRECONDITION(aFrame, "non-null frame expected");
  NS_PRECONDITION(aRenderingContext, "non-null rendering context expected");
  NS_WARN_IF_FALSE(aContainingBlockISize != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained inline-size; this should only result from "
                   "very large sizes, not attempts at intrinsic inline-size "
                   "calculation");
  NS_PRECONDITION(aContainingBlockISize >= 0,
                  "inline-size less than zero");

  nscoord result;
  if (aCoord.IsCoordPercentCalcUnit()) {
    result = nsRuleNode::ComputeCoordPercentCalc(aCoord, 
                                                 aContainingBlockISize);
    
    
    
    result -= aContentEdgeToBoxSizing;
  } else {
    MOZ_ASSERT(eStyleUnit_Enumerated == aCoord.GetUnit());
    
    
    AutoMaybeDisableFontInflation an(aFrame);

    int32_t val = aCoord.GetIntValue();
    switch (val) {
      case NS_STYLE_WIDTH_MAX_CONTENT:
        result = aFrame->GetPrefISize(aRenderingContext);
        NS_ASSERTION(result >= 0, "inline-size less than zero");
        break;
      case NS_STYLE_WIDTH_MIN_CONTENT:
        result = aFrame->GetMinISize(aRenderingContext);
        NS_ASSERTION(result >= 0, "inline-size less than zero");
        break;
      case NS_STYLE_WIDTH_FIT_CONTENT:
        {
          nscoord pref = aFrame->GetPrefISize(aRenderingContext),
                   min = aFrame->GetMinISize(aRenderingContext),
                  fill = aContainingBlockISize -
                         (aBoxSizingToMarginEdge + aContentEdgeToBoxSizing);
          result = std::max(min, std::min(pref, fill));
          NS_ASSERTION(result >= 0, "inline-size less than zero");
        }
        break;
      case NS_STYLE_WIDTH_AVAILABLE:
        result = aContainingBlockISize -
                 (aBoxSizingToMarginEdge + aContentEdgeToBoxSizing);
    }
  }

  return std::max(0, result);
}

 nscoord
nsLayoutUtils::ComputeBSizeDependentValue(
                 nscoord              aContainingBlockBSize,
                 const nsStyleCoord&  aCoord)
{
  
  
  
  
  
  
  
  NS_PRECONDITION(NS_AUTOHEIGHT != aContainingBlockBSize ||
                  !aCoord.HasPercent(),
                  "unexpected containing block block-size");

  if (aCoord.IsCoordPercentCalcUnit()) {
    return nsRuleNode::ComputeCoordPercentCalc(aCoord, aContainingBlockBSize);
  }

  NS_ASSERTION(aCoord.GetUnit() == eStyleUnit_None ||
               aCoord.GetUnit() == eStyleUnit_Auto,
               "unexpected block-size value");
  return 0;
}

 void
nsLayoutUtils::MarkDescendantsDirty(nsIFrame *aSubtreeRoot)
{
  nsAutoTArray<nsIFrame*, 4> subtrees;
  subtrees.AppendElement(aSubtreeRoot);

  
  
  do {
    nsIFrame *subtreeRoot = subtrees.ElementAt(subtrees.Length() - 1);
    subtrees.RemoveElementAt(subtrees.Length() - 1);

    
    
    
    
    nsAutoTArray<nsIFrame*, 32> stack;
    stack.AppendElement(subtreeRoot);

    do {
      nsIFrame *f = stack.ElementAt(stack.Length() - 1);
      stack.RemoveElementAt(stack.Length() - 1);

      f->MarkIntrinsicISizesDirty();

      if (f->GetType() == nsGkAtoms::placeholderFrame) {
        nsIFrame *oof = nsPlaceholderFrame::GetRealFrameForPlaceholder(f);
        if (!nsLayoutUtils::IsProperAncestorFrame(subtreeRoot, oof)) {
          
          subtrees.AppendElement(oof);
        }
      }

      nsIFrame::ChildListIterator lists(f);
      for (; !lists.IsDone(); lists.Next()) {
        nsFrameList::Enumerator childFrames(lists.CurrentList());
        for (; !childFrames.AtEnd(); childFrames.Next()) {
          nsIFrame* kid = childFrames.get();
          stack.AppendElement(kid);
        }
      }
    } while (stack.Length() != 0);
  } while (subtrees.Length() != 0);
}


LogicalSize
nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(WritingMode aWM,
                   nsRenderingContext* aRenderingContext, nsIFrame* aFrame,
                   const IntrinsicSize& aIntrinsicSize,
                   nsSize aIntrinsicRatio,
                   const mozilla::LogicalSize& aCBSize,
                   const mozilla::LogicalSize& aMargin,
                   const mozilla::LogicalSize& aBorder,
                   const mozilla::LogicalSize& aPadding)
{
  const nsStylePosition* stylePos = aFrame->StylePosition();

  
  bool isVertical = aWM.IsVertical();
  const nsStyleCoord* inlineStyleCoord =
    isVertical ? &stylePos->mHeight : &stylePos->mWidth;
  const nsStyleCoord* blockStyleCoord =
    isVertical ? &stylePos->mWidth : &stylePos->mHeight;

  bool isFlexItem = aFrame->IsFlexItem();
  bool isInlineFlexItem = false;

  if (isFlexItem) {
    
    
    
    
    uint32_t flexDirection =
      aFrame->GetParent()->StylePosition()->mFlexDirection;
    isInlineFlexItem =
      flexDirection == NS_STYLE_FLEX_DIRECTION_ROW ||
      flexDirection == NS_STYLE_FLEX_DIRECTION_ROW_REVERSE;

    
    
    const nsStyleCoord* flexBasis = &(stylePos->mFlexBasis);
    if (flexBasis->GetUnit() != eStyleUnit_Auto) {
      if (isInlineFlexItem) {
        inlineStyleCoord = flexBasis;
      } else {
        
        
        
        
        
        
        
        if (flexBasis->GetUnit() != eStyleUnit_Enumerated) {
          blockStyleCoord = flexBasis;
        }
      }
    }
  }

  
  
  

  
  
  

  const bool isAutoISize = inlineStyleCoord->GetUnit() == eStyleUnit_Auto;
  const bool isAutoBSize = IsAutoBSize(*blockStyleCoord, aCBSize.BSize(aWM));

  LogicalSize boxSizingAdjust(aWM);
  switch (stylePos->mBoxSizing) {
    case NS_STYLE_BOX_SIZING_BORDER:
      boxSizingAdjust += aBorder;
      
    case NS_STYLE_BOX_SIZING_PADDING:
      boxSizingAdjust += aPadding;
  }
  nscoord boxSizingToMarginEdgeISize =
    aMargin.ISize(aWM) + aBorder.ISize(aWM) + aPadding.ISize(aWM) -
      boxSizingAdjust.ISize(aWM);

  nscoord iSize, minISize, maxISize, bSize, minBSize, maxBSize;

  if (!isAutoISize) {
    iSize = nsLayoutUtils::ComputeISizeValue(aRenderingContext,
              aFrame, aCBSize.ISize(aWM), boxSizingAdjust.ISize(aWM),
              boxSizingToMarginEdgeISize, *inlineStyleCoord);
  }

  const nsStyleCoord& maxISizeCoord =
    isVertical ? stylePos->mMaxHeight : stylePos->mMaxWidth;

  if (maxISizeCoord.GetUnit() != eStyleUnit_None &&
      !(isFlexItem && isInlineFlexItem)) {
    maxISize = nsLayoutUtils::ComputeISizeValue(aRenderingContext,
                 aFrame, aCBSize.ISize(aWM), boxSizingAdjust.ISize(aWM),
                 boxSizingToMarginEdgeISize, maxISizeCoord);
  } else {
    maxISize = nscoord_MAX;
  }

  
  
  

  const nsStyleCoord& minISizeCoord =
    isVertical ? stylePos->mMinHeight : stylePos->mMinWidth;

  if (minISizeCoord.GetUnit() != eStyleUnit_Auto &&
      !(isFlexItem && isInlineFlexItem)) {
    minISize = nsLayoutUtils::ComputeISizeValue(aRenderingContext,
                 aFrame, aCBSize.ISize(aWM), boxSizingAdjust.ISize(aWM),
                 boxSizingToMarginEdgeISize, minISizeCoord);
  } else {
    
    
    
    
    
    minISize = 0;
  }

  if (!isAutoBSize) {
    bSize = nsLayoutUtils::ComputeBSizeValue(aCBSize.BSize(aWM),
                boxSizingAdjust.BSize(aWM),
                *blockStyleCoord);
  }

  const nsStyleCoord& maxBSizeCoord =
    isVertical ? stylePos->mMaxWidth : stylePos->mMaxHeight;

  if (!IsAutoBSize(maxBSizeCoord, aCBSize.BSize(aWM)) &&
      !(isFlexItem && !isInlineFlexItem)) {
    maxBSize = nsLayoutUtils::ComputeBSizeValue(aCBSize.BSize(aWM),
                  boxSizingAdjust.BSize(aWM), maxBSizeCoord);
  } else {
    maxBSize = nscoord_MAX;
  }

  const nsStyleCoord& minBSizeCoord =
    isVertical ? stylePos->mMinWidth : stylePos->mMinHeight;

  if (!IsAutoBSize(minBSizeCoord, aCBSize.BSize(aWM)) &&
      !(isFlexItem && !isInlineFlexItem)) {
    minBSize = nsLayoutUtils::ComputeBSizeValue(aCBSize.BSize(aWM),
                  boxSizingAdjust.BSize(aWM), minBSizeCoord);
  } else {
    minBSize = 0;
  }

  

  NS_ASSERTION(aCBSize.ISize(aWM) != NS_UNCONSTRAINEDSIZE,
               "Our containing block must not have unconstrained inline-size!");

  const nsStyleCoord& isizeCoord =
    isVertical ? aIntrinsicSize.height : aIntrinsicSize.width;
  const nsStyleCoord& bsizeCoord =
    isVertical ? aIntrinsicSize.width : aIntrinsicSize.height;

  bool hasIntrinsicISize, hasIntrinsicBSize;
  nscoord intrinsicISize, intrinsicBSize;

  if (isizeCoord.GetUnit() == eStyleUnit_Coord) {
    hasIntrinsicISize = true;
    intrinsicISize = isizeCoord.GetCoordValue();
    if (intrinsicISize < 0)
      intrinsicISize = 0;
  } else {
    NS_ASSERTION(isizeCoord.GetUnit() == eStyleUnit_None,
                 "unexpected unit");
    hasIntrinsicISize = false;
    intrinsicISize = 0;
  }

  if (bsizeCoord.GetUnit() == eStyleUnit_Coord) {
    hasIntrinsicBSize = true;
    intrinsicBSize = bsizeCoord.GetCoordValue();
    if (intrinsicBSize < 0)
      intrinsicBSize = 0;
  } else {
    NS_ASSERTION(bsizeCoord.GetUnit() == eStyleUnit_None,
                 "unexpected unit");
    hasIntrinsicBSize = false;
    intrinsicBSize = 0;
  }

  NS_ASSERTION(aIntrinsicRatio.width >= 0 && aIntrinsicRatio.height >= 0,
               "Intrinsic ratio has a negative component!");
  LogicalSize logicalRatio(aWM, aIntrinsicRatio);

  

  if (isAutoISize) {
    if (isAutoBSize) {

      

      

      nscoord tentISize, tentBSize;

      if (hasIntrinsicISize) {
        tentISize = intrinsicISize;
      } else if (hasIntrinsicBSize && logicalRatio.BSize(aWM) > 0) {
        tentISize = NSCoordMulDiv(intrinsicBSize, logicalRatio.ISize(aWM), logicalRatio.BSize(aWM));
      } else if (logicalRatio.ISize(aWM) > 0) {
        tentISize = aCBSize.ISize(aWM) - boxSizingToMarginEdgeISize; 
        if (tentISize < 0) tentISize = 0;
      } else {
        tentISize = nsPresContext::CSSPixelsToAppUnits(300);
      }

      if (hasIntrinsicBSize) {
        tentBSize = intrinsicBSize;
      } else if (logicalRatio.ISize(aWM) > 0) {
        tentBSize = NSCoordMulDiv(tentISize, logicalRatio.BSize(aWM), logicalRatio.ISize(aWM));
      } else {
        tentBSize = nsPresContext::CSSPixelsToAppUnits(150);
      }

      if (aIntrinsicRatio != nsSize(0, 0)) {
        nsSize autoSize =
          ComputeAutoSizeWithIntrinsicDimensions(minISize, minBSize,
                                                 maxISize, maxBSize,
                                                 tentISize, tentBSize);
        
        
        
        
        iSize = autoSize.width;
        bSize = autoSize.height;
      } else {
        
        
        
        
        
        iSize = NS_CSS_MINMAX(tentISize, minISize, maxISize);
        bSize = NS_CSS_MINMAX(tentBSize, minBSize, maxBSize);
      }
    } else {

      
      bSize = NS_CSS_MINMAX(bSize, minBSize, maxBSize);
      if (logicalRatio.BSize(aWM) > 0) {
        iSize = NSCoordMulDiv(bSize, logicalRatio.ISize(aWM), logicalRatio.BSize(aWM));
      } else if (hasIntrinsicISize) {
        iSize = intrinsicISize;
      } else {
        iSize = nsPresContext::CSSPixelsToAppUnits(300);
      }
      iSize = NS_CSS_MINMAX(iSize, minISize, maxISize);

    }
  } else {
    if (isAutoBSize) {

      
      iSize = NS_CSS_MINMAX(iSize, minISize, maxISize);
      if (logicalRatio.ISize(aWM) > 0) {
        bSize = NSCoordMulDiv(iSize, logicalRatio.BSize(aWM), logicalRatio.ISize(aWM));
      } else if (hasIntrinsicBSize) {
        bSize = intrinsicBSize;
      } else {
        bSize = nsPresContext::CSSPixelsToAppUnits(150);
      }
      bSize = NS_CSS_MINMAX(bSize, minBSize, maxBSize);

    } else {

      
      iSize = NS_CSS_MINMAX(iSize, minISize, maxISize);
      bSize = NS_CSS_MINMAX(bSize, minBSize, maxBSize);

    }
  }

  return LogicalSize(aWM, iSize, bSize);
}

nsSize
nsLayoutUtils::ComputeAutoSizeWithIntrinsicDimensions(nscoord minWidth, nscoord minHeight,
                                                      nscoord maxWidth, nscoord maxHeight,
                                                      nscoord tentWidth, nscoord tentHeight)
{
  

  if (minWidth > maxWidth)
    maxWidth = minWidth;
  if (minHeight > maxHeight)
    maxHeight = minHeight;

  nscoord heightAtMaxWidth, heightAtMinWidth,
          widthAtMaxHeight, widthAtMinHeight;

  if (tentWidth > 0) {
    heightAtMaxWidth = NSCoordMulDiv(maxWidth, tentHeight, tentWidth);
    if (heightAtMaxWidth < minHeight)
      heightAtMaxWidth = minHeight;
    heightAtMinWidth = NSCoordMulDiv(minWidth, tentHeight, tentWidth);
    if (heightAtMinWidth > maxHeight)
      heightAtMinWidth = maxHeight;
  } else {
    heightAtMaxWidth = heightAtMinWidth = NS_CSS_MINMAX(tentHeight, minHeight, maxHeight);
  }

  if (tentHeight > 0) {
    widthAtMaxHeight = NSCoordMulDiv(maxHeight, tentWidth, tentHeight);
    if (widthAtMaxHeight < minWidth)
      widthAtMaxHeight = minWidth;
    widthAtMinHeight = NSCoordMulDiv(minHeight, tentWidth, tentHeight);
    if (widthAtMinHeight > maxWidth)
      widthAtMinHeight = maxWidth;
  } else {
    widthAtMaxHeight = widthAtMinHeight = NS_CSS_MINMAX(tentWidth, minWidth, maxWidth);
  }

  

  nscoord width, height;

  if (tentWidth > maxWidth) {
    if (tentHeight > maxHeight) {
      if (int64_t(maxWidth) * int64_t(tentHeight) <=
          int64_t(maxHeight) * int64_t(tentWidth)) {
        width = maxWidth;
        height = heightAtMaxWidth;
      } else {
        width = widthAtMaxHeight;
        height = maxHeight;
      }
    } else {
      
      
      
      width = maxWidth;
      height = heightAtMaxWidth;
    }
  } else if (tentWidth < minWidth) {
    if (tentHeight < minHeight) {
      if (int64_t(minWidth) * int64_t(tentHeight) <=
          int64_t(minHeight) * int64_t(tentWidth)) {
        width = widthAtMinHeight;
        height = minHeight;
      } else {
        width = minWidth;
        height = heightAtMinWidth;
      }
    } else {
      
      
      
      width = minWidth;
      height = heightAtMinWidth;
    }
  } else {
    if (tentHeight > maxHeight) {
      width = widthAtMaxHeight;
      height = maxHeight;
    } else if (tentHeight < minHeight) {
      width = widthAtMinHeight;
      height = minHeight;
    } else {
      width = tentWidth;
      height = tentHeight;
    }
  }

  return nsSize(width, height);
}

 nscoord
nsLayoutUtils::MinISizeFromInline(nsIFrame* aFrame,
                                  nsRenderingContext* aRenderingContext)
{
  NS_ASSERTION(!aFrame->IsContainerForFontSizeInflation(),
               "should not be container for font size inflation");

  nsIFrame::InlineMinISizeData data;
  DISPLAY_MIN_WIDTH(aFrame, data.prevLines);
  aFrame->AddInlineMinISize(aRenderingContext, &data);
  data.ForceBreak(aRenderingContext);
  return data.prevLines;
}

 nscoord
nsLayoutUtils::PrefISizeFromInline(nsIFrame* aFrame,
                                   nsRenderingContext* aRenderingContext)
{
  NS_ASSERTION(!aFrame->IsContainerForFontSizeInflation(),
               "should not be container for font size inflation");

  nsIFrame::InlinePrefISizeData data;
  DISPLAY_PREF_WIDTH(aFrame, data.prevLines);
  aFrame->AddInlinePrefISize(aRenderingContext, &data);
  data.ForceBreak(aRenderingContext);
  return data.prevLines;
}

static nscolor
DarkenColor(nscolor aColor)
{
  uint16_t  hue, sat, value;
  uint8_t alpha;

  
  NS_RGB2HSV(aColor, hue, sat, value, alpha);

  
  
  
  
  
  
  if (value > sat) {
    value = sat;
    
    NS_HSV2RGB(aColor, hue, sat, value, alpha);
  }
  return aColor;
}




static bool
ShouldDarkenColors(nsPresContext* aPresContext)
{
  return !aPresContext->GetBackgroundColorDraw() &&
         !aPresContext->GetBackgroundImageDraw();
}

nscolor
nsLayoutUtils::GetColor(nsIFrame* aFrame, nsCSSProperty aProperty)
{
  nscolor color = aFrame->GetVisitedDependentColor(aProperty);
  if (ShouldDarkenColors(aFrame->PresContext())) {
    color = DarkenColor(color);
  }
  return color;
}

gfxFloat
nsLayoutUtils::GetSnappedBaselineY(nsIFrame* aFrame, gfxContext* aContext,
                                   nscoord aY, nscoord aAscent)
{
  gfxFloat appUnitsPerDevUnit = aFrame->PresContext()->AppUnitsPerDevPixel();
  gfxFloat baseline = gfxFloat(aY) + aAscent;
  gfxRect putativeRect(0, baseline/appUnitsPerDevUnit, 1, 1);
  if (!aContext->UserToDevicePixelSnapped(putativeRect, true))
    return baseline;
  return aContext->DeviceToUser(putativeRect.TopLeft()).y * appUnitsPerDevUnit;
}

gfxFloat
nsLayoutUtils::GetSnappedBaselineX(nsIFrame* aFrame, gfxContext* aContext,
                                   nscoord aX, nscoord aAscent)
{
  gfxFloat appUnitsPerDevUnit = aFrame->PresContext()->AppUnitsPerDevPixel();
  gfxFloat baseline = gfxFloat(aX) + aAscent;
  gfxRect putativeRect(baseline / appUnitsPerDevUnit, 0, 1, 1);
  if (!aContext->UserToDevicePixelSnapped(putativeRect, true)) {
    return baseline;
  }
  return aContext->DeviceToUser(putativeRect.TopLeft()).x * appUnitsPerDevUnit;
}



#define MAX_GFX_TEXT_BUF_SIZE 8000

static int32_t FindSafeLength(const char16_t *aString, uint32_t aLength,
                              uint32_t aMaxChunkLength)
{
  if (aLength <= aMaxChunkLength)
    return aLength;

  int32_t len = aMaxChunkLength;

  
  while (len > 0 && NS_IS_LOW_SURROGATE(aString[len])) {
    len--;
  }
  if (len == 0) {
    
    
    
    
    
    return aMaxChunkLength;
  }
  return len;
}

static int32_t GetMaxChunkLength(nsFontMetrics& aFontMetrics)
{
  return std::min(aFontMetrics.GetMaxStringLength(), MAX_GFX_TEXT_BUF_SIZE);
}

nscoord
nsLayoutUtils::AppUnitWidthOfString(const char16_t *aString,
                                    uint32_t aLength,
                                    nsFontMetrics& aFontMetrics,
                                    nsRenderingContext& aContext)
{
  uint32_t maxChunkLength = GetMaxChunkLength(aFontMetrics);
  nscoord width = 0;
  while (aLength > 0) {
    int32_t len = FindSafeLength(aString, aLength, maxChunkLength);
    width += aFontMetrics.GetWidth(aString, len, &aContext);
    aLength -= len;
    aString += len;
  }
  return width;
}

nscoord
nsLayoutUtils::AppUnitWidthOfStringBidi(const char16_t* aString,
                                        uint32_t aLength,
                                        const nsIFrame* aFrame,
                                        nsFontMetrics& aFontMetrics,
                                        nsRenderingContext& aContext)
{
  nsPresContext* presContext = aFrame->PresContext();
  if (presContext->BidiEnabled()) {
    nsBidiLevel level =
      nsBidiPresUtils::BidiLevelFromStyle(aFrame->StyleContext());
    return nsBidiPresUtils::MeasureTextWidth(aString, aLength, level,
                                             presContext, aContext,
                                             aFontMetrics);
  }
  aFontMetrics.SetTextRunRTL(false);
  aFontMetrics.SetVertical(aFrame->GetWritingMode().IsVertical());
  aFontMetrics.SetTextOrientation(aFrame->StyleVisibility()->mTextOrientation);
  return nsLayoutUtils::AppUnitWidthOfString(aString, aLength, aFontMetrics,
                                             aContext);
}

bool
nsLayoutUtils::StringWidthIsGreaterThan(const nsString& aString,
                                        nsFontMetrics& aFontMetrics,
                                        nsRenderingContext& aContext,
                                        nscoord aWidth)
{
  const char16_t *string = aString.get();
  uint32_t length = aString.Length();
  uint32_t maxChunkLength = GetMaxChunkLength(aFontMetrics);
  nscoord width = 0;
  while (length > 0) {
    int32_t len = FindSafeLength(string, length, maxChunkLength);
    width += aFontMetrics.GetWidth(string, len, &aContext);
    if (width > aWidth) {
      return true;
    }
    length -= len;
    string += len;
  }
  return false;
}

nsBoundingMetrics
nsLayoutUtils::AppUnitBoundsOfString(const char16_t* aString,
                                     uint32_t aLength,
                                     nsFontMetrics& aFontMetrics,
                                     nsRenderingContext& aContext)
{
  uint32_t maxChunkLength = GetMaxChunkLength(aFontMetrics);
  int32_t len = FindSafeLength(aString, aLength, maxChunkLength);
  
  
  
  nsBoundingMetrics totalMetrics =
    aFontMetrics.GetBoundingMetrics(aString, len, &aContext);
  aLength -= len;
  aString += len;

  while (aLength > 0) {
    len = FindSafeLength(aString, aLength, maxChunkLength);
    nsBoundingMetrics metrics =
      aFontMetrics.GetBoundingMetrics(aString, len, &aContext);
    totalMetrics += metrics;
    aLength -= len;
    aString += len;
  }
  return totalMetrics;
}

void
nsLayoutUtils::DrawString(const nsIFrame*       aFrame,
                          nsFontMetrics&        aFontMetrics,
                          nsRenderingContext*   aContext,
                          const char16_t*      aString,
                          int32_t               aLength,
                          nsPoint               aPoint,
                          nsStyleContext*       aStyleContext)
{
  nsresult rv = NS_ERROR_FAILURE;

  
  if (!aStyleContext) {
    aStyleContext = aFrame->StyleContext();
  }
  aFontMetrics.SetVertical(WritingMode(aStyleContext).IsVertical());
  aFontMetrics.SetTextOrientation(
    aStyleContext->StyleVisibility()->mTextOrientation);

  nsPresContext* presContext = aFrame->PresContext();
  if (presContext->BidiEnabled()) {
    nsBidiLevel level =
      nsBidiPresUtils::BidiLevelFromStyle(aStyleContext);
    rv = nsBidiPresUtils::RenderText(aString, aLength, level,
                                     presContext, *aContext, *aContext,
                                     aFontMetrics,
                                     aPoint.x, aPoint.y);
  }
  if (NS_FAILED(rv))
  {
    aFontMetrics.SetTextRunRTL(false);
    DrawUniDirString(aString, aLength, aPoint, aFontMetrics, *aContext);
  }
}

void
nsLayoutUtils::DrawUniDirString(const char16_t* aString,
                                uint32_t aLength,
                                nsPoint aPoint,
                                nsFontMetrics& aFontMetrics,
                                nsRenderingContext& aContext)
{
  nscoord x = aPoint.x;
  nscoord y = aPoint.y;

  uint32_t maxChunkLength = GetMaxChunkLength(aFontMetrics);
  if (aLength <= maxChunkLength) {
    aFontMetrics.DrawString(aString, aLength, x, y, &aContext, &aContext);
    return;
  }

  bool isRTL = aFontMetrics.GetTextRunRTL();

  
  if (isRTL) {
    x += nsLayoutUtils::AppUnitWidthOfString(aString, aLength, aFontMetrics,
                                             aContext);
  }

  while (aLength > 0) {
    int32_t len = FindSafeLength(aString, aLength, maxChunkLength);
    nscoord width = aFontMetrics.GetWidth(aString, len, &aContext);
    if (isRTL) {
      x -= width;
    }
    aFontMetrics.DrawString(aString, len, x, y, &aContext, &aContext);
    if (!isRTL) {
      x += width;
    }
    aLength -= len;
    aString += len;
  }
}

 void
nsLayoutUtils::PaintTextShadow(const nsIFrame* aFrame,
                               nsRenderingContext* aContext,
                               const nsRect& aTextRect,
                               const nsRect& aDirtyRect,
                               const nscolor& aForegroundColor,
                               TextShadowCallback aCallback,
                               void* aCallbackData)
{
  const nsStyleText* textStyle = aFrame->StyleText();
  if (!textStyle->HasTextShadow())
    return;

  
  
  gfxContext* aDestCtx = aContext->ThebesContext();
  for (uint32_t i = textStyle->mTextShadow->Length(); i > 0; --i) {
    nsCSSShadowItem* shadowDetails = textStyle->mTextShadow->ShadowAt(i - 1);
    nsPoint shadowOffset(shadowDetails->mXOffset,
                         shadowDetails->mYOffset);
    nscoord blurRadius = std::max(shadowDetails->mRadius, 0);

    nsRect shadowRect(aTextRect);
    shadowRect.MoveBy(shadowOffset);

    nsPresContext* presCtx = aFrame->PresContext();
    nsContextBoxBlur contextBoxBlur;
    gfxContext* shadowContext = contextBoxBlur.Init(shadowRect, 0, blurRadius,
                                                    presCtx->AppUnitsPerDevPixel(),
                                                    aDestCtx, aDirtyRect, nullptr);
    if (!shadowContext)
      continue;

    nscolor shadowColor;
    if (shadowDetails->mHasColor)
      shadowColor = shadowDetails->mColor;
    else
      shadowColor = aForegroundColor;

    
    
    nsRenderingContext renderingContext(shadowContext);

    aDestCtx->Save();
    aDestCtx->NewPath();
    aDestCtx->SetColor(gfxRGBA(shadowColor));

    
    aCallback(&renderingContext, shadowOffset, shadowColor, aCallbackData);

    contextBoxBlur.DoPaint();
    aDestCtx->Restore();
  }
}

 nscoord
nsLayoutUtils::GetCenteredFontBaseline(nsFontMetrics* aFontMetrics,
                                       nscoord        aLineHeight,
                                       bool           aIsInverted)
{
  nscoord fontAscent = aIsInverted ? aFontMetrics->MaxDescent()
                                   : aFontMetrics->MaxAscent();
  nscoord fontHeight = aFontMetrics->MaxHeight();

  nscoord leading = aLineHeight - fontHeight;
  return fontAscent + leading/2;
}


 bool
nsLayoutUtils::GetFirstLineBaseline(WritingMode aWritingMode,
                                    const nsIFrame* aFrame, nscoord* aResult)
{
  LinePosition position;
  if (!GetFirstLinePosition(aWritingMode, aFrame, &position))
    return false;
  *aResult = position.mBaseline;
  return true;
}

 bool
nsLayoutUtils::GetFirstLinePosition(WritingMode aWM,
                                    const nsIFrame* aFrame,
                                    LinePosition* aResult)
{
  const nsBlockFrame* block = nsLayoutUtils::GetAsBlock(const_cast<nsIFrame*>(aFrame));
  if (!block) {
    
    
    nsIAtom* fType = aFrame->GetType();
    if (fType == nsGkAtoms::tableOuterFrame) {
      aResult->mBStart = 0;
      aResult->mBaseline = aFrame->GetLogicalBaseline(aWM);
      
      
      aResult->mBEnd = aFrame->BSize(aWM);
      return true;
    }

    
    if (fType == nsGkAtoms::scrollFrame) {
      nsIScrollableFrame *sFrame = do_QueryFrame(const_cast<nsIFrame*>(aFrame));
      if (!sFrame) {
        NS_NOTREACHED("not scroll frame");
      }
      LinePosition kidPosition;
      if (GetFirstLinePosition(aWM,
                               sFrame->GetScrolledFrame(), &kidPosition)) {
        
        
        
        *aResult = kidPosition +
          aFrame->GetLogicalUsedBorderAndPadding(aWM).BStart(aWM);
        return true;
      }
      return false;
    }

    if (fType == nsGkAtoms::fieldSetFrame) {
      LinePosition kidPosition;
      nsIFrame* kid = aFrame->GetFirstPrincipalChild();
      
      if (GetFirstLinePosition(aWM, kid, &kidPosition)) {
        *aResult = kidPosition +
          kid->GetLogicalNormalPosition(aWM, aFrame->GetSize().width).B(aWM);
        return true;
      }
      return false;
    }

    
    return false;
  }

  for (nsBlockFrame::const_line_iterator line = block->begin_lines(),
                                     line_end = block->end_lines();
       line != line_end; ++line) {
    if (line->IsBlock()) {
      nsIFrame *kid = line->mFirstChild;
      LinePosition kidPosition;
      if (GetFirstLinePosition(aWM, kid, &kidPosition)) {
        
        
        
        nscoord containerWidth = line->mContainerWidth;
        *aResult = kidPosition +
                   kid->GetLogicalNormalPosition(aWM, containerWidth).B(aWM);
        return true;
      }
    } else {
      
      
      if (line->BSize() != 0 || !line->IsEmpty()) {
        nscoord bStart = line->BStart();
        aResult->mBStart = bStart;
        aResult->mBaseline = bStart + line->GetLogicalAscent();
        aResult->mBEnd = bStart + line->BSize();
        return true;
      }
    }
  }
  return false;
}

 bool
nsLayoutUtils::GetLastLineBaseline(WritingMode aWM,
                                   const nsIFrame* aFrame, nscoord* aResult)
{
  const nsBlockFrame* block = nsLayoutUtils::GetAsBlock(const_cast<nsIFrame*>(aFrame));
  if (!block)
    
    return false;

  for (nsBlockFrame::const_reverse_line_iterator line = block->rbegin_lines(),
                                             line_end = block->rend_lines();
       line != line_end; ++line) {
    if (line->IsBlock()) {
      nsIFrame *kid = line->mFirstChild;
      nscoord kidBaseline;
      nscoord containerWidth = line->mContainerWidth;
      if (GetLastLineBaseline(aWM, kid, &kidBaseline)) {
        
        *aResult = kidBaseline +
          kid->GetLogicalNormalPosition(aWM, containerWidth).B(aWM);
        return true;
      } else if (kid->GetType() == nsGkAtoms::scrollFrame) {
        
        
        *aResult = kid->GetLogicalNormalPosition(aWM, containerWidth).B(aWM) +
                   kid->BSize(aWM);
        return true;
      }
    } else {
      
      
      if (line->BSize() != 0 || !line->IsEmpty()) {
        *aResult = line->BStart() + line->GetLogicalAscent();
        return true;
      }
    }
  }
  return false;
}

static nscoord
CalculateBlockContentBEnd(WritingMode aWM, nsBlockFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");

  nscoord contentBEnd = 0;

  for (nsBlockFrame::line_iterator line = aFrame->begin_lines(),
                                   line_end = aFrame->end_lines();
       line != line_end; ++line) {
    if (line->IsBlock()) {
      nsIFrame* child = line->mFirstChild;
      nscoord containerWidth = line->mContainerWidth;
      nscoord offset =
        child->GetLogicalNormalPosition(aWM, containerWidth).B(aWM);
      contentBEnd =
        std::max(contentBEnd,
                 nsLayoutUtils::CalculateContentBEnd(aWM, child) + offset);
    }
    else {
      contentBEnd = std::max(contentBEnd, line->BEnd());
    }
  }
  return contentBEnd;
}

 nscoord
nsLayoutUtils::CalculateContentBEnd(WritingMode aWM, nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame, "null ptr");

  nscoord contentBEnd = aFrame->BSize(aWM);

  
  
  LogicalSize overflowSize(aWM, aFrame->GetScrollableOverflowRect().Size());
  if (overflowSize.BSize(aWM) > contentBEnd) {
    nsIFrame::ChildListIDs skip(nsIFrame::kOverflowList |
                                nsIFrame::kExcessOverflowContainersList |
                                nsIFrame::kOverflowOutOfFlowList);
    nsBlockFrame* blockFrame = GetAsBlock(aFrame);
    if (blockFrame) {
      contentBEnd =
        std::max(contentBEnd, CalculateBlockContentBEnd(aWM, blockFrame));
      skip |= nsIFrame::kPrincipalList;
    }
    nsIFrame::ChildListIterator lists(aFrame);
    for (; !lists.IsDone(); lists.Next()) {
      if (!skip.Contains(lists.CurrentID())) {
        nsFrameList::Enumerator childFrames(lists.CurrentList()); 
        for (; !childFrames.AtEnd(); childFrames.Next()) {
          nsIFrame* child = childFrames.get();
          nscoord offset =
            child->GetLogicalNormalPosition(aWM,
                                            aFrame->GetSize().width).B(aWM);
          contentBEnd = std::max(contentBEnd,
                                 CalculateContentBEnd(aWM, child) + offset);
        }
      }
    }
  }
  return contentBEnd;
}

 nsIFrame*
nsLayoutUtils::GetClosestLayer(nsIFrame* aFrame)
{
  nsIFrame* layer;
  for (layer = aFrame; layer; layer = layer->GetParent()) {
    if (layer->IsAbsPosContaininingBlock() ||
        (layer->GetParent() &&
          layer->GetParent()->GetType() == nsGkAtoms::scrollFrame))
      break;
  }
  if (layer)
    return layer;
  return aFrame->PresContext()->PresShell()->FrameManager()->GetRootFrame();
}

GraphicsFilter
nsLayoutUtils::GetGraphicsFilterForFrame(nsIFrame* aForFrame)
{
  GraphicsFilter defaultFilter = GraphicsFilter::FILTER_GOOD;
  nsStyleContext *sc;
  if (nsCSSRendering::IsCanvasFrame(aForFrame)) {
    nsCSSRendering::FindBackground(aForFrame, &sc);
  } else {
    sc = aForFrame->StyleContext();
  }

  switch (sc->StyleSVG()->mImageRendering) {
  case NS_STYLE_IMAGE_RENDERING_OPTIMIZESPEED:
    return GraphicsFilter::FILTER_FAST;
  case NS_STYLE_IMAGE_RENDERING_OPTIMIZEQUALITY:
    return GraphicsFilter::FILTER_BEST;
  case NS_STYLE_IMAGE_RENDERING_CRISPEDGES:
    return GraphicsFilter::FILTER_NEAREST;
  default:
    return defaultFilter;
  }
}









static gfxPoint
MapToFloatImagePixels(const gfxSize& aSize,
                      const gfxRect& aDest, const gfxPoint& aPt)
{
  return gfxPoint(((aPt.x - aDest.X())*aSize.width)/aDest.Width(),
                  ((aPt.y - aDest.Y())*aSize.height)/aDest.Height());
}









static gfxPoint
MapToFloatUserPixels(const gfxSize& aSize,
                     const gfxRect& aDest, const gfxPoint& aPt)
{
  return gfxPoint(aPt.x*aDest.Width()/aSize.width + aDest.X(),
                  aPt.y*aDest.Height()/aSize.height + aDest.Y());
}

 gfxRect
nsLayoutUtils::RectToGfxRect(const nsRect& aRect, int32_t aAppUnitsPerDevPixel)
{
  return gfxRect(gfxFloat(aRect.x) / aAppUnitsPerDevPixel,
                 gfxFloat(aRect.y) / aAppUnitsPerDevPixel,
                 gfxFloat(aRect.width) / aAppUnitsPerDevPixel,
                 gfxFloat(aRect.height) / aAppUnitsPerDevPixel);
}

struct SnappedImageDrawingParameters {
  
  gfxMatrix imageSpaceToDeviceSpace;
  
  
  nsIntSize size;
  
  
  ImageRegion region;
  
  
  
  
  CSSIntSize svgViewportSize;
  
  bool shouldDraw;

  SnappedImageDrawingParameters()
   : region(ImageRegion::Empty())
   , shouldDraw(false)
  {}

  SnappedImageDrawingParameters(const gfxMatrix&   aImageSpaceToDeviceSpace,
                                const nsIntSize&   aSize,
                                const ImageRegion& aRegion,
                                const CSSIntSize&  aSVGViewportSize)
   : imageSpaceToDeviceSpace(aImageSpaceToDeviceSpace)
   , size(aSize)
   , region(aRegion)
   , svgViewportSize(aSVGViewportSize)
   , shouldDraw(true)
  {}
};








static gfxMatrix
TransformBetweenRects(const gfxRect& aFrom, const gfxRect& aTo)
{
  gfxSize scale(aTo.width / aFrom.width,
                aTo.height / aFrom.height);
  gfxPoint translation(aTo.x - aFrom.x * scale.width,
                       aTo.y - aFrom.y * scale.height);
  return gfxMatrix(scale.width, 0, 0, scale.height,
                   translation.x, translation.y);
}

static nsRect
TileNearRect(const nsRect& aAnyTile, const nsRect& aTargetRect)
{
  nsPoint distance = aTargetRect.TopLeft() - aAnyTile.TopLeft();
  return aAnyTile + nsPoint(distance.x / aAnyTile.width * aAnyTile.width,
                            distance.y / aAnyTile.height * aAnyTile.height);
}

static gfxFloat
StableRound(gfxFloat aValue)
{
  
  
  return floor(aValue + 0.5001);
}

static gfxPoint
StableRound(const gfxPoint& aPoint)
{
  return gfxPoint(StableRound(aPoint.x), StableRound(aPoint.y));
}








static SnappedImageDrawingParameters
ComputeSnappedImageDrawingParameters(gfxContext*     aCtx,
                                     int32_t         aAppUnitsPerDevPixel,
                                     const nsRect    aDest,
                                     const nsRect    aFill,
                                     const nsPoint   aAnchor,
                                     const nsRect    aDirty,
                                     imgIContainer*  aImage,
                                     GraphicsFilter  aGraphicsFilter,
                                     uint32_t        aImageFlags)
{
  if (aDest.IsEmpty() || aFill.IsEmpty())
    return SnappedImageDrawingParameters();

  
  bool doTile = !aDest.Contains(aFill);
  nsRect dest = doTile ? TileNearRect(aDest, aFill.Intersect(aDirty)) : aDest;
  nsPoint anchor = aAnchor + (dest.TopLeft() - aDest.TopLeft());

  gfxRect devPixelDest =
    nsLayoutUtils::RectToGfxRect(dest, aAppUnitsPerDevPixel);
  gfxRect devPixelFill =
    nsLayoutUtils::RectToGfxRect(aFill, aAppUnitsPerDevPixel);
  gfxRect devPixelDirty =
    nsLayoutUtils::RectToGfxRect(aDirty, aAppUnitsPerDevPixel);

  gfxMatrix currentMatrix = aCtx->CurrentMatrix();
  gfxRect fill = devPixelFill;
  bool didSnap;
  
  
  
  if (!currentMatrix.HasNonAxisAlignedTransform() &&
      currentMatrix._11 > 0.0 && currentMatrix._22 > 0.0 &&
      aCtx->UserToDevicePixelSnapped(fill, true)) {
    didSnap = true;
    if (fill.IsEmpty()) {
      return SnappedImageDrawingParameters();
    }
  } else {
    didSnap = false;
    fill = devPixelFill;
  }

  
  gfxSize destScale = didSnap ? gfxSize(currentMatrix._11, currentMatrix._22)
                              : currentMatrix.ScaleFactors(true);
  gfxSize appUnitScaledDest(dest.width * destScale.width,
                            dest.height * destScale.height);
  gfxSize scaledDest = appUnitScaledDest / aAppUnitsPerDevPixel;
  if (scaledDest.IsEmpty()) {
    return SnappedImageDrawingParameters();
  }

  
  
  
  
  gfxSize snappedScaledDest =
    gfxSize(NSAppUnitsToIntPixels(appUnitScaledDest.width, aAppUnitsPerDevPixel),
            NSAppUnitsToIntPixels(appUnitScaledDest.height, aAppUnitsPerDevPixel));
  snappedScaledDest.width = std::max(snappedScaledDest.width, 1.0);
  snappedScaledDest.height = std::max(snappedScaledDest.height, 1.0);

  nsIntSize intImageSize =
    aImage->OptimalImageSizeForDest(snappedScaledDest,
                                    imgIContainer::FRAME_CURRENT,
                                    aGraphicsFilter, aImageFlags);
  gfxSize imageSize(intImageSize.width, intImageSize.height);

  CSSIntSize svgViewportSize = currentMatrix.IsIdentity()
    ? CSSIntSize(intImageSize.width, intImageSize.height)
    : CSSIntSize(NSAppUnitsToIntPixels(dest.width, aAppUnitsPerDevPixel), 
                 NSAppUnitsToIntPixels(dest.height, aAppUnitsPerDevPixel)); 

  
  gfxPoint subimageTopLeft =
    MapToFloatImagePixels(imageSize, devPixelDest, devPixelFill.TopLeft());
  gfxPoint subimageBottomRight =
    MapToFloatImagePixels(imageSize, devPixelDest, devPixelFill.BottomRight());
  gfxRect subimage;
  subimage.MoveTo(NSToIntFloor(subimageTopLeft.x),
                  NSToIntFloor(subimageTopLeft.y));
  subimage.SizeTo(NSToIntCeil(subimageBottomRight.x) - subimage.x,
                  NSToIntCeil(subimageBottomRight.y) - subimage.y);

  gfxMatrix transform;
  gfxMatrix invTransform;

  bool anchorAtUpperLeft = anchor.x == dest.x && anchor.y == dest.y;
  bool exactlyOneImageCopy = aFill.IsEqualEdges(dest);
  if (anchorAtUpperLeft && exactlyOneImageCopy) {
    
    
    
    
    transform = TransformBetweenRects(subimage, fill);
    invTransform = TransformBetweenRects(fill, subimage);
  } else {
    
    
    

    
    
    gfxPoint anchorPoint(gfxFloat(anchor.x)/aAppUnitsPerDevPixel,
                         gfxFloat(anchor.y)/aAppUnitsPerDevPixel);
    gfxPoint imageSpaceAnchorPoint =
      MapToFloatImagePixels(imageSize, devPixelDest, anchorPoint);

    if (didSnap) {
      imageSpaceAnchorPoint = StableRound(imageSpaceAnchorPoint);
      anchorPoint = imageSpaceAnchorPoint;
      anchorPoint = MapToFloatUserPixels(imageSize, devPixelDest, anchorPoint);
      anchorPoint = currentMatrix.Transform(anchorPoint);
      anchorPoint = StableRound(anchorPoint);
    }

    gfxRect anchoredDestRect(anchorPoint, scaledDest);
    gfxRect anchoredImageRect(imageSpaceAnchorPoint, imageSize);

    
    
    if (fill.Width() != devPixelFill.Width() &&
        devPixelDest.x == devPixelFill.x &&
        devPixelDest.XMost() == devPixelFill.XMost()) {
      anchoredDestRect.width = fill.width;
    }
    if (fill.Height() != devPixelFill.Height() &&
        devPixelDest.y == devPixelFill.y &&
        devPixelDest.YMost() == devPixelFill.YMost()) {
      anchoredDestRect.height = fill.height;
    }

    transform = TransformBetweenRects(anchoredImageRect, anchoredDestRect);
    invTransform = TransformBetweenRects(anchoredDestRect, anchoredImageRect);
  }

  
  
  
  
  
  
  if (didSnap && !invTransform.HasNonIntegerTranslation()) {
    
    
    devPixelDirty = currentMatrix.Transform(devPixelDirty);
    devPixelDirty.RoundOut();
    fill = fill.Intersect(devPixelDirty);
  }
  if (fill.IsEmpty())
    return SnappedImageDrawingParameters();

  gfxRect imageSpaceFill(didSnap ? invTransform.Transform(fill)
                                 : invTransform.TransformBounds(fill));

  
  
  
  if (!didSnap) {
    transform = transform * currentMatrix;
  }

  ImageRegion region =
    ImageRegion::CreateWithSamplingRestriction(imageSpaceFill, subimage);

  return SnappedImageDrawingParameters(transform, intImageSize,
                                       region, svgViewportSize);
}


static DrawResult
DrawImageInternal(gfxContext&            aContext,
                  nsPresContext*         aPresContext,
                  imgIContainer*         aImage,
                  GraphicsFilter         aGraphicsFilter,
                  const nsRect&          aDest,
                  const nsRect&          aFill,
                  const nsPoint&         aAnchor,
                  const nsRect&          aDirty,
                  const SVGImageContext* aSVGContext,
                  uint32_t               aImageFlags)
{
  if (aPresContext->Type() == nsPresContext::eContext_Print) {
    
    
    aImageFlags |= imgIContainer::FLAG_BYPASS_SURFACE_CACHE;
  }
  if (aDest.Contains(aFill)) {
    aImageFlags |= imgIContainer::FLAG_CLAMP;
  }
  int32_t appUnitsPerDevPixel =
   aPresContext->AppUnitsPerDevPixel();

  SnappedImageDrawingParameters params =
    ComputeSnappedImageDrawingParameters(&aContext, appUnitsPerDevPixel, aDest,
                                         aFill, aAnchor, aDirty, aImage,
                                         aGraphicsFilter, aImageFlags);

  if (!params.shouldDraw) {
    return DrawResult::SUCCESS;
  }

  gfxContextMatrixAutoSaveRestore contextMatrixRestorer(&aContext);
  aContext.SetMatrix(params.imageSpaceToDeviceSpace);

  Maybe<SVGImageContext> svgContext = ToMaybe(aSVGContext);
  if (!svgContext) {
    
    svgContext = Some(SVGImageContext(params.svgViewportSize, Nothing()));
  }

  return aImage->Draw(&aContext, params.size, params.region,
                      imgIContainer::FRAME_CURRENT, aGraphicsFilter,
                      svgContext, aImageFlags);
}

 DrawResult
nsLayoutUtils::DrawSingleUnscaledImage(gfxContext&          aContext,
                                       nsPresContext*       aPresContext,
                                       imgIContainer*       aImage,
                                       GraphicsFilter       aGraphicsFilter,
                                       const nsPoint&       aDest,
                                       const nsRect*        aDirty,
                                       uint32_t             aImageFlags,
                                       const nsRect*        aSourceArea)
{
  CSSIntSize imageSize;
  aImage->GetWidth(&imageSize.width);
  aImage->GetHeight(&imageSize.height);
  if (imageSize.width < 1 || imageSize.height < 1) {
    NS_WARNING("Image width or height is non-positive");
    return DrawResult::TEMPORARY_ERROR;
  }

  nsSize size(CSSPixel::ToAppUnits(imageSize));
  nsRect source;
  if (aSourceArea) {
    source = *aSourceArea;
  } else {
    source.SizeTo(size);
  }

  nsRect dest(aDest - source.TopLeft(), size);
  nsRect fill(aDest, source.Size());
  
  
  
  fill.IntersectRect(fill, dest);
  return DrawImageInternal(aContext, aPresContext,
                           aImage, aGraphicsFilter,
                           dest, fill, aDest, aDirty ? *aDirty : dest,
                           nullptr, aImageFlags);
}

 DrawResult
nsLayoutUtils::DrawSingleImage(gfxContext&            aContext,
                               nsPresContext*         aPresContext,
                               imgIContainer*         aImage,
                               GraphicsFilter         aGraphicsFilter,
                               const nsRect&          aDest,
                               const nsRect&          aDirty,
                               const SVGImageContext* aSVGContext,
                               uint32_t               aImageFlags,
                               const nsPoint*         aAnchorPoint,
                               const nsRect*          aSourceArea)
{
  nscoord appUnitsPerCSSPixel = nsDeviceContext::AppUnitsPerCSSPixel();
  CSSIntSize pixelImageSize(ComputeSizeForDrawingWithFallback(aImage, aDest.Size()));
  if (pixelImageSize.width < 1 || pixelImageSize.height < 1) {
    NS_WARNING("Image width or height is non-positive");
    return DrawResult::TEMPORARY_ERROR;
  }

  nsSize imageSize(CSSPixel::ToAppUnits(pixelImageSize));
  nsRect source;
  nsCOMPtr<imgIContainer> image;
  if (aSourceArea) {
    source = *aSourceArea;
    nsIntRect subRect(source.x, source.y, source.width, source.height);
    subRect.ScaleInverseRoundOut(appUnitsPerCSSPixel);
    image = ImageOps::Clip(aImage, subRect);

    nsRect imageRect;
    imageRect.SizeTo(imageSize);
    nsRect clippedSource = imageRect.Intersect(source);

    source -= clippedSource.TopLeft();
    imageSize = clippedSource.Size();
  } else {
    source.SizeTo(imageSize);
    image = aImage;
  }

  nsRect dest = GetWholeImageDestination(imageSize, source, aDest);

  
  
  
  nsRect fill;
  fill.IntersectRect(aDest, dest);
  return DrawImageInternal(aContext, aPresContext, image,
                           aGraphicsFilter, dest, fill,
                           aAnchorPoint ? *aAnchorPoint : fill.TopLeft(),
                           aDirty, aSVGContext, aImageFlags);
}

 void
nsLayoutUtils::ComputeSizeForDrawing(imgIContainer *aImage,
                                     CSSIntSize&    aImageSize, 
                                     nsSize&        aIntrinsicRatio, 
                                     bool&          aGotWidth,  
                                     bool&          aGotHeight  )
{
  aGotWidth  = NS_SUCCEEDED(aImage->GetWidth(&aImageSize.width));
  aGotHeight = NS_SUCCEEDED(aImage->GetHeight(&aImageSize.height));
  bool gotRatio = NS_SUCCEEDED(aImage->GetIntrinsicRatio(&aIntrinsicRatio));

  if (!(aGotWidth && aGotHeight) && !gotRatio) {
    
    
    aGotWidth = aGotHeight = true;
    aImageSize = CSSIntSize(0, 0);
    aIntrinsicRatio = nsSize(0, 0);
  }
}

 CSSIntSize
nsLayoutUtils::ComputeSizeForDrawingWithFallback(imgIContainer* aImage,
                                                 const nsSize&  aFallbackSize)
{
  CSSIntSize imageSize;
  nsSize imageRatio;
  bool gotHeight, gotWidth;
  ComputeSizeForDrawing(aImage, imageSize, imageRatio, gotWidth, gotHeight);

  
  
  if (gotWidth != gotHeight) {
    if (!gotWidth) {
      if (imageRatio.height != 0) {
        imageSize.width =
          NSCoordSaturatingNonnegativeMultiply(imageSize.height,
                                               float(imageRatio.width) /
                                               float(imageRatio.height));
        gotWidth = true;
      }
    } else {
      if (imageRatio.width != 0) {
        imageSize.height =
          NSCoordSaturatingNonnegativeMultiply(imageSize.width,
                                               float(imageRatio.height) /
                                               float(imageRatio.width));
        gotHeight = true;
      }
    }
  }

  
  
  if (!gotWidth) {
    imageSize.width = nsPresContext::AppUnitsToIntCSSPixels(aFallbackSize.width);
  }
  if (!gotHeight) {
    imageSize.height = nsPresContext::AppUnitsToIntCSSPixels(aFallbackSize.height);
  }

  return imageSize;
}

 DrawResult
nsLayoutUtils::DrawBackgroundImage(gfxContext&         aContext,
                                   nsPresContext*      aPresContext,
                                   imgIContainer*      aImage,
                                   const CSSIntSize&   aImageSize,
                                   GraphicsFilter      aGraphicsFilter,
                                   const nsRect&       aDest,
                                   const nsRect&       aFill,
                                   const nsPoint&      aAnchor,
                                   const nsRect&       aDirty,
                                   uint32_t            aImageFlags)
{
  PROFILER_LABEL("layout", "nsLayoutUtils::DrawBackgroundImage",
                 js::ProfileEntry::Category::GRAPHICS);

  if (UseBackgroundNearestFiltering()) {
    aGraphicsFilter = GraphicsFilter::FILTER_NEAREST;
  }

  SVGImageContext svgContext(aImageSize, Nothing());

  return DrawImageInternal(aContext, aPresContext, aImage,
                           aGraphicsFilter, aDest, aFill, aAnchor,
                           aDirty, &svgContext, aImageFlags);
}

 DrawResult
nsLayoutUtils::DrawImage(gfxContext&         aContext,
                         nsPresContext*      aPresContext,
                         imgIContainer*      aImage,
                         GraphicsFilter      aGraphicsFilter,
                         const nsRect&       aDest,
                         const nsRect&       aFill,
                         const nsPoint&      aAnchor,
                         const nsRect&       aDirty,
                         uint32_t            aImageFlags)
{
  return DrawImageInternal(aContext, aPresContext, aImage,
                           aGraphicsFilter, aDest, aFill, aAnchor,
                           aDirty, nullptr, aImageFlags);
}

 nsRect
nsLayoutUtils::GetWholeImageDestination(const nsSize& aWholeImageSize,
                                        const nsRect& aImageSourceArea,
                                        const nsRect& aDestArea)
{
  double scaleX = double(aDestArea.width)/aImageSourceArea.width;
  double scaleY = double(aDestArea.height)/aImageSourceArea.height;
  nscoord destOffsetX = NSToCoordRound(aImageSourceArea.x*scaleX);
  nscoord destOffsetY = NSToCoordRound(aImageSourceArea.y*scaleY);
  nscoord wholeSizeX = NSToCoordRound(aWholeImageSize.width*scaleX);
  nscoord wholeSizeY = NSToCoordRound(aWholeImageSize.height*scaleY);
  return nsRect(aDestArea.TopLeft() - nsPoint(destOffsetX, destOffsetY),
                nsSize(wholeSizeX, wholeSizeY));
}

 already_AddRefed<imgIContainer>
nsLayoutUtils::OrientImage(imgIContainer* aContainer,
                           const nsStyleImageOrientation& aOrientation)
{
  MOZ_ASSERT(aContainer, "Should have an image container");
  nsCOMPtr<imgIContainer> img(aContainer);

  if (aOrientation.IsFromImage()) {
    img = ImageOps::Orient(img, img->GetOrientation());
  } else if (!aOrientation.IsDefault()) {
    Angle angle = aOrientation.Angle();
    Flip flip  = aOrientation.IsFlipped() ? Flip::Horizontal
                                          : Flip::Unflipped;
    img = ImageOps::Orient(img, Orientation(angle, flip));
  }

  return img.forget();
}

static bool NonZeroStyleCoord(const nsStyleCoord& aCoord)
{
  if (aCoord.IsCoordPercentCalcUnit()) {
    
    return nsRuleNode::ComputeCoordPercentCalc(aCoord, nscoord_MAX) > 0 ||
           nsRuleNode::ComputeCoordPercentCalc(aCoord, 0) > 0;
  }

  return true;
}

 bool
nsLayoutUtils::HasNonZeroCorner(const nsStyleCorners& aCorners)
{
  NS_FOR_CSS_HALF_CORNERS(corner) {
    if (NonZeroStyleCoord(aCorners.Get(corner)))
      return true;
  }
  return false;
}


static bool IsCornerAdjacentToSide(uint8_t aCorner, css::Side aSide)
{
  PR_STATIC_ASSERT((int)NS_SIDE_TOP == NS_CORNER_TOP_LEFT);
  PR_STATIC_ASSERT((int)NS_SIDE_RIGHT == NS_CORNER_TOP_RIGHT);
  PR_STATIC_ASSERT((int)NS_SIDE_BOTTOM == NS_CORNER_BOTTOM_RIGHT);
  PR_STATIC_ASSERT((int)NS_SIDE_LEFT == NS_CORNER_BOTTOM_LEFT);
  PR_STATIC_ASSERT((int)NS_SIDE_TOP == ((NS_CORNER_TOP_RIGHT - 1)&3));
  PR_STATIC_ASSERT((int)NS_SIDE_RIGHT == ((NS_CORNER_BOTTOM_RIGHT - 1)&3));
  PR_STATIC_ASSERT((int)NS_SIDE_BOTTOM == ((NS_CORNER_BOTTOM_LEFT - 1)&3));
  PR_STATIC_ASSERT((int)NS_SIDE_LEFT == ((NS_CORNER_TOP_LEFT - 1)&3));

  return aSide == aCorner || aSide == ((aCorner - 1)&3);
}

 bool
nsLayoutUtils::HasNonZeroCornerOnSide(const nsStyleCorners& aCorners,
                                      css::Side aSide)
{
  PR_STATIC_ASSERT(NS_CORNER_TOP_LEFT_X/2 == NS_CORNER_TOP_LEFT);
  PR_STATIC_ASSERT(NS_CORNER_TOP_LEFT_Y/2 == NS_CORNER_TOP_LEFT);
  PR_STATIC_ASSERT(NS_CORNER_TOP_RIGHT_X/2 == NS_CORNER_TOP_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_TOP_RIGHT_Y/2 == NS_CORNER_TOP_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_RIGHT_X/2 == NS_CORNER_BOTTOM_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_RIGHT_Y/2 == NS_CORNER_BOTTOM_RIGHT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_LEFT_X/2 == NS_CORNER_BOTTOM_LEFT);
  PR_STATIC_ASSERT(NS_CORNER_BOTTOM_LEFT_Y/2 == NS_CORNER_BOTTOM_LEFT);

  NS_FOR_CSS_HALF_CORNERS(corner) {
    
    
    if (NonZeroStyleCoord(aCorners.Get(corner)) &&
        IsCornerAdjacentToSide(corner/2, aSide))
      return true;
  }
  return false;
}

 nsTransparencyMode
nsLayoutUtils::GetFrameTransparency(nsIFrame* aBackgroundFrame,
                                    nsIFrame* aCSSRootFrame) {
  if (aCSSRootFrame->StyleDisplay()->mOpacity < 1.0f)
    return eTransparencyTransparent;

  if (HasNonZeroCorner(aCSSRootFrame->StyleBorder()->mBorderRadius))
    return eTransparencyTransparent;

  if (aCSSRootFrame->StyleDisplay()->mAppearance == NS_THEME_WIN_GLASS)
    return eTransparencyGlass;

  if (aCSSRootFrame->StyleDisplay()->mAppearance == NS_THEME_WIN_BORDERLESS_GLASS)
    return eTransparencyBorderlessGlass;

  nsITheme::Transparency transparency;
  if (aCSSRootFrame->IsThemed(&transparency))
    return transparency == nsITheme::eTransparent
         ? eTransparencyTransparent
         : eTransparencyOpaque;

  
  
  
  if (aBackgroundFrame->GetType() == nsGkAtoms::viewportFrame &&
      !aBackgroundFrame->GetFirstPrincipalChild()) {
    return eTransparencyOpaque;
  }

  nsStyleContext* bgSC;
  if (!nsCSSRendering::FindBackground(aBackgroundFrame, &bgSC)) {
    return eTransparencyTransparent;
  }
  const nsStyleBackground* bg = bgSC->StyleBackground();
  if (NS_GET_A(bg->mBackgroundColor) < 255 ||
      
      bg->BottomLayer().mClip != NS_STYLE_BG_CLIP_BORDER)
    return eTransparencyTransparent;
  return eTransparencyOpaque;
}

static bool IsPopupFrame(nsIFrame* aFrame)
{
  
  nsIAtom* frameType = aFrame->GetType();
  if (frameType == nsGkAtoms::listControlFrame) {
    nsListControlFrame* lcf = static_cast<nsListControlFrame*>(aFrame);
    return lcf->IsInDropDownMode();
  }

  
  return frameType == nsGkAtoms::menuPopupFrame;
}

 bool
nsLayoutUtils::IsPopup(nsIFrame* aFrame)
{
  
  if (!aFrame->HasView()) {
    NS_ASSERTION(!IsPopupFrame(aFrame), "popup frame must have a view");
    return false;
  }
  return IsPopupFrame(aFrame);
}

 nsIFrame*
nsLayoutUtils::GetDisplayRootFrame(nsIFrame* aFrame)
{
  
  
  nsIFrame* f = aFrame;
  for (;;) {
    if (!f->HasAnyStateBits(NS_FRAME_IN_POPUP)) {
      f = f->PresContext()->FrameManager()->GetRootFrame();
    } else if (IsPopup(f)) {
      return f;
    }
    nsIFrame* parent = GetCrossDocParentFrame(f);
    if (!parent)
      return f;
    f = parent;
  }
}

 nsIFrame*
nsLayoutUtils::GetReferenceFrame(nsIFrame* aFrame)
{
  nsIFrame *f = aFrame;
  for (;;) {
    if (f->IsTransformed() || IsPopup(f)) {
      return f;
    }
    nsIFrame* parent = GetCrossDocParentFrame(f);
    if (!parent) {
      return f;
    }
    f = parent;
  }
}

 nsIFrame*
nsLayoutUtils::GetTransformRootFrame(nsIFrame* aFrame)
{
  nsIFrame *parent = nsLayoutUtils::GetCrossDocParentFrame(aFrame);
  while (parent && parent->Preserves3DChildren()) {
    parent = nsLayoutUtils::GetCrossDocParentFrame(parent);
  }
  return parent;
}

 uint32_t
nsLayoutUtils::GetTextRunFlagsForStyle(nsStyleContext* aStyleContext,
                                       const nsStyleFont* aStyleFont,
                                       const nsStyleText* aStyleText,
                                       nscoord aLetterSpacing)
{
  uint32_t result = 0;
  if (aLetterSpacing != 0) {
    result |= gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES;
  }
  if (aStyleText->mControlCharacterVisibility == NS_STYLE_CONTROL_CHARACTER_VISIBILITY_HIDDEN) {
    result |= gfxTextRunFactory::TEXT_HIDE_CONTROL_CHARACTERS;
  }
  switch (aStyleContext->StyleSVG()->mTextRendering) {
  case NS_STYLE_TEXT_RENDERING_OPTIMIZESPEED:
    result |= gfxTextRunFactory::TEXT_OPTIMIZE_SPEED;
    break;
  case NS_STYLE_TEXT_RENDERING_AUTO:
    if (aStyleFont->mFont.size <
        aStyleContext->PresContext()->GetAutoQualityMinFontSize()) {
      result |= gfxTextRunFactory::TEXT_OPTIMIZE_SPEED;
    }
    break;
  default:
    break;
  }
  WritingMode wm(aStyleContext);
  if (wm.IsVertical()) {
    switch (aStyleContext->StyleVisibility()->mTextOrientation) {
    case NS_STYLE_TEXT_ORIENTATION_MIXED:
      result |= gfxTextRunFactory::TEXT_ORIENT_VERTICAL_MIXED;
      break;
    case NS_STYLE_TEXT_ORIENTATION_UPRIGHT:
      result |= gfxTextRunFactory::TEXT_ORIENT_VERTICAL_UPRIGHT;
      break;
    case NS_STYLE_TEXT_ORIENTATION_SIDEWAYS:
      
      
      
      







    case NS_STYLE_TEXT_ORIENTATION_SIDEWAYS_RIGHT:
      result |= gfxTextRunFactory::TEXT_ORIENT_VERTICAL_SIDEWAYS_RIGHT;
      break;
    case NS_STYLE_TEXT_ORIENTATION_SIDEWAYS_LEFT:
      
      



    default:
      NS_NOTREACHED("unknown text-orientation");
      break;
    }
  }
  return result;
}

 void
nsLayoutUtils::GetRectDifferenceStrips(const nsRect& aR1, const nsRect& aR2,
                                       nsRect* aHStrip, nsRect* aVStrip) {
  NS_ASSERTION(aR1.TopLeft() == aR2.TopLeft(),
               "expected rects at the same position");
  nsRect unionRect(aR1.x, aR1.y, std::max(aR1.width, aR2.width),
                   std::max(aR1.height, aR2.height));
  nscoord VStripStart = std::min(aR1.width, aR2.width);
  nscoord HStripStart = std::min(aR1.height, aR2.height);
  *aVStrip = unionRect;
  aVStrip->x += VStripStart;
  aVStrip->width -= VStripStart;
  *aHStrip = unionRect;
  aHStrip->y += HStripStart;
  aHStrip->height -= HStripStart;
}

nsDeviceContext*
nsLayoutUtils::GetDeviceContextForScreenInfo(nsPIDOMWindow* aWindow)
{
  if (!aWindow) {
    return nullptr;
  }

  nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell();
  while (docShell) {
    
    
    
    
    nsCOMPtr<nsPIDOMWindow> win = docShell->GetWindow();
    if (!win) {
      
      return nullptr;
    }

    win->EnsureSizeUpToDate();

    nsRefPtr<nsPresContext> presContext;
    docShell->GetPresContext(getter_AddRefs(presContext));
    if (presContext) {
      nsDeviceContext* context = presContext->DeviceContext();
      if (context) {
        return context;
      }
    }

    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    docShell->GetParent(getter_AddRefs(parentItem));
    docShell = do_QueryInterface(parentItem);
  }

  return nullptr;
}

 bool
nsLayoutUtils::IsReallyFixedPos(nsIFrame* aFrame)
{
  NS_PRECONDITION(aFrame->GetParent(),
                  "IsReallyFixedPos called on frame not in tree");
  NS_PRECONDITION(aFrame->StyleDisplay()->mPosition ==
                    NS_STYLE_POSITION_FIXED,
                  "IsReallyFixedPos called on non-'position:fixed' frame");

  nsIAtom *parentType = aFrame->GetParent()->GetType();
  return parentType == nsGkAtoms::viewportFrame ||
         parentType == nsGkAtoms::pageContentFrame;
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(nsIImageLoadingContent* aElement,
                                  uint32_t aSurfaceFlags,
                                  DrawTarget* aTarget)
{
  SurfaceFromElementResult result;
  nsresult rv;

  nsCOMPtr<imgIRequest> imgRequest;
  rv = aElement->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                            getter_AddRefs(imgRequest));
  if (NS_FAILED(rv) || !imgRequest)
    return result;

  uint32_t status;
  imgRequest->GetImageStatus(&status);
  if ((status & imgIRequest::STATUS_LOAD_COMPLETE) == 0) {
    
    
    
    result.mIsStillLoading = (status & imgIRequest::STATUS_ERROR) == 0;
    return result;
  }

  nsCOMPtr<nsIPrincipal> principal;
  rv = imgRequest->GetImagePrincipal(getter_AddRefs(principal));
  if (NS_FAILED(rv))
    return result;

  nsCOMPtr<imgIContainer> imgContainer;
  rv = imgRequest->GetImage(getter_AddRefs(imgContainer));
  if (NS_FAILED(rv))
    return result;

  uint32_t noRasterize = aSurfaceFlags & SFE_NO_RASTERIZING_VECTORS;

  uint32_t whichFrame = (aSurfaceFlags & SFE_WANT_FIRST_FRAME)
                        ? (uint32_t) imgIContainer::FRAME_FIRST
                        : (uint32_t) imgIContainer::FRAME_CURRENT;
  uint32_t frameFlags = imgIContainer::FLAG_SYNC_DECODE;
  if (aSurfaceFlags & SFE_NO_COLORSPACE_CONVERSION)
    frameFlags |= imgIContainer::FLAG_DECODE_NO_COLORSPACE_CONVERSION;
  if (aSurfaceFlags & SFE_PREFER_NO_PREMULTIPLY_ALPHA) {
    frameFlags |= imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA;
    result.mIsPremultiplied = false;
  }

  int32_t imgWidth, imgHeight;
  rv = imgContainer->GetWidth(&imgWidth);
  nsresult rv2 = imgContainer->GetHeight(&imgHeight);
  if (NS_FAILED(rv) || NS_FAILED(rv2))
    return result;

  if (!noRasterize || imgContainer->GetType() == imgIContainer::TYPE_RASTER) {
    if (aSurfaceFlags & SFE_WANT_IMAGE_SURFACE) {
      frameFlags |= imgIContainer::FLAG_WANT_DATA_SURFACE;
    }
    result.mSourceSurface = imgContainer->GetFrame(whichFrame, frameFlags);
    if (!result.mSourceSurface) {
      return result;
    }
    
    
    
    
    if (aTarget) {
      RefPtr<SourceSurface> optSurface =
        aTarget->OptimizeSourceSurface(result.mSourceSurface);
      if (optSurface) {
        result.mSourceSurface = optSurface;
      }
    }
  } else {
    result.mDrawInfo.mImgContainer = imgContainer;
    result.mDrawInfo.mWhichFrame = whichFrame;
    result.mDrawInfo.mDrawingFlags = frameFlags;
  }

  int32_t corsmode;
  if (NS_SUCCEEDED(imgRequest->GetCORSMode(&corsmode))) {
    result.mCORSUsed = (corsmode != imgIRequest::CORS_NONE);
  }

  result.mSize = gfxIntSize(imgWidth, imgHeight);
  result.mPrincipal = principal.forget();
  
  result.mIsWriteOnly = false;
  result.mImageRequest = imgRequest.forget();

  return result;
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(HTMLImageElement *aElement,
                                  uint32_t aSurfaceFlags,
                                  DrawTarget* aTarget)
{
  return SurfaceFromElement(static_cast<nsIImageLoadingContent*>(aElement),
                            aSurfaceFlags, aTarget);
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(HTMLCanvasElement* aElement,
                                  uint32_t aSurfaceFlags,
                                  DrawTarget* aTarget)
{
  SurfaceFromElementResult result;

  bool* isPremultiplied = nullptr;
  if (aSurfaceFlags & SFE_PREFER_NO_PREMULTIPLY_ALPHA) {
    isPremultiplied = &result.mIsPremultiplied;
  }

  gfxIntSize size = aElement->GetSize();

  result.mSourceSurface = aElement->GetSurfaceSnapshot(isPremultiplied);
  if (!result.mSourceSurface) {
     
     
     DrawTarget *ref = aTarget ? aTarget : gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget();
     RefPtr<DrawTarget> dt = ref->CreateSimilarDrawTarget(IntSize(size.width, size.height),
                                                          SurfaceFormat::B8G8R8A8);
     if (dt) {
       result.mSourceSurface = dt->Snapshot();
     }
  } else if (aTarget) {
    RefPtr<SourceSurface> opt = aTarget->OptimizeSourceSurface(result.mSourceSurface);
    if (opt) {
      result.mSourceSurface = opt;
    }
  }

  
  
  aElement->MarkContextClean();

  result.mSize = size;
  result.mPrincipal = aElement->NodePrincipal();
  result.mIsWriteOnly = aElement->IsWriteOnly();

  return result;
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(HTMLVideoElement* aElement,
                                  uint32_t aSurfaceFlags,
                                  DrawTarget* aTarget)
{
  SurfaceFromElementResult result;

  NS_WARN_IF_FALSE((aSurfaceFlags & SFE_PREFER_NO_PREMULTIPLY_ALPHA) == 0, "We can't support non-premultiplied alpha for video!");

#ifdef MOZ_EME
  if (aElement->ContainsRestrictedContent()) {
    return result;
  }
#endif

  uint16_t readyState;
  if (NS_SUCCEEDED(aElement->GetReadyState(&readyState)) &&
      (readyState == nsIDOMHTMLMediaElement::HAVE_NOTHING ||
       readyState == nsIDOMHTMLMediaElement::HAVE_METADATA)) {
    result.mIsStillLoading = true;
    return result;
  }

  
  nsCOMPtr<nsIPrincipal> principal = aElement->GetCurrentPrincipal();
  if (!principal)
    return result;

  ImageContainer *container = aElement->GetImageContainer();
  if (!container)
    return result;

  mozilla::gfx::IntSize size;
  result.mSourceSurface = container->GetCurrentAsSourceSurface(&size);
  if (!result.mSourceSurface)
    return result;

  if (aTarget) {
    RefPtr<SourceSurface> opt = aTarget->OptimizeSourceSurface(result.mSourceSurface);
    if (opt) {
      result.mSourceSurface = opt;
    }
  }

  result.mCORSUsed = aElement->GetCORSMode() != CORS_NONE;
  result.mSize = size;
  result.mPrincipal = principal.forget();
  result.mIsWriteOnly = false;

  return result;
}

nsLayoutUtils::SurfaceFromElementResult
nsLayoutUtils::SurfaceFromElement(dom::Element* aElement,
                                  uint32_t aSurfaceFlags,
                                  DrawTarget* aTarget)
{
  
  if (HTMLCanvasElement* canvas =
        HTMLCanvasElement::FromContentOrNull(aElement)) {
    return SurfaceFromElement(canvas, aSurfaceFlags, aTarget);
  }

  
  if (HTMLVideoElement* video =
        HTMLVideoElement::FromContentOrNull(aElement)) {
    return SurfaceFromElement(video, aSurfaceFlags, aTarget);
  }

  
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(aElement);

  if (!imageLoader) {
    return SurfaceFromElementResult();
  }

  return SurfaceFromElement(imageLoader, aSurfaceFlags, aTarget);
}


nsIContent*
nsLayoutUtils::GetEditableRootContentByContentEditable(nsIDocument* aDocument)
{
  
  if (!aDocument || aDocument->HasFlag(NODE_IS_EDITABLE)) {
    return nullptr;
  }

  
  
  
  
  nsCOMPtr<nsIDOMHTMLDocument> domHTMLDoc = do_QueryInterface(aDocument);
  if (!domHTMLDoc) {
    return nullptr;
  }

  Element* rootElement = aDocument->GetRootElement();
  if (rootElement && rootElement->IsEditable()) {
    return rootElement;
  }

  
  
  nsCOMPtr<nsIDOMHTMLElement> body;
  nsresult rv = domHTMLDoc->GetBody(getter_AddRefs(body));
  nsCOMPtr<nsIContent> content = do_QueryInterface(body);
  if (NS_SUCCEEDED(rv) && content && content->IsEditable()) {
    return content;
  }
  return nullptr;
}

#ifdef DEBUG
 void
nsLayoutUtils::AssertNoDuplicateContinuations(nsIFrame* aContainer,
                                              const nsFrameList& aFrameList)
{
  for (nsIFrame* f = aFrameList.FirstChild(); f ; f = f->GetNextSibling()) {
    
    
    
    for (nsIFrame *c = f; (c = c->GetNextInFlow());) {
      NS_ASSERTION(c->GetParent() != aContainer ||
                   !aFrameList.ContainsFrame(c),
                   "Two continuations of the same frame in the same "
                   "frame list");
    }
  }
}


static bool
IsInLetterFrame(nsIFrame *aFrame)
{
  for (nsIFrame *f = aFrame->GetParent(); f; f = f->GetParent()) {
    if (f->GetType() == nsGkAtoms::letterFrame) {
      return true;
    }
  }
  return false;
}

 void
nsLayoutUtils::AssertTreeOnlyEmptyNextInFlows(nsIFrame *aSubtreeRoot)
{
  NS_ASSERTION(aSubtreeRoot->GetPrevInFlow(),
               "frame tree not empty, but caller reported complete status");

  
  int32_t start, end;
  nsresult rv = aSubtreeRoot->GetOffsets(start, end);
  NS_ASSERTION(NS_SUCCEEDED(rv), "GetOffsets failed");
  
  
  
  
  
  
  
  NS_ASSERTION(start == end || IsInLetterFrame(aSubtreeRoot),
               "frame tree not empty, but caller reported complete status");

  nsIFrame::ChildListIterator lists(aSubtreeRoot);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      nsLayoutUtils::AssertTreeOnlyEmptyNextInFlows(childFrames.get());
    }
  }
}
#endif

static void
GetFontFacesForFramesInner(nsIFrame* aFrame, nsFontFaceList* aFontFaceList)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  if (aFrame->GetType() == nsGkAtoms::textFrame) {
    if (!aFrame->GetPrevContinuation()) {
      nsLayoutUtils::GetFontFacesForText(aFrame, 0, INT32_MAX, true,
                                         aFontFaceList);
    }
    return;
  }

  nsIFrame::ChildListID childLists[] = { nsIFrame::kPrincipalList,
                                         nsIFrame::kPopupList };
  for (size_t i = 0; i < ArrayLength(childLists); ++i) {
    nsFrameList children(aFrame->GetChildList(childLists[i]));
    for (nsFrameList::Enumerator e(children); !e.AtEnd(); e.Next()) {
      nsIFrame* child = e.get();
      child = nsPlaceholderFrame::GetRealFrameFor(child);
      GetFontFacesForFramesInner(child, aFontFaceList);
    }
  }
}


nsresult
nsLayoutUtils::GetFontFacesForFrames(nsIFrame* aFrame,
                                     nsFontFaceList* aFontFaceList)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  while (aFrame) {
    GetFontFacesForFramesInner(aFrame, aFontFaceList);
    aFrame = GetNextContinuationOrIBSplitSibling(aFrame);
  }

  return NS_OK;
}


nsresult
nsLayoutUtils::GetFontFacesForText(nsIFrame* aFrame,
                                   int32_t aStartOffset, int32_t aEndOffset,
                                   bool aFollowContinuations,
                                   nsFontFaceList* aFontFaceList)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  if (aFrame->GetType() != nsGkAtoms::textFrame) {
    return NS_OK;
  }

  nsTextFrame* curr = static_cast<nsTextFrame*>(aFrame);
  do {
    int32_t fstart = std::max(curr->GetContentOffset(), aStartOffset);
    int32_t fend = std::min(curr->GetContentEnd(), aEndOffset);
    if (fstart >= fend) {
      curr = static_cast<nsTextFrame*>(curr->GetNextContinuation());
      continue;
    }

    
    gfxSkipCharsIterator iter = curr->EnsureTextRun(nsTextFrame::eInflated);
    gfxTextRun* textRun = curr->GetTextRun(nsTextFrame::eInflated);
    NS_ENSURE_TRUE(textRun, NS_ERROR_OUT_OF_MEMORY);

    
    nsTextFrame* next = nullptr;
    if (aFollowContinuations && fend < aEndOffset) {
      next = static_cast<nsTextFrame*>(curr->GetNextContinuation());
      while (next && next->GetTextRun(nsTextFrame::eInflated) == textRun) {
        fend = std::min(next->GetContentEnd(), aEndOffset);
        next = fend < aEndOffset ?
          static_cast<nsTextFrame*>(next->GetNextContinuation()) : nullptr;
      }
    }

    uint32_t skipStart = iter.ConvertOriginalToSkipped(fstart);
    uint32_t skipEnd = iter.ConvertOriginalToSkipped(fend);
    aFontFaceList->AddFontsFromTextRun(textRun, skipStart, skipEnd - skipStart);
    curr = next;
  } while (aFollowContinuations && curr);

  return NS_OK;
}


size_t
nsLayoutUtils::SizeOfTextRunsForFrames(nsIFrame* aFrame,
                                       MallocSizeOf aMallocSizeOf,
                                       bool clear)
{
  NS_PRECONDITION(aFrame, "NULL frame pointer");

  size_t total = 0;

  if (aFrame->GetType() == nsGkAtoms::textFrame) {
    nsTextFrame* textFrame = static_cast<nsTextFrame*>(aFrame);
    for (uint32_t i = 0; i < 2; ++i) {
      gfxTextRun *run = textFrame->GetTextRun(
        (i != 0) ? nsTextFrame::eInflated : nsTextFrame::eNotInflated);
      if (run) {
        if (clear) {
          run->ResetSizeOfAccountingFlags();
        } else {
          total += run->MaybeSizeOfIncludingThis(aMallocSizeOf);
        }
      }
    }
    return total;
  }

  nsAutoTArray<nsIFrame::ChildList,4> childListArray;
  aFrame->GetChildLists(&childListArray);

  for (nsIFrame::ChildListArrayIterator childLists(childListArray);
       !childLists.IsDone(); childLists.Next()) {
    for (nsFrameList::Enumerator e(childLists.CurrentList());
         !e.AtEnd(); e.Next()) {
      total += SizeOfTextRunsForFrames(e.get(), aMallocSizeOf, clear);
    }
  }
  return total;
}


void
nsLayoutUtils::Initialize()
{
  Preferences::AddUintVarCache(&sFontSizeInflationMaxRatio,
                               "font.size.inflation.maxRatio");
  Preferences::AddUintVarCache(&sFontSizeInflationEmPerLine,
                               "font.size.inflation.emPerLine");
  Preferences::AddUintVarCache(&sFontSizeInflationMinTwips,
                               "font.size.inflation.minTwips");
  Preferences::AddUintVarCache(&sFontSizeInflationLineThreshold,
                               "font.size.inflation.lineThreshold");
  Preferences::AddIntVarCache(&sFontSizeInflationMappingIntercept,
                              "font.size.inflation.mappingIntercept");
  Preferences::AddBoolVarCache(&sFontSizeInflationForceEnabled,
                               "font.size.inflation.forceEnabled");
  Preferences::AddBoolVarCache(&sFontSizeInflationDisabledInMasterProcess,
                               "font.size.inflation.disabledInMasterProcess");
  Preferences::AddBoolVarCache(&sInvalidationDebuggingIsEnabled,
                               "nglayout.debug.invalidation");
  Preferences::AddBoolVarCache(&sCSSVariablesEnabled,
                               "layout.css.variables.enabled");
  Preferences::AddBoolVarCache(&sInterruptibleReflowEnabled,
                               "layout.interruptible-reflow.enabled");

  Preferences::RegisterCallback(GridEnabledPrefChangeCallback,
                                GRID_ENABLED_PREF_NAME);
  GridEnabledPrefChangeCallback(GRID_ENABLED_PREF_NAME, nullptr);
  Preferences::RegisterCallback(RubyEnabledPrefChangeCallback,
                                RUBY_ENABLED_PREF_NAME);
  RubyEnabledPrefChangeCallback(RUBY_ENABLED_PREF_NAME, nullptr);
  Preferences::RegisterCallback(StickyEnabledPrefChangeCallback,
                                STICKY_ENABLED_PREF_NAME);
  StickyEnabledPrefChangeCallback(STICKY_ENABLED_PREF_NAME, nullptr);
  Preferences::RegisterCallback(TextAlignTrueEnabledPrefChangeCallback,
                                TEXT_ALIGN_TRUE_ENABLED_PREF_NAME);
  Preferences::RegisterCallback(DisplayContentsEnabledPrefChangeCallback,
                                DISPLAY_CONTENTS_ENABLED_PREF_NAME);
  DisplayContentsEnabledPrefChangeCallback(DISPLAY_CONTENTS_ENABLED_PREF_NAME,
                                           nullptr);
  TextAlignTrueEnabledPrefChangeCallback(TEXT_ALIGN_TRUE_ENABLED_PREF_NAME,
                                         nullptr);

  nsComputedDOMStyle::RegisterPrefChangeCallbacks();
}


void
nsLayoutUtils::Shutdown()
{
  if (sContentMap) {
    delete sContentMap;
    sContentMap = nullptr;
  }

  Preferences::UnregisterCallback(GridEnabledPrefChangeCallback,
                                  GRID_ENABLED_PREF_NAME);
  Preferences::UnregisterCallback(RubyEnabledPrefChangeCallback,
                                  RUBY_ENABLED_PREF_NAME);
  Preferences::UnregisterCallback(StickyEnabledPrefChangeCallback,
                                  STICKY_ENABLED_PREF_NAME);

  nsComputedDOMStyle::UnregisterPrefChangeCallbacks();
}


void
nsLayoutUtils::RegisterImageRequest(nsPresContext* aPresContext,
                                    imgIRequest* aRequest,
                                    bool* aRequestRegistered)
{
  if (!aPresContext) {
    return;
  }

  if (aRequestRegistered && *aRequestRegistered) {
    
    
    return;
  }

  if (aRequest) {
    if (!aPresContext->RefreshDriver()->AddImageRequest(aRequest)) {
      NS_WARNING("Unable to add image request");
      return;
    }

    if (aRequestRegistered) {
      *aRequestRegistered = true;
    }
  }
}


void
nsLayoutUtils::RegisterImageRequestIfAnimated(nsPresContext* aPresContext,
                                              imgIRequest* aRequest,
                                              bool* aRequestRegistered)
{
  if (!aPresContext) {
    return;
  }

  if (aRequestRegistered && *aRequestRegistered) {
    
    
    return;
  }

  if (aRequest) {
    nsCOMPtr<imgIContainer> image;
    if (NS_SUCCEEDED(aRequest->GetImage(getter_AddRefs(image)))) {
      
      
      bool isAnimated = false;
      nsresult rv = image->GetAnimated(&isAnimated);
      if (NS_SUCCEEDED(rv) && isAnimated) {
        if (!aPresContext->RefreshDriver()->AddImageRequest(aRequest)) {
          NS_WARNING("Unable to add image request");
          return;
        }

        if (aRequestRegistered) {
          *aRequestRegistered = true;
        }
      }
    }
  }
}


void
nsLayoutUtils::DeregisterImageRequest(nsPresContext* aPresContext,
                                      imgIRequest* aRequest,
                                      bool* aRequestRegistered)
{
  if (!aPresContext) {
    return;
  }

  
  
  if (aRequestRegistered && !*aRequestRegistered) {
    return;
  }

  if (aRequest) {
    nsCOMPtr<imgIContainer> image;
    if (NS_SUCCEEDED(aRequest->GetImage(getter_AddRefs(image)))) {
      aPresContext->RefreshDriver()->RemoveImageRequest(aRequest);

      if (aRequestRegistered) {
        *aRequestRegistered = false;
      }
    }
  }
}


void
nsLayoutUtils::PostRestyleEvent(Element* aElement,
                                nsRestyleHint aRestyleHint,
                                nsChangeHint aMinChangeHint)
{
  nsIDocument* doc = aElement->GetComposedDoc();
  if (doc) {
    nsCOMPtr<nsIPresShell> presShell = doc->GetShell();
    if (presShell) {
      presShell->GetPresContext()->RestyleManager()->PostRestyleEvent(
        aElement, aRestyleHint, aMinChangeHint);
    }
  }
}

nsSetAttrRunnable::nsSetAttrRunnable(nsIContent* aContent, nsIAtom* aAttrName,
                                     const nsAString& aValue)
  : mContent(aContent),
    mAttrName(aAttrName),
    mValue(aValue)
{
  NS_ASSERTION(aContent && aAttrName, "Missing stuff, prepare to crash");
}

nsSetAttrRunnable::nsSetAttrRunnable(nsIContent* aContent, nsIAtom* aAttrName,
                                     int32_t aValue)
  : mContent(aContent),
    mAttrName(aAttrName)
{
  NS_ASSERTION(aContent && aAttrName, "Missing stuff, prepare to crash");
  mValue.AppendInt(aValue);
}

NS_IMETHODIMP
nsSetAttrRunnable::Run()
{
  return mContent->SetAttr(kNameSpaceID_None, mAttrName, mValue, true);
}

nsUnsetAttrRunnable::nsUnsetAttrRunnable(nsIContent* aContent,
                                         nsIAtom* aAttrName)
  : mContent(aContent),
    mAttrName(aAttrName)
{
  NS_ASSERTION(aContent && aAttrName, "Missing stuff, prepare to crash");
}

NS_IMETHODIMP
nsUnsetAttrRunnable::Run()
{
  return mContent->UnsetAttr(kNameSpaceID_None, mAttrName, true);
}






static nscoord
MinimumFontSizeFor(nsPresContext* aPresContext, WritingMode aWritingMode,
                   nscoord aContainerISize)
{
  nsIPresShell* presShell = aPresContext->PresShell();

  uint32_t emPerLine = presShell->FontSizeInflationEmPerLine();
  uint32_t minTwips = presShell->FontSizeInflationMinTwips();
  if (emPerLine == 0 && minTwips == 0) {
    return 0;
  }

  
  nscoord iFrameISize = aWritingMode.IsVertical()
    ? aPresContext->GetVisibleArea().height
    : aPresContext->GetVisibleArea().width;
  nscoord effectiveContainerISize = std::min(iFrameISize, aContainerISize);

  nscoord byLine = 0, byInch = 0;
  if (emPerLine != 0) {
    byLine = effectiveContainerISize / emPerLine;
  }
  if (minTwips != 0) {
    
    
    gfxSize screenSize = aPresContext->ScreenSizeInchesForFontInflation();
    float deviceISizeInches = aWritingMode.IsVertical()
      ? screenSize.height : screenSize.width;
    byInch = NSToCoordRound(effectiveContainerISize /
                            (deviceISizeInches * 1440 /
                             minTwips ));
  }
  return std::max(byLine, byInch);
}

 float
nsLayoutUtils::FontSizeInflationInner(const nsIFrame *aFrame,
                                      nscoord aMinFontSize)
{
  
  
  
  nscoord styleFontSize = aFrame->StyleFont()->mFont.size;
  if (styleFontSize <= 0) {
    
    return 1.0;
  }

  if (aMinFontSize <= 0) {
    
    return 1.0;
  }

  
  
  
  for (const nsIFrame* f = aFrame;
       f && !f->IsContainerForFontSizeInflation();
       f = f->GetParent()) {
    nsIContent* content = f->GetContent();
    nsIAtom* fType = f->GetType();
    nsIFrame* parent = f->GetParent();
    
    
    if (!(parent && parent->GetContent() == content) &&
        
        fType != nsGkAtoms::inlineFrame &&
        
        
        fType != nsGkAtoms::formControlFrame) {
      
      
      if (fType == nsGkAtoms::rubyTextFrame) {
        MOZ_ASSERT(parent &&
                   parent->GetType() == nsGkAtoms::rubyTextContainerFrame);
        nsIFrame* grandparent = parent->GetParent();
        MOZ_ASSERT(grandparent &&
                   grandparent->GetType() == nsGkAtoms::rubyFrame);
        return FontSizeInflationFor(grandparent);
      }
      nsStyleCoord stylePosWidth = f->StylePosition()->mWidth;
      nsStyleCoord stylePosHeight = f->StylePosition()->mHeight;
      if (stylePosWidth.GetUnit() != eStyleUnit_Auto ||
          stylePosHeight.GetUnit() != eStyleUnit_Auto) {

        return 1.0;
      }
    }
  }

  int32_t interceptParam = nsLayoutUtils::FontSizeInflationMappingIntercept();
  float maxRatio = (float)nsLayoutUtils::FontSizeInflationMaxRatio() / 100.0f;

  float ratio = float(styleFontSize) / float(aMinFontSize);
  float inflationRatio;

  
  
  if (interceptParam >= 0) {
    
    
    
    
    
    

    float intercept = 1 + float(interceptParam)/2.0f;
    if (ratio >= intercept) {
      
      return 1.0;
    }

    
    
    
    
    
    inflationRatio = (1.0f + (ratio * (intercept - 1) / intercept)) / ratio;
  } else {
    
    
    
    inflationRatio = 1 + 1.0f / ratio;
  }

  if (maxRatio > 1.0 && inflationRatio > maxRatio) {
    return maxRatio;
  } else {
    return inflationRatio;
  }
}

static bool
ShouldInflateFontsForContainer(const nsIFrame *aFrame)
{
  
  
  
  
  
  
  
  const nsStyleText* styleText = aFrame->StyleText();

  return styleText->mTextSizeAdjust != NS_STYLE_TEXT_SIZE_ADJUST_NONE &&
         !(aFrame->GetStateBits() & NS_FRAME_IN_CONSTRAINED_BSIZE) &&
         
         
         
         (styleText->WhiteSpaceCanWrap(aFrame) ||
          aFrame->IsFrameOfType(nsIFrame::eMathML));
}

nscoord
nsLayoutUtils::InflationMinFontSizeFor(const nsIFrame *aFrame)
{
  nsPresContext *presContext = aFrame->PresContext();
  if (!FontSizeInflationEnabled(presContext) ||
      presContext->mInflationDisabledForShrinkWrap) {
    return 0;
  }

  for (const nsIFrame *f = aFrame; f; f = f->GetParent()) {
    if (f->IsContainerForFontSizeInflation()) {
      if (!ShouldInflateFontsForContainer(f)) {
        return 0;
      }

      nsFontInflationData *data =
        nsFontInflationData::FindFontInflationDataFor(aFrame);
      
      
      if (!data || !data->InflationEnabled()) {
        return 0;
      }

      return MinimumFontSizeFor(aFrame->PresContext(),
                                aFrame->GetWritingMode(),
                                data->EffectiveISize());
    }
  }

  MOZ_ASSERT(false, "root should always be container");

  return 0;
}

float
nsLayoutUtils::FontSizeInflationFor(const nsIFrame *aFrame)
{
  if (aFrame->IsSVGText()) {
    const nsIFrame* container = aFrame;
    while (container->GetType() != nsGkAtoms::svgTextFrame) {
      container = container->GetParent();
    }
    NS_ASSERTION(container, "expected to find an ancestor SVGTextFrame");
    return
      static_cast<const SVGTextFrame*>(container)->GetFontSizeScaleFactor();
  }

  if (!FontSizeInflationEnabled(aFrame->PresContext())) {
    return 1.0f;
  }

  return FontSizeInflationInner(aFrame, InflationMinFontSizeFor(aFrame));
}

 bool
nsLayoutUtils::FontSizeInflationEnabled(nsPresContext *aPresContext)
{
  nsIPresShell* presShell = aPresContext->GetPresShell();

  if (!presShell) {
    return false;
  }

  return presShell->FontSizeInflationEnabled();
}

 nsRect
nsLayoutUtils::GetBoxShadowRectForFrame(nsIFrame* aFrame, 
                                        const nsSize& aFrameSize)
{
  nsCSSShadowArray* boxShadows = aFrame->StyleBorder()->mBoxShadow;
  if (!boxShadows) {
    return nsRect();
  }
  
  nsRect shadows;
  int32_t A2D = aFrame->PresContext()->AppUnitsPerDevPixel();
  for (uint32_t i = 0; i < boxShadows->Length(); ++i) {
    nsRect tmpRect(nsPoint(0, 0), aFrameSize);
    nsCSSShadowItem* shadow = boxShadows->ShadowAt(i);

    
    if (shadow->mInset)
      continue;

    tmpRect.MoveBy(nsPoint(shadow->mXOffset, shadow->mYOffset));
    tmpRect.Inflate(shadow->mSpread);
    tmpRect.Inflate(
      nsContextBoxBlur::GetBlurRadiusMargin(shadow->mRadius, A2D));
    shadows.UnionRect(shadows, tmpRect);
  }
  return shadows;
}

 void
nsLayoutUtils::UpdateImageVisibilityForFrame(nsIFrame* aImageFrame)
{
#ifdef DEBUG
  nsIAtom* type = aImageFrame->GetType();
  MOZ_ASSERT(type == nsGkAtoms::imageFrame ||
             type == nsGkAtoms::imageControlFrame ||
             type == nsGkAtoms::svgImageFrame, "wrong type of frame");
#endif

  nsCOMPtr<nsIImageLoadingContent> content = do_QueryInterface(aImageFrame->GetContent());
  if (!content) {
    return;
  }

  nsIPresShell* presShell = aImageFrame->PresContext()->PresShell();
  if (presShell->AssumeAllImagesVisible()) {
    presShell->EnsureImageInVisibleList(content);
    return;
  }

  bool visible = true;
  nsIFrame* f = aImageFrame->GetParent();
  nsRect rect = aImageFrame->GetContentRectRelativeToSelf();
  nsIFrame* rectFrame = aImageFrame;
  while (f) {
    nsIScrollableFrame* sf = do_QueryFrame(f);
    if (sf) {
      nsRect transformedRect =
        nsLayoutUtils::TransformFrameRectToAncestor(rectFrame, rect, f);
      if (!sf->IsRectNearlyVisible(transformedRect)) {
        visible = false;
        break;
      }
      
      
      nsRect scrollPort = sf->GetScrollPortRect();
      if (transformedRect.XMost() > scrollPort.XMost()) {
        transformedRect.x -= transformedRect.XMost() - scrollPort.XMost();
      }
      if (transformedRect.x < scrollPort.x) {
        transformedRect.x = scrollPort.x;
      }
      if (transformedRect.YMost() > scrollPort.YMost()) {
        transformedRect.y -= transformedRect.YMost() - scrollPort.YMost();
      }
      if (transformedRect.y < scrollPort.y) {
        transformedRect.y = scrollPort.y;
      }
      transformedRect.width = std::min(transformedRect.width, scrollPort.width);
      transformedRect.height = std::min(transformedRect.height, scrollPort.height);
      rect = transformedRect;
      rectFrame = f;
    }
    nsIFrame* parent = f->GetParent();
    if (!parent) {
      parent = nsLayoutUtils::GetCrossDocParentFrame(f);
      if (parent && parent->PresContext()->IsChrome()) {
        break;
      }
    }
    f = parent;
  }

  if (visible) {
    presShell->EnsureImageInVisibleList(content);
  } else {
    presShell->RemoveImageFromVisibleList(content);
  }
}

 bool
nsLayoutUtils::GetContentViewerSize(nsPresContext* aPresContext,
                                    LayoutDeviceIntSize& aOutSize)
{
  nsCOMPtr<nsIDocShell> docShell = aPresContext->GetDocShell();
  if (!docShell) {
    return false;
  }

  nsCOMPtr<nsIContentViewer> cv;
  docShell->GetContentViewer(getter_AddRefs(cv));
  if (!cv) {
    return false;
  }

  nsIntRect bounds;
  cv->GetBounds(bounds);
  aOutSize = LayoutDeviceIntRect::FromUntyped(bounds).Size();
  return true;
}

 nsSize
nsLayoutUtils::CalculateCompositionSizeForFrame(nsIFrame* aFrame)
{
  
  
  
  nsIScrollableFrame* scrollableFrame = aFrame->GetScrollTargetFrame();
  nsSize size = scrollableFrame ? scrollableFrame->GetScrollPortRect().Size() : aFrame->GetSize();

  nsPresContext* presContext = aFrame->PresContext();
  nsIPresShell* presShell = presContext->PresShell();

  
  
  
  bool isRootContentDocRootScrollFrame = presContext->IsRootContentDocument()
                                      && aFrame == presShell->GetRootScrollFrame();
  if (isRootContentDocRootScrollFrame) {
    if (nsIFrame* rootFrame = presShell->GetRootFrame()) {
#ifdef MOZ_WIDGET_ANDROID
      nsIWidget* widget = rootFrame->GetNearestWidget();
#else
      nsView* view = rootFrame->GetView();
      nsIWidget* widget = view ? view->GetWidget() : nullptr;
#endif
      int32_t auPerDevPixel = presContext->AppUnitsPerDevPixel();
      if (widget) {
        nsIntRect widgetBounds;
        widget->GetBounds(widgetBounds);
        size = nsSize(widgetBounds.width * auPerDevPixel,
                      widgetBounds.height * auPerDevPixel);
#ifdef MOZ_WIDGET_ANDROID
        nsRect frameRect = aFrame->GetRect();
        float cumulativeResolution = presShell->GetCumulativeResolution();
        LayoutDeviceToParentLayerScale layoutToParentLayerScale =
          
          
          
          LayoutDeviceToLayerScale(cumulativeResolution) *
          LayerToScreenScale(1.0) * ScreenToParentLayerScale(1.0);
        ParentLayerRect frameRectPixels =
          LayoutDeviceRect::FromAppUnits(frameRect, auPerDevPixel)
          * layoutToParentLayerScale;
        if (frameRectPixels.height < ParentLayerRect(ViewAs<ParentLayerPixel>(widgetBounds)).height) {
          
          
          size.height =
            NSToCoordRound(frameRect.height * cumulativeResolution);
        }
#endif
      } else {
        LayoutDeviceIntSize contentSize;
        if (nsLayoutUtils::GetContentViewerSize(presContext, contentSize)) {
          size = LayoutDevicePixel::ToAppUnits(contentSize, auPerDevPixel);
        }
      }

      if (scrollableFrame && !LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars)) {
        nsMargin margins = scrollableFrame->GetActualScrollbarSizes();
        size.width -= margins.LeftRight();
        size.height -= margins.TopBottom();
      }
    }
  }

  return size;
}

 CSSSize
nsLayoutUtils::CalculateRootCompositionSize(nsIFrame* aFrame,
                                            bool aIsRootContentDocRootScrollFrame,
                                            const FrameMetrics& aMetrics)
{

  if (aIsRootContentDocRootScrollFrame) {
    return ViewAs<LayerPixel>(aMetrics.mCompositionBounds.Size(),
                              PixelCastJustification::ParentLayerToLayerForRootComposition)
           * LayerToScreenScale(1.0f)
           / aMetrics.DisplayportPixelsPerCSSPixel();
  }
  nsPresContext* presContext = aFrame->PresContext();
  ScreenSize rootCompositionSize;
  nsPresContext* rootPresContext =
    presContext->GetToplevelContentDocumentPresContext();
  if (!rootPresContext) {
    rootPresContext = presContext->GetRootPresContext();
  }
  nsIPresShell* rootPresShell = nullptr;
  if (rootPresContext) {
    
    
    
    nsIPresShell* rootPresShell = rootPresContext->PresShell();
    if (nsIFrame* rootFrame = rootPresShell->GetRootFrame()) {
      LayoutDeviceToLayerScale2D cumulativeResolution(
        rootPresShell->GetCumulativeResolution()
      * nsLayoutUtils::GetTransformToAncestorScale(rootFrame));
      int32_t rootAUPerDevPixel = rootPresContext->AppUnitsPerDevPixel();
      LayerSize frameSize =
        (LayoutDeviceRect::FromAppUnits(rootFrame->GetRect(), rootAUPerDevPixel)
         * cumulativeResolution).Size();
      rootCompositionSize = frameSize * LayerToScreenScale(1.0f);
#ifdef MOZ_WIDGET_ANDROID
      nsIWidget* widget = rootFrame->GetNearestWidget();
#else
      nsView* view = rootFrame->GetView();
      nsIWidget* widget = view ? view->GetWidget() : nullptr;
#endif
      if (widget) {
        nsIntRect widgetBounds;
        widget->GetBounds(widgetBounds);
        rootCompositionSize = ScreenSize(ViewAs<ScreenPixel>(widgetBounds.Size()));
#ifdef MOZ_WIDGET_ANDROID
        if (frameSize.height < rootCompositionSize.height) {
          rootCompositionSize.height = frameSize.height;
        }
#endif
      } else {
        LayoutDeviceIntSize contentSize;
        if (nsLayoutUtils::GetContentViewerSize(rootPresContext, contentSize)) {
          LayoutDeviceToLayerScale scale;
          if (rootPresContext->GetParentPresContext()) {
            float res = rootPresContext->GetParentPresContext()->PresShell()->GetCumulativeResolution();
            scale = LayoutDeviceToLayerScale(res);
          }
          rootCompositionSize = contentSize * scale * LayerToScreenScale(1.0f);
        }
      }
    }
  } else {
    nsIWidget* widget = aFrame->GetNearestWidget();
    nsIntRect widgetBounds;
    widget->GetBounds(widgetBounds);
    rootCompositionSize = ScreenSize(ViewAs<ScreenPixel>(widgetBounds.Size()));
  }

  
  nsIFrame* rootRootScrollFrame = rootPresShell ? rootPresShell->GetRootScrollFrame() : nullptr;
  nsIScrollableFrame* rootScrollableFrame = nullptr;
  if (rootRootScrollFrame) {
    rootScrollableFrame = rootRootScrollFrame->GetScrollTargetFrame();
  }
  if (rootScrollableFrame && !LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars)) {
    CSSMargin margins = CSSMargin::FromAppUnits(rootScrollableFrame->GetActualScrollbarSizes());
    
    rootCompositionSize.width -= margins.LeftRight();
    rootCompositionSize.height -= margins.TopBottom();
  }

  return rootCompositionSize / aMetrics.DisplayportPixelsPerCSSPixel();
}

 nsRect
nsLayoutUtils::CalculateScrollableRectForFrame(nsIScrollableFrame* aScrollableFrame, nsIFrame* aRootFrame)
{
  nsRect contentBounds;
  if (aScrollableFrame) {
    contentBounds = aScrollableFrame->GetScrollRange();

    
    
    
    
    
#if !defined(MOZ_WIDGET_ANDROID) || defined(MOZ_ANDROID_APZ)
    nsPoint scrollPosition = aScrollableFrame->GetScrollPosition();
    if (aScrollableFrame->GetScrollbarStyles().mVertical == NS_STYLE_OVERFLOW_HIDDEN) {
      contentBounds.y = scrollPosition.y;
      contentBounds.height = 0;
    }
    if (aScrollableFrame->GetScrollbarStyles().mHorizontal == NS_STYLE_OVERFLOW_HIDDEN) {
      contentBounds.x = scrollPosition.x;
      contentBounds.width = 0;
    }
#endif

    contentBounds.width += aScrollableFrame->GetScrollPortRect().width;
    contentBounds.height += aScrollableFrame->GetScrollPortRect().height;
  } else {
    contentBounds = aRootFrame->GetRect();
  }
  return contentBounds;
}

 nsRect
nsLayoutUtils::CalculateExpandedScrollableRect(nsIFrame* aFrame)
{
  nsRect scrollableRect =
    CalculateScrollableRectForFrame(aFrame->GetScrollTargetFrame(),
                                    aFrame->PresContext()->PresShell()->GetRootFrame());
  nsSize compSize = CalculateCompositionSizeForFrame(aFrame);

  if (aFrame == aFrame->PresContext()->PresShell()->GetRootScrollFrame()) {
    
    
    float res = aFrame->PresContext()->PresShell()->GetResolution();
    compSize.width = NSToCoordRound(compSize.width / res);
    compSize.height = NSToCoordRound(compSize.height / res);
  }

  if (scrollableRect.width < compSize.width) {
    scrollableRect.x = std::max(0,
                                scrollableRect.x - (compSize.width - scrollableRect.width));
    scrollableRect.width = compSize.width;
  }

  if (scrollableRect.height < compSize.height) {
    scrollableRect.y = std::max(0,
                                scrollableRect.y - (compSize.height - scrollableRect.height));
    scrollableRect.height = compSize.height;
  }
  return scrollableRect;
}

 bool
nsLayoutUtils::UsesAsyncScrolling()
{
#ifdef MOZ_WIDGET_ANDROID
  
  return true;
#endif

  return gfxPrefs::AsyncPanZoomEnabled();
}

 void
nsLayoutUtils::DoLogTestDataForPaint(LayerManager* aManager,
                                     ViewID aScrollId,
                                     const std::string& aKey,
                                     const std::string& aValue)
{
  if (aManager->GetBackendType() == LayersBackend::LAYERS_CLIENT) {
    static_cast<ClientLayerManager*>(aManager)->LogTestDataForCurrentPaint(aScrollId, aKey, aValue);
  }
}

 bool
nsLayoutUtils::IsAPZTestLoggingEnabled()
{
  return gfxPrefs::APZTestLoggingEnabled();
}

nsLayoutUtils::SurfaceFromElementResult::SurfaceFromElementResult()
  
  : mIsWriteOnly(true)
  , mIsStillLoading(false)
  , mCORSUsed(false)
  , mIsPremultiplied(true)
{
}

bool
nsLayoutUtils::IsNonWrapperBlock(nsIFrame* aFrame)
{
  return GetAsBlock(aFrame) && !aFrame->IsBlockWrapper();
}

bool
nsLayoutUtils::NeedsPrintPreviewBackground(nsPresContext* aPresContext)
{
  return aPresContext->IsRootPaginatedDocument() &&
    (aPresContext->Type() == nsPresContext::eContext_PrintPreview ||
     aPresContext->Type() == nsPresContext::eContext_PageLayout);
}

AutoMaybeDisableFontInflation::AutoMaybeDisableFontInflation(nsIFrame *aFrame)
{
  
  
  
  
  
  if (aFrame->IsContainerForFontSizeInflation() &&
      !aFrame->IsFrameOfType(nsIFrame::eMathML)) {
    mPresContext = aFrame->PresContext();
    mOldValue = mPresContext->mInflationDisabledForShrinkWrap;
    mPresContext->mInflationDisabledForShrinkWrap = true;
  } else {
    
    mPresContext = nullptr;
  }
}

AutoMaybeDisableFontInflation::~AutoMaybeDisableFontInflation()
{
  if (mPresContext) {
    mPresContext->mInflationDisabledForShrinkWrap = mOldValue;
  }
}

namespace mozilla {

Rect NSRectToRect(const nsRect& aRect, double aAppUnitsPerPixel)
{
  
  
  return Rect(Float(aRect.x / aAppUnitsPerPixel),
              Float(aRect.y / aAppUnitsPerPixel),
              Float(aRect.width / aAppUnitsPerPixel),
              Float(aRect.height / aAppUnitsPerPixel));
}

Rect NSRectToSnappedRect(const nsRect& aRect, double aAppUnitsPerPixel,
                         const gfx::DrawTarget& aSnapDT)
{
  
  
  Rect rect(Float(aRect.x / aAppUnitsPerPixel),
            Float(aRect.y / aAppUnitsPerPixel),
            Float(aRect.width / aAppUnitsPerPixel),
            Float(aRect.height / aAppUnitsPerPixel));
  MaybeSnapToDevicePixels(rect, aSnapDT, true);
  return rect;
}

void StrokeLineWithSnapping(const nsPoint& aP1, const nsPoint& aP2,
                            int32_t aAppUnitsPerDevPixel,
                            DrawTarget& aDrawTarget,
                            const Pattern& aPattern,
                            const StrokeOptions& aStrokeOptions,
                            const DrawOptions& aDrawOptions)
{
  Point p1 = NSPointToPoint(aP1, aAppUnitsPerDevPixel);
  Point p2 = NSPointToPoint(aP2, aAppUnitsPerDevPixel);
  SnapLineToDevicePixelsForStroking(p1, p2, aDrawTarget);
  aDrawTarget.StrokeLine(p1, p2, aPattern, aStrokeOptions, aDrawOptions);
}

namespace layout {

  
void
MaybeSetupTransactionIdAllocator(layers::LayerManager* aManager, nsView* aView)
{
  if (aManager->GetBackendType() == layers::LayersBackend::LAYERS_CLIENT) {
    layers::ClientLayerManager *manager = static_cast<layers::ClientLayerManager*>(aManager);
    nsRefreshDriver *refresh = aView->GetViewManager()->GetPresShell()->GetPresContext()->RefreshDriver();
    manager->SetTransactionIdAllocator(refresh);
  }
}

}
}

 bool
nsLayoutUtils::IsOutlineStyleAutoEnabled()
{
  static bool sOutlineStyleAutoEnabled;
  static bool sOutlineStyleAutoPrefCached = false;

  if (!sOutlineStyleAutoPrefCached) {
    sOutlineStyleAutoPrefCached = true;
    Preferences::AddBoolVarCache(&sOutlineStyleAutoEnabled,
                                 "layout.css.outline-style-auto.enabled",
                                 false);
  }
  return sOutlineStyleAutoEnabled;
}

 void
nsLayoutUtils::SetBSizeFromFontMetrics(const nsIFrame* aFrame,
                                       nsHTMLReflowMetrics& aMetrics,
                                       const LogicalMargin& aFramePadding,
                                       WritingMode aLineWM,
                                       WritingMode aFrameWM)
{
  nsRefPtr<nsFontMetrics> fm;
  float inflation = nsLayoutUtils::FontSizeInflationFor(aFrame);
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(fm), inflation);

  if (fm) {
    
    
    
    
    
    
    
    
    
    
    aMetrics.SetBlockStartAscent(aLineWM.IsLineInverted() ? fm->MaxDescent()
                                                          : fm->MaxAscent());
    aMetrics.BSize(aLineWM) = fm->MaxHeight();
  } else {
    NS_WARNING("Cannot get font metrics - defaulting sizes to 0");
    aMetrics.SetBlockStartAscent(aMetrics.BSize(aLineWM) = 0);
  }
  aMetrics.SetBlockStartAscent(aMetrics.BlockStartAscent() +
                               aFramePadding.BStart(aFrameWM));
  aMetrics.BSize(aLineWM) += aFramePadding.BStartEnd(aFrameWM);
}

 bool
nsLayoutUtils::HasApzAwareListeners(EventListenerManager* aElm)
{
  if (!aElm) {
    return false;
  }
  return aElm->HasListenersFor(nsGkAtoms::ontouchstart) ||
         aElm->HasListenersFor(nsGkAtoms::ontouchmove) ||
         aElm->HasListenersFor(nsGkAtoms::onwheel) ||
         aElm->HasListenersFor(nsGkAtoms::onDOMMouseScroll) ||
         aElm->HasListenersFor(nsHtml5Atoms::onmousewheel);
}

 bool
nsLayoutUtils::HasDocumentLevelListenersForApzAwareEvents(nsIPresShell* aShell)
{
  if (nsIDocument* doc = aShell->GetDocument()) {
    WidgetEvent event(true, NS_EVENT_NULL);
    nsTArray<EventTarget*> targets;
    nsresult rv = EventDispatcher::Dispatch(doc, nullptr, &event, nullptr,
        nullptr, nullptr, &targets);
    NS_ENSURE_SUCCESS(rv, false);
    for (size_t i = 0; i < targets.Length(); i++) {
      if (HasApzAwareListeners(targets[i]->GetExistingListenerManager())) {
        return true;
      }
    }
  }
  return false;
}

 float
nsLayoutUtils::GetResolution(nsIPresShell* aPresShell)
{
  nsIScrollableFrame* sf = aPresShell->GetRootScrollFrameAsScrollable();
  if (sf) {
    return sf->GetResolution();
  }
  return aPresShell->GetResolution();
}

 void
nsLayoutUtils::SetResolutionAndScaleTo(nsIPresShell* aPresShell, float aResolution)
{
  nsIScrollableFrame* sf = aPresShell->GetRootScrollFrameAsScrollable();
  if (sf) {
    sf->SetResolutionAndScaleTo(aResolution);
    aPresShell->SetResolutionAndScaleTo(aResolution);
  }
}

static void
MaybeReflowForInflationScreenSizeChange(nsPresContext *aPresContext)
{
  if (aPresContext) {
    nsIPresShell* presShell = aPresContext->GetPresShell();
    bool fontInflationWasEnabled = presShell->FontSizeInflationEnabled();
    presShell->NotifyFontSizeInflationEnabledIsDirty();
    bool changed = false;
    if (presShell && presShell->FontSizeInflationEnabled() &&
        presShell->FontSizeInflationMinTwips() != 0) {
      aPresContext->ScreenSizeInchesForFontInflation(&changed);
    }

    changed = changed ||
      (fontInflationWasEnabled != presShell->FontSizeInflationEnabled());
    if (changed) {
      nsCOMPtr<nsIDocShell> docShell = aPresContext->GetDocShell();
      if (docShell) {
        nsCOMPtr<nsIContentViewer> cv;
        docShell->GetContentViewer(getter_AddRefs(cv));
        if (cv) {
          nsTArray<nsCOMPtr<nsIContentViewer> > array;
          cv->AppendSubtree(array);
          for (uint32_t i = 0, iEnd = array.Length(); i < iEnd; ++i) {
            nsCOMPtr<nsIPresShell> shell;
            nsCOMPtr<nsIContentViewer> cv = array[i];
            cv->GetPresShell(getter_AddRefs(shell));
            if (shell) {
              nsIFrame *rootFrame = shell->GetRootFrame();
              if (rootFrame) {
                shell->FrameNeedsReflow(rootFrame,
                                        nsIPresShell::eStyleChange,
                                        NS_FRAME_IS_DIRTY);
              }
            }
          }
        }
      }
    }
  }
}

 void
nsLayoutUtils::SetScrollPositionClampingScrollPortSize(nsIPresShell* aPresShell, CSSSize aSize)
{
  MOZ_ASSERT(aSize.width >= 0.0 && aSize.height >= 0.0);

  aPresShell->SetScrollPositionClampingScrollPortSize(
    nsPresContext::CSSPixelsToAppUnits(aSize.width),
    nsPresContext::CSSPixelsToAppUnits(aSize.height));

  
  
  
  
  
  nsPresContext* presContext = aPresShell->GetPresContext();
  MaybeReflowForInflationScreenSizeChange(presContext);
}

 void
nsLayoutUtils::SetCSSViewport(nsIPresShell* aPresShell, CSSSize aSize)
{
  MOZ_ASSERT(aSize.width >= 0.0 && aSize.height >= 0.0);

  nscoord width = nsPresContext::CSSPixelsToAppUnits(aSize.width);
  nscoord height = nsPresContext::CSSPixelsToAppUnits(aSize.height);

  aPresShell->ResizeReflowOverride(width, height);
}

 uint32_t
nsLayoutUtils::GetTouchActionFromFrame(nsIFrame* aFrame)
{
  
  if (!aFrame) {
    return NS_STYLE_TOUCH_ACTION_AUTO;
  }

  
  
  bool isNonReplacedInlineElement = aFrame->IsFrameOfType(nsIFrame::eLineParticipant);
  if (isNonReplacedInlineElement) {
    return NS_STYLE_TOUCH_ACTION_AUTO;
  }

  const nsStyleDisplay* disp = aFrame->StyleDisplay();
  bool isTableElement = disp->IsInnerTableStyle() &&
    disp->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL &&
    disp->mDisplay != NS_STYLE_DISPLAY_TABLE_CAPTION;
  if (isTableElement) {
    return NS_STYLE_TOUCH_ACTION_AUTO;
  }

  return disp->mTouchAction;
}

  void
nsLayoutUtils::TransformToAncestorAndCombineRegions(
  const nsRect& aBounds,
  nsIFrame* aFrame,
  const nsIFrame* aAncestorFrame,
  nsRegion* aPreciseTargetDest,
  nsRegion* aImpreciseTargetDest)
{
  if (aBounds.IsEmpty()) {
    return;
  }
  Matrix4x4 matrix = GetTransformToAncestor(aFrame, aAncestorFrame);
  Matrix matrix2D;
  bool isPrecise = (matrix.Is2D(&matrix2D)
    && !matrix2D.HasNonAxisAlignedTransform());
  nsRect transformed = TransformFrameRectToAncestor(
    aFrame, aBounds, aAncestorFrame);
  nsRegion* dest = isPrecise ? aPreciseTargetDest : aImpreciseTargetDest;
  dest->OrWith(transformed);
}
