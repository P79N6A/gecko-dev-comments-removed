




































#include "nsLayoutStatics.h"
#include "nscore.h"

#include "nsAttrValue.h"
#include "nsAutoCopyListener.h"
#include "nsColorNames.h"
#include "nsComputedDOMStyle.h"
#include "nsContentDLF.h"
#include "nsContentUtils.h"
#include "nsCSSAnonBoxes.h"
#include "nsCSSFrameConstructor.h"
#include "nsCSSKeywords.h"
#include "nsCSSLoader.h"
#include "nsCSSProps.h"
#include "nsCSSPseudoClasses.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSRendering.h"
#include "nsCSSScanner.h"
#include "nsICSSStyleSheet.h"
#include "nsDOMAttribute.h"
#include "nsDOMClassInfo.h"
#include "nsEventListenerManager.h"
#include "nsFrame.h"
#include "nsGenericElement.h"  
#include "nsGlobalWindow.h"
#include "nsGkAtoms.h"
#include "nsImageFrame.h"
#include "nsLayoutStylesheetCache.h"
#include "nsNodeInfo.h"
#include "nsRange.h"
#include "nsRepeatService.h"
#include "nsFloatManager.h"
#include "nsSprocketLayout.h"
#include "nsStackLayout.h"
#include "nsStyleSet.h"
#include "nsTextControlFrame.h"
#include "nsXBLWindowKeyHandler.h"
#include "txMozillaXSLTProcessor.h"
#include "nsDOMStorage.h"
#include "nsCellMap.h"
#include "nsTextFrameTextRunCache.h"
#include "nsCCUncollectableMarker.h"
#include "nsTextFragment.h"
#include "nsCSSRuleProcessor.h"
#include "nsXMLHttpRequest.h"
#include "nsDOMThreadService.h"
#include "nsHTMLDNSPrefetch.h"
#include "nsHtml5Module.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsFocusManager.h"
#include "nsFrameList.h"
#include "nsListControlFrame.h"

#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#include "nsXULContentUtils.h"
#include "nsXULElement.h"
#include "nsXULPrototypeCache.h"
#include "nsXULTooltipListener.h"

#ifndef MOZ_NO_INSPECTOR_APIS
#include "inDOMView.h"
#endif
#endif

#ifdef MOZ_MATHML
#include "nsMathMLAtoms.h"
#include "nsMathMLOperators.h"
#endif

#ifdef MOZ_SVG
PRBool NS_SVGEnabled();
#endif

#ifndef MOZILLA_PLAINTEXT_EDITOR_ONLY
#include "nsHTMLEditor.h"
#include "nsTextServicesDocument.h"
#endif

#ifdef MOZ_MEDIA
#include "nsMediaDecoder.h"
#include "nsHTMLMediaElement.h"
#endif

#ifdef MOZ_SYDNEYAUDIO
#include "nsAudioStream.h"
#endif

#include "nsError.h"
#include "nsTraceRefcnt.h"

#include "nsCycleCollector.h"
#include "nsJSEnvironment.h"

extern void NS_ShutdownChainItemPool();

static nsrefcnt sLayoutStaticRefcnt;

nsresult
nsLayoutStatics::Initialize()
{
  NS_ASSERTION(sLayoutStaticRefcnt == 0,
               "nsLayoutStatics isn't zero!");

  sLayoutStaticRefcnt = 1;
  NS_LOG_ADDREF(&sLayoutStaticRefcnt, sLayoutStaticRefcnt,
                "nsLayoutStatics", 1);

  nsresult rv;

  
  nsCSSAnonBoxes::AddRefAtoms();
  nsCSSPseudoClasses::AddRefAtoms();
  nsCSSPseudoElements::AddRefAtoms();
  nsCSSKeywords::AddRefTable();
  nsCSSProps::AddRefTable();
  nsColorNames::AddRefTable();
  nsGkAtoms::AddRefAtoms();

  nsJSRuntime::Startup();
  rv = nsContentUtils::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsContentUtils");
    return rv;
  }

  rv = nsAttrValue::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsAttrValue");
    return rv;
  }

  rv = nsTextFragment::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsTextFragment");
    return rv;
  }

  rv = nsCellMap::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsCellMap");
    return rv;
  }

  rv = nsCSSRendering::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsCSSRendering");
    return rv;
  }

  rv = nsTextFrameTextRunCache::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize textframe textrun cache");
    return rv;
  }

  rv = nsHTMLDNSPrefetch::Initialize();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize HTML DNS prefetch");
    return rv;
  }

#ifdef MOZ_XUL
  rv = nsXULContentUtils::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsXULContentUtils");
    return rv;
  }

#ifndef MOZ_NO_INSPECTOR_APIS
  inDOMView::InitAtoms();
#endif

#endif

#ifdef MOZ_MATHML
  nsMathMLOperators::AddRefTable();
#endif

#ifdef MOZ_SVG
  if (NS_SVGEnabled())
    nsContentDLF::RegisterSVG();
#endif

#ifndef MOZILLA_PLAINTEXT_EDITOR_ONLY
  nsEditProperty::RegisterAtoms();
  nsTextServicesDocument::RegisterAtoms();
#endif

#ifdef DEBUG
  nsFrame::DisplayReflowStartup();
#endif
  nsDOMAttribute::Initialize();

  rv = txMozillaXSLTProcessor::Startup();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize txMozillaXSLTProcessor");
    return rv;
  }

  rv = nsDOMStorageManager::Initialize();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsDOMStorageManager");
    return rv;
  }

  rv = nsCCUncollectableMarker::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsCCUncollectableMarker");
    return rv;
  }

  nsCSSRuleProcessor::Startup();

#ifdef MOZ_XUL
  rv = nsXULPopupManager::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsXULPopupManager");
    return rv;
  }
#endif

  rv = nsFocusManager::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsFocusManager");
    return rv;
  }

#ifdef MOZ_MEDIA
  rv = nsMediaDecoder::InitLogger();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsMediaDecoder");
    return rv;
  }
  
  nsHTMLMediaElement::InitMediaTypes();
#endif

#ifdef MOZ_SYDNEYAUDIO
  nsAudioStream::InitLibrary();
#endif

  nsHtml5Module::InitializeStatics();
  
  nsCrossSiteListenerProxy::Startup();

  rv = nsFrameList::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsFrameList");
    return rv;
  }

  return NS_OK;
}

void
nsLayoutStatics::Shutdown()
{
  nsFocusManager::Shutdown();
#ifdef MOZ_XUL
  nsXULPopupManager::Shutdown();
#endif
  nsDOMStorageManager::Shutdown();
  txMozillaXSLTProcessor::Shutdown();
  nsDOMAttribute::Shutdown();
  nsDOMEventRTTearoff::Shutdown();
  nsEventListenerManager::Shutdown();
  nsContentList::Shutdown();
  nsComputedDOMStyle::Shutdown();
  CSSLoaderImpl::Shutdown();
  nsCSSRuleProcessor::FreeSystemMetrics();
  nsTextFrameTextRunCache::Shutdown();
  nsHTMLDNSPrefetch::Shutdown();
  nsCSSRendering::Shutdown();
#ifdef DEBUG
  nsFrame::DisplayReflowShutdown();
#endif
  nsCellMap::Shutdown();

  
  nsColorNames::ReleaseTable();
  nsCSSProps::ReleaseTable();
  nsCSSKeywords::ReleaseTable();
  nsRepeatService::Shutdown();
  nsStackLayout::Shutdown();
  nsBox::Shutdown();

#ifdef MOZ_XUL
  nsXULContentUtils::Finish();
  nsXULElement::ReleaseGlobals();
  nsXULPrototypeCache::ReleaseGlobals();
  nsXULPrototypeElement::ReleaseGlobals();
  nsSprocketLayout::Shutdown();
#endif

#ifdef MOZ_MATHML
  nsMathMLOperators::ReleaseTable();
#endif

  nsCSSFrameConstructor::ReleaseGlobals();
  nsFloatManager::Shutdown();
  nsImageFrame::ReleaseGlobals();

  nsCSSScanner::ReleaseGlobals();

  NS_IF_RELEASE(nsRuleNode::gLangService);

  nsTextFragment::Shutdown();

  nsAttrValue::Shutdown();
  nsContentUtils::Shutdown();
  nsNodeInfo::ClearCache();
  nsLayoutStylesheetCache::Shutdown();
  NS_NameSpaceManagerShutdown();

  nsJSRuntime::Shutdown();
  nsGlobalWindow::ShutDown();
  nsDOMClassInfo::ShutDown();
  nsTextControlFrame::ShutDown();
  nsListControlFrame::Shutdown();
  nsXBLWindowKeyHandler::ShutDown();
  nsAutoCopyListener::Shutdown();

#ifndef MOZILLA_PLAINTEXT_EDITOR_ONLY
  nsHTMLEditor::Shutdown();
  nsTextServicesDocument::Shutdown();
#endif

  nsDOMThreadService::Shutdown();

#ifdef MOZ_MEDIA
  nsHTMLMediaElement::ShutdownMediaTypes();
#endif
#ifdef MOZ_SYDNEYAUDIO
  nsAudioStream::ShutdownLibrary();
#endif

  nsXMLHttpRequest::ShutdownACCache();
  
  nsHtml5Module::ReleaseStatics();

  NS_ShutdownChainItemPool();

  nsFrameList::Shutdown();
}

void
nsLayoutStatics::AddRef()
{
  NS_ASSERTION(sLayoutStaticRefcnt,
               "nsLayoutStatics already dropped to zero!");

  ++sLayoutStaticRefcnt;
  NS_LOG_ADDREF(&sLayoutStaticRefcnt, sLayoutStaticRefcnt,
                "nsLayoutStatics", 1);
}

void
nsLayoutStatics::Release()
{
  --sLayoutStaticRefcnt;
  NS_LOG_RELEASE(&sLayoutStaticRefcnt, sLayoutStaticRefcnt,
                 "nsLayoutStatics");

  if (!sLayoutStaticRefcnt)
    Shutdown();
}
