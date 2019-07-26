






#ifndef nsPresContext_h___
#define nsPresContext_h___

#include "mozilla/Attributes.h"
#include "nsISupports.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsRect.h"
#include "nsDeviceContext.h"
#include "nsFont.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsCRT.h"
#include "FramePropertyTable.h"
#include "nsGkAtoms.h"
#include "nsRefPtrHashtable.h"
#include "nsCycleCollectionParticipant.h"
#include "nsChangeHint.h"
#include <algorithm>

#include "gfxRect.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsIWidget.h"
#include "mozilla/TimeStamp.h"
#include "prclist.h"
#include "Layers.h"
#include "nsRefreshDriver.h"

#ifdef IBMBIDI
class nsBidiPresUtils;
#endif 

struct nsRect;

class imgIRequest;

class nsAString;
class nsIPrintSettings;
class nsIDocument;
class nsILanguageAtomService;
class nsITheme;
class nsIContent;
class nsFontMetrics;
class nsIFrame;
class nsFrameManager;
class nsILinkHandler;
class nsStyleContext;
class nsIAtom;
class nsEventStateManager;
class nsIURI;
class nsICSSPseudoComparator;
struct nsStyleBackground;
struct nsStyleBorder;
class nsIRunnable;
class gfxUserFontSet;
class nsUserFontSet;
struct nsFontFaceRuleContainer;
class nsObjectFrame;
class nsTransitionManager;
class nsAnimationManager;
class imgIContainer;
class nsIDOMMediaQueryList;

#ifdef MOZ_REFLOW_PERF
class nsRenderingContext;
#endif


enum nsPresContext_CachedBoolPrefType {
  kPresContext_UseDocumentColors = 1,
  kPresContext_UseDocumentFonts,
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






class ContainerLayerPresContext : public mozilla::layers::LayerUserData {
public:
  nsPresContext* mPresContext;
};
extern uint8_t gNotifySubDocInvalidationData;


#define NS_AUTHOR_SPECIFIED_BACKGROUND      (1 << 0)
#define NS_AUTHOR_SPECIFIED_BORDER          (1 << 1)
#define NS_AUTHOR_SPECIFIED_PADDING         (1 << 2)
#define NS_AUTHOR_SPECIFIED_TEXT_SHADOW     (1 << 3)

class nsRootPresContext;




class nsPresContext : public nsIObserver {
public:
  typedef mozilla::FramePropertyTable FramePropertyTable;

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

  
  enum StyleRebuildType {
    eRebuildStyleIfNeeded,
    eAlwaysRebuildStyle
  };

  nsPresContext(nsIDocument* aDocument, nsPresContextType aType) NS_HIDDEN;

  


  NS_HIDDEN_(nsresult) Init(nsDeviceContext* aDeviceContext);

  



  NS_HIDDEN_(void) SetShell(nsIPresShell* aShell);


  NS_HIDDEN_(nsPresContextType) Type() const { return mType; }

  


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

#ifdef _IMPL_NS_LAYOUT
  nsStyleSet* StyleSet() { return GetPresShell()->StyleSet(); }

  nsFrameManager* FrameManager()
    { return GetPresShell()->FrameManager(); }

  nsTransitionManager* TransitionManager() { return mTransitionManager; }
  nsAnimationManager* AnimationManager() { return mAnimationManager; }

  nsRefreshDriver* RefreshDriver() { return mRefreshDriver; }
#endif

  





  void RebuildAllStyleData(nsChangeHint aExtraHint);
  



  void PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint);

  void MediaFeatureValuesChanged(StyleRebuildType aShouldRebuild,
                                 nsChangeHint aChangeHint = nsChangeHint(0));
  void PostMediaFeatureValuesChangedEvent();
  NS_HIDDEN_(void) HandleMediaFeatureValuesChangedEvent();
  void FlushPendingMediaFeatureValuesChanged() {
    if (mPendingMediaFeatureValuesChanged)
      MediaFeatureValuesChanged(eRebuildStyleIfNeeded);
  }

  


  void MatchMedia(const nsAString& aMediaQueryList,
                  nsIDOMMediaQueryList** aResult);

  



  nsCompatibility CompatibilityMode() const;

  


  NS_HIDDEN_(void) CompatibilityModeChanged();

  


  uint16_t     ImageAnimationMode() const { return mImageAnimationMode; }
  virtual NS_HIDDEN_(void) SetImageAnimationModeExternal(uint16_t aMode);
  NS_HIDDEN_(void) SetImageAnimationModeInternal(uint16_t aMode);
#ifdef _IMPL_NS_LAYOUT
  void SetImageAnimationMode(uint16_t aMode)
  { SetImageAnimationModeInternal(aMode); }
#else
  void SetImageAnimationMode(uint16_t aMode)
  { SetImageAnimationModeExternal(aMode); }
#endif

  


  nsIAtom* Medium() { return mMedium; }

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

  


















  NS_HIDDEN_(const nsFont*) GetDefaultFont(uint8_t aFontID,
                                           nsIAtom *aLanguage) const;

  
  
  bool GetCachedBoolPref(nsPresContext_CachedBoolPrefType aPrefType) const
  {
    
    
    switch (aPrefType) {
    case kPresContext_UseDocumentFonts:
      return mUseDocumentFonts;
    case kPresContext_UseDocumentColors:
      return mUseDocumentColors;
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

  NS_HIDDEN_(void) SetContainer(nsISupports* aContainer);

  virtual NS_HIDDEN_(already_AddRefed<nsISupports>) GetContainerExternal() const;
  NS_HIDDEN_(already_AddRefed<nsISupports>) GetContainerInternal() const;
#ifdef _IMPL_NS_LAYOUT
  already_AddRefed<nsISupports> GetContainer() const
  { return GetContainerInternal(); }
#else
  already_AddRefed<nsISupports> GetContainer() const
  { return GetContainerExternal(); }
#endif

  
  void SetLinkHandler(nsILinkHandler* aHandler) { mLinkHandler = aHandler; }
  nsILinkHandler* GetLinkHandler() { return mLinkHandler; }

  





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
  
  



  NS_HIDDEN_(void) SetPaginatedScrolling(bool aResult);

  



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
  nsEventStateManager* EventStateManager() { return mEventManager; }
  nsIAtom* GetLanguageFromCharset() { return mLanguage; }

  float TextZoom() { return mTextZoom; }
  void SetTextZoom(float aZoom) {
    if (aZoom == mTextZoom)
      return;

    mTextZoom = aZoom;
    if (HasCachedStyleData()) {
      
      
      MediaFeatureValuesChanged(eAlwaysRebuildStyle, NS_STYLE_HINT_REFLOW);
    }
  }

  



  int32_t MinFontSize(nsIAtom *aLanguage) const {
    const LangGroupFontPrefs *prefs = GetFontPrefsForLang(aLanguage);
    return std::max(mMinFontSize, prefs->mMinimumFontSize);
  }

  void SetMinFontSize(int32_t aMinFontSize) {
    if (aMinFontSize == mMinFontSize)
      return;

    mMinFontSize = aMinFontSize;
    if (HasCachedStyleData()) {
      
      
      MediaFeatureValuesChanged(eAlwaysRebuildStyle, NS_STYLE_HINT_REFLOW);
    }
  }

  float GetFullZoom() { return mFullZoom; }
  void SetFullZoom(float aZoom);

  nscoord GetAutoQualityMinFontSize() {
    return DevPixelsToAppUnits(mAutoQualityMinFontSizePixelsPref);
  }

  








  float ScreenWidthInchesForFontInflation(bool* aChanged = nullptr);

  static int32_t AppUnitsPerCSSPixel() { return nsDeviceContext::AppUnitsPerCSSPixel(); }
  int32_t AppUnitsPerDevPixel() const  { return mDeviceContext->AppUnitsPerDevPixel(); }
  static int32_t AppUnitsPerCSSInch() { return nsDeviceContext::AppUnitsPerCSSInch(); }

  static nscoord CSSPixelsToAppUnits(int32_t aPixels)
  { return NSIntPixelsToAppUnits(aPixels,
                                 nsDeviceContext::AppUnitsPerCSSPixel()); }

  static nscoord CSSPixelsToAppUnits(float aPixels)
  { return NSFloatPixelsToAppUnits(aPixels,
             float(nsDeviceContext::AppUnitsPerCSSPixel())); }

  static int32_t AppUnitsToIntCSSPixels(nscoord aAppUnits)
  { return NSAppUnitsToIntPixels(aAppUnits,
             float(nsDeviceContext::AppUnitsPerCSSPixel())); }

  static float AppUnitsToFloatCSSPixels(nscoord aAppUnits)
  { return NSAppUnitsToFloatPixels(aAppUnits,
             float(nsDeviceContext::AppUnitsPerCSSPixel())); }

  nscoord DevPixelsToAppUnits(int32_t aPixels) const
  { return NSIntPixelsToAppUnits(aPixels,
                                 mDeviceContext->AppUnitsPerDevPixel()); }

  int32_t AppUnitsToDevPixels(nscoord aAppUnits) const
  { return NSAppUnitsToIntPixels(aAppUnits,
             float(mDeviceContext->AppUnitsPerDevPixel())); }

  int32_t CSSPixelsToDevPixels(int32_t aPixels)
  { return AppUnitsToDevPixels(CSSPixelsToAppUnits(aPixels)); }

  float CSSPixelsToDevPixels(float aPixels)
  {
    return NSAppUnitsToFloatPixels(CSSPixelsToAppUnits(aPixels),
                                   float(mDeviceContext->AppUnitsPerDevPixel()));
  }

  int32_t DevPixelsToIntCSSPixels(int32_t aPixels)
  { return AppUnitsToIntCSSPixels(DevPixelsToAppUnits(aPixels)); }

  float DevPixelsToFloatCSSPixels(int32_t aPixels)
  { return AppUnitsToFloatCSSPixels(DevPixelsToAppUnits(aPixels)); }

  
  nscoord GfxUnitsToAppUnits(gfxFloat aGfxUnits) const
  { return mDeviceContext->GfxUnitsToAppUnits(aGfxUnits); }

  gfxFloat AppUnitsToGfxUnits(nscoord aAppUnits) const
  { return mDeviceContext->AppUnitsToGfxUnits(aAppUnits); }

  gfxRect AppUnitsToGfxUnits(const nsRect& aAppRect) const
  { return gfxRect(AppUnitsToGfxUnits(aAppRect.x),
                   AppUnitsToGfxUnits(aAppRect.y),
                   AppUnitsToGfxUnits(aAppRect.width),
                   AppUnitsToGfxUnits(aAppRect.height)); }

  static nscoord CSSTwipsToAppUnits(float aTwips)
  { return NSToCoordRoundWithClamp(
      nsDeviceContext::AppUnitsPerCSSInch() * NS_TWIPS_TO_INCHES(aTwips)); }

  
  static nsMargin CSSTwipsToAppUnits(const nsIntMargin &marginInTwips)
  { return nsMargin(CSSTwipsToAppUnits(float(marginInTwips.left)), 
                    CSSTwipsToAppUnits(float(marginInTwips.top)),
                    CSSTwipsToAppUnits(float(marginInTwips.right)),
                    CSSTwipsToAppUnits(float(marginInTwips.bottom))); }

  static nscoord CSSPointsToAppUnits(float aPoints)
  { return NSToCoordRound(aPoints * nsDeviceContext::AppUnitsPerCSSInch() /
                          POINTS_PER_INCH_FLOAT); }

  nscoord RoundAppUnitsToNearestDevPixels(nscoord aAppUnits) const
  { return DevPixelsToAppUnits(AppUnitsToDevPixels(aAppUnits)); }

  struct ScrollbarStyles {
    
    
    uint8_t mHorizontal, mVertical;
    ScrollbarStyles(uint8_t h, uint8_t v) : mHorizontal(h), mVertical(v) {}
    ScrollbarStyles() {}
    bool operator==(const ScrollbarStyles& aStyles) const {
      return aStyles.mHorizontal == mHorizontal && aStyles.mVertical == mVertical;
    }
    bool operator!=(const ScrollbarStyles& aStyles) const {
      return aStyles.mHorizontal != mHorizontal || aStyles.mVertical != mVertical;
    }
  };
  void SetViewportOverflowOverride(uint8_t aX, uint8_t aY)
  {
    mViewportStyleOverflow.mHorizontal = aX;
    mViewportStyleOverflow.mVertical = aY;
  }
  ScrollbarStyles GetViewportOverflowOverride()
  {
    return mViewportStyleOverflow;
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
  
  


  bool ThrottledStyleIsUpToDate() const {
    return mLastUpdateThrottledStyle == mRefreshDriver->MostRecentRefresh();
  }
  void TickLastUpdateThrottledStyle() {
    mLastUpdateThrottledStyle = mRefreshDriver->MostRecentRefresh();
  }
  bool StyleUpdateForAllAnimationsIsUpToDate() const {
    return mLastStyleUpdateForAllAnimations == mRefreshDriver->MostRecentRefresh();
  }
  void TickLastStyleUpdateForAllAnimations() {
    mLastStyleUpdateForAllAnimations = mRefreshDriver->MostRecentRefresh();
  }

#ifdef IBMBIDI
  






#ifdef _IMPL_NS_LAYOUT
  bool BidiEnabled() const { return BidiEnabledInternal(); }
#else
  bool BidiEnabled() const { return BidiEnabledExternal(); }
#endif
  virtual bool BidiEnabledExternal() const;
  bool BidiEnabledInternal() const;

  




  NS_HIDDEN_(void) SetBidiEnabled() const;

  













  void SetVisualMode(bool aIsVisual)
  {
    mIsVisual = aIsVisual;
  }

  




  bool IsVisualMode() const { return mIsVisual; }



  

  
  NS_HIDDEN_(void) SetBidi(uint32_t aBidiOptions,
                           bool aForceRestyle = false);

  




  NS_HIDDEN_(uint32_t) GetBidi() const;
#endif 

  


  void SetIsRenderingOnlySelection(bool aResult)
  {
    mIsRenderingOnlySelection = aResult;
  }

  bool IsRenderingOnlySelection() const { return mIsRenderingOnlySelection; }

  NS_HIDDEN_(bool) IsTopLevelWindowInactive();

  


  NS_HIDDEN_(nsITheme*) GetTheme();

  





  NS_HIDDEN_(void) ThemeChanged();

  




  NS_HIDDEN_(void) UIResolutionChanged();

  


  NS_HIDDEN_(void) SysColorChanged();

  
  NS_HIDDEN_(void) SetPrintSettings(nsIPrintSettings *aPrintSettings);

  nsIPrintSettings* GetPrintSettings() { return mPrintSettings; }

  
  FramePropertyTable* PropertyTable() { return &mPropertyTable; }

  



  NS_HIDDEN_(bool) EnsureVisible();
  
#ifdef MOZ_REFLOW_PERF
  NS_HIDDEN_(void) CountReflows(const char * aName,
                                nsIFrame * aFrame);
#endif

  



  const nscoord* GetBorderWidthTable() { return mBorderWidthTable; }

  bool IsDynamic() { return (mType == eContext_PageLayout || mType == eContext_Galley); }
  bool IsScreen() { return (mMedium == nsGkAtoms::screen ||
                              mType == eContext_PageLayout ||
                              mType == eContext_PrintPreview); }

  
  bool IsChrome() const
  {
    return mIsChromeIsCached ? mIsChrome : IsChromeSlow();
  }

  virtual void InvalidateIsChromeCacheExternal();
  void InvalidateIsChromeCacheInternal() { mIsChromeIsCached = false; }
#ifdef _IMPL_NS_LAYOUT
  void InvalidateIsChromeCache()
  { InvalidateIsChromeCacheInternal(); }
#else
  void InvalidateIsChromeCache()
  { InvalidateIsChromeCacheExternal(); }
#endif

  
  virtual bool HasAuthorSpecifiedRules(nsIFrame *aFrame, uint32_t ruleTypeMask) const;

  
  bool UseDocumentColors() const {
    return GetCachedBoolPref(kPresContext_UseDocumentColors) || IsChrome();
  }

  bool             SupressingResizeReflow() const { return mSupressResizeReflow; }
  
  virtual NS_HIDDEN_(gfxUserFontSet*) GetUserFontSetExternal();
  NS_HIDDEN_(gfxUserFontSet*) GetUserFontSetInternal();
#ifdef _IMPL_NS_LAYOUT
  gfxUserFontSet* GetUserFontSet() { return GetUserFontSetInternal(); }
#else
  gfxUserFontSet* GetUserFontSet() { return GetUserFontSetExternal(); }
#endif

  void FlushUserFontSet();
  void RebuildUserFontSet(); 

  
  
  
  void UserFontSetUpdated();

  
  
  
  
  bool EnsureSafeToHandOutCSSRules();

  void NotifyInvalidation(uint32_t aFlags);
  void NotifyInvalidation(const nsRect& aRect, uint32_t aFlags);
  
  void NotifyInvalidation(const nsIntRect& aRect, uint32_t aFlags);
  
  void NotifyDidPaintForSubtree(uint32_t aFlags);
  void FireDOMPaintEvent(nsInvalidateRequestList* aList);

  
  
  static void NotifySubDocInvalidation(mozilla::layers::ContainerLayer* aContainer,
                                       const nsIntRegion& aRegion);
  bool IsDOMPaintEventPending();
  void ClearMozAfterPaintEvents() {
    mInvalidateRequestsSinceLastPaint.mRequests.Clear();
    mUndeliveredInvalidateRequestsBeforeLastPaint.mRequests.Clear();
    mAllInvalidated = false;
  }

  bool IsProcessingRestyles() const {
    return mProcessingRestyles;
  }

  void SetProcessingRestyles(bool aProcessing) {
    NS_ASSERTION(aProcessing != bool(mProcessingRestyles),
                 "should never nest");
    mProcessingRestyles = aProcessing;
  }

  bool IsProcessingAnimationStyleChange() const {
    return mProcessingAnimationStyleChange;
  }

  void SetProcessingAnimationStyleChange(bool aProcessing) {
    NS_ASSERTION(aProcessing != bool(mProcessingAnimationStyleChange),
                 "should never nest");
    mProcessingAnimationStyleChange = aProcessing;
  }

  








  void ReflowStarted(bool aInterruptible);

  


  class InterruptPreventer;
  friend class InterruptPreventer;
  class NS_STACK_CLASS InterruptPreventer {
  public:
    InterruptPreventer(nsPresContext* aCtx) :
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

  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const;
  virtual size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
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

protected:
  friend class nsRunnableMethod<nsPresContext>;
  NS_HIDDEN_(void) ThemeChangedInternal();
  NS_HIDDEN_(void) SysColorChangedInternal();
  NS_HIDDEN_(void) UIResolutionChangedInternal();

  static NS_HIDDEN_(bool)
  UIResolutionChangedSubdocumentCallback(nsIDocument* aDocument, void* aData);

  NS_HIDDEN_(void) SetImgAnimations(nsIContent *aParent, uint16_t aMode);
  NS_HIDDEN_(void) SetSMILAnimations(nsIDocument *aDoc, uint16_t aNewMode,
                                     uint16_t aOldMode);
  NS_HIDDEN_(void) GetDocumentColorPreferences();

  NS_HIDDEN_(void) PreferenceChanged(const char* aPrefName);
  static NS_HIDDEN_(int) PrefChangedCallback(const char*, void*);

  NS_HIDDEN_(void) UpdateAfterPreferencesChanged();
  static NS_HIDDEN_(void) PrefChangedUpdateTimerCallback(nsITimer *aTimer, void *aClosure);

  NS_HIDDEN_(void) GetUserPreferences();

  
  
  struct LangGroupFontPrefs;
  friend class nsAutoPtr<LangGroupFontPrefs>;
  struct LangGroupFontPrefs {
    
    LangGroupFontPrefs()
      : mLangGroup(nullptr)
      , mMinimumFontSize(0)
      , mDefaultVariableFont("serif", NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                             NS_FONT_WEIGHT_NORMAL, NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultFixedFont("monospace", NS_FONT_STYLE_NORMAL,
                          NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL,
                          NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultSerifFont("serif", NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                        NS_FONT_WEIGHT_NORMAL, NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultSansSerifFont("sans-serif", NS_FONT_STYLE_NORMAL,
                              NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL,
                              NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultMonospaceFont("monospace", NS_FONT_STYLE_NORMAL,
                              NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL,
                              NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultCursiveFont("cursive", NS_FONT_STYLE_NORMAL,
                            NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL,
                            NS_FONT_STRETCH_NORMAL, 0, 0)
      , mDefaultFantasyFont("fantasy", NS_FONT_STYLE_NORMAL,
                            NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL,
                            NS_FONT_STRETCH_NORMAL, 0, 0)
    {}

    size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
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

  NS_HIDDEN_(void) UpdateCharSet(const nsCString& aCharSet);

public:
  void DoChangeCharSet(const nsCString& aCharSet);

  


  bool MayHavePaintEventListener();

  




  bool MayHavePaintEventListenerInSubDocument();

protected:
  void InvalidateThebesLayers();
  void AppUnitsPerDevPixelChanged();

  void HandleRebuildUserFontSet() {
    mPostedFlushUserFontSet = false;
    FlushUserFontSet();
  }

  bool HavePendingInputEvent();

  
  bool HasCachedStyleData();

  bool IsChromeSlow() const;

  
  
  

  nsPresContextType     mType;
  nsIPresShell*         mShell;         
  nsCOMPtr<nsIDocument> mDocument;
  nsRefPtr<nsDeviceContext> mDeviceContext; 
                                            
                                            
                                            
                                            
  nsRefPtr<nsEventStateManager> mEventManager;
  nsRefPtr<nsRefreshDriver> mRefreshDriver;
  nsRefPtr<nsTransitionManager> mTransitionManager;
  nsRefPtr<nsAnimationManager> mAnimationManager;
  nsIAtom*              mMedium;        
                                        

  nsILinkHandler*       mLinkHandler;   

  
  
  
  
  
  nsCOMPtr<nsIAtom>     mLanguage;

public:
  
  

  
  
  bool                  mInflationDisabledForShrinkWrap;

protected:

  nsWeakPtr             mContainer;

  PRCList               mDOMMediaQueryLists;

  int32_t               mMinFontSize;   
  float                 mTextZoom;      
  float                 mFullZoom;      

  float                 mLastFontInflationScreenWidth;

  int32_t               mCurAppUnitsPerDevPixel;
  int32_t               mAutoQualityMinFontSizePixelsPref;

  nsCOMPtr<nsITheme> mTheme;
  nsCOMPtr<nsILanguageAtomService> mLangService;
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  nsCOMPtr<nsITimer>    mPrefChangedTimer;

  FramePropertyTable    mPropertyTable;

  nsInvalidateRequestList mInvalidateRequestsSinceLastPaint;
  nsInvalidateRequestList mUndeliveredInvalidateRequestsBeforeLastPaint;

  
  nsUserFontSet*        mUserFontSet;

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

  ScrollbarStyles       mViewportStyleOverflow;
  uint8_t               mFocusRingWidth;

  bool mExistThrottledUpdates;

  uint16_t              mImageAnimationMode;
  uint16_t              mImageAnimationModePref;

  LangGroupFontPrefs    mLangGroupFontPrefs;

  nscoord               mBorderWidthTable[3];

  uint32_t              mInterruptChecksToSkip;

  mozilla::TimeStamp    mReflowStartTime;

  
  mozilla::TimeStamp    mLastUpdateThrottledStyle;
  
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
  unsigned              mEnableJapaneseTransform : 1;
  unsigned              mIsRootPaginatedDocument : 1;
  unsigned              mPrefBidiDirection : 1;
  unsigned              mPrefScrollbarSide : 2;
  unsigned              mPendingSysColorChanged : 1;
  unsigned              mPendingThemeChanged : 1;
  unsigned              mPendingUIResolutionChanged : 1;
  unsigned              mPendingMediaFeatureValuesChanged : 1;
  unsigned              mPrefChangePendingNeedsReflow : 1;
  
  
  unsigned              mAllInvalidated : 1;

  
  unsigned              mIsGlyph : 1;

  
  unsigned              mUsesRootEMUnits : 1;
  
  unsigned              mUsesViewportUnits : 1;

  
  unsigned              mPendingViewportChange : 1;

  
  unsigned              mUserFontSetDirty : 1;
  
  unsigned              mGetUserFontSetCalled : 1;
  
  unsigned              mPostedFlushUserFontSet : 1;

  
  
  unsigned              mSupressResizeReflow : 1;

  unsigned              mIsVisual : 1;

  unsigned              mProcessingRestyles : 1;
  unsigned              mProcessingAnimationStyleChange : 1;

  unsigned              mFireAfterPaintEvents : 1;

  
  
  
  mutable unsigned      mIsChromeIsCached : 1;
  mutable unsigned      mIsChrome : 1;

#ifdef DEBUG
  bool                  mInitialized;
#endif


protected:

  virtual ~nsPresContext() NS_HIDDEN;

  
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

class nsRootPresContext : public nsPresContext {
public:
  nsRootPresContext(nsIDocument* aDocument, nsPresContextType aType) NS_HIDDEN;
  virtual ~nsRootPresContext();

  



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

  virtual bool IsRoot() MOZ_OVERRIDE { return true; }

  




  void IncrementDOMGeneration() { mDOMGeneration++; }

  





  uint32_t GetDOMGeneration() { return mDOMGeneration; }

  




  void AddWillPaintObserver(nsIRunnable* aRunnable);

  


  void FlushWillPaintObservers();

  virtual size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE;

protected:
  


  void InitApplyPluginGeometryTimer();
  


  void CancelApplyPluginGeometryTimer();

  class RunWillPaintObservers : public nsRunnable {
  public:
    RunWillPaintObservers(nsRootPresContext* aPresContext) : mPresContext(aPresContext) {}
    void Revoke() { mPresContext = nullptr; }
    NS_IMETHOD Run()
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
