











#include "mozilla/ArrayUtils.h"

#include "nsContentSink.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIHTMLContentSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsScriptLoader.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIContentViewer.h"
#include "mozilla/dom/NodeInfo.h"
#include "nsToken.h"
#include "nsIAppShell.h"
#include "nsCRT.h"
#include "prtime.h"
#include "prlog.h"
#include "nsNodeUtils.h"
#include "nsIContent.h"
#include "mozilla/dom/Element.h"
#include "mozilla/Preferences.h"

#include "nsGenericHTMLElement.h"

#include "nsIDOMDocument.h"
#include "nsIDOMDocumentType.h"
#include "nsIScriptElement.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"

#include "nsGkAtoms.h"
#include "nsContentUtils.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsStubDocumentObserver.h"
#include "nsIHTMLDocument.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsICookieService.h"
#include "nsTArray.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsTextFragment.h"
#include "nsIScriptGlobalObject.h"
#include "nsNameSpaceManager.h"

#include "nsIParserService.h"

#include "nsIStyleSheetLinkingElement.h"
#include "nsITimer.h"
#include "nsError.h"
#include "nsContentPolicyUtils.h"
#include "nsIScriptContext.h"
#include "nsStyleLinkElement.h"

#include "nsWeakReference.h" 
#include "nsIPrompt.h"
#include "nsLayoutCID.h"
#include "nsIDocShellTreeItem.h"

#include "nsEscape.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "mozAutoDocUpdate.h"
#include "nsTextNode.h"

using namespace mozilla;
using namespace mozilla::dom;



typedef nsGenericHTMLElement*
  (*contentCreatorCallback)(already_AddRefed<mozilla::dom::NodeInfo>&&,
                            FromParser aFromParser);

nsGenericHTMLElement*
NS_NewHTMLNOTUSEDElement(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                         FromParser aFromParser)
{
  NS_NOTREACHED("The element ctor should never be called");
  return nullptr;
}

#define HTML_TAG(_tag, _classname) NS_NewHTML##_classname##Element,
#define HTML_HTMLELEMENT_TAG(_tag) NS_NewHTMLElement,
#define HTML_OTHER(_tag) NS_NewHTMLNOTUSEDElement,
static const contentCreatorCallback sContentCreatorCallbacks[] = {
  NS_NewHTMLUnknownElement,
#include "nsHTMLTagList.h"
#undef HTML_TAG
#undef HTML_HTMLELEMENT_TAG
#undef HTML_OTHER
  NS_NewHTMLUnknownElement
};

class SinkContext;
class HTMLContentSink;





class HTMLContentSink : public nsContentSink,
                        public nsIHTMLContentSink
{
public:
  friend class SinkContext;

  HTMLContentSink();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  nsresult Init(nsIDocument* aDoc, nsIURI* aURI, nsISupports* aContainer,
                nsIChannel* aChannel);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLContentSink, nsContentSink)

  
  NS_IMETHOD WillParse(void);
  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);
  NS_IMETHOD DidBuildModel(bool aTerminated);
  NS_IMETHOD WillInterrupt(void);
  NS_IMETHOD WillResume(void);
  NS_IMETHOD SetParser(nsParserBase* aParser);
  virtual void FlushPendingNotifications(mozFlushType aType);
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset);
  virtual nsISupports *GetTarget();
  virtual bool IsScriptExecuting();

  
  NS_IMETHOD OpenContainer(ElementType aNodeType);
  NS_IMETHOD CloseContainer(ElementType aTag);

protected:
  virtual ~HTMLContentSink();

  nsCOMPtr<nsIHTMLDocument> mHTMLDocument;

  
  int32_t mMaxTextRun;

  nsRefPtr<nsGenericHTMLElement> mRoot;
  nsRefPtr<nsGenericHTMLElement> mBody;
  nsRefPtr<nsGenericHTMLElement> mHead;

  nsAutoTArray<SinkContext*, 8> mContextStack;
  SinkContext* mCurrentContext;
  SinkContext* mHeadContext;

  
  
  bool mHaveSeenHead;

  
  
  bool mNotifiedRootInsertion;

  uint8_t mScriptEnabled : 1;
  uint8_t mFramesEnabled : 1;
  uint8_t unused : 6;  

  mozilla::dom::NodeInfo* mNodeInfoCache[NS_HTML_TAG_MAX + 1];

  nsresult FlushTags();

  
  nsresult CloseHTML();
  nsresult OpenBody();
  nsresult CloseBody();

  void CloseHeadContext();

  
  void UpdateChildCounts();

  void NotifyInsert(nsIContent* aContent,
                    nsIContent* aChildContent,
                    int32_t aIndexInContainer);
  void NotifyRootInsertion();
};

class SinkContext
{
public:
  explicit SinkContext(HTMLContentSink* aSink);
  ~SinkContext();

  nsresult Begin(nsHTMLTag aNodeType, nsGenericHTMLElement* aRoot,
                 uint32_t aNumFlushed, int32_t aInsertionPoint);
  nsresult OpenBody();
  nsresult CloseBody();
  nsresult End();

  nsresult GrowStack();
  nsresult FlushTags();

  bool     IsCurrentContainer(nsHTMLTag mType);

  void DidAddContent(nsIContent* aContent);
  void UpdateChildCounts();

private:
  
  
  
  bool HaveNotifiedForCurrentContent() const;
  
public:
  HTMLContentSink* mSink;
  int32_t mNotifyLevel;

  struct Node {
    nsHTMLTag mType;
    nsGenericHTMLElement* mContent;
    uint32_t mNumFlushed;
    int32_t mInsertionPoint;

    nsIContent *Add(nsIContent *child);
  };

  Node* mStack;
  int32_t mStackSize;
  int32_t mStackPos;
};

nsresult
NS_NewHTMLElement(Element** aResult, already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                  FromParser aFromParser)
{
  *aResult = nullptr;

  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo = aNodeInfo;

  nsIParserService* parserService = nsContentUtils::GetParserService();
  if (!parserService)
    return NS_ERROR_OUT_OF_MEMORY;

  nsIAtom *name = nodeInfo->NameAtom();

  NS_ASSERTION(nodeInfo->NamespaceEquals(kNameSpaceID_XHTML), 
               "Trying to HTML elements that don't have the XHTML namespace");

  
  
  int32_t tag = parserService->HTMLCaseSensitiveAtomTagToId(name);
  if (tag == eHTMLTag_userdefined &&
      nsContentUtils::IsCustomElementName(name)) {
    nsIDocument* doc = nodeInfo->GetDocument();

    NS_IF_ADDREF(*aResult = NS_NewHTMLElement(nodeInfo.forget(), aFromParser));
    if (!*aResult) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    doc->RegisterUnresolvedElement(*aResult);

    
    
    
    doc->EnqueueLifecycleCallback(nsIDocument::eCreated, *aResult);

    return NS_OK;
  }

  *aResult = CreateHTMLElement(tag,
                               nodeInfo.forget(), aFromParser).take();
  return *aResult ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

already_AddRefed<nsGenericHTMLElement>
CreateHTMLElement(uint32_t aNodeType,
                  already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo,
                  FromParser aFromParser)
{
  NS_ASSERTION(aNodeType <= NS_HTML_TAG_MAX ||
               aNodeType == eHTMLTag_userdefined,
               "aNodeType is out of bounds");

  contentCreatorCallback cb = sContentCreatorCallbacks[aNodeType];

  NS_ASSERTION(cb != NS_NewHTMLNOTUSEDElement,
               "Don't know how to construct tag element!");

  nsRefPtr<nsGenericHTMLElement> result = cb(Move(aNodeInfo), aFromParser);

  return result.forget();
}



SinkContext::SinkContext(HTMLContentSink* aSink)
  : mSink(aSink),
    mNotifyLevel(0),
    mStack(nullptr),
    mStackSize(0),
    mStackPos(0)
{
  MOZ_COUNT_CTOR(SinkContext);
}

SinkContext::~SinkContext()
{
  MOZ_COUNT_DTOR(SinkContext);

  if (mStack) {
    for (int32_t i = 0; i < mStackPos; i++) {
      NS_RELEASE(mStack[i].mContent);
    }
    delete [] mStack;
  }
}

nsresult
SinkContext::Begin(nsHTMLTag aNodeType,
                   nsGenericHTMLElement* aRoot,
                   uint32_t aNumFlushed,
                   int32_t aInsertionPoint)
{
  if (mStackSize < 1) {
    nsresult rv = GrowStack();
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  mStack[0].mType = aNodeType;
  mStack[0].mContent = aRoot;
  mStack[0].mNumFlushed = aNumFlushed;
  mStack[0].mInsertionPoint = aInsertionPoint;
  NS_ADDREF(aRoot);
  mStackPos = 1;

  return NS_OK;
}

bool
SinkContext::IsCurrentContainer(nsHTMLTag aTag)
{
  if (aTag == mStack[mStackPos - 1].mType) {
    return true;
  }

  return false;
}

void
SinkContext::DidAddContent(nsIContent* aContent)
{
  if ((mStackPos == 2) && (mSink->mBody == mStack[1].mContent)) {
    
    mNotifyLevel = 0;
  }

  
  
  
  if (0 < mStackPos &&
      mStack[mStackPos - 1].mInsertionPoint != -1 &&
      mStack[mStackPos - 1].mNumFlushed <
      mStack[mStackPos - 1].mContent->GetChildCount()) {
    nsIContent* parent = mStack[mStackPos - 1].mContent;
    int32_t childIndex = mStack[mStackPos - 1].mInsertionPoint - 1;
    NS_ASSERTION(parent->GetChildAt(childIndex) == aContent,
                 "Flushing the wrong child.");
    mSink->NotifyInsert(parent, aContent, childIndex);
    mStack[mStackPos - 1].mNumFlushed = parent->GetChildCount();
  } else if (mSink->IsTimeToNotify()) {
    FlushTags();
  }
}

nsresult
SinkContext::OpenBody()
{
  if (mStackPos <= 0) {
    NS_ERROR("container w/o parent");

    return NS_ERROR_FAILURE;
  }

  nsresult rv;
  if (mStackPos + 1 > mStackSize) {
    rv = GrowStack();
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

    nsRefPtr<mozilla::dom::NodeInfo> nodeInfo =
      mSink->mNodeInfoManager->GetNodeInfo(nsGkAtoms::body, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_UNEXPECTED);

  
  nsRefPtr<nsGenericHTMLElement> body =
    NS_NewHTMLBodyElement(nodeInfo.forget(), FROM_PARSER_NETWORK);
  if (!body) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mStack[mStackPos].mType = eHTMLTag_body;
  body.forget(&mStack[mStackPos].mContent);
  mStack[mStackPos].mNumFlushed = 0;
  mStack[mStackPos].mInsertionPoint = -1;
  ++mStackPos;
  mStack[mStackPos - 2].Add(mStack[mStackPos - 1].mContent);

  return NS_OK;
}

bool
SinkContext::HaveNotifiedForCurrentContent() const
{
  if (0 < mStackPos) {
    nsIContent* parent = mStack[mStackPos - 1].mContent;
    return mStack[mStackPos-1].mNumFlushed == parent->GetChildCount();
  }

  return true;
}

nsIContent *
SinkContext::Node::Add(nsIContent *child)
{
  NS_ASSERTION(mContent, "No parent to insert/append into!");
  if (mInsertionPoint != -1) {
    NS_ASSERTION(mNumFlushed == mContent->GetChildCount(),
                 "Inserting multiple children without flushing.");
    mContent->InsertChildAt(child, mInsertionPoint++, false);
  } else {
    mContent->AppendChildTo(child, false);
  }
  return child;
}

nsresult
SinkContext::CloseBody()
{
  NS_ASSERTION(mStackPos > 0,
               "stack out of bounds. wrong context probably!");

  if (mStackPos <= 0) {
    return NS_OK; 
  }

  --mStackPos;
  NS_ASSERTION(mStack[mStackPos].mType == eHTMLTag_body,
               "Tag mismatch.  Closing tag on wrong context or something?");

  nsGenericHTMLElement* content = mStack[mStackPos].mContent;

  content->Compact();

  
  
  
  if (mNotifyLevel >= mStackPos) {
    
    

    if (mStack[mStackPos].mNumFlushed < content->GetChildCount()) {
      mSink->NotifyAppend(content, mStack[mStackPos].mNumFlushed);
      mStack[mStackPos].mNumFlushed = content->GetChildCount();
    }

    
    mNotifyLevel = mStackPos - 1;
  }

  DidAddContent(content);
  NS_IF_RELEASE(content);

  return NS_OK;
}

nsresult
SinkContext::End()
{
  for (int32_t i = 0; i < mStackPos; i++) {
    NS_RELEASE(mStack[i].mContent);
  }

  mStackPos = 0;

  return NS_OK;
}

nsresult
SinkContext::GrowStack()
{
  int32_t newSize = mStackSize * 2;
  if (newSize == 0) {
    newSize = 32;
  }

  Node* stack = new Node[newSize];

  if (mStackPos != 0) {
    memcpy(stack, mStack, sizeof(Node) * mStackPos);
    delete [] mStack;
  }

  mStack = stack;
  mStackSize = newSize;

  return NS_OK;
}










nsresult
SinkContext::FlushTags()
{
  mSink->mDeferredFlushTags = false;
  bool oldBeganUpdate = mSink->mBeganUpdate;
  uint32_t oldUpdates = mSink->mUpdatesInNotification;

  ++(mSink->mInNotification);
  mSink->mUpdatesInNotification = 0;
  {
    
    mozAutoDocUpdate updateBatch(mSink->mDocument, UPDATE_CONTENT_MODEL,
                                 true);
    mSink->mBeganUpdate = true;

    
    
    

    
    
    
    int32_t stackPos = 0;
    bool flushed = false;
    uint32_t childCount;
    nsGenericHTMLElement* content;

    while (stackPos < mStackPos) {
      content = mStack[stackPos].mContent;
      childCount = content->GetChildCount();

      if (!flushed && (mStack[stackPos].mNumFlushed < childCount)) {
        if (mStack[stackPos].mInsertionPoint != -1) {
          
          
          

          int32_t childIndex = mStack[stackPos].mInsertionPoint - 1;
          nsIContent* child = content->GetChildAt(childIndex);
          
          NS_ASSERTION(!(mStackPos > (stackPos + 1)) ||
                       (child == mStack[stackPos + 1].mContent),
                       "Flushing the wrong child.");
          mSink->NotifyInsert(content, child, childIndex);
        } else {
          mSink->NotifyAppend(content, mStack[stackPos].mNumFlushed);
        }

        flushed = true;
      }

      mStack[stackPos].mNumFlushed = childCount;
      stackPos++;
    }
    mNotifyLevel = mStackPos - 1;
  }
  --(mSink->mInNotification);

  if (mSink->mUpdatesInNotification > 1) {
    UpdateChildCounts();
  }

  mSink->mUpdatesInNotification = oldUpdates;
  mSink->mBeganUpdate = oldBeganUpdate;

  return NS_OK;
}




void
SinkContext::UpdateChildCounts()
{
  
  
  
  
  
  int32_t stackPos = mStackPos - 1;
  while (stackPos >= 0) {
    Node & node = mStack[stackPos];
    node.mNumFlushed = node.mContent->GetChildCount();

    stackPos--;
  }

  mNotifyLevel = mStackPos - 1;
}

nsresult
NS_NewHTMLContentSink(nsIHTMLContentSink** aResult,
                      nsIDocument* aDoc,
                      nsIURI* aURI,
                      nsISupports* aContainer,
                      nsIChannel* aChannel)
{
  NS_ENSURE_ARG_POINTER(aResult);

  nsRefPtr<HTMLContentSink> it = new HTMLContentSink();

  nsresult rv = it->Init(aDoc, aURI, aContainer, aChannel);

  NS_ENSURE_SUCCESS(rv, rv);

  *aResult = it;
  NS_ADDREF(*aResult);

  return NS_OK;
}

HTMLContentSink::HTMLContentSink()
{
  
}

HTMLContentSink::~HTMLContentSink()
{
  if (mNotificationTimer) {
    mNotificationTimer->Cancel();
  }

  int32_t numContexts = mContextStack.Length();

  if (mCurrentContext == mHeadContext && numContexts > 0) {
    
    mContextStack.RemoveElementAt(--numContexts);
  }

  int32_t i;
  for (i = 0; i < numContexts; i++) {
    SinkContext* sc = mContextStack.ElementAt(i);
    if (sc) {
      sc->End();
      if (sc == mCurrentContext) {
        mCurrentContext = nullptr;
      }

      delete sc;
    }
  }

  if (mCurrentContext == mHeadContext) {
    mCurrentContext = nullptr;
  }

  delete mCurrentContext;

  delete mHeadContext;

  for (i = 0; uint32_t(i) < ArrayLength(mNodeInfoCache); ++i) {
    NS_IF_RELEASE(mNodeInfoCache[i]);
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLContentSink)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLContentSink, nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mHTMLDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRoot)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mBody)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mHead)
  for (uint32_t i = 0; i < ArrayLength(tmp->mNodeInfoCache); ++i) {
    NS_IF_RELEASE(tmp->mNodeInfoCache[i]);
  }
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLContentSink,
                                                  nsContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mHTMLDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRoot)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mBody)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mHead)
  for (uint32_t i = 0; i < ArrayLength(tmp->mNodeInfoCache); ++i) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "mNodeInfoCache[i]");
    cb.NoteNativeChild(tmp->mNodeInfoCache[i],
                       NS_CYCLE_COLLECTION_PARTICIPANT(NodeInfo));
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLContentSink)
  NS_INTERFACE_TABLE_BEGIN
    NS_INTERFACE_TABLE_ENTRY(HTMLContentSink, nsIContentSink)
    NS_INTERFACE_TABLE_ENTRY(HTMLContentSink, nsIHTMLContentSink)
  NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL_INHERITING(nsContentSink)

NS_IMPL_ADDREF_INHERITED(HTMLContentSink, nsContentSink)
NS_IMPL_RELEASE_INHERITED(HTMLContentSink, nsContentSink)

static bool
IsScriptEnabled(nsIDocument *aDoc, nsIDocShell *aContainer)
{
  NS_ENSURE_TRUE(aDoc && aContainer, true);

  nsCOMPtr<nsIScriptGlobalObject> globalObject =
    do_QueryInterface(aDoc->GetInnerWindow());

  
  
  if (!globalObject) {
    globalObject = aContainer->GetScriptGlobalObject();
  }

  NS_ENSURE_TRUE(globalObject && globalObject->GetGlobalJSObject(), true);
  return nsContentUtils::GetSecurityManager()->
           ScriptAllowed(globalObject->GetGlobalJSObject());
}

nsresult
HTMLContentSink::Init(nsIDocument* aDoc,
                      nsIURI* aURI,
                      nsISupports* aContainer,
                      nsIChannel* aChannel)
{
  NS_ENSURE_TRUE(aContainer, NS_ERROR_NULL_POINTER);
  
  nsresult rv = nsContentSink::Init(aDoc, aURI, aContainer, aChannel);
  if (NS_FAILED(rv)) {
    return rv;
  }

  aDoc->AddObserver(this);
  mIsDocumentObserver = true;
  mHTMLDocument = do_QueryInterface(aDoc);

  NS_ASSERTION(mDocShell, "oops no docshell!");

  
  if (mDocShell) {
    bool subFramesEnabled = true;
    mDocShell->GetAllowSubframes(&subFramesEnabled);
    if (subFramesEnabled) {
      mFramesEnabled = true;
    }
  }

  
  if (IsScriptEnabled(aDoc, mDocShell)) {
    mScriptEnabled = true;
  }


  
  
  mMaxTextRun = Preferences::GetInt("content.maxtextrun", 8191);

  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::html, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);

  
  mRoot = NS_NewHTMLHtmlElement(nodeInfo.forget());
  if (!mRoot) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ASSERTION(mDocument->GetChildCount() == 0,
               "Document should have no kids here!");
  rv = mDocument->AppendChildTo(mRoot, false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::head,
                                           nullptr, kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);

  mHead = NS_NewHTMLHeadElement(nodeInfo.forget());
  if (NS_FAILED(rv)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mRoot->AppendChildTo(mHead, false);

  mCurrentContext = new SinkContext(this);
  mCurrentContext->Begin(eHTMLTag_html, mRoot, 0, -1);
  mContextStack.AppendElement(mCurrentContext);

  return NS_OK;
}

NS_IMETHODIMP
HTMLContentSink::WillParse(void)
{
  return WillParseImpl();
}

NS_IMETHODIMP
HTMLContentSink::WillBuildModel(nsDTDMode aDTDMode)
{
  WillBuildModelImpl();

  if (mHTMLDocument) {
    nsCompatibility mode = eCompatibility_NavQuirks;
    switch (aDTDMode) {
      case eDTDMode_full_standards:
        mode = eCompatibility_FullStandards;
        break;
      case eDTDMode_almost_standards:
        mode = eCompatibility_AlmostStandards;
        break;
      default:
        break;
    }
    mHTMLDocument->SetCompatibilityMode(mode);
  }

  
  mDocument->BeginLoad();

  return NS_OK;
}

NS_IMETHODIMP
HTMLContentSink::DidBuildModel(bool aTerminated)
{
  DidBuildModelImpl(aTerminated);

  
  if (mBody) {
    mCurrentContext->FlushTags();
  } else if (!mLayoutStarted) {
    
    
    
    
    
    
    bool bDestroying = true;
    if (mDocShell) {
      mDocShell->IsBeingDestroyed(&bDestroying);
    }

    if (!bDestroying) {
      StartLayout(false);
    }
  }

  ScrollToRef();

  
  

  
  
  
  mDocument->RemoveObserver(this);
  mIsDocumentObserver = false;
  
  mDocument->EndLoad();

  DropParserAndPerfHint();

  return NS_OK;
}

NS_IMETHODIMP
HTMLContentSink::SetParser(nsParserBase* aParser)
{
  NS_PRECONDITION(aParser, "Should have a parser here!");
  mParser = aParser;
  return NS_OK;
}

nsresult
HTMLContentSink::CloseHTML()
{
  if (mHeadContext) {
    if (mCurrentContext == mHeadContext) {
      uint32_t numContexts = mContextStack.Length();

      
      mCurrentContext = mContextStack.ElementAt(--numContexts);
      mContextStack.RemoveElementAt(numContexts);
    }

    mHeadContext->End();

    delete mHeadContext;
    mHeadContext = nullptr;
  }

  return NS_OK;
}

nsresult
HTMLContentSink::OpenBody()
{
  CloseHeadContext();  

  
  if (mBody) {
    return NS_OK;
  }

  nsresult rv = mCurrentContext->OpenBody();

  if (NS_FAILED(rv)) {
    return rv;
  }

  mBody = mCurrentContext->mStack[mCurrentContext->mStackPos - 1].mContent;

  if (mCurrentContext->mStackPos > 1) {
    int32_t parentIndex    = mCurrentContext->mStackPos - 2;
    nsGenericHTMLElement *parent = mCurrentContext->mStack[parentIndex].mContent;
    int32_t numFlushed     = mCurrentContext->mStack[parentIndex].mNumFlushed;
    int32_t childCount = parent->GetChildCount();
    NS_ASSERTION(numFlushed < childCount, "Already notified on the body?");
    
    int32_t insertionPoint =
      mCurrentContext->mStack[parentIndex].mInsertionPoint;

    
    
    

    uint32_t oldUpdates = mUpdatesInNotification;
    mUpdatesInNotification = 0;
    if (insertionPoint != -1) {
      NotifyInsert(parent, mBody, insertionPoint - 1);
    } else {
      NotifyAppend(parent, numFlushed);
    }
    mCurrentContext->mStack[parentIndex].mNumFlushed = childCount;
    if (mUpdatesInNotification > 1) {
      UpdateChildCounts();
    }
    mUpdatesInNotification = oldUpdates;
  }

  StartLayout(false);

  return NS_OK;
}

nsresult
HTMLContentSink::CloseBody()
{
  
  mCurrentContext->FlushTags();
  mCurrentContext->CloseBody();

  return NS_OK;
}

NS_IMETHODIMP
HTMLContentSink::OpenContainer(ElementType aElementType)
{
  nsresult rv = NS_OK;

  switch (aElementType) {
    case eBody:
      rv = OpenBody();
      break;
    case eHTML:
      if (mRoot) {
        
        if (!mNotifiedRootInsertion) {
          NotifyRootInsertion();
        }
        ProcessOfflineManifest(mRoot);
      }
      break;
  }

  return rv;
}

NS_IMETHODIMP
HTMLContentSink::CloseContainer(const ElementType aTag)
{
  nsresult rv = NS_OK;

  switch (aTag) {
    case eBody:
      rv = CloseBody();
      break;
    case eHTML:
      rv = CloseHTML();
      break;
  }

  return rv;
}

NS_IMETHODIMP
HTMLContentSink::WillInterrupt()
{
  return WillInterruptImpl();
}

NS_IMETHODIMP
HTMLContentSink::WillResume()
{
  return WillResumeImpl();
}

void
HTMLContentSink::CloseHeadContext()
{
  if (mCurrentContext) {
    if (!mCurrentContext->IsCurrentContainer(eHTMLTag_head))
      return;

    mCurrentContext->FlushTags();
  }

  if (!mContextStack.IsEmpty())
  {
    uint32_t n = mContextStack.Length() - 1;
    mCurrentContext = mContextStack.ElementAt(n);
    mContextStack.RemoveElementAt(n);
  }
}

void
HTMLContentSink::NotifyInsert(nsIContent* aContent,
                              nsIContent* aChildContent,
                              int32_t aIndexInContainer)
{
  if (aContent && aContent->GetCurrentDoc() != mDocument) {
    
    
    return;
  }

  mInNotification++;

  {
    
    MOZ_AUTO_DOC_UPDATE(mDocument, UPDATE_CONTENT_MODEL, !mBeganUpdate);
    nsNodeUtils::ContentInserted(NODE_FROM(aContent, mDocument),
                                 aChildContent, aIndexInContainer);
    mLastNotificationTime = PR_Now();
  }

  mInNotification--;
}

void
HTMLContentSink::NotifyRootInsertion()
{
  NS_PRECONDITION(!mNotifiedRootInsertion, "Double-notifying on root?");
  NS_ASSERTION(!mLayoutStarted,
               "How did we start layout without notifying on root?");
  
  
  
  
  
  
  
  mNotifiedRootInsertion = true;
  int32_t index = mDocument->IndexOf(mRoot);
  NS_ASSERTION(index != -1, "mRoot not child of document?");
  NotifyInsert(nullptr, mRoot, index);

  
  
  
  UpdateChildCounts();
}

void
HTMLContentSink::UpdateChildCounts()
{
  uint32_t numContexts = mContextStack.Length();
  for (uint32_t i = 0; i < numContexts; i++) {
    SinkContext* sc = mContextStack.ElementAt(i);

    sc->UpdateChildCounts();
  }

  mCurrentContext->UpdateChildCounts();
}

void
HTMLContentSink::FlushPendingNotifications(mozFlushType aType)
{
  
  
  if (!mInNotification) {
    
    
    if (mIsDocumentObserver) {
      if (aType >= Flush_ContentAndNotify) {
        FlushTags();
      }
    }
    if (aType >= Flush_InterruptibleLayout) {
      
      
      StartLayout(true);
    }
  }
}

nsresult
HTMLContentSink::FlushTags()
{
  if (!mNotifiedRootInsertion) {
    NotifyRootInsertion();
    return NS_OK;
  }
  
  return mCurrentContext ? mCurrentContext->FlushTags() : NS_OK;
}

NS_IMETHODIMP
HTMLContentSink::SetDocumentCharset(nsACString& aCharset)
{
  MOZ_ASSERT_UNREACHABLE("<meta charset> case doesn't occur with about:blank");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsISupports *
HTMLContentSink::GetTarget()
{
  return mDocument;
}

bool
HTMLContentSink::IsScriptExecuting()
{
  return IsScriptExecutingImpl();
}
