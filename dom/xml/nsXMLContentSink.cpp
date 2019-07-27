





#include "nsCOMPtr.h"
#include "nsXMLContentSink.h"
#include "nsIParser.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIContent.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIDocShell.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsIDOMComment.h"
#include "nsIDOMCDATASection.h"
#include "DocumentType.h"
#include "nsHTMLParts.h"
#include "nsCRT.h"
#include "mozilla/CSSStyleSheet.h"
#include "mozilla/css/Loader.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsIScriptContext.h"
#include "nsNameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIContentViewer.h"
#include "prtime.h"
#include "prlog.h"
#include "prmem.h"
#include "nsRect.h"
#include "nsIWebNavigation.h"
#include "nsIScriptElement.h"
#include "nsScriptLoader.h"
#include "nsStyleLinkElement.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsICookieService.h"
#include "nsIPrompt.h"
#include "nsIChannel.h"
#include "nsIPrincipal.h"
#include "nsXMLPrettyPrinter.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsError.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsNodeUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsIHTMLDocument.h"
#include "mozAutoDocUpdate.h"
#include "nsMimeTypes.h"
#include "nsHtml5SVGLoadDispatcher.h"
#include "nsTextNode.h"
#include "mozilla/dom/CDATASection.h"
#include "mozilla/dom/Comment.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLTemplateElement.h"
#include "mozilla/dom/ProcessingInstruction.h"

using namespace mozilla;
using namespace mozilla::dom;










nsresult
NS_NewXMLContentSink(nsIXMLContentSink** aResult,
                     nsIDocument* aDoc,
                     nsIURI* aURI,
                     nsISupports* aContainer,
                     nsIChannel* aChannel)
{
  NS_PRECONDITION(nullptr != aResult, "null ptr");
  if (nullptr == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsRefPtr<nsXMLContentSink> it = new nsXMLContentSink();
  
  nsresult rv = it->Init(aDoc, aURI, aContainer, aChannel);
  NS_ENSURE_SUCCESS(rv, rv);

  it.forget(aResult);
  return NS_OK;
}

nsXMLContentSink::nsXMLContentSink()
  : mConstrainSize(true),
    mPrettyPrintXML(true)
{
}

nsXMLContentSink::~nsXMLContentSink()
{
  if (mText) {
    PR_Free(mText);  
  }
}

nsresult
nsXMLContentSink::Init(nsIDocument* aDoc,
                       nsIURI* aURI,
                       nsISupports* aContainer,
                       nsIChannel* aChannel)
{
  nsresult rv = nsContentSink::Init(aDoc, aURI, aContainer, aChannel);
  NS_ENSURE_SUCCESS(rv, rv);

  aDoc->AddObserver(this);
  mIsDocumentObserver = true;

  if (!mDocShell) {
    mPrettyPrintXML = false;
  }
  
  mState = eXMLContentSinkState_InProlog;
  mDocElement = nullptr;

  return NS_OK;
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsXMLContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIXMLContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIExpatSink)
  NS_INTERFACE_MAP_ENTRY(nsITransformObserver)
NS_INTERFACE_MAP_END_INHERITING(nsContentSink)

NS_IMPL_ADDREF_INHERITED(nsXMLContentSink, nsContentSink)
NS_IMPL_RELEASE_INHERITED(nsXMLContentSink, nsContentSink)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsXMLContentSink)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsXMLContentSink,
                                                  nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mCurrentHead)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocElement)
  for (uint32_t i = 0, count = tmp->mContentStack.Length(); i < count; i++) {
    const StackNode& node = tmp->mContentStack.ElementAt(i);
    cb.NoteXPCOMChild(node.mContent);
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END


NS_IMETHODIMP
nsXMLContentSink::WillParse(void)
{
  return WillParseImpl();
}

NS_IMETHODIMP
nsXMLContentSink::WillBuildModel(nsDTDMode aDTDMode)
{
  WillBuildModelImpl();

  
  mDocument->BeginLoad();

  
  if (mPrettyPrintXML) {
    nsAutoCString command;
    GetParser()->GetCommand(command);
    if (!command.EqualsLiteral("view")) {
      mPrettyPrintXML = false;
    }
  }
  
  return NS_OK;
}

bool
nsXMLContentSink::CanStillPrettyPrint()
{
  return mPrettyPrintXML &&
         (!mPrettyPrintHasFactoredElements || mPrettyPrintHasSpecialRoot);
}

nsresult
nsXMLContentSink::MaybePrettyPrint()
{
  if (!CanStillPrettyPrint()) {
    mPrettyPrintXML = false;

    return NS_OK;
  }

  
  mDocument->RemoveObserver(this);
  mIsDocumentObserver = false;

  
  if (mCSSLoader) {
    mCSSLoader->SetEnabled(true);
  }
  
  nsRefPtr<nsXMLPrettyPrinter> printer;
  nsresult rv = NS_NewXMLPrettyPrinter(getter_AddRefs(printer));
  NS_ENSURE_SUCCESS(rv, rv);

  bool isPrettyPrinting;
  rv = printer->PrettyPrint(mDocument, &isPrettyPrinting);
  NS_ENSURE_SUCCESS(rv, rv);

  mPrettyPrinting = isPrettyPrinting;
  return NS_OK;
}

static void
CheckXSLTParamPI(nsIDOMProcessingInstruction* aPi,
                 nsIDocumentTransformer* aProcessor,
                 nsIDocument* aDocument)
{
  nsAutoString target, data;
  aPi->GetTarget(target);

  
  if (target.EqualsLiteral("xslt-param-namespace")) {
    aPi->GetData(data);
    nsAutoString prefix, namespaceAttr;
    nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::prefix,
                                            prefix);
    if (!prefix.IsEmpty() &&
        nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::_namespace,
                                                namespaceAttr)) {
      aProcessor->AddXSLTParamNamespace(prefix, namespaceAttr);
    }
  }

  
  else if (target.EqualsLiteral("xslt-param")) {
    aPi->GetData(data);
    nsAutoString name, namespaceAttr, select, value;
    nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::name,
                                            name);
    nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::_namespace,
                                            namespaceAttr);
    if (!nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::select, select)) {
      select.SetIsVoid(true);
    }
    if (!nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::value, value)) {
      value.SetIsVoid(true);
    }
    if (!name.IsEmpty()) {
      nsCOMPtr<nsIDOMNode> doc = do_QueryInterface(aDocument);
      aProcessor->AddXSLTParam(name, namespaceAttr, select, value, doc);
    }
  }
}

NS_IMETHODIMP
nsXMLContentSink::DidBuildModel(bool aTerminated)
{
  if (!mParser) {
    
    
    
    
    
    return NS_OK;
  }

  DidBuildModelImpl(aTerminated);

  if (mXSLTProcessor) {
    
    mDocument->RemoveObserver(this);
    mIsDocumentObserver = false;

    
    for (nsIContent* child = mDocument->GetFirstChild();
         child;
         child = child->GetNextSibling()) {
      if (child->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION)) {
        nsCOMPtr<nsIDOMProcessingInstruction> pi = do_QueryInterface(child);
        CheckXSLTParamPI(pi, mXSLTProcessor, mDocument);
      }
      else if (child->IsElement()) {
        
        break;
      }
    }

    nsCOMPtr<nsIDOMDocument> currentDOMDoc(do_QueryInterface(mDocument));
    mXSLTProcessor->SetSourceContentModel(currentDOMDoc);
    
    
    mXSLTProcessor = nullptr;
  }
  else {
    

    
    MaybePrettyPrint();

    bool startLayout = true;
    
    if (mPrettyPrinting) {
      NS_ASSERTION(!mPendingSheetCount, "Shouldn't have pending sheets here!");
      
      
      
      if (mDocument->CSSLoader()->HasPendingLoads() &&
          NS_SUCCEEDED(mDocument->CSSLoader()->AddObserver(this))) {
        
        startLayout = false;
      }
    }
    
    if (startLayout) {
      StartLayout(false);

      ScrollToRef();
    }

    mDocument->RemoveObserver(this);
    mIsDocumentObserver = false;

    mDocument->EndLoad();
  }

  DropParserAndPerfHint();

  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::OnDocumentCreated(nsIDocument* aResultDocument)
{
  NS_ENSURE_ARG(aResultDocument);

  nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(aResultDocument);
  if (htmlDoc) {
    htmlDoc->SetDocWriteDisabled(true);
  }

  nsCOMPtr<nsIContentViewer> contentViewer;
  mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (contentViewer) {
    return contentViewer->SetDocumentInternal(aResultDocument, true);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::OnTransformDone(nsresult aResult,
                                  nsIDocument* aResultDocument)
{
  NS_ASSERTION(NS_FAILED(aResult) || aResultDocument,
               "Don't notify about transform success without a document.");

  nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(aResultDocument);

  nsCOMPtr<nsIContentViewer> contentViewer;
  mDocShell->GetContentViewer(getter_AddRefs(contentViewer));

  if (NS_FAILED(aResult) && contentViewer) {
    
    if (domDoc) {
      aResultDocument->SetMayStartLayout(false);
      
      contentViewer->SetDOMDocument(domDoc);
    }
    else {
      
      
      nsCOMPtr<nsIDOMDocument> document = do_QueryInterface(mDocument);
      contentViewer->SetDOMDocument(document);
    }
  }

  nsCOMPtr<nsIDocument> originalDocument = mDocument;
  if (NS_SUCCEEDED(aResult) || aResultDocument) {
    
    
    mDocument = aResultDocument;
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(mDocument);
    if (htmlDoc) {
      htmlDoc->SetDocWriteDisabled(false);
    }
  }

  
  
  
  
  nsIContent *rootElement = mDocument->GetRootElement();
  if (rootElement) {
    NS_ASSERTION(mDocument->IndexOf(rootElement) != -1,
                 "rootElement not in doc?");
    mDocument->BeginUpdate(UPDATE_CONTENT_MODEL);
    nsNodeUtils::ContentInserted(mDocument, rootElement,
                                 mDocument->IndexOf(rootElement));
    mDocument->EndUpdate(UPDATE_CONTENT_MODEL);
  }

  
  StartLayout(false);

  ScrollToRef();

  originalDocument->EndLoad();

  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::StyleSheetLoaded(CSSStyleSheet* aSheet,
                                   bool aWasAlternate,
                                   nsresult aStatus)
{
  if (!mPrettyPrinting) {
    return nsContentSink::StyleSheetLoaded(aSheet, aWasAlternate, aStatus);
  }

  if (!mDocument->CSSLoader()->HasPendingLoads()) {
    mDocument->CSSLoader()->RemoveObserver(this);
    StartLayout(false);
    ScrollToRef();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::WillInterrupt(void)
{
  return WillInterruptImpl();
}

NS_IMETHODIMP
nsXMLContentSink::WillResume(void)
{
  return WillResumeImpl();
}

NS_IMETHODIMP
nsXMLContentSink::SetParser(nsParserBase* aParser)
{
  NS_PRECONDITION(aParser, "Should have a parser here!");
  mParser = aParser;
  return NS_OK;
}

nsresult
nsXMLContentSink::CreateElement(const char16_t** aAtts, uint32_t aAttsCount,
                                mozilla::dom::NodeInfo* aNodeInfo, uint32_t aLineNumber,
                                nsIContent** aResult, bool* aAppendContent,
                                FromParser aFromParser)
{
  NS_ASSERTION(aNodeInfo, "can't create element without nodeinfo");

  *aResult = nullptr;
  *aAppendContent = true;
  nsresult rv = NS_OK;

  nsRefPtr<mozilla::dom::NodeInfo> ni = aNodeInfo;
  nsRefPtr<Element> content;
  rv = NS_NewElement(getter_AddRefs(content), ni.forget(), aFromParser);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_XHTML)
      || aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_SVG)
    ) {
    nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(content);
    sele->SetScriptLineNumber(aLineNumber);
    sele->SetCreatorParser(GetParser());
    mConstrainSize = false;
  }

  
  if (aNodeInfo->NamespaceEquals(kNameSpaceID_XHTML)) {
    mPrettyPrintHasFactoredElements = true;
  }
  else {
    
    if (!mPrettyPrintHasFactoredElements && !mPrettyPrintHasSpecialRoot &&
        mPrettyPrintXML) {
      mPrettyPrintHasFactoredElements =
        nsContentUtils::NameSpaceManager()->
          HasElementCreator(aNodeInfo->NamespaceID());
    }

    if (!aNodeInfo->NamespaceEquals(kNameSpaceID_SVG)) {
      content.forget(aResult);

      return NS_OK;
    }
  }

  if (aNodeInfo->Equals(nsGkAtoms::link, kNameSpaceID_XHTML) ||
      aNodeInfo->Equals(nsGkAtoms::style, kNameSpaceID_XHTML) ||
      aNodeInfo->Equals(nsGkAtoms::style, kNameSpaceID_SVG)) {
    nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(content));
    if (ssle) {
      ssle->InitStyleLinkElement(false);
      if (aFromParser) {
        ssle->SetEnableUpdates(false);
      }
      if (!aNodeInfo->Equals(nsGkAtoms::link, kNameSpaceID_XHTML)) {
        ssle->SetLineNumber(aFromParser ? aLineNumber : 0);
      }
    }
  } 

  content.forget(aResult);

  return NS_OK;
}


nsresult
nsXMLContentSink::CloseElement(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "missing element to close");

  mozilla::dom::NodeInfo *nodeInfo = aContent->NodeInfo();

  
  
  if ((nodeInfo->NamespaceID() == kNameSpaceID_XHTML &&
       (nodeInfo->NameAtom() == nsGkAtoms::select ||
        nodeInfo->NameAtom() == nsGkAtoms::textarea ||
        nodeInfo->NameAtom() == nsGkAtoms::video ||
        nodeInfo->NameAtom() == nsGkAtoms::audio ||
        nodeInfo->NameAtom() == nsGkAtoms::object ||
        nodeInfo->NameAtom() == nsGkAtoms::applet))
      || nodeInfo->NameAtom() == nsGkAtoms::title
      ) {
    aContent->DoneAddingChildren(HaveNotifiedForCurrentContent());
  }
  
  if (IsMonolithicContainer(nodeInfo)) {
    mInMonolithicContainer--;
  }

  if (!nodeInfo->NamespaceEquals(kNameSpaceID_XHTML) &&
      !nodeInfo->NamespaceEquals(kNameSpaceID_SVG)) {
    return NS_OK;
  }

  if (nodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_XHTML)
      || nodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_SVG)
    ) {
    mConstrainSize = true; 
    nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(aContent);

    if (mPreventScriptExecution) {
      sele->PreventExecution();
      return NS_OK;
    }

    
    StopDeflecting();

    
    
    bool block = sele->AttemptToExecute();

    
    
    if (mParser && !mParser->IsParserEnabled()) {
      
      GetParser()->BlockParser();
      block = true;
    }

    return block ? NS_ERROR_HTMLPARSER_BLOCK : NS_OK;
  }
  
  nsresult rv = NS_OK;
  if (nodeInfo->Equals(nsGkAtoms::meta, kNameSpaceID_XHTML) &&
           
           
           (!mPrettyPrintXML || !mPrettyPrintHasSpecialRoot)) {
    rv = ProcessMETATag(aContent);
  }
  else if (nodeInfo->Equals(nsGkAtoms::link, kNameSpaceID_XHTML) ||
           nodeInfo->Equals(nsGkAtoms::style, kNameSpaceID_XHTML) ||
           nodeInfo->Equals(nsGkAtoms::style, kNameSpaceID_SVG)) {
    nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(aContent));
    if (ssle) {
      ssle->SetEnableUpdates(true);
      bool willNotify;
      bool isAlternate;
      rv = ssle->UpdateStyleSheet(mRunsToCompletion ? nullptr : this,
                                  &willNotify,
                                  &isAlternate);
      if (NS_SUCCEEDED(rv) && willNotify && !isAlternate && !mRunsToCompletion) {
        ++mPendingSheetCount;
        mScriptLoader->AddExecuteBlocker();
      }
    }
    
    
    if (nodeInfo->Equals(nsGkAtoms::link, kNameSpaceID_XHTML)) {
      nsAutoString relVal;
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::rel, relVal);
      if (!relVal.IsEmpty()) {
        uint32_t linkTypes =
          nsStyleLinkElement::ParseLinkTypes(relVal, aContent->NodePrincipal());
        bool hasPrefetch = linkTypes & nsStyleLinkElement::ePREFETCH;
        if (hasPrefetch || (linkTypes & nsStyleLinkElement::eNEXT)) {
          nsAutoString hrefVal;
          aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::href, hrefVal);
          if (!hrefVal.IsEmpty()) {
            PrefetchHref(hrefVal, aContent, hasPrefetch);
          }
        }
        if (linkTypes & nsStyleLinkElement::eDNS_PREFETCH) {
          nsAutoString hrefVal;
          aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::href, hrefVal);
          if (!hrefVal.IsEmpty()) {
            PrefetchDNS(hrefVal);
          }
        }
      }
    }
  }

  return rv;
}  

nsresult
nsXMLContentSink::AddContentAsLeaf(nsIContent *aContent)
{
  nsresult result = NS_OK;

  if ((eXMLContentSinkState_InProlog == mState) ||
      (eXMLContentSinkState_InEpilog == mState)) {
    NS_ASSERTION(mDocument, "Fragments have no prolog or epilog");
    mDocument->AppendChildTo(aContent, false);
  }
  else {
    nsCOMPtr<nsIContent> parent = GetCurrentContent();

    if (parent) {
      result = parent->AppendChildTo(aContent, false);
    }
  }
  return result;
}



nsresult
nsXMLContentSink::LoadXSLStyleSheet(nsIURI* aUrl)
{
  nsCOMPtr<nsIDocumentTransformer> processor =
    do_CreateInstance("@mozilla.org/document-transformer;1?type=xslt");
  if (!processor) {
    
    return NS_OK;
  }

  processor->SetTransformObserver(this);

  if (NS_SUCCEEDED(processor->LoadStyleSheet(aUrl, mDocument))) {
    mXSLTProcessor.swap(processor);
  }

  
  
  

  return NS_OK;
}

nsresult
nsXMLContentSink::ProcessStyleLink(nsIContent* aElement,
                                   const nsSubstring& aHref,
                                   bool aAlternate,
                                   const nsSubstring& aTitle,
                                   const nsSubstring& aType,
                                   const nsSubstring& aMedia)
{
  nsresult rv = NS_OK;
  mPrettyPrintXML = false;

  nsAutoCString cmd;
  if (mParser)
    GetParser()->GetCommand(cmd);
  if (cmd.EqualsASCII(kLoadAsData))
    return NS_OK; 

  NS_ConvertUTF16toUTF8 type(aType);
  if (type.EqualsIgnoreCase(TEXT_XSL) ||
      type.EqualsIgnoreCase(APPLICATION_XSLT_XML) ||
      type.EqualsIgnoreCase(TEXT_XML) ||
      type.EqualsIgnoreCase(APPLICATION_XML)) {
    if (aAlternate) {
      
      return NS_OK;
    }
    
    if (!mDocShell)
      return NS_OK;

    nsCOMPtr<nsIURI> url;
    rv = NS_NewURI(getter_AddRefs(url), aHref, nullptr,
                   mDocument->GetDocBaseURI());
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
    rv = secMan->
      CheckLoadURIWithPrincipal(mDocument->NodePrincipal(), url,
                                nsIScriptSecurityManager::ALLOW_CHROME);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    
    int16_t decision = nsIContentPolicy::ACCEPT;
    rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_XSLT,
                                   url,
                                   mDocument->NodePrincipal(),
                                   aElement,
                                   type,
                                   nullptr,
                                   &decision,
                                   nsContentUtils::GetContentPolicy(),
                                   nsContentUtils::GetSecurityManager());

    NS_ENSURE_SUCCESS(rv, rv);

    if (NS_CP_REJECTED(decision)) {
      return NS_OK;
    }

    return LoadXSLStyleSheet(url);
  }

  
  rv = nsContentSink::ProcessStyleLink(aElement, aHref, aAlternate,
                                       aTitle, aType, aMedia);

  
  
  
  return rv;
}

NS_IMETHODIMP 
nsXMLContentSink::SetDocumentCharset(nsACString& aCharset)
{
  if (mDocument) {
    mDocument->SetDocumentCharacterSet(aCharset);
  }
  
  return NS_OK;
}

nsISupports *
nsXMLContentSink::GetTarget()
{
  return mDocument;
}

bool
nsXMLContentSink::IsScriptExecuting()
{
  return IsScriptExecutingImpl();
}

nsresult
nsXMLContentSink::FlushText(bool aReleaseTextNode)
{
  nsresult rv = NS_OK;

  if (mTextLength != 0) {
    if (mLastTextNode) {
      if ((mLastTextNodeSize + mTextLength) > mTextSize && !mXSLTProcessor) {
        mLastTextNodeSize = 0;
        mLastTextNode = nullptr;
        FlushText(aReleaseTextNode);
      } else {
        bool notify = HaveNotifiedForCurrentContent();
        
        
        
        if (notify) {
          ++mInNotification;
        }
        rv = mLastTextNode->AppendText(mText, mTextLength, notify);
        if (notify) {
          --mInNotification;
        }

        mLastTextNodeSize += mTextLength;
        mTextLength = 0;
      }
    } else {
      nsRefPtr<nsTextNode> textContent = new nsTextNode(mNodeInfoManager);

      mLastTextNode = textContent;
      
      
      textContent->SetText(mText, mTextLength, false);
      mLastTextNodeSize += mTextLength;
      mTextLength = 0;

      
      rv = AddContentAsLeaf(textContent);
    }
  }

  if (aReleaseTextNode) {
    mLastTextNodeSize = 0;
    mLastTextNode = nullptr;
  }
  
  return rv;
}

nsIContent*
nsXMLContentSink::GetCurrentContent()
{
  if (mContentStack.Length() == 0) {
    return nullptr;
  }
  return GetCurrentStackNode()->mContent;
}

StackNode*
nsXMLContentSink::GetCurrentStackNode()
{
  int32_t count = mContentStack.Length();
  return count != 0 ? &mContentStack[count-1] : nullptr;
}


nsresult
nsXMLContentSink::PushContent(nsIContent *aContent)
{
  NS_PRECONDITION(aContent, "Null content being pushed!");
  StackNode *sn = mContentStack.AppendElement();
  NS_ENSURE_TRUE(sn, NS_ERROR_OUT_OF_MEMORY);

  nsIContent* contentToPush = aContent;

  
  
  if (contentToPush->IsHTMLElement(nsGkAtoms::_template)) {
    HTMLTemplateElement* templateElement =
      static_cast<HTMLTemplateElement*>(contentToPush);
    contentToPush = templateElement->Content();
  }

  sn->mContent = contentToPush;
  sn->mNumFlushed = 0;
  return NS_OK;
}

void
nsXMLContentSink::PopContent()
{
  int32_t count = mContentStack.Length();

  if (count == 0) {
    NS_WARNING("Popping empty stack");
    return;
  }

  mContentStack.RemoveElementAt(count - 1);
}

bool
nsXMLContentSink::HaveNotifiedForCurrentContent() const
{
  uint32_t stackLength = mContentStack.Length();
  if (stackLength) {
    const StackNode& stackNode = mContentStack[stackLength - 1];
    nsIContent* parent = stackNode.mContent;
    return stackNode.mNumFlushed == parent->GetChildCount();
  }
  return true;
}

void
nsXMLContentSink::MaybeStartLayout(bool aIgnorePendingSheets)
{
  
  
  if (mLayoutStarted || mXSLTProcessor || CanStillPrettyPrint()) {
    return;
  }
  StartLayout(aIgnorePendingSheets);
}



bool
nsXMLContentSink::SetDocElement(int32_t aNameSpaceID,
                                nsIAtom* aTagName,
                                nsIContent *aContent)
{
  if (mDocElement)
    return false;

  
  
  if ((aNameSpaceID == kNameSpaceID_XBL &&
       aTagName == nsGkAtoms::bindings) ||
      (aNameSpaceID == kNameSpaceID_XSLT &&
       (aTagName == nsGkAtoms::stylesheet ||
        aTagName == nsGkAtoms::transform))) {
    mPrettyPrintHasSpecialRoot = true;
    if (mPrettyPrintXML) {
      
      
      mDocument->ScriptLoader()->SetEnabled(false);
      if (mCSSLoader) {
        mCSSLoader->SetEnabled(false);
      }
    }        
  }

  mDocElement = aContent;
  nsresult rv = mDocument->AppendChildTo(mDocElement, NotifyForDocElement());
  if (NS_FAILED(rv)) {
    
    
    return false;
  }

  if (aTagName == nsGkAtoms::html &&
      aNameSpaceID == kNameSpaceID_XHTML) {
    ProcessOfflineManifest(aContent);
  }

  return true;
}

NS_IMETHODIMP
nsXMLContentSink::HandleStartElement(const char16_t *aName,
                                     const char16_t **aAtts,
                                     uint32_t aAttsCount,
                                     uint32_t aLineNumber)
{
  return HandleStartElement(aName, aAtts, aAttsCount, aLineNumber,
                            true);
}

nsresult
nsXMLContentSink::HandleStartElement(const char16_t *aName,
                                     const char16_t **aAtts,
                                     uint32_t aAttsCount,
                                     uint32_t aLineNumber,
                                     bool aInterruptable)
{
  NS_PRECONDITION(aAttsCount % 2 == 0, "incorrect aAttsCount");
  
  aAttsCount /= 2;

  nsresult result = NS_OK;
  bool appendContent = true;
  nsCOMPtr<nsIContent> content;

  
  
  
  PR_ASSERT(eXMLContentSinkState_InEpilog != mState);

  FlushText();
  DidAddContent();

  mState = eXMLContentSinkState_InDocumentElement;

  int32_t nameSpaceID;
  nsCOMPtr<nsIAtom> prefix, localName;
  nsContentUtils::SplitExpatName(aName, getter_AddRefs(prefix),
                                 getter_AddRefs(localName), &nameSpaceID);

  if (!OnOpenContainer(aAtts, aAttsCount, nameSpaceID, localName, aLineNumber)) {
    return NS_OK;
  }
  
  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(localName, prefix, nameSpaceID,
                                           nsIDOMNode::ELEMENT_NODE);

  result = CreateElement(aAtts, aAttsCount, nodeInfo, aLineNumber,
                         getter_AddRefs(content), &appendContent,
                         FROM_PARSER_NETWORK);
  NS_ENSURE_SUCCESS(result, result);

  
  
  
  
  nsCOMPtr<nsIContent> parent = GetCurrentContent();
  
  result = PushContent(content);
  NS_ENSURE_SUCCESS(result, result);

  
  result = AddAttributes(aAtts, content);

  if (NS_OK == result) {
    
    if (!SetDocElement(nameSpaceID, localName, content) && appendContent) {
      NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);

      parent->AppendChildTo(content, false);
    }
  }

  
  
  if (nodeInfo->NamespaceID() == kNameSpaceID_XHTML) {
    if (nodeInfo->NameAtom() == nsGkAtoms::input ||
        nodeInfo->NameAtom() == nsGkAtoms::button ||
        nodeInfo->NameAtom() == nsGkAtoms::menuitem ||
        nodeInfo->NameAtom() == nsGkAtoms::audio ||
        nodeInfo->NameAtom() == nsGkAtoms::video) {
      content->DoneCreatingElement();
    } else if (nodeInfo->NameAtom() == nsGkAtoms::head && !mCurrentHead) {
      mCurrentHead = content;
    }
  }

  if (IsMonolithicContainer(nodeInfo)) {
    mInMonolithicContainer++;
  }

  if (content != mDocElement && !mCurrentHead) {
    
    
    MaybeStartLayout(false);
  }

  if (content == mDocElement) {
    NotifyDocElementCreated(mDocument);
  }

  return aInterruptable && NS_SUCCEEDED(result) ? DidProcessATokenImpl() :
                                                  result;
}

NS_IMETHODIMP
nsXMLContentSink::HandleEndElement(const char16_t *aName)
{
  return HandleEndElement(aName, true);
}

nsresult
nsXMLContentSink::HandleEndElement(const char16_t *aName,
                                   bool aInterruptable)
{
  nsresult result = NS_OK;

  
  
  
  PR_ASSERT(eXMLContentSinkState_InDocumentElement == mState);

  FlushText();

  StackNode* sn = GetCurrentStackNode();
  if (!sn) {
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIContent> content;
  sn->mContent.swap(content);
  uint32_t numFlushed = sn->mNumFlushed;

  PopContent();
  NS_ASSERTION(content, "failed to pop content");
#ifdef DEBUG
  
  nsCOMPtr<nsIAtom> debugNameSpacePrefix, debugTagAtom;
  int32_t debugNameSpaceID;
  nsContentUtils::SplitExpatName(aName, getter_AddRefs(debugNameSpacePrefix),
                                 getter_AddRefs(debugTagAtom),
                                 &debugNameSpaceID);
  
  
  
  bool isTemplateElement = debugTagAtom == nsGkAtoms::_template &&
                           debugNameSpaceID == kNameSpaceID_XHTML;
  NS_ASSERTION(content->NodeInfo()->Equals(debugTagAtom, debugNameSpaceID) ||
               isTemplateElement, "Wrong element being closed");
#endif  

  result = CloseElement(content);

  if (mCurrentHead == content) {
    mCurrentHead = nullptr;
  }
  
  if (mDocElement == content) {
    
    
    mState = eXMLContentSinkState_InEpilog;

    
    MaybeStartLayout(false);
  }

  int32_t stackLen = mContentStack.Length();
  if (mNotifyLevel >= stackLen) {
    if (numFlushed < content->GetChildCount()) {
    	  NotifyAppend(content, numFlushed);
    }
    mNotifyLevel = stackLen - 1;
  }
  DidAddContent();

  if (content->IsSVGElement(nsGkAtoms::svg)) {
    FlushTags();
    nsCOMPtr<nsIRunnable> event = new nsHtml5SVGLoadDispatcher(content);
    if (NS_FAILED(NS_DispatchToMainThread(event))) {
      NS_WARNING("failed to dispatch svg load dispatcher");
    }
  }

  return aInterruptable && NS_SUCCEEDED(result) ? DidProcessATokenImpl() :
                                                  result;
}

NS_IMETHODIMP 
nsXMLContentSink::HandleComment(const char16_t *aName)
{
  FlushText();

  nsRefPtr<Comment> comment = new Comment(mNodeInfoManager);
  comment->SetText(nsDependentString(aName), false);
  nsresult rv = AddContentAsLeaf(comment);
  DidAddContent();

  return NS_SUCCEEDED(rv) ? DidProcessATokenImpl() : rv;
}

NS_IMETHODIMP 
nsXMLContentSink::HandleCDataSection(const char16_t *aData, 
                                     uint32_t aLength)
{
  
  
  if (mXSLTProcessor) {
    return AddText(aData, aLength);
  }

  FlushText();
  
  nsRefPtr<CDATASection> cdata = new CDATASection(mNodeInfoManager);
  cdata->SetText(aData, aLength, false);
  nsresult rv = AddContentAsLeaf(cdata);
  DidAddContent();

  return NS_SUCCEEDED(rv) ? DidProcessATokenImpl() : rv;
}

NS_IMETHODIMP
nsXMLContentSink::HandleDoctypeDecl(const nsAString & aSubset, 
                                    const nsAString & aName, 
                                    const nsAString & aSystemId, 
                                    const nsAString & aPublicId,
                                    nsISupports* aCatalogData)
{
  FlushText();

  nsresult rv = NS_OK;

  NS_ASSERTION(mDocument, "Shouldn't get here from a document fragment");

  nsCOMPtr<nsIAtom> name = do_GetAtom(aName);
  NS_ENSURE_TRUE(name, NS_ERROR_OUT_OF_MEMORY);

  
  nsCOMPtr<nsIDOMDocumentType> docType;
  rv = NS_NewDOMDocumentType(getter_AddRefs(docType), mNodeInfoManager,
                             name, aPublicId, aSystemId, aSubset);
  if (NS_FAILED(rv) || !docType) {
    return rv;
  }

  MOZ_ASSERT(!aCatalogData, "Need to add back support for catalog style "
                            "sheets");

  nsCOMPtr<nsIContent> content = do_QueryInterface(docType);
  NS_ASSERTION(content, "doctype isn't content?");

  rv = mDocument->AppendChildTo(content, false);
  DidAddContent();
  return NS_SUCCEEDED(rv) ? DidProcessATokenImpl() : rv;
}

NS_IMETHODIMP
nsXMLContentSink::HandleCharacterData(const char16_t *aData, 
                                      uint32_t aLength)
{
  return HandleCharacterData(aData, aLength, true);
}

nsresult
nsXMLContentSink::HandleCharacterData(const char16_t *aData, uint32_t aLength,
                                      bool aInterruptable)
{
  nsresult rv = NS_OK;
  if (aData && mState != eXMLContentSinkState_InProlog &&
      mState != eXMLContentSinkState_InEpilog) {
    rv = AddText(aData, aLength);
  }
  return aInterruptable && NS_SUCCEEDED(rv) ? DidProcessATokenImpl() : rv;
}

NS_IMETHODIMP
nsXMLContentSink::HandleProcessingInstruction(const char16_t *aTarget, 
                                              const char16_t *aData)
{
  FlushText();

  const nsDependentString target(aTarget);
  const nsDependentString data(aData);

  nsCOMPtr<nsIContent> node =
    NS_NewXMLProcessingInstruction(mNodeInfoManager, target, data);

  nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(node));
  if (ssle) {
    ssle->InitStyleLinkElement(false);
    ssle->SetEnableUpdates(false);
    mPrettyPrintXML = false;
  }

  nsresult rv = AddContentAsLeaf(node);
  NS_ENSURE_SUCCESS(rv, rv);
  DidAddContent();

  if (ssle) {
    
    
    ssle->SetEnableUpdates(true);
    bool willNotify;
    bool isAlternate;
    rv = ssle->UpdateStyleSheet(mRunsToCompletion ? nullptr : this,
                                &willNotify,
                                &isAlternate);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (willNotify) {
      
      if (!isAlternate && !mRunsToCompletion) {
        ++mPendingSheetCount;
        mScriptLoader->AddExecuteBlocker();
      }

      return NS_OK;
    }
  }

  
  nsAutoString type;
  nsContentUtils::GetPseudoAttributeValue(data, nsGkAtoms::type, type);

  if (mState != eXMLContentSinkState_InProlog ||
      !target.EqualsLiteral("xml-stylesheet") ||
      type.IsEmpty()                          ||
      type.LowerCaseEqualsLiteral("text/css")) {
    return DidProcessATokenImpl();
  }

  nsAutoString href, title, media;
  bool isAlternate = false;

  
  if (!ParsePIData(data, href, title, media, isAlternate)) {
      return DidProcessATokenImpl();
  }

  rv = ProcessStyleLink(node, href, isAlternate, title, type, media);
  return NS_SUCCEEDED(rv) ? DidProcessATokenImpl() : rv;
}


bool
nsXMLContentSink::ParsePIData(const nsString &aData, nsString &aHref,
                              nsString &aTitle, nsString &aMedia,
                              bool &aIsAlternate)
{
  
  if (!nsContentUtils::GetPseudoAttributeValue(aData, nsGkAtoms::href, aHref)) {
    return false;
  }

  nsContentUtils::GetPseudoAttributeValue(aData, nsGkAtoms::title, aTitle);

  nsContentUtils::GetPseudoAttributeValue(aData, nsGkAtoms::media, aMedia);

  nsAutoString alternate;
  nsContentUtils::GetPseudoAttributeValue(aData,
                                          nsGkAtoms::alternate,
                                          alternate);

  aIsAlternate = alternate.EqualsLiteral("yes");

  return true;
}

NS_IMETHODIMP
nsXMLContentSink::HandleXMLDeclaration(const char16_t *aVersion,
                                       const char16_t *aEncoding,
                                       int32_t aStandalone)
{
  mDocument->SetXMLDeclaration(aVersion, aEncoding, aStandalone);

  return DidProcessATokenImpl();
}

NS_IMETHODIMP
nsXMLContentSink::ReportError(const char16_t* aErrorText, 
                              const char16_t* aSourceText,
                              nsIScriptError *aError,
                              bool *_retval)
{
  NS_PRECONDITION(aError && aSourceText && aErrorText, "Check arguments!!!");
  nsresult rv = NS_OK;

  
  *_retval = true;
  
  mPrettyPrintXML = false;

  mState = eXMLContentSinkState_InProlog;

  

  
  mDocument->RemoveObserver(this);
  mIsDocumentObserver = false;

  
  
  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(mDocument));
  if (node) {
    for (;;) {
      nsCOMPtr<nsIDOMNode> child, dummy;
      node->GetLastChild(getter_AddRefs(child));
      if (!child)
        break;
      node->RemoveChild(child, getter_AddRefs(dummy));
    }
  }
  mDocElement = nullptr;

  
  
  
  mTextLength = 0;

  if (mXSLTProcessor) {
    
    mXSLTProcessor->CancelLoads();
    mXSLTProcessor = nullptr;
  }

  
  mContentStack.Clear();
  mNotifyLevel = 0;

  rv = HandleProcessingInstruction(MOZ_UTF16("xml-stylesheet"),
                                   MOZ_UTF16("href=\"chrome://global/locale/intl.css\" type=\"text/css\""));
  NS_ENSURE_SUCCESS(rv, rv);

  const char16_t* noAtts[] = { 0, 0 };

  NS_NAMED_LITERAL_STRING(errorNs,
                          "http://www.mozilla.org/newlayout/xml/parsererror.xml");

  nsAutoString parsererror(errorNs);
  parsererror.Append((char16_t)0xFFFF);
  parsererror.AppendLiteral("parsererror");
  
  rv = HandleStartElement(parsererror.get(), noAtts, 0, (uint32_t)-1,
                          false);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = HandleCharacterData(aErrorText, NS_strlen(aErrorText), false);
  NS_ENSURE_SUCCESS(rv, rv);  
  
  nsAutoString sourcetext(errorNs);
  sourcetext.Append((char16_t)0xFFFF);
  sourcetext.AppendLiteral("sourcetext");

  rv = HandleStartElement(sourcetext.get(), noAtts, 0, (uint32_t)-1,
                          false);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = HandleCharacterData(aSourceText, NS_strlen(aSourceText), false);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = HandleEndElement(sourcetext.get(), false);
  NS_ENSURE_SUCCESS(rv, rv); 
  
  rv = HandleEndElement(parsererror.get(), false);
  NS_ENSURE_SUCCESS(rv, rv);

  FlushTags();

  return NS_OK;
}

nsresult
nsXMLContentSink::AddAttributes(const char16_t** aAtts,
                                nsIContent* aContent)
{
  
  nsCOMPtr<nsIAtom> prefix, localName;
  while (*aAtts) {
    int32_t nameSpaceID;
    nsContentUtils::SplitExpatName(aAtts[0], getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    
    aContent->SetAttr(nameSpaceID, localName, prefix,
                      nsDependentString(aAtts[1]), false);
    aAtts += 2;
  }

  return NS_OK;
}

#define NS_ACCUMULATION_BUFFER_SIZE 4096

nsresult
nsXMLContentSink::AddText(const char16_t* aText, 
                          int32_t aLength)
{
  
  if (0 == mTextSize) {
    mText = (char16_t *) PR_MALLOC(sizeof(char16_t) * NS_ACCUMULATION_BUFFER_SIZE);
    if (nullptr == mText) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mTextSize = NS_ACCUMULATION_BUFFER_SIZE;
  }

  
  int32_t offset = 0;
  while (0 != aLength) {
    int32_t amount = mTextSize - mTextLength;
    if (0 == amount) {
      
      if (mConstrainSize && !mXSLTProcessor) {
        nsresult rv = FlushText();
        if (NS_OK != rv) {
          return rv;
        }

        amount = mTextSize - mTextLength;
      }
      else {
        mTextSize += aLength;
        mText = (char16_t *) PR_REALLOC(mText, sizeof(char16_t) * mTextSize);
        if (nullptr == mText) {
          mTextSize = 0;

          return NS_ERROR_OUT_OF_MEMORY;
        }

        amount = aLength;
      }
    }
    if (amount > aLength) {
      amount = aLength;
    }
    memcpy(&mText[mTextLength], &aText[offset], sizeof(char16_t) * amount);
    mTextLength += amount;
    offset += amount;
    aLength -= amount;
  }

  return NS_OK;
}

void
nsXMLContentSink::FlushPendingNotifications(mozFlushType aType)
{
  
  
  if (!mInNotification) {
    if (mIsDocumentObserver) {
      
      
      if (aType >= Flush_ContentAndNotify) {
        FlushTags();
      }
      else {
        FlushText(false);
      }
    }
    if (aType >= Flush_InterruptibleLayout) {
      
      
      MaybeStartLayout(true);
    }
  }
}










nsresult
nsXMLContentSink::FlushTags()
{
  mDeferredFlushTags = false;
  bool oldBeganUpdate = mBeganUpdate;
  uint32_t oldUpdates = mUpdatesInNotification;

  mUpdatesInNotification = 0;
  ++mInNotification;
  {
    
    mozAutoDocUpdate updateBatch(mDocument, UPDATE_CONTENT_MODEL, true);
    mBeganUpdate = true;

    
    FlushText(false);

    
    
    

    int32_t stackPos;
    int32_t stackLen = mContentStack.Length();
    bool flushed = false;
    uint32_t childCount;
    nsIContent* content;

    for (stackPos = 0; stackPos < stackLen; ++stackPos) {
      content = mContentStack[stackPos].mContent;
      childCount = content->GetChildCount();

      if (!flushed && (mContentStack[stackPos].mNumFlushed < childCount)) {
        NotifyAppend(content, mContentStack[stackPos].mNumFlushed);
        flushed = true;
      }

      mContentStack[stackPos].mNumFlushed = childCount;
    }
    mNotifyLevel = stackLen - 1;
  }
  --mInNotification;

  if (mUpdatesInNotification > 1) {
    UpdateChildCounts();
  }

  mUpdatesInNotification = oldUpdates;
  mBeganUpdate = oldBeganUpdate;
  
  return NS_OK;
}




void
nsXMLContentSink::UpdateChildCounts()
{
  
  
  
  
  
  int32_t stackLen = mContentStack.Length();
  int32_t stackPos = stackLen - 1;
  while (stackPos >= 0) {
    StackNode & node = mContentStack[stackPos];
    node.mNumFlushed = node.mContent->GetChildCount();

    stackPos--;
  }
  mNotifyLevel = stackLen - 1;
}

bool
nsXMLContentSink::IsMonolithicContainer(mozilla::dom::NodeInfo* aNodeInfo)
{
  return ((aNodeInfo->NamespaceID() == kNameSpaceID_XHTML &&
          (aNodeInfo->NameAtom() == nsGkAtoms::tr ||
           aNodeInfo->NameAtom() == nsGkAtoms::select ||
           aNodeInfo->NameAtom() == nsGkAtoms::object ||
           aNodeInfo->NameAtom() == nsGkAtoms::applet)) ||
          (aNodeInfo->NamespaceID() == kNameSpaceID_MathML &&
          (aNodeInfo->NameAtom() == nsGkAtoms::math))
          );
}

void
nsXMLContentSink::ContinueInterruptedParsingIfEnabled()
{
  if (mParser && mParser->IsParserEnabled()) {
    GetParser()->ContinueInterruptedParsing();
  }
}

void
nsXMLContentSink::ContinueInterruptedParsingAsync()
{
  nsCOMPtr<nsIRunnable> ev = NS_NewRunnableMethod(this,
    &nsXMLContentSink::ContinueInterruptedParsingIfEnabled);

  NS_DispatchToCurrentThread(ev);
}

nsIParser*
nsXMLContentSink::GetParser()
{
  return static_cast<nsIParser*>(mParser.get());
}
