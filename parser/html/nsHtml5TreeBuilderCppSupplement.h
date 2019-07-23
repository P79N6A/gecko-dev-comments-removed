







































#include "nsContentErrors.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsEvent.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsContentUtils.h"
#include "nsNodeUtils.h"


jArray<PRUnichar,PRInt32> nsHtml5TreeBuilder::ISINDEX_PROMPT = jArray<PRUnichar,PRInt32>();
nsHtml5TreeBuilder::nsHtml5TreeBuilder(nsHtml5TreeOpExecutor* aExec)
  : scriptingEnabled(PR_FALSE)
  , fragment(PR_FALSE)
  , contextNode(nsnull)
  , formPointer(nsnull)
  , headPointer(nsnull)
  , mExecutor(aExec)
  , mHandles(new nsIContent*[NS_HTML5_TREE_BUILDER_HANDLE_ARRAY_LENGTH])
  , mHandlesUsed(0)
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
}

nsIContent**
nsHtml5TreeBuilder::createElement(PRInt32 aNamespace, nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes)
{
  nsIContent** content = AllocateContentHandle();
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(aNamespace, aName, aAttributes, content);
  return content;
}

nsIContent**
nsHtml5TreeBuilder::createElement(PRInt32 aNamespace, nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes, nsIContent** aFormElement)
{
  nsIContent** content = createElement(aNamespace, aName, aAttributes);
  if (aFormElement) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpSetFormElement, content, aFormElement);
  }
  return content;
}

nsIContent**
nsHtml5TreeBuilder::createHtmlElementSetAsRoot(nsHtml5HtmlAttributes* aAttributes)
{
  nsIContent** content = createElement(kNameSpaceID_XHTML, nsHtml5Atoms::html, aAttributes);
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpAppendToDocument, content);
  return content;
}

void
nsHtml5TreeBuilder::detachFromParent(nsIContent** aElement)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpDetach, aElement);
}

void
nsHtml5TreeBuilder::appendElement(nsIContent** aChild, nsIContent** aParent)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(aChild, aParent);
}

void
nsHtml5TreeBuilder::appendChildrenToNewParent(nsIContent** aOldParent, nsIContent** aNewParent)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpAppendChildrenToNewParent, aOldParent, aNewParent);
}

void
nsHtml5TreeBuilder::insertFosterParentedCharacters(PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength, nsIContent** aTable, nsIContent** aStackParent)
{
  PRUnichar* bufferCopy = new PRUnichar[aLength];
  memcpy(bufferCopy, aBuffer, aLength * sizeof(PRUnichar));
  
  nsIContent** text = AllocateContentHandle();

  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpCreateTextNode, bufferCopy, aLength, text);

  treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpFosterParent, text, aStackParent, aTable);
}

void
nsHtml5TreeBuilder::insertFosterParentedChild(nsIContent** aChild, nsIContent** aTable, nsIContent** aStackParent)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpFosterParent, aChild, aStackParent, aTable);
}

void
nsHtml5TreeBuilder::appendCharacters(nsIContent** aParent, PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  PRUnichar* bufferCopy = new PRUnichar[aLength];
  memcpy(bufferCopy, aBuffer, aLength * sizeof(PRUnichar));
  
  nsIContent** text = AllocateContentHandle();

  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpCreateTextNode, bufferCopy, aLength, text);

  treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(text, aParent);
}

void
nsHtml5TreeBuilder::appendComment(nsIContent** aParent, PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  PRUnichar* bufferCopy = new PRUnichar[aLength];
  memcpy(bufferCopy, aBuffer, aLength * sizeof(PRUnichar));
  
  nsIContent** comment = AllocateContentHandle();

  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpCreateComment, bufferCopy, aLength, comment);

  treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(comment, aParent);
}

void
nsHtml5TreeBuilder::appendCommentToDocument(PRUnichar* aBuffer, PRInt32 aStart, PRInt32 aLength)
{
  PRUnichar* bufferCopy = new PRUnichar[aLength];
  memcpy(bufferCopy, aBuffer, aLength * sizeof(PRUnichar));
  
  nsIContent** comment = AllocateContentHandle();

  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpCreateComment, bufferCopy, aLength, comment);

  treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpAppendToDocument, comment);
}

void
nsHtml5TreeBuilder::addAttributesToElement(nsIContent** aElement, nsHtml5HtmlAttributes* aAttributes)
{
  if (aAttributes == nsHtml5HtmlAttributes::EMPTY_ATTRIBUTES) {
    return;
  }
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(aElement, aAttributes);
}

void
nsHtml5TreeBuilder::markMalformedIfScript(nsIContent** elt)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpMarkMalformedIfScript, elt);
}

void
nsHtml5TreeBuilder::start(PRBool fragment)
{
  
  if (!fragment) {
    




    mExecutor->WillBuildModel(eDTDMode_unknown);
  }
  mExecutor->Start();
#ifdef DEBUG
  mActive = PR_TRUE;
#endif
}

void
nsHtml5TreeBuilder::end()
{
  mExecutor->End();
  mOpQueue.Clear();
#ifdef DEBUG
  mActive = PR_FALSE;
#endif
}

void
nsHtml5TreeBuilder::appendDoctypeToDocument(nsIAtom* aName, nsString* aPublicId, nsString* aSystemId)
{
  nsIContent** content = AllocateContentHandle();

  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(aName, *aPublicId, *aSystemId, content);
  
  treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpAppendToDocument, content);
  
  
}

void
nsHtml5TreeBuilder::elementPushed(PRInt32 aNamespace, nsIAtom* aName, nsIContent** aElement)
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
  mExecutor->MaybeSuspend();
}

void
nsHtml5TreeBuilder::elementPopped(PRInt32 aNamespace, nsIAtom* aName, nsIContent** aElement)
{
  NS_ASSERTION(aNamespace == kNameSpaceID_XHTML || aNamespace == kNameSpaceID_SVG || aNamespace == kNameSpaceID_MathML, "Element isn't HTML, SVG or MathML!");
  NS_ASSERTION(aName, "Element doesn't have local name!");
  NS_ASSERTION(aElement, "No element!");
  mExecutor->MaybeSuspend();
  if (aNamespace == kNameSpaceID_MathML) {
    return;
  }
  
  if (aName == nsHtml5Atoms::script) {
    requestSuspension();
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpRunScript, aElement);
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

nsIContent**
nsHtml5TreeBuilder::AllocateContentHandle()
{
  if (mHandlesUsed == NS_HTML5_TREE_BUILDER_HANDLE_ARRAY_LENGTH) {
    mOldHandles.AppendElement(mHandles.forget());
    mHandles = new nsIContent*[NS_HTML5_TREE_BUILDER_HANDLE_ARRAY_LENGTH];
    mHandlesUsed = 0;
  }
  return &mHandles[mHandlesUsed++];
}

PRBool
nsHtml5TreeBuilder::HasScript()
{
  PRUint32 len = mOpQueue.Length();
  if (!len) {
    return PR_FALSE;
  }
  return mOpQueue.ElementAt(len - 1).IsRunScript();
}


void
nsHtml5TreeBuilder::documentMode(nsHtml5DocumentMode m)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(m);
}
