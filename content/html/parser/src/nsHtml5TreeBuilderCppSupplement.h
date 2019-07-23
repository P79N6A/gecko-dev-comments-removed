







































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
    fragment(PR_FALSE),
    documentModeHandler(aParser),
    mParser(aParser),
    formPointer(nsnull),
    headPointer(nsnull)
{
  MOZ_COUNT_CTOR(nsHtml5TreeBuilder);
}

nsHtml5TreeBuilder::~nsHtml5TreeBuilder()
{
  MOZ_COUNT_DTOR(nsHtml5TreeBuilder);
  delete MARKER;
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
  nsIDocument* doc = mParser->GetDocument();
  PRUint32 childCount = doc->GetChildCount();
  doc->AppendChildTo(content, PR_FALSE);
  
  nsNodeUtils::ContentInserted(doc, content, childCount);
  return content;
}

void
nsHtml5TreeBuilder::detachFromParent(nsIContent* aElement)
{
  Flush();
  nsIContent* parent = aElement->GetParent();
  if (parent) {
    PRUint32 pos = parent->IndexOf(aElement);
    NS_ASSERTION((pos >= 0), "Element not found as child of its parent");
    parent->RemoveChildAt(pos, PR_FALSE);
    
    nsNodeUtils::ContentRemoved(parent, aElement, pos);
  }
}

PRBool
nsHtml5TreeBuilder::hasChildren(nsIContent* aElement)
{
  Flush();
  return !!(aElement->GetChildCount());
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
  PRUint32 childCount = aParent->GetChildCount();
  aParent->AppendChildTo(aChild, PR_FALSE);
  
  mParser->NotifyAppend(aParent, childCount);
}

void
nsHtml5TreeBuilder::appendChildrenToNewParent(nsIContent* aOldParent, nsIContent* aNewParent)
{
  Flush();
  while (aOldParent->GetChildCount()) {
    nsCOMPtr<nsIContent> child = aOldParent->GetChildAt(0);
    aOldParent->RemoveChildAt(0, PR_FALSE);
    nsNodeUtils::ContentRemoved(aOldParent, child, 0);
    PRUint32 childCount = aNewParent->GetChildCount();
    aNewParent->AppendChildTo(child, PR_FALSE);
    mParser->NotifyAppend(aNewParent, childCount);
  }
}

nsIContent*
nsHtml5TreeBuilder::parentElementFor(nsIContent* aElement)
{
  Flush();
  return aElement->GetParent();
}

void
nsHtml5TreeBuilder::insertBefore(nsIContent* aNewChild, nsIContent* aReferenceSibling, nsIContent* aParent)
{
  PRUint32 pos = aParent->IndexOf(aReferenceSibling);
  aParent->InsertChildAt(aNewChild, pos, PR_FALSE);
  
  nsNodeUtils::ContentInserted(aParent, aNewChild, pos);
}

void
nsHtml5TreeBuilder::insertCharactersBefore(PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength, nsIContent* aReferenceSibling, nsIContent* aParent)
{
  
  nsCOMPtr<nsIContent> text;
  NS_NewTextNode(getter_AddRefs(text), mParser->GetNodeInfoManager());
  
  text->SetText(aBuffer + aStart, aLength, PR_FALSE);
  
  PRUint32 pos = aParent->IndexOf(aReferenceSibling);
  aParent->InsertChildAt(text, pos, PR_FALSE);
  
  nsNodeUtils::ContentInserted(aParent, text, pos);
}

void
nsHtml5TreeBuilder::appendCharacters(nsIContent* aParent, PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsCOMPtr<nsIContent> text;
  NS_NewTextNode(getter_AddRefs(text), mParser->GetNodeInfoManager());
  
  text->SetText(aBuffer + aStart, aLength, PR_FALSE);
  
  PRUint32 childCount = aParent->GetChildCount();
  aParent->AppendChildTo(text, PR_FALSE);  
  
  mParser->NotifyAppend(aParent, childCount);
}

void
nsHtml5TreeBuilder::appendComment(nsIContent* aParent, PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsCOMPtr<nsIContent> comment;
  NS_NewCommentNode(getter_AddRefs(comment), mParser->GetNodeInfoManager());
  
  comment->SetText(aBuffer + aStart, aLength, PR_FALSE);
  
  PRUint32 childCount = aParent->GetChildCount();
  aParent->AppendChildTo(comment, PR_FALSE);  
  
  mParser->NotifyAppend(aParent, childCount);
}

void
nsHtml5TreeBuilder::appendCommentToDocument(PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  nsIDocument* doc = mParser->GetDocument();
  nsCOMPtr<nsIContent> comment;
  NS_NewCommentNode(getter_AddRefs(comment), mParser->GetNodeInfoManager());
  
  comment->SetText(aBuffer + aStart, aLength, PR_FALSE);
  
  PRUint32 childCount = doc->GetChildCount();
  doc->AppendChildTo(comment, PR_FALSE);
  
  nsNodeUtils::ContentInserted(doc, comment, childCount);
}

void
nsHtml5TreeBuilder::addAttributesToElement(nsIContent* aElement, nsHtml5HtmlAttributes* aAttributes)
{
  PRInt32 len = aAttributes->getLength();
  for (PRInt32 i = 0; i < len; ++i) {
    nsIAtom* localName = aAttributes->getLocalName(i);
    PRInt32 nsuri = aAttributes->getURI(i);
    if (!aElement->HasAttr(nsuri, localName)) {
      aElement->SetAttr(nsuri, localName, aAttributes->getPrefix(i), *(aAttributes->getValue(i)), PR_TRUE);
      
    }
  }  
}

void
nsHtml5TreeBuilder::startCoalescing()
{
  mCharBufferFillLength = 0;
  mCharBufferAllocLength = 1024;
  mCharBuffer = new PRUnichar[mCharBufferAllocLength];
}

void
nsHtml5TreeBuilder::endCoalescing()
{
  delete[] mCharBuffer;
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
}

void
nsHtml5TreeBuilder::end()
{
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

  mParser->GetDocument()->AppendChildTo(content, PR_TRUE);
  

  
  
}

void
nsHtml5TreeBuilder::elementPushed(PRInt32 aNamespace, nsIAtom* aName, nsIContent* aElement)
{
  NS_ASSERTION((aNamespace == kNameSpaceID_XHTML || aNamespace == kNameSpaceID_SVG || aNamespace == kNameSpaceID_MathML), "Element isn't HTML, SVG or MathML!");
  NS_ASSERTION(aName, "Element doesn't have local name!");
  NS_ASSERTION(aElement, "No element!");
  
  if (aNamespace == kNameSpaceID_XHTML) {
    if (aName == nsHtml5Atoms::body) {
      mParser->StartLayout(PR_FALSE);
    }
  } else {
    nsIDocShell* docShell = mParser->GetDocShell();
    if (docShell) {
      nsresult rv = aElement->MaybeTriggerAutoLink(docShell);
      if (rv == NS_XML_AUTOLINK_REPLACE ||
          rv == NS_XML_AUTOLINK_UNDEFINED) {
        
        
        mParser->Terminate();
      }
    }
  }
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
    mParser->SetScriptElement(aElement);
    return;
  }
  
  if (aName == nsHtml5Atoms::title) {
    Flush();
    aElement->DoneAddingChildren(PR_TRUE);
    return;
  }
  
  if (aName == nsHtml5Atoms::style || (aNamespace == kNameSpaceID_XHTML && aName == nsHtml5Atoms::link)) {
    mParser->UpdateStyleSheet(aElement);
    return;
  }


  if (aNamespace == kNameSpaceID_SVG) {
#ifdef MOZ_SVG
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
    Flush();
    aElement->DoneAddingChildren(PR_TRUE);
    return;
  }
  
  if (aName == nsHtml5Atoms::base && !mHasProcessedBase) {
    
    mParser->ProcessBASETag(aElement);
    
    mHasProcessedBase = PR_TRUE;
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
  PRInt32 newFillLen = mCharBufferFillLength + aLength;
  if (newFillLen > mCharBufferAllocLength) {
    PRInt32 newAllocLength = newFillLen + (newFillLen >> 1);
    PRUnichar* newBuf = new PRUnichar[newAllocLength];
    memcpy(newBuf, mCharBuffer, sizeof(PRUnichar) * mCharBufferFillLength);
    delete[] mCharBuffer;
    mCharBuffer = newBuf;
    mCharBufferAllocLength = newAllocLength;
  }
  memcpy(mCharBuffer + mCharBufferFillLength, aBuf + aStart, sizeof(PRUnichar) * aLength);
  mCharBufferFillLength = newFillLen;
}

void
nsHtml5TreeBuilder::flushCharacters()
{
  if (mCharBufferFillLength > 0) {
    appendCharacters(currentNode(), mCharBuffer, 0,
            mCharBufferFillLength);
    mCharBufferFillLength = 0;
  }
}

void
nsHtml5TreeBuilder::MaybeFlushAndMaybeSuspend()
{
  if (mParser->DidProcessATokenImpl() == NS_ERROR_HTMLPARSER_INTERRUPTED) {
    mParser->Suspend();
    requestSuspension();
  }
  if (mParser->IsTimeToNotify()) {
    Flush();
  }
}

void
nsHtml5TreeBuilder::Flush()
{

}
