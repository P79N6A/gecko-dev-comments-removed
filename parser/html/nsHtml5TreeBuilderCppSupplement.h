







































#include "nsContentErrors.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsEvent.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsContentUtils.h"
#include "nsNodeUtils.h"
#include "nsHtml5SpeculativeLoader.h"


jArray<PRUnichar,PRInt32> nsHtml5TreeBuilder::ISINDEX_PROMPT = jArray<PRUnichar,PRInt32>();
nsHtml5TreeBuilder::nsHtml5TreeBuilder(nsAHtml5TreeOpSink* aOpSink)
  : scriptingEnabled(PR_FALSE)
  , fragment(PR_FALSE)
  , contextNode(nsnull)
  , formPointer(nsnull)
  , headPointer(nsnull)
  , mOpSink(aOpSink)
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

class nsHtml5SpeculativeScript : public nsRunnable
{
private:
  nsRefPtr<nsHtml5SpeculativeLoader> mSpeculativeLoader;
  nsString                           mURL;
  nsString                           mCharset;
  nsString                           mType;
public:
  nsHtml5SpeculativeScript(nsHtml5SpeculativeLoader* aSpeculativeLoader,
                           const nsAString& aURL,
                           const nsAString& aCharset,
                           const nsAString& aType)
    : mSpeculativeLoader(aSpeculativeLoader)
    , mURL(aURL)
    , mCharset(aCharset)
    , mType(aType)
  {}
  NS_IMETHODIMP Run()
  {
    mSpeculativeLoader->PreloadScript(mURL, mCharset, mType);
    return NS_OK;
  }
};

class nsHtml5SpeculativeStyle : public nsRunnable
{
private:
  nsRefPtr<nsHtml5SpeculativeLoader> mSpeculativeLoader;
  nsString                           mURL;
  nsString                           mCharset;
public:
  nsHtml5SpeculativeStyle(nsHtml5SpeculativeLoader* aSpeculativeLoader,
                          const nsAString& aURL,
                          const nsAString& aCharset)
    : mSpeculativeLoader(aSpeculativeLoader)
    , mURL(aURL)
    , mCharset(aCharset)
  {}
  NS_IMETHODIMP Run()
  {
    mSpeculativeLoader->PreloadStyle(mURL, mCharset);
    return NS_OK;
  }
};

class nsHtml5SpeculativeImage : public nsRunnable
{
private:
  nsRefPtr<nsHtml5SpeculativeLoader> mSpeculativeLoader;
  nsString                           mURL;
public:
  nsHtml5SpeculativeImage(nsHtml5SpeculativeLoader* aSpeculativeLoader,
                          const nsAString& aURL)
    : mSpeculativeLoader(aSpeculativeLoader)
    , mURL(aURL)
  {}
  NS_IMETHODIMP Run()
  {
    mSpeculativeLoader->PreloadImage(mURL);
    return NS_OK;
  }
};

nsIContent**
nsHtml5TreeBuilder::createElement(PRInt32 aNamespace, nsIAtom* aName, nsHtml5HtmlAttributes* aAttributes)
{
  NS_PRECONDITION(aAttributes, "Got null attributes.");
  
  
  
  if (mSpeculativeLoader) {
    switch (aNamespace) {
      case kNameSpaceID_XHTML:
        if (nsHtml5Atoms::img == aName) {
          nsString* url = aAttributes->getValue(nsHtml5AttributeName::ATTR_SRC);
          if (url) {
            Dispatch(new nsHtml5SpeculativeImage(mSpeculativeLoader, *url));
          }
        } else if (nsHtml5Atoms::script == aName) {
          nsString* url = aAttributes->getValue(nsHtml5AttributeName::ATTR_SRC);
          if (url) {
            nsString* charset = aAttributes->getValue(nsHtml5AttributeName::ATTR_CHARSET);
            nsString* type = aAttributes->getValue(nsHtml5AttributeName::ATTR_TYPE);
            Dispatch(new nsHtml5SpeculativeScript(mSpeculativeLoader, 
                                                  *url,
                                                  (charset) ? *charset : EmptyString(),
                                                  (type) ? *type : EmptyString()));
          }
        } else if (nsHtml5Atoms::link == aName) {
          nsString* rel = aAttributes->getValue(nsHtml5AttributeName::ATTR_REL);
          
          
          if (rel && rel->LowerCaseEqualsASCII("stylesheet")) {
            nsString* url = aAttributes->getValue(nsHtml5AttributeName::ATTR_HREF);
            if (url) {
              nsString* charset = aAttributes->getValue(nsHtml5AttributeName::ATTR_CHARSET);
              Dispatch(new nsHtml5SpeculativeStyle(mSpeculativeLoader, 
                                                   *url,
                                                   (charset) ? *charset : EmptyString()));
            }
          }
        } else if (nsHtml5Atoms::video == aName) {
          nsString* url = aAttributes->getValue(nsHtml5AttributeName::ATTR_POSTER);
          if (url) {
            Dispatch(new nsHtml5SpeculativeImage(mSpeculativeLoader, *url));
          }
        }
        break;
      case kNameSpaceID_SVG:
        if (nsHtml5Atoms::image == aName) {
          nsString* url = aAttributes->getValue(nsHtml5AttributeName::ATTR_XLINK_HREF);
          if (url) {
            Dispatch(new nsHtml5SpeculativeImage(mSpeculativeLoader, *url));
          }
        } else if (nsHtml5Atoms::script == aName) {
          nsString* url = aAttributes->getValue(nsHtml5AttributeName::ATTR_XLINK_HREF);
          if (url) {
            nsString* type = aAttributes->getValue(nsHtml5AttributeName::ATTR_TYPE);
            Dispatch(new nsHtml5SpeculativeScript(mSpeculativeLoader, 
                                                  *url,
                                                  EmptyString(),
                                                  (type) ? *type : EmptyString()));
          }
        } else if (nsHtml5Atoms::style == aName) {
          nsString* url = aAttributes->getValue(nsHtml5AttributeName::ATTR_XLINK_HREF);
          if (url) {
            Dispatch(new nsHtml5SpeculativeStyle(mSpeculativeLoader, 
                                                 *url,
                                                 EmptyString()));
          }
        }        
        break;
    }
  }

  
  
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
  
  treeOp->Init(eTreeOpAppend, aChild, aParent);
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
  
  treeOp->Init(eTreeOpAppend, text, aParent);
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
  
  treeOp->Init(eTreeOpAppend, comment, aParent);
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
  
#ifdef DEBUG
  mActive = PR_TRUE;
#endif
}

void
nsHtml5TreeBuilder::end()
{
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
    if (aName == nsHtml5Atoms::html) {
      nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
      
      treeOp->Init(eTreeOpProcessOfflineManifest, aElement);
      return;
    }
  }
}

void
nsHtml5TreeBuilder::elementPopped(PRInt32 aNamespace, nsIAtom* aName, nsIContent** aElement)
{
  NS_ASSERTION(aNamespace == kNameSpaceID_XHTML || aNamespace == kNameSpaceID_SVG || aNamespace == kNameSpaceID_MathML, "Element isn't HTML, SVG or MathML!");
  NS_ASSERTION(aName, "Element doesn't have local name!");
  NS_ASSERTION(aElement, "No element!");
  if (aNamespace == kNameSpaceID_MathML) {
    return;
  }
  
  if (aName == nsHtml5Atoms::script) {
    requestSuspension();
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->InitScript(aElement);
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
  if (aName == nsHtml5Atoms::head) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpStartLayout, nsnull);
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
nsHtml5TreeBuilder::Flush()
{
  mOpSink->ForcedFlush(mOpQueue);
}

void
nsHtml5TreeBuilder::MaybeFlush()
{
  mOpSink->MaybeFlush(mOpQueue);
}

void
nsHtml5TreeBuilder::SetDocumentCharset(nsACString& aCharset)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpSetDocumentCharset, aCharset);  
}

void
nsHtml5TreeBuilder::StreamEnded()
{
  
  
  
  
  
  if (!fragment) {
    nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
    
    treeOp->Init(eTreeOpStreamEnded);
  }
}

void
nsHtml5TreeBuilder::NeedsCharsetSwitchTo(const nsACString& aCharset)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(eTreeOpNeedsCharsetSwitchTo, aCharset);  
}

void
nsHtml5TreeBuilder::AddSnapshotToScript(nsAHtml5TreeBuilderState* aSnapshot)
{
  NS_PRECONDITION(HasScript(), "No script to add a snapshot to!");
  NS_PRECONDITION(aSnapshot, "Got null snapshot.");
  mOpQueue.ElementAt(mOpQueue.Length() - 1).SetSnapshot(aSnapshot);
}

void
nsHtml5TreeBuilder::SetSpeculativeLoaderWithDocument(nsIDocument* aDocument) {
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mSpeculativeLoader = new nsHtml5SpeculativeLoader(aDocument);
}

void
nsHtml5TreeBuilder::DropSpeculativeLoader() {
  mSpeculativeLoader = nsnull;
}


void
nsHtml5TreeBuilder::documentMode(nsHtml5DocumentMode m)
{
  nsHtml5TreeOperation* treeOp = mOpQueue.AppendElement();
  
  treeOp->Init(m);
}
