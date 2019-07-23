






































#include "nsCOMPtr.h"
#include "nsXMLContentSink.h"
#include "nsIParser.h"
#include "nsIUnicharInputStream.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMNSDocument.h"
#include "nsIContent.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIDOMComment.h"
#include "nsIDOMCDATASection.h"
#include "nsDOMDocumentType.h"
#include "nsHTMLParts.h"
#include "nsCRT.h"
#include "nsICSSLoader.h"
#include "nsICSSStyleSheet.h"
#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsIScriptContext.h"
#include "nsINameSpaceManager.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIContentViewer.h"
#include "prtime.h"
#include "prlog.h"
#include "prmem.h"
#include "nsParserUtils.h"
#include "nsRect.h"
#include "nsGenericElement.h"
#include "nsIWebNavigation.h"
#include "nsIScriptElement.h"
#include "nsScriptLoader.h"
#include "nsStyleLinkElement.h"
#include "nsIImageLoadingContent.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsICookieService.h"
#include "nsIPrompt.h"
#include "nsIDOMWindowInternal.h"
#include "nsIChannel.h"
#include "nsIPrincipal.h"
#include "nsXMLPrettyPrinter.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"
#include "nsIDOMProcessingInstruction.h"
#include "nsNodeUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsEventDispatcher.h"
#include "mozAutoDocUpdate.h"

#ifdef MOZ_SVG
#include "nsGUIEvent.h"
#endif

#define kXSLType "text/xsl"










nsresult
NS_NewXMLContentSink(nsIXMLContentSink** aResult,
                     nsIDocument* aDoc,
                     nsIURI* aURI,
                     nsISupports* aContainer,
                     nsIChannel* aChannel)
{
  NS_PRECONDITION(nsnull != aResult, "null ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsXMLContentSink* it;
  NS_NEWXPCOM(it, nsXMLContentSink);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  nsCOMPtr<nsIXMLContentSink> kungFuDeathGrip = it;
  nsresult rv = it->Init(aDoc, aURI, aContainer, aChannel);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return CallQueryInterface(it, aResult);
}

nsXMLContentSink::nsXMLContentSink()
  : mConstrainSize(PR_TRUE),
    mPrettyPrintXML(PR_TRUE),
    mAllowAutoXLinks(PR_TRUE)
{
}

nsXMLContentSink::~nsXMLContentSink()
{
  NS_IF_RELEASE(mDocElement);
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
  mIsDocumentObserver = PR_TRUE;

  if (!mDocShell) {
    mPrettyPrintXML = PR_FALSE;
  }
  
  mState = eXMLContentSinkState_InProlog;
  mDocElement = nsnull;

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
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCurrentHead)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(mDocElement)
  for (PRUint32 i = 0, count = tmp->mContentStack.Length(); i < count; i++) {
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
    nsCAutoString command;
    mParser->GetCommand(command);
    if (!command.EqualsLiteral("view")) {
      mPrettyPrintXML = PR_FALSE;
    }
  }
  
  return NS_OK;
}

PRBool
nsXMLContentSink::CanStillPrettyPrint()
{
  return mPrettyPrintXML &&
         (!mPrettyPrintHasFactoredElements || mPrettyPrintHasSpecialRoot);
}

nsresult
nsXMLContentSink::MaybePrettyPrint()
{
  if (!CanStillPrettyPrint()) {
    mPrettyPrintXML = PR_FALSE;

    return NS_OK;
  }

  
  mDocument->RemoveObserver(this);
  mIsDocumentObserver = PR_FALSE;

  
  if (mCSSLoader) {
    mCSSLoader->SetEnabled(PR_TRUE);
  }
  
  nsCOMPtr<nsXMLPrettyPrinter> printer;
  nsresult rv = NS_NewXMLPrettyPrinter(getter_AddRefs(printer));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isPrettyPrinting;
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
    nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::prefix,
                                           prefix);
    if (!prefix.IsEmpty() &&
        nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::_namespace,
                                               namespaceAttr)) {
      aProcessor->AddXSLTParamNamespace(prefix, namespaceAttr);
    }
  }

  
  else if (target.EqualsLiteral("xslt-param")) {
    aPi->GetData(data);
    nsAutoString name, namespaceAttr, select, value;
    nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::name,
                                           name);
    nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::_namespace,
                                           namespaceAttr);
    if (!nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::select, select)) {
      select.SetIsVoid(PR_TRUE);
    }
    if (!nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::value, value)) {
      value.SetIsVoid(PR_TRUE);
    }
    if (!name.IsEmpty()) {
      nsCOMPtr<nsIDOMNode> doc = do_QueryInterface(aDocument);
      aProcessor->AddXSLTParam(name, namespaceAttr, select, value, doc);
    }
  }
}

NS_IMETHODIMP
nsXMLContentSink::DidBuildModel(PRBool aTerminated)
{
  DidBuildModelImpl(aTerminated);

  if (mXSLTProcessor) {
    
    mDocument->RemoveObserver(this);
    mIsDocumentObserver = PR_FALSE;

    
    PRUint32 i;
    nsIContent* child;
    for (i = 0; (child = mDocument->GetChildAt(i)); ++i) {
      if (child->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION)) {
        nsCOMPtr<nsIDOMProcessingInstruction> pi = do_QueryInterface(child);
        CheckXSLTParamPI(pi, mXSLTProcessor, mDocument);
      }
      else if (child->IsNodeOfType(nsINode::eELEMENT)) {
        
        break;
      }
    }

    nsCOMPtr<nsIDOMDocument> currentDOMDoc(do_QueryInterface(mDocument));
    mXSLTProcessor->SetSourceContentModel(currentDOMDoc);
    
    
    mXSLTProcessor = nsnull;
  }
  else {
    
    mDocument->ScriptLoader()->RemoveObserver(this);

    if (mDocElement) {
      
      
      
      
      NS_ASSERTION(mDocument->IndexOf(mDocElement) != -1,
                   "mDocElement not in doc?");
    }

    
    MaybePrettyPrint();

    PRBool startLayout = PR_TRUE;
    
    if (mPrettyPrinting) {
      NS_ASSERTION(!mPendingSheetCount, "Shouldn't have pending sheets here!");
      
      
      
      if (mDocument->CSSLoader()->HasPendingLoads() &&
          NS_SUCCEEDED(mDocument->CSSLoader()->AddObserver(this))) {
        
        startLayout = PR_FALSE;
      }
    }
    
    if (startLayout) {
      StartLayout(PR_FALSE);

      ScrollToRef();
    }

    mDocument->RemoveObserver(this);
    mIsDocumentObserver = PR_FALSE;

    mDocument->EndLoad();
  }

  DropParserAndPerfHint();

  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::OnDocumentCreated(nsIDocument* aResultDocument)
{
  NS_ENSURE_ARG(aResultDocument);

  nsCOMPtr<nsIContentViewer> contentViewer;
  mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (contentViewer) {
    nsCOMPtr<nsIDOMDocument> doc = do_QueryInterface(aResultDocument);
    return contentViewer->SetDOMDocument(doc);
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
      aResultDocument->SetMayStartLayout(PR_FALSE);
      
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
  }

  originalDocument->ScriptLoader()->RemoveObserver(this);

  
  
  
  
  nsIContent *rootContent = mDocument->GetRootContent();
  if (rootContent) {
    NS_ASSERTION(mDocument->IndexOf(rootContent) != -1,
                 "rootContent not in doc?");
    mDocument->BeginUpdate(UPDATE_CONTENT_MODEL);
    nsNodeUtils::ContentInserted(mDocument, rootContent,
                                 mDocument->IndexOf(rootContent));
    mDocument->EndUpdate(UPDATE_CONTENT_MODEL);
  }
  
  
  StartLayout(PR_FALSE);

  ScrollToRef();

  originalDocument->EndLoad();

  return NS_OK;
}

NS_IMETHODIMP
nsXMLContentSink::StyleSheetLoaded(nsICSSStyleSheet* aSheet,
                                   PRBool aWasAlternate,
                                   nsresult aStatus)
{
  if (!mPrettyPrinting) {
    return nsContentSink::StyleSheetLoaded(aSheet, aWasAlternate, aStatus);
  }

  if (!mDocument->CSSLoader()->HasPendingLoads()) {
    mDocument->CSSLoader()->RemoveObserver(this);
    StartLayout(PR_FALSE);
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
nsXMLContentSink::SetParser(nsIParser* aParser)
{
  NS_PRECONDITION(aParser, "Should have a parser here!");
  mParser = aParser;
  return NS_OK;
}

nsresult
nsXMLContentSink::CreateElement(const PRUnichar** aAtts, PRUint32 aAttsCount,
                                nsINodeInfo* aNodeInfo, PRUint32 aLineNumber,
                                nsIContent** aResult, PRBool* aAppendContent,
                                PRBool aFromParser)
{
  NS_ASSERTION(aNodeInfo, "can't create element without nodeinfo");

  *aResult = nsnull;
  *aAppendContent = PR_TRUE;
  nsresult rv = NS_OK;

  nsCOMPtr<nsIContent> content;
  rv = NS_NewElement(getter_AddRefs(content), aNodeInfo->NamespaceID(),
                     aNodeInfo, aFromParser);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_XHTML)
#ifdef MOZ_SVG
      || aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_SVG)
#endif
    ) {
    nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(content);
    sele->SetScriptLineNumber(aLineNumber);
    if (aNodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_SVG)) {
      sele->WillCallDoneAddingChildren();
    }
    mConstrainSize = PR_FALSE;
  }

  
  if (aNodeInfo->NamespaceEquals(kNameSpaceID_XHTML)) {
    mPrettyPrintHasFactoredElements = PR_TRUE;
  }
  else {
    
    if (!mPrettyPrintHasFactoredElements && !mPrettyPrintHasSpecialRoot &&
        mPrettyPrintXML) {
      mPrettyPrintHasFactoredElements =
        nsContentUtils::NameSpaceManager()->
          HasElementCreator(aNodeInfo->NamespaceID());
    }

    if (!aNodeInfo->NamespaceEquals(kNameSpaceID_SVG)) {
      content.swap(*aResult);

      return NS_OK;
    }
  }

  if (aNodeInfo->Equals(nsGkAtoms::link, kNameSpaceID_XHTML) ||
      aNodeInfo->Equals(nsGkAtoms::style, kNameSpaceID_XHTML) ||
      aNodeInfo->Equals(nsGkAtoms::style, kNameSpaceID_SVG)) {
    nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(content));
    if (ssle) {
      ssle->InitStyleLinkElement(PR_FALSE);
      ssle->SetEnableUpdates(PR_FALSE);
      if (!aNodeInfo->Equals(nsGkAtoms::link, kNameSpaceID_XHTML)) {
        ssle->SetLineNumber(aLineNumber);
      }
    }
  } 

  content.swap(*aResult);

  return NS_OK;
}


nsresult
nsXMLContentSink::CloseElement(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "missing element to close");

  nsINodeInfo *nodeInfo = aContent->NodeInfo();

  
  
  if ((nodeInfo->NamespaceID() == kNameSpaceID_XHTML &&
       (nodeInfo->NameAtom() == nsGkAtoms::select ||
        nodeInfo->NameAtom() == nsGkAtoms::textarea ||
#ifdef MOZ_MEDIA
        nodeInfo->NameAtom() == nsGkAtoms::video ||
        nodeInfo->NameAtom() == nsGkAtoms::audio ||
#endif
        nodeInfo->NameAtom() == nsGkAtoms::object ||
        nodeInfo->NameAtom() == nsGkAtoms::applet))
#ifdef MOZ_XTF
      || nodeInfo->NamespaceID() > kNameSpaceID_LastBuiltin
#endif
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

  nsresult rv = NS_OK;

  if (nodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_XHTML)
#ifdef MOZ_SVG
      || nodeInfo->Equals(nsGkAtoms::script, kNameSpaceID_SVG)
#endif
    ) {
    mConstrainSize = PR_TRUE; 

    
    
    
    rv = aContent->DoneAddingChildren(PR_TRUE);

    
    
    if (rv == NS_ERROR_HTMLPARSER_BLOCK) {
      nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(aContent);
      mScriptElements.AppendObject(sele);
    }

    
    
    if (mParser && !mParser->IsParserEnabled()) {
      
      mParser->BlockParser();
      rv = NS_ERROR_HTMLPARSER_BLOCK;
    }

    return rv;
  }
  
  if (nodeInfo->Equals(nsGkAtoms::base, kNameSpaceID_XHTML) &&
      !mHasProcessedBase) {
    
    ProcessBASETag(aContent);
    mHasProcessedBase = PR_TRUE;
  }
  else if (nodeInfo->Equals(nsGkAtoms::meta, kNameSpaceID_XHTML) &&
           
           
           (!mPrettyPrintXML || !mPrettyPrintHasSpecialRoot)) {
    rv = ProcessMETATag(aContent);
  }
  else if (nodeInfo->Equals(nsGkAtoms::link, kNameSpaceID_XHTML) ||
           nodeInfo->Equals(nsGkAtoms::style, kNameSpaceID_XHTML) ||
           nodeInfo->Equals(nsGkAtoms::style, kNameSpaceID_SVG)) {
    nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(aContent));
    if (ssle) {
      ssle->SetEnableUpdates(PR_TRUE);
      PRBool willNotify;
      PRBool isAlternate;
      rv = ssle->UpdateStyleSheet(this, &willNotify, &isAlternate);
      if (NS_SUCCEEDED(rv) && willNotify && !isAlternate) {
        ++mPendingSheetCount;
        mScriptLoader->AddExecuteBlocker();
      }
    }
    
    if (nodeInfo->Equals(nsGkAtoms::link, kNameSpaceID_XHTML)) {
      nsAutoString relVal;
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::rel, relVal);
      if (relVal.EqualsLiteral("dns-prefetch")) {
        nsAutoString hrefVal;
        aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::href, hrefVal);
        if (!hrefVal.IsEmpty()) {
          PrefetchDNS(hrefVal);
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
    mDocument->AppendChildTo(aContent, PR_FALSE);
  }
  else {
    nsCOMPtr<nsIContent> parent = GetCurrentContent();

    if (parent) {
      result = parent->AppendChildTo(aContent, PR_FALSE);
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

  processor->Init(mDocument->NodePrincipal());
  processor->SetTransformObserver(this);

  nsCOMPtr<nsILoadGroup> loadGroup = mDocument->GetDocumentLoadGroup();
  if (!loadGroup) {
    return NS_ERROR_FAILURE;
  }

  if (NS_SUCCEEDED(processor->LoadStyleSheet(aUrl, loadGroup))) {
    mXSLTProcessor.swap(processor);
  }

  
  
  

  return NS_OK;
}

nsresult
nsXMLContentSink::ProcessStyleLink(nsIContent* aElement,
                                   const nsSubstring& aHref,
                                   PRBool aAlternate,
                                   const nsSubstring& aTitle,
                                   const nsSubstring& aType,
                                   const nsSubstring& aMedia)
{
  nsresult rv = NS_OK;
  mPrettyPrintXML = PR_FALSE;

  nsCAutoString cmd;
  if (mParser)
    mParser->GetCommand(cmd);
  if (cmd.EqualsASCII(kLoadAsData))
    return NS_OK; 

  NS_ConvertUTF16toUTF8 type(aType);
  if (type.EqualsIgnoreCase(kXSLType) ||
      type.EqualsIgnoreCase(kXMLTextContentType) ||
      type.EqualsIgnoreCase(kXMLApplicationContentType)) {
    if (aAlternate) {
      
      return NS_OK;
    }
    
    if (!mDocShell)
      return NS_OK;

    nsCOMPtr<nsIURI> url;
    rv = NS_NewURI(getter_AddRefs(url), aHref, nsnull, mDocumentBaseURI);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();
    rv = secMan->
      CheckLoadURIWithPrincipal(mDocument->NodePrincipal(), url,
                                nsIScriptSecurityManager::ALLOW_CHROME);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    
    PRInt16 decision = nsIContentPolicy::ACCEPT;
    rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_STYLESHEET,
                                   url,
                                   mDocument->NodePrincipal(),
                                   aElement,
                                   type,
                                   nsnull,
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

void
nsXMLContentSink::ProcessBASETag(nsIContent* aContent)
{
  NS_ASSERTION(aContent, "missing base-element");

  if (mDocument) {
    nsAutoString value;
  
    if (aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::target, value)) {
      mDocument->SetBaseTarget(value);
    }

    if (aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::href, value)) {
      nsCOMPtr<nsIURI> baseURI;
      nsresult rv = NS_NewURI(getter_AddRefs(baseURI), value);
      if (NS_SUCCEEDED(rv)) {
        rv = mDocument->SetBaseURI(baseURI); 
        if (NS_SUCCEEDED(rv)) {
          mDocumentBaseURI = mDocument->GetBaseURI();
        }
      }
    }
  }
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

PRBool
nsXMLContentSink::IsScriptExecuting()
{
  return IsScriptExecutingImpl();
}

nsresult
nsXMLContentSink::FlushText(PRBool aReleaseTextNode)
{
  nsresult rv = NS_OK;

  if (mTextLength != 0) {
    if (mLastTextNode) {
      if ((mLastTextNodeSize + mTextLength) > mTextSize && !mXSLTProcessor) {
        mLastTextNodeSize = 0;
        mLastTextNode = nsnull;
        FlushText(aReleaseTextNode);
      } else {
        PRBool notify = HaveNotifiedForCurrentContent();
        
        
        
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
      nsCOMPtr<nsIContent> textContent;
      rv = NS_NewTextNode(getter_AddRefs(textContent),
                          mNodeInfoManager);
      NS_ENSURE_SUCCESS(rv, rv);

      mLastTextNode = textContent;
      
      
      textContent->SetText(mText, mTextLength, PR_FALSE);
      mLastTextNodeSize += mTextLength;
      mTextLength = 0;

      
      rv = AddContentAsLeaf(textContent);
    }
  }

  if (aReleaseTextNode) {
    mLastTextNodeSize = 0;
    mLastTextNode = nsnull;
  }
  
  return rv;
}

nsIContent*
nsXMLContentSink::GetCurrentContent()
{
  if (mContentStack.Length() == 0) {
    return nsnull;
  }
  return GetCurrentStackNode().mContent;
}

StackNode &
nsXMLContentSink::GetCurrentStackNode()
{
  PRInt32 count = mContentStack.Length();
  NS_ASSERTION(count > 0, "Bogus Length()");
  return mContentStack[count-1];
}


nsresult
nsXMLContentSink::PushContent(nsIContent *aContent)
{
  NS_PRECONDITION(aContent, "Null content being pushed!");
  StackNode *sn = mContentStack.AppendElement();
  NS_ENSURE_TRUE(sn, NS_ERROR_OUT_OF_MEMORY);

  sn->mContent = aContent;
  sn->mNumFlushed = 0;
  return NS_OK;
}

void
nsXMLContentSink::PopContent()
{
  PRInt32 count = mContentStack.Length();

  if (count == 0) {
    NS_WARNING("Popping empty stack");
    return;
  }

  mContentStack.RemoveElementAt(count - 1);
}

PRBool
nsXMLContentSink::HaveNotifiedForCurrentContent() const
{
  PRUint32 stackLength = mContentStack.Length();
  if (stackLength) {
    const StackNode& stackNode = mContentStack[stackLength - 1];
    nsIContent* parent = stackNode.mContent;
    return stackNode.mNumFlushed == parent->GetChildCount();
  }
  return PR_TRUE;
}

void
nsXMLContentSink::MaybeStartLayout(PRBool aIgnorePendingSheets)
{
  
  
  if (mLayoutStarted || mXSLTProcessor || CanStillPrettyPrint()) {
    return;
  }
  StartLayout(aIgnorePendingSheets);
}



PRBool
nsXMLContentSink::SetDocElement(PRInt32 aNameSpaceID,
                                nsIAtom* aTagName,
                                nsIContent *aContent)
{
  if (mDocElement)
    return PR_FALSE;

  
  
  if ((aNameSpaceID == kNameSpaceID_XBL &&
       aTagName == nsGkAtoms::bindings) ||
      (aNameSpaceID == kNameSpaceID_XSLT &&
       (aTagName == nsGkAtoms::stylesheet ||
        aTagName == nsGkAtoms::transform))) {
    mPrettyPrintHasSpecialRoot = PR_TRUE;
    if (mPrettyPrintXML) {
      
      
      mAllowAutoXLinks = PR_FALSE;
      mDocument->ScriptLoader()->SetEnabled(PR_FALSE);
      if (mCSSLoader) {
        mCSSLoader->SetEnabled(PR_FALSE);
      }
    }        
  }

  mDocElement = aContent;
  NS_ADDREF(mDocElement);
  nsresult rv = mDocument->AppendChildTo(mDocElement, PR_TRUE);
  if (NS_FAILED(rv)) {
    
    
    return PR_FALSE;
  }

  if (aTagName == nsGkAtoms::html &&
      aNameSpaceID == kNameSpaceID_XHTML) {
    ProcessOfflineManifest(aContent);
  }

  return PR_TRUE;
}

NS_IMETHODIMP
nsXMLContentSink::HandleStartElement(const PRUnichar *aName,
                                     const PRUnichar **aAtts,
                                     PRUint32 aAttsCount,
                                     PRInt32 aIndex,
                                     PRUint32 aLineNumber)
{
  return HandleStartElement(aName, aAtts, aAttsCount, aIndex, aLineNumber,
                            PR_TRUE);
}

nsresult
nsXMLContentSink::HandleStartElement(const PRUnichar *aName,
                                     const PRUnichar **aAtts,
                                     PRUint32 aAttsCount,
                                     PRInt32 aIndex,
                                     PRUint32 aLineNumber,
                                     PRBool aInterruptable)
{
  NS_PRECONDITION(aIndex >= -1, "Bogus aIndex");
  NS_PRECONDITION(aAttsCount % 2 == 0, "incorrect aAttsCount");
  
  aAttsCount /= 2;

  nsresult result = NS_OK;
  PRBool appendContent = PR_TRUE;
  nsCOMPtr<nsIContent> content;

  
  
  
  PR_ASSERT(eXMLContentSinkState_InEpilog != mState);

  FlushText();
  DidAddContent();

  mState = eXMLContentSinkState_InDocumentElement;

  PRInt32 nameSpaceID;
  nsCOMPtr<nsIAtom> prefix, localName;
  nsContentUtils::SplitExpatName(aName, getter_AddRefs(prefix),
                                 getter_AddRefs(localName), &nameSpaceID);

  if (!OnOpenContainer(aAtts, aAttsCount, nameSpaceID, localName, aLineNumber)) {
    return NS_OK;
  }
  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(localName, prefix, nameSpaceID);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  result = CreateElement(aAtts, aAttsCount, nodeInfo, aLineNumber,
                         getter_AddRefs(content), &appendContent, PR_TRUE);
  NS_ENSURE_SUCCESS(result, result);

  
  
  
  
  nsCOMPtr<nsIContent> parent = GetCurrentContent();
  
  result = PushContent(content);
  NS_ENSURE_SUCCESS(result, result);

  
  
  
  if (aIndex != -1 && NS_SUCCEEDED(result)) {
    nsCOMPtr<nsIAtom> IDAttr = do_GetAtom(aAtts[aIndex]);

    if (IDAttr) {
      nodeInfo->SetIDAttributeAtom(IDAttr);
    }
  }
  
#ifdef MOZ_XTF
  if (nameSpaceID > kNameSpaceID_LastBuiltin)
    content->BeginAddingChildren();
#endif

  
  result = AddAttributes(aAtts, content);

  if (NS_OK == result) {
    
    if (!SetDocElement(nameSpaceID, localName, content) && appendContent) {
      NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);

      parent->AppendChildTo(content, PR_FALSE);
    }
  }

  
  
  if (nodeInfo->NamespaceID() == kNameSpaceID_XHTML) {
    if (nodeInfo->NameAtom() == nsGkAtoms::input ||
        nodeInfo->NameAtom() == nsGkAtoms::button) {
      content->DoneCreatingElement();
    } else if (nodeInfo->NameAtom() == nsGkAtoms::head && !mCurrentHead) {
      mCurrentHead = content;
    }
  }

  if (IsMonolithicContainer(nodeInfo)) {
    mInMonolithicContainer++;
  }

  if (content != mDocElement && !mCurrentHead) {
    
    
    MaybeStartLayout(PR_FALSE);
  }

  return aInterruptable && NS_SUCCEEDED(result) ? DidProcessATokenImpl() :
                                                  result;
}

NS_IMETHODIMP
nsXMLContentSink::HandleEndElement(const PRUnichar *aName)
{
  return HandleEndElement(aName, PR_TRUE);
}

nsresult
nsXMLContentSink::HandleEndElement(const PRUnichar *aName,
                                   PRBool aInterruptable)
{
  nsresult result = NS_OK;

  
  
  
  PR_ASSERT(eXMLContentSinkState_InDocumentElement == mState);

  FlushText();

  StackNode & sn = GetCurrentStackNode();

  nsCOMPtr<nsIContent> content;
  sn.mContent.swap(content);
  PRUint32 numFlushed = sn.mNumFlushed;

  PopContent();
  NS_ASSERTION(content, "failed to pop content");
#ifdef DEBUG
  
  nsCOMPtr<nsIAtom> debugNameSpacePrefix, debugTagAtom;
  PRInt32 debugNameSpaceID;
  nsContentUtils::SplitExpatName(aName, getter_AddRefs(debugNameSpacePrefix),
                                 getter_AddRefs(debugTagAtom),
                                 &debugNameSpaceID);
  NS_ASSERTION(content->NodeInfo()->Equals(debugTagAtom, debugNameSpaceID),
               "Wrong element being closed");
#endif  

  result = CloseElement(content);

  if (mCurrentHead == content) {
    mCurrentHead = nsnull;
  }
  
  if (mDocElement == content) {
    
    
    mState = eXMLContentSinkState_InEpilog;

    
    MaybeStartLayout(PR_FALSE);
  }

  PRInt32 stackLen = mContentStack.Length();
  if (mNotifyLevel >= stackLen) {
    if (numFlushed < content->GetChildCount()) {
    	  NotifyAppend(content, numFlushed);
    }
    mNotifyLevel = stackLen - 1;
  }
  DidAddContent();

#ifdef MOZ_SVG
  if (mDocument &&
      content->GetNameSpaceID() == kNameSpaceID_SVG &&
      (
#ifdef MOZ_SMIL
       content->Tag() == nsGkAtoms::svg ||
#endif
       content->HasAttr(kNameSpaceID_None, nsGkAtoms::onload))) {
    FlushTags();

    nsEvent event(PR_TRUE, NS_SVG_LOAD);
    event.eventStructType = NS_SVG_EVENT;
    event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

    
    
    
    
    nsRefPtr<nsPresContext> ctx;
    nsCOMPtr<nsIPresShell> shell = mDocument->GetPrimaryShell();
    if (shell) {
      ctx = shell->GetPresContext();
    }
    nsEventDispatcher::Dispatch(content, ctx, &event);
  }
#endif

  return aInterruptable && NS_SUCCEEDED(result) ? DidProcessATokenImpl() :
                                                  result;
}

NS_IMETHODIMP 
nsXMLContentSink::HandleComment(const PRUnichar *aName)
{
  FlushText();

  nsCOMPtr<nsIContent> comment;
  nsresult rv = NS_NewCommentNode(getter_AddRefs(comment), mNodeInfoManager);
  if (comment) {
    comment->SetText(nsDependentString(aName), PR_FALSE);
    rv = AddContentAsLeaf(comment);
    DidAddContent();
  }

  return NS_SUCCEEDED(rv) ? DidProcessATokenImpl() : rv;
}

NS_IMETHODIMP 
nsXMLContentSink::HandleCDataSection(const PRUnichar *aData, 
                                     PRUint32 aLength)
{
  
  
  if (mXSLTProcessor) {
    return AddText(aData, aLength);
  }

  FlushText();
  
  nsCOMPtr<nsIContent> cdata;
  nsresult rv = NS_NewXMLCDATASection(getter_AddRefs(cdata), mNodeInfoManager);
  if (cdata) {
    cdata->SetText(aData, aLength, PR_FALSE);
    rv = AddContentAsLeaf(cdata);
    DidAddContent();
  }

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
  rv = NS_NewDOMDocumentType(getter_AddRefs(docType), mNodeInfoManager, nsnull,
                             name, nsnull, nsnull, aPublicId, aSystemId,
                             aSubset);
  if (NS_FAILED(rv) || !docType) {
    return rv;
  }

  if (aCatalogData && mCSSLoader && mDocument) {
    
    
    nsCOMPtr<nsIURI> uri(do_QueryInterface(aCatalogData));
    if (uri) {
      nsCOMPtr<nsICSSStyleSheet> sheet;
      mCSSLoader->LoadSheetSync(uri, PR_TRUE, PR_TRUE, getter_AddRefs(sheet));
      
#ifdef NS_DEBUG
      nsCAutoString uriStr;
      uri->GetSpec(uriStr);
      printf("Loading catalog stylesheet: %s ... %s\n", uriStr.get(), sheet.get() ? "Done" : "Failed");
#endif
      if (sheet) {
        mDocument->BeginUpdate(UPDATE_STYLE);
        mDocument->AddCatalogStyleSheet(sheet);
        mDocument->EndUpdate(UPDATE_STYLE);
      }
    }
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(docType);
  NS_ASSERTION(content, "doctype isn't content?");

  rv = mDocument->AppendChildTo(content, PR_FALSE);
  DidAddContent();
  return NS_SUCCEEDED(rv) ? DidProcessATokenImpl() : rv;
}

NS_IMETHODIMP
nsXMLContentSink::HandleCharacterData(const PRUnichar *aData, 
                                      PRUint32 aLength)
{
  return HandleCharacterData(aData, aLength, PR_TRUE);
}

nsresult
nsXMLContentSink::HandleCharacterData(const PRUnichar *aData, PRUint32 aLength,
                                      PRBool aInterruptable)
{
  nsresult rv = NS_OK;
  if (aData && mState != eXMLContentSinkState_InProlog &&
      mState != eXMLContentSinkState_InEpilog) {
    rv = AddText(aData, aLength);
  }
  return aInterruptable && NS_SUCCEEDED(rv) ? DidProcessATokenImpl() : rv;
}

NS_IMETHODIMP
nsXMLContentSink::HandleProcessingInstruction(const PRUnichar *aTarget, 
                                              const PRUnichar *aData)
{
  FlushText();

  const nsDependentString target(aTarget);
  const nsDependentString data(aData);

  nsCOMPtr<nsIContent> node;

  nsresult rv = NS_NewXMLProcessingInstruction(getter_AddRefs(node),
                                               mNodeInfoManager, target, data);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(node));
  if (ssle) {
    ssle->InitStyleLinkElement(PR_FALSE);
    ssle->SetEnableUpdates(PR_FALSE);
    mPrettyPrintXML = PR_FALSE;
  }

  rv = AddContentAsLeaf(node);
  NS_ENSURE_SUCCESS(rv, rv);
  DidAddContent();

  if (ssle) {
    
    
    ssle->SetEnableUpdates(PR_TRUE);
    PRBool willNotify;
    PRBool isAlternate;
    rv = ssle->UpdateStyleSheet(this, &willNotify, &isAlternate);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (willNotify) {
      
      if (!isAlternate) {
        ++mPendingSheetCount;
        mScriptLoader->AddExecuteBlocker();
      }

      return NS_OK;
    }
  }

  
  nsAutoString type;
  nsParserUtils::GetQuotedAttributeValue(data, nsGkAtoms::type, type);

  if (mState != eXMLContentSinkState_InProlog ||
      !target.EqualsLiteral("xml-stylesheet") ||
      type.IsEmpty()                          ||
      type.LowerCaseEqualsLiteral("text/css")) {
    return DidProcessATokenImpl();
  }

  nsAutoString href, title, media;
  PRBool isAlternate = PR_FALSE;

  
  if (!ParsePIData(data, href, title, media, isAlternate)) {
      return DidProcessATokenImpl();
  }

  rv = ProcessStyleLink(node, href, isAlternate, title, type, media);
  return NS_SUCCEEDED(rv) ? DidProcessATokenImpl() : rv;
}


PRBool
nsXMLContentSink::ParsePIData(const nsString &aData, nsString &aHref,
                              nsString &aTitle, nsString &aMedia,
                              PRBool &aIsAlternate)
{
  
  if (!nsParserUtils::GetQuotedAttributeValue(aData, nsGkAtoms::href, aHref)) {
    return PR_FALSE;
  }

  nsParserUtils::GetQuotedAttributeValue(aData, nsGkAtoms::title, aTitle);

  nsParserUtils::GetQuotedAttributeValue(aData, nsGkAtoms::media, aMedia);

  nsAutoString alternate;
  nsParserUtils::GetQuotedAttributeValue(aData, nsGkAtoms::alternate, alternate);

  aIsAlternate = alternate.EqualsLiteral("yes");

  return PR_TRUE;
}

NS_IMETHODIMP
nsXMLContentSink::HandleXMLDeclaration(const PRUnichar *aVersion,
                                       const PRUnichar *aEncoding,
                                       PRInt32 aStandalone)
{
  mDocument->SetXMLDeclaration(aVersion, aEncoding, aStandalone);

  return DidProcessATokenImpl();
}

NS_IMETHODIMP
nsXMLContentSink::ReportError(const PRUnichar* aErrorText, 
                              const PRUnichar* aSourceText,
                              nsIScriptError *aError,
                              PRBool *_retval)
{
  NS_PRECONDITION(aError && aSourceText && aErrorText, "Check arguments!!!");
  nsresult rv = NS_OK;

  
  *_retval = PR_TRUE;
  
  mPrettyPrintXML = PR_FALSE;

  mState = eXMLContentSinkState_InProlog;

  

  
  mDocument->RemoveObserver(this);
  mIsDocumentObserver = PR_FALSE;

  
  
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
  NS_IF_RELEASE(mDocElement); 

  
  
  
  mTextLength = 0;

  if (mXSLTProcessor) {
    
    mXSLTProcessor->CancelLoads();
    mXSLTProcessor = nsnull;
  }

  
  mContentStack.Clear();
  mNotifyLevel = 0;

  rv = HandleProcessingInstruction(NS_LITERAL_STRING("xml-stylesheet").get(),
                                   NS_LITERAL_STRING("href=\"chrome://global/locale/intl.css\" type=\"text/css\"").get());
  NS_ENSURE_SUCCESS(rv, rv);

  const PRUnichar* noAtts[] = { 0, 0 };

  NS_NAMED_LITERAL_STRING(errorNs,
                          "http://www.mozilla.org/newlayout/xml/parsererror.xml");

  nsAutoString parsererror(errorNs);
  parsererror.Append((PRUnichar)0xFFFF);
  parsererror.AppendLiteral("parsererror");
  
  rv = HandleStartElement(parsererror.get(), noAtts, 0, -1, (PRUint32)-1,
                          PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = HandleCharacterData(aErrorText, nsCRT::strlen(aErrorText), PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);  
  
  nsAutoString sourcetext(errorNs);
  sourcetext.Append((PRUnichar)0xFFFF);
  sourcetext.AppendLiteral("sourcetext");

  rv = HandleStartElement(sourcetext.get(), noAtts, 0, -1, (PRUint32)-1,
                          PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = HandleCharacterData(aSourceText, nsCRT::strlen(aSourceText), PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = HandleEndElement(sourcetext.get(), PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv); 
  
  rv = HandleEndElement(parsererror.get(), PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  FlushTags();

  return NS_OK;
}

nsresult
nsXMLContentSink::AddAttributes(const PRUnichar** aAtts,
                                nsIContent* aContent)
{
  
  nsCOMPtr<nsIAtom> prefix, localName;
  while (*aAtts) {
    PRInt32 nameSpaceID;
    nsContentUtils::SplitExpatName(aAtts[0], getter_AddRefs(prefix),
                                   getter_AddRefs(localName), &nameSpaceID);

    
    aContent->SetAttr(nameSpaceID, localName, prefix,
                      nsDependentString(aAtts[1]), PR_FALSE);
    aAtts += 2;
  }

  
  if (mDocShell && mAllowAutoXLinks) {
    nsresult rv = aContent->MaybeTriggerAutoLink(mDocShell);
    if (rv == NS_XML_AUTOLINK_REPLACE ||
        rv == NS_XML_AUTOLINK_UNDEFINED) {
      
      
      mParser->Terminate();
    }
  }

  return NS_OK;
}

#define NS_ACCUMULATION_BUFFER_SIZE 4096

nsresult
nsXMLContentSink::AddText(const PRUnichar* aText, 
                          PRInt32 aLength)
{
  
  if (0 == mTextSize) {
    mText = (PRUnichar *) PR_MALLOC(sizeof(PRUnichar) * NS_ACCUMULATION_BUFFER_SIZE);
    if (nsnull == mText) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mTextSize = NS_ACCUMULATION_BUFFER_SIZE;
  }

  
  PRInt32 offset = 0;
  while (0 != aLength) {
    PRInt32 amount = mTextSize - mTextLength;
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
        mText = (PRUnichar *) PR_REALLOC(mText, sizeof(PRUnichar) * mTextSize);
        if (nsnull == mText) {
          mTextSize = 0;

          return NS_ERROR_OUT_OF_MEMORY;
        }

        amount = aLength;
      }
    }
    if (amount > aLength) {
      amount = aLength;
    }
    memcpy(&mText[mTextLength], &aText[offset], sizeof(PRUnichar) * amount);
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
        FlushText(PR_FALSE);
      }
    }
    if (aType >= Flush_InterruptibleLayout) {
      
      
      MaybeStartLayout(PR_TRUE);
    }
  }
}










nsresult
nsXMLContentSink::FlushTags()
{
  mDeferredFlushTags = PR_FALSE;
  PRBool oldBeganUpdate = mBeganUpdate;
  PRUint32 oldUpdates = mUpdatesInNotification;

  mUpdatesInNotification = 0;
  ++mInNotification;
  {
    
    mozAutoDocUpdate updateBatch(mDocument, UPDATE_CONTENT_MODEL, PR_TRUE);
    mBeganUpdate = PR_TRUE;

    
    FlushText(PR_FALSE);

    
    
    

    PRInt32 stackPos;
    PRInt32 stackLen = mContentStack.Length();
    PRBool flushed = PR_FALSE;
    PRUint32 childCount;
    nsIContent* content;

    for (stackPos = 0; stackPos < stackLen; ++stackPos) {
      content = mContentStack[stackPos].mContent;
      childCount = content->GetChildCount();

      if (!flushed && (mContentStack[stackPos].mNumFlushed < childCount)) {
        NotifyAppend(content, mContentStack[stackPos].mNumFlushed);
        flushed = PR_TRUE;
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
  
  
  
  
  
  PRInt32 stackLen = mContentStack.Length();
  PRInt32 stackPos = stackLen - 1;
  while (stackPos >= 0) {
    StackNode & node = mContentStack[stackPos];
    node.mNumFlushed = node.mContent->GetChildCount();

    stackPos--;
  }
  mNotifyLevel = stackLen - 1;
}

PRBool
nsXMLContentSink::IsMonolithicContainer(nsINodeInfo* aNodeInfo)
{
  return ((aNodeInfo->NamespaceID() == kNameSpaceID_XHTML &&
          (aNodeInfo->NameAtom() == nsGkAtoms::tr ||
           aNodeInfo->NameAtom() == nsGkAtoms::select ||
           aNodeInfo->NameAtom() == nsGkAtoms::object ||
           aNodeInfo->NameAtom() == nsGkAtoms::applet))
#ifdef MOZ_MATHML
       || (aNodeInfo->NamespaceID() == kNameSpaceID_MathML &&
          (aNodeInfo->NameAtom() == nsGkAtoms::math))
#endif
          );
}
