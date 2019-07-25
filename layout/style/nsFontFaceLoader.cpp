








































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "nsFontFaceLoader.h"

#include "nsError.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsIStreamListener.h"
#include "nsNetUtil.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsContentUtils.h"
#include "mozilla/Preferences.h"

#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"

#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIChannelPolicy.h"
#include "nsChannelPolicy.h"

#include "nsStyleSet.h"

using namespace mozilla;

#ifdef PR_LOGGING
static PRLogModuleInfo *gFontDownloaderLog = PR_NewLogModule("fontdownloader");
#endif 

#define LOG(args) PR_LOG(gFontDownloaderLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gFontDownloaderLog, PR_LOG_DEBUG)


nsFontFaceLoader::nsFontFaceLoader(gfxProxyFontEntry *aProxy, nsIURI *aFontURI,
                                   nsUserFontSet *aFontSet, nsIChannel *aChannel)
  : mFontEntry(aProxy), mFontURI(aFontURI), mFontSet(aFontSet),
    mChannel(aChannel)
{
}

nsFontFaceLoader::~nsFontFaceLoader()
{
  if (mLoadTimer) {
    mLoadTimer->Cancel();
    mLoadTimer = nsnull;
  }
  if (mFontSet) {
    mFontSet->RemoveLoader(this);
  }
}

void
nsFontFaceLoader::StartedLoading(nsIStreamLoader *aStreamLoader)
{
  PRInt32 loadTimeout =
    Preferences::GetInt("gfx.downloadable_fonts.fallback_delay", 3000);
  if (loadTimeout > 0) {
    mLoadTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mLoadTimer) {
      mLoadTimer->InitWithFuncCallback(LoadTimerCallback,
                                       static_cast<void*>(this),
                                       loadTimeout,
                                       nsITimer::TYPE_ONE_SHOT);
    }
  } else {
    mFontEntry->mLoadingState = gfxProxyFontEntry::LOADING_SLOWLY;
  }
  mStreamLoader = aStreamLoader;
}

void
nsFontFaceLoader::LoadTimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsFontFaceLoader *loader = static_cast<nsFontFaceLoader*>(aClosure);

  gfxProxyFontEntry *pe = loader->mFontEntry.get();
  bool updateUserFontSet = true;

  
  
  if (pe->mLoadingState == gfxProxyFontEntry::LOADING_STARTED) {
    PRInt32 contentLength;
    loader->mChannel->GetContentLength(&contentLength);
    PRUint32 numBytesRead;
    loader->mStreamLoader->GetNumBytesRead(&numBytesRead);

    if (contentLength > 0 &&
        numBytesRead > 3 * (PRUint32(contentLength) >> 2))
    {
      
      
      
      pe->mLoadingState = gfxProxyFontEntry::LOADING_ALMOST_DONE;
      PRUint32 delay;
      loader->mLoadTimer->GetDelay(&delay);
      loader->mLoadTimer->InitWithFuncCallback(LoadTimerCallback,
                                               static_cast<void*>(loader),
                                               delay >> 1,
                                               nsITimer::TYPE_ONE_SHOT);
      updateUserFontSet = false;
      LOG(("fontdownloader (%p) 75%% done, resetting timer\n", loader));
    }
  }

  
  
  
  if (updateUserFontSet) {
    pe->mLoadingState = gfxProxyFontEntry::LOADING_SLOWLY;
    nsPresContext *ctx = loader->mFontSet->GetPresContext();
    NS_ASSERTION(ctx, "fontSet doesn't have a presContext?");
    gfxUserFontSet *fontSet;
    if (ctx && (fontSet = ctx->GetUserFontSet()) != nsnull) {
      fontSet->IncrementGeneration();
      ctx->UserFontSetUpdated();
      LOG(("fontdownloader (%p) timeout reflow\n", loader));
    }
  }
}

NS_IMPL_ISUPPORTS1(nsFontFaceLoader, nsIStreamLoaderObserver)

NS_IMETHODIMP
nsFontFaceLoader::OnStreamComplete(nsIStreamLoader* aLoader,
                                   nsISupports* aContext,
                                   nsresult aStatus,
                                   PRUint32 aStringLen,
                                   const PRUint8* aString)
{
  if (!mFontSet) {
    
    return aStatus;
  }

  mFontSet->RemoveLoader(this);

#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsCAutoString fontURI;
    mFontURI->GetSpec(fontURI);
    if (NS_SUCCEEDED(aStatus)) {
      LOG(("fontdownloader (%p) download completed - font uri: (%s)\n", 
           this, fontURI.get()));
    } else {
      LOG(("fontdownloader (%p) download failed - font uri: (%s) error: %8.8x\n", 
           this, fontURI.get(), aStatus));
    }
  }
#endif

  nsPresContext *ctx = mFontSet->GetPresContext();
  NS_ASSERTION(ctx && !ctx->PresShell()->IsDestroying(),
               "We should have been canceled already");

  
  gfxUserFontSet *userFontSet = ctx->GetUserFontSet();
  if (!userFontSet) {
    return aStatus;
  }

  
  
  
  PRBool fontUpdate = userFontSet->OnLoadComplete(mFontEntry,
                                                  aString, aStringLen,
                                                  aStatus);

  
  if (fontUpdate) {
    
    
    ctx->UserFontSetUpdated();
    LOG(("fontdownloader (%p) reflow\n", this));
  }

  return NS_SUCCESS_ADOPTED_DATA;
}

void
nsFontFaceLoader::Cancel()
{
  mFontEntry->mLoadingState = gfxProxyFontEntry::NOT_LOADING;
  mFontSet = nsnull;
  if (mLoadTimer) {
    mLoadTimer->Cancel();
    mLoadTimer = nsnull;
  }
  mChannel->Cancel(NS_BINDING_ABORTED);
}

nsresult
nsFontFaceLoader::CheckLoadAllowed(nsIPrincipal* aSourcePrincipal,
                                   nsIURI* aTargetURI,
                                   nsISupports* aContext)
{
  nsresult rv;
  
  if (!aSourcePrincipal)
    return NS_OK;

  
  nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
  rv = secMan->CheckLoadURIWithPrincipal(aSourcePrincipal, aTargetURI,
                                        nsIScriptSecurityManager::STANDARD);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_FONT,
                                 aTargetURI,
                                 aSourcePrincipal,
                                 aContext,
                                 EmptyCString(), 
                                 nsnull,
                                 &shouldLoad,
                                 nsContentUtils::GetContentPolicy(),
                                 nsContentUtils::GetSecurityManager());

  if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
    return NS_ERROR_CONTENT_BLOCKED;
  }

  return NS_OK;
}
  
nsUserFontSet::nsUserFontSet(nsPresContext *aContext)
  : mPresContext(aContext)
{
  NS_ASSERTION(mPresContext, "null context passed to nsUserFontSet");
  mLoaders.Init();
}

nsUserFontSet::~nsUserFontSet()
{
  NS_ASSERTION(mLoaders.Count() == 0, "mLoaders should have been emptied");
}

static PLDHashOperator DestroyIterator(nsPtrHashKey<nsFontFaceLoader>* aKey,
                                       void* aUserArg)
{
  aKey->GetKey()->Cancel();
  return PL_DHASH_REMOVE;
}

void
nsUserFontSet::Destroy()
{
  mPresContext = nsnull;
  mLoaders.EnumerateEntries(DestroyIterator, nsnull);
}

void
nsUserFontSet::RemoveLoader(nsFontFaceLoader *aLoader)
{
  mLoaders.RemoveEntry(aLoader);
}

nsresult 
nsUserFontSet::StartLoad(gfxProxyFontEntry *aProxy,
                         const gfxFontFaceSrc *aFontFaceSrc)
{
  nsresult rv;
  
  
  nsIPresShell *ps = mPresContext->PresShell();
  if (!ps)
    return NS_ERROR_FAILURE;
    
  NS_ASSERTION(aFontFaceSrc && !aFontFaceSrc->mIsLocal, 
               "bad font face url passed to fontloader");
  NS_ASSERTION(aFontFaceSrc->mURI, "null font uri");
  if (!aFontFaceSrc->mURI)
    return NS_ERROR_FAILURE;

  
  
  
  nsCOMPtr<nsIPrincipal> principal = ps->GetDocument()->NodePrincipal();

  NS_ASSERTION(aFontFaceSrc->mOriginPrincipal, 
               "null origin principal in @font-face rule");
  if (aFontFaceSrc->mUseOriginPrincipal) {
    principal = do_QueryInterface(aFontFaceSrc->mOriginPrincipal);
  }
  
  rv = nsFontFaceLoader::CheckLoadAllowed(principal, aFontFaceSrc->mURI, 
                                          ps->GetDocument());
  if (NS_FAILED(rv)) {
#ifdef PR_LOGGING
    if (LOG_ENABLED()) {
      nsCAutoString fontURI, referrerURI;
      aFontFaceSrc->mURI->GetSpec(fontURI);
      if (aFontFaceSrc->mReferrer)
        aFontFaceSrc->mReferrer->GetSpec(referrerURI);
      LOG(("fontdownloader download blocked - font uri: (%s) "
           "referrer uri: (%s) err: %8.8x\n", 
          fontURI.get(), referrerURI.get(), rv));
    }
#endif    
    return rv;
  }

  nsCOMPtr<nsIStreamLoader> streamLoader;
  nsCOMPtr<nsILoadGroup> loadGroup(ps->GetDocument()->GetDocumentLoadGroup());

  nsCOMPtr<nsIChannel> channel;
  
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = principal->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);
  if (csp) {
    channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_FONT);
  }
  rv = NS_NewChannel(getter_AddRefs(channel),
                     aFontFaceSrc->mURI,
                     nsnull,
                     loadGroup,
                     nsnull,
                     nsIRequest::LOAD_NORMAL,
                     channelPolicy);

  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsFontFaceLoader> fontLoader =
    new nsFontFaceLoader(aProxy, aFontFaceSrc->mURI, this, channel);

  if (!fontLoader)
    return NS_ERROR_OUT_OF_MEMORY;

#ifdef PR_LOGGING
  if (LOG_ENABLED()) {
    nsCAutoString fontURI, referrerURI;
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
  rv = NS_NewStreamLoader(getter_AddRefs(streamLoader), fontLoader);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRBool inherits = PR_FALSE;
  rv = NS_URIChainHasFlags(aFontFaceSrc->mURI,
                           nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT,
                           &inherits);
  if (NS_SUCCEEDED(rv) && inherits) {
    
    rv = channel->AsyncOpen(streamLoader, nsnull);
  } else {
    nsCOMPtr<nsIStreamListener> listener =
      new nsCORSListenerProxy(streamLoader, principal, channel, 
                              PR_FALSE, &rv);
    if (NS_FAILED(rv)) {
      fontLoader->DropChannel();  
    }
    NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = channel->AsyncOpen(listener, nsnull);
  }

  if (NS_SUCCEEDED(rv)) {
    mLoaders.PutEntry(fontLoader);
    fontLoader->StartedLoading(streamLoader);
  }

  return rv;
}

PRBool
nsUserFontSet::UpdateRules(const nsTArray<nsFontFaceRuleContainer>& aRules)
{
  PRBool modified = PR_FALSE;

  
  
  if (mLoaders.Count() > 0) {
    modified = PR_TRUE; 
                        
  }
  mLoaders.EnumerateEntries(DestroyIterator, nsnull);

  nsTArray<FontFaceRuleRecord> oldRules;
  mRules.SwapElements(oldRules);

  
  
  
  mFontFamilies.Clear();

  for (PRUint32 i = 0, i_end = aRules.Length(); i < i_end; ++i) {
    
    
    
    InsertRule(aRules[i].mRule, aRules[i].mSheetType, oldRules, modified);
  }

  
  if (oldRules.Length() > 0) {
    modified = PR_TRUE;
  }

  if (modified) {
    IncrementGeneration();
  }

  return modified;
}

void
nsUserFontSet::InsertRule(nsCSSFontFaceRule *aRule, PRUint8 aSheetType,
                          nsTArray<FontFaceRuleRecord>& aOldRules,
                          PRBool& aFontSetModified)
{
  NS_ABORT_IF_FALSE(aRule->GetType() == mozilla::css::Rule::FONT_FACE_RULE,
                    "InsertRule passed a non-fontface CSS rule");

  
  nsAutoString fontfamily;
  nsCSSValue val;
  PRUint32 unit;

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

  
  
  for (PRUint32 i = 0; i < aOldRules.Length(); ++i) {
    const FontFaceRuleRecord& ruleRec = aOldRules[i];
    if (ruleRec.mContainer.mRule == aRule &&
        ruleRec.mContainer.mSheetType == aSheetType) {
      AddFontFace(fontfamily, ruleRec.mFontEntry);
      mRules.AppendElement(ruleRec);
      aOldRules.RemoveElementAt(i);
      
      
      if (i > 0) {
        aFontSetModified = PR_TRUE;
      }
      return;
    }
  }

  

  PRUint32 weight = NS_STYLE_FONT_WEIGHT_NORMAL;
  PRUint32 stretch = NS_STYLE_FONT_STRETCH_NORMAL;
  PRUint32 italicStyle = FONT_STYLE_NORMAL;
  nsString featureSettings, languageOverride;

  
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

  
  aRule->GetDesc(eCSSFontDesc_FontFeatureSettings, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Normal) {
    
  } else if (unit == eCSSUnit_String) {
    val.GetStringValue(featureSettings);
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null,
                 "@font-face font-feature-settings has unexpected unit");
  }

  
  aRule->GetDesc(eCSSFontDesc_FontLanguageOverride, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Normal) {
    
  } else if (unit == eCSSUnit_String) {
    val.GetStringValue(languageOverride);
  } else {
    NS_ASSERTION(unit == eCSSUnit_Null,
                 "@font-face font-language-override has unexpected unit");
  }

  
  nsTArray<gfxFontFaceSrc> srcArray;

  aRule->GetDesc(eCSSFontDesc_Src, val);
  unit = val.GetUnit();
  if (unit == eCSSUnit_Array) {
    nsCSSValue::Array *srcArr = val.GetArrayValue();
    size_t numSrc = srcArr->Count();
    
    for (size_t i = 0; i < numSrc; i++) {
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
          if (valueString.LowerCaseEqualsASCII("woff")) {
            face->mFormatFlags |= FLAG_FORMAT_WOFF;
          } else if (valueString.LowerCaseEqualsASCII("opentype")) {
            face->mFormatFlags |= FLAG_FORMAT_OPENTYPE;
          } else if (valueString.LowerCaseEqualsASCII("truetype")) {
            face->mFormatFlags |= FLAG_FORMAT_TRUETYPE;
          } else if (valueString.LowerCaseEqualsASCII("truetype-aat")) {
            face->mFormatFlags |= FLAG_FORMAT_TRUETYPE_AAT;
          } else if (valueString.LowerCaseEqualsASCII("embedded-opentype")) {
            face->mFormatFlags |= FLAG_FORMAT_EOT;
          } else if (valueString.LowerCaseEqualsASCII("svg")) {
            face->mFormatFlags |= FLAG_FORMAT_SVG;
          } else {
            
            
            face->mFormatFlags |= FLAG_FORMAT_UNKNOWN;
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

  if (srcArray.Length() > 0) {
    FontFaceRuleRecord ruleRec;
    ruleRec.mContainer.mRule = aRule;
    ruleRec.mContainer.mSheetType = aSheetType;
    ruleRec.mFontEntry = AddFontFace(fontfamily, srcArray,
                                     weight, stretch, italicStyle,
                                     featureSettings, languageOverride);
    if (ruleRec.mFontEntry) {
      mRules.AppendElement(ruleRec);
    }
    
    aFontSetModified = PR_TRUE;
  }
}

void
nsUserFontSet::ReplaceFontEntry(gfxProxyFontEntry *aProxy,
                                gfxFontEntry *aFontEntry)
{
  for (PRUint32 i = 0; i < mRules.Length(); ++i) {
    if (mRules[i].mFontEntry == aProxy) {
      mRules[i].mFontEntry = aFontEntry;
      break;
    }
  }
  static_cast<gfxMixedFontFamily*>(aProxy->Family())->
    ReplaceFontEntry(aProxy, aFontEntry);
}
