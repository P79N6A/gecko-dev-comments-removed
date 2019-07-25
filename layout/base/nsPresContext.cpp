






#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsILinkHandler.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShell.h"
#include "nsIContentViewer.h"
#include "nsPIDOMWindow.h"
#include "nsStyleSet.h"
#include "nsImageLoader.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIURL.h"
#include "nsIDocument.h"
#include "nsStyleContext.h"
#include "mozilla/LookAndFeel.h"
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
#include "nsEventListenerManager.h"
#include "nsStyleStructInlines.h"
#include "nsIAppShell.h"
#include "prenv.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsObjectFrame.h"
#include "nsTransitionManager.h"
#include "nsAnimationManager.h"
#include "mozilla/dom/Element.h"
#include "nsIFrameMessageManager.h"
#include "FrameLayerBuilder.h"
#include "nsDOMMediaQueryList.h"
#include "nsSMILAnimationController.h"

#ifdef IBMBIDI
#include "nsBidiPresUtils.h"
#endif 

#include "nsContentUtils.h"
#include "nsPIWindowRoot.h"
#include "mozilla/Preferences.h"


#include "imgIContainer.h"
#include "nsIImageLoadingContent.h"


#include "nsLayoutCID.h"

#include "nsCSSParser.h"

using namespace mozilla;
using namespace mozilla::dom;

namespace {

class CharSetChangingRunnable : public nsRunnable
{
public:
  CharSetChangingRunnable(nsPresContext* aPresContext,
                          const nsCString& aCharSet)
    : mPresContext(aPresContext),
      mCharSet(aCharSet)
  {
  }

  NS_IMETHOD Run()
  {
    mPresContext->DoChangeCharSet(mCharSet);
    return NS_OK;
  }

private:
  nsRefPtr<nsPresContext> mPresContext;
  nsCString mCharSet;
};

} 

static nscolor
MakeColorPref(const nsString& aColor)
{
  nscolor color;
  nsCSSParser parser;
  nsresult rv =
    parser.ParseColorString(aColor, nsnull, 0, &color);
  if (NS_FAILED(rv)) {
    
    color = NS_RGB(0, 0, 0);
  }
  return color;
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
static bool
IsVisualCharset(const nsCString& aCharset)
{
  if (aCharset.LowerCaseEqualsLiteral("ibm864")             
      || aCharset.LowerCaseEqualsLiteral("ibm862")          
      || aCharset.LowerCaseEqualsLiteral("iso-8859-8") ) {  
    return true; 
  }
  else {
    return false; 
  }
}
#endif 


static PLDHashOperator
destroy_loads(nsIFrame* aKey, nsRefPtr<nsImageLoader>& aData, void* closure)
{
  aData->Destroy();
  return PL_DHASH_NEXT;
}

#include "nsContentCID.h"

  
  

nsPresContext::nsPresContext(nsIDocument* aDocument, nsPresContextType aType)
  : mType(aType), mDocument(aDocument), mMinFontSize(0),
    mTextZoom(1.0), mFullZoom(1.0), mLastFontInflationScreenWidth(-1.0),
    mPageSize(-1, -1), mPPScale(1.0f),
    mViewportStyleOverflow(NS_STYLE_OVERFLOW_AUTO, NS_STYLE_OVERFLOW_AUTO),
    mImageAnimationModePref(imgIContainer::kNormalAnimMode)
{
  
  

  mDoScaledTwips = true;

  SetBackgroundImageDraw(true);		
  SetBackgroundColorDraw(true);

  mBackgroundColor = NS_RGB(0xFF, 0xFF, 0xFF);
  
  mUseDocumentColors = true;
  mUseDocumentFonts = true;

  

  mLinkColor = NS_RGB(0x00, 0x00, 0xEE);
  mActiveLinkColor = NS_RGB(0xEE, 0x00, 0x00);
  mVisitedLinkColor = NS_RGB(0x55, 0x1A, 0x8B);
  mUnderlineLinks = true;
  mSendAfterPaintToContent = false;

  mFocusTextColor = mDefaultColor;
  mFocusBackgroundColor = mBackgroundColor;
  mFocusRingWidth = 1;

  mBodyTextColor = mDefaultColor;

  if (aType == eContext_Galley) {
    mMedium = nsGkAtoms::screen;
  } else {
    mMedium = nsGkAtoms::print;
    mPaginated = true;
  }

  if (!IsDynamic()) {
    mImageAnimationMode = imgIContainer::kDontAnimMode;
    mNeverAnimate = true;
  } else {
    mImageAnimationMode = imgIContainer::kNormalAnimMode;
    mNeverAnimate = false;
  }
  NS_ASSERTION(mDocument, "Null document");
  mUserFontSet = nsnull;
  mUserFontSetDirty = true;

  PR_INIT_CLIST(&mDOMMediaQueryLists);
}

nsPresContext::~nsPresContext()
{
  NS_PRECONDITION(!mShell, "Presshell forgot to clear our mShell pointer");
  SetShell(nsnull);

  NS_ABORT_IF_FALSE(PR_CLIST_IS_EMPTY(&mDOMMediaQueryLists),
                    "must not have media query lists left");

  
  
  if (mRefreshDriver && mRefreshDriver->PresContext() == this) {
    mRefreshDriver->Disconnect();
  }

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

  
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "font.",
                                  this);
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "browser.display.",
                                  this);
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "browser.underline_anchors",
                                  this);
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "browser.anchor_color",
                                  this);
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "browser.active_color",
                                  this);
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "browser.visited_color",
                                  this);
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "image.animation_mode",
                                  this);
#ifdef IBMBIDI
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "bidi.",
                                  this);
#endif 
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "dom.send_after_paint_to_content",
                                  this);
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "gfx.font_rendering.",
                                  this);
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "layout.css.dpi",
                                  this);
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "layout.css.devPixelsPerPx",
                                  this);

  NS_IF_RELEASE(mDeviceContext);
  NS_IF_RELEASE(mLanguage);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsPresContext)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsPresContext)
   NS_INTERFACE_MAP_ENTRY(nsISupports)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsPresContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsPresContext)

static PLDHashOperator
TraverseImageLoader(nsIFrame* aKey, nsRefPtr<nsImageLoader>& aData,
                    void* aClosure)
{
  nsCycleCollectionTraversalCallback *cb =
    static_cast<nsCycleCollectionTraversalCallback*>(aClosure);

  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*cb, "mImageLoaders[i] item");
  cb->NoteXPCOMChild(aData);

  return PL_DHASH_NEXT;
}

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsPresContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocument);
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mEventManager, nsIObserver);
  

  for (PRUint32 i = 0; i < IMAGE_LOAD_TYPE_COUNT; ++i)
    tmp->mImageLoaders[i].Enumerate(TraverseImageLoader, &cb);

  
  
  
  for (PRCList *l = PR_LIST_HEAD(&tmp->mDOMMediaQueryLists);
       l != &tmp->mDOMMediaQueryLists; l = PR_NEXT_LINK(l)) {
    nsDOMMediaQueryList *mql = static_cast<nsDOMMediaQueryList*>(l);
    if (mql->HasListeners()) {
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mDOMMediaQueryLists item");
      cb.NoteXPCOMChild(mql);
    }
  }

  
  
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

  
  
  
  for (PRCList *l = PR_LIST_HEAD(&tmp->mDOMMediaQueryLists);
       l != &tmp->mDOMMediaQueryLists; ) {
    PRCList *next = PR_NEXT_LINK(l);
    nsDOMMediaQueryList *mql = static_cast<nsDOMMediaQueryList*>(l);
    mql->RemoveAllListeners();
    l = next;
  }

  

  
  
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



static bool sNoTheme = false;




static bool sLookAndFeelChanged;




static bool sThemeChanged;

const nsPresContext::LangGroupFontPrefs*
nsPresContext::GetFontPrefsForLang(nsIAtom *aLanguage) const
{
  

  nsresult rv;
  nsIAtom *langGroupAtom = nsnull;
  if (!aLanguage) {
    aLanguage = mLanguage;
  }
  if (aLanguage && mLangService) {
    langGroupAtom = mLangService->GetLanguageGroup(aLanguage, &rv);
  }
  if (NS_FAILED(rv) || !langGroupAtom) {
    langGroupAtom = nsGkAtoms::x_western; 
  }

  
  
  
  
  
  

  LangGroupFontPrefs *prefs =
    const_cast<LangGroupFontPrefs*>(&mLangGroupFontPrefs);
  if (prefs->mLangGroup) { 
    DebugOnly<PRUint32> count = 0;
    for (;;) {
      NS_ASSERTION(++count < 35, "Lang group count exceeded!!!");
      if (prefs->mLangGroup == langGroupAtom) {
        return prefs;
      }
      if (!prefs->mNext) {
        break;
      }
      prefs = prefs->mNext;
    }

    
    prefs = prefs->mNext = new LangGroupFontPrefs;
  }

  prefs->mLangGroup = langGroupAtom;

  















  nsCAutoString langGroup;
  langGroupAtom->ToUTF8String(langGroup);

  prefs->mDefaultVariableFont.size = CSSPixelsToAppUnits(16);
  prefs->mDefaultFixedFont.size = CSSPixelsToAppUnits(13);

  nsCAutoString pref;

  
  enum {eUnit_unknown = -1, eUnit_px, eUnit_pt};
  PRInt32 unit = eUnit_px;

  nsAdoptingCString cvalue =
    Preferences::GetCString("font.size.unit");

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

  

  MAKE_FONT_PREF_KEY(pref, "font.minimum-size.", langGroup);

  PRInt32 size = Preferences::GetInt(pref.get());
  if (unit == eUnit_px) {
    prefs->mMinimumFontSize = CSSPixelsToAppUnits(size);
  }
  else if (unit == eUnit_pt) {
    prefs->mMinimumFontSize = CSSPointsToAppUnits(size);
  }

  nsFont* fontTypes[] = {
    &prefs->mDefaultVariableFont,
    &prefs->mDefaultFixedFont,
    &prefs->mDefaultSerifFont,
    &prefs->mDefaultSansSerifFont,
    &prefs->mDefaultMonospaceFont,
    &prefs->mDefaultCursiveFont,
    &prefs->mDefaultFantasyFont
  };
  MOZ_STATIC_ASSERT(NS_ARRAY_LENGTH(fontTypes) == eDefaultFont_COUNT,
                    "FontTypes array count is not correct");

  
  
  
  
  
  nsCAutoString generic_dot_langGroup;
  for (PRUint32 eType = 0; eType < ArrayLength(fontTypes); ++eType) {
    generic_dot_langGroup.Assign(kGenericFont[eType]);
    generic_dot_langGroup.Append(langGroup);

    nsFont* font = fontTypes[eType];

    
    
    if (eType == eDefaultFont_Variable) {
      MAKE_FONT_PREF_KEY(pref, "font.name.variable.", langGroup);

      nsAdoptingString value = Preferences::GetString(pref.get());
      if (!value.IsEmpty()) {
        prefs->mDefaultVariableFont.name.Assign(value);
      }
      else {
        MAKE_FONT_PREF_KEY(pref, "font.default.", langGroup);
        value = Preferences::GetString(pref.get());
        if (!value.IsEmpty()) {
          prefs->mDefaultVariableFont.name.Assign(value);
        }
      } 
    }
    else {
      if (eType == eDefaultFont_Monospace) {
        
        
        
        
        prefs->mDefaultMonospaceFont.size = prefs->mDefaultFixedFont.size;
      }
      else if (eType != eDefaultFont_Fixed) {
        
        
        font->size = prefs->mDefaultVariableFont.size;
      }
    }

    
    
    
    

    
    
    MAKE_FONT_PREF_KEY(pref, "font.size", generic_dot_langGroup);
    size = Preferences::GetInt(pref.get());
    if (size > 0) {
      if (unit == eUnit_px) {
        font->size = CSSPixelsToAppUnits(size);
      }
      else if (unit == eUnit_pt) {
        font->size = CSSPointsToAppUnits(size);
      }
    }

    
    
    MAKE_FONT_PREF_KEY(pref, "font.size-adjust", generic_dot_langGroup);
    cvalue = Preferences::GetCString(pref.get());
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

  return prefs;
}

void
nsPresContext::GetDocumentColorPreferences()
{
  PRInt32 useAccessibilityTheme = 0;
  bool usePrefColors = true;
  nsCOMPtr<nsIDocShellTreeItem> docShell(do_QueryReferent(mContainer));
  if (docShell) {
    PRInt32 docShellType;
    docShell->GetItemType(&docShellType);
    if (nsIDocShellTreeItem::typeChrome == docShellType) {
      usePrefColors = false;
    }
    else {
      useAccessibilityTheme =
        LookAndFeel::GetInt(LookAndFeel::eIntID_UseAccessibilityTheme, 0);
      usePrefColors = !useAccessibilityTheme;
    }

  }
  if (usePrefColors) {
    usePrefColors =
      !Preferences::GetBool("browser.display.use_system_colors", false);
  }

  if (usePrefColors) {
    nsAdoptingString colorStr =
      Preferences::GetString("browser.display.foreground_color");

    if (!colorStr.IsEmpty()) {
      mDefaultColor = MakeColorPref(colorStr);
    }

    colorStr = Preferences::GetString("browser.display.background_color");

    if (!colorStr.IsEmpty()) {
      mBackgroundColor = MakeColorPref(colorStr);
    }
  }
  else {
    mDefaultColor =
      LookAndFeel::GetColor(LookAndFeel::eColorID_WindowForeground,
                            NS_RGB(0x00, 0x00, 0x00));
    mBackgroundColor =
      LookAndFeel::GetColor(LookAndFeel::eColorID_WindowBackground,
                            NS_RGB(0xFF, 0xFF, 0xFF));
  }

  
  
  mBackgroundColor = NS_ComposeColors(NS_RGB(0xFF, 0xFF, 0xFF),
                                      mBackgroundColor);

  mUseDocumentColors = !useAccessibilityTheme &&
    Preferences::GetBool("browser.display.use_document_colors",
                         mUseDocumentColors);
}

void
nsPresContext::GetUserPreferences()
{
  if (!GetPresShell()) {
    
    
    return;
  }

  mAutoQualityMinFontSizePixelsPref =
    Preferences::GetInt("browser.display.auto_quality_min_font_size");

  
  GetDocumentColorPreferences();

  mSendAfterPaintToContent =
    Preferences::GetBool("dom.send_after_paint_to_content",
                         mSendAfterPaintToContent);

  
  mUnderlineLinks =
    Preferences::GetBool("browser.underline_anchors", mUnderlineLinks);

  nsAdoptingString colorStr = Preferences::GetString("browser.anchor_color");

  if (!colorStr.IsEmpty()) {
    mLinkColor = MakeColorPref(colorStr);
  }

  colorStr = Preferences::GetString("browser.active_color");

  if (!colorStr.IsEmpty()) {
    mActiveLinkColor = MakeColorPref(colorStr);
  }

  colorStr = Preferences::GetString("browser.visited_color");

  if (!colorStr.IsEmpty()) {
    mVisitedLinkColor = MakeColorPref(colorStr);
  }

  mUseFocusColors =
    Preferences::GetBool("browser.display.use_focus_colors", mUseFocusColors);

  mFocusTextColor = mDefaultColor;
  mFocusBackgroundColor = mBackgroundColor;

  colorStr = Preferences::GetString("browser.display.focus_text_color");

  if (!colorStr.IsEmpty()) {
    mFocusTextColor = MakeColorPref(colorStr);
  }

  colorStr = Preferences::GetString("browser.display.focus_background_color");

  if (!colorStr.IsEmpty()) {
    mFocusBackgroundColor = MakeColorPref(colorStr);
  }

  mFocusRingWidth =
    Preferences::GetInt("browser.display.focus_ring_width", mFocusRingWidth);

  mFocusRingOnAnything =
    Preferences::GetBool("browser.display.focus_ring_on_anything",
                         mFocusRingOnAnything);

  mFocusRingStyle =
    Preferences::GetInt("browser.display.focus_ring_style", mFocusRingStyle);

  mBodyTextColor = mDefaultColor;
  
  
  mUseDocumentFonts =
    Preferences::GetInt("browser.display.use_document_fonts") != 0;

  
  mEnableJapaneseTransform =
    Preferences::GetBool("layout.enable_japanese_specific_transform");

  mPrefScrollbarSide = Preferences::GetInt("layout.scrollbar.side");

  ResetCachedFontPrefs();

  
  const nsAdoptingCString& animatePref =
    Preferences::GetCString("image.animation_mode");
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
    Preferences::GetInt(IBMBIDI_TEXTDIRECTION_STR,
                        GET_BIDI_OPTION_DIRECTION(bidiOptions));
  SET_BIDI_OPTION_DIRECTION(bidiOptions, prefInt);
  mPrefBidiDirection = prefInt;

  prefInt =
    Preferences::GetInt(IBMBIDI_TEXTTYPE_STR,
                        GET_BIDI_OPTION_TEXTTYPE(bidiOptions));
  SET_BIDI_OPTION_TEXTTYPE(bidiOptions, prefInt);

  prefInt =
    Preferences::GetInt(IBMBIDI_NUMERAL_STR,
                        GET_BIDI_OPTION_NUMERAL(bidiOptions));
  SET_BIDI_OPTION_NUMERAL(bidiOptions, prefInt);

  prefInt =
    Preferences::GetInt(IBMBIDI_SUPPORTMODE_STR,
                        GET_BIDI_OPTION_SUPPORT(bidiOptions));
  SET_BIDI_OPTION_SUPPORT(bidiOptions, prefInt);

  
  
  
  SetBidi(bidiOptions, false);
}

void
nsPresContext::InvalidateThebesLayers()
{
  if (!mShell)
    return;
  nsIFrame* rootFrame = mShell->FrameManager()->GetRootFrame();
  if (rootFrame) {
    
    
    
    FrameLayerBuilder::InvalidateThebesLayersInSubtree(rootFrame);
  }
}

void
nsPresContext::AppUnitsPerDevPixelChanged()
{
  InvalidateThebesLayers();

  mDeviceContext->FlushFontCache();

  
  if (HasCachedStyleData()) {
    MediaFeatureValuesChanged(true);
    RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
  }
}

void
nsPresContext::PreferenceChanged(const char* aPrefName)
{
  nsDependentCString prefName(aPrefName);
  if (prefName.EqualsLiteral("layout.css.dpi") ||
      prefName.EqualsLiteral("layout.css.devPixelsPerPx")) {
    PRInt32 oldAppUnitsPerDevPixel = AppUnitsPerDevPixel();
    if (mDeviceContext->CheckDPIChange() && mShell) {
      
      
      nscoord oldWidthAppUnits, oldHeightAppUnits;
      nsIViewManager* vm = mShell->GetViewManager();
      vm->GetWindowDimensions(&oldWidthAppUnits, &oldHeightAppUnits);
      float oldWidthDevPixels = oldWidthAppUnits/oldAppUnitsPerDevPixel;
      float oldHeightDevPixels = oldHeightAppUnits/oldAppUnitsPerDevPixel;

      nscoord width = NSToCoordRound(oldWidthDevPixels*AppUnitsPerDevPixel());
      nscoord height = NSToCoordRound(oldHeightDevPixels*AppUnitsPerDevPixel());
      vm->SetWindowDimensions(width, height);

      AppUnitsPerDevPixelChanged();
    }
    return;
  }
  if (StringBeginsWith(prefName, NS_LITERAL_CSTRING("font."))) {
    
    
    

    
    
    
    
    mPrefChangePendingNeedsReflow = true;
  }
  if (StringBeginsWith(prefName, NS_LITERAL_CSTRING("bidi."))) {
    
    mPrefChangePendingNeedsReflow = true;

    
    
  }
  if (StringBeginsWith(prefName, NS_LITERAL_CSTRING("gfx.font_rendering."))) {
    
    mPrefChangePendingNeedsReflow = true;
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
    mShell->SetPreferenceStyleRules(true);
  }

  InvalidateThebesLayers();
  mDeviceContext->FlushFontCache();

  nsChangeHint hint = nsChangeHint(0);

  if (mPrefChangePendingNeedsReflow) {
    NS_UpdateHint(hint, NS_STYLE_HINT_REFLOW);
  }

  RebuildAllStyleData(hint);
}

nsresult
nsPresContext::Init(nsDeviceContext* aDeviceContext)
{
  NS_ASSERTION(!mInitialized, "attempt to reinit pres context");
  NS_ENSURE_ARG(aDeviceContext);

  mDeviceContext = aDeviceContext;
  NS_ADDREF(mDeviceContext);

  if (mDeviceContext->SetPixelScale(mFullZoom))
    mDeviceContext->FlushFontCache();
  mCurAppUnitsPerDevPixel = AppUnitsPerDevPixel();

  for (PRUint32 i = 0; i < IMAGE_LOAD_TYPE_COUNT; ++i) {
    mImageLoaders[i].Init();
  }

  mEventManager = new nsEventStateManager();
  NS_ADDREF(mEventManager);

  mTransitionManager = new nsTransitionManager(this);

  mAnimationManager = new nsAnimationManager(this);

  if (mDocument->GetDisplayDocument()) {
    NS_ASSERTION(mDocument->GetDisplayDocument()->GetShell() &&
                 mDocument->GetDisplayDocument()->GetShell()->GetPresContext(),
                 "Why are we being initialized?");
    mRefreshDriver = mDocument->GetDisplayDocument()->GetShell()->
      GetPresContext()->RefreshDriver();
  } else {
    nsIDocument* parent = mDocument->GetParentDocument();
    
    
    
    
    NS_ASSERTION(!parent || mDocument->IsStaticDocument() || parent->GetShell(),
                 "How did we end up with a presshell if our parent doesn't "
                 "have one?");
    if (parent && parent->GetShell()) {
      NS_ASSERTION(parent->GetShell()->GetPresContext(),
                   "How did we get a presshell?");

      
      nsCOMPtr<nsISupports> ourContainer = mDocument->GetContainer();

      nsCOMPtr<nsIDocShellTreeItem> ourItem = do_QueryInterface(ourContainer);
      if (ourItem) {
        nsCOMPtr<nsIDocShellTreeItem> parentItem;
        ourItem->GetSameTypeParent(getter_AddRefs(parentItem));
        if (parentItem) {
          mRefreshDriver = parent->GetShell()->GetPresContext()->RefreshDriver();
        }
      }
    }

    if (!mRefreshDriver) {
      mRefreshDriver = new nsRefreshDriver(this);
    }
  }

  mLangService = do_GetService(NS_LANGUAGEATOMSERVICE_CONTRACTID);

  
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "font.",
                                this);
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "browser.display.",
                                this);
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "browser.underline_anchors",
                                this);
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "browser.anchor_color",
                                this);
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "browser.active_color",
                                this);
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "browser.visited_color",
                                this);
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "image.animation_mode",
                                this);
#ifdef IBMBIDI
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "bidi.",
                                this);
#endif
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "dom.send_after_paint_to_content",
                                this);
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "gfx.font_rendering.",
                                this);
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "layout.css.dpi",
                                this);
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "layout.css.devPixelsPerPx",
                                this);

  nsresult rv = mEventManager->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mEventManager->SetPresContext(this);

#ifdef DEBUG
  mInitialized = true;
#endif

  mBorderWidthTable[NS_STYLE_BORDER_WIDTH_THIN] = CSSPixelsToAppUnits(1);
  mBorderWidthTable[NS_STYLE_BORDER_WIDTH_MEDIUM] = CSSPixelsToAppUnits(3);
  mBorderWidthTable[NS_STYLE_BORDER_WIDTH_THICK] = CSSPixelsToAppUnits(5);

  return NS_OK;
}



void
nsPresContext::SetShell(nsIPresShell* aShell)
{
  if (mUserFontSet) {
    
    mUserFontSet->Destroy();
    NS_RELEASE(mUserFontSet);
  }

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
        bool isChrome = false;
        bool isRes = false;
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
  } else {
    if (mTransitionManager) {
      mTransitionManager->Disconnect();
      mTransitionManager = nsnull;
    }
    if (mAnimationManager) {
      mAnimationManager->Disconnect();
      mAnimationManager = nsnull;
    }

    if (IsRoot()) {
      
      
      static_cast<nsRootPresContext*>(this)->CancelUpdatePluginGeometryTimer();
    }
  }
}

void
nsPresContext::DestroyImageLoaders()
{
  
  
  
  for (PRUint32 i = 0; i < IMAGE_LOAD_TYPE_COUNT; ++i) {
    mImageLoaders[i].Enumerate(destroy_loads, nsnull);
    mImageLoaders[i].Clear();
  }
}

void
nsPresContext::DoChangeCharSet(const nsCString& aCharSet)
{
  UpdateCharSet(aCharSet);
  mDeviceContext->FlushFontCache();
  RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
}

void
nsPresContext::UpdateCharSet(const nsCString& aCharSet)
{
  if (mLangService) {
    NS_IF_RELEASE(mLanguage);
    mLanguage = mLangService->LookupCharSet(aCharSet.get()).get();  
    

    
    if (mLanguage == nsGkAtoms::Unicode) {
      NS_RELEASE(mLanguage);
      NS_IF_ADDREF(mLanguage = mLangService->GetLocaleLanguage()); 
    }
    ResetCachedFontPrefs();
  }
#ifdef IBMBIDI
  

  switch (GET_BIDI_OPTION_TEXTTYPE(GetBidi())) {

    case IBMBIDI_TEXTTYPE_LOGICAL:
      SetVisualMode(false);
      break;

    case IBMBIDI_TEXTTYPE_VISUAL:
      SetVisualMode(true);
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
    nsRefPtr<CharSetChangingRunnable> runnable =
      new CharSetChangingRunnable(this, NS_LossyConvertUTF16toASCII(aData));
    return NS_DispatchToCurrentThread(runnable);
  }

  NS_WARNING("unrecognized topic in nsPresContext::Observe");
  return NS_ERROR_FAILURE;
}

static nsPresContext*
GetParentPresContext(nsPresContext* aPresContext)
{
  nsIPresShell* shell = aPresContext->GetPresShell();
  if (shell) {
    nsIFrame* rootFrame = shell->FrameManager()->GetRootFrame();
    if (rootFrame) {
      nsIFrame* f = nsLayoutUtils::GetCrossDocParentFrame(rootFrame);
      if (f)
        return f->PresContext();
    }
  }
  return nsnull;
}


nsRootPresContext*
nsPresContext::GetRootPresContext()
{
  nsPresContext* pc = this;
  for (;;) {
    nsPresContext* parent = GetParentPresContext(pc);
    if (!parent)
      break;
    pc = parent;
  }
  return pc->IsRoot() ? static_cast<nsRootPresContext*>(pc) : nsnull;
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
set_animation_mode(nsIFrame* aKey, nsRefPtr<nsImageLoader>& aData, void* closure)
{
  for (nsImageLoader *loader = aData; loader;
       loader = loader->GetNextLoader()) {
    imgIRequest* imgReq = loader->GetRequest();
    SetImgAnimModeOnImgReq(imgReq, (PRUint16)NS_PTR_TO_INT32(closure));
  }
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
nsPresContext::SetSMILAnimations(nsIDocument *aDoc, PRUint16 aNewMode,
                                 PRUint16 aOldMode)
{
  if (aDoc->HasAnimationController()) {
    nsSMILAnimationController* controller = aDoc->GetAnimationController();
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

void
nsPresContext::SetImageAnimationModeInternal(PRUint16 aMode)
{
  NS_ASSERTION(aMode == imgIContainer::kNormalAnimMode ||
               aMode == imgIContainer::kDontAnimMode ||
               aMode == imgIContainer::kLoopOnceAnimMode, "Wrong Animation Mode is being set!");

  
  if (!IsDynamic())
    return;

  
  for (PRUint32 i = 0; i < IMAGE_LOAD_TYPE_COUNT; ++i)
    mImageLoaders[i].Enumerate(set_animation_mode, NS_INT32_TO_PTR(aMode));

  
  
  if (mShell != nsnull) {
    nsIDocument *doc = mShell->GetDocument();
    if (doc) {
      Element *rootElement = doc->GetRootElement();
      if (rootElement) {
        SetImgAnimations(rootElement, aMode);
      }
      SetSMILAnimations(doc, aMode, mImageAnimationMode);
    }
  }

  mImageAnimationMode = aMode;
}

void
nsPresContext::SetImageAnimationModeExternal(PRUint16 aMode)
{
  SetImageAnimationModeInternal(aMode);
}

const nsFont*
nsPresContext::GetDefaultFont(PRUint8 aFontID, nsIAtom *aLanguage) const
{
  const LangGroupFontPrefs *prefs = GetFontPrefsForLang(aLanguage);

  const nsFont *font;
  switch (aFontID) {
    
    case kPresContext_DefaultVariableFont_ID:
      font = &prefs->mDefaultVariableFont;
      break;
    case kPresContext_DefaultFixedFont_ID:
      font = &prefs->mDefaultFixedFont;
      break;
    
    case kGenericFont_serif:
      font = &prefs->mDefaultSerifFont;
      break;
    case kGenericFont_sans_serif:
      font = &prefs->mDefaultSansSerifFont;
      break;
    case kGenericFont_monospace:
      font = &prefs->mDefaultMonospaceFont;
      break;
    case kGenericFont_cursive:
      font = &prefs->mDefaultCursiveFont;
      break;
    case kGenericFont_fantasy: 
      font = &prefs->mDefaultFantasyFont;
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
  mShell->GetViewManager()->GetWindowDimensions(&oldWidthAppUnits, &oldHeightAppUnits);
  float oldWidthDevPixels = oldWidthAppUnits / float(mCurAppUnitsPerDevPixel);
  float oldHeightDevPixels = oldHeightAppUnits / float(mCurAppUnitsPerDevPixel);
  mDeviceContext->SetPixelScale(aZoom);

  NS_ASSERTION(!mSupressResizeReflow, "two zooms happening at the same time? impossible!");
  mSupressResizeReflow = true;

  mFullZoom = aZoom;
  mShell->GetViewManager()->
    SetWindowDimensions(NSToCoordRound(oldWidthDevPixels * AppUnitsPerDevPixel()),
                        NSToCoordRound(oldHeightDevPixels * AppUnitsPerDevPixel()));

  AppUnitsPerDevPixelChanged();

  mSupressResizeReflow = false;

  mCurAppUnitsPerDevPixel = AppUnitsPerDevPixel();
}

float
nsPresContext::ScreenWidthInchesForFontInflation(bool* aChanged)
{
  if (aChanged) {
    *aChanged = false;
  }

  nsDeviceContext *dx = DeviceContext();
  nsRect clientRect;
  dx->GetClientRect(clientRect); 
  float deviceWidthInches =
    float(clientRect.width) / float(dx->AppUnitsPerPhysicalInch());

  if (deviceWidthInches != mLastFontInflationScreenWidth) {
    if (mLastFontInflationScreenWidth != -1.0) {
      if (aChanged) {
        *aChanged = true;
      } else {
        NS_NOTREACHED("somebody should have checked for screen width change "
                      "and triggered a reflow");
      }
    }
    mLastFontInflationScreenWidth = deviceWidthInches;
  }

  return deviceWidthInches;
}

void
nsPresContext::SetImageLoaders(nsIFrame* aTargetFrame,
                               ImageLoadType aType,
                               nsImageLoader* aImageLoaders)
{
  NS_ASSERTION(mShell || !aImageLoaders,
               "Shouldn't add new image loader after the shell is gone");

  nsRefPtr<nsImageLoader> oldLoaders;
  mImageLoaders[aType].Get(aTargetFrame, getter_AddRefs(oldLoaders));

  if (aImageLoaders) {
    mImageLoaders[aType].Put(aTargetFrame, aImageLoaders);
  } else if (oldLoaders) {
    mImageLoaders[aType].Remove(aTargetFrame);
  }

  if (oldLoaders)
    oldLoaders->Destroy();
}

void
nsPresContext::SetupBackgroundImageLoaders(nsIFrame* aFrame,
                                     const nsStyleBackground* aStyleBackground)
{
  nsRefPtr<nsImageLoader> loaders;
  NS_FOR_VISIBLE_BACKGROUND_LAYERS_BACK_TO_FRONT(i, aStyleBackground) {
    if (aStyleBackground->mLayers[i].mImage.GetType() == eStyleImageType_Image) {
      PRUint32 actions = nsImageLoader::ACTION_REDRAW_ON_DECODE;
      imgIRequest *image = aStyleBackground->mLayers[i].mImage.GetImageData();
      loaders = nsImageLoader::Create(aFrame, image, actions, loaders);
    }
  }
  SetImageLoaders(aFrame, BACKGROUND_IMAGE, loaders);
}

void
nsPresContext::SetupBorderImageLoaders(nsIFrame* aFrame,
                                       const nsStyleBorder* aStyleBorder)
{
  
  
  
  imgIRequest *borderImage = aStyleBorder->GetBorderImage();
  if (!borderImage) {
    SetImageLoaders(aFrame, BORDER_IMAGE, nsnull);
    return;
  }

  PRUint32 actions = nsImageLoader::ACTION_REDRAW_ON_LOAD;
  nsRefPtr<nsImageLoader> loader =
    nsImageLoader::Create(aFrame, borderImage, actions, nsnull);
  SetImageLoaders(aFrame, BORDER_IMAGE, loader);
}

void
nsPresContext::StopImagesFor(nsIFrame* aTargetFrame)
{
  for (PRUint32 i = 0; i < IMAGE_LOAD_TYPE_COUNT; ++i)
    SetImageLoaders(aTargetFrame, ImageLoadType(i), nsnull);
}

void
nsPresContext::SetContainer(nsISupports* aHandler)
{
  mContainer = do_GetWeakReference(aHandler);
  InvalidateIsChromeCache();
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

void
nsPresContext::SetBidi(PRUint32 aSource, bool aForceRestyle)
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
    SetVisualMode(true);
  }
  else if (IBMBIDI_TEXTTYPE_LOGICAL == GET_BIDI_OPTION_TEXTTYPE(aSource)) {
    SetVisualMode(false);
  }
  else {
    nsIDocument* doc = mShell->GetDocument();
    if (doc) {
      SetVisualMode(IsVisualCharset(doc->GetDocumentCharacterSet()));
    }
  }
  if (aForceRestyle && mShell) {
    
    
    RebuildUserFontSet();
    mShell->ReconstructFrames();
  }
}

PRUint32
nsPresContext::GetBidi() const
{
  return Document()->GetBidiOptions();
}

#endif 

bool
nsPresContext::IsTopLevelWindowInactive()
{
  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_QueryReferent(mContainer));
  if (!treeItem)
    return false;

  nsCOMPtr<nsIDocShellTreeItem> rootItem;
  treeItem->GetRootTreeItem(getter_AddRefs(rootItem));
  nsCOMPtr<nsPIDOMWindow> domWindow(do_GetInterface(rootItem));

  return domWindow && !domWindow->IsActive();
}

nsITheme*
nsPresContext::GetTheme()
{
  if (!sNoTheme && !mTheme) {
    mTheme = do_GetService("@mozilla.org/chrome/chrome-native-theme;1");
    if (!mTheme)
      sNoTheme = true;
  }

  return mTheme;
}

void
nsPresContext::ThemeChanged()
{
  if (!mPendingThemeChanged) {
    sLookAndFeelChanged = true;
    sThemeChanged = true;

    nsCOMPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(this, &nsPresContext::ThemeChangedInternal);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mPendingThemeChanged = true;
    }
  }    
}

void
nsPresContext::ThemeChangedInternal()
{
  mPendingThemeChanged = false;
  
  
  
  if (mTheme && sThemeChanged) {
    mTheme->ThemeChanged();
    sThemeChanged = false;
  }

  
  if (sLookAndFeelChanged) {
    LookAndFeel::Refresh();
    sLookAndFeelChanged = false;
  }

  
  nsCSSRuleProcessor::FreeSystemMetrics();

  
  MediaFeatureValuesChanged(true);

  
  
  
  
  RebuildAllStyleData(NS_STYLE_HINT_REFLOW);
}

void
nsPresContext::SysColorChanged()
{
  if (!mPendingSysColorChanged) {
    sLookAndFeelChanged = true;
    nsCOMPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(this, &nsPresContext::SysColorChangedInternal);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mPendingSysColorChanged = true;
    }
  }
}

void
nsPresContext::SysColorChangedInternal()
{
  mPendingSysColorChanged = false;
  
  if (sLookAndFeelChanged) {
     
    LookAndFeel::Refresh();
    sLookAndFeelChanged = false;
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
  AnimationManager()->KeyframesListIsDirty();

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
nsPresContext::MediaFeatureValuesChanged(bool aCallerWillRebuildStyleData)
{
  mPendingMediaFeatureValuesChanged = false;
  if (mShell &&
      mShell->StyleSet()->MediumFeaturesChanged(this) &&
      !aCallerWillRebuildStyleData) {
    RebuildAllStyleData(nsChangeHint(0));
  }

  if (!nsContentUtils::IsSafeToRunScript()) {
    NS_ABORT_IF_FALSE(mDocument->IsBeingUsedAsImage(),
                      "How did we get here?  Are we failing to notify "
                      "listeners that we should notify?");
    return;
  }

  
  
  
  
  
  

  if (!PR_CLIST_IS_EMPTY(&mDOMMediaQueryLists)) {
    
    
    
    
    
    
    
    
    
    
    
    
    nsDOMMediaQueryList::NotifyList notifyList;
    for (PRCList *l = PR_LIST_HEAD(&mDOMMediaQueryLists);
         l != &mDOMMediaQueryLists; l = PR_NEXT_LINK(l)) {
      nsDOMMediaQueryList *mql = static_cast<nsDOMMediaQueryList*>(l);
      mql->MediumFeaturesChanged(notifyList);
    }

    if (!notifyList.IsEmpty()) {
      nsPIDOMWindow *win = mDocument->GetInnerWindow();
      nsCOMPtr<nsIDOMEventTarget> et = do_QueryInterface(win);
      nsCxPusher pusher;

      for (PRUint32 i = 0, i_end = notifyList.Length(); i != i_end; ++i) {
        if (pusher.RePush(et)) {
          nsAutoMicroTask mt;
          nsDOMMediaQueryList::HandleChangeData &d = notifyList[i];
          d.listener->HandleChange(d.mql);
        }
      }
    }

    
  }
}

void
nsPresContext::PostMediaFeatureValuesChangedEvent()
{
  
  
  
  if (!mPendingMediaFeatureValuesChanged) {
    nsCOMPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(this, &nsPresContext::HandleMediaFeatureValuesChangedEvent);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mPendingMediaFeatureValuesChanged = true;
      mDocument->SetNeedStyleFlush();
    }
  }
}

void
nsPresContext::HandleMediaFeatureValuesChangedEvent()
{
  
  
  if (mPendingMediaFeatureValuesChanged && mShell) {
    MediaFeatureValuesChanged(false);
  }
}

void
nsPresContext::MatchMedia(const nsAString& aMediaQueryList,
                          nsIDOMMediaQueryList** aResult)
{
  nsRefPtr<nsDOMMediaQueryList> result =
    new nsDOMMediaQueryList(this, aMediaQueryList);

  
  PR_INSERT_BEFORE(result, &mDOMMediaQueryLists);

  result.forget(aResult);
}

void
nsPresContext::SetPaginatedScrolling(bool aPaginated)
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

bool
nsPresContext::EnsureVisible()
{
  nsCOMPtr<nsIDocShell> docShell(do_QueryReferent(mContainer));
  if (docShell) {
    nsCOMPtr<nsIContentViewer> cv;
    docShell->GetContentViewer(getter_AddRefs(cv));
    
    if (cv) {
      nsRefPtr<nsPresContext> currentPresContext;
      cv->GetPresContext(getter_AddRefs(currentPresContext));
      if (currentPresContext == this) {
        
        cv->Show();
        return true;
      }
    }
  }
  return false;
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

bool
nsPresContext::IsChromeSlow() const
{
  bool isChrome = false;
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
  mIsChrome = isChrome;
  mIsChromeIsCached = true;
  return mIsChrome;
}

void
nsPresContext::InvalidateIsChromeCacheExternal()
{
  InvalidateIsChromeCacheInternal();
}

 bool
nsPresContext::HasAuthorSpecifiedRules(nsIFrame *aFrame, PRUint32 ruleTypeMask) const
{
  return
    nsRuleNode::HasAuthorSpecifiedRules(aFrame->GetStyleContext(),
                                        ruleTypeMask,
                                        UseDocumentColors());
}

gfxUserFontSet*
nsPresContext::GetUserFontSetInternal()
{
  
  
  
  
  
  
  
#ifdef DEBUG
  bool userFontSetGottenBefore = mGetUserFontSetCalled;
#endif
  
  
  mGetUserFontSetCalled = true;
  if (mUserFontSetDirty) {
    
    
    
    
    
    
    NS_ASSERTION(!userFontSetGottenBefore || !mShell->IsReflowLocked(),
                 "FlushUserFontSet should have been called first");
    FlushUserFontSet();
  }

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
  if (!mShell) {
    return; 
  }

  if (!mGetUserFontSetCalled) {
    return; 
            
            
  }

  if (mUserFontSetDirty) {
    if (gfxPlatform::GetPlatform()->DownloadableFontsEnabled()) {
      nsTArray<nsFontFaceRuleContainer> rules;
      if (!mShell->StyleSet()->AppendFontFaceRules(this, rules)) {
        if (mUserFontSet) {
          mUserFontSet->Destroy();
          NS_RELEASE(mUserFontSet);
        }
        return;
      }

      bool changed = false;

      if (rules.Length() == 0) {
        if (mUserFontSet) {
          mUserFontSet->Destroy();
          NS_RELEASE(mUserFontSet);
          changed = true;
        }
      } else {
        if (!mUserFontSet) {
          mUserFontSet = new nsUserFontSet(this);
          NS_ADDREF(mUserFontSet);
        }
        changed = mUserFontSet->UpdateRules(rules);
      }

      
      
      
      
      if (changed) {
        UserFontSetUpdated();
      }
    }

    mUserFontSetDirty = false;
  }
}

void
nsPresContext::RebuildUserFontSet()
{
  if (!mGetUserFontSetCalled) {
    
    
    
    return;
  }

  mUserFontSetDirty = true;
  mDocument->SetNeedStyleFlush();

  
  
  
  
  
  
  
  if (!mPostedFlushUserFontSet) {
    nsCOMPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(this, &nsPresContext::HandleRebuildUserFontSet);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mPostedFlushUserFontSet = true;
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

bool
nsPresContext::EnsureSafeToHandOutCSSRules()
{
  nsCSSStyleSheet::EnsureUniqueInnerResult res =
    mShell->StyleSet()->EnsureUniqueInnerOnCSSSheets();
  if (res == nsCSSStyleSheet::eUniqueInner_AlreadyUnique) {
    
    return true;
  }
  if (res == nsCSSStyleSheet::eUniqueInner_CloneFailed) {
    return false;
  }

  NS_ABORT_IF_FALSE(res == nsCSSStyleSheet::eUniqueInner_ClonedInner,
                    "unexpected result");
  RebuildAllStyleData(nsChangeHint(0));
  return true;
}

void
nsPresContext::FireDOMPaintEvent()
{
  nsPIDOMWindow* ourWindow = mDocument->GetWindow();
  if (!ourWindow)
    return;

  nsCOMPtr<nsIDOMEventTarget> dispatchTarget = do_QueryInterface(ourWindow);
  nsCOMPtr<nsIDOMEventTarget> eventTarget = dispatchTarget;
  if (!IsChrome()) {
    bool notifyContent = mSendAfterPaintToContent;

    if (notifyContent) {
      
      
      notifyContent = false;
      for (PRUint32 i = 0; i < mInvalidateRequests.mRequests.Length(); ++i) {
        if (!(mInvalidateRequests.mRequests[i].mFlags &
              nsIFrame::INVALIDATE_CROSS_DOC)) {
          notifyContent = true;
        }
      }
    }
    if (!notifyContent) {
      
      
      
      
      dispatchTarget = do_QueryInterface(ourWindow->GetParentTarget());
      if (!dispatchTarget) {
        return;
      }
    }
  }
  
  
  nsCOMPtr<nsIDOMEvent> event;
  
  
  
  NS_NewDOMNotifyPaintEvent(getter_AddRefs(event), this, nsnull,
                            NS_AFTERPAINT,
                            &mInvalidateRequests);
  nsCOMPtr<nsIPrivateDOMEvent> pEvent = do_QueryInterface(event);
  if (!pEvent) return;

  
  
  
  pEvent->SetTarget(eventTarget);
  pEvent->SetTrusted(true);
  nsEventDispatcher::DispatchDOMEvent(dispatchTarget, nsnull, event, this, nsnull);
}

static bool
MayHavePaintEventListener(nsPIDOMWindow* aInnerWindow)
{
  if (!aInnerWindow)
    return false;
  if (aInnerWindow->HasPaintEventListeners())
    return true;

  nsIDOMEventTarget* parentTarget = aInnerWindow->GetParentTarget();
  if (!parentTarget)
    return false;

  nsEventListenerManager* manager = nsnull;
  if ((manager = parentTarget->GetListenerManager(false)) &&
      manager->MayHavePaintEventListener()) {
    return true;
  }

  nsCOMPtr<nsINode> node;
  if (parentTarget != aInnerWindow->GetChromeEventHandler()) {
    nsCOMPtr<nsIInProcessContentFrameMessageManager> mm =
      do_QueryInterface(parentTarget);
    if (mm) {
      node = mm->GetOwnerContent();
    }
  }

  if (!node) {
    node = do_QueryInterface(parentTarget);
  }
  if (node)
    return MayHavePaintEventListener(node->OwnerDoc()->GetInnerWindow());

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(parentTarget);
  if (window)
    return MayHavePaintEventListener(window);

  nsCOMPtr<nsPIWindowRoot> root = do_QueryInterface(parentTarget);
  nsIDOMEventTarget* tabChildGlobal;
  return root &&
         (tabChildGlobal = root->GetParentTarget()) &&
         (manager = tabChildGlobal->GetListenerManager(false)) &&
         manager->MayHavePaintEventListener();
}

bool
nsPresContext::MayHavePaintEventListener()
{
  return ::MayHavePaintEventListener(mDocument->GetInnerWindow());
}

void
nsPresContext::NotifyInvalidation(const nsRect& aRect, PRUint32 aFlags)
{
  
  
  
  
  
  if (aRect.IsEmpty() || !MayHavePaintEventListener())
    return;

  nsPresContext* pc;
  for (pc = this; pc; pc = GetParentPresContext(pc)) {
    if (pc->mFireAfterPaintEvents)
      break;
    pc->mFireAfterPaintEvents = true;
  }
  if (!pc) {
    nsRootPresContext* rpc = GetRootPresContext();
    if (rpc) {
      rpc->EnsureEventualDidPaintEvent();
    }
  }

  nsInvalidateRequestList::Request* request =
    mInvalidateRequests.mRequests.AppendElement();
  if (!request)
    return;

  request->mRect = aRect;
  request->mFlags = aFlags;
}

static bool
NotifyDidPaintSubdocumentCallback(nsIDocument* aDocument, void* aData)
{
  nsIPresShell* shell = aDocument->GetShell();
  if (shell) {
    nsPresContext* pc = shell->GetPresContext();
    if (pc) {
      pc->NotifyDidPaintForSubtree();
    }
  }
  return true;
}

void
nsPresContext::NotifyDidPaintForSubtree()
{
  if (!mFireAfterPaintEvents)
    return;
  mFireAfterPaintEvents = false;

  if (IsRoot()) {
    static_cast<nsRootPresContext*>(this)->CancelDidPaintTimer();
  }

  if (!mInvalidateRequests.mRequests.IsEmpty()) {
    nsCOMPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(this, &nsPresContext::FireDOMPaintEvent);
    nsContentUtils::AddScriptRunner(ev);
  }

  mDocument->EnumerateSubDocuments(NotifyDidPaintSubdocumentCallback, nsnull);
}

bool
nsPresContext::HasCachedStyleData()
{
  return mShell && mShell->StyleSet()->HasCachedStyleData();
}

static bool sGotInterruptEnv = false;
enum InterruptMode {
  ModeRandom,
  ModeCounter,
  ModeEvent
};



static InterruptMode sInterruptMode = ModeEvent;


static PRUint32 sInterruptSeed = 1;



static PRUint32 sInterruptMaxCounter = 10;


static PRUint32 sInterruptCounter;


static PRUint32 sInterruptChecksToSkip = 200;




static TimeDuration sInterruptTimeout;

static void GetInterruptEnv()
{
  char *ev = PR_GetEnv("GECKO_REFLOW_INTERRUPT_MODE");
  if (ev) {
#ifndef XP_WIN
    if (PL_strcasecmp(ev, "random") == 0) {
      ev = PR_GetEnv("GECKO_REFLOW_INTERRUPT_SEED");
      if (ev) {
        sInterruptSeed = atoi(ev);
      }
      srandom(sInterruptSeed);
      sInterruptMode = ModeRandom;
    } else
#endif
      if (PL_strcasecmp(ev, "counter") == 0) {
      ev = PR_GetEnv("GECKO_REFLOW_INTERRUPT_FREQUENCY");
      if (ev) {
        sInterruptMaxCounter = atoi(ev);
      }
      sInterruptCounter = 0;
      sInterruptMode = ModeCounter;
    }
  }
  ev = PR_GetEnv("GECKO_REFLOW_INTERRUPT_CHECKS_TO_SKIP");
  if (ev) {
    sInterruptChecksToSkip = atoi(ev);
  }

  ev = PR_GetEnv("GECKO_REFLOW_MIN_NOINTERRUPT_DURATION");
  int duration_ms = ev ? atoi(ev) : 100;
  sInterruptTimeout = TimeDuration::FromMilliseconds(duration_ms);
}

bool
nsPresContext::HavePendingInputEvent()
{
  switch (sInterruptMode) {
#ifndef XP_WIN
    case ModeRandom:
      return (random() & 1);
#endif
    case ModeCounter:
      if (sInterruptCounter < sInterruptMaxCounter) {
        ++sInterruptCounter;
        return false;
      }
      sInterruptCounter = 0;
      return true;
    default:
    case ModeEvent: {
      nsIFrame* f = PresShell()->GetRootFrame();
      if (f) {
        nsIWidget* w = f->GetNearestWidget();
        if (w) {
          return w->HasPendingInputEvent();
        }
      }
      return false;
    }
  }
}

void
nsPresContext::ReflowStarted(bool aInterruptible)
{
#ifdef NOISY_INTERRUPTIBLE_REFLOW
  if (!aInterruptible) {
    printf("STARTING NONINTERRUPTIBLE REFLOW\n");
  }
#endif
  
  
  mInterruptsEnabled = aInterruptible && !IsPaginated();

  
  
  
  
  
  
  mHasPendingInterrupt = false;

  mInterruptChecksToSkip = sInterruptChecksToSkip;

  if (mInterruptsEnabled) {
    mReflowStartTime = TimeStamp::Now();
  }
}

bool
nsPresContext::CheckForInterrupt(nsIFrame* aFrame)
{
  if (mHasPendingInterrupt) {
    mShell->FrameNeedsToContinueReflow(aFrame);
    return true;
  }

  if (!sGotInterruptEnv) {
    sGotInterruptEnv = true;
    GetInterruptEnv();
  }

  if (!mInterruptsEnabled) {
    return false;
  }

  if (mInterruptChecksToSkip > 0) {
    --mInterruptChecksToSkip;
    return false;
  }
  mInterruptChecksToSkip = sInterruptChecksToSkip;

  
  
  mHasPendingInterrupt =
    TimeStamp::Now() - mReflowStartTime > sInterruptTimeout &&
    HavePendingInputEvent() &&
    !IsChrome();
  if (mHasPendingInterrupt) {
#ifdef NOISY_INTERRUPTIBLE_REFLOW
    printf("*** DETECTED pending interrupt (time=%lld)\n", PR_Now());
#endif 
    mShell->FrameNeedsToContinueReflow(aFrame);
  }
  return mHasPendingInterrupt;
}

size_t
nsPresContext::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return mPropertyTable.SizeOfExcludingThis(aMallocSizeOf);
         mLangGroupFontPrefs.SizeOfExcludingThis(aMallocSizeOf);

  
  
}

bool
nsPresContext::IsRootContentDocument()
{
  
  
  if (mDocument->IsResourceDoc()) {
    return false;
  }
  if (IsChrome()) {
    return false;
  }
  
  nsIView* view = PresShell()->GetViewManager()->GetRootView();
  if (!view) {
    return false;
  }
  view = view->GetParent(); 
  if (!view) {
    return true;
  }
  view = view->GetParent(); 
  if (!view) {
    return true;
  }

  nsIFrame* f = view->GetFrame();
  return (f && f->PresContext()->IsChrome());
}

nsRootPresContext::nsRootPresContext(nsIDocument* aDocument,
                                     nsPresContextType aType)
  : nsPresContext(aDocument, aType),
    mUpdatePluginGeometryForFrame(nsnull),
    mDOMGeneration(0),
    mNeedsToUpdatePluginGeometry(false)
{
  mRegisteredPlugins.Init();
}

nsRootPresContext::~nsRootPresContext()
{
  NS_ASSERTION(mRegisteredPlugins.Count() == 0,
               "All plugins should have been unregistered");
  CancelDidPaintTimer();
  CancelUpdatePluginGeometryTimer();
}

void
nsRootPresContext::RegisterPluginForGeometryUpdates(nsObjectFrame* aPlugin)
{
  mRegisteredPlugins.PutEntry(aPlugin);
}

void
nsRootPresContext::UnregisterPluginForGeometryUpdates(nsObjectFrame* aPlugin)
{
  mRegisteredPlugins.RemoveEntry(aPlugin);
}

struct PluginGeometryClosure {
  nsIFrame* mRootFrame;
  PRInt32   mRootAPD;
  nsIFrame* mChangedSubtree;
  nsRect    mChangedRect;
  nsTHashtable<nsPtrHashKey<nsObjectFrame> > mAffectedPlugins;
  nsRect    mAffectedPluginBounds;
  nsTArray<nsIWidget::Configuration>* mOutputConfigurations;
};
static PLDHashOperator
PluginBoundsEnumerator(nsPtrHashKey<nsObjectFrame>* aEntry, void* userArg)
{
  PluginGeometryClosure* closure = static_cast<PluginGeometryClosure*>(userArg);
  nsObjectFrame* f = aEntry->GetKey();
  nsRect fBounds = f->GetContentRect() +
      f->GetParent()->GetOffsetToCrossDoc(closure->mRootFrame);
  PRInt32 APD = f->PresContext()->AppUnitsPerDevPixel();
  fBounds = fBounds.ConvertAppUnitsRoundOut(APD, closure->mRootAPD);
  
  
  
  
  
  
  
  if (fBounds.Intersects(closure->mChangedRect) ||
      nsLayoutUtils::IsAncestorFrameCrossDoc(closure->mChangedSubtree, f)) {
    closure->mAffectedPluginBounds.UnionRect(
        closure->mAffectedPluginBounds, fBounds);
    closure->mAffectedPlugins.PutEntry(f);
  }
  return PL_DHASH_NEXT;
}

static PLDHashOperator
PluginHideEnumerator(nsPtrHashKey<nsObjectFrame>* aEntry, void* userArg)
{
  PluginGeometryClosure* closure = static_cast<PluginGeometryClosure*>(userArg);
  nsObjectFrame* f = aEntry->GetKey();
  f->GetEmptyClipConfiguration(closure->mOutputConfigurations);
  return PL_DHASH_NEXT;
}

static void
RecoverPluginGeometry(nsDisplayListBuilder* aBuilder,
    nsDisplayList* aList, bool aInTransform, PluginGeometryClosure* aClosure)
{
  for (nsDisplayItem* i = aList->GetBottom(); i; i = i->GetAbove()) {
    switch (i->GetType()) {
    case nsDisplayItem::TYPE_PLUGIN: {
      nsDisplayPlugin* displayPlugin = static_cast<nsDisplayPlugin*>(i);
      nsObjectFrame* f = static_cast<nsObjectFrame*>(
          displayPlugin->GetUnderlyingFrame());
      
      
      
      
      nsPtrHashKey<nsObjectFrame>* entry =
        aClosure->mAffectedPlugins.GetEntry(f);
      
      
      if (entry && (!aInTransform || f->PaintedByGecko())) {
        displayPlugin->GetWidgetConfiguration(aBuilder,
                                              aClosure->mOutputConfigurations);
        
        aClosure->mAffectedPlugins.RawRemoveEntry(entry);
      }
      break;
    }
    case nsDisplayItem::TYPE_TRANSFORM: {
      nsDisplayList* sublist =
          static_cast<nsDisplayTransform*>(i)->GetStoredList()->GetList();
      RecoverPluginGeometry(aBuilder, sublist, true, aClosure);
      break;
    }
    default: {
      nsDisplayList* sublist = i->GetList();
      if (sublist) {
        RecoverPluginGeometry(aBuilder, sublist, aInTransform, aClosure);
      }
      break;
    }
    }
  }
}

#ifdef DEBUG
#include <stdio.h>

static bool gDumpPluginList = false;
#endif

void
nsRootPresContext::GetPluginGeometryUpdates(nsIFrame* aChangedSubtree,
                                            nsTArray<nsIWidget::Configuration>* aConfigurations)
{
  if (mRegisteredPlugins.Count() == 0)
    return;

  PluginGeometryClosure closure;
  closure.mRootFrame = mShell->FrameManager()->GetRootFrame();
  closure.mRootAPD = closure.mRootFrame->PresContext()->AppUnitsPerDevPixel();
  closure.mChangedSubtree = aChangedSubtree;
  closure.mChangedRect = aChangedSubtree->GetVisualOverflowRect() +
      aChangedSubtree->GetOffsetToCrossDoc(closure.mRootFrame);
  PRInt32 subtreeAPD = aChangedSubtree->PresContext()->AppUnitsPerDevPixel();
  closure.mChangedRect =
    closure.mChangedRect.ConvertAppUnitsRoundOut(subtreeAPD, closure.mRootAPD);
  closure.mAffectedPlugins.Init();
  closure.mOutputConfigurations = aConfigurations;
  
  mRegisteredPlugins.EnumerateEntries(PluginBoundsEnumerator, &closure);

  nsRect bounds;
  if (bounds.IntersectRect(closure.mAffectedPluginBounds,
                           closure.mRootFrame->GetRect())) {
    nsDisplayListBuilder builder(closure.mRootFrame,
    		nsDisplayListBuilder::PLUGIN_GEOMETRY, false);
    builder.SetAccurateVisibleRegions();
    nsDisplayList list;

    builder.EnterPresShell(closure.mRootFrame, bounds);
    closure.mRootFrame->BuildDisplayListForStackingContext(
        &builder, bounds, &list);
    builder.LeavePresShell(closure.mRootFrame, bounds);

#ifdef DEBUG
    if (gDumpPluginList) {
      fprintf(stderr, "Plugins --- before optimization (bounds %d,%d,%d,%d):\n",
          bounds.x, bounds.y, bounds.width, bounds.height);
      nsFrame::PrintDisplayList(&builder, list);
    }
#endif

    nsRegion visibleRegion(bounds);
    list.ComputeVisibilityForRoot(&builder, &visibleRegion);

#ifdef DEBUG
    if (gDumpPluginList) {
      fprintf(stderr, "Plugins --- after optimization:\n");
      nsFrame::PrintDisplayList(&builder, list);
    }
#endif

    RecoverPluginGeometry(&builder, &list, false, &closure);
    list.DeleteAll();
  }

  
  closure.mAffectedPlugins.EnumerateEntries(PluginHideEnumerator, &closure);
}

static bool
HasOverlap(const nsIntPoint& aOffset1, const nsTArray<nsIntRect>& aClipRects1,
           const nsIntPoint& aOffset2, const nsTArray<nsIntRect>& aClipRects2)
{
  nsIntPoint offsetDelta = aOffset1 - aOffset2;
  for (PRUint32 i = 0; i < aClipRects1.Length(); ++i) {
    for (PRUint32 j = 0; j < aClipRects2.Length(); ++j) {
      if ((aClipRects1[i] + offsetDelta).Intersects(aClipRects2[j]))
        return true;
    }
  }
  return false;
}










static void
SortConfigurations(nsTArray<nsIWidget::Configuration>* aConfigurations)
{
  if (aConfigurations->Length() > 10) {
    
    return;
  }

  nsTArray<nsIWidget::Configuration> pluginsToMove;
  pluginsToMove.SwapElements(*aConfigurations);

  
  
  
  
  while (!pluginsToMove.IsEmpty()) {
    
    PRUint32 i;
    for (i = 0; i + 1 < pluginsToMove.Length(); ++i) {
      nsIWidget::Configuration* config = &pluginsToMove[i];
      bool foundOverlap = false;
      for (PRUint32 j = 0; j < pluginsToMove.Length(); ++j) {
        if (i == j)
          continue;
        nsIntRect bounds;
        pluginsToMove[j].mChild->GetBounds(bounds);
        nsAutoTArray<nsIntRect,1> clipRects;
        pluginsToMove[j].mChild->GetWindowClipRegion(&clipRects);
        if (HasOverlap(bounds.TopLeft(), clipRects,
                       config->mBounds.TopLeft(),
                       config->mClipRegion)) {
          foundOverlap = true;
          break;
        }
      }
      if (!foundOverlap)
        break;
    }
    
    
    aConfigurations->AppendElement(pluginsToMove[i]);
    pluginsToMove.RemoveElementAt(i);
  }
}

void
nsRootPresContext::UpdatePluginGeometry()
{
  if (!mNeedsToUpdatePluginGeometry)
    return;
  mNeedsToUpdatePluginGeometry = false;
  
  
  CancelUpdatePluginGeometryTimer();

  nsIFrame* f = mUpdatePluginGeometryForFrame;
  if (f) {
    mUpdatePluginGeometryForFrame->PresContext()->
      SetContainsUpdatePluginGeometryFrame(false);
    mUpdatePluginGeometryForFrame = nsnull;
  } else {
    f = FrameManager()->GetRootFrame();
  }

  nsTArray<nsIWidget::Configuration> configurations;
  GetPluginGeometryUpdates(f, &configurations);
  if (configurations.IsEmpty())
    return;
  SortConfigurations(&configurations);
  nsIWidget* widget = FrameManager()->GetRootFrame()->GetNearestWidget();
  NS_ASSERTION(widget, "Plugins must have a parent window");
  widget->ConfigureChildren(configurations);
  DidApplyPluginGeometryUpdates();
}

static void
UpdatePluginGeometryCallback(nsITimer *aTimer, void *aClosure)
{
  static_cast<nsRootPresContext*>(aClosure)->UpdatePluginGeometry();
}

void
nsRootPresContext::RequestUpdatePluginGeometry(nsIFrame* aFrame)
{
  if (mRegisteredPlugins.Count() == 0)
    return;

  if (!mNeedsToUpdatePluginGeometry) {
    
    
    
    
    
    
    
    
    
    mUpdatePluginGeometryTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mUpdatePluginGeometryTimer) {
      mUpdatePluginGeometryTimer->
        InitWithFuncCallback(UpdatePluginGeometryCallback, this,
                             nsRefreshDriver::DefaultInterval() * 2,
                             nsITimer::TYPE_ONE_SHOT);
    }
    mNeedsToUpdatePluginGeometry = true;
    mUpdatePluginGeometryForFrame = aFrame;
    mUpdatePluginGeometryForFrame->PresContext()->
      SetContainsUpdatePluginGeometryFrame(true);
  } else {
    if (!mUpdatePluginGeometryForFrame ||
        aFrame == mUpdatePluginGeometryForFrame)
      return;
    mUpdatePluginGeometryForFrame->PresContext()->
      SetContainsUpdatePluginGeometryFrame(false);
    mUpdatePluginGeometryForFrame = nsnull;
  }
}

static PLDHashOperator
PluginDidSetGeometryEnumerator(nsPtrHashKey<nsObjectFrame>* aEntry, void* userArg)
{
  nsObjectFrame* f = aEntry->GetKey();
  f->DidSetWidgetGeometry();
  return PL_DHASH_NEXT;
}

void
nsRootPresContext::DidApplyPluginGeometryUpdates()
{
  mRegisteredPlugins.EnumerateEntries(PluginDidSetGeometryEnumerator, nsnull);
}

void
nsRootPresContext::RootForgetUpdatePluginGeometryFrame(nsIFrame* aFrame)
{
  if (aFrame == mUpdatePluginGeometryForFrame) {
    mUpdatePluginGeometryForFrame->PresContext()->
      SetContainsUpdatePluginGeometryFrame(false);
    mUpdatePluginGeometryForFrame = nsnull;
  }
}

void
nsRootPresContext::RootForgetUpdatePluginGeometryFrameForPresContext(
  nsPresContext* aPresContext)
{
  if (aPresContext->GetContainsUpdatePluginGeometryFrame()) {
    aPresContext->SetContainsUpdatePluginGeometryFrame(false);
    mUpdatePluginGeometryForFrame = nsnull;
  }
}

static void
NotifyDidPaintForSubtreeCallback(nsITimer *aTimer, void *aClosure)
{
  nsPresContext* presContext = (nsPresContext*)aClosure;
  nsAutoScriptBlocker blockScripts;
  presContext->NotifyDidPaintForSubtree();
}

void
nsRootPresContext::EnsureEventualDidPaintEvent()
{
  if (mNotifyDidPaintTimer)
    return;
  mNotifyDidPaintTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (!mNotifyDidPaintTimer)
    return;
  mNotifyDidPaintTimer->InitWithFuncCallback(NotifyDidPaintForSubtreeCallback,
                                             (void*)this, 100, nsITimer::TYPE_ONE_SHOT);
}

void
nsRootPresContext::AddWillPaintObserver(nsIRunnable* aRunnable)
{
  if (!mWillPaintFallbackEvent.IsPending()) {
    mWillPaintFallbackEvent = new RunWillPaintObservers(this);
    NS_DispatchToMainThread(mWillPaintFallbackEvent.get());
  }
  mWillPaintObservers.AppendElement(aRunnable);
}




void
nsRootPresContext::FlushWillPaintObservers()
{
  mWillPaintFallbackEvent = nsnull;
  nsTArray<nsCOMPtr<nsIRunnable> > observers;
  observers.SwapElements(mWillPaintObservers);
  for (PRUint32 i = 0; i < observers.Length(); ++i) {
    observers[i]->Run();
  }
}

size_t
nsRootPresContext::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
  return nsPresContext::SizeOfExcludingThis(aMallocSizeOf);

  
  
  
  
  
  
  
  
  
}

