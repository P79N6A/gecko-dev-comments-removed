









































#include "nsContentSink.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIHTMLContentSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIParser.h"
#include "nsParserUtils.h"
#include "nsScriptLoader.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsIContentViewer.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsINodeInfo.h"
#include "nsHTMLTokens.h"
#include "nsIAppShell.h"
#include "nsCRT.h"
#include "prtime.h"
#include "prlog.h"
#include "nsInt64.h"
#include "nsNodeUtils.h"
#include "nsIContent.h"

#include "nsGenericHTMLElement.h"

#include "nsIDOMText.h"
#include "nsIDOMComment.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMDOMImplementation.h"
#include "nsIDOMDocumentType.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIScriptElement.h"

#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIFormControl.h"
#include "nsIForm.h"

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
#include "nsINameSpaceManager.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsICookieService.h"
#include "nsTArray.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsTextFragment.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptGlobalObjectOwner.h"

#include "nsIParserService.h"
#include "nsISelectElement.h"

#include "nsIStyleSheetLinkingElement.h"
#include "nsITimer.h"
#include "nsDOMError.h"
#include "nsContentPolicyUtils.h"
#include "nsIScriptContext.h"
#include "nsStyleLinkElement.h"

#include "nsReadableUtils.h"
#include "nsWeakReference.h" 
#include "nsIPrompt.h"
#include "nsLayoutCID.h"
#include "nsIDocShellTreeItem.h"

#include "nsEscape.h"
#include "nsIElementObserver.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "mozAutoDocUpdate.h"

#ifdef NS_DEBUG
static PRLogModuleInfo* gSinkLogModuleInfo;

#define SINK_TRACE_NODE(_bit, _msg, _tag, _sp, _obj) \
  _obj->SinkTraceNode(_bit, _msg, _tag, _sp, this)

#else
#define SINK_TRACE_NODE(_bit, _msg, _tag, _sp, _obj)
#endif



typedef nsGenericHTMLElement* (*contentCreatorCallback)(nsINodeInfo*, PRBool aFromParser);

nsGenericHTMLElement*
NS_NewHTMLNOTUSEDElement(nsINodeInfo *aNodeInfo, PRBool aFromParser)
{
  NS_NOTREACHED("The element ctor should never be called");
  return nsnull;
}

#define HTML_TAG(_tag, _classname) NS_NewHTML##_classname##Element,
#define HTML_OTHER(_tag) NS_NewHTMLNOTUSEDElement,
static const contentCreatorCallback sContentCreatorCallbacks[] = {
  NS_NewHTMLUnknownElement,
#include "nsHTMLTagList.h"
#undef HTML_TAG
#undef HTML_OTHER
  NS_NewHTMLUnknownElement
};

class SinkContext;
class HTMLContentSink;

static void MaybeSetForm(nsGenericHTMLElement*, nsHTMLTag, HTMLContentSink*);

class HTMLContentSink : public nsContentSink,
#ifdef DEBUG
                        public nsIDebugDumpContent,
#endif
                        public nsIHTMLContentSink
{
public:
  friend class SinkContext;
  friend void MaybeSetForm(nsGenericHTMLElement*, nsHTMLTag, HTMLContentSink*);

  HTMLContentSink();
  virtual ~HTMLContentSink();

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  nsresult Init(nsIDocument* aDoc, nsIURI* aURI, nsISupports* aContainer,
                nsIChannel* aChannel);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD WillParse(void);
  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);
  NS_IMETHOD DidBuildModel(PRBool aTerminated);
  NS_IMETHOD WillInterrupt(void);
  NS_IMETHOD WillResume(void);
  NS_IMETHOD SetParser(nsIParser* aParser);
  virtual void FlushPendingNotifications(mozFlushType aType);
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset);
  virtual nsISupports *GetTarget();
  virtual PRBool IsScriptExecuting();

  
  NS_IMETHOD OpenContainer(const nsIParserNode& aNode);
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag);
  NS_IMETHOD CloseMalformedContainer(const nsHTMLTag aTag);
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode);
  NS_IMETHOD AddComment(const nsIParserNode& aNode);
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode);
  NS_IMETHOD AddDocTypeDecl(const nsIParserNode& aNode);
  NS_IMETHOD DidProcessTokens(void);
  NS_IMETHOD WillProcessAToken(void);
  NS_IMETHOD DidProcessAToken(void);
  NS_IMETHOD NotifyTagObservers(nsIParserNode* aNode);
  NS_IMETHOD BeginContext(PRInt32 aID);
  NS_IMETHOD EndContext(PRInt32 aID);
  NS_IMETHOD OpenHead();
  NS_IMETHOD IsEnabled(PRInt32 aTag, PRBool* aReturn);
  NS_IMETHOD_(PRBool) IsFormOnStack();

#ifdef DEBUG
  
  NS_IMETHOD DumpContentModel();
#endif

protected:
  
  
  nsresult AddAttributes(const nsIParserNode& aNode, nsIContent* aContent,
                         PRBool aNotify = PR_FALSE,
                         PRBool aCheckIfPresent = PR_FALSE);
  already_AddRefed<nsGenericHTMLElement>
  CreateContentObject(const nsIParserNode& aNode, nsHTMLTag aNodeType);

#ifdef NS_DEBUG
  void SinkTraceNode(PRUint32 aBit,
                     const char* aMsg,
                     const nsHTMLTag aTag,
                     PRInt32 aStackPos,
                     void* aThis);
#endif

  nsIHTMLDocument* mHTMLDocument;

  
  PRInt32 mMaxTextRun;

  nsGenericHTMLElement* mRoot;
  nsGenericHTMLElement* mBody;
  nsRefPtr<nsGenericHTMLElement> mFrameset;
  nsGenericHTMLElement* mHead;

  nsRefPtr<nsGenericHTMLElement> mCurrentForm;

  nsAutoTArray<SinkContext*, 8> mContextStack;
  SinkContext* mCurrentContext;
  SinkContext* mHeadContext;
  PRInt32 mNumOpenIFRAMES;

  nsCOMPtr<nsIURI> mBaseHref;
  nsCOMPtr<nsIAtom> mBaseTarget;

  
  PRInt32 mInsideNoXXXTag;

  
  
  PRPackedBool mHaveSeenHead;

  
  
  PRPackedBool mNotifiedRootInsertion;

  PRUint8 mScriptEnabled : 1;
  PRUint8 mFramesEnabled : 1;
  PRUint8 mFormOnStack : 1;
  PRUint8 unused : 5;  

  nsCOMPtr<nsIObserverEntry> mObservers;

  nsINodeInfo* mNodeInfoCache[NS_HTML_TAG_MAX + 1];

  nsresult FlushTags();

  void StartLayout(PRBool aIgnorePendingSheets);

  





  void AddBaseTagInfo(nsIContent* aContent);

  
  nsresult CloseHTML();
  nsresult OpenFrameset(const nsIParserNode& aNode);
  nsresult CloseFrameset();
  nsresult OpenBody(const nsIParserNode& aNode);
  nsresult CloseBody();
  nsresult OpenForm(const nsIParserNode& aNode);
  nsresult CloseForm();
  void ProcessBASEElement(nsGenericHTMLElement* aElement);
  nsresult ProcessLINKTag(const nsIParserNode& aNode);

  
  
  nsresult ProcessSCRIPTEndTag(nsGenericHTMLElement* content,
                               PRBool aMalformed);
  nsresult ProcessSTYLEEndTag(nsGenericHTMLElement* content);

  nsresult OpenHeadContext();
  void CloseHeadContext();

  
  virtual void PreEvaluateScript();
  virtual void PostEvaluateScript(nsIScriptElement *aElement);

  void UpdateChildCounts();

  void NotifyInsert(nsIContent* aContent,
                    nsIContent* aChildContent,
                    PRInt32 aIndexInContainer);
  void NotifyRootInsertion();
  
  PRBool IsMonolithicContainer(nsHTMLTag aTag);

#ifdef NS_DEBUG
  void ForceReflow();
#endif
};

class SinkContext
{
public:
  SinkContext(HTMLContentSink* aSink);
  ~SinkContext();

  nsresult Begin(nsHTMLTag aNodeType, nsGenericHTMLElement* aRoot,
                 PRUint32 aNumFlushed, PRInt32 aInsertionPoint);
  nsresult OpenContainer(const nsIParserNode& aNode);
  nsresult CloseContainer(const nsHTMLTag aTag, PRBool aMalformed);
  nsresult AddLeaf(const nsIParserNode& aNode);
  nsresult AddLeaf(nsIContent* aContent);
  nsresult AddComment(const nsIParserNode& aNode);
  nsresult End();

  nsresult GrowStack();
  nsresult AddText(const nsAString& aText);
  nsresult FlushText(PRBool* aDidFlush = nsnull,
                     PRBool aReleaseLast = PR_FALSE);
  nsresult FlushTextAndRelease(PRBool* aDidFlush = nsnull)
  {
    return FlushText(aDidFlush, PR_TRUE);
  }

  nsresult FlushTags();

  PRBool   IsCurrentContainer(nsHTMLTag mType);
  PRBool   IsAncestorContainer(nsHTMLTag mType);

  void DidAddContent(nsIContent* aContent);
  void UpdateChildCounts();

private:
  
  
  
  PRBool HaveNotifiedForCurrentContent() const;
  
public:
  HTMLContentSink* mSink;
  PRInt32 mNotifyLevel;
  nsCOMPtr<nsIContent> mLastTextNode;
  PRInt32 mLastTextNodeSize;

  struct Node {
    nsHTMLTag mType;
    nsGenericHTMLElement* mContent;
    PRUint32 mNumFlushed;
    PRInt32 mInsertionPoint;

    nsIContent *Add(nsIContent *child);
  };

  Node* mStack;
  PRInt32 mStackSize;
  PRInt32 mStackPos;

  PRUnichar* mText;
  PRInt32 mTextLength;
  PRInt32 mTextSize;

private:
  PRBool mLastTextCharWasCR;
};



#ifdef NS_DEBUG
void
HTMLContentSink::SinkTraceNode(PRUint32 aBit,
                               const char* aMsg,
                               const nsHTMLTag aTag,
                               PRInt32 aStackPos,
                               void* aThis)
{
  if (SINK_LOG_TEST(gSinkLogModuleInfo, aBit)) {
    nsIParserService *parserService = nsContentUtils::GetParserService();
    if (!parserService)
      return;

    NS_ConvertUTF16toUTF8 tag(parserService->HTMLIdToStringTag(aTag));
    PR_LogPrint("%s: this=%p node='%s' stackPos=%d", 
                aMsg, aThis, tag.get(), aStackPos);
  }
}
#endif

nsresult
HTMLContentSink::AddAttributes(const nsIParserNode& aNode,
                               nsIContent* aContent, PRBool aNotify,
                               PRBool aCheckIfPresent)
{
  

  PRInt32 ac = aNode.GetAttributeCount();

  if (ac == 0) {
    
    

    return NS_OK;
  }

  nsCAutoString k;
  nsHTMLTag nodeType = nsHTMLTag(aNode.GetNodeType());

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  PRInt32 i, limit, step;
  if (aCheckIfPresent) {
    i = 0;
    limit = ac;
    step = 1;
  } else {
    i = ac - 1;
    limit = -1;
    step = -1;
  }
  
  for (; i != limit; i += step) {
    
    const nsAString& key = aNode.GetKeyAt(i);
    
    
    CopyUTF16toUTF8(key, k);
    ToLowerCase(k);

    nsCOMPtr<nsIAtom> keyAtom = do_GetAtom(k);

    if (aCheckIfPresent && aContent->HasAttr(kNameSpaceID_None, keyAtom)) {
      continue;
    }

    
    static const char* kWhitespace = "\n\r\t\b";

    
    
    
    const nsAString& v =
      nsContentUtils::TrimCharsInSet(
        (nodeType == eHTMLTag_input &&
          keyAtom == nsGkAtoms::value) ?
        "" : kWhitespace, aNode.GetValueAt(i));

    if (nodeType == eHTMLTag_a && keyAtom == nsGkAtoms::name) {
      NS_ConvertUTF16toUTF8 cname(v);
      NS_ConvertUTF8toUTF16 uv(nsUnescape(cname.BeginWriting()));

      
      aContent->SetAttr(kNameSpaceID_None, keyAtom, uv, aNotify);
    } else {
      
      aContent->SetAttr(kNameSpaceID_None, keyAtom, v, aNotify);
    }
  }

  return NS_OK;
}

static void
MaybeSetForm(nsGenericHTMLElement* aContent, nsHTMLTag aNodeType,
             HTMLContentSink* aSink)
{
  nsGenericHTMLElement* form = aSink->mCurrentForm;

  if (!form || aSink->mInsideNoXXXTag) {
    return;
  }

  switch (aNodeType) {
    case eHTMLTag_button:
    case eHTMLTag_fieldset:
    case eHTMLTag_label:
    case eHTMLTag_legend:
    case eHTMLTag_object:
    case eHTMLTag_input:
    case eHTMLTag_select:
    case eHTMLTag_textarea:
      break;
    default:
      return;
  }
  
  nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(aContent));
  NS_ASSERTION(formControl,
               "nsGenericHTMLElement didn't implement nsIFormControl");
  nsCOMPtr<nsIDOMHTMLFormElement> formElement(do_QueryInterface(form));
  NS_ASSERTION(formElement,
               "nsGenericHTMLElement didn't implement nsIDOMHTMLFormElement");

  formControl->SetForm(formElement);
}




already_AddRefed<nsGenericHTMLElement>
HTMLContentSink::CreateContentObject(const nsIParserNode& aNode,
                                     nsHTMLTag aNodeType)
{
  

  nsCOMPtr<nsINodeInfo> nodeInfo;

  if (aNodeType == eHTMLTag_userdefined) {
    NS_ConvertUTF16toUTF8 tmp(aNode.GetText());
    ToLowerCase(tmp);

    nsCOMPtr<nsIAtom> name = do_GetAtom(tmp);
    nodeInfo = mNodeInfoManager->GetNodeInfo(name, nsnull, kNameSpaceID_XHTML);
  }
  else if (mNodeInfoCache[aNodeType]) {
    nodeInfo = mNodeInfoCache[aNodeType];
  }
  else {
    nsIParserService *parserService = nsContentUtils::GetParserService();
    if (!parserService)
      return nsnull;

    nsIAtom *name = parserService->HTMLIdToAtomTag(aNodeType);
    NS_ASSERTION(name, "What? Reverse mapping of id to string broken!!!");

    nodeInfo = mNodeInfoManager->GetNodeInfo(name, nsnull, kNameSpaceID_XHTML);
    NS_IF_ADDREF(mNodeInfoCache[aNodeType] = nodeInfo);
  }

  NS_ENSURE_TRUE(nodeInfo, nsnull);

  
  return CreateHTMLElement(aNodeType, nodeInfo, PR_TRUE);
}

nsresult
NS_NewHTMLElement(nsIContent** aResult, nsINodeInfo *aNodeInfo,
                  PRBool aFromParser)
{
  *aResult = nsnull;

  nsIParserService* parserService = nsContentUtils::GetParserService();
  if (!parserService)
    return NS_ERROR_OUT_OF_MEMORY;

  nsIAtom *name = aNodeInfo->NameAtom();

  NS_ASSERTION(aNodeInfo->NamespaceEquals(kNameSpaceID_XHTML), 
               "Trying to HTML elements that don't have the XHTML namespace");
  
  *aResult = CreateHTMLElement(parserService->
                                 HTMLCaseSensitiveAtomTagToId(name),
                               aNodeInfo, aFromParser).get();
  return *aResult ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

already_AddRefed<nsGenericHTMLElement>
CreateHTMLElement(PRUint32 aNodeType, nsINodeInfo *aNodeInfo,
                  PRBool aFromParser)
{
  NS_ASSERTION(aNodeType <= NS_HTML_TAG_MAX ||
               aNodeType == eHTMLTag_userdefined,
               "aNodeType is out of bounds");

  contentCreatorCallback cb = sContentCreatorCallbacks[aNodeType];

  NS_ASSERTION(cb != NS_NewHTMLNOTUSEDElement,
               "Don't know how to construct tag element!");

  nsGenericHTMLElement* result = cb(aNodeInfo, aFromParser);
  NS_IF_ADDREF(result);

  return result;
}



SinkContext::SinkContext(HTMLContentSink* aSink)
  : mSink(aSink),
    mNotifyLevel(0),
    mLastTextNodeSize(0),
    mStack(nsnull),
    mStackSize(0),
    mStackPos(0),
    mText(nsnull),
    mTextLength(0),
    mTextSize(0),
    mLastTextCharWasCR(PR_FALSE)
{
  MOZ_COUNT_CTOR(SinkContext);
}

SinkContext::~SinkContext()
{
  MOZ_COUNT_DTOR(SinkContext);

  if (mStack) {
    for (PRInt32 i = 0; i < mStackPos; i++) {
      NS_RELEASE(mStack[i].mContent);
    }
    delete [] mStack;
  }

  delete [] mText;
}

nsresult
SinkContext::Begin(nsHTMLTag aNodeType,
                   nsGenericHTMLElement* aRoot,
                   PRUint32 aNumFlushed,
                   PRInt32 aInsertionPoint)
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
  mTextLength = 0;

  return NS_OK;
}

PRBool
SinkContext::IsCurrentContainer(nsHTMLTag aTag)
{
  if (aTag == mStack[mStackPos - 1].mType) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool
SinkContext::IsAncestorContainer(nsHTMLTag aTag)
{
  PRInt32 stackPos = mStackPos - 1;

  while (stackPos >= 0) {
    if (aTag == mStack[stackPos].mType) {
      return PR_TRUE;
    }
    stackPos--;
  }

  return PR_FALSE;
}

void
SinkContext::DidAddContent(nsIContent* aContent)
{
  if ((mStackPos == 2) && (mSink->mBody == mStack[1].mContent ||
                           mSink->mFrameset == mStack[1].mContent)) {
    
    mNotifyLevel = 0;
  }

  
  
  
  if (0 < mStackPos &&
      mStack[mStackPos - 1].mInsertionPoint != -1 &&
      mStack[mStackPos - 1].mNumFlushed <
      mStack[mStackPos - 1].mContent->GetChildCount()) {
    nsIContent* parent = mStack[mStackPos - 1].mContent;

#ifdef NS_DEBUG
    
    nsIParserService *parserService = nsContentUtils::GetParserService();
    if (parserService) {
      nsHTMLTag tag = nsHTMLTag(mStack[mStackPos - 1].mType);
      NS_ConvertUTF16toUTF8 str(parserService->HTMLIdToStringTag(tag));

      SINK_TRACE(gSinkLogModuleInfo, SINK_TRACE_REFLOW,
                 ("SinkContext::DidAddContent: Insertion notification for "
                  "parent=%s at position=%d and stackPos=%d",
                  str.get(), mStack[mStackPos - 1].mInsertionPoint - 1,
                  mStackPos - 1));
    }
#endif

    PRInt32 childIndex = mStack[mStackPos - 1].mInsertionPoint - 1;
    NS_ASSERTION(parent->GetChildAt(childIndex) == aContent,
                 "Flushing the wrong child.");
    mSink->NotifyInsert(parent, aContent, childIndex);
    mStack[mStackPos - 1].mNumFlushed = parent->GetChildCount();
  } else if (mSink->IsTimeToNotify()) {
    SINK_TRACE(gSinkLogModuleInfo, SINK_TRACE_REFLOW,
               ("SinkContext::DidAddContent: Notification as a result of the "
                "interval expiring; backoff count: %d", mSink->mBackoffCount));
    FlushTags();
  }
}

nsresult
SinkContext::OpenContainer(const nsIParserNode& aNode)
{
  FlushTextAndRelease();

  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                  "SinkContext::OpenContainer", 
                  nsHTMLTag(aNode.GetNodeType()), 
                  mStackPos, 
                  mSink);

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

  
  nsHTMLTag nodeType = nsHTMLTag(aNode.GetNodeType());
  nsGenericHTMLElement* content =
    mSink->CreateContentObject(aNode, nodeType).get();
  if (!content) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mStack[mStackPos].mType = nodeType;
  mStack[mStackPos].mContent = content;
  mStack[mStackPos].mNumFlushed = 0;
  mStack[mStackPos].mInsertionPoint = -1;
  ++mStackPos;

  
  if (nodeType == eHTMLTag_style) {
    nsCOMPtr<nsIStyleSheetLinkingElement> ssle = do_QueryInterface(content);
    NS_ASSERTION(ssle, "Style content isn't a style sheet?");
    ssle->SetLineNumber(aNode.GetSourceLineNumber());

    
    
    if (!mSink->mInsideNoXXXTag) {
      ssle->InitStyleLinkElement(PR_FALSE);
    }
    else {
      
      ssle->InitStyleLinkElement(PR_TRUE);
    }

    ssle->SetEnableUpdates(PR_FALSE);
  }

  
  
  
  
  switch (nodeType) {
    
    case eHTMLTag_a:
    case eHTMLTag_map:

    
    case eHTMLTag_script:
    
    
    case eHTMLTag_form:

    
    case eHTMLTag_object:

    
    case eHTMLTag_table:
    case eHTMLTag_thead:
    case eHTMLTag_tbody:
    case eHTMLTag_tfoot:
    case eHTMLTag_tr:
    case eHTMLTag_td:
    case eHTMLTag_th:
      mSink->AddBaseTagInfo(content);

      break;
    default:
      break;    
  }
  
  rv = mSink->AddAttributes(aNode, content);
  MaybeSetForm(content, nodeType, mSink);

  mStack[mStackPos - 2].Add(content);

  NS_ENSURE_SUCCESS(rv, rv);

  if (mSink->IsMonolithicContainer(nodeType)) {
    mSink->mInMonolithicContainer++;
  }

  
  switch (nodeType) {
    case eHTMLTag_form:
      mSink->mCurrentForm = content;
      break;

    case eHTMLTag_frameset:
      if (!mSink->mFrameset && mSink->mFramesEnabled) {
        mSink->mFrameset = content;
      }
      break;

    case eHTMLTag_noembed:
    case eHTMLTag_noframes:
      mSink->mInsideNoXXXTag++;
      break;

    case eHTMLTag_iframe:
      mSink->mNumOpenIFRAMES++;
      break;

    case eHTMLTag_script:
      {
        nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(content);
        NS_ASSERTION(sele, "Script content isn't a script element?");
        sele->SetScriptLineNumber(aNode.GetSourceLineNumber());
      }
      break;

    case eHTMLTag_button:
      content->DoneCreatingElement();
      break;
      
    default:
      break;
  }

  return NS_OK;
}

PRBool
SinkContext::HaveNotifiedForCurrentContent() const
{
  if (0 < mStackPos) {
    nsIContent* parent = mStack[mStackPos - 1].mContent;
    return mStack[mStackPos-1].mNumFlushed == parent->GetChildCount();
  }

  return PR_TRUE;
}

nsIContent *
SinkContext::Node::Add(nsIContent *child)
{
  NS_ASSERTION(mContent, "No parent to insert/append into!");
  if (mInsertionPoint != -1) {
    NS_ASSERTION(mNumFlushed == mContent->GetChildCount(),
                 "Inserting multiple children without flushing.");
    mContent->InsertChildAt(child, mInsertionPoint++, PR_FALSE);
  } else {
    mContent->AppendChildTo(child, PR_FALSE);
  }
  return child;
}

nsresult
SinkContext::CloseContainer(const nsHTMLTag aTag, PRBool aMalformed)
{
  nsresult result = NS_OK;

  
  
  FlushTextAndRelease();

  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                  "SinkContext::CloseContainer", 
                  aTag, mStackPos - 1, mSink);

  NS_ASSERTION(mStackPos > 0,
               "stack out of bounds. wrong context probably!");

  if (mStackPos <= 0) {
    return NS_OK; 
  }

  --mStackPos;
  nsHTMLTag nodeType = mStack[mStackPos].mType;

  NS_ASSERTION(nodeType == eHTMLTag_form || nodeType == aTag,
               "Tag mismatch.  Closing tag on wrong context or something?");

  nsGenericHTMLElement* content = mStack[mStackPos].mContent;

  content->Compact();

  
  
  
  if (mNotifyLevel >= mStackPos) {
    
    

    if (mStack[mStackPos].mNumFlushed < content->GetChildCount()) {
#ifdef NS_DEBUG
      {
        
        const char *tagStr;
        mStack[mStackPos].mContent->Tag()->GetUTF8String(&tagStr);

        SINK_TRACE(gSinkLogModuleInfo, SINK_TRACE_REFLOW,
                   ("SinkContext::CloseContainer: reflow on notifyImmediate "
                    "tag=%s newIndex=%d stackPos=%d",
                    tagStr,
                    mStack[mStackPos].mNumFlushed, mStackPos));
      }
#endif
      mSink->NotifyAppend(content, mStack[mStackPos].mNumFlushed);
      mStack[mStackPos].mNumFlushed = content->GetChildCount();
    }

    
    mNotifyLevel = mStackPos - 1;
  }

  if (mSink->IsMonolithicContainer(nodeType)) {
    --mSink->mInMonolithicContainer;
  }

  DidAddContent(content);

  
  switch (nodeType) {
  case eHTMLTag_noembed:
  case eHTMLTag_noframes:
    
    NS_ASSERTION((mSink->mInsideNoXXXTag > 0), "mInsideNoXXXTag underflow");
    if (mSink->mInsideNoXXXTag > 0) {
      mSink->mInsideNoXXXTag--;
    }

    break;
  case eHTMLTag_form:
    {
      mSink->mFormOnStack = PR_FALSE;
      
      
      
      
      
      if (aTag != nodeType) {
        result = CloseContainer(aTag, PR_FALSE);
      }
    }

    break;
  case eHTMLTag_iframe:
    mSink->mNumOpenIFRAMES--;

    break;

#ifdef MOZ_MEDIA
  case eHTMLTag_video:
  case eHTMLTag_audio:
#endif
  case eHTMLTag_select:
  case eHTMLTag_textarea:
  case eHTMLTag_object:
  case eHTMLTag_applet:
  case eHTMLTag_title:
    content->DoneAddingChildren(HaveNotifiedForCurrentContent());
    break;

  case eHTMLTag_script:
    result = mSink->ProcessSCRIPTEndTag(content,
                                        aMalformed);
    break;

  case eHTMLTag_style:
    result = mSink->ProcessSTYLEEndTag(content);
    break;

  default:
    break;
  }

  NS_IF_RELEASE(content);

#ifdef DEBUG
  if (SINK_LOG_TEST(gSinkLogModuleInfo, SINK_ALWAYS_REFLOW)) {
    mSink->ForceReflow();
  }
#endif

  return result;
}

nsresult
SinkContext::AddLeaf(const nsIParserNode& aNode)
{
  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                  "SinkContext::AddLeaf", 
                  nsHTMLTag(aNode.GetNodeType()), 
                  mStackPos, mSink);

  nsresult rv = NS_OK;

  switch (aNode.GetTokenType()) {
  case eToken_start:
    {
      FlushTextAndRelease();

      
      nsHTMLTag nodeType = nsHTMLTag(aNode.GetNodeType());
      nsRefPtr<nsGenericHTMLElement> content =
        mSink->CreateContentObject(aNode, nodeType);
      NS_ENSURE_TRUE(content, NS_ERROR_OUT_OF_MEMORY);

      
      
      
      
      switch (nodeType) {
      case eHTMLTag_area:
      case eHTMLTag_meta:
      case eHTMLTag_img:
      case eHTMLTag_frame:
      case eHTMLTag_input:
      case eHTMLTag_embed:
        mSink->AddBaseTagInfo(content);
        break;

      
      case eHTMLTag_form:
        mSink->AddBaseTagInfo(content);
        mSink->mCurrentForm = content;

        break;
      default:
        break;
      }

      rv = mSink->AddAttributes(aNode, content);

      NS_ENSURE_SUCCESS(rv, rv);

      MaybeSetForm(content, nodeType, mSink);

      
      AddLeaf(content);

      
      switch (nodeType) {
      case eHTMLTag_base:
        if (!mSink->mInsideNoXXXTag) {
          mSink->ProcessBASEElement(content);
        }
        break;

      case eHTMLTag_meta:
        
        
        
        if (!mSink->mInsideNoXXXTag && !mSink->mFrameset) {
          rv = mSink->ProcessMETATag(content);
        }
        break;

      case eHTMLTag_input:
        content->DoneCreatingElement();

        break;

      default:
        break;
      }
    }
    break;

  case eToken_text:
  case eToken_whitespace:
  case eToken_newline:
    rv = AddText(aNode.GetText());

    break;
  case eToken_entity:
    {
      nsAutoString tmp;
      PRInt32 unicode = aNode.TranslateToUnicodeStr(tmp);
      if (unicode < 0) {
        rv = AddText(aNode.GetText());
      } else {
        
        if (!tmp.IsEmpty()) {
          if (tmp.CharAt(0) == '\r') {
            tmp.Assign((PRUnichar)'\n');
          }
          rv = AddText(tmp);
        }
      }
    }

    break;
  default:
    break;
  }

  return rv;
}

nsresult
SinkContext::AddLeaf(nsIContent* aContent)
{
  NS_ASSERTION(mStackPos > 0, "leaf w/o container");
  if (mStackPos <= 0) {
    return NS_ERROR_FAILURE;
  }
  
  DidAddContent(mStack[mStackPos - 1].Add(aContent));

#ifdef DEBUG
  if (SINK_LOG_TEST(gSinkLogModuleInfo, SINK_ALWAYS_REFLOW)) {
    mSink->ForceReflow();
  }
#endif

  return NS_OK;
}

nsresult
SinkContext::AddComment(const nsIParserNode& aNode)
{
  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                  "SinkContext::AddLeaf", 
                  nsHTMLTag(aNode.GetNodeType()), 
                  mStackPos, mSink);
  FlushTextAndRelease();

  if (!mSink) {
    return NS_ERROR_UNEXPECTED;
  }
  
  nsCOMPtr<nsIContent> comment;
  nsresult rv = NS_NewCommentNode(getter_AddRefs(comment),
                                  mSink->mNodeInfoManager);
  NS_ENSURE_SUCCESS(rv, rv);

  comment->SetText(aNode.GetText(), PR_FALSE);

  NS_ASSERTION(mStackPos > 0, "stack out of bounds");
  if (mStackPos <= 0) {
    return NS_ERROR_FAILURE;
  }

  {
    Node &parentNode = mStack[mStackPos - 1];
    nsGenericHTMLElement *parent = parentNode.mContent;
    if (!mSink->mBody && !mSink->mFrameset && mSink->mHead)
      
      
      parentNode.mContent = mSink->mHead;
    DidAddContent(parentNode.Add(comment));
    parentNode.mContent = parent;
  }

#ifdef DEBUG
  if (SINK_LOG_TEST(gSinkLogModuleInfo, SINK_ALWAYS_REFLOW)) {
    mSink->ForceReflow();
  }
#endif

  return rv;
}

nsresult
SinkContext::End()
{
  for (PRInt32 i = 0; i < mStackPos; i++) {
    NS_RELEASE(mStack[i].mContent);
  }

  mStackPos = 0;
  mTextLength = 0;

  return NS_OK;
}

nsresult
SinkContext::GrowStack()
{
  PRInt32 newSize = mStackSize * 2;
  if (newSize == 0) {
    newSize = 32;
  }

  Node* stack = new Node[newSize];
  if (!stack) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (mStackPos != 0) {
    memcpy(stack, mStack, sizeof(Node) * mStackPos);
    delete [] mStack;
  }

  mStack = stack;
  mStackSize = newSize;

  return NS_OK;
}









nsresult
SinkContext::AddText(const nsAString& aText)
{
  PRInt32 addLen = aText.Length();
  if (addLen == 0) {
    return NS_OK;
  }
  
  
  if (mTextSize == 0) {
    mText = new PRUnichar[4096];
    if (!mText) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mTextSize = 4096;
  }

  
  PRInt32 offset = 0;

  while (addLen != 0) {
    PRInt32 amount = mTextSize - mTextLength;

    if (amount > addLen) {
      amount = addLen;
    }

    if (amount == 0) {
      
      nsresult rv = FlushText();
      if (NS_FAILED(rv)) {
        return rv;
      }

      
      
      
      continue;
    }

    mTextLength +=
      nsContentUtils::CopyNewlineNormalizedUnicodeTo(aText, offset,
                                                     &mText[mTextLength],
                                                     amount,
                                                     mLastTextCharWasCR);
    offset += amount;
    addLen -= amount;
  }

  return NS_OK;
}










nsresult
SinkContext::FlushTags()
{
  mSink->mDeferredFlushTags = PR_FALSE;
  PRBool oldBeganUpdate = mSink->mBeganUpdate;
  PRUint32 oldUpdates = mSink->mUpdatesInNotification;

  ++(mSink->mInNotification);
  mSink->mUpdatesInNotification = 0;
  {
    
    mozAutoDocUpdate updateBatch(mSink->mDocument, UPDATE_CONTENT_MODEL,
                                 PR_TRUE);
    mSink->mBeganUpdate = PR_TRUE;

    
    FlushText();

    
    
    

    
    
    
    PRInt32 stackPos = 0;
    PRBool flushed = PR_FALSE;
    PRUint32 childCount;
    nsGenericHTMLElement* content;

    while (stackPos < mStackPos) {
      content = mStack[stackPos].mContent;
      childCount = content->GetChildCount();

      if (!flushed && (mStack[stackPos].mNumFlushed < childCount)) {
#ifdef NS_DEBUG
        {
          
          const char* tagStr;
          mStack[stackPos].mContent->Tag()->GetUTF8String(&tagStr);

          SINK_TRACE(gSinkLogModuleInfo, SINK_TRACE_REFLOW,
                     ("SinkContext::FlushTags: tag=%s from newindex=%d at "
                      "stackPos=%d", tagStr,
                      mStack[stackPos].mNumFlushed, stackPos));
        }
#endif
        if (mStack[stackPos].mInsertionPoint != -1) {
          
          
          

          PRInt32 childIndex = mStack[stackPos].mInsertionPoint - 1;
          nsIContent* child = content->GetChildAt(childIndex);
          
          NS_ASSERTION(!(mStackPos > (stackPos + 1)) ||
                       (child == mStack[stackPos + 1].mContent),
                       "Flushing the wrong child.");
          mSink->NotifyInsert(content, child, childIndex);
        } else {
          mSink->NotifyAppend(content, mStack[stackPos].mNumFlushed);
        }

        flushed = PR_TRUE;
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
  
  
  
  
  
  PRInt32 stackPos = mStackPos - 1;
  while (stackPos >= 0) {
    Node & node = mStack[stackPos];
    node.mNumFlushed = node.mContent->GetChildCount();

    stackPos--;
  }

  mNotifyLevel = mStackPos - 1;
}





nsresult
SinkContext::FlushText(PRBool* aDidFlush, PRBool aReleaseLast)
{
  nsresult rv = NS_OK;
  PRBool didFlush = PR_FALSE;

  if (mTextLength != 0) {
    if (mLastTextNode) {
      if ((mLastTextNodeSize + mTextLength) > mSink->mMaxTextRun) {
        mLastTextNodeSize = 0;
        mLastTextNode = nsnull;
        FlushText(aDidFlush, aReleaseLast);
      } else {
        PRBool notify = HaveNotifiedForCurrentContent();
        
        
        
        if (notify) {
          ++mSink->mInNotification;
        }
        rv = mLastTextNode->AppendText(mText, mTextLength, notify);
        if (notify) {
          --mSink->mInNotification;
        }

        mLastTextNodeSize += mTextLength;
        mTextLength = 0;
        didFlush = PR_TRUE;
      }
    } else {
      nsCOMPtr<nsIContent> textContent;
      rv = NS_NewTextNode(getter_AddRefs(textContent),
                          mSink->mNodeInfoManager);
      NS_ENSURE_SUCCESS(rv, rv);

      mLastTextNode = textContent;

      
      mLastTextNode->SetText(mText, mTextLength, PR_FALSE);

      
      mLastTextNodeSize += mTextLength;
      mTextLength = 0;

      rv = AddLeaf(mLastTextNode);
      NS_ENSURE_SUCCESS(rv, rv);

      didFlush = PR_TRUE;
    }
  }

  if (aDidFlush) {
    *aDidFlush = didFlush;
  }

  if (aReleaseLast) {
    mLastTextNodeSize = 0;
    mLastTextNode = nsnull;
    mLastTextCharWasCR = PR_FALSE;
  }

#ifdef DEBUG
  if (didFlush &&
      SINK_LOG_TEST(gSinkLogModuleInfo, SINK_ALWAYS_REFLOW)) {
    mSink->ForceReflow();
  }
#endif

  return rv;
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

  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = it->Init(aDoc, aURI, aContainer, aChannel);

  NS_ENSURE_SUCCESS(rv, rv);

  *aResult = it;
  NS_ADDREF(*aResult);

  return NS_OK;
}

HTMLContentSink::HTMLContentSink()
{
  


#ifdef NS_DEBUG
  if (!gSinkLogModuleInfo) {
    gSinkLogModuleInfo = PR_NewLogModule("htmlcontentsink");
  }
#endif
}

HTMLContentSink::~HTMLContentSink()
{
  NS_IF_RELEASE(mHead);
  NS_IF_RELEASE(mBody);
  NS_IF_RELEASE(mRoot);

  NS_IF_RELEASE(mHTMLDocument);

  if (mNotificationTimer) {
    mNotificationTimer->Cancel();
  }

  PRInt32 numContexts = mContextStack.Length();

  if (mCurrentContext == mHeadContext && numContexts > 0) {
    
    mContextStack.RemoveElementAt(--numContexts);
  }

  PRInt32 i;
  for (i = 0; i < numContexts; i++) {
    SinkContext* sc = mContextStack.ElementAt(i);
    if (sc) {
      sc->End();
      if (sc == mCurrentContext) {
        mCurrentContext = nsnull;
      }

      delete sc;
    }
  }

  if (mCurrentContext == mHeadContext) {
    mCurrentContext = nsnull;
  }

  delete mCurrentContext;

  delete mHeadContext;

  for (i = 0; PRUint32(i) < NS_ARRAY_LENGTH(mNodeInfoCache); ++i) {
    NS_IF_RELEASE(mNodeInfoCache[i]);
  }
}

#if DEBUG
NS_IMPL_ISUPPORTS_INHERITED3(HTMLContentSink,
                             nsContentSink,
                             nsIContentSink,
                             nsIHTMLContentSink,
                             nsIDebugDumpContent)
#else
NS_IMPL_ISUPPORTS_INHERITED2(HTMLContentSink,
                             nsContentSink,
                             nsIContentSink,
                             nsIHTMLContentSink)
#endif

static PRBool
IsScriptEnabled(nsIDocument *aDoc, nsIDocShell *aContainer)
{
  NS_ENSURE_TRUE(aDoc && aContainer, PR_TRUE);

  nsCOMPtr<nsIScriptGlobalObject> globalObject = aDoc->GetScriptGlobalObject();

  
  
  if (!globalObject) {
    nsCOMPtr<nsIScriptGlobalObjectOwner> owner = do_GetInterface(aContainer);
    NS_ENSURE_TRUE(owner, PR_TRUE);

    globalObject = owner->GetScriptGlobalObject();
    NS_ENSURE_TRUE(globalObject, PR_TRUE);
  }

  nsIScriptContext *scriptContext = globalObject->GetContext();
  NS_ENSURE_TRUE(scriptContext, PR_TRUE);

  JSContext* cx = (JSContext *) scriptContext->GetNativeContext();
  NS_ENSURE_TRUE(cx, PR_TRUE);

  PRBool enabled = PR_TRUE;
  nsContentUtils::GetSecurityManager()->
    CanExecuteScripts(cx, aDoc->NodePrincipal(), &enabled);
  return enabled;
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
  mIsDocumentObserver = PR_TRUE;
  CallQueryInterface(aDoc, &mHTMLDocument);

  mObservers = nsnull;
  nsIParserService* service = nsContentUtils::GetParserService();
  if (!service) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  service->GetTopicObservers(NS_LITERAL_STRING("text/html"),
                             getter_AddRefs(mObservers));

  NS_ASSERTION(mDocShell, "oops no docshell!");

  
  if (mDocShell) {
    PRBool subFramesEnabled = PR_TRUE;
    mDocShell->GetAllowSubframes(&subFramesEnabled);
    if (subFramesEnabled) {
      mFramesEnabled = PR_TRUE;
    }
  }

  
  if (IsScriptEnabled(aDoc, mDocShell)) {
    mScriptEnabled = PR_TRUE;
  }


  
  
  mMaxTextRun = nsContentUtils::GetIntPref("content.maxtextrun", 8191);

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::html, nsnull,
                                           kNameSpaceID_XHTML);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  
  nsIContent *doc_root = mDocument->GetRootContent();

  if (doc_root) {
    
    

    NS_ADDREF(mRoot = static_cast<nsGenericHTMLElement*>(doc_root));
  } else {
    mRoot = NS_NewHTMLHtmlElement(nodeInfo);
    if (!mRoot) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_ADDREF(mRoot);

    NS_ASSERTION(mDocument->GetChildCount() == 0,
                 "Document should have no kids here!");
    rv = mDocument->AppendChildTo(mRoot, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::head,
                                           nsnull, kNameSpaceID_XHTML);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  mHead = NS_NewHTMLHeadElement(nodeInfo);
  if (NS_FAILED(rv)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(mHead);

  mRoot->AppendChildTo(mHead, PR_FALSE);

  mCurrentContext = new SinkContext(this);
  NS_ENSURE_TRUE(mCurrentContext, NS_ERROR_OUT_OF_MEMORY);
  mCurrentContext->Begin(eHTMLTag_html, mRoot, 0, -1);
  mContextStack.AppendElement(mCurrentContext);

#ifdef NS_DEBUG
  nsCAutoString spec;
  (void)aURI->GetSpec(spec);
  SINK_TRACE(gSinkLogModuleInfo, SINK_TRACE_CALLS,
             ("HTMLContentSink::Init: this=%p url='%s'",
              this, spec.get()));
#endif
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
HTMLContentSink::DidBuildModel(PRBool aTerminated)
{
  DidBuildModelImpl(aTerminated);

  
  if (mBody || mFrameset) {
    SINK_TRACE(gSinkLogModuleInfo, SINK_TRACE_REFLOW,
               ("HTMLContentSink::DidBuildModel: layout final content"));
    mCurrentContext->FlushTags();
  } else if (!mLayoutStarted) {
    
    
    SINK_TRACE(gSinkLogModuleInfo, SINK_TRACE_REFLOW,
               ("HTMLContentSink::DidBuildModel: forcing reflow on empty "
                "document"));

    
    
    
    
    PRBool bDestroying = PR_TRUE;
    if (mDocShell) {
      mDocShell->IsBeingDestroyed(&bDestroying);
    }

    if (!bDestroying) {
      StartLayout(PR_FALSE);
    }
  }

  ScrollToRef();

  mDocument->ScriptLoader()->RemoveObserver(this);

  
  

  
  
  
  mDocument->RemoveObserver(this);
  mIsDocumentObserver = PR_FALSE;
  
  mDocument->EndLoad();

  DropParserAndPerfHint();

  return NS_OK;
}

NS_IMETHODIMP
HTMLContentSink::SetParser(nsIParser* aParser)
{
  NS_PRECONDITION(aParser, "Should have a parser here!");
  mParser = aParser;
  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
HTMLContentSink::IsFormOnStack()
{
  return mFormOnStack;
}

NS_IMETHODIMP
HTMLContentSink::BeginContext(PRInt32 aPosition)
{
  NS_PRECONDITION(aPosition > -1, "out of bounds");

  
  SinkContext* sc = new SinkContext(this);
  if (!sc) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!mCurrentContext) {
    NS_ERROR("Non-existing context");

    return NS_ERROR_FAILURE;
  }

  
  
  mCurrentContext->FlushTags();

  
  if (mCurrentContext->mStackPos <= aPosition) {
    NS_ERROR("Out of bounds position");
    return NS_ERROR_FAILURE;
  }

  PRInt32 insertionPoint = -1;
  nsHTMLTag nodeType      = mCurrentContext->mStack[aPosition].mType;
  nsGenericHTMLElement* content = mCurrentContext->mStack[aPosition].mContent;

  
  
  
  if (aPosition < (mCurrentContext->mStackPos - 1)) {
    insertionPoint = content->GetChildCount() - 1;
  }

  sc->Begin(nodeType,
            content,
            mCurrentContext->mStack[aPosition].mNumFlushed,
            insertionPoint);
  NS_ADDREF(sc->mSink);

  mContextStack.AppendElement(mCurrentContext);
  mCurrentContext = sc;
  return NS_OK;
}

NS_IMETHODIMP
HTMLContentSink::EndContext(PRInt32 aPosition)
{
  NS_PRECONDITION(mCurrentContext && aPosition > -1, "non-existing context");

  PRUint32 n = mContextStack.Length() - 1;
  SinkContext* sc = mContextStack.ElementAt(n);

  const SinkContext::Node &bottom = mCurrentContext->mStack[0];
  
  NS_ASSERTION(sc->mStack[aPosition].mType == bottom.mType,
               "ending a wrong context");

  mCurrentContext->FlushTextAndRelease();
  
  NS_ASSERTION(bottom.mContent->GetChildCount() == bottom.mNumFlushed,
               "Node at base of context stack not fully flushed.");

  
  
  
  
  
  mCurrentContext->FlushTags();

  sc->mStack[aPosition].mNumFlushed = bottom.mNumFlushed;

  for (PRInt32 i = 0; i<mCurrentContext->mStackPos; i++) {
    NS_IF_RELEASE(mCurrentContext->mStack[i].mContent);
  }

  delete [] mCurrentContext->mStack;

  mCurrentContext->mStack      = nsnull;
  mCurrentContext->mStackPos   = 0;
  mCurrentContext->mStackSize  = 0;

  delete [] mCurrentContext->mText;

  mCurrentContext->mText       = nsnull;
  mCurrentContext->mTextLength = 0;
  mCurrentContext->mTextSize   = 0;

  NS_IF_RELEASE(mCurrentContext->mSink);

  delete mCurrentContext;

  mCurrentContext = sc;
  mContextStack.RemoveElementAt(n);
  return NS_OK;
}

nsresult
HTMLContentSink::CloseHTML()
{
  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                 "HTMLContentSink::CloseHTML", 
                 eHTMLTag_html, 0, this);

  if (mHeadContext) {
    if (mCurrentContext == mHeadContext) {
      PRUint32 numContexts = mContextStack.Length();

      
      mCurrentContext = mContextStack.ElementAt(--numContexts);
      mContextStack.RemoveElementAt(numContexts);
    }

    NS_ASSERTION(mHeadContext->mTextLength == 0, "Losing text");

    mHeadContext->End();

    delete mHeadContext;
    mHeadContext = nsnull;
  }

  return NS_OK;
}

nsresult
HTMLContentSink::OpenHead()
{
  nsresult rv = OpenHeadContext();
  return rv;
}

nsresult
HTMLContentSink::OpenBody(const nsIParserNode& aNode)
{
  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                  "HTMLContentSink::OpenBody", 
                  eHTMLTag_body,
                  mCurrentContext->mStackPos, 
                  this);

  CloseHeadContext();  

  
  if (mBody) {
    AddAttributes(aNode, mBody, PR_TRUE, PR_TRUE);
    return NS_OK;
  }

  nsresult rv = mCurrentContext->OpenContainer(aNode);

  if (NS_FAILED(rv)) {
    return rv;
  }

  mBody = mCurrentContext->mStack[mCurrentContext->mStackPos - 1].mContent;

  NS_ADDREF(mBody);

  if (mCurrentContext->mStackPos > 1) {
    PRInt32 parentIndex    = mCurrentContext->mStackPos - 2;
    nsGenericHTMLElement *parent = mCurrentContext->mStack[parentIndex].mContent;
    PRInt32 numFlushed     = mCurrentContext->mStack[parentIndex].mNumFlushed;
    PRInt32 childCount = parent->GetChildCount();
    NS_ASSERTION(numFlushed < childCount, "Already notified on the body?");
    
    PRInt32 insertionPoint =
      mCurrentContext->mStack[parentIndex].mInsertionPoint;

    
    
    

    PRUint32 oldUpdates = mUpdatesInNotification;
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

  StartLayout(PR_FALSE);

  return NS_OK;
}

nsresult
HTMLContentSink::CloseBody()
{
  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                  "HTMLContentSink::CloseBody", 
                  eHTMLTag_body,
                  mCurrentContext->mStackPos - 1, 
                  this);

  PRBool didFlush;
  nsresult rv = mCurrentContext->FlushTextAndRelease(&didFlush);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  SINK_TRACE(gSinkLogModuleInfo, SINK_TRACE_REFLOW,
             ("HTMLContentSink::CloseBody: layout final body content"));

  mCurrentContext->FlushTags();
  mCurrentContext->CloseContainer(eHTMLTag_body, PR_FALSE);

  return NS_OK;
}

nsresult
HTMLContentSink::OpenForm(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;

  mCurrentContext->FlushTextAndRelease();

  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                  "HTMLContentSink::OpenForm", 
                  eHTMLTag_form,
                  mCurrentContext->mStackPos, 
                  this);

  
  
  mCurrentForm = nsnull;

  
  
  if (mCurrentContext->IsCurrentContainer(eHTMLTag_table) ||
      mCurrentContext->IsCurrentContainer(eHTMLTag_tbody) ||
      mCurrentContext->IsCurrentContainer(eHTMLTag_thead) ||
      mCurrentContext->IsCurrentContainer(eHTMLTag_tfoot) ||
      mCurrentContext->IsCurrentContainer(eHTMLTag_tr) ||
      mCurrentContext->IsCurrentContainer(eHTMLTag_col) ||
      mCurrentContext->IsCurrentContainer(eHTMLTag_colgroup)) {
    result = mCurrentContext->AddLeaf(aNode);
  } else {
    mFormOnStack = PR_TRUE;
    
    result = mCurrentContext->OpenContainer(aNode);
  }

  return result;
}

nsresult
HTMLContentSink::CloseForm()
{
  nsresult result = NS_OK;

  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                  "HTMLContentSink::CloseForm",
                  eHTMLTag_form,
                  mCurrentContext->mStackPos - 1, 
                  this);

  if (mCurrentForm) {
    
    if (mCurrentContext->IsCurrentContainer(eHTMLTag_form)) {
      result = mCurrentContext->CloseContainer(eHTMLTag_form, PR_FALSE);
      mFormOnStack = PR_FALSE;
    }

    mCurrentForm = nsnull;
  }

  return result;
}

nsresult
HTMLContentSink::OpenFrameset(const nsIParserNode& aNode)
{
  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                  "HTMLContentSink::OpenFrameset", 
                  eHTMLTag_frameset,
                  mCurrentContext->mStackPos, 
                  this);

  CloseHeadContext(); 

  
  nsGenericHTMLElement* oldFrameset = mFrameset;
  nsresult rv = mCurrentContext->OpenContainer(aNode);
  PRBool isFirstFrameset = NS_SUCCEEDED(rv) && mFrameset != oldFrameset;

  if (isFirstFrameset && mCurrentContext->mStackPos > 1) {
    NS_ASSERTION(mFrameset, "Must have frameset!");
    
    
    PRInt32 parentIndex    = mCurrentContext->mStackPos - 2;
    nsGenericHTMLElement *parent = mCurrentContext->mStack[parentIndex].mContent;
    PRInt32 numFlushed     = mCurrentContext->mStack[parentIndex].mNumFlushed;
    PRInt32 childCount = parent->GetChildCount();
    NS_ASSERTION(numFlushed < childCount, "Already notified on the frameset?");

    PRInt32 insertionPoint =
      mCurrentContext->mStack[parentIndex].mInsertionPoint;

    
    
    

    PRUint32 oldUpdates = mUpdatesInNotification;
    mUpdatesInNotification = 0;
    if (insertionPoint != -1) {
      NotifyInsert(parent, mFrameset, insertionPoint - 1);
    } else {
      NotifyAppend(parent, numFlushed);
    }
    mCurrentContext->mStack[parentIndex].mNumFlushed = childCount;
    if (mUpdatesInNotification > 1) {
      UpdateChildCounts();
    }
    mUpdatesInNotification = oldUpdates;
  }
  
  return rv;
}

nsresult
HTMLContentSink::CloseFrameset()
{
  SINK_TRACE_NODE(SINK_TRACE_CALLS,
                   "HTMLContentSink::CloseFrameset", 
                   eHTMLTag_frameset,
                   mCurrentContext->mStackPos - 1,
                   this);

  SinkContext* sc = mCurrentContext;
  nsGenericHTMLElement* fs = sc->mStack[sc->mStackPos - 1].mContent;
  PRBool done = fs == mFrameset;

  nsresult rv;
  if (done) {
    PRBool didFlush;
    rv = sc->FlushTextAndRelease(&didFlush);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    SINK_TRACE(gSinkLogModuleInfo, SINK_TRACE_REFLOW,
               ("HTMLContentSink::CloseFrameset: layout final content"));

    sc->FlushTags();
  }

  rv = sc->CloseContainer(eHTMLTag_frameset, PR_FALSE);    

  if (done && mFramesEnabled) {
    StartLayout(PR_FALSE);
  }

  return rv;
}

NS_IMETHODIMP
HTMLContentSink::IsEnabled(PRInt32 aTag, PRBool* aReturn)
{
  nsHTMLTag theHTMLTag = nsHTMLTag(aTag);

  if (theHTMLTag == eHTMLTag_script) {
    *aReturn = mScriptEnabled;
  } else if (theHTMLTag == eHTMLTag_frameset) {
    *aReturn = mFramesEnabled;
  } else {
    *aReturn = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLContentSink::OpenContainer(const nsIParserNode& aNode)
{
  nsresult rv = NS_OK;

  switch (aNode.GetNodeType()) {
    case eHTMLTag_frameset:
      rv = OpenFrameset(aNode);
      break;
    case eHTMLTag_head:
      rv = OpenHeadContext();
      if (NS_SUCCEEDED(rv)) {
        rv = AddAttributes(aNode, mHead, PR_TRUE, mHaveSeenHead);
        mHaveSeenHead = PR_TRUE;
      }
      break;
    case eHTMLTag_body:
      rv = OpenBody(aNode);
      break;
    case eHTMLTag_html:
      if (mRoot) {
        
        
        AddAttributes(aNode, mRoot, PR_TRUE, mNotifiedRootInsertion);
        if (!mNotifiedRootInsertion) {
          NotifyRootInsertion();
        }
        ProcessOfflineManifest(mRoot);
      }
      break;
    case eHTMLTag_form:
      rv = OpenForm(aNode);
      break;
    default:
      rv = mCurrentContext->OpenContainer(aNode);
      break;
  }

  return rv;
}

NS_IMETHODIMP
HTMLContentSink::CloseContainer(const eHTMLTags aTag)
{
  nsresult rv = NS_OK;

  switch (aTag) {
    case eHTMLTag_frameset:
      rv = CloseFrameset();
      break;
    case eHTMLTag_head:
      CloseHeadContext();
      break;
    case eHTMLTag_body:
      rv = CloseBody();
      break;
    case eHTMLTag_html:
      rv = CloseHTML();
      break;
    case eHTMLTag_form:
      rv = CloseForm();
      break;
    default:
      rv = mCurrentContext->CloseContainer(aTag, PR_FALSE);
      break;
  }

  return rv;
}

NS_IMETHODIMP
HTMLContentSink::CloseMalformedContainer(const eHTMLTags aTag)
{
  return mCurrentContext->CloseContainer(aTag, PR_TRUE);
}

NS_IMETHODIMP
HTMLContentSink::AddLeaf(const nsIParserNode& aNode)
{
  nsresult rv;

  nsHTMLTag nodeType = nsHTMLTag(aNode.GetNodeType());
  switch (nodeType) {
  case eHTMLTag_link:
    mCurrentContext->FlushTextAndRelease();
    rv = ProcessLINKTag(aNode);

    break;
  default:
    rv = mCurrentContext->AddLeaf(aNode);

    break;
  }

  return rv;
}







nsresult
HTMLContentSink::AddComment(const nsIParserNode& aNode)
{
  nsresult rv = mCurrentContext->AddComment(aNode);

  return rv;
}







nsresult
HTMLContentSink::AddProcessingInstruction(const nsIParserNode& aNode)
{
  nsresult result = NS_OK;
  

  return result;
}






NS_IMETHODIMP
HTMLContentSink::AddDocTypeDecl(const nsIParserNode& aNode)
{
  nsAutoString docTypeStr(aNode.GetText());
  nsresult rv = NS_OK;

  PRInt32 publicStart = docTypeStr.Find("PUBLIC", PR_TRUE);
  PRInt32 systemStart = docTypeStr.Find("SYSTEM", PR_TRUE);
  nsAutoString name, publicId, systemId;

  if (publicStart >= 0 || systemStart >= 0) {
    




    if (systemStart >= 0 && (publicStart > systemStart)) {
      publicStart = -1;
    }

    



    docTypeStr.Mid(name, 0, publicStart >= 0 ? publicStart : systemStart);

    if (publicStart >= 0) {
      
      docTypeStr.Mid(publicId, publicStart + 6,
                     docTypeStr.Length() - publicStart);
      publicId.Trim(" \t\n\r");

      
      PRUnichar ch = publicId.IsEmpty() ? '\0' : publicId.First();

      PRBool hasQuote = PR_FALSE;
      if (ch == '"' || ch == '\'') {
        publicId.Cut(0, 1);

        PRInt32 end = publicId.FindChar(ch);

        if (end < 0) {
          




          end = publicId.FindChar('>');
        } else {
          hasQuote = PR_TRUE;
        }

        



        if (end >= 0) {
          publicId.Truncate(end);
        }
      } else {
        
        publicId.Truncate();
      }

      


      PRInt32 pos = docTypeStr.Find(publicId);

      if (systemStart > 0) {
        if (systemStart < pos + (PRInt32)publicId.Length()) {
          systemStart = docTypeStr.Find("SYSTEM", PR_TRUE,
                                        pos + publicId.Length());
        }
      }

      



      if (systemStart < 0) {
        
        systemStart = pos + publicId.Length() + (hasQuote ? 1 : 0);
      }
    }

    if (systemStart >= 0) {
      
      docTypeStr.Mid(systemId, systemStart,
                     docTypeStr.Length() - systemStart);

      
      if (StringBeginsWith(systemId, NS_LITERAL_STRING("SYSTEM")))
        systemId.Cut(0, 6);

      systemId.Trim(" \t\n\r");

      
      PRUnichar ch = systemId.IsEmpty() ? '\0' : systemId.First();

      if (ch == '"' || ch == '\'') {
        systemId.Cut(0, 1);

        PRInt32 end = systemId.FindChar(ch);

        if (end < 0) {
          
          

          end = systemId.FindChar('>');
        }

        
        
        if (end >= 0) {
          systemId.Truncate(end);
        }
      } else {
        systemId.Truncate();
      }
    }
  } else {
    name.Assign(docTypeStr);
  }

  
  if (StringBeginsWith(name, NS_LITERAL_STRING("<!DOCTYPE"), nsCaseInsensitiveStringComparator())) {
    name.Cut(0, 9);
  } else if (StringBeginsWith(name, NS_LITERAL_STRING("DOCTYPE"), nsCaseInsensitiveStringComparator())) {
    name.Cut(0, 7);
  }

  name.Trim(" \t\n\r");

  
  
  
  PRInt32 nameEnd = 0;

  if (name.IsEmpty() || (name.First() != '"' && name.First() != '\'')) {
    nameEnd = name.FindCharInSet(" \n\r\t");
  }

  
  
  if (publicStart < 0) {
    name.Mid(publicId, nameEnd, name.Length() - nameEnd);
    publicId.Trim(" \t\n\r");

    PRUnichar ch = publicId.IsEmpty() ? '\0' : publicId.First();

    if (ch == '"' || ch == '\'') {
      publicId.Cut(0, 1);

      PRInt32 publicEnd = publicId.FindChar(ch);

      if (publicEnd < 0) {
        publicEnd = publicId.FindChar('>');
      }

      if (publicEnd < 0) {
        publicEnd = publicId.Length();
      }

      publicId.Truncate(publicEnd);
    } else {
      
      publicId.Truncate();
    }
  }

  if (nameEnd >= 0) {
    name.Truncate(nameEnd);
  } else {
    nameEnd = name.FindChar('>');

    if (nameEnd >= 0) {
      name.Truncate(nameEnd);
    }
  }

  if (!publicId.IsEmpty() || !systemId.IsEmpty() || !name.IsEmpty()) {
    nsCOMPtr<nsIDOMDocumentType> oldDocType;
    nsCOMPtr<nsIDOMDocumentType> docType;

    nsCOMPtr<nsIDOMDocument> doc(do_QueryInterface(mHTMLDocument));
    doc->GetDoctype(getter_AddRefs(oldDocType));

    
    
    if (name.IsEmpty() || name.LowerCaseEqualsLiteral("html")) {
      name.AssignLiteral("HTML");
    }

    nsCOMPtr<nsIAtom> nameAtom = do_GetAtom(name);
    if (!nameAtom) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    nsAutoString voidString;
    voidString.SetIsVoid(PR_TRUE);
    rv = NS_NewDOMDocumentType(getter_AddRefs(docType),
                               mDocument->NodeInfoManager(), nsnull,
                               nameAtom, nsnull, nsnull, publicId, systemId,
                               voidString);
    NS_ENSURE_SUCCESS(rv, rv);

    if (oldDocType) {
      
      nsCOMPtr<nsIDOMNode> tmpNode;
      rv = doc->ReplaceChild(oldDocType, docType, getter_AddRefs(tmpNode));
    } else {
      
      
      
      nsCOMPtr<nsIContent> content = do_QueryInterface(docType);
      NS_ASSERTION(content, "Doctype isn't content?");
      
      mDocument->InsertChildAt(content, 0, PR_TRUE);
    }
  }

  return rv;
}

NS_IMETHODIMP
HTMLContentSink::DidProcessTokens(void)
{
  return NS_OK;
}

NS_IMETHODIMP
HTMLContentSink::WillProcessAToken(void)
{
  return NS_OK;
}

NS_IMETHODIMP
HTMLContentSink::DidProcessAToken(void)
{
  return DidProcessATokenImpl();
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

NS_IMETHODIMP
HTMLContentSink::NotifyTagObservers(nsIParserNode* aNode)
{
  
  
  
  

  if (!mObservers) {
    return NS_OK;
  }

  PRUint32 flag = 0;

  if (mHTMLDocument && mHTMLDocument->IsWriting()) {
    flag = nsIElementObserver::IS_DOCUMENT_WRITE;
  }

  return mObservers->Notify(aNode, mParser, mDocShell, flag);
}

void
HTMLContentSink::StartLayout(PRBool aIgnorePendingSheets)
{
  if (mLayoutStarted) {
    return;
  }

  mHTMLDocument->SetIsFrameset(mFrameset != nsnull);

  nsContentSink::StartLayout(aIgnorePendingSheets);
}

void
HTMLContentSink::AddBaseTagInfo(nsIContent* aContent)
{
  nsresult rv;
  if (mBaseHref) {
    rv = aContent->SetProperty(nsGkAtoms::htmlBaseHref, mBaseHref,
                               nsPropertyTable::SupportsDtorFunc, PR_TRUE);
    if (NS_SUCCEEDED(rv)) {
      
      NS_ADDREF(static_cast<nsIURI*>(mBaseHref));
    }
  }
  if (mBaseTarget) {
    rv = aContent->SetProperty(nsGkAtoms::htmlBaseTarget, mBaseTarget,
                               nsPropertyTable::SupportsDtorFunc, PR_TRUE);
    if (NS_SUCCEEDED(rv)) {
      
      NS_ADDREF(static_cast<nsIAtom*>(mBaseTarget));
    }
  }
}

nsresult
HTMLContentSink::OpenHeadContext()
{
  if (mCurrentContext && mCurrentContext->IsCurrentContainer(eHTMLTag_head))
    return NS_OK;

  
  
  
  
  
  
  
  
  if (mCurrentContext && (mCurrentContext != mHeadContext)) {
    mCurrentContext->FlushTags();
  }

  if (!mHeadContext) {
    mHeadContext = new SinkContext(this);
    NS_ENSURE_TRUE(mHeadContext, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = mHeadContext->Begin(eHTMLTag_head, mHead, 0, -1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mContextStack.AppendElement(mCurrentContext);
  mCurrentContext = mHeadContext;

  return NS_OK;
}

void
HTMLContentSink::CloseHeadContext()
{
  if (mCurrentContext) {
    if (!mCurrentContext->IsCurrentContainer(eHTMLTag_head))
      return;

    mCurrentContext->FlushTextAndRelease();
    mCurrentContext->FlushTags();
  }

  if (!mContextStack.IsEmpty())
  {
    PRUint32 n = mContextStack.Length() - 1;
    mCurrentContext = mContextStack.ElementAt(n);
    mContextStack.RemoveElementAt(n);
  }
}

void
HTMLContentSink::ProcessBASEElement(nsGenericHTMLElement* aElement)
{
  
  nsAutoString attrValue;
  if (aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::href, attrValue)) {
    
    nsresult rv;
    nsCOMPtr<nsIURI> baseHrefURI;
    rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(baseHrefURI),
                                                   attrValue, mDocument,
                                                   nsnull);
    if (NS_FAILED(rv))
      return;

    
    if (!mBody) {
      
      
      rv = mDocument->SetBaseURI(baseHrefURI);
      if (NS_SUCCEEDED(rv)) {
        mDocumentBaseURI = mDocument->GetBaseURI();
      }
    } else {
      

      nsIScriptSecurityManager *securityManager =
        nsContentUtils::GetSecurityManager();

      rv = securityManager->
        CheckLoadURIWithPrincipal(mDocument->NodePrincipal(), baseHrefURI,
                                  nsIScriptSecurityManager::STANDARD);
      if (NS_SUCCEEDED(rv)) {
        mBaseHref = baseHrefURI;
      }
    }
  }

  
  if (aElement->GetAttr(kNameSpaceID_None, nsGkAtoms::target, attrValue)) {
    if (!mBody) {
      
      mDocument->SetBaseTarget(attrValue);
    } else {
      
      mBaseTarget = do_GetAtom(attrValue);
    }
  }
}

nsresult
HTMLContentSink::ProcessLINKTag(const nsIParserNode& aNode)
{
  nsresult  result = NS_OK;

  if (mCurrentContext) {
    
    nsCOMPtr<nsIContent> element;
    nsCOMPtr<nsINodeInfo> nodeInfo;
    nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::link, nsnull, kNameSpaceID_XHTML);

    result = NS_NewHTMLElement(getter_AddRefs(element), nodeInfo, PR_FALSE);
    NS_ENSURE_SUCCESS(result, result);

    nsCOMPtr<nsIStyleSheetLinkingElement> ssle(do_QueryInterface(element));

    if (ssle) {
      
      if (!mInsideNoXXXTag) {
        ssle->InitStyleLinkElement(PR_FALSE);
        ssle->SetEnableUpdates(PR_FALSE);
      } else {
        ssle->InitStyleLinkElement(PR_TRUE);
      }
    }

    
    
    AddBaseTagInfo(element);
    result = AddAttributes(aNode, element);
    if (NS_FAILED(result)) {
      return result;
    }

    mCurrentContext->AddLeaf(element); 

    if (ssle) {
      ssle->SetEnableUpdates(PR_TRUE);
      PRBool willNotify;
      PRBool isAlternate;
      result = ssle->UpdateStyleSheet(this, &willNotify, &isAlternate);
      if (NS_SUCCEEDED(result) && willNotify && !isAlternate) {
        ++mPendingSheetCount;
        mScriptLoader->AddExecuteBlocker();
      }

      
      nsAutoString relVal;
      element->GetAttr(kNameSpaceID_None, nsGkAtoms::rel, relVal);
      if (!relVal.IsEmpty()) {
        
        nsAutoTArray<nsString, 4> linkTypes;
        nsStyleLinkElement::ParseLinkTypes(relVal, linkTypes);
        PRBool hasPrefetch = linkTypes.Contains(NS_LITERAL_STRING("prefetch"));
        if (hasPrefetch || linkTypes.Contains(NS_LITERAL_STRING("next"))) {
          nsAutoString hrefVal;
          element->GetAttr(kNameSpaceID_None, nsGkAtoms::href, hrefVal);
          if (!hrefVal.IsEmpty()) {
            PrefetchHref(hrefVal, element, hasPrefetch);
          }
        }
        if (linkTypes.Contains(NS_LITERAL_STRING("dns-prefetch"))) {
          nsAutoString hrefVal;
          element->GetAttr(kNameSpaceID_None, nsGkAtoms::href, hrefVal);
          if (!hrefVal.IsEmpty()) {
            PrefetchDNS(hrefVal);
          }
        }
      }
    }
  }

  return result;
}

#ifdef DEBUG
void
HTMLContentSink::ForceReflow()
{
  mCurrentContext->FlushTags();
}
#endif

void
HTMLContentSink::NotifyInsert(nsIContent* aContent,
                              nsIContent* aChildContent,
                              PRInt32 aIndexInContainer)
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
  
  
  
  
  
  
  
  mNotifiedRootInsertion = PR_TRUE;
  PRInt32 index = mDocument->IndexOf(mRoot);
  NS_ASSERTION(index != -1, "mRoot not child of document?");
  NotifyInsert(nsnull, mRoot, index);

  
  
  
  UpdateChildCounts();
}

PRBool
HTMLContentSink::IsMonolithicContainer(nsHTMLTag aTag)
{
  if (aTag == eHTMLTag_tr     ||
      aTag == eHTMLTag_select ||
      aTag == eHTMLTag_applet ||
      aTag == eHTMLTag_object) {
    return PR_TRUE;
  }

  return PR_FALSE;
}

void
HTMLContentSink::UpdateChildCounts()
{
  PRUint32 numContexts = mContextStack.Length();
  for (PRUint32 i = 0; i < numContexts; i++) {
    SinkContext* sc = mContextStack.ElementAt(i);

    sc->UpdateChildCounts();
  }

  mCurrentContext->UpdateChildCounts();
}

void
HTMLContentSink::PreEvaluateScript()
{
  
  
  SINK_TRACE(gSinkLogModuleInfo, SINK_TRACE_CALLS,
             ("HTMLContentSink::PreEvaluateScript: flushing tags before "
              "evaluating script"));

  
  mCurrentContext->FlushText();
}

void
HTMLContentSink::PostEvaluateScript(nsIScriptElement *aElement)
{
  mHTMLDocument->ScriptExecuted(aElement);
}

nsresult
HTMLContentSink::ProcessSCRIPTEndTag(nsGenericHTMLElement *content,
                                     PRBool aMalformed)
{
  
  
  
  
  
  
  

  
  mCurrentContext->FlushText();

  nsCOMPtr<nsIScriptElement> sele = do_QueryInterface(content);
  NS_ASSERTION(sele, "Not really closing a script tag?");

  if (aMalformed) {
    
    sele->SetIsMalformed();
  }
  if (mFrameset) {
    sele->PreventExecution();
  }

  
  mHTMLDocument->ScriptLoading(sele);

  
  
  
  nsresult rv = content->DoneAddingChildren(PR_TRUE);

  
  
  if (rv == NS_ERROR_HTMLPARSER_BLOCK) {
    
    
    
    
    mScriptElements.AppendObject(sele);
  }
  else {
    
    
    mHTMLDocument->ScriptExecuted(sele);
  }

  
  
  if (mParser && !mParser->IsParserEnabled()) {
    rv = NS_ERROR_HTMLPARSER_BLOCK;
  }

  return rv;
}




nsresult
HTMLContentSink::ProcessSTYLEEndTag(nsGenericHTMLElement* content)
{
  nsCOMPtr<nsIStyleSheetLinkingElement> ssle = do_QueryInterface(content);

  NS_ASSERTION(ssle,
               "html:style doesn't implement nsIStyleSheetLinkingElement");

  nsresult rv = NS_OK;

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

  return rv;
}

void
HTMLContentSink::FlushPendingNotifications(mozFlushType aType)
{
  
  
  if (!mInNotification) {
    
    
    if (mIsDocumentObserver) {
      if (aType >= Flush_ContentAndNotify) {
        FlushTags();
      }
      else if (mCurrentContext) {
        mCurrentContext->FlushText();
      }
    }
    if (aType >= Flush_InterruptibleLayout) {
      
      
      StartLayout(PR_TRUE);
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
  if (mDocShell) {
    
    
    
    
    nsCOMPtr<nsIMarkupDocumentViewer> muCV;
    nsCOMPtr<nsIContentViewer> cv;
    mDocShell->GetContentViewer(getter_AddRefs(cv));
    if (cv) {
       muCV = do_QueryInterface(cv);
    } else {
      
      
      

      nsCOMPtr<nsIDocShellTreeItem> docShellAsItem =
        do_QueryInterface(mDocShell);
      NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);

      nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
      docShellAsItem->GetSameTypeParent(getter_AddRefs(parentAsItem));

      nsCOMPtr<nsIDocShell> parent(do_QueryInterface(parentAsItem));
      if (parent) {
        nsCOMPtr<nsIContentViewer> parentContentViewer;
        nsresult rv =
          parent->GetContentViewer(getter_AddRefs(parentContentViewer));
        if (NS_SUCCEEDED(rv) && parentContentViewer) {
          muCV = do_QueryInterface(parentContentViewer);
        }
      }
    }

    if (muCV) {
      muCV->SetPrevDocCharacterSet(aCharset);
    }
  }

  if (mDocument) {
    mDocument->SetDocumentCharacterSet(aCharset);
  }

  return NS_OK;
}

nsISupports *
HTMLContentSink::GetTarget()
{
  return mDocument;
}

PRBool
HTMLContentSink::IsScriptExecuting()
{
  return IsScriptExecutingImpl();
}

#ifdef DEBUG








NS_IMETHODIMP
HTMLContentSink::DumpContentModel()
{
  FILE* out = ::fopen("rtest_html.txt", "a");
  if (out) {
    if (mDocument) {
      nsIContent* root = mDocument->GetRootContent();
      if (root) {
        if (mDocumentURI) {
          nsCAutoString buf;
          mDocumentURI->GetSpec(buf);
          fputs(buf.get(), out);
        }

        fputs(";", out);
        root->DumpContent(out, 0, PR_FALSE);
        fputs(";\n", out);
      }
    }

    fclose(out);
  }

  return NS_OK;
}
#endif

