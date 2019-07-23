







































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
#include "mozAutoDocUpdate.h"
#include "nsIScriptElement.h"

#define NS_HTML5_TREE_BUILDER_MAX_QUEUE_TIME 3000UL // milliseconds
#define NS_HTML5_TREE_BUILDER_DEFAULT_QUEUE_LENGTH 200
#define NS_HTML5_TREE_BUILDER_MIN_QUEUE_LENGTH 100
#define NS_HTML5_TREE_BUILDER_MAX_TIME_WITHOUT_FLUSH 5000 // milliseconds


jArray<PRUnichar,PRInt32> nsHtml5TreeBuilder::ISINDEX_PROMPT = jArray<PRUnichar,PRInt32>();
nsHtml5TreeBuilder::nsHtml5TreeBuilder(nsHtml5Parser* aParser)
  : parser(aParser)
  , scriptingEnabled(PR_FALSE)
  , fragment(PR_FALSE)
  , contextNode(nsnull)
  , formPointer(nsnull)
  , headPointer(nsnull)
  , mNeedsFlush(PR_FALSE)
  , mFlushTimer(do_CreateInstance("@mozilla.org/timer;1"))
  , mHasProcessedBase(PR_FALSE)
#ifdef DEBUG
  , mActive(PR_FALSE)
#endif
{
  MOZ_COUNT_CTOR(nsHtml5TreeBuilder);
}

nsHtml5TreeBuilder::~nsHtml5TreeBuilder()
{
  MOZ_COUNT_DTOR(nsHtml5TreeBuilder);
  NS_ASSERTION(!mActive, "nsHtml5TreeBuilder deleted without ever calling end() on it!");
  mOpQueue.Clear();
  if (mFlushTimer) {
    mFlushTimer->Cancel(); 
  }
}

nsIContent*
nsHtml5TreeBuilder::createElement(PRInt32 aNamespace, nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes)
{
  nsIContent* newContent;
  nsCOMPtr<nsINodeInfo> nodeInfo = parser->GetNodeInfoManager()->GetNodeInfo(aName, nsnull, aNamespace);
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
  
  NS_ASSERTION(clone->IsNodeOfType(nsINode::eCONTENT), "Cloning an element didn't yield a content node.");
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
  NS_NewTextNode(getter_AddRefs(text), parser->GetNodeInfoManager());
  
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
  NS_NewTextNode(getter_AddRefs(text), parser->GetNodeInfoManager());
  
  text->SetText(aBuffer + aStart, aLength, PR_FALSE);
  
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(text, aParent);
}

void
nsHtml5TreeBuilder::appendComment(nsIContent* aParent, PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsCOMPtr<nsIContent> comment;
  NS_NewCommentNode(getter_AddRefs(comment), parser->GetNodeInfoManager());
  
  comment->SetText(aBuffer + aStart, aLength, PR_FALSE);
  
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(comment, aParent);
}

void
nsHtml5TreeBuilder::appendCommentToDocument(PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsCOMPtr<nsIContent> comment;
  NS_NewCommentNode(getter_AddRefs(comment), parser->GetNodeInfoManager());
  
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
nsHtml5TreeBuilder::markMalformedIfScript(nsIContent* elt)
{
  nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(elt);
  if (sele) {
    
    sele->SetIsMalformed();
  }
}

void
nsHtml5TreeBuilder::start(PRBool fragment)
{
  
  if (fragment) {
    mHasProcessedBase = PR_TRUE;
  } else {
    mHasProcessedBase = PR_FALSE;
    parser->WillBuildModelImpl();
    parser->GetDocument()->BeginLoad(); 
  }
  mNeedsFlush = PR_FALSE;
#ifdef DEBUG
  mActive = PR_TRUE;
#endif
}

void
nsHtml5TreeBuilder::end()
{
  mFlushTimer->Cancel();
#ifdef DEBUG
  mActive = PR_FALSE;
#endif
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
  NS_NewDOMDocumentType(getter_AddRefs(docType),
                        parser->GetNodeInfoManager(),
                        nsnull,
                        aName,
                        nsnull,
                        nsnull,
                        *aPublicId,
                        *aSystemId,
                        voidString);
  NS_ASSERTION(docType, "Doctype creation failed.");
  nsCOMPtr<nsIContent> content = do_QueryInterface(docType);
  NS_ASSERTION(content, "doctype isn't content?");
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpAppendToDocument, content);
  
  
}

void
nsHtml5TreeBuilder::elementPushed(PRInt32 aNamespace, nsIAtom* aName, nsIContent* aElement)
{
  NS_ASSERTION(aNamespace == kNameSpaceID_XHTML || aNamespace == kNameSpaceID_SVG || aNamespace == kNameSpaceID_MathML, "Element isn't HTML, SVG or MathML!");
  NS_ASSERTION(aName, "Element doesn't have local name!");
  NS_ASSERTION(aElement, "No element!");
  
  if (aNamespace == kNameSpaceID_XHTML) {
    if (aName == nsHtml5Atoms::body) {
      nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
      
      treeOp->Init(eTreeOpStartLayout, nsnull);
    } else if (aName == nsHtml5Atoms::html) {
      nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
      
      treeOp->Init(eTreeOpProcessOfflineManifest, aElement);
      return;
    }
  }
  #if 0
    else {
    nsIDocShell* docShell = parser->GetDocShell();
    if (docShell) {
      nsresult rv = aElement->MaybeTriggerAutoLink(docShell);
      if (rv == NS_XML_AUTOLINK_REPLACE ||
          rv == NS_XML_AUTOLINK_UNDEFINED) {
        
        
        parser->Terminate();
      }
    }
  }
  #endif
  MaybeSuspend();
}

void
nsHtml5TreeBuilder::elementPopped(PRInt32 aNamespace, nsIAtom* aName, nsIContent* aElement)
{
  NS_ASSERTION(aNamespace == kNameSpaceID_XHTML || aNamespace == kNameSpaceID_SVG || aNamespace == kNameSpaceID_MathML, "Element isn't HTML, SVG or MathML!");
  NS_ASSERTION(aName, "Element doesn't have local name!");
  NS_ASSERTION(aElement, "No element!");
  MaybeSuspend();
  if (aNamespace == kNameSpaceID_MathML) {
    return;
  }
  
  if (aName == nsHtml5Atoms::script) {
    requestSuspension();
    parser->SetScriptElement(aElement);
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
      nsCOMPtr<nsIPresShell> shell = parser->GetDocument()->GetPrimaryShell();
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
  if (aName == nsHtml5Atoms::input ||
      aName == nsHtml5Atoms::button) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpDoneCreatingElement, aElement);
    return;
  }
  if (aName == nsHtml5Atoms::base) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpProcessBase, aElement);
    return;
  }
  if (aName == nsHtml5Atoms::meta) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpProcessMeta, aElement);
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

static void
TimerCallbackFunc(nsITimer* aTimer, void* aClosure)
{
  (static_cast<nsHtml5TreeBuilder*> (aClosure))->DeferredTimerFlush();
}

void
nsHtml5TreeBuilder::Flush()
{
  mNeedsFlush = PR_FALSE;
  MOZ_AUTO_DOC_UPDATE(parser->GetDocument(), UPDATE_CONTENT_MODEL, PR_TRUE);
  PRIntervalTime flushStart = 0;
  PRUint32 opQueueLength = mOpQueue.Length();
  if (opQueueLength > NS_HTML5_TREE_BUILDER_MIN_QUEUE_LENGTH) { 
    flushStart = PR_IntervalNow();
  }
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
  if (flushStart) {
    PRUint32 delta = PR_IntervalToMilliseconds(PR_IntervalNow() - flushStart);
    sTreeOpQueueMaxLength = delta ?
      (PRUint32)((NS_HTML5_TREE_BUILDER_MAX_QUEUE_TIME * (PRUint64)opQueueLength) / delta) :
      0;
    if (sTreeOpQueueMaxLength < NS_HTML5_TREE_BUILDER_MIN_QUEUE_LENGTH) {
      sTreeOpQueueMaxLength = NS_HTML5_TREE_BUILDER_MIN_QUEUE_LENGTH;
    }
#ifdef DEBUG_hsivonen
    printf("QUEUE MAX LENGTH: %d\n", sTreeOpQueueMaxLength);
#endif
  }
  mFlushTimer->InitWithFuncCallback(TimerCallbackFunc, static_cast<void*> (this), NS_HTML5_TREE_BUILDER_MAX_TIME_WITHOUT_FLUSH, nsITimer::TYPE_ONE_SHOT);
}

void
nsHtml5TreeBuilder::DoUnlink()
{
  nsHtml5TreeBuilder* tmp = this;
  if (mFlushTimer) {
    mFlushTimer->Cancel();
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mFlushTimer);
  NS_IF_RELEASE(contextNode);
  NS_IF_RELEASE(formPointer);
  NS_IF_RELEASE(headPointer);
  while (currentPtr > -1) {
    stack[currentPtr]->release();
    currentPtr--;
  }
  while (listPtr > -1) {
    if (listOfActiveFormattingElements[listPtr]) {
      listOfActiveFormattingElements[listPtr]->release();
    }
    listPtr--;
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSTARRAY(mOpQueue);
}

void
nsHtml5TreeBuilder::DoTraverse(nsCycleCollectionTraversalCallback &cb)
{
  nsHtml5TreeBuilder* tmp = this;
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mFlushTimer);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(contextNode);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(formPointer);
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_RAWPTR(headPointer);
  if (stack) {
    for (PRInt32 i = 0; i <= currentPtr; i++) {
#ifdef DEBUG_CC
      NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "stack[i]");
#endif
      cb.NoteNativeChild(stack[i], &NS_CYCLE_COLLECTION_NAME(nsHtml5StackNode));
    }
  }
  if (listOfActiveFormattingElements) {
    for (PRInt32 i = 0; i <= listPtr; i++) {
      if (listOfActiveFormattingElements[i]) {
#ifdef DEBUG_CC
        NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "listOfActiveFormattingElements[i]");
#endif
        cb.NoteNativeChild(listOfActiveFormattingElements[i], &NS_CYCLE_COLLECTION_NAME(nsHtml5StackNode));
      }
    }
  }
  const nsHtml5TreeOperation* start = mOpQueue.Elements();
  const nsHtml5TreeOperation* end = start + mOpQueue.Length();
  for (nsHtml5TreeOperation* iter = (nsHtml5TreeOperation*)start; iter < end; ++iter) {
#ifdef DEBUG_CC
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mOpQueue[i]");
#endif
    iter->DoTraverse(cb);
  }
}

#ifdef DEBUG_hsivonen
PRUint32 nsHtml5TreeBuilder::sInsertionBatchMaxLength = 0;
PRUint32 nsHtml5TreeBuilder::sAppendBatchMaxSize = 0;
PRUint32 nsHtml5TreeBuilder::sAppendBatchSlotsExamined = 0;
PRUint32 nsHtml5TreeBuilder::sAppendBatchExaminations = 0;
#endif
PRUint32 nsHtml5TreeBuilder::sTreeOpQueueMaxLength = NS_HTML5_TREE_BUILDER_DEFAULT_QUEUE_LENGTH;
