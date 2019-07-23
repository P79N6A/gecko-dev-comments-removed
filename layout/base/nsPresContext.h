







































#ifndef nsPresContext_h___
#define nsPresContext_h___

#include "nsISupports.h"
#include "nsColor.h"
#include "nsCoord.h"
#include "nsAString.h"
#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsRect.h"
#include "nsIDeviceContext.h"
#include "nsFont.h"
#include "nsIWeakReference.h"
#include "nsITheme.h"
#include "nsILanguageAtomService.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsCRT.h"
#include "nsIPrintSettings.h"
#include "nsPropertyTable.h"
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
#include "nsContentUtils.h"
#include "nsIWidget.h"

class nsImageLoader;
#ifdef IBMBIDI
class nsBidiPresUtils;
#endif 

struct nsRect;

class imgIRequest;

class nsIContent;
class nsIFontMetrics;
class nsIFrame;
class nsFrameManager;
class nsILinkHandler;
class nsStyleContext;
class nsIAtom;
class nsIEventStateManager;
class nsIURI;
class nsILookAndFeel;
class nsICSSPseudoComparator;
class nsIAtom;
struct nsStyleBackground;
struct nsStyleBorder;
class nsIRunnable;
class gfxUserFontSet;
class nsUserFontSet;
struct nsFontFaceRuleContainer;
class nsObjectFrame;

#ifdef MOZ_REFLOW_PERF
class nsIRenderingContext;
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


#define NS_AUTHOR_SPECIFIED_BACKGROUND      (1 << 0)
#define NS_AUTHOR_SPECIFIED_BORDER          (1 << 1)
#define NS_AUTHOR_SPECIFIED_PADDING         (1 << 2)

class nsRootPresContext;




class nsPresContext : public nsIObserver {
public:
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

  


  NS_HIDDEN_(nsresult) Init(nsIDeviceContext* aDeviceContext);

  



  NS_HIDDEN_(void) SetShell(nsIPresShell* aShell);


  NS_HIDDEN_(nsPresContextType) Type() const { return mType; }

  


  nsIPresShell* PresShell() const
  {
    NS_ASSERTION(mShell, "Null pres shell");
    return mShell;
  }

  nsIPresShell* GetPresShell() const { return mShell; }

  
  
  nsRootPresContext* RootPresContext();

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
#endif

  





  void RebuildAllStyleData(nsChangeHint aExtraHint);
  



  void PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint);

  void MediaFeatureValuesChanged(PRBool aCallerWillRebuildStyleData);
  void PostMediaFeatureValuesChangedEvent();
  NS_HIDDEN_(void) HandleMediaFeatureValuesChangedEvent();
  void FlushPendingMediaFeatureValuesChanged() {
    if (mPendingMediaFeatureValuesChanged)
      MediaFeatureValuesChanged(PR_FALSE);
  }

  



  nsCompatibility CompatibilityMode() const {
    return Document()->GetCompatibilityMode();
  }
  


  NS_HIDDEN_(void) CompatibilityModeChanged();

  


  PRUint16     ImageAnimationMode() const { return mImageAnimationMode; }
  void RestoreImageAnimationMode() { SetImageAnimationMode(mImageAnimationModePref); }
  virtual NS_HIDDEN_(void) SetImageAnimationModeExternal(PRUint16 aMode);
  NS_HIDDEN_(void) SetImageAnimationModeInternal(PRUint16 aMode);
#ifdef _IMPL_NS_LAYOUT
  void SetImageAnimationMode(PRUint16 aMode)
  { SetImageAnimationModeInternal(aMode); }
#else
  void SetImageAnimationMode(PRUint16 aMode)
  { SetImageAnimationModeExternal(aMode); }
#endif

  



  nsILookAndFeel* LookAndFeel() { return mLookAndFeel; }

  


  nsIAtom* Medium() { return mMedium; }

  void* AllocateFromShell(size_t aSize)
  {
    if (mShell)
      return mShell->AllocateFrame(aSize);
    return nsnull;
  }

  void FreeToShell(size_t aSize, void* aFreeChunk)
  {
    NS_ASSERTION(mShell, "freeing after shutdown");
    if (mShell)
      mShell->FreeFrame(aSize, aFreeChunk);
  }

  







  NS_HIDDEN_(already_AddRefed<nsIFontMetrics>)
  GetMetricsFor(const nsFont& aFont, PRBool aUseUserFontSet = PR_TRUE);

  
















  NS_HIDDEN_(const nsFont*) GetDefaultFont(PRUint8 aFontID) const;

  
  
  PRBool GetCachedBoolPref(nsPresContext_CachedBoolPrefType aPrefType) const
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

    return PR_FALSE;
  }

  
  
  PRInt32 GetCachedIntPref(nsPresContext_CachedIntPrefType aPrefType) const
  {
    
    
    switch (aPrefType) {
    case kPresContext_MinimumFontSize:
      return mMinimumFontSize;
    case kPresContext_ScrollbarSide:
      return mPrefScrollbarSide;
    case kPresContext_BidiDirection:
      return mPrefBidiDirection;
    default:
      NS_ERROR("invalid arg passed to GetCachedIntPref");
    }

    return PR_FALSE;
  }

  


  PRInt32 FontScaler() const { return mFontScaler; }

  


  const nscolor DefaultColor() const { return mDefaultColor; }
  const nscolor DefaultBackgroundColor() const { return mBackgroundColor; }
  const nscolor DefaultLinkColor() const { return mLinkColor; }
  const nscolor DefaultActiveLinkColor() const { return mActiveLinkColor; }
  const nscolor DefaultVisitedLinkColor() const { return mVisitedLinkColor; }
  const nscolor FocusBackgroundColor() const { return mFocusBackgroundColor; }
  const nscolor FocusTextColor() const { return mFocusTextColor; }

  PRBool GetUseFocusColors() const { return mUseFocusColors; }
  PRUint8 FocusRingWidth() const { return mFocusRingWidth; }
  PRBool GetFocusRingOnAnything() const { return mFocusRingOnAnything; }
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
    if (!r.IsExactEqual(mVisibleArea)) {
      mVisibleArea = r;
      
      if (!IsPaginated() && HasCachedStyleData())
        PostMediaFeatureValuesChangedEvent();
    }
  }

  



  PRBool IsPaginated() const { return mPaginated; }
  
  



  NS_HIDDEN_(void) SetPaginatedScrolling(PRBool aResult);

  



  PRBool HasPaginatedScrolling() const { return mCanPaginatedScroll; }

  


  nsSize GetPageSize() { return mPageSize; }
  void SetPageSize(nsSize aSize) { mPageSize = aSize; }

  




  PRBool IsRootPaginatedDocument() { return mIsRootPaginatedDocument; }
  void SetIsRootPaginatedDocument(PRBool aIsRootPaginatedDocument)
    { mIsRootPaginatedDocument = aIsRootPaginatedDocument; }

  





  float GetPageScale() { return mPageScale; }
  void SetPageScale(float aScale) { mPageScale = aScale; }

  







  float GetPrintPreviewScale() { return mPPScale; }
  void SetPrintPreviewScale(float aScale) { mPPScale = aScale; }

  nsIDeviceContext* DeviceContext() { return mDeviceContext; }
  nsIEventStateManager* EventStateManager() { return mEventManager; }
  nsIAtom* GetLangGroup() { return mLangGroup; }

  float TextZoom() { return mTextZoom; }
  void SetTextZoom(float aZoom) {
    if (aZoom == mTextZoom)
      return;

    mTextZoom = aZoom;
    if (HasCachedStyleData()) {
      
      
      MediaFeatureValuesChanged(PR_TRUE);
      RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
    }
  }

  float GetFullZoom() { return mFullZoom; }
  void SetFullZoom(float aZoom);

  nscoord GetAutoQualityMinFontSize() {
    return DevPixelsToAppUnits(mAutoQualityMinFontSizePixelsPref);
  }
  
  static PRInt32 AppUnitsPerCSSPixel() { return nsIDeviceContext::AppUnitsPerCSSPixel(); }
  PRInt32 AppUnitsPerDevPixel() const  { return mDeviceContext->AppUnitsPerDevPixel(); }
  PRInt32 AppUnitsPerInch() const      { return mDeviceContext->AppUnitsPerInch(); }

  static nscoord CSSPixelsToAppUnits(PRInt32 aPixels)
  { return NSIntPixelsToAppUnits(aPixels,
                                 nsIDeviceContext::AppUnitsPerCSSPixel()); }

  static nscoord CSSPixelsToAppUnits(float aPixels)
  { return NSFloatPixelsToAppUnits(aPixels,
             float(nsIDeviceContext::AppUnitsPerCSSPixel())); }

  static PRInt32 AppUnitsToIntCSSPixels(nscoord aAppUnits)
  { return NSAppUnitsToIntPixels(aAppUnits,
             float(nsIDeviceContext::AppUnitsPerCSSPixel())); }

  static float AppUnitsToFloatCSSPixels(nscoord aAppUnits)
  { return NSAppUnitsToFloatPixels(aAppUnits,
             float(nsIDeviceContext::AppUnitsPerCSSPixel())); }

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

  nscoord TwipsToAppUnits(PRInt32 aTwips) const
  { return NSToCoordRound(NS_TWIPS_TO_INCHES(aTwips) *
                          mDeviceContext->AppUnitsPerInch()); }

  
  nsMargin TwipsToAppUnits(const nsIntMargin &marginInTwips) const
  { return nsMargin(TwipsToAppUnits(marginInTwips.left), 
                    TwipsToAppUnits(marginInTwips.top),
                    TwipsToAppUnits(marginInTwips.right),
                    TwipsToAppUnits(marginInTwips.bottom)); }

  nscoord PointsToAppUnits(float aPoints) const
  { return NSToCoordRound(aPoints * mDeviceContext->AppUnitsPerInch() /
                          POINTS_PER_INCH_FLOAT); }

  nscoord RoundAppUnitsToNearestDevPixels(nscoord aAppUnits) const
  { return DevPixelsToAppUnits(AppUnitsToDevPixels(aAppUnits)); }

  struct ScrollbarStyles {
    
    
    PRUint8 mHorizontal, mVertical;
    ScrollbarStyles(PRUint8 h, PRUint8 v) : mHorizontal(h), mVertical(v) {}
    ScrollbarStyles() {}
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

  


  PRBool GetBackgroundImageDraw() const { return mDrawImageBackground; }
  void   SetBackgroundImageDraw(PRBool aCanDraw)
  {
    NS_ASSERTION(!(aCanDraw & ~1), "Value must be true or false");
    mDrawImageBackground = aCanDraw;
  }

  PRBool GetBackgroundColorDraw() const { return mDrawColorBackground; }
  void   SetBackgroundColorDraw(PRBool aCanDraw)
  {
    NS_ASSERTION(!(aCanDraw & ~1), "Value must be true or false");
    mDrawColorBackground = aCanDraw;
  }

#ifdef IBMBIDI
  






  virtual NS_HIDDEN_(PRBool) BidiEnabledExternal() const;
  NS_HIDDEN_(PRBool) BidiEnabledInternal() const;
#ifdef _IMPL_NS_LAYOUT
  PRBool BidiEnabled() const { return BidiEnabledInternal(); }
#else
  PRBool BidiEnabled() const { return BidiEnabledExternal(); }
#endif

  




  NS_HIDDEN_(void) SetBidiEnabled() const;

  













  void SetVisualMode(PRBool aIsVisual)
  {
    NS_ASSERTION(!(aIsVisual & ~1), "Value must be true or false");
    mIsVisual = aIsVisual;
  }

  




  PRBool IsVisualMode() const { return mIsVisual; }



  


  NS_HIDDEN_(nsBidiPresUtils*) GetBidiUtils();

  

  
  NS_HIDDEN_(void) SetBidi(PRUint32 aBidiOptions,
                           PRBool aForceRestyle = PR_FALSE);

  



  
  NS_HIDDEN_(PRUint32) GetBidi() const;
#endif 

  


  void SetIsRenderingOnlySelection(PRBool aResult)
  {
    NS_ASSERTION(!(aResult & ~1), "Value must be true or false");
    mIsRenderingOnlySelection = aResult;
  }

  PRBool IsRenderingOnlySelection() const { return mIsRenderingOnlySelection; }

  


  NS_HIDDEN_(nsITheme*) GetTheme();

  





  NS_HIDDEN_(void) ThemeChanged();

  


  NS_HIDDEN_(void) SysColorChanged();

  
  NS_HIDDEN_(void) SetPrintSettings(nsIPrintSettings *aPrintSettings);

  nsIPrintSettings* GetPrintSettings() { return mPrintSettings; }

  
  nsPropertyTable* PropertyTable() { return &mPropertyTable; }

  



  NS_HIDDEN_(PRBool) EnsureVisible();
  
#ifdef MOZ_REFLOW_PERF
  NS_HIDDEN_(void) CountReflows(const char * aName,
                                nsIFrame * aFrame);
#endif

  



  const nscoord* GetBorderWidthTable() { return mBorderWidthTable; }

  PRBool IsDynamic() { return (mType == eContext_PageLayout || mType == eContext_Galley); }
  PRBool IsScreen() { return (mMedium == nsGkAtoms::screen ||
                              mType == eContext_PageLayout ||
                              mType == eContext_PrintPreview); }

  
  PRBool IsChrome() const;

  
  virtual PRBool HasAuthorSpecifiedRules(nsIFrame *aFrame, PRUint32 ruleTypeMask) const;

  
  PRBool UseDocumentColors() const {
    return GetCachedBoolPref(kPresContext_UseDocumentColors) || IsChrome();
  }

  PRBool           SupressingResizeReflow() const { return mSupressResizeReflow; }
  
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

  void NotifyInvalidation(const nsRect& aRect, PRBool aIsCrossDoc);
  void FireDOMPaintEvent();
  PRBool IsDOMPaintEventPending() {
    return !mSameDocDirtyRegion.IsEmpty() || !mCrossDocDirtyRegion.IsEmpty();
  }

  void ClearMozAfterPaintEvents() {
    mSameDocDirtyRegion.SetEmpty();
    mCrossDocDirtyRegion.SetEmpty();
  }

  








  void ReflowStarted(PRBool aInterruptible);

  


  class InterruptPreventer;
  friend class InterruptPreventer;
  class NS_STACK_CLASS InterruptPreventer {
  public:
    InterruptPreventer(nsPresContext* aCtx) :
      mCtx(aCtx),
      mInterruptsEnabled(aCtx->mInterruptsEnabled),
      mHasPendingInterrupt(aCtx->mHasPendingInterrupt)
    {
      mCtx->mInterruptsEnabled = PR_FALSE;
      mCtx->mHasPendingInterrupt = PR_FALSE;
    }
    ~InterruptPreventer() {
      mCtx->mInterruptsEnabled = mInterruptsEnabled;
      mCtx->mHasPendingInterrupt = mHasPendingInterrupt;
    }

  private:
    nsPresContext* mCtx;
    PRBool mInterruptsEnabled;
    PRBool mHasPendingInterrupt;
  };
    
  







  PRBool CheckForInterrupt(nsIFrame* aFrame);
  



  PRBool HasPendingInterrupt() { return mHasPendingInterrupt; }

protected:
  friend class nsRunnableMethod<nsPresContext>;
  NS_HIDDEN_(void) ThemeChangedInternal();
  NS_HIDDEN_(void) SysColorChangedInternal();

  NS_HIDDEN_(void) SetImgAnimations(nsIContent *aParent, PRUint16 aMode);
#ifdef MOZ_SMIL
  NS_HIDDEN_(void) SetSMILAnimations(nsIDocument *aDoc, PRUint16 aNewMode,
                                     PRUint16 aOldMode);
#endif 
  NS_HIDDEN_(void) GetDocumentColorPreferences();

  NS_HIDDEN_(void) PreferenceChanged(const char* aPrefName);
  static NS_HIDDEN_(int) PrefChangedCallback(const char*, void*);

  NS_HIDDEN_(void) UpdateAfterPreferencesChanged();
  static NS_HIDDEN_(void) PrefChangedUpdateTimerCallback(nsITimer *aTimer, void *aClosure);

  NS_HIDDEN_(void) GetUserPreferences();
  NS_HIDDEN_(void) GetFontPreferences();

  NS_HIDDEN_(void) UpdateCharSet(const nsAFlatCString& aCharSet);

  void HandleRebuildUserFontSet() {
    mPostedFlushUserFontSet = PR_FALSE;
    FlushUserFontSet();
  }

  PRBool HavePendingInputEvent();

  
  PRBool HasCachedStyleData();

  
  
  
  
  nsPresContextType     mType;
  nsIPresShell*         mShell;         
  nsCOMPtr<nsIDocument> mDocument;
  nsIDeviceContext*     mDeviceContext; 
                                        
                                        
                                        
                                        
  nsIEventStateManager* mEventManager;  
  nsILookAndFeel*       mLookAndFeel;   
  nsIAtom*              mMedium;        
                                        

  nsILinkHandler*       mLinkHandler;   
  nsIAtom*              mLangGroup;     

  nsRefPtrHashtable<nsVoidPtrHashKey, nsImageLoader>
                        mImageLoaders[IMAGE_LOAD_TYPE_COUNT];

  nsWeakPtr             mContainer;

  float                 mTextZoom;      
  float                 mFullZoom;      

  PRInt32               mCurAppUnitsPerDevPixel;
  PRInt32               mAutoQualityMinFontSizePixelsPref;

#ifdef IBMBIDI
  nsBidiPresUtils*      mBidiUtils;
#endif

  nsCOMPtr<nsITheme> mTheme;
  nsCOMPtr<nsILanguageAtomService> mLangService;
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  nsCOMPtr<nsITimer>    mPrefChangedTimer;

  nsPropertyTable       mPropertyTable;

  nsRegion              mSameDocDirtyRegion;
  nsRegion              mCrossDocDirtyRegion;

  
  nsUserFontSet*        mUserFontSet;
  
  nsTArray<nsFontFaceRuleContainer> mFontFaceRules;
  
  PRInt32               mFontScaler;
  nscoord               mMinimumFontSize;

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

  unsigned              mHasPendingInterrupt : 1;
  unsigned              mInterruptsEnabled : 1;
  unsigned              mUseDocumentFonts : 1;
  unsigned              mUseDocumentColors : 1;
  unsigned              mUnderlineLinks : 1;
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

  
  unsigned              mUserFontSetDirty : 1;
  
  unsigned              mGetUserFontSetCalled : 1;
  
  unsigned              mPostedFlushUserFontSet : 1;

  
  
  unsigned              mSupressResizeReflow : 1;

#ifdef IBMBIDI
  unsigned              mIsVisual : 1;

#endif
#ifdef DEBUG
  PRBool                mInitialized;
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

  





  void RegisterPluginForGeometryUpdates(nsObjectFrame* aPlugin);
  




  void UnregisterPluginForGeometryUpdates(nsObjectFrame* aPlugin);

  





  void UpdatePluginGeometry(nsIFrame* aChangedRoot);

  






  void GetPluginGeometryUpdates(nsIFrame* aChangedRoot,
                                nsTArray<nsIWidget::Configuration>* aConfigurations);

  




  void DidApplyPluginGeometryUpdates();

private:
  nsTHashtable<nsPtrHashKey<nsObjectFrame> > mRegisteredPlugins;
};

#ifdef DEBUG

struct nsAutoLayoutPhase {
  nsAutoLayoutPhase(nsPresContext* aPresContext, nsLayoutPhase aPhase)
    : mPresContext(aPresContext), mPhase(aPhase), mCount(0)
  {
    Enter();
  }

  ~nsAutoLayoutPhase()
  {
    Exit();
    NS_ASSERTION(mCount == 0, "imbalanced");
  }

  void Enter()
  {
    switch (mPhase) {
      case eLayoutPhase_Paint:
        NS_ASSERTION(mPresContext->mLayoutPhaseCount[eLayoutPhase_Paint] == 0,
                     "recurring into paint");
        NS_ASSERTION(mPresContext->mLayoutPhaseCount[eLayoutPhase_Reflow] == 0,
                     "painting in the middle of reflow");
        NS_ASSERTION(mPresContext->mLayoutPhaseCount[eLayoutPhase_FrameC] == 0,
                     "painting in the middle of frame construction");
        break;
      case eLayoutPhase_Reflow:
        NS_ASSERTION(mPresContext->mLayoutPhaseCount[eLayoutPhase_Paint] == 0,
                     "reflowing in the middle of a paint");
        NS_ASSERTION(mPresContext->mLayoutPhaseCount[eLayoutPhase_Reflow] == 0,
                     "recurring into reflow");
        NS_ASSERTION(mPresContext->mLayoutPhaseCount[eLayoutPhase_FrameC] == 0,
                     "reflowing in the middle of frame construction");
        break;
      case eLayoutPhase_FrameC:
        NS_ASSERTION(mPresContext->mLayoutPhaseCount[eLayoutPhase_Paint] == 0,
                     "constructing frames in the middle of a paint");
        NS_ASSERTION(mPresContext->mLayoutPhaseCount[eLayoutPhase_Reflow] == 0,
                     "constructing frames in the middle of reflow");
        
        NS_WARN_IF_FALSE(mPresContext->mLayoutPhaseCount[eLayoutPhase_FrameC] == 0,
                         "recurring into frame construction");
        NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
                     "constructing frames and scripts are not blocked");
        break;
      default:
        break;
    }
    ++(mPresContext->mLayoutPhaseCount[mPhase]);
    ++mCount;
  }

  void Exit()
  {
    NS_ASSERTION(mCount > 0 && mPresContext->mLayoutPhaseCount[mPhase] > 0,
                 "imbalanced");
    --(mPresContext->mLayoutPhaseCount[mPhase]);
    --mCount;
  }

private:
  nsPresContext *mPresContext;
  nsLayoutPhase mPhase;
  PRUint32 mCount;
};

#define AUTO_LAYOUT_PHASE_ENTRY_POINT(pc_, phase_) \
  nsAutoLayoutPhase autoLayoutPhase((pc_), (eLayoutPhase_##phase_))
#define LAYOUT_PHASE_TEMP_EXIT() \
  PR_BEGIN_MACRO \
    autoLayoutPhase.Exit(); \
  PR_END_MACRO
#define LAYOUT_PHASE_TEMP_REENTER() \
  PR_BEGIN_MACRO \
    autoLayoutPhase.Enter(); \
  PR_END_MACRO

#else

#define AUTO_LAYOUT_PHASE_ENTRY_POINT(pc_, phase_) \
  PR_BEGIN_MACRO PR_END_MACRO
#define LAYOUT_PHASE_TEMP_EXIT() \
  PR_BEGIN_MACRO PR_END_MACRO
#define LAYOUT_PHASE_TEMP_REENTER() \
  PR_BEGIN_MACRO PR_END_MACRO

#endif

#ifdef MOZ_REFLOW_PERF

#define DO_GLOBAL_REFLOW_COUNT(_name) \
  aPresContext->CountReflows((_name), (nsIFrame*)this); 
#else
#define DO_GLOBAL_REFLOW_COUNT(_name)
#endif 

#endif
