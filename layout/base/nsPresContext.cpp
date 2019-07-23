






































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
#include "nsStyleChangeList.h"

#ifdef IBMBIDI
#include "nsBidiPresUtils.h"
#endif 

#include "nsContentUtils.h"


#include "imgIContainer.h"
#include "nsIImageLoadingContent.h"


#include "nsLayoutCID.h"
#ifndef MOZ_CAIRO_GFX
#include "nsISelectionImageService.h"
#endif
static nscolor
MakeColorPref(const char *colstr)
{
  PRUint32 red, green, blue;
  nscolor colorref;

  
  
  PR_sscanf(colstr, "#%02x%02x%02x", &red, &green, &blue);
  colorref = NS_RGB(red, green, blue);
  return colorref;
}

int PR_CALLBACK
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


PR_STATIC_CALLBACK(PLDHashOperator)
destroy_loads(const void * aKey, nsCOMPtr<nsImageLoader>& aData, void* closure)
{
  aData->Destroy();
  return PL_DHASH_NEXT;
}

static NS_DEFINE_CID(kLookAndFeelCID,  NS_LOOKANDFEEL_CID);
#include "nsContentCID.h"
static NS_DEFINE_CID(kSelectionImageService, NS_SELECTIONIMAGESERVICE_CID);

  
  

nsPresContext::nsPresContext(nsIDocument* aDocument, nsPresContextType aType)
  : mType(aType), mDocument(aDocument), mTextZoom(1.0),
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
    mIsRootPaginatedDocument(PR_FALSE)
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

  mLanguageSpecificTransformType = eLanguageSpecificTransformType_Unknown;
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
}

nsPresContext::~nsPresContext()
{
  mImageLoaders.Enumerate(destroy_loads, nsnull);

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
}

NS_IMPL_ISUPPORTS1(nsPresContext, nsIObserver)

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
  if (!mLangGroup)
    return;

  















  mDefaultVariableFont.size = CSSPixelsToAppUnits(16);
  mDefaultFixedFont.size = CSSPixelsToAppUnits(13);

  const char *langGroup;
  mLangGroup->GetUTF8String(&langGroup);

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
nsPresContext::ClearStyleDataAndReflow()
{
  
  
  
  if (mShell && mShell->GetRootFrame()) {
    
    
    nsresult rv = mShell->StyleSet()->BeginReconstruct();
    if (NS_FAILED(rv))
      return;
    
    
    
    
    
    
    nsStyleChangeList changeList;
    mShell->FrameManager()->ComputeStyleChangeFor(mShell->GetRootFrame(),
                                                  &changeList, nsChangeHint(0));
    
    mShell->StyleSet()->EndReconstruct();
    
    mShell->FrameConstructor()->ProcessRestyledFrames(changeList);
  }
}

void
nsPresContext::PreferenceChanged(const char* aPrefName)
{
  if (!nsCRT::strcmp(aPrefName, "layout.css.dpi")) {
    nsRect bounds(mVisibleArea);
    bounds *= 1.0f / AppUnitsPerDevPixel();
    if (mDeviceContext->CheckDPIChange() && mShell) {
      mDeviceContext->FlushFontCache();

      nsIViewManager* vm = GetViewManager();
      nscoord width = DevPixelsToAppUnits(bounds.width);
      nscoord height = DevPixelsToAppUnits(bounds.height);
      vm->SetWindowDimensions(width, height);

      ClearStyleDataAndReflow();
    }
    return;
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
  ClearStyleDataAndReflow();
}

nsresult
nsPresContext::Init(nsIDeviceContext* aDeviceContext)
{
  NS_ASSERTION(!(mInitialized == PR_TRUE), "attempt to reinit pres context");
  NS_ENSURE_ARG(aDeviceContext);

  mDeviceContext = aDeviceContext;
  NS_ADDREF(mDeviceContext);

  if (!mImageLoaders.Init())
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

    if (mLangGroup == nsGkAtoms::Japanese && mEnableJapaneseTransform) {
      mLanguageSpecificTransformType =
        eLanguageSpecificTransformType_Japanese;
    }
    else {
      mLanguageSpecificTransformType =
        eLanguageSpecificTransformType_None;
    }
    
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
    ClearStyleDataAndReflow();

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

 
PR_STATIC_CALLBACK(PLDHashOperator)
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

void
nsPresContext::SetImageAnimationModeInternal(PRUint16 aMode)
{
  NS_ASSERTION(aMode == imgIContainer::kNormalAnimMode ||
               aMode == imgIContainer::kDontAnimMode ||
               aMode == imgIContainer::kLoopOnceAnimMode, "Wrong Animation Mode is being set!");

  
  if (!IsDynamic())
    return;

  
  
  mImageLoaders.Enumerate(set_animation_mode, NS_INT32_TO_PTR(aMode));

  
  
  if (mShell != nsnull) {
    nsIDocument *doc = mShell->GetDocument();
    if (doc) {
      nsIContent *rootContent = doc->GetRootContent();
      if (rootContent) {
        SetImgAnimations(rootContent, aMode);
      }
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
nsPresContext::GetMetricsForInternal(const nsFont& aFont)
{
  nsIFontMetrics* metrics = nsnull;
  mDeviceContext->GetMetricsFor(aFont, mLangGroup, metrics);
  return metrics;
}

already_AddRefed<nsIFontMetrics>
nsPresContext::GetMetricsForExternal(const nsFont& aFont)
{
  return GetMetricsForInternal(aFont);
}

const nsFont*
nsPresContext::GetDefaultFontInternal(PRUint8 aFontID) const
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

const nsFont*
nsPresContext::GetDefaultFontExternal(PRUint8 aFontID) const
{
  return GetDefaultFontInternal(aFontID);
}

void
nsPresContext::SetFullZoom(float aZoom)
{
  nsPresContext* rootPresContext = RootPresContext();
  if (rootPresContext != this) {
    NS_WARNING("Zoom set on non-root prescontext");
    rootPresContext->SetFullZoom(aZoom);
    return;
  }
  nsRect bounds(mVisibleArea);
  bounds.ScaleRoundPreservingCentersInverse(AppUnitsPerDevPixel());
  if (!mShell || !mDeviceContext->SetPixelScale(aZoom))
    return;
  mDeviceContext->FlushFontCache();
  nscoord width = DevPixelsToAppUnits(bounds.width);
  nscoord height = DevPixelsToAppUnits(bounds.height);
  GetViewManager()->SetWindowDimensions(width, height);
  ClearStyleDataAndReflow();
}

imgIRequest*
nsPresContext::LoadImage(imgIRequest* aImage, nsIFrame* aTargetFrame)
{
  
  nsCOMPtr<nsImageLoader> loader;
  mImageLoaders.Get(aTargetFrame, getter_AddRefs(loader));

  if (!loader) {
    loader = new nsImageLoader();
    if (!loader)
      return nsnull;

    loader->Init(aTargetFrame, this);
    mImageLoaders.Put(aTargetFrame, loader);
  }

  loader->Load(aImage);

  imgIRequest *request = loader->GetRequest();

  return request;
}


void
nsPresContext::StopImagesFor(nsIFrame* aTargetFrame)
{
  nsCOMPtr<nsImageLoader> loader;
  mImageLoaders.Get(aTargetFrame, getter_AddRefs(loader));

  if (loader) {
    loader->Destroy();

    mImageLoaders.Remove(aTargetFrame);
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
nsPresContext::GetContainerInternal()
{
  nsISupports *result = nsnull;
  if (mContainer)
    CallQueryReferent(mContainer.get(), &result);

  return result;
}

already_AddRefed<nsISupports>
nsPresContext::GetContainerExternal()
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
nsPresContext::SetBidiEnabled(PRBool aBidiEnabled) const
{
  if (mShell) {
    nsIDocument *doc = mShell->GetDocument();
    if (doc) {
      doc->SetBidiEnabled(aBidiEnabled);
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
nsPresContext::SetBidi(PRUint32 aSource, PRBool aForceReflow)
{
  
  if (aSource == GetBidi()) {
    return;
  }

  NS_ASSERTION(!(aForceReflow && (GetBidi() == 0)), 
               "ForceReflow on new prescontext");

  Document()->SetBidiOptions(aSource);
  if (IBMBIDI_TEXTDIRECTION_RTL == GET_BIDI_OPTION_DIRECTION(aSource)
      || IBMBIDI_NUMERAL_HINDI == GET_BIDI_OPTION_NUMERAL(aSource)) {
    SetBidiEnabled(PR_TRUE);
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
  if (aForceReflow) {
    ClearStyleDataAndReflow();
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

  
  
  
  
  nsPresContext::ClearStyleDataAndReflow();
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

#ifndef MOZ_CAIRO_GFX
  
  
  nsCOMPtr<nsISelectionImageService> imageService;
  nsresult result;
  imageService = do_GetService(kSelectionImageService, &result);
  if (NS_SUCCEEDED(result) && imageService)
  {
    imageService->Reset();
  }
#endif

  
  
  
  
  ClearStyleDataAndReflow();
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
nsPresContext::IsChrome()
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

