






#include "mozilla/ArrayUtils.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStateManager.h"

#include "base/basictypes.h"

#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsDocShell.h"
#include "nsIContentViewer.h"
#include "nsPIDOMWindow.h"
#include "nsStyleSet.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsIPrintSettings.h"
#include "nsILanguageAtomService.h"
#include "mozilla/LookAndFeel.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWeakReferenceUtils.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsFrameManager.h"
#include "nsLayoutUtils.h"
#include "nsViewManager.h"
#include "RestyleManager.h"
#include "SurfaceCache.h"
#include "nsCSSRuleProcessor.h"
#include "nsRuleNode.h"
#include "gfxPlatform.h"
#include "nsCSSRules.h"
#include "nsFontFaceLoader.h"
#include "mozilla/EventListenerManager.h"
#include "prenv.h"
#include "nsPluginFrame.h"
#include "nsTransitionManager.h"
#include "nsAnimationManager.h"
#include "CounterStyleManager.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/Element.h"
#include "nsIMessageManager.h"
#include "mozilla/dom/MediaQueryList.h"
#include "nsSMILAnimationController.h"
#include "mozilla/css/ImageLoader.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/dom/TabParent.h"
#include "nsRefreshDriver.h"
#include "Layers.h"
#include "ClientLayerManager.h"
#include "nsIDOMEvent.h"
#include "gfxPrefs.h"
#include "nsIDOMChromeWindow.h"
#include "nsFrameLoader.h"
#include "mozilla/dom/FontFaceSet.h"
#include "nsContentUtils.h"
#include "nsPIWindowRoot.h"
#include "mozilla/Preferences.h"
#include "gfxTextRun.h"
#include "nsFontFaceUtils.h"


#include "imgIContainer.h"
#include "nsIImageLoadingContent.h"

#include "nsCSSParser.h"
#include "nsBidiUtils.h"
#include "nsServiceManagerUtils.h"

#include "URL.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layers;

uint8_t gNotifySubDocInvalidationData;






class ContainerLayerPresContext : public LayerUserData {
public:
  nsPresContext* mPresContext;
};

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

nscolor
nsPresContext::MakeColorPref(const nsString& aColor)
{
  nsCSSParser parser;
  nsCSSValue value;
  if (!parser.ParseColorString(aColor, nullptr, 0, value)) {
    
    return NS_RGB(0, 0, 0);
  }

  nscolor color;
  return nsRuleNode::ComputeColor(value, this, nullptr, color)
    ? color
    : NS_RGB(0, 0, 0);
}

bool
nsPresContext::IsDOMPaintEventPending()
{
  if (mFireAfterPaintEvents) {
    return true;
  }
  nsRootPresContext* drpc = GetDisplayRootPresContext();
  if (drpc && drpc->mRefreshDriver->ViewManagerFlushIsPending()) {
    
    
    
    NotifyInvalidation(nsRect(0, 0, 0, 0), 0);
    NS_ASSERTION(mFireAfterPaintEvents, "Why aren't we planning to fire the event?");
    return true;
  }
  return false;
}

void
nsPresContext::PrefChangedCallback(const char* aPrefName, void* instance_data)
{
  nsRefPtr<nsPresContext>  presContext =
    static_cast<nsPresContext*>(instance_data);

  NS_ASSERTION(nullptr != presContext, "bad instance data");
  if (nullptr != presContext) {
    presContext->PreferenceChanged(aPrefName);
  }
}

void
nsPresContext::PrefChangedUpdateTimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsPresContext*  presContext = (nsPresContext*)aClosure;
  NS_ASSERTION(presContext != nullptr, "bad instance data");
  if (presContext)
    presContext->UpdateAfterPreferencesChanged();
}

static bool
IsVisualCharset(const nsCString& aCharset)
{
  if (aCharset.LowerCaseEqualsLiteral("ibm862")             
      || aCharset.LowerCaseEqualsLiteral("iso-8859-8") ) {  
    return true; 
  }
  else {
    return false; 
  }
}

  
  

nsPresContext::nsPresContext(nsIDocument* aDocument, nsPresContextType aType)
  : mType(aType), mDocument(aDocument), mBaseMinFontSize(0),
    mTextZoom(1.0), mFullZoom(1.0),
    mLastFontInflationScreenSize(gfxSize(-1.0, -1.0)),
    mPageSize(-1, -1), mPPScale(1.0f),
    mViewportStyleScrollbar(NS_STYLE_OVERFLOW_AUTO, NS_STYLE_OVERFLOW_AUTO),
    mImageAnimationModePref(imgIContainer::kNormalAnimMode),
    mAllInvalidated(false),
    mPaintFlashing(false), mPaintFlashingInitialized(false)
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
  mMediaEmulated = mMedium;

  if (!IsDynamic()) {
    mImageAnimationMode = imgIContainer::kDontAnimMode;
    mNeverAnimate = true;
  } else {
    mImageAnimationMode = imgIContainer::kNormalAnimMode;
    mNeverAnimate = false;
  }
  NS_ASSERTION(mDocument, "Null document");
  mFontFaceSetDirty = true;

  mCounterStylesDirty = true;

  
  PRLogModuleInfo *log = gfxPlatform::GetLog(eGfxLog_textperf);
  if (log && log->level >= PR_LOG_WARNING) {
    mTextPerf = new gfxTextPerfMetrics();
  }

  if (Preferences::GetBool(GFX_MISSING_FONTS_NOTIFY_PREF)) {
    mMissingFonts = new gfxMissingFontRecorder();
  }
}

void
nsPresContext::Destroy()
{
  if (mEventManager) {
    
    mEventManager->NotifyDestroyPresContext(this);
    mEventManager->SetPresContext(nullptr);
    mEventManager = nullptr;
  }

  if (mPrefChangedTimer)
  {
    mPrefChangedTimer->Cancel();
    mPrefChangedTimer = nullptr;
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
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "bidi.",
                                  this);
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
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "nglayout.debug.paint_flashing",
                                  this);
  Preferences::UnregisterCallback(nsPresContext::PrefChangedCallback,
                                  "nglayout.debug.paint_flashing_chrome",
                                  this);

  
  
  if (mRefreshDriver && mRefreshDriver->PresContext() == this) {
    mRefreshDriver->Disconnect();
    mRefreshDriver = nullptr;
  }
}

nsPresContext::~nsPresContext()
{
  NS_PRECONDITION(!mShell, "Presshell forgot to clear our mShell pointer");
  SetShell(nullptr);

  Destroy();
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsPresContext)
   NS_INTERFACE_MAP_ENTRY(nsISupports)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsPresContext)
NS_IMPL_CYCLE_COLLECTING_RELEASE_WITH_LAST_RELEASE(nsPresContext, LastRelease())

void
nsPresContext::LastRelease()
{
  if (IsRoot()) {
    static_cast<nsRootPresContext*>(this)->CancelDidPaintTimer();
  }
  if (mMissingFonts) {
    mMissingFonts->Clear();
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsPresContext)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsPresContext)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocument);
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mEventManager);
  

  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPrintSettings);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPrefChangedTimer);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsPresContext)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDocument);
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDeviceContext); 
  
  
  
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPrintSettings);

  tmp->Destroy();
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
  

  nsresult rv = NS_OK;
  nsIAtom *langGroupAtom = nullptr;
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
    DebugOnly<uint32_t> count = 0;
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

  















  nsAutoCString langGroup;
  langGroupAtom->ToUTF8String(langGroup);

  prefs->mDefaultVariableFont.size = CSSPixelsToAppUnits(16);
  prefs->mDefaultFixedFont.size = CSSPixelsToAppUnits(13);

  nsAutoCString pref;

  
  enum {eUnit_unknown = -1, eUnit_px, eUnit_pt};
  int32_t unit = eUnit_px;

  nsAdoptingCString cvalue =
    Preferences::GetCString("font.size.unit");

  if (!cvalue.IsEmpty()) {
    if (cvalue.EqualsLiteral("px")) {
      unit = eUnit_px;
    }
    else if (cvalue.EqualsLiteral("pt")) {
      unit = eUnit_pt;
    }
    else {
      
      
      NS_WARNING("unexpected font-size unit -- expected: 'px' or 'pt'");
      unit = eUnit_unknown;
    }
  }

  

  MAKE_FONT_PREF_KEY(pref, "font.minimum-size.", langGroup);

  int32_t size = Preferences::GetInt(pref.get());
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
  static_assert(MOZ_ARRAY_LENGTH(fontTypes) == eDefaultFont_COUNT,
                "FontTypes array count is not correct");

  
  
  
  
  
  nsAutoCString generic_dot_langGroup;
  for (uint32_t eType = 0; eType < ArrayLength(fontTypes); ++eType) {
    generic_dot_langGroup.Assign(kGenericFont[eType]);
    generic_dot_langGroup.Append(langGroup);

    nsFont* font = fontTypes[eType];

    
    
    if (eType == eDefaultFont_Variable) {
      MAKE_FONT_PREF_KEY(pref, "font.name.variable.", langGroup);

      nsAdoptingString value = Preferences::GetString(pref.get());
      if (!value.IsEmpty()) {
        FontFamilyName defaultVariableName = FontFamilyName::Convert(value);
        FontFamilyType defaultType = defaultVariableName.mType;
        NS_ASSERTION(defaultType == eFamily_serif ||
                     defaultType == eFamily_sans_serif,
                     "default type must be serif or sans-serif");
        prefs->mDefaultVariableFont.fontlist = FontFamilyList(defaultType);
      }
      else {
        MAKE_FONT_PREF_KEY(pref, "font.default.", langGroup);
        value = Preferences::GetString(pref.get());
        if (!value.IsEmpty()) {
          FontFamilyName defaultVariableName = FontFamilyName::Convert(value);
          FontFamilyType defaultType = defaultVariableName.mType;
          NS_ASSERTION(defaultType == eFamily_serif ||
                       defaultType == eFamily_sans_serif,
                       "default type must be serif or sans-serif");
          prefs->mDefaultVariableFont.fontlist = FontFamilyList(defaultType);
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
  
  
  
  gfxPrefs::GetSingleton();

  int32_t useAccessibilityTheme = 0;
  bool usePrefColors = true;
  bool isChromeDocShell = false;
  static int32_t sDocumentColorsSetting;
  static bool sDocumentColorsSettingPrefCached = false;
  if (!sDocumentColorsSettingPrefCached) {
    sDocumentColorsSettingPrefCached = true;
    Preferences::AddIntVarCache(&sDocumentColorsSetting,
                                "browser.display.document_color_use",
                                0);
  }

  nsIDocument* doc = mDocument->GetDisplayDocument();
  if (doc && doc->GetDocShell()) {
    isChromeDocShell = nsIDocShellTreeItem::typeChrome ==
                       doc->GetDocShell()->ItemType();
  } else {
    nsCOMPtr<nsIDocShellTreeItem> docShell(mContainer);
    if (docShell) {
      isChromeDocShell = nsIDocShellTreeItem::typeChrome == docShell->ItemType();
    }
  }

  mIsChromeOriginImage = mDocument->IsBeingUsedAsImage() &&
                         IsChromeURI(mDocument->GetDocumentURI());

  if (isChromeDocShell || mIsChromeOriginImage) {
    usePrefColors = false;
  } else {
    useAccessibilityTheme =
      LookAndFeel::GetInt(LookAndFeel::eIntID_UseAccessibilityTheme, 0);
    usePrefColors = !useAccessibilityTheme;
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


  
  
  
  
  if (sDocumentColorsSetting == 1) {
    mUseDocumentColors = true;
  } else if (sDocumentColorsSetting == 2) {
    mUseDocumentColors = isChromeDocShell || mIsChromeOriginImage;
  } else {
    MOZ_ASSERT(!useAccessibilityTheme ||
               !(isChromeDocShell || mIsChromeOriginImage),
               "The accessibility theme should only be on for non-chrome");
    mUseDocumentColors = !useAccessibilityTheme;
  }
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

  mPrefScrollbarSide = Preferences::GetInt("layout.scrollbar.side");

  ResetCachedFontPrefs();

  
  const nsAdoptingCString& animatePref =
    Preferences::GetCString("image.animation_mode");
  if (animatePref.EqualsLiteral("normal"))
    mImageAnimationModePref = imgIContainer::kNormalAnimMode;
  else if (animatePref.EqualsLiteral("none"))
    mImageAnimationModePref = imgIContainer::kDontAnimMode;
  else if (animatePref.EqualsLiteral("once"))
    mImageAnimationModePref = imgIContainer::kLoopOnceAnimMode;
  else 
    mImageAnimationModePref = imgIContainer::kNormalAnimMode;

  uint32_t bidiOptions = GetBidi();

  int32_t prefInt =
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
nsPresContext::InvalidatePaintedLayers()
{
  if (!mShell)
    return;
  nsIFrame* rootFrame = mShell->FrameManager()->GetRootFrame();
  if (rootFrame) {
    
    
    
    rootFrame->InvalidateFrameSubtree();
  }
}

void
nsPresContext::AppUnitsPerDevPixelChanged()
{
  InvalidatePaintedLayers();

  if (mDeviceContext) {
    mDeviceContext->FlushFontCache();
  }

  if (HasCachedStyleData()) {
    
    MediaFeatureValuesChanged(eRestyle_ForceDescendants, NS_STYLE_HINT_REFLOW);
  }

  mCurAppUnitsPerDevPixel = AppUnitsPerDevPixel();
}

void
nsPresContext::PreferenceChanged(const char* aPrefName)
{
  nsDependentCString prefName(aPrefName);
  if (prefName.EqualsLiteral("layout.css.dpi") ||
      prefName.EqualsLiteral("layout.css.devPixelsPerPx")) {
    int32_t oldAppUnitsPerDevPixel = AppUnitsPerDevPixel();
    if (mDeviceContext->CheckDPIChange() && mShell) {
      nsCOMPtr<nsIPresShell> shell = mShell;
      
      
      nscoord oldWidthAppUnits, oldHeightAppUnits;
      nsRefPtr<nsViewManager> vm = shell->GetViewManager();
      if (!vm) {
        return;
      }
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
  if (prefName.EqualsLiteral(GFX_MISSING_FONTS_NOTIFY_PREF)) {
    if (Preferences::GetBool(GFX_MISSING_FONTS_NOTIFY_PREF)) {
      if (!mMissingFonts) {
        mMissingFonts = new gfxMissingFontRecorder();
        
        mPrefChangePendingNeedsReflow = true;
      }
    } else {
      if (mMissingFonts) {
        mMissingFonts->Clear();
      }
      mMissingFonts = nullptr;
    }
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
  if (prefName.EqualsLiteral("nglayout.debug.paint_flashing") ||
      prefName.EqualsLiteral("nglayout.debug.paint_flashing_chrome")) {
    mPaintFlashingInitialized = false;
    return;
  }
}

void
nsPresContext::UpdateAfterPreferencesChanged()
{
  mPrefChangedTimer = nullptr;

  nsCOMPtr<nsIDocShellTreeItem> docShell(mContainer);
  if (docShell && nsIDocShellTreeItem::typeChrome == docShell->ItemType()) {
    return;
  }

  
  GetUserPreferences();

  
  if (mShell) {
    mShell->SetPreferenceStyleRules(true);
  }

  InvalidatePaintedLayers();
  mDeviceContext->FlushFontCache();

  nsChangeHint hint = nsChangeHint(0);

  if (mPrefChangePendingNeedsReflow) {
    NS_UpdateHint(hint, NS_STYLE_HINT_REFLOW);
  }

  
  
  RebuildAllStyleData(hint, eRestyle_Subtree);
}

nsresult
nsPresContext::Init(nsDeviceContext* aDeviceContext)
{
  NS_ASSERTION(!mInitialized, "attempt to reinit pres context");
  NS_ENSURE_ARG(aDeviceContext);

  mDeviceContext = aDeviceContext;

  if (mDeviceContext->SetFullZoom(mFullZoom))
    mDeviceContext->FlushFontCache();
  mCurAppUnitsPerDevPixel = AppUnitsPerDevPixel();

  mEventManager = new mozilla::EventStateManager();

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

      
      nsCOMPtr<nsIDocShellTreeItem> ourItem = mDocument->GetDocShell();
      if (ourItem) {
        nsCOMPtr<nsIDocShellTreeItem> parentItem;
        ourItem->GetSameTypeParent(getter_AddRefs(parentItem));
        if (parentItem) {
          Element* containingElement =
            parent->FindContentForSubDocument(mDocument);
          if (!containingElement->IsXULElement() ||
              !containingElement->
                HasAttr(kNameSpaceID_None,
                        nsGkAtoms::forceOwnRefreshDriver)) {
            mRefreshDriver = parent->GetShell()->GetPresContext()->RefreshDriver();
          }
        }
      }
    }

    if (!mRefreshDriver) {
      mRefreshDriver = new nsRefreshDriver(this);
    }
  }

  
  mLastStyleUpdateForAllAnimations = mRefreshDriver->MostRecentRefresh();

  
  
  
  
  mRestyleManager = new mozilla::RestyleManager(this);

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
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "bidi.",
                                this);
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
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "nglayout.debug.paint_flashing",
                                this);
  Preferences::RegisterCallback(nsPresContext::PrefChangedCallback,
                                "nglayout.debug.paint_flashing_chrome",
                                this);

  nsresult rv = mEventManager->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mEventManager->SetPresContext(this);

#ifdef RESTYLE_LOGGING
  mRestyleLoggingEnabled = RestyleManager::RestyleLoggingInitiallyEnabled();
#endif

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
  if (mFontFaceSet) {
    
    mFontFaceSet->DestroyUserFontSet();
    mFontFaceSet = nullptr;
  }
  if (mCounterStyleManager) {
    mCounterStyleManager->Disconnect();
    mCounterStyleManager = nullptr;
  }

  if (mShell) {
    
    
    nsIDocument *doc = mShell->GetDocument();
    if (doc) {
      doc->RemoveCharSetObserver(this);
    }
  }

  mShell = aShell;

  if (mShell) {
    
    
    
    mCounterStyleManager = new mozilla::CounterStyleManager(this);

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
      mTransitionManager = nullptr;
    }
    if (mAnimationManager) {
      mAnimationManager->Disconnect();
      mAnimationManager = nullptr;
    }
    if (mRestyleManager) {
      mRestyleManager->Disconnect();
      mRestyleManager = nullptr;
    }

    if (IsRoot()) {
      
      
      static_cast<nsRootPresContext*>(this)->CancelApplyPluginGeometryTimer();
    }
  }
}

void
nsPresContext::DoChangeCharSet(const nsCString& aCharSet)
{
  UpdateCharSet(aCharSet);
  mDeviceContext->FlushFontCache();
  RebuildAllStyleData(NS_STYLE_HINT_REFLOW, nsRestyleHint(0));
}

void
nsPresContext::UpdateCharSet(const nsCString& aCharSet)
{
  if (mLangService) {
    mLanguage = mLangService->LookupCharSet(aCharSet);
    

    
    if (mLanguage == nsGkAtoms::Unicode) {
      mLanguage = mLangService->GetLocaleLanguage();
    }
    ResetCachedFontPrefs();
  }

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
}

NS_IMETHODIMP
nsPresContext::Observe(nsISupports* aSubject,
                        const char* aTopic,
                        const char16_t* aData)
{
  if (!nsCRT::strcmp(aTopic, "charset")) {
    nsRefPtr<CharSetChangingRunnable> runnable =
      new CharSetChangingRunnable(this, NS_LossyConvertUTF16toASCII(aData));
    return NS_DispatchToCurrentThread(runnable);
  }

  NS_WARNING("unrecognized topic in nsPresContext::Observe");
  return NS_ERROR_FAILURE;
}

nsPresContext*
nsPresContext::GetParentPresContext()
{
  nsIPresShell* shell = GetPresShell();
  if (shell) {
    nsViewManager* viewManager = shell->GetViewManager();
    if (viewManager) {
      nsView* view = viewManager->GetRootView();
      if (view) {
        view = view->GetParent(); 
        if (view) {
          view = view->GetParent(); 
          if (view) {
            nsIFrame* f = view->GetFrame();
            if (f) {
              return f->PresContext();
            }
          }
        }
      }
    }
  }
  return nullptr;
}

nsPresContext*
nsPresContext::GetToplevelContentDocumentPresContext()
{
  if (IsChrome())
    return nullptr;
  nsPresContext* pc = this;
  for (;;) {
    nsPresContext* parent = pc->GetParentPresContext();
    if (!parent || parent->IsChrome())
      return pc;
    pc = parent;
  }
}

nsIWidget*
nsPresContext::GetNearestWidget(nsPoint* aOffset)
{
  NS_ENSURE_TRUE(mShell, nullptr);
  nsIFrame* frame = mShell->GetRootFrame();
  NS_ENSURE_TRUE(frame, nullptr);
  return frame->GetView()->GetNearestWidget(aOffset);
}

nsIWidget*
nsPresContext::GetRootWidget()
{
  NS_ENSURE_TRUE(mShell, nullptr);
  nsViewManager* vm = mShell->GetViewManager();
  if (!vm) {
    return nullptr;
  }
  nsCOMPtr<nsIWidget> widget;
  vm->GetRootWidget(getter_AddRefs(widget));
  return widget.get();
}


nsRootPresContext*
nsPresContext::GetRootPresContext()
{
  nsPresContext* pc = this;
  for (;;) {
    nsPresContext* parent = pc->GetParentPresContext();
    if (!parent)
      break;
    pc = parent;
  }
  return pc->IsRoot() ? static_cast<nsRootPresContext*>(pc) : nullptr;
}

nsRootPresContext*
nsPresContext::GetDisplayRootPresContext()
{
  nsPresContext* pc = this;
  for (;;) {
    nsPresContext* parent = pc->GetParentPresContext();
    if (!parent) {
      
      
      nsIDocument *doc = pc->Document();
      if (doc) {
        doc = doc->GetParentDocument();
        if (doc) {
          nsIPresShell* shell = doc->GetShell();
          if (shell) {
            parent = shell->GetPresContext();
          }
        }
      }
    }
    if (!parent || parent == pc)
      break;
    pc = parent;
  }
  return pc->IsRoot() ? static_cast<nsRootPresContext*>(pc) : nullptr;
}

void
nsPresContext::CompatibilityModeChanged()
{
  if (!mShell)
    return;

  
  mShell->StyleSet()->
    EnableQuirkStyleSheet(CompatibilityMode() == eCompatibility_NavQuirks);
}


static void SetImgAnimModeOnImgReq(imgIRequest* aImgReq, uint16_t aMode)
{
  if (aImgReq) {
    nsCOMPtr<imgIContainer> imgCon;
    aImgReq->GetImage(getter_AddRefs(imgCon));
    if (imgCon) {
      imgCon->SetAnimationMode(aMode);
    }
  }
}






void nsPresContext::SetImgAnimations(nsIContent *aParent, uint16_t aMode)
{
  nsCOMPtr<nsIImageLoadingContent> imgContent(do_QueryInterface(aParent));
  if (imgContent) {
    nsCOMPtr<imgIRequest> imgReq;
    imgContent->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                           getter_AddRefs(imgReq));
    SetImgAnimModeOnImgReq(imgReq, aMode);
  }

  uint32_t count = aParent->GetChildCount();
  for (uint32_t i = 0; i < count; ++i) {
    SetImgAnimations(aParent->GetChildAt(i), aMode);
  }
}

void
nsPresContext::SetSMILAnimations(nsIDocument *aDoc, uint16_t aNewMode,
                                 uint16_t aOldMode)
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
nsPresContext::SetImageAnimationModeInternal(uint16_t aMode)
{
  NS_ASSERTION(aMode == imgIContainer::kNormalAnimMode ||
               aMode == imgIContainer::kDontAnimMode ||
               aMode == imgIContainer::kLoopOnceAnimMode, "Wrong Animation Mode is being set!");

  
  if (!IsDynamic())
    return;

  
  
  if (mShell != nullptr) {
    nsIDocument *doc = mShell->GetDocument();
    if (doc) {
      doc->StyleImageLoader()->SetAnimationMode(aMode);

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
nsPresContext::SetImageAnimationModeExternal(uint16_t aMode)
{
  SetImageAnimationModeInternal(aMode);
}

const nsFont*
nsPresContext::GetDefaultFont(uint8_t aFontID, nsIAtom *aLanguage) const
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
      font = nullptr;
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
  mDeviceContext->SetFullZoom(aZoom);

  NS_ASSERTION(!mSupressResizeReflow, "two zooms happening at the same time? impossible!");
  mSupressResizeReflow = true;

  mFullZoom = aZoom;
  mShell->GetViewManager()->
    SetWindowDimensions(NSToCoordRound(oldWidthDevPixels * AppUnitsPerDevPixel()),
                        NSToCoordRound(oldHeightDevPixels * AppUnitsPerDevPixel()));

  AppUnitsPerDevPixelChanged();

  mSupressResizeReflow = false;
}

gfxSize
nsPresContext::ScreenSizeInchesForFontInflation(bool* aChanged)
{
  if (aChanged) {
    *aChanged = false;
  }

  nsDeviceContext *dx = DeviceContext();
  nsRect clientRect;
  dx->GetClientRect(clientRect); 
  float unitsPerInch = dx->AppUnitsPerPhysicalInch();
  gfxSize deviceSizeInches(float(clientRect.width) / unitsPerInch,
                           float(clientRect.height) / unitsPerInch);

  if (mLastFontInflationScreenSize == gfxSize(-1.0, -1.0)) {
    mLastFontInflationScreenSize = deviceSizeInches;
  }

  if (deviceSizeInches != mLastFontInflationScreenSize && aChanged) {
    *aChanged = true;
    mLastFontInflationScreenSize = deviceSizeInches;
  }

  return deviceSizeInches;
}

void
nsPresContext::SetContainer(nsIDocShell* aDocShell)
{
  if (aDocShell) {
    mContainer = static_cast<nsDocShell*>(aDocShell);
  } else {
    mContainer = WeakPtr<nsDocShell>();
  }
  UpdateIsChrome();
  if (mContainer) {
    GetDocumentColorPreferences();
  }
}

nsISupports*
nsPresContext::GetContainerWeakInternal() const
{
  return static_cast<nsIDocShell*>(mContainer);
}

nsISupports*
nsPresContext::GetContainerWeakExternal() const
{
  return GetContainerWeakInternal();
}

nsIDocShell*
nsPresContext::GetDocShell() const
{
  return mContainer;
}

 void
nsPresContext::Detach()
{
  SetContainer(nullptr);
  SetLinkHandler(nullptr);
  if (mShell) {
    mShell->CancelInvalidatePresShellIfHidden();
  }
}

bool
nsPresContext::StyleUpdateForAllAnimationsIsUpToDate() const
{
  return mLastStyleUpdateForAllAnimations == mRefreshDriver->MostRecentRefresh();
}

void
nsPresContext::TickLastStyleUpdateForAllAnimations()
{
  mLastStyleUpdateForAllAnimations = mRefreshDriver->MostRecentRefresh();
}

void
nsPresContext::ClearLastStyleUpdateForAllAnimations()
{
  mLastStyleUpdateForAllAnimations = TimeStamp();
}

bool
nsPresContext::BidiEnabledExternal() const
{
  return BidiEnabledInternal();
}

bool
nsPresContext::BidiEnabledInternal() const
{
  return Document()->GetBidiEnabled();
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

void
nsPresContext::SetBidi(uint32_t aSource, bool aForceRestyle)
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

uint32_t
nsPresContext::GetBidi() const
{
  return Document()->GetBidiOptions();
}

bool
nsPresContext::IsTopLevelWindowInactive()
{
  nsCOMPtr<nsIDocShellTreeItem> treeItem(mContainer);
  if (!treeItem)
    return false;

  nsCOMPtr<nsIDocShellTreeItem> rootItem;
  treeItem->GetRootTreeItem(getter_AddRefs(rootItem));
  if (!rootItem) {
    return false;
  }

  nsCOMPtr<nsPIDOMWindow> domWindow = rootItem->GetWindow();

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

static void
NotifyThemeChanged(TabParent* aTabParent, void* aArg)
{
  aTabParent->ThemeChanged();
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

    
    
    
    mozilla::image::SurfaceCache::DiscardAll();
  }

  
  nsCSSRuleProcessor::FreeSystemMetrics();

  
  
  
  
  
  
  MediaFeatureValuesChanged(eRestyle_Subtree, NS_STYLE_HINT_REFLOW);

  
  
  nsContentUtils::CallOnAllRemoteChildren(mDocument->GetWindow(),
                                          NotifyThemeChanged, nullptr);
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

  
  
  RebuildAllStyleData(nsChangeHint(0), nsRestyleHint(0));
}

void
nsPresContext::UIResolutionChanged()
{
  if (!mPendingUIResolutionChanged) {
    nsCOMPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(this, &nsPresContext::UIResolutionChangedInternal);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mPendingUIResolutionChanged = true;
    }
  }
}

 bool
nsPresContext::UIResolutionChangedSubdocumentCallback(nsIDocument* aDocument,
                                                      void* aData)
{
  nsIPresShell* shell = aDocument->GetShell();
  if (shell) {
    nsPresContext* pc = shell->GetPresContext();
    if (pc) {
      pc->UIResolutionChangedInternal();
    }
  }
  return true;
}

static void
NotifyUIResolutionChanged(TabParent* aTabParent, void* aArg)
{
  aTabParent->UIResolutionChanged();
}

void
nsPresContext::UIResolutionChangedInternal()
{
  mPendingUIResolutionChanged = false;

  mDeviceContext->CheckDPIChange();
  if (mCurAppUnitsPerDevPixel != AppUnitsPerDevPixel()) {
    AppUnitsPerDevPixelChanged();
  }

  
  
  nsContentUtils::CallOnAllRemoteChildren(mDocument->GetWindow(),
                                          NotifyUIResolutionChanged, nullptr);

  mDocument->EnumerateSubDocuments(UIResolutionChangedSubdocumentCallback,
                                   nullptr);
}

void
nsPresContext::EmulateMedium(const nsAString& aMediaType)
{
  nsIAtom* previousMedium = Medium();
  mIsEmulatingMedia = true;

  nsAutoString mediaType;
  nsContentUtils::ASCIIToLower(aMediaType, mediaType);

  mMediaEmulated = do_GetAtom(mediaType);
  if (mMediaEmulated != previousMedium && mShell) {
    MediaFeatureValuesChanged(nsRestyleHint(0), nsChangeHint(0));
  }
}

void nsPresContext::StopEmulatingMedium()
{
  nsIAtom* previousMedium = Medium();
  mIsEmulatingMedia = false;
  if (Medium() != previousMedium) {
    MediaFeatureValuesChanged(nsRestyleHint(0), nsChangeHint(0));
  }
}

void
nsPresContext::RebuildAllStyleData(nsChangeHint aExtraHint,
                                   nsRestyleHint aRestyleHint)
{
  if (!mShell) {
    
    return;
  }

  mUsesRootEMUnits = false;
  mUsesExChUnits = false;
  mUsesViewportUnits = false;
  RebuildUserFontSet();
  RebuildCounterStyles();

  RestyleManager()->RebuildAllStyleData(aExtraHint, aRestyleHint);
}

void
nsPresContext::PostRebuildAllStyleDataEvent(nsChangeHint aExtraHint,
                                            nsRestyleHint aRestyleHint)
{
  if (!mShell) {
    
    return;
  }
  RestyleManager()->PostRebuildAllStyleDataEvent(aExtraHint, aRestyleHint);
}

void
nsPresContext::MediaFeatureValuesChanged(nsRestyleHint aRestyleHint,
                                         nsChangeHint aChangeHint)
{
  mPendingMediaFeatureValuesChanged = false;

  
  if (mShell && mShell->StyleSet()->MediumFeaturesChanged()) {
    aRestyleHint |= eRestyle_Subtree;
  }

  if (mUsesViewportUnits && mPendingViewportChange) {
    
    aRestyleHint |= eRestyle_ForceDescendants;
  }

  if (aRestyleHint || aChangeHint) {
    RebuildAllStyleData(aChangeHint, aRestyleHint);
  }

  mPendingViewportChange = false;

  if (mDocument->IsBeingUsedAsImage()) {
    MOZ_ASSERT(PR_CLIST_IS_EMPTY(mDocument->MediaQueryLists()));
    return;
  }

  MOZ_ASSERT(nsContentUtils::IsSafeToRunScript());

  
  
  
  
  
  

  if (!PR_CLIST_IS_EMPTY(mDocument->MediaQueryLists())) {
    
    
    
    
    
    
    
    
    
    
    
    
    MediaQueryList::NotifyList notifyList;
    for (PRCList *l = PR_LIST_HEAD(mDocument->MediaQueryLists());
         l != mDocument->MediaQueryLists(); l = PR_NEXT_LINK(l)) {
      MediaQueryList *mql = static_cast<MediaQueryList*>(l);
      mql->MediumFeaturesChanged(notifyList);
    }

    if (!notifyList.IsEmpty()) {
      for (uint32_t i = 0, i_end = notifyList.Length(); i != i_end; ++i) {
        nsAutoMicroTask mt;
        MediaQueryList::HandleChangeData &d = notifyList[i];
        ErrorResult result;
        d.callback->Call(*d.mql, result);
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
    MediaFeatureValuesChanged(nsRestyleHint(0));
  }
}

nsCompatibility
nsPresContext::CompatibilityMode() const
{
  return Document()->GetCompatibilityMode();
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
  nsCOMPtr<nsIDocShell> docShell(mContainer);
  if (docShell) {
    nsCOMPtr<nsIContentViewer> cv;
    docShell->GetContentViewer(getter_AddRefs(cv));
    
    if (cv) {
      nsRefPtr<nsPresContext> currentPresContext;
      cv->GetPresContext(getter_AddRefs(currentPresContext));
      if (currentPresContext == this) {
        
        nsresult result = cv->Show();
        if (NS_SUCCEEDED(result)) {
          return true;
        }
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

void
nsPresContext::UpdateIsChrome()
{
  mIsChrome = mContainer &&
              nsIDocShellTreeItem::typeChrome == mContainer->ItemType();
}

 bool
nsPresContext::HasAuthorSpecifiedRules(nsIFrame *aFrame, uint32_t ruleTypeMask) const
{
  return
    nsRuleNode::HasAuthorSpecifiedRules(aFrame->StyleContext(),
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
  if (mFontFaceSetDirty) {
    
    
    
    
    
    
    NS_ASSERTION(!userFontSetGottenBefore || !mShell->IsReflowLocked(),
                 "FlushUserFontSet should have been called first");
    FlushUserFontSet();
  }

  if (!mFontFaceSet) {
    return nullptr;
  }

  return mFontFaceSet->GetUserFontSet();
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

  if (mFontFaceSetDirty) {
    if (gfxPlatform::GetPlatform()->DownloadableFontsEnabled()) {
      nsTArray<nsFontFaceRuleContainer> rules;
      if (!mShell->StyleSet()->AppendFontFaceRules(rules)) {
        return;
      }

      if (!mFontFaceSet) {
        mFontFaceSet = new FontFaceSet(mDocument->GetInnerWindow(), this);
      }
      mFontFaceSet->EnsureUserFontSet(this);
      bool changed = mFontFaceSet->UpdateRules(rules);

      
      
      
      
      if (changed) {
        UserFontSetUpdated();
      }
    }

    mFontFaceSetDirty = false;
  }
}

void
nsPresContext::RebuildUserFontSet()
{
  if (!mGetUserFontSetCalled) {
    
    
    
    return;
  }

  mFontFaceSetDirty = true;
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
nsPresContext::UserFontSetUpdated(gfxUserFontEntry* aUpdatedFont)
{
  if (!mShell)
    return;

  bool usePlatformFontList = true;
#if defined(MOZ_WIDGET_GTK) || defined(MOZ_WIDGET_QT)
  usePlatformFontList = false;
#endif

  
  
  
  
  
  
  
  if (!usePlatformFontList || !aUpdatedFont) {
    PostRebuildAllStyleDataEvent(NS_STYLE_HINT_REFLOW, eRestyle_ForceDescendants);
    return;
  }

  
  
  
  
  if (UsesExChUnits()) {
    PostRebuildAllStyleDataEvent(nsChangeHint(0), eRestyle_ForceDescendants);
  }

  
  
  
  
  
  
  nsIFrame* root = mShell->GetRootFrame();
  if (root) {
    nsFontFaceUtils::MarkDirtyForFontChange(root, aUpdatedFont);
  }
}

FontFaceSet*
nsPresContext::Fonts()
{
  if (!mFontFaceSet) {
    mFontFaceSet = new FontFaceSet(mDocument->GetInnerWindow(), this);
    GetUserFontSet();  
  }
  return mFontFaceSet;
}

void
nsPresContext::FlushCounterStyles()
{
  if (!mShell) {
    return; 
  }
  if (mCounterStyleManager->IsInitial()) {
    
    return;
  }

  if (mCounterStylesDirty) {
    bool changed = mCounterStyleManager->NotifyRuleChanged();
    if (changed) {
      PresShell()->NotifyCounterStylesAreDirty();
      PostRebuildAllStyleDataEvent(NS_STYLE_HINT_REFLOW,
                                   eRestyle_ForceDescendants);
    }
    mCounterStylesDirty = false;
  }
}

void
nsPresContext::RebuildCounterStyles()
{
  if (mCounterStyleManager->IsInitial()) {
    
    return;
  }

  mCounterStylesDirty = true;
  mDocument->SetNeedStyleFlush();
  if (!mPostedFlushCounterStyles) {
    nsCOMPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(this, &nsPresContext::HandleRebuildCounterStyles);
    if (NS_SUCCEEDED(NS_DispatchToCurrentThread(ev))) {
      mPostedFlushCounterStyles = true;
    }
  }
}

void
nsPresContext::NotifyMissingFonts()
{
  if (mMissingFonts) {
    mMissingFonts->Flush();
  }
}

void
nsPresContext::EnsureSafeToHandOutCSSRules()
{
  CSSStyleSheet::EnsureUniqueInnerResult res =
    mShell->StyleSet()->EnsureUniqueInnerOnCSSSheets();
  if (res == CSSStyleSheet::eUniqueInner_AlreadyUnique) {
    
    return;
  }

  MOZ_ASSERT(res == CSSStyleSheet::eUniqueInner_ClonedInner);
  RebuildAllStyleData(nsChangeHint(0), eRestyle_Subtree);
}

void
nsPresContext::FireDOMPaintEvent(nsInvalidateRequestList* aList)
{
  nsPIDOMWindow* ourWindow = mDocument->GetWindow();
  if (!ourWindow)
    return;

  nsCOMPtr<EventTarget> dispatchTarget = do_QueryInterface(ourWindow);
  nsCOMPtr<EventTarget> eventTarget = dispatchTarget;
  if (!IsChrome() && !mSendAfterPaintToContent) {
    
    
    
    
    dispatchTarget = do_QueryInterface(ourWindow->GetParentTarget());
    if (!dispatchTarget) {
      return;
    }
  }
  
  
  nsCOMPtr<nsIDOMEvent> event;
  
  
  
  NS_NewDOMNotifyPaintEvent(getter_AddRefs(event), eventTarget, this, nullptr,
                            NS_AFTERPAINT, aList);
  if (!event) {
    return;
  }

  
  
  
  event->SetTarget(eventTarget);
  event->SetTrusted(true);
  EventDispatcher::DispatchDOMEvent(dispatchTarget, nullptr, event, this,
                                    nullptr);
}

static bool
MayHavePaintEventListenerSubdocumentCallback(nsIDocument* aDocument, void* aData)
{
  bool *result = static_cast<bool*>(aData);
  nsIPresShell* shell = aDocument->GetShell();
  if (shell) {
    nsPresContext* pc = shell->GetPresContext();
    if (pc) {
      *result = pc->MayHavePaintEventListenerInSubDocument();

      
      
      return !*result;
    }
  }
  return true;
}

static bool
MayHavePaintEventListener(nsPIDOMWindow* aInnerWindow)
{
  if (!aInnerWindow)
    return false;
  if (aInnerWindow->HasPaintEventListeners())
    return true;

  EventTarget* parentTarget = aInnerWindow->GetParentTarget();
  if (!parentTarget)
    return false;

  EventListenerManager* manager = nullptr;
  if ((manager = parentTarget->GetExistingListenerManager()) &&
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
  EventTarget* tabChildGlobal;
  return root &&
         (tabChildGlobal = root->GetParentTarget()) &&
         (manager = tabChildGlobal->GetExistingListenerManager()) &&
         manager->MayHavePaintEventListener();
}

bool
nsPresContext::MayHavePaintEventListener()
{
  return ::MayHavePaintEventListener(mDocument->GetInnerWindow());
}

bool
nsPresContext::MayHavePaintEventListenerInSubDocument()
{
  if (MayHavePaintEventListener()) {
    return true;
  }

  bool result = false;
  mDocument->EnumerateSubDocuments(MayHavePaintEventListenerSubdocumentCallback, &result);
  return result;
}

void
nsPresContext::NotifyInvalidation(uint32_t aFlags)
{
  nsIFrame* rootFrame = PresShell()->FrameManager()->GetRootFrame();
  NotifyInvalidation(rootFrame->GetVisualOverflowRect(), aFlags);
  mAllInvalidated = true;
}

void
nsPresContext::NotifyInvalidation(const nsIntRect& aRect, uint32_t aFlags)
{
  nsRect rect(DevPixelsToAppUnits(aRect.x),
              DevPixelsToAppUnits(aRect.y),
              DevPixelsToAppUnits(aRect.width),
              DevPixelsToAppUnits(aRect.height));
  NotifyInvalidation(rect, aFlags);
}

void
nsPresContext::NotifyInvalidation(const nsRect& aRect, uint32_t aFlags)
{
  MOZ_ASSERT(GetContainerWeak(), "Invalidation in detached pres context");

  
  
  
  
  

  if (mAllInvalidated) {
    return;
  }

  nsPresContext* pc;
  for (pc = this; pc; pc = pc->GetParentPresContext()) {
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
    mInvalidateRequestsSinceLastPaint.mRequests.AppendElement();
  if (!request)
    return;

  request->mRect = aRect;
  request->mFlags = aFlags;
}

 void
nsPresContext::NotifySubDocInvalidation(ContainerLayer* aContainer,
                                        const nsIntRegion& aRegion)
{
  ContainerLayerPresContext *data =
    static_cast<ContainerLayerPresContext*>(
      aContainer->GetUserData(&gNotifySubDocInvalidationData));
  if (!data) {
    return;
  }

  nsIntPoint topLeft = aContainer->GetVisibleRegion().GetBounds().TopLeft();

  nsIntRegionRectIterator iter(aRegion);
  while (const nsIntRect* r = iter.Next()) {
    nsIntRect rect = *r;
    
    
    
    rect.MoveBy(-topLeft);
    data->mPresContext->NotifyInvalidation(rect, 0);
  }
}

void
nsPresContext::SetNotifySubDocInvalidationData(ContainerLayer* aContainer)
{
  ContainerLayerPresContext* pres = new ContainerLayerPresContext;
  pres->mPresContext = this;
  aContainer->SetUserData(&gNotifySubDocInvalidationData, pres);
}

 void
nsPresContext::ClearNotifySubDocInvalidationData(ContainerLayer* aContainer)
{
  aContainer->SetUserData(&gNotifySubDocInvalidationData, nullptr);
}

struct NotifyDidPaintSubdocumentCallbackClosure {
  uint32_t mFlags;
  bool mNeedsAnotherDidPaintNotification;
};
static bool
NotifyDidPaintSubdocumentCallback(nsIDocument* aDocument, void* aData)
{
  NotifyDidPaintSubdocumentCallbackClosure* closure =
    static_cast<NotifyDidPaintSubdocumentCallbackClosure*>(aData);
  nsIPresShell* shell = aDocument->GetShell();
  if (shell) {
    nsPresContext* pc = shell->GetPresContext();
    if (pc) {
      pc->NotifyDidPaintForSubtree(closure->mFlags);
      if (pc->IsDOMPaintEventPending()) {
        closure->mNeedsAnotherDidPaintNotification = true;
      }
    }
  }
  return true;
}

class DelayedFireDOMPaintEvent : public nsRunnable {
public:
  DelayedFireDOMPaintEvent(nsPresContext* aPresContext,
                           nsInvalidateRequestList* aList)
    : mPresContext(aPresContext)
  {
    MOZ_ASSERT(mPresContext->GetContainerWeak(),
               "DOMPaintEvent requested for a detached pres context");
    mList.TakeFrom(aList);
  }
  NS_IMETHOD Run() override
  {
    
    
    if (mPresContext->GetContainerWeak()) {
      mPresContext->FireDOMPaintEvent(&mList);
    }
    return NS_OK;
  }

  nsRefPtr<nsPresContext> mPresContext;
  nsInvalidateRequestList mList;
};

void
nsPresContext::NotifyDidPaintForSubtree(uint32_t aFlags)
{
  if (IsRoot()) {
    static_cast<nsRootPresContext*>(this)->CancelDidPaintTimer();

    if (!mFireAfterPaintEvents) {
      return;
    }
  }

  if (!PresShell()->IsVisible() && !mFireAfterPaintEvents) {
    return;
  }

  
  
  
  
  

  if (aFlags & nsIPresShell::PAINT_LAYERS) {
    mUndeliveredInvalidateRequestsBeforeLastPaint.TakeFrom(
        &mInvalidateRequestsSinceLastPaint);
    mAllInvalidated = false;
  }
  if (aFlags & nsIPresShell::PAINT_COMPOSITE) {
    nsCOMPtr<nsIRunnable> ev =
      new DelayedFireDOMPaintEvent(this, &mUndeliveredInvalidateRequestsBeforeLastPaint);
    nsContentUtils::AddScriptRunner(ev);
  }

  NotifyDidPaintSubdocumentCallbackClosure closure = { aFlags, false };
  mDocument->EnumerateSubDocuments(NotifyDidPaintSubdocumentCallback, &closure);

  if (!closure.mNeedsAnotherDidPaintNotification &&
      mInvalidateRequestsSinceLastPaint.IsEmpty() &&
      mUndeliveredInvalidateRequestsBeforeLastPaint.IsEmpty()) {
    
    mFireAfterPaintEvents = false;
  } else {
    if (IsRoot()) {
      static_cast<nsRootPresContext*>(this)->EnsureEventualDidPaintEvent();
    }
  }
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
#ifndef XP_WIN


static uint32_t sInterruptSeed = 1;
#endif



static uint32_t sInterruptMaxCounter = 10;


static uint32_t sInterruptCounter;


static uint32_t sInterruptChecksToSkip = 200;




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
nsPresContext::NotifyFontFaceSetOnRefresh()
{
  if (mFontFaceSet) {
    mFontFaceSet->DidRefresh();
  }
}

bool
nsPresContext::HasPendingRestyleOrReflow()
{
  return (mRestyleManager && mRestyleManager->HasPendingRestyles()) ||
         PresShell()->HasPendingReflow();
}

void
nsPresContext::ReflowStarted(bool aInterruptible)
{
#ifdef NOISY_INTERRUPTIBLE_REFLOW
  if (!aInterruptible) {
    printf("STARTING NONINTERRUPTIBLE REFLOW\n");
  }
#endif
  
  
  mInterruptsEnabled = aInterruptible && !IsPaginated() &&
                       nsLayoutUtils::InterruptibleReflowEnabled();

  
  
  
  
  
  
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

nsIFrame*
nsPresContext::GetPrimaryFrameFor(nsIContent* aContent)
{
  NS_PRECONDITION(aContent, "Don't do that");
  if (GetPresShell() &&
      GetPresShell()->GetDocument() == aContent->GetComposedDoc()) {
    return aContent->GetPrimaryFrame();
  }
  return nullptr;
}


size_t
nsPresContext::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
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
  
  nsView* view = PresShell()->GetViewManager()->GetRootView();
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

bool
nsPresContext::IsCrossProcessRootContentDocument()
{
  if (!IsRootContentDocument()) {
    return false;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    return true;
  }

  TabChild* tabChild = TabChild::GetFrom(mShell);
  return (tabChild && tabChild->IsRootContentDocument());
}

bool nsPresContext::GetPaintFlashing() const
{
  if (!mPaintFlashingInitialized) {
    bool pref = Preferences::GetBool("nglayout.debug.paint_flashing");
    if (!pref && IsChrome()) {
      pref = Preferences::GetBool("nglayout.debug.paint_flashing_chrome");
    }
    mPaintFlashing = pref;
    mPaintFlashingInitialized = true;
  }
  return mPaintFlashing;
}

int32_t
nsPresContext::AppUnitsPerDevPixel() const
{
  return mDeviceContext->AppUnitsPerDevPixel();
}

nscoord
nsPresContext::GfxUnitsToAppUnits(gfxFloat aGfxUnits) const
{
  return mDeviceContext->GfxUnitsToAppUnits(aGfxUnits);
}

gfxFloat
nsPresContext::AppUnitsToGfxUnits(nscoord aAppUnits) const
{
  return mDeviceContext->AppUnitsToGfxUnits(aAppUnits);
}

bool
nsPresContext::IsDeviceSizePageSize()
{
  bool isDeviceSizePageSize = false;
  nsCOMPtr<nsIDocShell> docShell(mContainer);
  if (docShell) {
    isDeviceSizePageSize = docShell->GetDeviceSizeIsPageSize();
  }
  return isDeviceSizePageSize;
}

nsRootPresContext::nsRootPresContext(nsIDocument* aDocument,
                                     nsPresContextType aType)
  : nsPresContext(aDocument, aType),
    mDOMGeneration(0)
{
}

nsRootPresContext::~nsRootPresContext()
{
  NS_ASSERTION(mRegisteredPlugins.Count() == 0,
               "All plugins should have been unregistered");
  CancelDidPaintTimer();
  CancelApplyPluginGeometryTimer();
}

 void
nsRootPresContext::Detach()
{
  CancelDidPaintTimer();
  
  nsPresContext::Detach();
}

void
nsRootPresContext::RegisterPluginForGeometryUpdates(nsIContent* aPlugin)
{
  mRegisteredPlugins.PutEntry(aPlugin);
}

void
nsRootPresContext::UnregisterPluginForGeometryUpdates(nsIContent* aPlugin)
{
  mRegisteredPlugins.RemoveEntry(aPlugin);
}

static PLDHashOperator
SetPluginHidden(nsRefPtrHashKey<nsIContent>* aEntry, void* userArg)
{
  nsIFrame* root = static_cast<nsIFrame*>(userArg);
  nsPluginFrame* f = static_cast<nsPluginFrame*>(aEntry->GetKey()->GetPrimaryFrame());
  if (!f) {
    NS_WARNING("Null frame in SetPluginHidden");
    return PL_DHASH_NEXT;
  }
  if (!nsLayoutUtils::IsAncestorFrameCrossDoc(root, f)) {
    
    return PL_DHASH_NEXT;
  }
  f->SetEmptyWidgetConfiguration();
  return PL_DHASH_NEXT;
}

void
nsRootPresContext::ComputePluginGeometryUpdates(nsIFrame* aFrame,
                                                nsDisplayListBuilder* aBuilder,
                                                nsDisplayList* aList)
{
  if (mRegisteredPlugins.Count() == 0) {
    return;
  }

  
  
  
  mRegisteredPlugins.EnumerateEntries(SetPluginHidden, aFrame);

  nsIFrame* rootFrame = FrameManager()->GetRootFrame();

  if (rootFrame && aBuilder->ContainsPluginItem()) {
    aBuilder->SetForPluginGeometry();
    aBuilder->SetAccurateVisibleRegions();
    
    
    
    aBuilder->SetAllowMergingAndFlattening(false);
    nsRegion region = rootFrame->GetVisualOverflowRectRelativeToSelf();
    
    
    aList->ComputeVisibilityForRoot(aBuilder, &region);
  }

#ifdef XP_MACOSX
  
  
  ApplyPluginGeometryUpdates();
#else
  if (XRE_GetProcessType() == GeckoProcessType_Default) {
    InitApplyPluginGeometryTimer();
  }
#endif
}

static void
ApplyPluginGeometryUpdatesCallback(nsITimer *aTimer, void *aClosure)
{
  static_cast<nsRootPresContext*>(aClosure)->ApplyPluginGeometryUpdates();
}

void
nsRootPresContext::InitApplyPluginGeometryTimer()
{
  if (mApplyPluginGeometryTimer) {
    return;
  }

  
  
  
  
  
  
  
  mApplyPluginGeometryTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mApplyPluginGeometryTimer) {
    mApplyPluginGeometryTimer->
      InitWithFuncCallback(ApplyPluginGeometryUpdatesCallback, this,
                           nsRefreshDriver::DefaultInterval() * 2,
                           nsITimer::TYPE_ONE_SHOT);
  }
}

void
nsRootPresContext::CancelApplyPluginGeometryTimer()
{
  if (mApplyPluginGeometryTimer) {
    mApplyPluginGeometryTimer->Cancel();
    mApplyPluginGeometryTimer = nullptr;
  }
}

#ifndef XP_MACOSX

static bool
HasOverlap(const nsIntPoint& aOffset1, const nsTArray<nsIntRect>& aClipRects1,
           const nsIntPoint& aOffset2, const nsTArray<nsIntRect>& aClipRects2)
{
  nsIntPoint offsetDelta = aOffset1 - aOffset2;
  for (uint32_t i = 0; i < aClipRects1.Length(); ++i) {
    for (uint32_t j = 0; j < aClipRects2.Length(); ++j) {
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
    
    uint32_t i;
    for (i = 0; i + 1 < pluginsToMove.Length(); ++i) {
      nsIWidget::Configuration* config = &pluginsToMove[i];
      bool foundOverlap = false;
      for (uint32_t j = 0; j < pluginsToMove.Length(); ++j) {
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

struct PluginGetGeometryUpdateClosure {
  nsTArray<nsIWidget::Configuration> mConfigurations;
};
static PLDHashOperator
PluginGetGeometryUpdate(nsRefPtrHashKey<nsIContent>* aEntry, void* userArg)
{
  PluginGetGeometryUpdateClosure* closure =
    static_cast<PluginGetGeometryUpdateClosure*>(userArg);
  nsPluginFrame* f = static_cast<nsPluginFrame*>(aEntry->GetKey()->GetPrimaryFrame());
  if (!f) {
    NS_WARNING("Null frame in GetPluginGeometryUpdate");
    return PL_DHASH_NEXT;
  }
  f->GetWidgetConfiguration(&closure->mConfigurations);
  return PL_DHASH_NEXT;
}

#endif  

static PLDHashOperator
PluginDidSetGeometryEnumerator(nsRefPtrHashKey<nsIContent>* aEntry, void* userArg)
{
  nsPluginFrame* f = static_cast<nsPluginFrame*>(aEntry->GetKey()->GetPrimaryFrame());
  if (!f) {
    NS_WARNING("Null frame in PluginDidSetGeometryEnumerator");
    return PL_DHASH_NEXT;
  }
  f->DidSetWidgetGeometry();
  return PL_DHASH_NEXT;
}

void
nsRootPresContext::ApplyPluginGeometryUpdates()
{
#ifndef XP_MACOSX
  CancelApplyPluginGeometryTimer();

  PluginGetGeometryUpdateClosure closure;
  mRegisteredPlugins.EnumerateEntries(PluginGetGeometryUpdate, &closure);
  
  if (!closure.mConfigurations.IsEmpty()) {
    nsIWidget* widget = closure.mConfigurations[0].mChild->GetParent();
    NS_ASSERTION(widget, "Plugins must have a parent window");
    SortConfigurations(&closure.mConfigurations);
    widget->ConfigureChildren(closure.mConfigurations);
  }
#endif  

  mRegisteredPlugins.EnumerateEntries(PluginDidSetGeometryEnumerator, nullptr);
}

void
nsRootPresContext::CollectPluginGeometryUpdates(LayerManager* aLayerManager)
{
#ifndef XP_MACOSX
  
  
  NS_ASSERTION(aLayerManager, "layer manager is invalid!");
  mozilla::layers::ClientLayerManager* clm = aLayerManager->AsClientLayerManager();
  PluginGetGeometryUpdateClosure closure;
  mRegisteredPlugins.EnumerateEntries(PluginGetGeometryUpdate, &closure);
  if (closure.mConfigurations.IsEmpty()) {
    mRegisteredPlugins.EnumerateEntries(PluginDidSetGeometryEnumerator, nullptr);
    return;
  }
  SortConfigurations(&closure.mConfigurations);
  if (clm) {
    clm->StorePluginWidgetConfigurations(closure.mConfigurations);
  }
  mRegisteredPlugins.EnumerateEntries(PluginDidSetGeometryEnumerator, nullptr);
#endif  
}

static void
NotifyDidPaintForSubtreeCallback(nsITimer *aTimer, void *aClosure)
{
  nsPresContext* presContext = (nsPresContext*)aClosure;
  nsAutoScriptBlocker blockScripts;
  
  
  presContext->NotifyDidPaintForSubtree(
      nsIPresShell::PAINT_LAYERS | nsIPresShell::PAINT_COMPOSITE);
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
  mWillPaintFallbackEvent = nullptr;
  nsTArray<nsCOMPtr<nsIRunnable> > observers;
  observers.SwapElements(mWillPaintObservers);
  for (uint32_t i = 0; i < observers.Length(); ++i) {
    observers[i]->Run();
  }
}

size_t
nsRootPresContext::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  return nsPresContext::SizeOfExcludingThis(aMallocSizeOf);

  
  
  
  
  
  
}
