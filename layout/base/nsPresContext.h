






#ifndef nsPresContext_h___
#define nsPresContext_h___

#include "mozilla/Attributes.h"
#include "mozilla/WeakPtr.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsRect.h"
#include "nsFont.h"
#include "gfxFontConstants.h"
#include "nsIAtom.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsCRT.h"
#include "FramePropertyTable.h"
#include "nsGkAtoms.h"
#include "nsCycleCollectionParticipant.h"
#include "nsChangeHint.h"
#include <algorithm>

#include "gfxRect.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/AppUnits.h"
#include "prclist.h"
#include "nsThreadUtils.h"
#include "ScrollbarStyles.h"
#include "nsIMessageManager.h"
#include "mozilla/RestyleLogging.h"

class nsAString;
class nsIPrintSettings;
class nsDocShell;
class nsIDocShell;
class nsIDocument;
class nsILanguageAtomService;
class nsITheme;
class nsIContent;
class nsIFrame;
class nsFrameManager;
class nsILinkHandler;
class nsIAtom;
class nsIRunnable;
class gfxUserFontEntry;
class gfxUserFontSet;
class gfxTextPerfMetrics;
class nsPluginFrame;
class nsTransitionManager;
class nsAnimationManager;
class nsRefreshDriver;
class nsIWidget;
class nsDeviceContext;
class gfxMissingFontRecorder;

namespace mozilla {
class EventStateManager;
class RestyleManager;
class CounterStyleManager;
namespace dom {
class FontFaceSet;
}
namespace layers {
class ContainerLayer;
class LayerManager;
}
}


enum nsPresContext_CachedBoolPrefType {
  kPresContext_UseDocumentFonts = 1,
  kPresContext_UnderlineLinks
};


enum nsPresContext_CachedIntPrefType {
  kPresContext_ScrollbarSide = 1,
  kPresContext_BidiDirection
};



const uint8_t kPresContext_DefaultVariableFont_ID = 0x00; 
const uint8_t kPresContext_DefaultFixedFont_ID    = 0x01; 

#ifdef DEBUG
struct nsAutoLayoutPhase;

enum nsLayoutPhase {
  eLayoutPhase_Paint,
  eLayoutPhase_Reflow,
  eLayoutPhase_FrameC,
  eLayoutPhase_COUNT
};
#endif

class nsInvalidateRequestList {
public:
  struct Request {
    nsRect   mRect;
    uint32_t mFlags;
  };

  void TakeFrom(nsInvalidateRequestList* aList)
  {
    mRequests.MoveElementsFrom(aList->mRequests);
  }
  bool IsEmpty() { return mRequests.IsEmpty(); }

  nsTArray<Request> mRequests;
};


#define NS_AUTHOR_SPECIFIED_BACKGROUND      (1 << 0)
#define NS_AUTHOR_SPECIFIED_BORDER          (1 << 1)
#define NS_AUTHOR_SPECIFIED_PADDING         (1 << 2)
#define NS_AUTHOR_SPECIFIED_TEXT_SHADOW     (1 << 3)

class nsRootPresContext;




class nsPresContext : public nsIObserver {
public:
  typedef mozilla::FramePropertyTable FramePropertyTable;
  typedef mozilla::ScrollbarStyles ScrollbarStyles;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW
  NS_DECL_CYCLE_COLLECTION_CLASS(nsPresContext)

  enum nsPresContextType {
    eContext_Galley,       
    eContext_PrintPreview, 
    eContext_Print,        
    eContext_PageLayout    
  };

  nsPresContext(nsIDocument* aDocument, nsPresContextType aType);

  


  nsresult Init(nsDeviceContext* aDeviceContext);

  



  void SetShell(nsIPresShell* aShell);


  nsPresContextType Type() const { return mType; }

  


  nsIPresShell* PresShell() const
  {
    NS_ASSERTION(mShell, "Null pres shell");
    return mShell;
  }

  nsIPresShell* GetPresShell() const { return mShell; }

  



  nsPresContext* GetParentPresContext();

  



  nsPresContext* GetToplevelContentDocumentPresContext();

  







  nsIWidget* GetNearestWidget(nsPoint* aOffset = nullptr);

  



  nsIWidget* GetRootWidget();

  




  nsRootPresContext* GetRootPresContext();
  nsRootPresContext* GetDisplayRootPresContext();
  virtual bool IsRoot() { return false; }

  nsIDocument* Document() const
  {
      NS_ASSERTION(!mShell || !mShell->GetDocument() ||
                   mShell->GetDocument() == mDocument,
                   "nsPresContext doesn't have the same document as nsPresShell!");
      return mDocument;
  }

#ifdef MOZILLA_INTERNAL_API
  nsStyleSet* StyleSet() { return GetPresShell()->StyleSet(); }

  nsFrameManager* FrameManager()
    { return PresShell()->FrameManager(); }

  nsCSSFrameConstructor* FrameConstructor()
    { return PresShell()->FrameConstructor(); }

  nsTransitionManager* TransitionManager() { return mTransitionManager; }
  nsAnimationManager* AnimationManager() { return mAnimationManager; }

  nsRefreshDriver* RefreshDriver() { return mRefreshDriver; }

  mozilla::RestyleManager* RestyleManager() { return mRestyleManager; }

  mozilla::CounterStyleManager* CounterStyleManager() {
    return mCounterStyleManager;
  }
#endif

  






  void RebuildAllStyleData(nsChangeHint aExtraHint, nsRestyleHint aRestyleHint);
  



  void PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint,
                                    nsRestyleHint aRestyleHint);

  



















  void MediaFeatureValuesChanged(nsRestyleHint aRestyleHint,
                                 nsChangeHint aChangeHint = nsChangeHint(0));
  void PostMediaFeatureValuesChangedEvent();
  void HandleMediaFeatureValuesChangedEvent();
  void FlushPendingMediaFeatureValuesChanged() {
    if (mPendingMediaFeatureValuesChanged)
      MediaFeatureValuesChanged(nsRestyleHint(0));
  }

  



  nsCompatibility CompatibilityMode() const;

  


  void CompatibilityModeChanged();

  


  uint16_t     ImageAnimationMode() const { return mImageAnimationMode; }
  virtual void SetImageAnimationModeExternal(uint16_t aMode);
  void SetImageAnimationModeInternal(uint16_t aMode);
#ifdef MOZILLA_INTERNAL_API
  void SetImageAnimationMode(uint16_t aMode)
  { SetImageAnimationModeInternal(aMode); }
#else
  void SetImageAnimationMode(uint16_t aMode)
  { SetImageAnimationModeExternal(aMode); }
#endif

  


  nsIAtom* Medium() {
    if (!mIsEmulatingMedia)
      return mMedium;
    return mMediaEmulated;
  }

  



  void EmulateMedium(const nsAString& aMediaType);

  


  void StopEmulatingMedium();

  void* AllocateFromShell(size_t aSize)
  {
    if (mShell)
      return mShell->AllocateMisc(aSize);
    return nullptr;
  }

  void FreeToShell(size_t aSize, void* aFreeChunk)
  {
    NS_ASSERTION(mShell, "freeing after shutdown");
    if (mShell)
      mShell->FreeMisc(aSize, aFreeChunk);
  }

  


















  const nsFont* GetDefaultFont(uint8_t aFontID,
                                           nsIAtom *aLanguage) const;

  
  
  bool GetCachedBoolPref(nsPresContext_CachedBoolPrefType aPrefType) const
  {
    
    
    switch (aPrefType) {
    case kPresContext_UseDocumentFonts:
      return mUseDocumentFonts;
    case kPresContext_UnderlineLinks:
      return mUnderlineLinks;
    default:
      NS_ERROR("Invalid arg passed to GetCachedBoolPref");
    }

    return false;
  }

  
  
  int32_t GetCachedIntPref(nsPresContext_CachedIntPrefType aPrefType) const
  {
    
    
    switch (aPrefType) {
    case kPresContext_ScrollbarSide:
      return mPrefScrollbarSide;
    case kPresContext_BidiDirection:
      return mPrefBidiDirection;
    default:
      NS_ERROR("invalid arg passed to GetCachedIntPref");
    }

    return false;
  }

  


  const nscolor DefaultColor() const { return mDefaultColor; }
  const nscolor DefaultBackgroundColor() const { return mBackgroundColor; }
  const nscolor DefaultLinkColor() const { return mLinkColor; }
  const nscolor DefaultActiveLinkColor() const { return mActiveLinkColor; }
  const nscolor DefaultVisitedLinkColor() const { return mVisitedLinkColor; }
  const nscolor FocusBackgroundColor() const { return mFocusBackgroundColor; }
  const nscolor FocusTextColor() const { return mFocusTextColor; }

  


  const nscolor BodyTextColor() const { return mBodyTextColor; }
  void SetBodyTextColor(nscolor aColor) { mBodyTextColor = aColor; }

  bool GetUseFocusColors() const { return mUseFocusColors; }
  uint8_t FocusRingWidth() const { return mFocusRingWidth; }
  bool GetFocusRingOnAnything() const { return mFocusRingOnAnything; }
  uint8_t GetFocusRingStyle() const { return mFocusRingStyle; }

  void SetContainer(nsIDocShell* aContainer);

  virtual nsISupports* GetContainerWeakExternal() const;
  nsISupports* GetContainerWeakInternal() const;
#ifdef MOZILLA_INTERNAL_API
  nsISupports* GetContainerWeak() const
  { return GetContainerWeakInternal(); }
#else
  nsISupports* GetContainerWeak() const
  { return GetContainerWeakExternal(); }
#endif

  nsIDocShell* GetDocShell() const;

  
  void SetLinkHandler(nsILinkHandler* aHandler) { mLinkHandler = aHandler; }
  nsILinkHandler* GetLinkHandler() { return mLinkHandler; }

  




  virtual void Detach();

  





  nsRect GetVisibleArea() { return mVisibleArea; }

  



  void SetVisibleArea(const nsRect& r) {
    if (!r.IsEqualEdges(mVisibleArea)) {
      mVisibleArea = r;
      
      if (!IsPaginated() && HasCachedStyleData()) {
        mPendingViewportChange = true;
        PostMediaFeatureValuesChangedEvent();
      }
    }
  }

  



  bool IsPaginated() const { return mPaginated; }

  



  void SetPaginatedScrolling(bool aResult);

  



  bool HasPaginatedScrolling() const { return mCanPaginatedScroll; }

  


  nsSize GetPageSize() { return mPageSize; }
  void SetPageSize(nsSize aSize) { mPageSize = aSize; }

  




  bool IsRootPaginatedDocument() { return mIsRootPaginatedDocument; }
  void SetIsRootPaginatedDocument(bool aIsRootPaginatedDocument)
    { mIsRootPaginatedDocument = aIsRootPaginatedDocument; }

  





  float GetPageScale() { return mPageScale; }
  void SetPageScale(float aScale) { mPageScale = aScale; }

  







  float GetPrintPreviewScale() { return mPPScale; }
  void SetPrintPreviewScale(float aScale) { mPPScale = aScale; }

  nsDeviceContext* DeviceContext() { return mDeviceContext; }
  mozilla::EventStateManager* EventStateManager() { return mEventManager; }
  nsIAtom* GetLanguageFromCharset() { return mLanguage; }

  float TextZoom() { return mTextZoom; }
  void SetTextZoom(float aZoom) {
    if (aZoom == mTextZoom)
      return;

    mTextZoom = aZoom;
    if (HasCachedStyleData()) {
      
      
      MediaFeatureValuesChanged(eRestyle_ForceDescendants,
                                NS_STYLE_HINT_REFLOW);
    }
  }

  





  int32_t MinFontSize(nsIAtom *aLanguage) const {
    const LangGroupFontPrefs *prefs = GetFontPrefsForLang(aLanguage);
    return std::max(mBaseMinFontSize, prefs->mMinimumFontSize);
  }

  



  int32_t BaseMinFontSize() const {
    return mBaseMinFontSize;
  }

  



  void SetBaseMinFontSize(int32_t aMinFontSize) {
    if (aMinFontSize == mBaseMinFontSize)
      return;

    mBaseMinFontSize = aMinFontSize;
    if (HasCachedStyleData()) {
      
      
      MediaFeatureValuesChanged(eRestyle_ForceDescendants,
                                NS_STYLE_HINT_REFLOW);
    }
  }

  float GetFullZoom() { return mFullZoom; }
  void SetFullZoom(float aZoom);

  nscoord GetAutoQualityMinFontSize() {
    return DevPixelsToAppUnits(mAutoQualityMinFontSizePixelsPref);
  }

  








  gfxSize ScreenSizeInchesForFontInflation(bool* aChanged = nullptr);

  static int32_t AppUnitsPerCSSPixel() { return mozilla::AppUnitsPerCSSPixel(); }
  int32_t AppUnitsPerDevPixel() const;
  static int32_t AppUnitsPerCSSInch() { return mozilla::AppUnitsPerCSSInch(); }

  static nscoord CSSPixelsToAppUnits(int32_t aPixels)
  { return NSToCoordRoundWithClamp(float(aPixels) *
             float(AppUnitsPerCSSPixel())); }

  static nscoord CSSPixelsToAppUnits(float aPixels)
  { return NSToCoordRoundWithClamp(aPixels *
             float(AppUnitsPerCSSPixel())); }

  static int32_t AppUnitsToIntCSSPixels(nscoord aAppUnits)
  { return NSAppUnitsToIntPixels(aAppUnits,
             float(AppUnitsPerCSSPixel())); }

  static float AppUnitsToFloatCSSPixels(nscoord aAppUnits)
  { return NSAppUnitsToFloatPixels(aAppUnits,
             float(AppUnitsPerCSSPixel())); }

  nscoord DevPixelsToAppUnits(int32_t aPixels) const
  { return NSIntPixelsToAppUnits(aPixels, AppUnitsPerDevPixel()); }

  int32_t AppUnitsToDevPixels(nscoord aAppUnits) const
  { return NSAppUnitsToIntPixels(aAppUnits,
             float(AppUnitsPerDevPixel())); }

  int32_t CSSPixelsToDevPixels(int32_t aPixels)
  { return AppUnitsToDevPixels(CSSPixelsToAppUnits(aPixels)); }

  float CSSPixelsToDevPixels(float aPixels)
  {
    return NSAppUnitsToFloatPixels(CSSPixelsToAppUnits(aPixels),
                                   float(AppUnitsPerDevPixel()));
  }

  int32_t DevPixelsToIntCSSPixels(int32_t aPixels)
  { return AppUnitsToIntCSSPixels(DevPixelsToAppUnits(aPixels)); }

  float DevPixelsToFloatCSSPixels(int32_t aPixels)
  { return AppUnitsToFloatCSSPixels(DevPixelsToAppUnits(aPixels)); }

  
  nscoord GfxUnitsToAppUnits(gfxFloat aGfxUnits) const;

  gfxFloat AppUnitsToGfxUnits(nscoord aAppUnits) const;

  gfxRect AppUnitsToGfxUnits(const nsRect& aAppRect) const
  { return gfxRect(AppUnitsToGfxUnits(aAppRect.x),
                   AppUnitsToGfxUnits(aAppRect.y),
                   AppUnitsToGfxUnits(aAppRect.width),
                   AppUnitsToGfxUnits(aAppRect.height)); }

  static nscoord CSSTwipsToAppUnits(float aTwips)
  { return NSToCoordRoundWithClamp(
      mozilla::AppUnitsPerCSSInch() * NS_TWIPS_TO_INCHES(aTwips)); }

  
  static nsMargin CSSTwipsToAppUnits(const nsIntMargin &marginInTwips)
  { return nsMargin(CSSTwipsToAppUnits(float(marginInTwips.top)),
                    CSSTwipsToAppUnits(float(marginInTwips.right)),
                    CSSTwipsToAppUnits(float(marginInTwips.bottom)),
                    CSSTwipsToAppUnits(float(marginInTwips.left))); }

  static nscoord CSSPointsToAppUnits(float aPoints)
  { return NSToCoordRound(aPoints * mozilla::AppUnitsPerCSSInch() /
                          POINTS_PER_INCH_FLOAT); }

  nscoord RoundAppUnitsToNearestDevPixels(nscoord aAppUnits) const
  { return DevPixelsToAppUnits(AppUnitsToDevPixels(aAppUnits)); }

  void SetViewportScrollbarStylesOverride(const ScrollbarStyles& aScrollbarStyle)
  {
    mViewportStyleScrollbar = aScrollbarStyle;
  }
  ScrollbarStyles GetViewportScrollbarStylesOverride()
  {
    return mViewportStyleScrollbar;
  }

  


  bool GetBackgroundImageDraw() const { return mDrawImageBackground; }
  void   SetBackgroundImageDraw(bool aCanDraw)
  {
    mDrawImageBackground = aCanDraw;
  }

  bool GetBackgroundColorDraw() const { return mDrawColorBackground; }
  void   SetBackgroundColorDraw(bool aCanDraw)
  {
    mDrawColorBackground = aCanDraw;
  }

  


  bool StyleUpdateForAllAnimationsIsUpToDate() const;
  void TickLastStyleUpdateForAllAnimations();
  void ClearLastStyleUpdateForAllAnimations();

  






#ifdef MOZILLA_INTERNAL_API
  bool BidiEnabled() const { return BidiEnabledInternal(); }
#else
  bool BidiEnabled() const { return BidiEnabledExternal(); }
#endif
  virtual bool BidiEnabledExternal() const;
  bool BidiEnabledInternal() const;

  




  void SetBidiEnabled() const;

  













  void SetVisualMode(bool aIsVisual)
  {
    mIsVisual = aIsVisual;
  }

  




  bool IsVisualMode() const { return mIsVisual; }



  


  void SetBidi(uint32_t aBidiOptions,
                           bool aForceRestyle = false);

  




  uint32_t GetBidi() const;

  


  void SetIsRenderingOnlySelection(bool aResult)
  {
    mIsRenderingOnlySelection = aResult;
  }

  bool IsRenderingOnlySelection() const { return mIsRenderingOnlySelection; }

  bool IsTopLevelWindowInactive();

  


  nsITheme* GetTheme();

  





  void ThemeChanged();

  




  void UIResolutionChanged();

  


  void SysColorChanged();

  
  void SetPrintSettings(nsIPrintSettings *aPrintSettings);

  nsIPrintSettings* GetPrintSettings() { return mPrintSettings; }

  
  FramePropertyTable* PropertyTable() { return &mPropertyTable; }

  



  bool EnsureVisible();

#ifdef MOZ_REFLOW_PERF
  void CountReflows(const char * aName,
                                nsIFrame * aFrame);
#endif

  void ConstructedFrame() {
    ++mFramesConstructed;
  }
  void ReflowedFrame() {
    ++mFramesReflowed;
  }

  uint64_t FramesConstructedCount() {
    return mFramesConstructed;
  }
  uint64_t FramesReflowedCount() {
    return mFramesReflowed;
  }

  



  const nscoord* GetBorderWidthTable() { return mBorderWidthTable; }

  gfxTextPerfMetrics *GetTextPerfMetrics() { return mTextPerf; }

  bool IsDynamic() { return (mType == eContext_PageLayout || mType == eContext_Galley); }
  bool IsScreen() { return (mMedium == nsGkAtoms::screen ||
                              mType == eContext_PageLayout ||
                              mType == eContext_PrintPreview); }

  
  bool IsChrome() const { return mIsChrome; }
  bool IsChromeOriginImage() const { return mIsChromeOriginImage; }
  void UpdateIsChrome();

  
  virtual bool HasAuthorSpecifiedRules(nsIFrame *aFrame, uint32_t ruleTypeMask) const;

  
  bool UseDocumentColors() const {
    MOZ_ASSERT(mUseDocumentColors || !(IsChrome() || IsChromeOriginImage()),
               "We should never have a chrome doc or image that can't use its colors.");
    return mUseDocumentColors;
  }

  
  void SetPaintFlashing(bool aPaintFlashing) {
    mPaintFlashing = aPaintFlashing;
    mPaintFlashingInitialized = true;
  }

  
  
  bool GetPaintFlashing() const;

  bool             SupressingResizeReflow() const { return mSupressResizeReflow; }

  virtual gfxUserFontSet* GetUserFontSetExternal();
  gfxUserFontSet* GetUserFontSetInternal();
#ifdef MOZILLA_INTERNAL_API
  gfxUserFontSet* GetUserFontSet() { return GetUserFontSetInternal(); }
#else
  gfxUserFontSet* GetUserFontSet() { return GetUserFontSetExternal(); }
#endif

  void FlushUserFontSet();
  void RebuildUserFontSet(); 

  
  
  
  void UserFontSetUpdated(gfxUserFontEntry* aUpdatedFont = nullptr);

  gfxMissingFontRecorder *MissingFontRecorder() { return mMissingFonts; }
  void NotifyMissingFonts();

  mozilla::dom::FontFaceSet* Fonts();

  void FlushCounterStyles();
  void RebuildCounterStyles(); 

  
  
  
  void EnsureSafeToHandOutCSSRules();

  void NotifyInvalidation(uint32_t aFlags);
  void NotifyInvalidation(const nsRect& aRect, uint32_t aFlags);
  
  void NotifyInvalidation(const nsIntRect& aRect, uint32_t aFlags);
  
  void NotifyDidPaintForSubtree(uint32_t aFlags);
  void FireDOMPaintEvent(nsInvalidateRequestList* aList);

  
  
  static void NotifySubDocInvalidation(mozilla::layers::ContainerLayer* aContainer,
                                       const nsIntRegion& aRegion);
  void SetNotifySubDocInvalidationData(mozilla::layers::ContainerLayer* aContainer);
  static void ClearNotifySubDocInvalidationData(mozilla::layers::ContainerLayer* aContainer);
  bool IsDOMPaintEventPending();
  void ClearMozAfterPaintEvents() {
    mInvalidateRequestsSinceLastPaint.mRequests.Clear();
    mUndeliveredInvalidateRequestsBeforeLastPaint.mRequests.Clear();
    mAllInvalidated = false;
  }

  


  bool HasPendingRestyleOrReflow();

  



  void NotifyFontFaceSetOnRefresh();

  








  void ReflowStarted(bool aInterruptible);

  


  class InterruptPreventer;
  friend class InterruptPreventer;
  class MOZ_STACK_CLASS InterruptPreventer {
  public:
    explicit InterruptPreventer(nsPresContext* aCtx) :
      mCtx(aCtx),
      mInterruptsEnabled(aCtx->mInterruptsEnabled),
      mHasPendingInterrupt(aCtx->mHasPendingInterrupt)
    {
      mCtx->mInterruptsEnabled = false;
      mCtx->mHasPendingInterrupt = false;
    }
    ~InterruptPreventer() {
      mCtx->mInterruptsEnabled = mInterruptsEnabled;
      mCtx->mHasPendingInterrupt = mHasPendingInterrupt;
    }

  private:
    nsPresContext* mCtx;
    bool mInterruptsEnabled;
    bool mHasPendingInterrupt;
  };

  







  bool CheckForInterrupt(nsIFrame* aFrame);
  



  bool HasPendingInterrupt() { return mHasPendingInterrupt; }

  





  nsIFrame* GetPrimaryFrameFor(nsIContent* aContent);

  void NotifyDestroyingFrame(nsIFrame* aFrame)
  {
    PropertyTable()->DeleteAllFor(aFrame);
  }

  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  bool IsRootContentDocument();
  bool IsCrossProcessRootContentDocument();

  bool IsGlyph() const {
    return mIsGlyph;
  }

  void SetIsGlyph(bool aValue) {
    mIsGlyph = aValue;
  }

  bool UsesRootEMUnits() const {
    return mUsesRootEMUnits;
  }

  void SetUsesRootEMUnits(bool aValue) {
    mUsesRootEMUnits = aValue;
  }

  bool UsesExChUnits() const {
    return mUsesExChUnits;
  }

  void SetUsesExChUnits(bool aValue) {
    mUsesExChUnits = aValue;
  }

  bool UsesViewportUnits() const {
    return mUsesViewportUnits;
  }

  void SetUsesViewportUnits(bool aValue) {
    mUsesViewportUnits = aValue;
  }

  
  
  
  bool ExistThrottledUpdates() const {
    return mExistThrottledUpdates;
  }

  void SetExistThrottledUpdates(bool aExistThrottledUpdates) {
    mExistThrottledUpdates = aExistThrottledUpdates;
  }

  bool IsDeviceSizePageSize();

  bool HasWarnedAboutPositionedTableParts() const {
    return mHasWarnedAboutPositionedTableParts;
  }

  void SetHasWarnedAboutPositionedTableParts() {
    mHasWarnedAboutPositionedTableParts = true;
  }

protected:
  friend class nsRunnableMethod<nsPresContext>;
  void ThemeChangedInternal();
  void SysColorChangedInternal();
  void UIResolutionChangedInternal();

  static bool
  UIResolutionChangedSubdocumentCallback(nsIDocument* aDocument, void* aData);

  void SetImgAnimations(nsIContent *aParent, uint16_t aMode);
  void SetSMILAnimations(nsIDocument *aDoc, uint16_t aNewMode,
                                     uint16_t aOldMode);
  void GetDocumentColorPreferences();

  void PreferenceChanged(const char* aPrefName);
  static void PrefChangedCallback(const char*, void*);

  void UpdateAfterPreferencesChanged();
  static void PrefChangedUpdateTimerCallback(nsITimer *aTimer, void *aClosure);

  void GetUserPreferences();

  
  
  struct LangGroupFontPrefs;
  friend class nsAutoPtr<LangGroupFontPrefs>;
  struct LangGroupFontPrefs {
    
    LangGroupFontPrefs()
      : mLangGroup(nullptr)
      , mMinimumFontSize(0)
      , mDefaultVariableFont(mozilla::eFamily_serif, NS_FONT_STYLE_NORMAL,
                             NS_FONT_WEIGHT_NORMAL,
                             NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultFixedFont(mozilla::eFamily_monospace, NS_FONT_STYLE_NORMAL,
                          NS_FONT_WEIGHT_NORMAL,
                          NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultSerifFont(mozilla::eFamily_serif, NS_FONT_STYLE_NORMAL,
                          NS_FONT_WEIGHT_NORMAL,
                          NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultSansSerifFont(mozilla::eFamily_sans_serif,
                              NS_FONT_STYLE_NORMAL,
                              NS_FONT_WEIGHT_NORMAL,
                              NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultMonospaceFont(mozilla::eFamily_monospace, NS_FONT_STYLE_NORMAL,
                              NS_FONT_WEIGHT_NORMAL,
                              NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultCursiveFont(mozilla::eFamily_cursive, NS_FONT_STYLE_NORMAL,
                            NS_FONT_WEIGHT_NORMAL,
                            NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultFantasyFont(mozilla::eFamily_fantasy, NS_FONT_STYLE_NORMAL,
                            NS_FONT_WEIGHT_NORMAL,
                            NS_FONT_STRETCH_NORMAL, 0, 0)
    {}

    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
      size_t n = 0;
      LangGroupFontPrefs *curr = mNext;
      while (curr) {
        n += aMallocSizeOf(curr);

        
        
        
        

        curr = curr->mNext;
      }
      return n;
    }

    nsCOMPtr<nsIAtom> mLangGroup;
    nscoord mMinimumFontSize;
    nsFont mDefaultVariableFont;
    nsFont mDefaultFixedFont;
    nsFont mDefaultSerifFont;
    nsFont mDefaultSansSerifFont;
    nsFont mDefaultMonospaceFont;
    nsFont mDefaultCursiveFont;
    nsFont mDefaultFantasyFont;
    nsAutoPtr<LangGroupFontPrefs> mNext;
  };

  



  const LangGroupFontPrefs* GetFontPrefsForLang(nsIAtom *aLanguage) const;

  void ResetCachedFontPrefs() {
    
    mLangGroupFontPrefs.mNext = nullptr;

    
    mLangGroupFontPrefs.mLangGroup = nullptr;
  }

  void UpdateCharSet(const nsCString& aCharSet);

public:
  void DoChangeCharSet(const nsCString& aCharSet);

  


  bool MayHavePaintEventListener();

  




  bool MayHavePaintEventListenerInSubDocument();

#ifdef RESTYLE_LOGGING
  
  
  bool RestyleLoggingEnabled() const { return mRestyleLoggingEnabled; }
  void StartRestyleLogging() { mRestyleLoggingEnabled = true; }
  void StopRestyleLogging() { mRestyleLoggingEnabled = false; }
#endif

  void InvalidatePaintedLayers();

protected:
  
  void Destroy();

  void AppUnitsPerDevPixelChanged();

  void HandleRebuildUserFontSet() {
    mPostedFlushUserFontSet = false;
    FlushUserFontSet();
  }

  void HandleRebuildCounterStyles() {
    mPostedFlushCounterStyles = false;
    FlushCounterStyles();
  }

  bool HavePendingInputEvent();

  
  bool HasCachedStyleData();

  bool IsChromeSlow() const;

  
  
  

  nsPresContextType     mType;
  nsIPresShell*         mShell;         
  nsCOMPtr<nsIDocument> mDocument;
  nsRefPtr<nsDeviceContext> mDeviceContext; 
                                            
                                            
                                            
                                            
  nsRefPtr<mozilla::EventStateManager> mEventManager;
  nsRefPtr<nsRefreshDriver> mRefreshDriver;
  nsRefPtr<nsTransitionManager> mTransitionManager;
  nsRefPtr<nsAnimationManager> mAnimationManager;
  nsRefPtr<mozilla::RestyleManager> mRestyleManager;
  nsRefPtr<mozilla::CounterStyleManager> mCounterStyleManager;
  nsIAtom*              mMedium;        
                                        
  nsCOMPtr<nsIAtom> mMediaEmulated;

  nsILinkHandler*       mLinkHandler;   

  
  
  
  
  
  nsCOMPtr<nsIAtom>     mLanguage;

public:
  
  

  
  
  bool                  mInflationDisabledForShrinkWrap;

protected:

  mozilla::WeakPtr<nsDocShell>             mContainer;

  
  int32_t               mBaseMinFontSize;
  float                 mTextZoom;      
  float                 mFullZoom;      

  gfxSize               mLastFontInflationScreenSize;

  int32_t               mCurAppUnitsPerDevPixel;
  int32_t               mAutoQualityMinFontSizePixelsPref;

  nsCOMPtr<nsITheme> mTheme;
  nsCOMPtr<nsILanguageAtomService> mLangService;
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  nsCOMPtr<nsITimer>    mPrefChangedTimer;

  FramePropertyTable    mPropertyTable;

  nsInvalidateRequestList mInvalidateRequestsSinceLastPaint;
  nsInvalidateRequestList mUndeliveredInvalidateRequestsBeforeLastPaint;

  
  nsRefPtr<mozilla::dom::FontFaceSet> mFontFaceSet;

  
  nsAutoPtr<gfxTextPerfMetrics>   mTextPerf;

  nsAutoPtr<gfxMissingFontRecorder> mMissingFonts;

  nsRect                mVisibleArea;
  nsSize                mPageSize;
  float                 mPageScale;
  float                 mPPScale;

  nscolor               mDefaultColor;
  nscolor               mBackgroundColor;

  nscolor               mLinkColor;
  nscolor               mActiveLinkColor;
  nscolor               mVisitedLinkColor;

  nscolor               mFocusBackgroundColor;
  nscolor               mFocusTextColor;

  nscolor               mBodyTextColor;

  ScrollbarStyles       mViewportStyleScrollbar;
  uint8_t               mFocusRingWidth;

  bool mExistThrottledUpdates;

  uint16_t              mImageAnimationMode;
  uint16_t              mImageAnimationModePref;

  LangGroupFontPrefs    mLangGroupFontPrefs;

  nscoord               mBorderWidthTable[3];

  uint32_t              mInterruptChecksToSkip;

  
  
  uint64_t              mFramesConstructed;
  uint64_t              mFramesReflowed;

  mozilla::TimeStamp    mReflowStartTime;

  
  mozilla::TimeStamp    mLastStyleUpdateForAllAnimations;

  unsigned              mHasPendingInterrupt : 1;
  unsigned              mInterruptsEnabled : 1;
  unsigned              mUseDocumentFonts : 1;
  unsigned              mUseDocumentColors : 1;
  unsigned              mUnderlineLinks : 1;
  unsigned              mSendAfterPaintToContent : 1;
  unsigned              mUseFocusColors : 1;
  unsigned              mFocusRingOnAnything : 1;
  unsigned              mFocusRingStyle : 1;
  unsigned              mDrawImageBackground : 1;
  unsigned              mDrawColorBackground : 1;
  unsigned              mNeverAnimate : 1;
  unsigned              mIsRenderingOnlySelection : 1;
  unsigned              mPaginated : 1;
  unsigned              mCanPaginatedScroll : 1;
  unsigned              mDoScaledTwips : 1;
  unsigned              mIsRootPaginatedDocument : 1;
  unsigned              mPrefBidiDirection : 1;
  unsigned              mPrefScrollbarSide : 2;
  unsigned              mPendingSysColorChanged : 1;
  unsigned              mPendingThemeChanged : 1;
  unsigned              mPendingUIResolutionChanged : 1;
  unsigned              mPendingMediaFeatureValuesChanged : 1;
  unsigned              mPrefChangePendingNeedsReflow : 1;
  unsigned              mIsEmulatingMedia : 1;
  
  
  unsigned              mAllInvalidated : 1;

  
  unsigned              mIsGlyph : 1;

  
  unsigned              mUsesRootEMUnits : 1;
  
  unsigned              mUsesExChUnits : 1;
  
  unsigned              mUsesViewportUnits : 1;

  
  unsigned              mPendingViewportChange : 1;

  
  unsigned              mFontFaceSetDirty : 1;
  
  unsigned              mGetUserFontSetCalled : 1;
  
  unsigned              mPostedFlushUserFontSet : 1;

  
  unsigned              mCounterStylesDirty : 1;
  
  unsigned              mPostedFlushCounterStyles: 1;

  
  
  unsigned              mSupressResizeReflow : 1;

  unsigned              mIsVisual : 1;

  unsigned              mFireAfterPaintEvents : 1;

  unsigned              mIsChrome : 1;
  unsigned              mIsChromeOriginImage : 1;

  
  
  mutable unsigned mPaintFlashing : 1;
  mutable unsigned mPaintFlashingInitialized : 1;

  unsigned mHasWarnedAboutPositionedTableParts : 1;

#ifdef RESTYLE_LOGGING
  
  bool                  mRestyleLoggingEnabled;
#endif

#ifdef DEBUG
  bool                  mInitialized;
#endif


protected:

  virtual ~nsPresContext();

  
  enum {
    eDefaultFont_Variable,
    eDefaultFont_Fixed,
    eDefaultFont_Serif,
    eDefaultFont_SansSerif,
    eDefaultFont_Monospace,
    eDefaultFont_Cursive,
    eDefaultFont_Fantasy,
    eDefaultFont_COUNT
  };

  nscolor MakeColorPref(const nsString& aColor);

  void LastRelease();

#ifdef DEBUG
private:
  friend struct nsAutoLayoutPhase;
  uint32_t mLayoutPhaseCount[eLayoutPhase_COUNT];
public:
  uint32_t LayoutPhaseCount(nsLayoutPhase aPhase) {
    return mLayoutPhaseCount[aPhase];
  }
#endif

};

class nsRootPresContext final : public nsPresContext {
public:
  nsRootPresContext(nsIDocument* aDocument, nsPresContextType aType);
  virtual ~nsRootPresContext();
  virtual void Detach() override;

  



  void EnsureEventualDidPaintEvent();

  void CancelDidPaintTimer()
  {
    if (mNotifyDidPaintTimer) {
      mNotifyDidPaintTimer->Cancel();
      mNotifyDidPaintTimer = nullptr;
    }
  }

  





  void RegisterPluginForGeometryUpdates(nsIContent* aPlugin);
  




  void UnregisterPluginForGeometryUpdates(nsIContent* aPlugin);

  bool NeedToComputePluginGeometryUpdates()
  {
    return mRegisteredPlugins.Count() > 0;
  }
  








  void ComputePluginGeometryUpdates(nsIFrame* aFrame,
                                    nsDisplayListBuilder* aBuilder,
                                    nsDisplayList* aList);

  




  void ApplyPluginGeometryUpdates();

  



  void CollectPluginGeometryUpdates(mozilla::layers::LayerManager* aLayerManager);

  virtual bool IsRoot() override { return true; }

  




  void IncrementDOMGeneration() { mDOMGeneration++; }

  





  uint32_t GetDOMGeneration() { return mDOMGeneration; }

  




  void AddWillPaintObserver(nsIRunnable* aRunnable);

  


  void FlushWillPaintObservers();

  virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const override;

protected:
  


  void InitApplyPluginGeometryTimer();
  


  void CancelApplyPluginGeometryTimer();

  class RunWillPaintObservers : public nsRunnable {
  public:
    explicit RunWillPaintObservers(nsRootPresContext* aPresContext) : mPresContext(aPresContext) {}
    void Revoke() { mPresContext = nullptr; }
    NS_IMETHOD Run() override
    {
      if (mPresContext) {
        mPresContext->FlushWillPaintObservers();
      }
      return NS_OK;
    }
    nsRootPresContext* mPresContext;
  };

  friend class nsPresContext;

  nsCOMPtr<nsITimer> mNotifyDidPaintTimer;
  nsCOMPtr<nsITimer> mApplyPluginGeometryTimer;
  nsTHashtable<nsRefPtrHashKey<nsIContent> > mRegisteredPlugins;
  nsTArray<nsCOMPtr<nsIRunnable> > mWillPaintObservers;
  nsRevocableEventPtr<RunWillPaintObservers> mWillPaintFallbackEvent;
  uint32_t mDOMGeneration;
};

#ifdef MOZ_REFLOW_PERF

#define DO_GLOBAL_REFLOW_COUNT(_name) \
  aPresContext->CountReflows((_name), (nsIFrame*)this);
#else
#define DO_GLOBAL_REFLOW_COUNT(_name)
#endif 

#endif 
