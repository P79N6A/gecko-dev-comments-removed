







































#include "nsContentErrors.h"
#include "nsContentCreatorFunctions.h"
#include "nsIDOMDocumentType.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsEvent.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsContentUtils.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIFormControl.h"
#include "nsNodeUtils.h"
#include "nsIStyleSheetLinkingElement.h"
#include "nsTraceRefcnt.h"


jArray<PRUnichar,PRInt32> nsHtml5TreeBuilder::ISINDEX_PROMPT = jArray<PRUnichar,PRInt32>();

nsHtml5TreeBuilder::nsHtml5TreeBuilder(nsHtml5Parser* aParser)
  : MARKER(new nsHtml5StackNode(0, nsHtml5ElementName::NULL_ELEMENT_NAME, nsnull)),
    documentModeHandler(aParser),
    fragment(PR_FALSE),
    formPointer(nsnull),
    headPointer(nsnull),
    mFlushing(PR_FALSE),
    mParser(aParser)
{
  MOZ_COUNT_CTOR(nsHtml5TreeBuilder);
}

nsHtml5TreeBuilder::~nsHtml5TreeBuilder()
{
  MOZ_COUNT_DTOR(nsHtml5TreeBuilder);
  delete MARKER;
  mOpQueue.Clear();
}

nsIContent*
nsHtml5TreeBuilder::createElement(PRInt32 aNamespace, nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes)
{
  
  nsIContent* newContent;
  nsCOMPtr<nsINodeInfo> nodeInfo = mParser->GetNodeInfoManager()->GetNodeInfo(aName, nsnull, aNamespace);
  NS_ASSERTION(nodeInfo, "Got null nodeinfo.");
  NS_NewElement(&newContent, nodeInfo->NamespaceID(), nodeInfo, PR_TRUE);
  NS_ASSERTION(newContent, "Element creation created null pointer.");
  PRInt32 len = aAttributes->getLength();
  for (PRInt32 i = 0; i < len; ++i) {
    newContent->SetAttr(aAttributes->getURI(i), aAttributes->getLocalName(i), aAttributes->getPrefix(i), *(aAttributes->getValue(i)), PR_FALSE);
    
  }
  
  
  if (aNamespace != kNameSpaceID_MathML && (aName == nsHtml5Atoms::style || (aNamespace == kNameSpaceID_XHTML && aName == nsHtml5Atoms::link))) {
    nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(newContent));
    if (ssle) {
      ssle->InitStyleLinkElement(PR_FALSE);
      ssle->SetEnableUpdates(PR_FALSE);
#if 0
      if (!aNodeInfo->Equals(nsGkAtoms::link, kNameSpaceID_XHTML)) {
        ssle->SetLineNumber(aLineNumber);
      }
#endif
    }
  } 
  
  return newContent;
}

nsIContent*
nsHtml5TreeBuilder::createElement(PRInt32 aNamespace, nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes, nsIContent* aFormElement)
{
  nsIContent* content = createElement(aNamespace, aName, aAttributes);
  if (aFormElement) {
    nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(content));
    NS_ASSERTION(formControl, "Form-associated element did not implement nsIFormControl.");
    nsCOMPtr<nsIDOMHTMLFormElement> formElement(do_QueryInterface(aFormElement));
    NS_ASSERTION(formElement, "The form element doesn't implement nsIDOMHTMLFormElement.");
    if (formControl) { 
      formControl->SetForm(formElement);
    }
  }
  return content; 
}

nsIContent*
nsHtml5TreeBuilder::createHtmlElementSetAsRoot(nsHtml5HtmlAttributes* aAttributes)
{
  nsIContent* content = createElement(kNameSpaceID_XHTML, nsHtml5Atoms::html, aAttributes);
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpAppendToDocument, content);
  return content;
}

void
nsHtml5TreeBuilder::detachFromParent(nsIContent* aElement)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpDetach, aElement);
}

nsIContent*
nsHtml5TreeBuilder::shallowClone(nsIContent* aElement)
{
  nsINode* clone;
  aElement->Clone(aElement->NodeInfo(), &clone);
  
  return static_cast<nsIContent*>(clone);
}

void
nsHtml5TreeBuilder::appendElement(nsIContent* aChild, nsIContent* aParent)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(aChild, aParent);
}

void
nsHtml5TreeBuilder::appendChildrenToNewParent(nsIContent* aOldParent, nsIContent* aNewParent)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpAppendChildrenToNewParent, aOldParent, aNewParent);
}

void
nsHtml5TreeBuilder::insertFosterParentedCharacters(PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength, nsIContent* aTable, nsIContent* aStackParent)
{
  nsCOMPtr<nsIContent> text;
  NS_NewTextNode(getter_AddRefs(text), mParser->GetNodeInfoManager());
  
  text->SetText(aBuffer + aStart, aLength, PR_FALSE);
  
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpFosterParent, text, aStackParent, aTable);
}

void
nsHtml5TreeBuilder::insertFosterParentedChild(nsIContent* aChild, nsIContent* aTable, nsIContent* aStackParent)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpFosterParent, aChild, aStackParent, aTable);
}

void
nsHtml5TreeBuilder::appendCharacters(nsIContent* aParent, PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsCOMPtr<nsIContent> text;
  NS_NewTextNode(getter_AddRefs(text), mParser->GetNodeInfoManager());
  
  text->SetText(aBuffer + aStart, aLength, PR_FALSE);
  
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(text, aParent);
}

void
nsHtml5TreeBuilder::appendComment(nsIContent* aParent, PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsCOMPtr<nsIContent> comment;
  NS_NewCommentNode(getter_AddRefs(comment), mParser->GetNodeInfoManager());
  
  comment->SetText(aBuffer + aStart, aLength, PR_FALSE);
  
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(comment, aParent);
}

void
nsHtml5TreeBuilder::appendCommentToDocument(PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsIDocument* doc = mParser->GetDocument();
  nsCOMPtr<nsIContent> comment;
  NS_NewCommentNode(getter_AddRefs(comment), mParser->GetNodeInfoManager());
  
  comment->SetText(aBuffer + aStart, aLength, PR_FALSE);
  
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpAppendToDocument, comment);
}

void
nsHtml5TreeBuilder::addAttributesToElement(nsIContent* aElement, nsHtml5HtmlAttributes* aAttributes)
{
  nsIContent* holder = createElement(kNameSpaceID_XHTML, nsHtml5Atoms::div, aAttributes);
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpAddAttributes, holder, aElement);
}

void
nsHtml5TreeBuilder::start(PRBool fragment)
{
  if (fragment) {
    mHasProcessedBase = PR_TRUE;  
  } else {
    mHasProcessedBase = PR_FALSE;
    mParser->WillBuildModelImpl();
    mParser->GetDocument()->BeginLoad(); 
  }
  mFlushing = PR_FALSE;
}

void
nsHtml5TreeBuilder::end()
{
  Flush();
#ifdef DEBUG_hsivonen
  printf("MAX INSERTION BATCH LEN: %d\n", sInsertionBatchMaxLength);
  printf("MAX NOTIFICATION BATCH LEN: %d\n", sAppendBatchMaxSize);
  if (sAppendBatchExaminations != 0) {
    printf("AVERAGE SLOTS EXAMINED: %d\n", sAppendBatchSlotsExamined / sAppendBatchExaminations);
  }
#endif
}

void
nsHtml5TreeBuilder::appendDoctypeToDocument(nsIAtom* aName, nsString* aPublicId, nsString* aSystemId)
{
  
  
  
  nsCOMPtr<nsIDOMDocumentType> docType;
  nsAutoString voidString;
  voidString.SetIsVoid(PR_TRUE);
  NS_NewDOMDocumentType(getter_AddRefs(docType), mParser->GetNodeInfoManager(), nsnull,
                             aName, nsnull, nsnull, *aPublicId, *aSystemId,
                             voidString);




  nsCOMPtr<nsIContent> content = do_QueryInterface(docType);
  NS_ASSERTION(content, "doctype isn't content?");

  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpAppendToDocument, content);

  
  
}

void
nsHtml5TreeBuilder::elementPushed(PRInt32 aNamespace, nsIAtom* aName, nsIContent* aElement)
{
  NS_ASSERTION((aNamespace == kNameSpaceID_XHTML || aNamespace == kNameSpaceID_SVG || aNamespace == kNameSpaceID_MathML), "Element isn't HTML, SVG or MathML!");
  NS_ASSERTION(aName, "Element doesn't have local name!");
  NS_ASSERTION(aElement, "No element!");
  
  if (aNamespace == kNameSpaceID_XHTML) {
    if (aName == nsHtml5Atoms::body) {
      nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
      
      treeOp->Init(eTreeOpStartLayout, nsnull);
    }
  }
  #if 0
    else {
    nsIDocShell* docShell = mParser->GetDocShell();
    if (docShell) {
      nsresult rv = aElement->MaybeTriggerAutoLink(docShell);
      if (rv == NS_XML_AUTOLINK_REPLACE ||
          rv == NS_XML_AUTOLINK_UNDEFINED) {
        
        
        mParser->Terminate();
      }
    }
  }
  #endif
  MaybeFlushAndMaybeSuspend();  
}

void
nsHtml5TreeBuilder::elementPopped(PRInt32 aNamespace, nsIAtom* aName, nsIContent* aElement)
{
  NS_ASSERTION((aNamespace == kNameSpaceID_XHTML || aNamespace == kNameSpaceID_SVG || aNamespace == kNameSpaceID_MathML), "Element isn't HTML, SVG or MathML!");
  NS_ASSERTION(aName, "Element doesn't have local name!");
  NS_ASSERTION(aElement, "No element!");

  MaybeFlushAndMaybeSuspend();  
  
  if (aNamespace == kNameSpaceID_MathML) {
    return;
  }  
  
  
  if (aName == nsHtml5Atoms::script) {

    requestSuspension();
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpScriptEnd, aElement);
    Flush();
    return;
  }
  
  if (aName == nsHtml5Atoms::title) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpDoneAddingChildren, aElement);
    return;
  }
  
  if (aName == nsHtml5Atoms::style || (aNamespace == kNameSpaceID_XHTML && aName == nsHtml5Atoms::link)) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpUpdateStyleSheet, aElement);
    return;
  }


  if (aNamespace == kNameSpaceID_SVG) {
#if 0
    if (aElement->HasAttr(kNameSpaceID_None, nsHtml5Atoms::onload)) {
      Flush();

      nsEvent event(PR_TRUE, NS_SVG_LOAD);
      event.eventStructType = NS_SVG_EVENT;
      event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;

      
      
      
      
      nsRefPtr<nsPresContext> ctx;
      nsCOMPtr<nsIPresShell> shell = mParser->GetDocument()->GetPrimaryShell();
      if (shell) {
        ctx = shell->GetPresContext();
      }
      nsEventDispatcher::Dispatch(aElement, ctx, &event);
    }
#endif
    return;
  }  
  

  
  
  if (aName == nsHtml5Atoms::select ||
        aName == nsHtml5Atoms::textarea ||
#ifdef MOZ_MEDIA
        aName == nsHtml5Atoms::video ||
        aName == nsHtml5Atoms::audio ||
#endif
        aName == nsHtml5Atoms::object ||
        aName == nsHtml5Atoms::applet) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpDoneAddingChildren, aElement);
    return;
  }
  
  if (aName == nsHtml5Atoms::base) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpProcessBase, aElement);
    return;
  }
  
  if (aName == nsHtml5Atoms::meta) {
    

    
    return;
  }
  return;
}

void
nsHtml5TreeBuilder::accumulateCharacters(PRUnichar* aBuf, PRInt32 aStart, PRInt32 aLength)
{
  PRInt32 newFillLen = charBufferLen + aLength;
  if (newFillLen > charBuffer.length) {
    PRInt32 newAllocLength = newFillLen + (newFillLen >> 1);
    jArray<PRUnichar,PRInt32> newBuf(newAllocLength);
    memcpy(newBuf, charBuffer, sizeof(PRUnichar) * charBufferLen);
    charBuffer.release();
    charBuffer = newBuf;
  }
  memcpy(charBuffer + charBufferLen, aBuf + aStart, sizeof(PRUnichar) * aLength);
  charBufferLen = newFillLen;
}

void
nsHtml5TreeBuilder::MaybeFlushAndMaybeSuspend()
{
  if (mParser->DidProcessATokenImpl() == NS_ERROR_HTMLPARSER_INTERRUPTED) {
    mParser->Suspend();
    requestSuspension();
    Flush();
  } else if (mParser->IsTimeToNotify()) {
    Flush();
  }
}

void
nsHtml5TreeBuilder::Flush()
{
  if (!mFlushing) {
    mFlushing = PR_TRUE;
    PRUint32 opQueueLength = mOpQueue.Length();
    mElementsSeenInThisAppendBatch.SetCapacity(opQueueLength * 2);
    
    const nsHtml5TreeOperation* start = mOpQueue.Elements();
    const nsHtml5TreeOperation* end = start + opQueueLength;
    for (nsHtml5TreeOperation* iter = (nsHtml5TreeOperation*)start; iter < end; ++iter) {
      iter->Perform(this);
    }
    FlushPendingAppendNotifications();
#ifdef DEBUG_hsivonen
    if (mOpQueue.Length() > sInsertionBatchMaxLength) {
      sInsertionBatchMaxLength = opQueueLength;
    }
#endif
    mOpQueue.Clear();
    mFlushing = PR_FALSE;
  }
}

#ifdef DEBUG_hsivonen
PRUint32 nsHtml5TreeBuilder::sInsertionBatchMaxLength = 0;
PRUint32 nsHtml5TreeBuilder::sAppendBatchMaxSize = 0;
PRUint32 nsHtml5TreeBuilder::sAppendBatchSlotsExamined = 0;
PRUint32 nsHtml5TreeBuilder::sAppendBatchExaminations = 0;
#endif

