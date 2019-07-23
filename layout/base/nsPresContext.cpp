








































#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsILinkHandler.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShell.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsPIDOMWindow.h"
#include "nsIFocusController.h"
#include "nsStyleSet.h"
#include "nsImageLoader.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIRenderingContext.h"
#include "nsIURL.h"
#include "nsIDocument.h"
#include "nsStyleContext.h"
#include "nsILookAndFeel.h"
#include "nsWidgetsCID.h"
#include "nsIComponentManager.h"
#include "nsIURIContentListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIServiceManager.h"
#include "nsIDOMElement.h"
#include "nsContentPolicyUtils.h"
#include "nsIDOMWindow.h"
#include "nsXPIDLString.h"
#include "nsIWeakReferenceUtils.h"
#include "nsCSSRendering.h"
#include "prprf.h"
#include "nsContentPolicyUtils.h"
#include "nsIDOMDocument.h"
#include "nsAutoPtr.h"
#include "nsEventStateManager.h"
#include "nsThreadUtils.h"
#include "nsFrameManager.h"
#include "nsLayoutUtils.h"
#include "nsIViewManager.h"
#include "nsCSSFrameConstructor.h"
#include "nsCSSRuleProcessor.h"
#include "nsStyleChangeList.h"
#include "nsRuleNode.h"
#include "nsEventDispatcher.h"
#include "gfxUserFontSet.h"
#include "gfxPlatform.h"
#include "nsCSSRules.h"
#include "nsFontFaceLoader.h"
#include "nsIEventListenerManager.h"

#ifdef MOZ_SMIL
#include "nsSMILAnimationController.h"
#endif 

#ifdef IBMBIDI
#include "nsBidiPresUtils.h"
#endif 

#include "nsContentUtils.h"


#include "imgIContainer.h"
#include "nsIImageLoadingContent.h"


#include "nsLayoutCID.h"

static nscolor
MakeColorPref(const char *colstr)
{
  PRUint32 red, green, blue;
  nscolor colorref;

  
  
  PR_sscanf(colstr, "#%02x%02x%02x", &red, &green, &blue);
  colorref = NS_RGB(red, green, blue);
  return colorref;
}

int
nsPresContext::PrefChangedCallback(const char* aPrefName, void* instance_data)
{
  nsPresContext*  presContext = (nsPresContext*)instance_data;

  NS_ASSERTION(nsnull != presContext, "bad instance data");
  if (nsnull != presContext) {
    presContext->PreferenceChanged(aPrefName);
  }
  return 0;  
}


void
nsPresContext::PrefChangedUpdateTimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsPresContext*  presContext = (nsPresContext*)aClosure;
  NS_ASSERTION(presContext != nsnull, "bad instance data");
  if (presContext)
    presContext->UpdateAfterPreferencesChanged();
}

#ifdef IBMBIDI
static PRBool
IsVisualCharset(const nsCString& aCharset)
{
  if (aCharset.LowerCaseEqualsLiteral("ibm864")             
      || aCharset.LowerCaseEqualsLiteral("ibm862")          
      || aCharset.LowerCaseEqualsLiteral("iso-8859-8") ) {  
    return PR_TRUE; 
  }
  else {
    return PR_FALSE; 
  }
}
#endif 


static PLDHashOperator
destroy_loads(const void * aKey, nsCOMPtr<nsImageLoader>& aData, void* closure)
{
  aData->Destroy();
  return PL_DHASH_NEXT;
}

static NS_DEFINE_CID(kLookAndFeelCID,  NS_LOOKANDFEEL_CID);
#include "nsContentCID.h"

  
  

nsPresContext::nsPresContext(nsIDocument* aDocument, nsPresContextType aType)
  : mType(aType), mDocument(aDocument), mTextZoom(1.0), mFullZoom(1.0),
    mPageSize(-1, -1), mPPScale(1.0f),
    mViewportStyleOverflow(NS_STYLE_OVERFLOW_AUTO, NS_STYLE_OVERFLOW_AUTO),
    mImageAnimationModePref(imgIContainer::kNormalAnimMode),
    
    mDefaultVariableFont("serif", NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                         NS_FONT_WEIGHT_NORMAL, 0, 0),
    mDefaultFixedFont("monospace", NS_FONT_STYLE_NORMAL,
                      NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL, 0, 0),
    mDefaultSerifFont("serif", NS_FONT_STYLE_NORMAL, NS_FONT_VARIANT_NORMAL,
                      NS_FONT_WEIGHT_NORMAL, 0, 0),
    mDefaultSansSerifFont("sans-serif", NS_FONT_STYLE_NORMAL,
                          NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL, 0, 0),
    mDefaultMonospaceFont("monospace", NS_FONT_STYLE_NORMAL,
                          NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL, 0, 0),
    mDefaultCursiveFont("cursive", NS_FONT_STYLE_NORMAL,
                        NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL, 0, 0),
    mDefaultFantasyFont("fantasy", NS_FONT_STYLE_NORMAL,
                        NS_FONT_VARIANT_NORMAL, NS_FONT_WEIGHT_NORMAL, 0, 0),
    mCanPaginatedScroll(PR_FALSE),
    mIsRootPaginatedDocument(PR_FALSE), mSupressResizeReflow(PR_FALSE)
{
  
  

  mDoScaledTwips = PR_TRUE;

  SetBackgroundImageDraw(PR_TRUE);		
  SetBackgroundColorDraw(PR_TRUE);

  mBackgroundColor = NS_RGB(0xFF, 0xFF, 0xFF);
  
  mUseDocumentColors = PR_TRUE;
  mUseDocumentFonts = PR_TRUE;

  

  mLinkColor = NS_RGB(0x00, 0x00, 0xEE);
  mActiveLinkColor = NS_RGB(0xEE, 0x00, 0x00);
  mVisitedLinkColor = NS_RGB(0x55, 0x1A, 0x8B);
  mUnderlineLinks = PR_TRUE;

  mFocusTextColor = mDefaultColor;
  mFocusBackgroundColor = mBackgroundColor;
  mFocusRingWidth = 1;

  if (aType == eContext_Galley) {
    mMedium = nsGkAtoms::screen;
  } else {
    SetBackgroundImageDraw(PR_FALSE);
    SetBackgroundColorDraw(PR_FALSE);
    mMedium = nsGkAtoms::print;
    mPaginated = PR_TRUE;
  }

  if (!IsDynamic()) {
    mImageAnimationMode = imgIContainer::kDontAnimMode;
    mNeverAnimate = PR_TRUE;
  } else {
    mImageAnimationMode = imgIContainer::kNormalAnimMode;
    mNeverAnimate = PR_FALSE;
  }
  NS_ASSERTION(mDocument, "Null document");
  mUserFontSet = nsnull;
  mUserFontSetDirty = PR_TRUE;
}

nsPresContext::~nsPresContext()
{
  mImageLoaders.Enumerate(destroy_loads, nsnull);
  mBorderImageLoaders.Enumerate(destroy_loads, nsnull);

  NS_PRECONDITION(!mShell, "Presshell forgot to clear our mShell pointer");
  SetShell(nsnull);

  if (mEventManager) {
    
    mEventManager->NotifyDestroyPresContext(this);
    mEventManager->SetPresContext(nsnull);

    NS_RELEASE(mEventManager);
  }

  if (mPrefChangedTimer)
  {
    mPrefChangedTimer->Cancel();
    mPrefChangedTimer = nsnull;
  }

  
  nsContentUtils::UnregisterPrefCallback("font.",
                                         nsPresContext::PrefChangedCallback,
                                         this);
  nsContentUtils::UnregisterPrefCallback("browser.display.",
                                         nsPresContext::PrefChangedCallback,
                                         this);
  nsContentUtils::UnregisterPrefCallback("browser.underline_anchors",
                                         nsPresContext::PrefChangedCallback,
                                         this);
  nsContentUtils::UnregisterPrefCallback("browser.anchor_color",
                                         nsPresContext::PrefChangedCallback,
                                         this);
  nsContentUtils::UnregisterPrefCallback("browser.active_color",
                                         nsPresContext::PrefChangedCallback,
                                         this);
  nsContentUtils::UnregisterPrefCallback("browser.visited_color",
                                         nsPresContext::PrefChangedCallback,
                                         this);
  nsContentUtils::UnregisterPrefCallback("image.animation_mode",
                                         nsPresContext::PrefChangedCallback,
                                         this);
#ifdef IBMBIDI
  nsContentUtils::UnregisterPrefCallback("bidi.", PrefChangedCallback, this);

  delete mBidiUtils;
#endif 
  nsContentUtils::UnregisterPrefCallback("layout.css.dpi",
                                         nsPresContext::PrefChangedCallback,
                                         this);

  NS_IF_RELEASE(mDeviceContext);
  NS_IF_RELEASE(mLookAndFeel);
  NS_IF_RELEASE(mLangGroup);
  NS_IF_RELEASE(mUserFontSet);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsPresContext)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsPresContext)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsPresContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsPresContext)

static PLDHashOperator
TraverseImageLoader(const void * aKey, nsCOMPtr<nsImageLoader>& aData,
                    void* aClosure)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);

  cb->NoteXPCOMChild(aData);

  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsPresContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocument);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(mDeviceContext); 
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(mEventManager);
  
  

  tmp->mImageLoaders.Enumerate(TraverseImageLoader, &cb);
  tmp->mBorderImageLoaders.Enumerate(TraverseImageLoader, &cb);

  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPrintSettings);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPrefChangedTimer);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsPresContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocument);
  NS_RELEASE(tmp->mDeviceContext); 
  if (tmp->mEventManager) {
    
    tmp->mEventManager->NotifyDestroyPresContext(tmp);
    tmp->mEventManager->SetPresContext(nsnull);

    NS_RELEASE(tmp->mEventManager);
  }

  
  

  tmp->mImageLoaders.Enumerate(destroy_loads, nsnull);
  tmp->mImageLoaders.Clear();
  tmp->mBorderImageLoaders.Enumerate(destroy_loads, nsnull);
  tmp->mBorderImageLoaders.Clear();

  
  
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPrintSettings);
  if (tmp->mPrefChangedTimer)
  {
    tmp->mPrefChangedTimer->Cancel();
    tmp->mPrefChangedTimer = nsnull;
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END


#define MAKE_FONT_PREF_KEY(_pref, _s0, _s1) \
 _pref.Assign(_s0); \
 _pref.Append(_s1);

static const char* const kGenericFont[] = {
  ".variable.",
  ".fixed.",
  ".serif.", 
  ".sans-serif.", 
  ".monospace.",
  ".cursive.",
  ".fantasy."
};




static PRBool sLookAndFeelChanged;




static PRBool sThemeChanged;

void
nsPresContext::GetFontPreferences()
{
  















  mDefaultVariableFont.size = CSSPixelsToAppUnits(16);
  mDefaultFixedFont.size = CSSPixelsToAppUnits(13);

  const char *langGroup = "x-western"; 
  if (mLangGroup) {
    mLangGroup->GetUTF8String(&langGroup);
  }

  nsCAutoString pref;

  
  enum {eUnit_unknown = -1, eUnit_px, eUnit_pt};
  PRInt32 unit = eUnit_px;

  nsAdoptingCString cvalue =
    nsContentUtils::GetCharPref("font.size.unit");

  if (!cvalue.IsEmpty()) {
    if (cvalue.Equals("px")) {
      unit = eUnit_px;
    }
    else if (cvalue.Equals("pt")) {
      unit = eUnit_pt;
    }
    else {
      NS_WARNING("unexpected font-size unit -- expected: 'px' or 'pt'");
      unit = eUnit_unknown;
    }
  }

  

  pref.Assign("font.minimum-size.");
  pref.Append(langGroup);

  PRInt32 size = nsContentUtils::GetIntPref(pref.get());
  if (unit == eUnit_px) {
    mMinimumFontSize = CSSPixelsToAppUnits(size);
  }
  else if (unit == eUnit_pt) {
    mMinimumFontSize = this->PointsToAppUnits(size);
  }

  
  nsCAutoString generic_dot_langGroup;
  for (PRInt32 eType = eDefaultFont_Variable; eType < eDefaultFont_COUNT; ++eType) {
    generic_dot_langGroup.Assign(kGenericFont[eType]);
    generic_dot_langGroup.Append(langGroup);

    nsFont* font;
    switch (eType) {
      case eDefaultFont_Variable:  font = &mDefaultVariableFont;  break;
      case eDefaultFont_Fixed:     font = &mDefaultFixedFont;     break;
      case eDefaultFont_Serif:     font = &mDefaultSerifFont;     break;
      case eDefaultFont_SansSerif: font = &mDefaultSansSerifFont; break;
      case eDefaultFont_Monospace: font = &mDefaultMonospaceFont; break;
      case eDefaultFont_Cursive:   font = &mDefaultCursiveFont;   break;
      case eDefaultFont_Fantasy:   font = &mDefaultFantasyFont;   break;
    }

    
    
    if (eType == eDefaultFont_Variable) {
      MAKE_FONT_PREF_KEY(pref, "font.name", generic_dot_langGroup);

      nsAdoptingString value =
        nsContentUtils::GetStringPref(pref.get());
      if (!value.IsEmpty()) {
        font->name.Assign(value);
      }
      else {
        MAKE_FONT_PREF_KEY(pref, "font.default.", langGroup);
        value = nsContentUtils::GetStringPref(pref.get());
        if (!value.IsEmpty()) {
          mDefaultVariableFont.name.Assign(value);
        }
      } 
    }
    else {
      if (eType == eDefaultFont_Monospace) {
        
        
        
        
        font->size = mDefaultFixedFont.size;
      }
      else if (eType != eDefaultFont_Fixed) {
        
        
        font->size = mDefaultVariableFont.size;
      }
    }

    
    
    
    

    
    
    MAKE_FONT_PREF_KEY(pref, "font.size", generic_dot_langGroup);
    size = nsContentUtils::GetIntPref(pref.get());
    if (size > 0) {
      if (unit == eUnit_px) {
        font->size = nsPresContext::CSSPixelsToAppUnits(size);
      }
      else if (unit == eUnit_pt) {
        font->size = this->PointsToAppUnits(size);
      }
    }

    
    
    MAKE_FONT_PREF_KEY(pref, "font.size-adjust", generic_dot_langGroup);
    cvalue = nsContentUtils::GetCharPref(pref.get());
    if (!cvalue.IsEmpty()) {
      font->sizeAdjust = (float)atof(cvalue.get());
    }

#ifdef DEBUG_rbs
    printf("%s Family-list:%s size:%d sizeAdjust:%.2f\n",
           generic_dot_langGroup.get(),
           NS_ConvertUTF16toUTF8(font->name).get(), font->size,
           font->sizeAdjust);
#endif
  }
}

void
nsPresContext::GetDocumentColorPreferences()
{
  PRInt32 useAccessibilityTheme = 0;
  PRBool usePrefColors = PR_TRUE;
  nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryReferent(mContainer));
  if (docShell) {
    PRInt32 docShellType;
    docShell->GetItemType(&docShellType);
    if (nsIDocShellTreeItem::typeChrome == docShellType) {
      usePrefColors = PR_FALSE;
    }
    else {
      mLookAndFeel->GetMetric(nsILookAndFeel::eMetric_UseAccessibilityTheme, useAccessibilityTheme);
      usePrefColors = !useAccessibilityTheme;
    }

  }
  if (usePrefColors) {
    usePrefColors =
      !nsContentUtils::GetBoolPref("browser.display.use_system_colors",
                                   PR_FALSE);
  }

  if (usePrefColors) {
    nsAdoptingCString colorStr =
      nsContentUtils::GetCharPref("browser.display.foreground_color");

    if (!colorStr.IsEmpty()) {
      mDefaultColor = MakeColorPref(colorStr);
    }

    colorStr =
      nsContentUtils::GetCharPref("browser.display.background_color");

    if (!colorStr.IsEmpty()) {
      mBackgroundColor = MakeColorPref(colorStr);
    }
  }
  else {
    mDefaultColor = NS_RGB(0x00, 0x00, 0x00);
    mBackgroundColor = NS_RGB(0xFF, 0xFF, 0xFF);
    mLookAndFeel->GetColor(nsILookAndFeel::eColor_WindowForeground,
                           mDefaultColor);
    mLookAndFeel->GetColor(nsILookAndFeel::eColor_WindowBackground,
                           mBackgroundColor);
  }

  
  
  mBackgroundColor = NS_ComposeColors(NS_RGB(0xFF, 0xFF, 0xFF),
                                      mBackgroundColor);

  mUseDocumentColors = !useAccessibilityTheme &&
    nsContentUtils::GetBoolPref("browser.display.use_document_colors",
                                mUseDocumentColors);
}

void
nsPresContext::GetUserPreferences()
{
  if (!GetPresShell()) {
    
    
    return;
  }
    
  mFontScaler =
    nsContentUtils::GetIntPref("browser.display.base_font_scaler",
                               mFontScaler);


  mAutoQualityMinFontSizePixelsPref =
    nsContentUtils::GetIntPref("browser.display.auto_quality_min_font_size");

  
  GetDocumentColorPreferences();

  
  mUnderlineLinks =
    nsContentUtils::GetBoolPref("browser.underline_anchors", mUnderlineLinks);

  nsAdoptingCString colorStr =
    nsContentUtils::GetCharPref("browser.anchor_color");

  if (!colorStr.IsEmpty()) {
    mLinkColor = MakeColorPref(colorStr);
  }

  colorStr =
    nsContentUtils::GetCharPref("browser.active_color");

  if (!colorStr.IsEmpty()) {
    mActiveLinkColor = MakeColorPref(colorStr);
  }

  colorStr = nsContentUtils::GetCharPref("browser.visited_color");

  if (!colorStr.IsEmpty()) {
    mVisitedLinkColor = MakeColorPref(colorStr);
  }

  mUseFocusColors =
    nsContentUtils::GetBoolPref("browser.display.use_focus_colors",
                                mUseFocusColors);

  mFocusTextColor = mDefaultColor;
  mFocusBackgroundColor = mBackgroundColor;

  colorStr = nsContentUtils::GetCharPref("browser.display.focus_text_color");

  if (!colorStr.IsEmpty()) {
    mFocusTextColor = MakeColorPref(colorStr);
  }

  colorStr =
    nsContentUtils::GetCharPref("browser.display.focus_background_color");

  if (!colorStr.IsEmpty()) {
    mFocusBackgroundColor = MakeColorPref(colorStr);
  }

  mFocusRingWidth =
    nsContentUtils::GetIntPref("browser.display.focus_ring_width",
                               mFocusRingWidth);

  mFocusRingOnAnything =
    nsContentUtils::GetBoolPref("browser.display.focus_ring_on_anything",
                                mFocusRingOnAnything);

  mFocusRingStyle =
          nsContentUtils::GetIntPref("browser.display.focus_ring_style",
                                      mFocusRingStyle);
  
  mUseDocumentFonts =
    nsContentUtils::GetIntPref("browser.display.use_document_fonts") != 0;

  
  mEnableJapaneseTransform =
    nsContentUtils::GetBoolPref("layout.enable_japanese_specific_transform");

  mPrefScrollbarSide =
    nsContentUtils::GetIntPref("layout.scrollbar.side");

  GetFontPreferences();

  
  const nsAdoptingCString& animatePref =
    nsContentUtils::GetCharPref("image.animation_mode");
  if (animatePref.Equals("normal"))
    mImageAnimationModePref = imgIContainer::kNormalAnimMode;
  else if (animatePref.Equals("none"))
    mImageAnimationModePref = imgIContainer::kDontAnimMode;
  else if (animatePref.Equals("once"))
    mImageAnimationModePref = imgIContainer::kLoopOnceAnimMode;
  else 
    mImageAnimationModePref = imgIContainer::kNormalAnimMode;

  PRUint32 bidiOptions = GetBidi();

  PRInt32 prefInt =
    nsContentUtils::GetIntPref(IBMBIDI_TEXTDIRECTION_STR,
                               GET_BIDI_OPTION_DIRECTION(bidiOptions));
  SET_BIDI_OPTION_DIRECTION(bidiOptions, prefInt);
  mPrefBidiDirection = prefInt;

  prefInt =
    nsContentUtils::GetIntPref(IBMBIDI_TEXTTYPE_STR,
                               GET_BIDI_OPTION_TEXTTYPE(bidiOptions));
  SET_BIDI_OPTION_TEXTTYPE(bidiOptions, prefInt);

  prefInt =
    nsContentUtils::GetIntPref(IBMBIDI_CONTROLSTEXTMODE_STR,
                               GET_BIDI_OPTION_CONTROLSTEXTMODE(bidiOptions));
  SET_BIDI_OPTION_CONTROLSTEXTMODE(bidiOptions, prefInt);

  prefInt =
    nsContentUtils::GetIntPref(IBMBIDI_NUMERAL_STR,
                               GET_BIDI_OPTION_NUMERAL(bidiOptions));
  SET_BIDI_OPTION_NUMERAL(bidiOptions, prefInt);

  prefInt =
    nsContentUtils::GetIntPref(IBMBIDI_SUPPORTMODE_STR,
                               GET_BIDI_OPTION_SUPPORT(bidiOptions));
  SET_BIDI_OPTION_SUPPORT(bidiOptions, prefInt);

  prefInt =
    nsContentUtils::GetIntPref(IBMBIDI_CHARSET_STR,
                               GET_BIDI_OPTION_CHARACTERSET(bidiOptions));
  SET_BIDI_OPTION_CHARACTERSET(bidiOptions, prefInt);

  
  
  
  SetBidi(bidiOptions, PR_FALSE);
}

void
nsPresContext::PreferenceChanged(const char* aPrefName)
{
  if (IsPaginated()) {
    
    
    
    return;
  }

  nsDependentCString prefName(aPrefName);
  if (prefName.EqualsLiteral("layout.css.dpi")) {
    PRInt32 oldAppUnitsPerDevPixel = AppUnitsPerDevPixel();
    if (mDeviceContext->CheckDPIChange() && mShell) {
      mDeviceContext->FlushFontCache();

      
      
      nscoord oldWidthAppUnits, oldHeightAppUnits;
      nsIViewManager* vm = GetViewManager();
      vm->GetWindowDimensions(&oldWidthAppUnits, &oldHeightAppUnits);
      float oldWidthDevPixels = oldWidthAppUnits/oldAppUnitsPerDevPixel;
      float oldHeightDevPixels = oldHeightAppUnits/oldAppUnitsPerDevPixel;

      nscoord width = NSToCoordRound(oldWidthDevPixels*AppUnitsPerDevPixel());
      nscoord height = NSToCoordRound(oldHeightDevPixels*AppUnitsPerDevPixel());
      vm->SetWindowDimensions(width, height);

      MediaFeatureValuesChanged(PR_TRUE);
      RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
    }
    return;
  }
  if (StringBeginsWith(prefName, NS_LITERAL_CSTRING("font."))) {
    
    
    

    
    
    
    
    mPrefChangePendingNeedsReflow = PR_TRUE;
  }
  if (StringBeginsWith(prefName, NS_LITERAL_CSTRING("bidi."))) {
    
    mPrefChangePendingNeedsReflow = PR_TRUE;

    
    
  }
  
  if (!mPrefChangedTimer)
  {
    mPrefChangedTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mPrefChangedTimer)
      return;
    mPrefChangedTimer->InitWithFuncCallback(nsPresContext::PrefChangedUpdateTimerCallback, (void*)this, 0, nsITimer::TYPE_ONE_SHOT);
  }
}

void
nsPresContext::UpdateAfterPreferencesChanged()
{
  mPrefChangedTimer = nsnull;

  nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryReferent(mContainer));
  if (docShell) {
    PRInt32 docShellType;
    docShell->GetItemType(&docShellType);
    if (nsIDocShellTreeItem::typeChrome == docShellType)
      return;
  }

  
  GetUserPreferences();

  
  if (mShell) {
    mShell->SetPreferenceStyleRules(PR_TRUE);
  }

  mDeviceContext->FlushFontCache();

  nsChangeHint hint = nsChangeHint(0);

  if (mPrefChangePendingNeedsReflow) {
    NS_UpdateHint(hint, NS_STYLE_HINT_REFLOW);
  }

  RebuildAllStyleData(hint);
}

nsresult
nsPresContext::Init(nsIDeviceContext* aDeviceContext)
{
  NS_ASSERTION(!(mInitialized == PR_TRUE), "attempt to reinit pres context");
  NS_ENSURE_ARG(aDeviceContext);

  mDeviceContext = aDeviceContext;
  NS_ADDREF(mDeviceContext);

  if (mDeviceContext->SetPixelScale(mFullZoom))
    mDeviceContext->FlushFontCache();
  mCurAppUnitsPerDevPixel = AppUnitsPerDevPixel();

  if (!mImageLoaders.Init())
    return NS_ERROR_OUT_OF_MEMORY;
  
  if (!mBorderImageLoaders.Init())
    return NS_ERROR_OUT_OF_MEMORY;
  
  
  
  nsresult rv = CallGetService(kLookAndFeelCID, &mLookAndFeel);
  if (NS_FAILED(rv)) {
    NS_ERROR("LookAndFeel service must be implemented for this toolkit");
    return rv;
  }

  mEventManager = new nsEventStateManager();
  if (!mEventManager)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(mEventManager);

  mLangService = do_GetService(NS_LANGUAGEATOMSERVICE_CONTRACTID);

  
  nsContentUtils::RegisterPrefCallback("font.",
                                       nsPresContext::PrefChangedCallback,
                                       this);
  nsContentUtils::RegisterPrefCallback("browser.display.",
                                       nsPresContext::PrefChangedCallback,
                                       this);
  nsContentUtils::RegisterPrefCallback("browser.underline_anchors",
                                       nsPresContext::PrefChangedCallback,
                                       this);
  nsContentUtils::RegisterPrefCallback("browser.anchor_color",
                                       nsPresContext::PrefChangedCallback,
                                       this);
  nsContentUtils::RegisterPrefCallback("browser.active_color",
                                       nsPresContext::PrefChangedCallback,
                                       this);
  nsContentUtils::RegisterPrefCallback("browser.visited_color",
                                       nsPresContext::PrefChangedCallback,
                                       this);
  nsContentUtils::RegisterPrefCallback("image.animation_mode",
                                       nsPresContext::PrefChangedCallback,
                                       this);
#ifdef IBMBIDI
  nsContentUtils::RegisterPrefCallback("bidi.", PrefChangedCallback,
                                       this);
#endif
  nsContentUtils::RegisterPrefCallback("layout.css.dpi",
                                       nsPresContext::PrefChangedCallback,
                                       this);

  rv = mEventManager->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mEventManager->SetPresContext(this);

#ifdef DEBUG
  mInitialized = PR_TRUE;
#endif

  mBorderWidthTable[NS_STYLE_BORDER_WIDTH_THIN] = CSSPixelsToAppUnits(1);
  mBorderWidthTable[NS_STYLE_BORDER_WIDTH_MEDIUM] = CSSPixelsToAppUnits(3);
  mBorderWidthTable[NS_STYLE_BORDER_WIDTH_THICK] = CSSPixelsToAppUnits(5);

  return NS_OK;
}



void
nsPresContext::SetShell(nsIPresShell* aShell)
{
  if (mShell) {
    
    
    nsIDocument *doc = mShell->GetDocument();
    if (doc) {
      doc->RemoveCharSetObserver(this);
    }
  }    

  mShell = aShell;

  if (mShell) {
    nsIDocument *doc = mShell->GetDocument();
    NS_ASSERTION(doc, "expect document here");
    if (doc) {
      
      mDocument = doc;
    }
    
    
    GetUserPreferences();

    if (doc) {
      nsIURI *docURI = doc->GetDocumentURI();

      if (IsDynamic() && docURI) {
        PRBool isChrome = PR_FALSE;
        PRBool isRes = PR_FALSE;
        docURI->SchemeIs("chrome", &isChrome);
        docURI->SchemeIs("resource", &isRes);

        if (!isChrome && !isRes)
          mImageAnimationMode = mImageAnimationModePref;
        else
          mImageAnimationMode = imgIContainer::kNormalAnimMode;
      }

      if (mLangService) {
        doc->AddCharSetObserver(this);
        UpdateCharSet(doc->GetDocumentCharacterSet());
      }
    }
  }
}

void
nsPresContext::UpdateCharSet(const nsAFlatCString& aCharSet)
{
  if (mLangService) {
    NS_IF_RELEASE(mLangGroup);
    mLangGroup = mLangService->LookupCharSet(aCharSet.get()).get();  

    
#if !defined(XP_BEOS) 
    if (mLangGroup == nsGkAtoms::Unicode) {
      NS_RELEASE(mLangGroup);
      NS_IF_ADDREF(mLangGroup = mLangService->GetLocaleLanguageGroup()); 
    }
#endif
    GetFontPreferences();
  }
#ifdef IBMBIDI
  

  switch (GET_BIDI_OPTION_TEXTTYPE(GetBidi())) {

    case IBMBIDI_TEXTTYPE_LOGICAL:
      SetVisualMode(PR_FALSE);
      break;

    case IBMBIDI_TEXTTYPE_VISUAL:
      SetVisualMode(PR_TRUE);
      break;

    case IBMBIDI_TEXTTYPE_CHARSET:
    default:
      SetVisualMode(IsVisualCharset(aCharSet));
  }
#endif 
}

NS_IMETHODIMP
nsPresContext::Observe(nsISupports* aSubject, 
                        const char* aTopic,
                        const PRUnichar* aData)
{
  if (!nsCRT::strcmp(aTopic, "charset")) {
    UpdateCharSet(NS_LossyConvertUTF16toASCII(aData));
    mDeviceContext->FlushFontCache();
    RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
    return NS_OK;
  }

  NS_WARNING("unrecognized topic in nsPresContext::Observe");
  return NS_ERROR_FAILURE;
}


nsPresContext*
nsPresContext::RootPresContext()
{
  nsPresContext* pc = this;
  for (;;) {
    if (pc->mShell) {
      nsIFrame* rootFrame = pc->mShell->FrameManager()->GetRootFrame();
      if (rootFrame) {
        nsIFrame* f = nsLayoutUtils::GetCrossDocParentFrame(rootFrame);
        if (f) {
          pc = f->PresContext();
          continue;
        }
      }
    }
    return pc;
  }
}

void
nsPresContext::CompatibilityModeChanged()
{
  if (!mShell)
    return;

  
  mShell->StyleSet()->
    EnableQuirkStyleSheet(CompatibilityMode() == eCompatibility_NavQuirks);
}


static void SetImgAnimModeOnImgReq(imgIRequest* aImgReq, PRUint16 aMode)
{
  if (aImgReq) {
    nsCOMPtr<imgIContainer> imgCon;
    aImgReq->GetImage(getter_AddRefs(imgCon));
    if (imgCon) {
      imgCon->SetAnimationMode(aMode);
    }
  }
}

 
static PLDHashOperator
set_animation_mode(const void * aKey, nsCOMPtr<nsImageLoader>& aData, void* closure)
{
  imgIRequest* imgReq = aData->GetRequest();
  SetImgAnimModeOnImgReq(imgReq, (PRUint16)NS_PTR_TO_INT32(closure));
  return PL_DHASH_NEXT;
}






void nsPresContext::SetImgAnimations(nsIContent *aParent, PRUint16 aMode)
{
  nsCOMPtr<nsIImageLoadingContent> imgContent(do_QueryInterface(aParent));
  if (imgContent) {
    nsCOMPtr<imgIRequest> imgReq;
    imgContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                           getter_AddRefs(imgReq));
    SetImgAnimModeOnImgReq(imgReq, aMode);
  }
  
  PRUint32 count = aParent->GetChildCount();
  for (PRUint32 i = 0; i < count; ++i) {
    SetImgAnimations(aParent->GetChildAt(i), aMode);
  }
}

#ifdef MOZ_SMIL
void
nsPresContext::SetSMILAnimations(nsIDocument *aDoc, PRUint16 aNewMode,
                                 PRUint16 aOldMode)
{
  nsSMILAnimationController *controller = aDoc->GetAnimationController();
  if (controller) {
    switch (aNewMode)
    {
      case imgIContainer::kNormalAnimMode:
      case imgIContainer::kLoopOnceAnimMode:
        if (aOldMode == imgIContainer::kDontAnimMode)
          controller->Resume(nsSMILTimeContainer::PAUSE_USERPREF);
        break;

      case imgIContainer::kDontAnimMode:
        if (aOldMode != imgIContainer::kDontAnimMode)
          controller->Pause(nsSMILTimeContainer::PAUSE_USERPREF);
        break;
    }
  }
}
#endif 

void
nsPresContext::SetImageAnimationModeInternal(PRUint16 aMode)
{
  NS_ASSERTION(aMode == imgIContainer::kNormalAnimMode ||
               aMode == imgIContainer::kDontAnimMode ||
               aMode == imgIContainer::kLoopOnceAnimMode, "Wrong Animation Mode is being set!");

  
  if (!IsDynamic())
    return;

  
  mImageLoaders.Enumerate(set_animation_mode, NS_INT32_TO_PTR(aMode));
  mBorderImageLoaders.Enumerate(set_animation_mode, NS_INT32_TO_PTR(aMode));

  
  
  if (mShell != nsnull) {
    nsIDocument *doc = mShell->GetDocument();
    if (doc) {
      nsIContent *rootContent = doc->GetRootContent();
      if (rootContent) {
        SetImgAnimations(rootContent, aMode);
      }

#ifdef MOZ_SMIL
      SetSMILAnimations(doc, aMode, mImageAnimationMode);
#endif 
    }
  }

  mImageAnimationMode = aMode;
}

void
nsPresContext::SetImageAnimationModeExternal(PRUint16 aMode)
{
  SetImageAnimationModeInternal(aMode);
}

already_AddRefed<nsIFontMetrics>
nsPresContext::GetMetricsFor(const nsFont& aFont)
{
  nsIFontMetrics* metrics = nsnull;
  mDeviceContext->GetMetricsFor(aFont, mLangGroup, GetUserFontSet(), metrics);
  return metrics;
}

const nsFont*
nsPresContext::GetDefaultFont(PRUint8 aFontID) const
{
  const nsFont *font;
  switch (aFontID) {
    
    case kPresContext_DefaultVariableFont_ID:
      font = &mDefaultVariableFont;
      break;
    case kPresContext_DefaultFixedFont_ID:
      font = &mDefaultFixedFont;
      break;
    
    case kGenericFont_serif:
      font = &mDefaultSerifFont;
      break;
    case kGenericFont_sans_serif:
      font = &mDefaultSansSerifFont;
      break;
    case kGenericFont_monospace:
      font = &mDefaultMonospaceFont;
      break;
    case kGenericFont_cursive:
      font = &mDefaultCursiveFont;
      break;
    case kGenericFont_fantasy: 
      font = &mDefaultFantasyFont;
      break;
    default:
      font = nsnull;
      NS_ERROR("invalid arg");
      break;
  }
  return font;
}

void
nsPresContext::SetFullZoom(float aZoom)
{
  if (!mShell || mFullZoom == aZoom) {
    return;
  }
  
  
  nscoord oldWidthAppUnits, oldHeightAppUnits;
  GetViewManager()->GetWindowDimensions(&oldWidthAppUnits, &oldHeightAppUnits);
  float oldWidthDevPixels = oldWidthAppUnits / float(mCurAppUnitsPerDevPixel);
  float oldHeightDevPixels = oldHeightAppUnits / float(mCurAppUnitsPerDevPixel);
  if (mDeviceContext->SetPixelScale(aZoom)) {
    mDeviceContext->FlushFontCache();
  }

  NS_ASSERTION(mSupressResizeReflow == PR_FALSE, "two zooms happening at the same time? impossible!");
  mSupressResizeReflow = PR_TRUE;

  mFullZoom = aZoom;
  GetViewManager()->SetWindowDimensions(NSToCoordRound(oldWidthDevPixels * AppUnitsPerDevPixel()),
                                        NSToCoordRound(oldHeightDevPixels * AppUnitsPerDevPixel()));
  if (HasCachedStyleData()) {
    MediaFeatureValuesChanged(PR_TRUE);
    RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
  }

  mSupressResizeReflow = PR_FALSE;

  mCurAppUnitsPerDevPixel = AppUnitsPerDevPixel();
}

imgIRequest*
nsPresContext::DoLoadImage(nsPresContext::ImageLoaderTable& aTable,
                           imgIRequest* aImage,
                           nsIFrame* aTargetFrame,
                           PRBool aReflowOnLoad)
{
  
  nsCOMPtr<nsImageLoader> loader;
  aTable.Get(aTargetFrame, getter_AddRefs(loader));

  if (!loader) {
    loader = new nsImageLoader();
    if (!loader)
      return nsnull;

    loader->Init(aTargetFrame, this, aReflowOnLoad);
    aTable.Put(aTargetFrame, loader);
  }

  loader->Load(aImage);

  imgIRequest *request = loader->GetRequest();

  return request;
}

imgIRequest*
nsPresContext::LoadImage(imgIRequest* aImage, nsIFrame* aTargetFrame)
{
  return DoLoadImage(mImageLoaders, aImage, aTargetFrame, PR_FALSE);
}

imgIRequest*
nsPresContext::LoadBorderImage(imgIRequest* aImage, nsIFrame* aTargetFrame)
{
  return DoLoadImage(mBorderImageLoaders, aImage, aTargetFrame,
                     aTargetFrame->GetStyleBorder()->ImageBorderDiffers());
}

void
nsPresContext::StopImagesFor(nsIFrame* aTargetFrame)
{
  StopBackgroundImageFor(aTargetFrame);
  StopBorderImageFor(aTargetFrame);
}

void
nsPresContext::DoStopImageFor(nsPresContext::ImageLoaderTable& aTable,
                              nsIFrame* aTargetFrame)
{
  nsCOMPtr<nsImageLoader> loader;
  aTable.Get(aTargetFrame, getter_AddRefs(loader));

  if (loader) {
    loader->Destroy();

    aTable.Remove(aTargetFrame);
  }
}
  
void
nsPresContext::SetContainer(nsISupports* aHandler)
{
  mContainer = do_GetWeakReference(aHandler);
  if (mContainer) {
    GetDocumentColorPreferences();
  }
}

already_AddRefed<nsISupports>
nsPresContext::GetContainerInternal() const
{
  nsISupports *result = nsnull;
  if (mContainer)
    CallQueryReferent(mContainer.get(), &result);

  return result;
}

already_AddRefed<nsISupports>
nsPresContext::GetContainerExternal() const
{
  return GetContainerInternal();
}

#ifdef IBMBIDI
PRBool
nsPresContext::BidiEnabledInternal() const
{
  PRBool bidiEnabled = PR_FALSE;
  NS_ASSERTION(mShell, "PresShell must be set on PresContext before calling nsPresContext::GetBidiEnabled");
  if (mShell) {
    nsIDocument *doc = mShell->GetDocument();
    NS_ASSERTION(doc, "PresShell has no document in nsPresContext::GetBidiEnabled");
    if (doc) {
      bidiEnabled = doc->GetBidiEnabled();
    }
  }
  return bidiEnabled;
}

PRBool
nsPresContext::BidiEnabledExternal() const
{
  return BidiEnabledInternal();
}

void
nsPresContext::SetBidiEnabled() const
{
  if (mShell) {
    nsIDocument *doc = mShell->GetDocument();
    if (doc) {
      doc->SetBidiEnabled();
    }
  }
}

nsBidiPresUtils*
nsPresContext::GetBidiUtils()
{
  if (!mBidiUtils)
    mBidiUtils = new nsBidiPresUtils;

  return mBidiUtils;
}

void
nsPresContext::SetBidi(PRUint32 aSource, PRBool aForceRestyle)
{
  
  if (aSource == GetBidi()) {
    return;
  }

  NS_ASSERTION(!(aForceRestyle && (GetBidi() == 0)), 
               "ForceReflow on new prescontext");

  Document()->SetBidiOptions(aSource);
  if (IBMBIDI_TEXTDIRECTION_RTL == GET_BIDI_OPTION_DIRECTION(aSource)
      || IBMBIDI_NUMERAL_HINDI == GET_BIDI_OPTION_NUMERAL(aSource)) {
    SetBidiEnabled();
  }
  if (IBMBIDI_TEXTTYPE_VISUAL == GET_BIDI_OPTION_TEXTTYPE(aSource)) {
    SetVisualMode(PR_TRUE);
  }
  else if (IBMBIDI_TEXTTYPE_LOGICAL == GET_BIDI_OPTION_TEXTTYPE(aSource)) {
    SetVisualMode(PR_FALSE);
  }
  else {
    nsIDocument* doc = mShell->GetDocument();
    if (doc) {
      SetVisualMode(IsVisualCharset(doc->GetDocumentCharacterSet()));
    }
  }
  if (aForceRestyle) {
    RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
  }
}

PRUint32
nsPresContext::GetBidi() const
{
  return Document()->GetBidiOptions();
}
#endif 

nsITheme*
nsPresContext::GetTheme()
{
  if (!mNoTheme && !mTheme) {
    mTheme = do_GetService("@mozilla.org/chrome/chrome-native-theme;1");
    if (!mTheme)
      mNoTheme = PR_TRUE;
  }

  return mTheme;
}

void
nsPresContext::ThemeChanged()
{
  if (!mPendingThemeChanged) {
    sLookAndFeelChanged = PR_TRUE;
    sThemeChanged = PR_TRUE;

    nsCOMPtr<nsIRunnable> ev =
      new nsRunnableMethod<nsPresContext>(this,
                                          &nsPresContext::ThemeChangedInternal);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mPendingThemeChanged = PR_TRUE;
    }
  }    
}

void
nsPresContext::ThemeChangedInternal()
{
  mPendingThemeChanged = PR_FALSE;
  
  
  
  if (mTheme && sThemeChanged) {
    mTheme->ThemeChanged();
    sThemeChanged = PR_FALSE;
  }

  
  if (mLookAndFeel && sLookAndFeelChanged) {
    mLookAndFeel->LookAndFeelChanged();
    sLookAndFeelChanged = PR_FALSE;
  }

  
  nsCSSRuleProcessor::FreeSystemMetrics();

  
  
  
  
  RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
}

void
nsPresContext::SysColorChanged()
{
  if (!mPendingSysColorChanged) {
    sLookAndFeelChanged = PR_TRUE;
    nsCOMPtr<nsIRunnable> ev =
      new nsRunnableMethod<nsPresContext>(this,
                                          &nsPresContext::SysColorChangedInternal);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mPendingSysColorChanged = PR_TRUE;
    }
  }
}

void
nsPresContext::SysColorChangedInternal()
{
  mPendingSysColorChanged = PR_FALSE;
  
  if (mLookAndFeel && sLookAndFeelChanged) {
     
    mLookAndFeel->LookAndFeelChanged();
    sLookAndFeelChanged = PR_FALSE;
  }
   
  
  
  GetDocumentColorPreferences();

  
  
  RebuildAllStyleData(nsChangeHint(0));
}

void
nsPresContext::RebuildAllStyleData(nsChangeHint aExtraHint)
{
  if (!mShell) {
    
    return;
  }

  RebuildUserFontSet();

  mShell->FrameConstructor()->RebuildAllStyleData(aExtraHint);
}

void
nsPresContext::PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint)
{
  if (!mShell) {
    
    return;
  }
  mShell->FrameConstructor()->PostRebuildAllStyleDataEvent(aExtraHint);
}

void
nsPresContext::MediaFeatureValuesChanged(PRBool aCallerWillRebuildStyleData)
{
  mPendingMediaFeatureValuesChanged = PR_FALSE;
  if (mShell->StyleSet()->MediumFeaturesChanged(this) &&
      !aCallerWillRebuildStyleData) {
    RebuildAllStyleData(nsChangeHint(0));
  }
}

void
nsPresContext::PostMediaFeatureValuesChangedEvent()
{
  if (!mPendingMediaFeatureValuesChanged) {
    nsCOMPtr<nsIRunnable> ev =
      new nsRunnableMethod<nsPresContext>(this,
                         &nsPresContext::HandleMediaFeatureValuesChangedEvent);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mPendingMediaFeatureValuesChanged = PR_TRUE;
    }
  }
}

void
nsPresContext::HandleMediaFeatureValuesChangedEvent()
{
  
  
  if (mPendingMediaFeatureValuesChanged && mShell) {
    MediaFeatureValuesChanged(PR_FALSE);
  }
}

void
nsPresContext::SetPaginatedScrolling(PRBool aPaginated)
{
  if (mType == eContext_PrintPreview || mType == eContext_PageLayout)
    mCanPaginatedScroll = aPaginated;
}

void
nsPresContext::SetPrintSettings(nsIPrintSettings *aPrintSettings)
{
  if (mMedium == nsGkAtoms::print)
    mPrintSettings = aPrintSettings;
}

PRBool
nsPresContext::EnsureVisible(PRBool aUnsuppressFocus)
{
  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  if (docShell) {
    nsCOMPtr<nsIContentViewer> cv;
    docShell->GetContentViewer(getter_AddRefs(cv));
    
    nsCOMPtr<nsIDocumentViewer> docV(do_QueryInterface(cv));
    if (docV) {
      nsCOMPtr<nsPresContext> currentPresContext;
      docV->GetPresContext(getter_AddRefs(currentPresContext));
      if (currentPresContext == this) {
        
        
        
        nsCOMPtr<nsPIDOMWindow> privWindow = do_GetInterface(docShell);
        
        nsIFocusController* fc =
          privWindow ? privWindow->GetRootFocusController() : nsnull;
        if (fc) {
          fc->SetSuppressFocus(PR_TRUE,
                               "nsPresContext::EnsureVisible Suppression");
        }
        cv->Show();
        if (fc && aUnsuppressFocus) {
          fc->SetSuppressFocus(PR_FALSE,
                               "nsPresContext::EnsureVisible Suppression");
        }
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
}

#ifdef MOZ_REFLOW_PERF
void
nsPresContext::CountReflows(const char * aName, nsIFrame * aFrame)
{
  if (mShell) {
    mShell->CountReflows(aName, aFrame);
  }
}
#endif

PRBool
nsPresContext::IsChrome() const
{
  PRBool isChrome = PR_FALSE;
  nsCOMPtr<nsISupports> container = GetContainer();
  if (container) {
    nsresult result;
    nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryInterface(container, &result));
    if (NS_SUCCEEDED(result) && docShell) {
      PRInt32 docShellType;
      result = docShell->GetItemType(&docShellType);
      if (NS_SUCCEEDED(result)) {
        isChrome = nsIDocShellTreeItem::typeChrome == docShellType;
      }
    }
  }
  return isChrome;
}

 PRBool
nsPresContext::HasAuthorSpecifiedRules(nsIFrame *aFrame, PRUint32 ruleTypeMask) const
{
  return
    UseDocumentColors() &&
    nsRuleNode::HasAuthorSpecifiedRules(aFrame->GetStyleContext(),
                                        ruleTypeMask);
}

static void
InsertFontFaceRule(nsCSSFontFaceRule *aRule, gfxUserFontSet* aFontSet,
                   PRUint8 aSheetType)
{
  PRInt32 type;
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(aRule->GetType(type)) 
                    && type == nsICSSRule::FONT_FACE_RULE, 
                    "InsertFontFaceRule passed a non-fontface CSS rule");

  

  nsAutoString fontfamily;
  nsCSSValue val;

  PRUint32 unit;
  PRUint32 weight = NS_STYLE_FONT_WEIGHT_NORMAL;
  PRUint32 stretch = NS_STYLE_FONT_STRETCH_NORMAL;
  PRUint32 italicStyle = FONT_STYLE_NORMAL;

  
  aRule->GetDesc(eCSSFontDesc_Family, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_String) {
    val.GetStringValue(fontfamily);
    fontfamily.Trim("\"");
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null,
                 "@font-face family name has unexpected unit");
    
    
    return;
  }

  
  aRule->GetDesc(eCSSFontDesc_Weight, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Integer || unit == eCSSUnit_Enumerated) {
    weight = val.GetIntValue();
  } else if (unit == eCSSUnit_Normal) {
    weight = NS_STYLE_FONT_WEIGHT_NORMAL;
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null,
                 "@font-face weight has unexpected unit");
  }

  
  aRule->GetDesc(eCSSFontDesc_Stretch, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Enumerated) {
    stretch = val.GetIntValue();
  } else if (unit == eCSSUnit_Normal) {
    stretch = NS_STYLE_FONT_STRETCH_NORMAL;
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null,
                 "@font-face stretch has unexpected unit");
  }

  
  aRule->GetDesc(eCSSFontDesc_Style, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Enumerated) {
    italicStyle = val.GetIntValue();
  } else if (unit == eCSSUnit_Normal) {
    italicStyle = FONT_STYLE_NORMAL;
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null,
                 "@font-face style has unexpected unit");
  }

  
  nsTArray<gfxFontFaceSrc> srcArray;

  aRule->GetDesc(eCSSFontDesc_Src, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Array) {
    nsCSSValue::Array *srcArr = val.GetArrayValue();
    PRUint32 i, numSrc = srcArr->Count();
    
    for (i = 0; i < numSrc; i++) {
      val = srcArr->Item(i);
      unit = val.GetUnit();
      gfxFontFaceSrc *face = srcArray.AppendElements(1);
      if (!face)
        return;
            
      switch (unit) {
       
      case eCSSUnit_Local_Font:
        val.GetStringValue(face->mLocalName);
        face->mIsLocal = PR_TRUE;
        face->mURI = nsnull;
        face->mFormatFlags = 0;
        break;
      case eCSSUnit_URL:
        face->mIsLocal = PR_FALSE;
        face->mURI = val.GetURLValue();
        NS_ASSERTION(face->mURI, "null url in @font-face rule");
        face->mReferrer = val.GetURLStructValue()->mReferrer;
        face->mOriginPrincipal = val.GetURLStructValue()->mOriginPrincipal;
        NS_ASSERTION(face->mOriginPrincipal, "null origin principal in @font-face rule");
        
        
        
        
        
        face->mUseOriginPrincipal = (aSheetType == nsStyleSet::eUserSheet ||
                                     aSheetType == nsStyleSet::eAgentSheet);
                                     
        face->mLocalName.Truncate();
        face->mFormatFlags = 0;
        while (i + 1 < numSrc && (val = srcArr->Item(i+1), 
                 val.GetUnit() == eCSSUnit_Font_Format)) {
          nsDependentString valueString(val.GetStringBufferValue());
          if (valueString.LowerCaseEqualsASCII("opentype")) {
            face->mFormatFlags |= gfxUserFontSet::FLAG_FORMAT_OPENTYPE; 
          } else if (valueString.LowerCaseEqualsASCII("truetype")) {
            face->mFormatFlags |= gfxUserFontSet::FLAG_FORMAT_TRUETYPE; 
          } else if (valueString.LowerCaseEqualsASCII("truetype-aat")) {
            face->mFormatFlags |= gfxUserFontSet::FLAG_FORMAT_TRUETYPE_AAT; 
          } else if (valueString.LowerCaseEqualsASCII("embedded-opentype")) {
            face->mFormatFlags |= gfxUserFontSet::FLAG_FORMAT_EOT;   
          } else if (valueString.LowerCaseEqualsASCII("svg")) {
            face->mFormatFlags |= gfxUserFontSet::FLAG_FORMAT_SVG;   
          } else {
            
            
            face->mFormatFlags |= gfxUserFontSet::FLAG_FORMAT_UNKNOWN;
          }
          i++;
        }
        break;
      default:
        NS_ASSERTION(unit == eCSSUnit_Local_Font || unit == eCSSUnit_URL,
                     "strange unit type in font-face src array");
        break;
      }
     }
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null, "@font-face src has unexpected unit");
  }
  
  if (!fontfamily.IsEmpty() && srcArray.Length() > 0) {
    aFontSet->AddFontFace(fontfamily, srcArray, weight, stretch, italicStyle);
  }
}

gfxUserFontSet*
nsPresContext::GetUserFontSetInternal()
{
  
  
  
  
  
  
  
  if (mUserFontSetDirty) {
    
    
    
    
    
    
#ifdef DEBUG
    {
      PRBool inReflow;
      NS_ASSERTION(!mGetUserFontSetCalled ||
                   (NS_SUCCEEDED(mShell->IsReflowLocked(&inReflow)) &&
                    !inReflow),
                   "FlushUserFontSet should have been called first");
    }
#endif
    FlushUserFontSet();
  }

  mGetUserFontSetCalled = PR_TRUE;
  return mUserFontSet;
}

gfxUserFontSet*
nsPresContext::GetUserFontSetExternal()
{
  return GetUserFontSetInternal();
}

void
nsPresContext::FlushUserFontSet()
{
  if (!mShell)
    return; 

  if (mUserFontSetDirty) {
    if (gfxPlatform::GetPlatform()->DownloadableFontsEnabled()) {
      nsRefPtr<gfxUserFontSet> oldUserFontSet = mUserFontSet;

      nsTArray<nsFontFaceRuleContainer> rules;
      if (!mShell->StyleSet()->AppendFontFaceRules(this, rules))
        return;

      PRBool differ;
      if (rules.Length() == mFontFaceRules.Length()) {
        differ = PR_FALSE;
        for (PRUint32 i = 0, i_end = rules.Length(); i < i_end; ++i) {
          if (rules[i].mRule != mFontFaceRules[i].mRule ||
              rules[i].mSheetType != mFontFaceRules[i].mSheetType) {
            differ = PR_TRUE;
            break;
          }
        }
      } else {
        differ = PR_TRUE;
      }

      
      if (differ) {
        NS_IF_RELEASE(mUserFontSet);

        if (rules.Length() > 0) {
          gfxUserFontSet *fs = new nsUserFontSet(this);
          if (!fs)
            return;
          mUserFontSet = fs;
          NS_ADDREF(mUserFontSet);

          for (PRUint32 i = 0, i_end = rules.Length(); i < i_end; ++i) {
            InsertFontFaceRule(rules[i].mRule, fs, rules[i].mSheetType);
          }
        }
      }

#ifdef DEBUG
      PRBool success =
#endif
        rules.SwapElements(mFontFaceRules);
      NS_ASSERTION(success, "should never fail given both are heap arrays");

      if (mGetUserFontSetCalled && oldUserFontSet != mUserFontSet) {
        
        
        
        
        
        
        UserFontSetUpdated();
      }
    }

    mUserFontSetDirty = PR_FALSE;
  }
}

void
nsPresContext::RebuildUserFontSet()
{
  if (!mGetUserFontSetCalled) {
    
    
    
    return;
  }

  mUserFontSetDirty = PR_TRUE;

  
  
  
  
  
  
  
  if (!mPostedFlushUserFontSet) {
    nsCOMPtr<nsIRunnable> ev =
      new nsRunnableMethod<nsPresContext>(this,
                                     &nsPresContext::HandleRebuildUserFontSet);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mPostedFlushUserFontSet = PR_TRUE;
    }
  }    
}

void
nsPresContext::UserFontSetUpdated()
{
  if (!mShell)
    return;

  
  
  
  
  
  
  
  
  
  
  

  PostRebuildAllStyleDataEvent(NS_STYLE_HINT_REFLOW);
}

void
nsPresContext::FireDOMPaintEvent()
{
  nsCOMPtr<nsPIDOMWindow> ourWindow = mDocument->GetWindow();
  if (!ourWindow)
    return;

  nsISupports* eventTarget = ourWindow;
  if (mSameDocDirtyRegion.IsEmpty() && !IsChrome()) {
    
    
    
    
    eventTarget = ourWindow->GetChromeEventHandler();
    if (!eventTarget) {
      return;
    }
  }
  
  

  nsNotifyPaintEvent event(PR_TRUE, NS_AFTERPAINT, mSameDocDirtyRegion,
                           mCrossDocDirtyRegion);
  
  
  
  mSameDocDirtyRegion.SetEmpty();
  mCrossDocDirtyRegion.SetEmpty();
  
  
  
  event.target = do_QueryInterface(ourWindow);
  nsEventDispatcher::Dispatch(eventTarget, this, &event);
}

static PRBool MayHavePaintEventListener(nsPIDOMWindow* aInnerWindow)
{
  if (!aInnerWindow)
    return PR_FALSE;
  if (aInnerWindow->HasPaintEventListeners())
    return PR_TRUE;

  nsPIDOMEventTarget* chromeEventHandler = aInnerWindow->GetChromeEventHandler();
  if (!chromeEventHandler)
    return PR_FALSE;

  nsCOMPtr<nsIEventListenerManager> manager;
  chromeEventHandler->GetListenerManager(PR_FALSE, getter_AddRefs(manager));
  if (manager && manager->MayHavePaintEventListener())
    return PR_TRUE;

  nsCOMPtr<nsINode> node = do_QueryInterface(chromeEventHandler);
  if (node)
    return MayHavePaintEventListener(node->GetOwnerDoc()->GetInnerWindow());

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(chromeEventHandler);
  if (window)
    return MayHavePaintEventListener(window);

  return PR_FALSE;
}

void
nsPresContext::NotifyInvalidation(const nsRect& aRect, PRBool aIsCrossDoc)
{
  
  
  
  
  
  if (aRect.IsEmpty() ||
      !MayHavePaintEventListener(mDocument->GetInnerWindow()))
    return;

  if (!IsDOMPaintEventPending()) {
    
    nsCOMPtr<nsIRunnable> ev =
      new nsRunnableMethod<nsPresContext>(this,
                                          &nsPresContext::FireDOMPaintEvent);
    NS_DispatchToCurrentThread(ev);
  }

  nsRegion* r = aIsCrossDoc ? &mCrossDocDirtyRegion : &mSameDocDirtyRegion;
  r->Or(*r, aRect);
  r->SimplifyOutward(10);
}

PRBool
nsPresContext::HasCachedStyleData()
{
  return mShell && mShell->StyleSet()->HasCachedStyleData();
}
