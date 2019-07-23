






































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
#include "nsInterfaceHashtable.h"
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
class nsIImage;
class nsILinkHandler;
class nsStyleContext;
class nsIAtom;
class nsIEventStateManager;
class nsIURI;
class nsILookAndFeel;
class nsICSSPseudoComparator;
class nsIAtom;
struct nsStyleStruct;
struct nsStyleBackground;
template <class T> class nsRunnableMethod;
class nsIRunnable;

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




class nsPresContext : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

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

  
  
  nsPresContext* RootPresContext();

  nsIDocument* Document() const
  {
      NS_ASSERTION(!mShell || !mShell->GetDocument() ||
                   mShell->GetDocument() == mDocument,
                   "nsPresContext doesn't have the same document as nsPresShell!");
      return mDocument;
  }

  nsIViewManager* GetViewManager() { return GetPresShell()->GetViewManager(); } 
#ifdef _IMPL_NS_LAYOUT
  nsStyleSet* StyleSet() { return GetPresShell()->StyleSet(); }

  nsFrameManager* FrameManager()
    { return GetPresShell()->FrameManager(); } 
#endif

  



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

  



  nsILookAndFeel* LookAndFeel() { return mLookAndFeel; }

  


  nsIAtom* Medium() { return mMedium; }

  


  NS_HIDDEN_(void) ClearStyleDataAndReflow();

  void* AllocateFromShell(size_t aSize)
  {
    if (mShell)
      return mShell->AllocateFrame(aSize);
    return nsnull;
  }

  void FreeToShell(size_t aSize, void* aFreeChunk)
  {
    if (mShell)
      mShell->FreeFrame(aSize, aFreeChunk);
  }

  


  virtual NS_HIDDEN_(already_AddRefed<nsIFontMetrics>)
   GetMetricsForExternal(const nsFont& aFont);
  NS_HIDDEN_(already_AddRefed<nsIFontMetrics>)
    GetMetricsForInternal(const nsFont& aFont);
#ifdef _IMPL_NS_LAYOUT
  already_AddRefed<nsIFontMetrics> GetMetricsFor(const nsFont& aFont)
  { return GetMetricsForInternal(aFont); }
#else
  already_AddRefed<nsIFontMetrics> GetMetricsFor(const nsFont& aFont)
  { return GetMetricsForExternal(aFont); }
#endif

  
















  virtual NS_HIDDEN_(const nsFont*) GetDefaultFontExternal(PRUint8 aFontID) const;
  NS_HIDDEN_(const nsFont*) GetDefaultFontInternal(PRUint8 aFontID) const;
#ifdef _IMPL_NS_LAYOUT
  const nsFont* GetDefaultFont(PRUint8 aFontID) const
  { return GetDefaultFontInternal(aFontID); }
#else
  const nsFont* GetDefaultFont(PRUint8 aFontID) const
  { return GetDefaultFontExternal(aFontID); }
#endif

  
  
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
 

  






  NS_HIDDEN_(imgIRequest*) LoadImage(imgIRequest* aImage,
                                     nsIFrame* aTargetFrame);

  



  NS_HIDDEN_(void) StopImagesFor(nsIFrame* aTargetFrame);

  NS_HIDDEN_(void) SetContainer(nsISupports* aContainer);

  virtual NS_HIDDEN_(already_AddRefed<nsISupports>) GetContainerExternal();
  NS_HIDDEN_(already_AddRefed<nsISupports>) GetContainerInternal();
#ifdef _IMPL_NS_LAYOUT
  already_AddRefed<nsISupports> GetContainer()
  { return GetContainerInternal(); }
#else
  already_AddRefed<nsISupports> GetContainer()
  { return GetContainerExternal(); }
#endif

  
  void SetLinkHandler(nsILinkHandler* aHandler) { mLinkHandler = aHandler; }
  nsILinkHandler* GetLinkHandler() { return mLinkHandler; }

  





  nsRect GetVisibleArea() { return mVisibleArea; }

  



  void SetVisibleArea(const nsRect& r) { mVisibleArea = r; }

  



  PRBool IsPaginated() const { return mPaginated; }
  
  PRBool GetRenderedPositionVaryingContent() const { return mRenderedPositionVaryingContent; }
  void SetRenderedPositionVaryingContent() { mRenderedPositionVaryingContent = PR_TRUE; }

  



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
    mTextZoom = aZoom;
    ClearStyleDataAndReflow();
  }

  float GetFullZoom() {return mDeviceContext->GetPixelScale();}
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
                                   nsIDeviceContext::AppUnitsPerCSSPixel()); }

  static PRInt32 AppUnitsToIntCSSPixels(nscoord aAppUnits)
  { return NSAppUnitsToIntPixels(aAppUnits,
                                 nsIDeviceContext::AppUnitsPerCSSPixel()); }

  static float AppUnitsToFloatCSSPixels(nscoord aAppUnits)
  { return NSAppUnitsToFloatPixels(aAppUnits,
                                   nsIDeviceContext::AppUnitsPerCSSPixel()); }

  nscoord DevPixelsToAppUnits(PRInt32 aPixels) const
  { return NSIntPixelsToAppUnits(aPixels,
                                 mDeviceContext->AppUnitsPerDevPixel()); }

  PRInt32 AppUnitsToDevPixels(nscoord aAppUnits) const
  { return NSAppUnitsToIntPixels(aAppUnits,
                                 mDeviceContext->AppUnitsPerDevPixel()); }

  nscoord TwipsToAppUnits(PRInt32 aTwips) const
  { return NSToCoordRound(NS_TWIPS_TO_INCHES(aTwips) *
                          mDeviceContext->AppUnitsPerInch()); }

  PRInt32 AppUnitsToTwips(nscoord aTwips) const
  { return NS_INCHES_TO_TWIPS((float)aTwips /
                              mDeviceContext->AppUnitsPerInch()); }

  nscoord PointsToAppUnits(float aPoints) const
  { return NSToCoordRound(aPoints * mDeviceContext->AppUnitsPerInch() /
                          72.0f); }
  float AppUnitsToPoints(nscoord aAppUnits) const
  { return (float)aAppUnits / mDeviceContext->AppUnitsPerInch() * 72.0f; }

  







  nsLanguageSpecificTransformType LanguageSpecificTransformType() const
  {
    return mLanguageSpecificTransformType;
  }

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

  




  NS_HIDDEN_(void) SetBidiEnabled(PRBool aBidiEnabled) const;

  













  void SetVisualMode(PRBool aIsVisual)
  {
    NS_ASSERTION(!(aIsVisual & ~1), "Value must be true or false");
    mIsVisual = aIsVisual;
  }

  




  PRBool IsVisualMode() const { return mIsVisual; }



  


  NS_HIDDEN_(nsBidiPresUtils*) GetBidiUtils();

  

  
  NS_HIDDEN_(void) SetBidi(PRUint32 aBidiOptions,
                           PRBool aForceReflow = PR_FALSE);

  



  
  NS_HIDDEN_(PRUint32) GetBidi() const;

  



  void SetIsBidiSystem(PRBool aIsBidi)
  {
    NS_ASSERTION(!(aIsBidi & ~1), "Value must be true or false");
    mIsBidiSystem = aIsBidi;
  }

  



  PRBool IsBidiSystem() const { return mIsBidiSystem; }
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

  







  NS_HIDDEN_(PRBool) EnsureVisible(PRBool aUnsuppressFocus);
  
#ifdef MOZ_REFLOW_PERF
  NS_HIDDEN_(void) CountReflows(const char * aName,
                                nsIFrame * aFrame);
#endif

  



  const nscoord* GetBorderWidthTable() { return mBorderWidthTable; }

  PRBool IsDynamic() { return (mType == eContext_PageLayout || mType == eContext_Galley); }
  PRBool IsScreen() { return (mMedium == nsGkAtoms::screen ||
                              mType == eContext_PageLayout ||
                              mType == eContext_PrintPreview); }

  
  PRBool IsChrome();

protected:
  friend class nsRunnableMethod<nsPresContext>;
  NS_HIDDEN_(void) ThemeChangedInternal();
  NS_HIDDEN_(void) SysColorChangedInternal();
  
  NS_HIDDEN_(void) SetImgAnimations(nsIContent *aParent, PRUint16 aMode);
  NS_HIDDEN_(void) GetDocumentColorPreferences();

  NS_HIDDEN_(void) PreferenceChanged(const char* aPrefName);
  static NS_HIDDEN_(int) PR_CALLBACK PrefChangedCallback(const char*, void*);

  NS_HIDDEN_(void) UpdateAfterPreferencesChanged();
  static NS_HIDDEN_(void) PR_CALLBACK PrefChangedUpdateTimerCallback(nsITimer *aTimer, void *aClosure);

  NS_HIDDEN_(void) GetUserPreferences();
  NS_HIDDEN_(void) GetFontPreferences();

  NS_HIDDEN_(void) UpdateCharSet(const nsAFlatCString& aCharSet);

  
  
  
  
  nsPresContextType     mType;
  nsIPresShell*         mShell;         
  nsCOMPtr<nsIDocument> mDocument;
  nsIDeviceContext*     mDeviceContext; 
                                        
                                        
                                        
                                        
  nsIEventStateManager* mEventManager;  
  nsILookAndFeel*       mLookAndFeel;   
  nsIAtom*              mMedium;        
                                        

  nsILinkHandler*       mLinkHandler;   
  nsIAtom*              mLangGroup;     

  nsInterfaceHashtable<nsVoidPtrHashKey, nsImageLoader> mImageLoaders;
  nsWeakPtr             mContainer;

  float                 mTextZoom;      
  PRInt32               mAutoQualityMinFontSizePixelsPref;

#ifdef IBMBIDI
  nsBidiPresUtils*      mBidiUtils;
#endif

  nsCOMPtr<nsITheme> mTheme;
  nsCOMPtr<nsILanguageAtomService> mLangService;
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  nsCOMPtr<nsITimer>    mPrefChangedTimer;

  nsPropertyTable       mPropertyTable;

  nsLanguageSpecificTransformType mLanguageSpecificTransformType;
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

  unsigned              mUseDocumentFonts : 1;
  unsigned              mUseDocumentColors : 1;
  unsigned              mUnderlineLinks : 1;
  unsigned              mUseFocusColors : 1;
  unsigned              mFocusRingOnAnything : 1;
  unsigned              mDrawImageBackground : 1;
  unsigned              mDrawColorBackground : 1;
  unsigned              mNeverAnimate : 1;
  unsigned              mIsRenderingOnlySelection : 1;
  unsigned              mNoTheme : 1;
  unsigned              mPaginated : 1;
  unsigned              mCanPaginatedScroll : 1;
  unsigned              mDoScaledTwips : 1;
  unsigned              mEnableJapaneseTransform : 1;
  unsigned              mIsRootPaginatedDocument : 1;
  unsigned              mPrefBidiDirection : 1;
  unsigned              mPrefScrollbarSide : 2;
  unsigned              mPendingSysColorChanged : 1;
  unsigned              mPendingThemeChanged : 1;
  unsigned              mRenderedPositionVaryingContent : 1;

#ifdef IBMBIDI
  unsigned              mIsVisual : 1;
  unsigned              mIsBidiSystem : 1;

#endif
#ifdef DEBUG
  PRBool                mInitialized;
#endif


protected:

  ~nsPresContext() NS_HIDDEN;

  
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


#define NS_LOAD_IMAGE_STATUS_ERROR      0x1
#define NS_LOAD_IMAGE_STATUS_SIZE       0x2
#define NS_LOAD_IMAGE_STATUS_BITS       0x4

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
