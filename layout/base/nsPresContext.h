







































#ifndef nsPresContext_h___
#define nsPresContext_h___

#include "nsISupports.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "nsAString.h"
#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsRect.h"
#include "nsDeviceContext.h"
#include "nsFont.h"
#include "nsIWeakReference.h"
#include "nsITheme.h"
#include "nsILanguageAtomService.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsCRT.h"
#include "nsIPrintSettings.h"
#include "FramePropertyTable.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "nsRefPtrHashtable.h"
#include "nsCycleCollectionParticipant.h"
#include "nsChangeHint.h"

#include "gfxRect.h"
#include "nsRegion.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsIWidget.h"
#include "mozilla/TimeStamp.h"
#include "nsIContent.h"
#include "prclist.h"

class nsImageLoader;
#ifdef IBMBIDI
class nsBidiPresUtils;
#endif 

struct nsRect;

class imgIRequest;

class nsFontMetrics;
class nsIFrame;
class nsFrameManager;
class nsILinkHandler;
class nsStyleContext;
class nsIAtom;
class nsEventStateManager;
class nsIURI;
class nsICSSPseudoComparator;
class nsIAtom;
struct nsStyleBackground;
struct nsStyleBorder;
class nsIRunnable;
class gfxUserFontSet;
class nsUserFontSet;
struct nsFontFaceRuleContainer;
class nsObjectFrame;
class nsTransitionManager;
class nsAnimationManager;
class nsRefreshDriver;
class imgIContainer;
class nsIDOMMediaQueryList;

#ifdef MOZ_REFLOW_PERF
class nsRenderingContext;
#endif

enum nsWidgetType {
  eWidgetType_Button  	= 1,
  eWidgetType_Checkbox	= 2,
  eWidgetType_Radio			= 3,
  eWidgetType_Text			= 4
};

enum nsLanguageSpecificTransformType {
  eLanguageSpecificTransformType_Unknown = -1,
  eLanguageSpecificTransformType_None = 0,
  eLanguageSpecificTransformType_Japanese
};


enum nsPresContext_CachedBoolPrefType {
  kPresContext_UseDocumentColors = 1,
  kPresContext_UseDocumentFonts,
  kPresContext_UnderlineLinks
};


enum nsPresContext_CachedIntPrefType {
  kPresContext_MinimumFontSize = 1,
  kPresContext_ScrollbarSide,
  kPresContext_BidiDirection
};



const PRUint8 kPresContext_DefaultVariableFont_ID = 0x00; 
const PRUint8 kPresContext_DefaultFixedFont_ID    = 0x01; 

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
    PRUint32 mFlags;
  };

  nsTArray<Request> mRequests;
};


#define NS_AUTHOR_SPECIFIED_BACKGROUND      (1 << 0)
#define NS_AUTHOR_SPECIFIED_BORDER          (1 << 1)
#define NS_AUTHOR_SPECIFIED_PADDING         (1 << 2)

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

  




  nsRootPresContext* GetRootPresContext();
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

  void MediaFeatureValuesChanged(bool aCallerWillRebuildStyleData);
  void PostMediaFeatureValuesChangedEvent();
  NS_HIDDEN_(void) HandleMediaFeatureValuesChangedEvent();
  void FlushPendingMediaFeatureValuesChanged() {
    if (mPendingMediaFeatureValuesChanged)
      MediaFeatureValuesChanged(false);
  }

  


  void MatchMedia(const nsAString& aMediaQueryList,
                  nsIDOMMediaQueryList** aResult);

  



  nsCompatibility CompatibilityMode() const {
    return Document()->GetCompatibilityMode();
  }
  


  NS_HIDDEN_(void) CompatibilityModeChanged();

  


  PRUint16     ImageAnimationMode() const { return mImageAnimationMode; }
  virtual NS_HIDDEN_(void) SetImageAnimationModeExternal(PRUint16 aMode);
  NS_HIDDEN_(void) SetImageAnimationModeInternal(PRUint16 aMode);
#ifdef _IMPL_NS_LAYOUT
  void SetImageAnimationMode(PRUint16 aMode)
  { SetImageAnimationModeInternal(aMode); }
#else
  void SetImageAnimationMode(PRUint16 aMode)
  { SetImageAnimationModeExternal(aMode); }
#endif

  


  nsIAtom* Medium() { return mMedium; }

  void* AllocateFromShell(size_t aSize)
  {
    if (mShell)
      return mShell->AllocateMisc(aSize);
    return nsnull;
  }

  void FreeToShell(size_t aSize, void* aFreeChunk)
  {
    NS_ASSERTION(mShell, "freeing after shutdown");
    if (mShell)
      mShell->FreeMisc(aSize, aFreeChunk);
  }

  
















  NS_HIDDEN_(const nsFont*) GetDefaultFont(PRUint8 aFontID) const;

  
  
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

  
  
  PRInt32 GetCachedIntPref(nsPresContext_CachedIntPrefType aPrefType) const
  {
    
    
    switch (aPrefType) {
    case kPresContext_MinimumFontSize:
      return mMinimumFontSizePref;
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
  PRUint8 FocusRingWidth() const { return mFocusRingWidth; }
  bool GetFocusRingOnAnything() const { return mFocusRingOnAnything; }
  PRUint8 GetFocusRingStyle() const { return mFocusRingStyle; }

  



  enum ImageLoadType {
    BACKGROUND_IMAGE,
    BORDER_IMAGE,
    IMAGE_LOAD_TYPE_COUNT
  };

  





  NS_HIDDEN_(void) SetImageLoaders(nsIFrame* aTargetFrame,
                                   ImageLoadType aType,
                                   nsImageLoader* aImageLoaders);

  




  NS_HIDDEN_(void) SetupBackgroundImageLoaders(nsIFrame* aFrame,
                                               const nsStyleBackground*
                                                 aStyleBackground);

  




  NS_HIDDEN_(void) SetupBorderImageLoaders(nsIFrame* aFrame,
                                           const nsStyleBorder* aStyleBorder);

  



  NS_HIDDEN_(void) StopImagesFor(nsIFrame* aTargetFrame);

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
      
      if (!IsPaginated() && HasCachedStyleData())
        PostMediaFeatureValuesChangedEvent();
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
      
      
      MediaFeatureValuesChanged(true);
      RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
    }
  }

  PRInt32 MinFontSize() const {
    return NS_MAX(mMinFontSize, mMinimumFontSizePref);
  }

  void SetMinFontSize(PRInt32 aMinFontSize) {
    if (aMinFontSize == mMinFontSize)
      return;

    mMinFontSize = aMinFontSize;
    if (HasCachedStyleData()) {
      
      
      MediaFeatureValuesChanged(true);
      RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
    }
  }

  float GetFullZoom() { return mFullZoom; }
  void SetFullZoom(float aZoom);

  nscoord GetAutoQualityMinFontSize() {
    return DevPixelsToAppUnits(mAutoQualityMinFontSizePixelsPref);
  }
  
  static PRInt32 AppUnitsPerCSSPixel() { return nsDeviceContext::AppUnitsPerCSSPixel(); }
  PRUint32 AppUnitsPerDevPixel() const  { return mDeviceContext->AppUnitsPerDevPixel(); }
  static PRInt32 AppUnitsPerCSSInch() { return nsDeviceContext::AppUnitsPerCSSInch(); }

  static nscoord CSSPixelsToAppUnits(PRInt32 aPixels)
  { return NSIntPixelsToAppUnits(aPixels,
                                 nsDeviceContext::AppUnitsPerCSSPixel()); }

  static nscoord CSSPixelsToAppUnits(float aPixels)
  { return NSFloatPixelsToAppUnits(aPixels,
             float(nsDeviceContext::AppUnitsPerCSSPixel())); }

  static PRInt32 AppUnitsToIntCSSPixels(nscoord aAppUnits)
  { return NSAppUnitsToIntPixels(aAppUnits,
             float(nsDeviceContext::AppUnitsPerCSSPixel())); }

  static float AppUnitsToFloatCSSPixels(nscoord aAppUnits)
  { return NSAppUnitsToFloatPixels(aAppUnits,
             float(nsDeviceContext::AppUnitsPerCSSPixel())); }

  nscoord DevPixelsToAppUnits(PRInt32 aPixels) const
  { return NSIntPixelsToAppUnits(aPixels,
                                 mDeviceContext->AppUnitsPerDevPixel()); }

  PRInt32 AppUnitsToDevPixels(nscoord aAppUnits) const
  { return NSAppUnitsToIntPixels(aAppUnits,
             float(mDeviceContext->AppUnitsPerDevPixel())); }

  PRInt32 CSSPixelsToDevPixels(PRInt32 aPixels)
  { return AppUnitsToDevPixels(CSSPixelsToAppUnits(aPixels)); }

  float CSSPixelsToDevPixels(float aPixels)
  {
    return NSAppUnitsToFloatPixels(CSSPixelsToAppUnits(aPixels),
                                   float(mDeviceContext->AppUnitsPerDevPixel()));
  }

  PRInt32 DevPixelsToIntCSSPixels(PRInt32 aPixels)
  { return AppUnitsToIntCSSPixels(DevPixelsToAppUnits(aPixels)); }

  float DevPixelsToFloatCSSPixels(PRInt32 aPixels)
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
    
    
    PRUint8 mHorizontal, mVertical;
    ScrollbarStyles(PRUint8 h, PRUint8 v) : mHorizontal(h), mVertical(v) {}
    ScrollbarStyles() {}
    bool operator==(const ScrollbarStyles& aStyles) const {
      return aStyles.mHorizontal == mHorizontal && aStyles.mVertical == mVertical;
    }
    bool operator!=(const ScrollbarStyles& aStyles) const {
      return aStyles.mHorizontal != mHorizontal || aStyles.mVertical != mVertical;
    }
  };
  void SetViewportOverflowOverride(PRUint8 aX, PRUint8 aY)
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

#ifdef IBMBIDI
  






  virtual bool BidiEnabledExternal() const { return BidiEnabledInternal(); }
  bool BidiEnabledInternal() const { return Document()->GetBidiEnabled(); }
#ifdef _IMPL_NS_LAYOUT
  bool BidiEnabled() const { return BidiEnabledInternal(); }
#else
  bool BidiEnabled() const { return BidiEnabledExternal(); }
#endif

  




  NS_HIDDEN_(void) SetBidiEnabled() const;

  













  void SetVisualMode(bool aIsVisual)
  {
    mIsVisual = aIsVisual;
  }

  




  bool IsVisualMode() const { return mIsVisual; }



  

  
  NS_HIDDEN_(void) SetBidi(PRUint32 aBidiOptions,
                           bool aForceRestyle = false);

  



  
  NS_HIDDEN_(PRUint32) GetBidi() const;
#endif 

  


  void SetIsRenderingOnlySelection(bool aResult)
  {
    mIsRenderingOnlySelection = aResult;
  }

  bool IsRenderingOnlySelection() const { return mIsRenderingOnlySelection; }

  NS_HIDDEN_(bool) IsTopLevelWindowInactive();

  


  NS_HIDDEN_(nsITheme*) GetTheme();

  





  NS_HIDDEN_(void) ThemeChanged();

  


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

  
  virtual bool HasAuthorSpecifiedRules(nsIFrame *aFrame, PRUint32 ruleTypeMask) const;

  
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

  void NotifyInvalidation(const nsRect& aRect, PRUint32 aFlags);
  void NotifyDidPaintForSubtree();
  void FireDOMPaintEvent();

  bool IsDOMPaintEventPending() {
    return !mInvalidateRequests.mRequests.IsEmpty();
  }
  void ClearMozAfterPaintEvents() {
    mInvalidateRequests.mRequests.Clear();
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

  





  nsIFrame* GetPrimaryFrameFor(nsIContent* aContent) {
    NS_PRECONDITION(aContent, "Don't do that");
    if (GetPresShell() &&
        GetPresShell()->GetDocument() == aContent->GetCurrentDoc()) {
      return aContent->GetPrimaryFrame();
    }
    return nsnull;
  }

  void NotifyDestroyingFrame(nsIFrame* aFrame)
  {
    PropertyTable()->DeleteAllFor(aFrame);
  }
  inline void ForgetUpdatePluginGeometryFrame(nsIFrame* aFrame);

  void DestroyImageLoaders();

  bool GetContainsUpdatePluginGeometryFrame()
  {
    return mContainsUpdatePluginGeometryFrame;
  }

  void SetContainsUpdatePluginGeometryFrame(bool aValue)
  {
    mContainsUpdatePluginGeometryFrame = aValue;
  }

  bool MayHaveFixedBackgroundFrames() { return mMayHaveFixedBackgroundFrames; }
  void SetHasFixedBackgroundFrame() { mMayHaveFixedBackgroundFrames = true; }

  virtual NS_MUST_OVERRIDE size_t
        SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
    
    
    return 0;
  }
  virtual NS_MUST_OVERRIDE size_t
        SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
    return aMallocSizeOf(this, sizeof(nsPresContext)) +
           SizeOfExcludingThis(aMallocSizeOf);
  }

  bool IsRootContentDocument();

protected:
  friend class nsRunnableMethod<nsPresContext>;
  NS_HIDDEN_(void) ThemeChangedInternal();
  NS_HIDDEN_(void) SysColorChangedInternal();

  NS_HIDDEN_(void) SetImgAnimations(nsIContent *aParent, PRUint16 aMode);
  NS_HIDDEN_(void) SetSMILAnimations(nsIDocument *aDoc, PRUint16 aNewMode,
                                     PRUint16 aOldMode);
  NS_HIDDEN_(void) GetDocumentColorPreferences();

  NS_HIDDEN_(void) PreferenceChanged(const char* aPrefName);
  static NS_HIDDEN_(int) PrefChangedCallback(const char*, void*);

  NS_HIDDEN_(void) UpdateAfterPreferencesChanged();
  static NS_HIDDEN_(void) PrefChangedUpdateTimerCallback(nsITimer *aTimer, void *aClosure);

  NS_HIDDEN_(void) GetUserPreferences();
  NS_HIDDEN_(void) GetFontPreferences();

  NS_HIDDEN_(void) UpdateCharSet(const nsAFlatCString& aCharSet);

  void InvalidateThebesLayers();
  void AppUnitsPerDevPixelChanged();

  bool MayHavePaintEventListener();

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
  nsDeviceContext*     mDeviceContext; 
                                        
                                        
                                        
                                        
  nsEventStateManager* mEventManager;   
  nsRefPtr<nsRefreshDriver> mRefreshDriver;
  nsRefPtr<nsTransitionManager> mTransitionManager;
  nsRefPtr<nsAnimationManager> mAnimationManager;
  nsIAtom*              mMedium;        
                                        

  nsILinkHandler*       mLinkHandler;   

  
  
  
  
  
  nsIAtom*              mLanguage;      

  nsRefPtrHashtable<nsVoidPtrHashKey, nsImageLoader>
                        mImageLoaders[IMAGE_LOAD_TYPE_COUNT];

  nsWeakPtr             mContainer;

  PRCList               mDOMMediaQueryLists;

  PRInt32               mMinFontSize;   
  float                 mTextZoom;      
  float                 mFullZoom;      

  PRInt32               mCurAppUnitsPerDevPixel;
  PRInt32               mAutoQualityMinFontSizePixelsPref;

  nsCOMPtr<nsITheme> mTheme;
  nsCOMPtr<nsILanguageAtomService> mLangService;
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  nsCOMPtr<nsITimer>    mPrefChangedTimer;

  FramePropertyTable    mPropertyTable;

  nsInvalidateRequestList mInvalidateRequests;

  
  nsUserFontSet*        mUserFontSet;
  
  PRInt32               mFontScaler;
  nscoord               mMinimumFontSizePref;

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
  PRUint8               mFocusRingWidth;

  PRUint16              mImageAnimationMode;
  PRUint16              mImageAnimationModePref;

  nsFont                mDefaultVariableFont;
  nsFont                mDefaultFixedFont;
  nsFont                mDefaultSerifFont;
  nsFont                mDefaultSansSerifFont;
  nsFont                mDefaultMonospaceFont;
  nsFont                mDefaultCursiveFont;
  nsFont                mDefaultFantasyFont;

  nscoord               mBorderWidthTable[3];

  PRUint32              mInterruptChecksToSkip;

  mozilla::TimeStamp    mReflowStartTime;

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
  unsigned              mPendingMediaFeatureValuesChanged : 1;
  unsigned              mPrefChangePendingNeedsReflow : 1;
  unsigned              mMayHaveFixedBackgroundFrames : 1;

  
  unsigned              mUserFontSetDirty : 1;
  
  unsigned              mGetUserFontSetCalled : 1;
  
  unsigned              mPostedFlushUserFontSet : 1;

  
  
  unsigned              mSupressResizeReflow : 1;

  unsigned              mIsVisual : 1;

  unsigned              mProcessingRestyles : 1;
  unsigned              mProcessingAnimationStyleChange : 1;

  unsigned              mContainsUpdatePluginGeometryFrame : 1;
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

#ifdef DEBUG
private:
  friend struct nsAutoLayoutPhase;
  PRUint32 mLayoutPhaseCount[eLayoutPhase_COUNT];
public:
  PRUint32 LayoutPhaseCount(nsLayoutPhase aPhase) {
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
      mNotifyDidPaintTimer = nsnull;
    }
  }

  





  void RegisterPluginForGeometryUpdates(nsObjectFrame* aPlugin);
  




  void UnregisterPluginForGeometryUpdates(nsObjectFrame* aPlugin);

  




  void UpdatePluginGeometry();

  






  void GetPluginGeometryUpdates(nsIFrame* aChangedRoot,
                                nsTArray<nsIWidget::Configuration>* aConfigurations);

  




  void DidApplyPluginGeometryUpdates();

  virtual bool IsRoot() { return true; }

  




  void RequestUpdatePluginGeometry(nsIFrame* aFrame);

  



  void RootForgetUpdatePluginGeometryFrame(nsIFrame* aFrame);

  





  void RootForgetUpdatePluginGeometryFrameForPresContext(nsPresContext* aPresContext);

  




  void IncrementDOMGeneration() { mDOMGeneration++; }

  


  PRUint32 GetDOMGeneration() { return mDOMGeneration; }

  




  void AddWillPaintObserver(nsIRunnable* aRunnable);

  


  void FlushWillPaintObservers();

  virtual NS_MUST_OVERRIDE size_t
        SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE {
    
    
    return nsPresContext::SizeOfExcludingThis(aMallocSizeOf);
  }
  virtual NS_MUST_OVERRIDE size_t
        SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const MOZ_OVERRIDE {
    return aMallocSizeOf(this, sizeof(nsRootPresContext)) +
           SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  class RunWillPaintObservers : public nsRunnable {
  public:
    RunWillPaintObservers(nsRootPresContext* aPresContext) : mPresContext(aPresContext) {}
    void Revoke() { mPresContext = nsnull; }
    NS_IMETHOD Run()
    {
      if (mPresContext) {
        mPresContext->FlushWillPaintObservers();
      }
      return NS_OK;
    }
    nsRootPresContext* mPresContext;
  };

  nsCOMPtr<nsITimer> mNotifyDidPaintTimer;
  nsTHashtable<nsPtrHashKey<nsObjectFrame> > mRegisteredPlugins;
  
  
  
  nsTArray<nsCOMPtr<nsIRunnable> > mWillPaintObservers;
  nsRevocableEventPtr<RunWillPaintObservers> mWillPaintFallbackEvent;
  nsIFrame* mUpdatePluginGeometryForFrame;
  PRUint32 mDOMGeneration;
  bool mNeedsToUpdatePluginGeometry;
};

inline void
nsPresContext::ForgetUpdatePluginGeometryFrame(nsIFrame* aFrame)
{
  if (mContainsUpdatePluginGeometryFrame) {
    nsRootPresContext* rootPC = GetRootPresContext();
    if (rootPC) {
      rootPC->RootForgetUpdatePluginGeometryFrame(aFrame);
    }
  }
}

#ifdef MOZ_REFLOW_PERF

#define DO_GLOBAL_REFLOW_COUNT(_name) \
  aPresContext->CountReflows((_name), (nsIFrame*)this); 
#else
#define DO_GLOBAL_REFLOW_COUNT(_name)
#endif 

#endif
