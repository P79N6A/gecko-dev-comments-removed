





#include "FontFaceSet.h"

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "mozilla/dom/FontFaceSetBinding.h"
#include "mozilla/dom/Promise.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsFontFaceLoader.h"
#include "nsIChannelPolicy.h"
#include "nsIConsoleService.h"
#include "nsIContentPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsINetworkPredictor.h"
#include "nsIPresShell.h"
#include "nsIPrincipal.h"
#include "nsISupportsPriority.h"
#include "nsIWebNavigation.h"
#include "nsNetUtil.h"
#include "nsPresContext.h"
#include "nsPrintfCString.h"
#include "nsStyleSet.h"

using namespace mozilla;
using namespace mozilla::dom;

#ifdef PR_LOGGING
static PRLogModuleInfo*
GetFontFaceSetLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog)
    sLog = PR_NewLogModule("fontfaceset");
  return sLog;
}
#endif 

#define LOG(args) PR_LOG(GetFontFaceSetLog(), PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(GetFontFaceSetLog(), PR_LOG_DEBUG)

NS_IMPL_CYCLE_COLLECTION_INHERITED(FontFaceSet,
                                   DOMEventTargetHelper,
                                   mReady)

NS_IMPL_ADDREF_INHERITED(FontFaceSet, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(FontFaceSet, DOMEventTargetHelper)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(FontFaceSet)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

FontFaceSet::FontFaceSet(nsPIDOMWindow* aWindow, nsPresContext* aPresContext)
  : DOMEventTargetHelper(aWindow)
  , mPresContext(aPresContext)
{
  MOZ_COUNT_CTOR(FontFaceSet);

  MOZ_ASSERT(mPresContext);
}

FontFaceSet::~FontFaceSet()
{
  MOZ_COUNT_DTOR(FontFaceSet);

  NS_ASSERTION(mLoaders.Count() == 0, "mLoaders should have been emptied");
}

JSObject*
FontFaceSet::WrapObject(JSContext* aContext)
{
  return FontFaceSetBinding::Wrap(aContext, this);
}

FontFaceSet::UserFontSet*
FontFaceSet::EnsureUserFontSet(nsPresContext* aPresContext)
{
  if (!mUserFontSet) {
    mUserFontSet = new UserFontSet(this);
    mPresContext = aPresContext;
  }
  return mUserFontSet;
}

already_AddRefed<Promise>
FontFaceSet::Load(const nsAString& aFont,
                  const nsAString& aText,
                  ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return nullptr;
}

bool
FontFaceSet::Check(const nsAString& aFont,
                   const nsAString& aText,
                   ErrorResult& aRv)
{
  aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
  return false;
}

Promise*
FontFaceSet::GetReady(ErrorResult& aRv)
{
  if (!mReady) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  return mReady;
}

FontFaceSetLoadStatus
FontFaceSet::Status()
{
  return FontFaceSetLoadStatus::Loaded;
}

FontFaceSet*
FontFaceSet::Add(FontFace& aFontFace, ErrorResult& aRv)
{
  return this;
}

void
FontFaceSet::Clear()
{
}

bool
FontFaceSet::Delete(FontFace& aFontFace, ErrorResult& aRv)
{
  return false;
}

bool
FontFaceSet::Has(FontFace& aFontFace)
{
  return false;
}

FontFace*
FontFaceSet::IndexedGetter(uint32_t aIndex, bool& aFound)
{
  aFound = false;
  return nullptr;
}

uint32_t
FontFaceSet::Length()
{
  return 0;
}

static PLDHashOperator DestroyIterator(nsPtrHashKey<nsFontFaceLoader>* aKey,
                                       void* aUserArg)
{
  aKey->GetKey()->Cancel();
  return PL_DHASH_REMOVE;
}

void
FontFaceSet::DestroyUserFontSet()
{
  mPresContext = nullptr;
  mLoaders.EnumerateEntries(DestroyIterator, nullptr);
  mRules.Clear();
  mUserFontSet = nullptr;
}

void
FontFaceSet::RemoveLoader(nsFontFaceLoader* aLoader)
{
  mLoaders.RemoveEntry(aLoader);
}

nsresult
FontFaceSet::StartLoad(gfxUserFontEntry* aUserFontEntry,
                       const gfxFontFaceSrc* aFontFaceSrc)
{
  nsresult rv;

  nsIPresShell* ps = mPresContext->PresShell();
  if (!ps)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStreamLoader> streamLoader;
  nsCOMPtr<nsILoadGroup> loadGroup(ps->GetDocument()->GetDocumentLoadGroup());

  nsCOMPtr<nsIChannel> channel;
  
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = aUserFontEntry->GetPrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);
  if (csp) {
    channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_FONT);
  }
  
  
  
  
  rv = NS_NewChannelInternal(getter_AddRefs(channel),
                             aFontFaceSrc->mURI,
                             ps->GetDocument(),
                             aUserFontEntry->GetPrincipal(),
                             nsILoadInfo::SEC_NORMAL,
                             nsIContentPolicy::TYPE_FONT,
                             channelPolicy,
                             loadGroup);

  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsFontFaceLoader> fontLoader =
    new nsFontFaceLoader(aUserFontEntry, aFontFaceSrc->mURI, this, channel);

  if (!fontLoader)
    return NS_ERROR_OUT_OF_MEMORY;

#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsAutoCString fontURI, referrerURI;
    aFontFaceSrc->mURI->GetSpec(fontURI);
    if (aFontFaceSrc->mReferrer)
      aFontFaceSrc->mReferrer->GetSpec(referrerURI);
    LOG(("fontdownloader (%p) download start - font uri: (%s) "
         "referrer uri: (%s)\n",
         fontLoader.get(), fontURI.get(), referrerURI.get()));
  }
#endif

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel)
    httpChannel->SetReferrer(aFontFaceSrc->mReferrer);
  nsCOMPtr<nsISupportsPriority> priorityChannel(do_QueryInterface(channel));
  if (priorityChannel) {
    priorityChannel->AdjustPriority(nsISupportsPriority::PRIORITY_HIGH);
  }

  rv = NS_NewStreamLoader(getter_AddRefs(streamLoader), fontLoader);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIDocument *document = ps->GetDocument();
  mozilla::net::PredictorLearn(aFontFaceSrc->mURI, document->GetDocumentURI(),
                               nsINetworkPredictor::LEARN_LOAD_SUBRESOURCE,
                               loadGroup);

  bool inherits = false;
  rv = NS_URIChainHasFlags(aFontFaceSrc->mURI,
                           nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT,
                           &inherits);
  if (NS_SUCCEEDED(rv) && inherits) {
    
    rv = channel->AsyncOpen(streamLoader, nullptr);
  } else {
    nsRefPtr<nsCORSListenerProxy> listener =
      new nsCORSListenerProxy(streamLoader, aUserFontEntry->GetPrincipal(), false);
    rv = listener->Init(channel);
    if (NS_SUCCEEDED(rv)) {
      rv = channel->AsyncOpen(listener, nullptr);
    }
    if (NS_FAILED(rv)) {
      fontLoader->DropChannel();  
    }
  }

  if (NS_SUCCEEDED(rv)) {
    mLoaders.PutEntry(fontLoader);
    fontLoader->StartedLoading(streamLoader);
    aUserFontEntry->SetLoader(fontLoader); 
                                           
  }

  return rv;
}

static PLDHashOperator DetachFontEntries(const nsAString& aKey,
                                         nsRefPtr<gfxUserFontFamily>& aFamily,
                                         void* aUserArg)
{
  aFamily->DetachFontEntries();
  return PL_DHASH_NEXT;
}

static PLDHashOperator RemoveIfEmpty(const nsAString& aKey,
                                     nsRefPtr<gfxUserFontFamily>& aFamily,
                                     void* aUserArg)
{
  return aFamily->GetFontList().Length() ? PL_DHASH_NEXT : PL_DHASH_REMOVE;
}

bool
FontFaceSet::UpdateRules(const nsTArray<nsFontFaceRuleContainer>& aRules)
{
  MOZ_ASSERT(mUserFontSet);

  bool modified = false;

  
  
  
  
  

  nsTArray<FontFaceRuleRecord> oldRules;
  mRules.SwapElements(oldRules);

  
  
  
  
  
  mUserFontSet->mFontFamilies.Enumerate(DetachFontEntries, nullptr);

  for (uint32_t i = 0, i_end = aRules.Length(); i < i_end; ++i) {
    
    
    
    InsertRule(aRules[i].mRule, aRules[i].mSheetType, oldRules, modified);
  }

  
  
  mUserFontSet->mFontFamilies.Enumerate(RemoveIfEmpty, nullptr);

  
  
  if (oldRules.Length() > 0) {
    modified = true;
    
    
    
    
    
    size_t count = oldRules.Length();
    for (size_t i = 0; i < count; ++i) {
      gfxUserFontEntry* userFontEntry = oldRules[i].mUserFontEntry;
      nsFontFaceLoader* loader = userFontEntry->GetLoader();
      if (loader) {
        loader->Cancel();
        RemoveLoader(loader);
      }
    }
  }

  if (modified) {
    IncrementGeneration(true);
  }

  
  mUserFontSet->mLocalRulesUsed = false;

  return modified;
}

static bool
HasLocalSrc(const nsCSSValue::Array *aSrcArr)
{
  size_t numSrc = aSrcArr->Count();
  for (size_t i = 0; i < numSrc; i++) {
    if (aSrcArr->Item(i).GetUnit() == eCSSUnit_Local_Font) {
      return true;
    }
  }
  return false;
}

void
FontFaceSet::IncrementGeneration(bool aIsRebuild)
{
  MOZ_ASSERT(mUserFontSet);
  mUserFontSet->IncrementGeneration(aIsRebuild);
}

void
FontFaceSet::InsertRule(nsCSSFontFaceRule* aRule, uint8_t aSheetType,
                        nsTArray<FontFaceRuleRecord>& aOldRules,
                        bool& aFontSetModified)
{
  NS_ABORT_IF_FALSE(aRule->GetType() == mozilla::css::Rule::FONT_FACE_RULE,
                    "InsertRule passed a non-fontface CSS rule");

  
  nsAutoString fontfamily;
  nsCSSValue val;
  uint32_t unit;

  aRule->GetDesc(eCSSFontDesc_Family, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_String) {
    val.GetStringValue(fontfamily);
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null,
                 "@font-face family name has unexpected unit");
  }
  if (fontfamily.IsEmpty()) {
    
    
    return;
  }

  
  
  for (uint32_t i = 0; i < aOldRules.Length(); ++i) {
    const FontFaceRuleRecord& ruleRec = aOldRules[i];

    if (ruleRec.mContainer.mRule == aRule &&
        ruleRec.mContainer.mSheetType == aSheetType) {

      
      
      if (mUserFontSet->mLocalRulesUsed) {
        aRule->GetDesc(eCSSFontDesc_Src, val);
        unit = val.GetUnit();
        if (unit == eCSSUnit_Array && HasLocalSrc(val.GetArrayValue())) {
          break;
        }
      }

      mUserFontSet->AddFontFace(fontfamily, ruleRec.mUserFontEntry);
      mRules.AppendElement(ruleRec);
      aOldRules.RemoveElementAt(i);
      
      
      if (i > 0) {
        aFontSetModified = true;
      }
      return;
    }
  }

  
  FontFaceRuleRecord ruleRec;
  ruleRec.mUserFontEntry =
    FindOrCreateFontFaceFromRule(fontfamily, aRule, aSheetType);

  if (!ruleRec.mUserFontEntry) {
    return;
  }

  ruleRec.mContainer.mRule = aRule;
  ruleRec.mContainer.mSheetType = aSheetType;

  
  
  
  
  mUserFontSet->AddFontFace(fontfamily, ruleRec.mUserFontEntry);

  mRules.AppendElement(ruleRec);

  
  aFontSetModified = true;
}

already_AddRefed<gfxUserFontEntry>
FontFaceSet::FindOrCreateFontFaceFromRule(const nsAString& aFamilyName,
                                          nsCSSFontFaceRule* aRule,
                                          uint8_t aSheetType)
{
  nsCSSValue val;
  uint32_t unit;

  uint32_t weight = NS_STYLE_FONT_WEIGHT_NORMAL;
  int32_t stretch = NS_STYLE_FONT_STRETCH_NORMAL;
  uint32_t italicStyle = NS_STYLE_FONT_STYLE_NORMAL;
  uint32_t languageOverride = NO_FONT_LANGUAGE_OVERRIDE;

  
  aRule->GetDesc(eCSSFontDesc_Weight, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Integer || unit == eCSSUnit_Enumerated) {
    weight = val.GetIntValue();
    if (weight == 0) {
      weight = NS_STYLE_FONT_WEIGHT_NORMAL;
    }
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
    italicStyle = NS_STYLE_FONT_STYLE_NORMAL;
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null,
                 "@font-face style has unexpected unit");
  }

  
  nsTArray<gfxFontFeature> featureSettings;
  aRule->GetDesc(eCSSFontDesc_FontFeatureSettings, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Normal) {
    
  } else if (unit == eCSSUnit_PairList || unit == eCSSUnit_PairListDep) {
    nsRuleNode::ComputeFontFeatures(val.GetPairListValue(), featureSettings);
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null,
                 "@font-face font-feature-settings has unexpected unit");
  }

  
  aRule->GetDesc(eCSSFontDesc_FontLanguageOverride, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Normal) {
    
  } else if (unit == eCSSUnit_String) {
    nsString stringValue;
    val.GetStringValue(stringValue);
    languageOverride = gfxFontStyle::ParseFontLanguageOverride(stringValue);
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null,
                 "@font-face font-language-override has unexpected unit");
  }

  
  nsTArray<gfxFontFaceSrc> srcArray;

  aRule->GetDesc(eCSSFontDesc_Src, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Array) {
    nsCSSValue::Array* srcArr = val.GetArrayValue();
    size_t numSrc = srcArr->Count();

    for (size_t i = 0; i < numSrc; i++) {
      val = srcArr->Item(i);
      unit = val.GetUnit();
      gfxFontFaceSrc* face = srcArray.AppendElements(1);
      if (!face)
        return nullptr;

      switch (unit) {

      case eCSSUnit_Local_Font:
        val.GetStringValue(face->mLocalName);
        face->mIsLocal = true;
        face->mURI = nullptr;
        face->mFormatFlags = 0;
        break;
      case eCSSUnit_URL:
        face->mIsLocal = false;
        face->mURI = val.GetURLValue();
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
          if (valueString.LowerCaseEqualsASCII("woff")) {
            face->mFormatFlags |= gfxUserFontSet::FLAG_FORMAT_WOFF;
          } else if (valueString.LowerCaseEqualsASCII("opentype")) {
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
        if (!face->mURI) {
          
          srcArray.RemoveElementAt(srcArray.Length() - 1);
          NS_WARNING("null url in @font-face rule");
          continue;
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

  if (srcArray.IsEmpty()) {
    return nullptr;
  }

  nsRefPtr<gfxUserFontEntry> entry =
    mUserFontSet->FindOrCreateFontFace(aFamilyName, srcArray, weight, stretch,
                                       italicStyle, featureSettings,
                                       languageOverride,
                                       nullptr );
  return entry.forget();
}

nsCSSFontFaceRule*
FontFaceSet::FindRuleForEntry(gfxFontEntry* aFontEntry)
{
  NS_ASSERTION(!aFontEntry->mIsUserFontContainer, "only platform font entries");
  for (uint32_t i = 0; i < mRules.Length(); ++i) {
    if (mRules[i].mUserFontEntry->GetPlatformFontEntry() == aFontEntry) {
      return mRules[i].mContainer.mRule;
    }
  }
  return nullptr;
}

nsCSSFontFaceRule*
FontFaceSet::FindRuleForUserFontEntry(gfxUserFontEntry* aUserFontEntry)
{
  for (uint32_t i = 0; i < mRules.Length(); ++i) {
    if (mRules[i].mUserFontEntry == aUserFontEntry) {
      return mRules[i].mContainer.mRule;
    }
  }
  return nullptr;
}

nsresult
FontFaceSet::LogMessage(gfxUserFontEntry* aUserFontEntry,
                        const char* aMessage,
                        uint32_t aFlags,
                        nsresult aStatus)
{
  nsCOMPtr<nsIConsoleService>
    console(do_GetService(NS_CONSOLESERVICE_CONTRACTID));
  if (!console) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsAutoCString familyName;
  nsAutoCString fontURI;
  aUserFontEntry->GetFamilyNameAndURIForLogging(familyName, fontURI);

  char weightKeywordBuf[8]; 
  const char* weightKeyword;
  const nsAFlatCString& weightKeywordString =
    nsCSSProps::ValueToKeyword(aUserFontEntry->Weight(),
                               nsCSSProps::kFontWeightKTable);
  if (weightKeywordString.Length() > 0) {
    weightKeyword = weightKeywordString.get();
  } else {
    sprintf(weightKeywordBuf, "%u", aUserFontEntry->Weight());
    weightKeyword = weightKeywordBuf;
  }

  nsPrintfCString message
       ("downloadable font: %s "
        "(font-family: \"%s\" style:%s weight:%s stretch:%s src index:%d)",
        aMessage,
        familyName.get(),
        aUserFontEntry->IsItalic() ? "italic" : "normal",
        weightKeyword,
        nsCSSProps::ValueToKeyword(aUserFontEntry->Stretch(),
                                   nsCSSProps::kFontStretchKTable).get(),
        aUserFontEntry->GetSrcIndex());

  if (NS_FAILED(aStatus)) {
    message.AppendLiteral(": ");
    switch (aStatus) {
    case NS_ERROR_DOM_BAD_URI:
      message.AppendLiteral("bad URI or cross-site access not allowed");
      break;
    case NS_ERROR_CONTENT_BLOCKED:
      message.AppendLiteral("content blocked");
      break;
    default:
      message.AppendLiteral("status=");
      message.AppendInt(static_cast<uint32_t>(aStatus));
      break;
    }
  }
  message.AppendLiteral("\nsource: ");
  message.Append(fontURI);

#ifdef PR_LOGGING
  if (PR_LOG_TEST(GetFontFaceSetLog(), PR_LOG_DEBUG)) {
    PR_LOG(GetFontFaceSetLog(), PR_LOG_DEBUG,
           ("userfonts (%p) %s", this, message.get()));
  }
#endif

  
  nsCSSFontFaceRule* rule = FindRuleForUserFontEntry(aUserFontEntry);
  nsString href;
  nsString text;
  nsresult rv;
  if (rule) {
    rv = rule->GetCssText(text);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDOMCSSStyleSheet> sheet;
    rv = rule->GetParentStyleSheet(getter_AddRefs(sheet));
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (sheet) {
      rv = sheet->GetHref(href);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      NS_WARNING("null parent stylesheet for @font-face rule");
      href.AssignLiteral("unknown");
    }
  }

  nsCOMPtr<nsIScriptError> scriptError =
    do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  uint64_t innerWindowID = GetPresContext()->Document()->InnerWindowID();
  rv = scriptError->InitWithWindowID(NS_ConvertUTF8toUTF16(message),
                                     href,         
                                     text,         
                                     0, 0,         
                                     aFlags,       
                                     "CSS Loader", 
                                     innerWindowID);
  if (NS_SUCCEEDED(rv)) {
    console->LogMessage(scriptError);
  }

  return NS_OK;
}

nsresult
FontFaceSet::CheckFontLoad(const gfxFontFaceSrc* aFontFaceSrc,
                           nsIPrincipal** aPrincipal,
                           bool* aBypassCache)
{
  
  nsIPresShell* ps = mPresContext->PresShell();
  if (!ps)
    return NS_ERROR_FAILURE;

  NS_ASSERTION(aFontFaceSrc && !aFontFaceSrc->mIsLocal,
               "bad font face url passed to fontloader");
  NS_ASSERTION(aFontFaceSrc->mURI, "null font uri");
  if (!aFontFaceSrc->mURI)
    return NS_ERROR_FAILURE;

  
  
  
  *aPrincipal = ps->GetDocument()->NodePrincipal();

  NS_ASSERTION(aFontFaceSrc->mOriginPrincipal,
               "null origin principal in @font-face rule");
  if (aFontFaceSrc->mUseOriginPrincipal) {
    *aPrincipal = aFontFaceSrc->mOriginPrincipal;
  }

  nsresult rv = nsFontFaceLoader::CheckLoadAllowed(*aPrincipal,
                                                   aFontFaceSrc->mURI,
                                                   ps->GetDocument());
  if (NS_FAILED(rv)) {
    return rv;
  }

  *aBypassCache = false;

  nsCOMPtr<nsIDocShell> docShell = ps->GetDocument()->GetDocShell();
  if (docShell) {
    uint32_t loadType;
    if (NS_SUCCEEDED(docShell->GetLoadType(&loadType))) {
      if ((loadType >> 16) & nsIWebNavigation::LOAD_FLAGS_BYPASS_CACHE) {
        *aBypassCache = true;
      }
    }
  }

  return rv;
}

nsresult
FontFaceSet::SyncLoadFontData(gfxUserFontEntry* aFontToLoad,
                              const gfxFontFaceSrc* aFontFaceSrc,
                              uint8_t*& aBuffer,
                              uint32_t& aBufferLength)
{
  nsresult rv;

  nsCOMPtr<nsIChannel> channel;
  
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = aFontToLoad->GetPrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);
  if (csp) {
    channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_FONT);
  }

  nsIPresShell* ps = mPresContext->PresShell();
  if (!ps) {
    return NS_ERROR_FAILURE;
  }
  
  
  
  
  rv = NS_NewChannelInternal(getter_AddRefs(channel),
                             aFontFaceSrc->mURI,
                             ps->GetDocument(),
                             aFontToLoad->GetPrincipal(),
                             nsILoadInfo::SEC_NORMAL,
                             nsIContentPolicy::TYPE_FONT,
                             channelPolicy);

  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIInputStream> stream;
  rv = channel->Open(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  uint64_t bufferLength64;
  rv = stream->Available(&bufferLength64);
  NS_ENSURE_SUCCESS(rv, rv);
  if (bufferLength64 == 0) {
    return NS_ERROR_FAILURE;
  }
  if (bufferLength64 > UINT32_MAX) {
    return NS_ERROR_FILE_TOO_BIG;
  }
  aBufferLength = static_cast<uint32_t>(bufferLength64);

  
  aBuffer = static_cast<uint8_t*> (NS_Alloc(sizeof(uint8_t) * aBufferLength));
  if (!aBuffer) {
    aBufferLength = 0;
    return NS_ERROR_OUT_OF_MEMORY;
  }

  uint32_t numRead, totalRead = 0;
  while (NS_SUCCEEDED(rv =
           stream->Read(reinterpret_cast<char*>(aBuffer + totalRead),
                        aBufferLength - totalRead, &numRead)) &&
         numRead != 0)
  {
    totalRead += numRead;
    if (totalRead > aBufferLength) {
      rv = NS_ERROR_FAILURE;
      break;
    }
  }

  
  if (NS_SUCCEEDED(rv)) {
    nsAutoCString mimeType;
    rv = channel->GetContentType(mimeType);
    aBufferLength = totalRead;
  }

  if (NS_FAILED(rv)) {
    NS_Free(aBuffer);
    aBuffer = nullptr;
    aBufferLength = 0;
    return rv;
  }

  return NS_OK;
}

bool
FontFaceSet::GetPrivateBrowsing()
{
  nsIPresShell* ps = mPresContext->PresShell();
  if (!ps) {
    return false;
  }

  nsCOMPtr<nsILoadContext> loadContext = ps->GetDocument()->GetLoadContext();
  return loadContext && loadContext->UsePrivateBrowsing();
}

void
FontFaceSet::DoRebuildUserFontSet()
{
  if (!mPresContext) {
    
    
    
    return;
  }

  mPresContext->RebuildUserFontSet();
}



 nsresult
FontFaceSet::UserFontSet::CheckFontLoad(const gfxFontFaceSrc* aFontFaceSrc,
                                        nsIPrincipal** aPrincipal,
                                        bool* aBypassCache)
{
  if (!mFontFaceSet) {
    return NS_ERROR_FAILURE;
  }
  return mFontFaceSet->CheckFontLoad(aFontFaceSrc, aPrincipal, aBypassCache);
}

 nsresult
FontFaceSet::UserFontSet::StartLoad(gfxUserFontEntry* aUserFontEntry,
                                    const gfxFontFaceSrc* aFontFaceSrc)
{
  if (!mFontFaceSet) {
    return NS_ERROR_FAILURE;
  }
  return mFontFaceSet->StartLoad(aUserFontEntry, aFontFaceSrc);
}

 nsresult
FontFaceSet::UserFontSet::LogMessage(gfxUserFontEntry* aUserFontEntry,
                                     const char* aMessage,
                                     uint32_t aFlags,
                                     nsresult aStatus)
{
  if (!mFontFaceSet) {
    return NS_ERROR_FAILURE;
  }
  return mFontFaceSet->LogMessage(aUserFontEntry, aMessage, aFlags, aStatus);
}

 nsresult
FontFaceSet::UserFontSet::SyncLoadFontData(gfxUserFontEntry* aFontToLoad,
                                           const gfxFontFaceSrc* aFontFaceSrc,
                                           uint8_t*& aBuffer,
                                           uint32_t& aBufferLength)
{
  if (!mFontFaceSet) {
    return NS_ERROR_FAILURE;
  }
  return mFontFaceSet->SyncLoadFontData(aFontToLoad, aFontFaceSrc,
                                        aBuffer, aBufferLength);
}

 bool
FontFaceSet::UserFontSet::GetPrivateBrowsing()
{
  return mFontFaceSet && mFontFaceSet->GetPrivateBrowsing();
}

 void
FontFaceSet::UserFontSet::DoRebuildUserFontSet()
{
  if (!mFontFaceSet) {
    return;
  }
  mFontFaceSet->DoRebuildUserFontSet();
}
