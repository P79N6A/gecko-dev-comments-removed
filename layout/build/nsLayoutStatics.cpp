




































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
#include "nsDOMScriptObjectFactory.h"
#include "nsEventListenerManager.h"
#include "nsFrame.h"
#include "nsGenericElement.h"  
#include "nsGenericHTMLElement.h"
#include "nsGlobalWindow.h"
#include "nsGkAtoms.h"
#include "nsImageFrame.h"
#include "nsLayoutStylesheetCache.h"
#include "nsNodeInfo.h"
#include "nsRange.h"
#include "nsRepeatService.h"
#include "nsSpaceManager.h"
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
#include "nsXULPopupManager.h"
#include "nsTextFragment.h"

#ifdef MOZ_XUL
#include "nsXULContentUtils.h"
#include "nsXULElement.h"
#include "nsXULPrototypeCache.h"
#include "nsXULTooltipListener.h"
#endif

#ifdef MOZ_MATHML
#include "nsMathMLAtoms.h"
#include "nsMathMLOperators.h"
#endif

#ifdef MOZ_SVG
PRBool NS_SVGEnabled();
#endif

#ifndef MOZ_NO_INSPECTOR_APIS
#include "inDOMView.h"
#endif

#ifndef MOZILLA_PLAINTEXT_EDITOR_ONLY
#include "nsHTMLEditor.h"
#include "nsTextServicesDocument.h"
#endif

#include "nsError.h"
#include "nsTraceRefcnt.h"

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

  nsDOMScriptObjectFactory::Startup();
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

#ifndef MOZ_NO_INSPECTOR_APIS
  inDOMView::InitAtoms();
#endif

#ifdef MOZ_XUL
  rv = nsXULContentUtils::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsXULContentUtils");
    return rv;
  }
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

  rv = txMozillaXSLTProcessor::Init();
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

  rv = nsXULPopupManager::Init();
  if (NS_FAILED(rv)) {
    NS_ERROR("Could not initialize nsXULPopupManager");
    return rv;
  }

  return NS_OK;
}

void
nsLayoutStatics::Shutdown()
{
  nsXULPopupManager::Shutdown();
  nsDOMStorageManager::Shutdown();
  txMozillaXSLTProcessor::Shutdown();
  nsDOMAttribute::Shutdown();
  nsDOMEventRTTearoff::Shutdown();
  nsEventListenerManager::Shutdown();
  nsContentList::Shutdown();
  nsComputedDOMStyle::Shutdown();
  CSSLoaderImpl::Shutdown();
  nsTextFrameTextRunCache::Shutdown();
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
  nsSpaceManager::Shutdown();
  nsImageFrame::ReleaseGlobals();

  nsCSSScanner::ReleaseGlobals();

  NS_IF_RELEASE(nsContentDLF::gUAStyleSheet);
  NS_IF_RELEASE(nsRuleNode::gLangService);
  nsGenericHTMLElement::Shutdown();

  nsTextFragment::Shutdown();

  nsAttrValue::Shutdown();
  nsContentUtils::Shutdown();
  nsLayoutStylesheetCache::Shutdown();
  NS_NameSpaceManagerShutdown();
  nsStyleSet::FreeGlobals();

  nsGlobalWindow::ShutDown();
  nsDOMClassInfo::ShutDown();
  nsTextControlFrame::ShutDown();
  nsXBLWindowKeyHandler::ShutDown();
  nsAutoCopyListener::Shutdown();

#ifndef MOZILLA_PLAINTEXT_EDITOR_ONLY
  nsHTMLEditor::Shutdown();
  nsTextServicesDocument::Shutdown();
#endif
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
