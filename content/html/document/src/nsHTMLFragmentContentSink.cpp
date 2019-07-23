





































#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIFragmentContentSink.h"
#include "nsIDTD.h"
#include "nsIHTMLContentSink.h"
#include "nsIParser.h"
#include "nsIParserService.h"
#include "nsGkAtoms.h"
#include "nsHTMLTokens.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMText.h"
#include "nsIDOMComment.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMDocumentFragment.h"
#include "nsTArray.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "nsINodeInfo.h"
#include "prmem.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsContentUtils.h"
#include "nsEscape.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsNetUtil.h"
#include "nsIScriptSecurityManager.h"
#include "nsContentSink.h"
#include "nsTHashtable.h"
#include "nsCycleCollectionParticipant.h"








class nsHTMLFragmentContentSink : public nsIFragmentContentSink,
                                  public nsIHTMLContentSink {
public:
  nsHTMLFragmentContentSink(PRBool aAllContent = PR_FALSE);
  virtual ~nsHTMLFragmentContentSink();

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsHTMLFragmentContentSink,
                                           nsIContentSink)

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  
  NS_IMETHOD WillParse(void) { return NS_OK; }
  NS_IMETHOD WillBuildModel(nsDTDMode aDTDMode);
  NS_IMETHOD DidBuildModel(PRBool aTerminated);
  NS_IMETHOD WillInterrupt(void);
  NS_IMETHOD WillResume(void);
  NS_IMETHOD SetParser(nsIParser* aParser);
  virtual void FlushPendingNotifications(mozFlushType aType) { }
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset) { return NS_OK; }
  virtual nsISupports *GetTarget() { return mTargetDocument; }

  
  NS_IMETHOD BeginContext(PRInt32 aID);
  NS_IMETHOD EndContext(PRInt32 aID);
  NS_IMETHOD OpenHead();
  NS_IMETHOD IsEnabled(PRInt32 aTag, PRBool* aReturn) {
    *aReturn = PR_TRUE;
    return NS_OK;
  }
  NS_IMETHOD_(PRBool) IsFormOnStack() { return PR_FALSE; }
  NS_IMETHOD DidProcessTokens(void) { return NS_OK; }
  NS_IMETHOD WillProcessAToken(void) { return NS_OK; }
  NS_IMETHOD DidProcessAToken(void) { return NS_OK; }
  NS_IMETHOD NotifyTagObservers(nsIParserNode* aNode) { return NS_OK; }
  NS_IMETHOD OpenContainer(const nsIParserNode& aNode);
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag);
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode);
  NS_IMETHOD AddComment(const nsIParserNode& aNode);
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode);
  NS_IMETHOD AddDocTypeDecl(const nsIParserNode& aNode);

  
  NS_IMETHOD GetFragment(PRBool aWillOwnFragment,
                         nsIDOMDocumentFragment** aFragment);
  NS_IMETHOD SetTargetDocument(nsIDocument* aDocument);
  NS_IMETHOD WillBuildContent();
  NS_IMETHOD DidBuildContent();
  NS_IMETHOD IgnoreFirstContainer();

  nsIContent* GetCurrentContent();
  PRInt32 PushContent(nsIContent *aContent);
  nsIContent* PopContent();

  virtual nsresult AddAttributes(const nsIParserNode& aNode,
                                 nsIContent* aContent);

  nsresult AddText(const nsAString& aString);
  nsresult AddTextToContent(nsIContent* aContent, const nsAString& aText);
  nsresult FlushText();

  void ProcessBaseTag(nsIContent* aContent);
  void AddBaseTagInfo(nsIContent* aContent);

  nsresult Init();

  PRPackedBool mAllContent;
  PRPackedBool mProcessing;
  PRPackedBool mSeenBody;
  PRPackedBool mIgnoreContainer;
  PRPackedBool mIgnoreNextCloseHead;

  nsCOMPtr<nsIContent> mRoot;
  nsCOMPtr<nsIParser> mParser;

  nsTArray<nsIContent*>* mContentStack;

  PRUnichar* mText;
  PRInt32 mTextLength;
  PRInt32 mTextSize;

  nsCOMPtr<nsIURI> mBaseHref;
  nsCOMPtr<nsIAtom> mBaseTarget;

  nsCOMPtr<nsIDocument> mTargetDocument;
  nsRefPtr<nsNodeInfoManager> mNodeInfoManager;

  nsINodeInfo* mNodeInfoCache[NS_HTML_TAG_MAX + 1];
};

static nsresult
NewHTMLFragmentContentSinkHelper(PRBool aAllContent, nsIFragmentContentSink** aResult)
{
  NS_PRECONDITION(aResult, "Null out ptr");
  if (nsnull == aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  nsHTMLFragmentContentSink* it = new nsHTMLFragmentContentSink(aAllContent);
  if (nsnull == it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  NS_ADDREF(*aResult = it);
  
  return NS_OK;
}

nsresult
NS_NewHTMLFragmentContentSink2(nsIFragmentContentSink** aResult)
{
  return NewHTMLFragmentContentSinkHelper(PR_TRUE,aResult);
}

nsresult
NS_NewHTMLFragmentContentSink(nsIFragmentContentSink** aResult)
{
  return NewHTMLFragmentContentSinkHelper(PR_FALSE,aResult);
}

nsHTMLFragmentContentSink::nsHTMLFragmentContentSink(PRBool aAllContent)
  : mAllContent(aAllContent),
    mProcessing(aAllContent),
    mSeenBody(!aAllContent)
{
  
}

nsHTMLFragmentContentSink::~nsHTMLFragmentContentSink()
{
  
  

  if (nsnull != mContentStack) {
    
    PRInt32 indx = mContentStack->Length();
    while (0 < indx--) {
      nsIContent* content = mContentStack->ElementAt(indx);
      NS_RELEASE(content);
    }
    delete mContentStack;
  }

  PR_FREEIF(mText);

  PRUint32 i;
  for (i = 0; i < NS_ARRAY_LENGTH(mNodeInfoCache); ++i) {
    NS_IF_RELEASE(mNodeInfoCache[i]);
  }
}

NS_IMPL_CYCLE_COLLECTING_ADDREF_AMBIGUOUS(nsHTMLFragmentContentSink,
                                          nsIContentSink)
NS_IMPL_CYCLE_COLLECTING_RELEASE_AMBIGUOUS(nsHTMLFragmentContentSink,
                                           nsIContentSink)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsHTMLFragmentContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIFragmentContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIHTMLContentSink)
  NS_INTERFACE_MAP_ENTRY(nsIContentSink)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContentSink)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLFragmentContentSink)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsHTMLFragmentContentSink)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mParser)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mTargetDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mRoot)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mNodeInfoManager)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsHTMLFragmentContentSink)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mParser)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mTargetDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRoot)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mNodeInfoManager,
                                                  nsNodeInfoManager)
  {
    PRUint32 i;
    for (i = 0; i < NS_ARRAY_LENGTH(tmp->mNodeInfoCache); ++i) {
      cb.NoteXPCOMChild(tmp->mNodeInfoCache[i]);
    }
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMETHODIMP
nsHTMLFragmentContentSink::WillBuildModel(nsDTDMode)
{
  if (mRoot) {
    return NS_OK;
  }

  NS_ASSERTION(mNodeInfoManager, "Need a nodeinfo manager!");

  nsCOMPtr<nsIDOMDocumentFragment> frag;
  nsresult rv = NS_NewDocumentFragment(getter_AddRefs(frag), mNodeInfoManager);
  NS_ENSURE_SUCCESS(rv, rv);

  mRoot = do_QueryInterface(frag, &rv);
  
  return rv;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::DidBuildModel(PRBool aTerminated)
{
  FlushText();

  
  
  mParser = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::WillInterrupt(void)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::WillResume(void)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::SetParser(nsIParser* aParser)
{
  mParser = aParser;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::BeginContext(PRInt32 aID)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::EndContext(PRInt32 aID)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::OpenHead()
{
  mIgnoreNextCloseHead = PR_TRUE;
  return NS_OK;
}

void
nsHTMLFragmentContentSink::ProcessBaseTag(nsIContent* aContent)
{
  nsAutoString value;
  if (aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::href, value)) {
    nsCOMPtr<nsIURI> baseHrefURI;
    nsresult rv = 
      nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(baseHrefURI),
                                                value, mTargetDocument,
                                                nsnull);
    if (NS_FAILED(rv))
      return;

    nsIScriptSecurityManager *securityManager =
      nsContentUtils::GetSecurityManager();

    NS_ASSERTION(aContent->NodePrincipal() == mTargetDocument->NodePrincipal(),
                 "How'd that happpen?");
    
    rv = securityManager->
      CheckLoadURIWithPrincipal(mTargetDocument->NodePrincipal(), baseHrefURI,
                                nsIScriptSecurityManager::STANDARD);
    if (NS_SUCCEEDED(rv)) {
      mBaseHref = baseHrefURI;
    }
  }
  if (aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::target, value)) {
    mBaseTarget = do_GetAtom(value);
  }
}

void
nsHTMLFragmentContentSink::AddBaseTagInfo(nsIContent* aContent)
{
  if (!aContent) {
    return;
  }

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

NS_IMETHODIMP
nsHTMLFragmentContentSink::OpenContainer(const nsIParserNode& aNode)
{
  NS_ENSURE_TRUE(mNodeInfoManager, NS_ERROR_NOT_INITIALIZED);

  nsresult result = NS_OK;

  nsHTMLTag nodeType = nsHTMLTag(aNode.GetNodeType());

  if (nodeType == eHTMLTag_html) {
    return NS_OK;
  }

  
  
  if (nodeType == eHTMLTag_body) {
    if (mSeenBody) {
      return NS_OK;
    }
    mSeenBody = PR_TRUE;
  }

  if (mProcessing && !mIgnoreContainer) {
    FlushText();

    nsIContent *content = nsnull;

    nsCOMPtr<nsINodeInfo> nodeInfo;

    if (nodeType == eHTMLTag_userdefined) {
      NS_ConvertUTF16toUTF8 tmp(aNode.GetText());
      ToLowerCase(tmp);

      nsCOMPtr<nsIAtom> name = do_GetAtom(tmp);
      nodeInfo = mNodeInfoManager->GetNodeInfo(name, 
                                               nsnull, 
                                               kNameSpaceID_XHTML);
      NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
    }
    else if (mNodeInfoCache[nodeType]) {
      nodeInfo = mNodeInfoCache[nodeType];
    }
    else {
      nsIParserService* parserService = nsContentUtils::GetParserService();
      if (!parserService)
        return NS_ERROR_OUT_OF_MEMORY;

      nsIAtom *name = parserService->HTMLIdToAtomTag(nodeType);
      NS_ASSERTION(name, "This should not happen!");

      nodeInfo = mNodeInfoManager->GetNodeInfo(name, 
                                               nsnull, 
                                               kNameSpaceID_XHTML);
      NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

      NS_ADDREF(mNodeInfoCache[nodeType] = nodeInfo);
    }

    content = CreateHTMLElement(nodeType, nodeInfo, PR_FALSE).get();
    NS_ENSURE_TRUE(content, NS_ERROR_OUT_OF_MEMORY);

    result = AddAttributes(aNode, content);
    if (NS_FAILED(result)) {
      NS_RELEASE(content);
      return result;
    }

    nsIContent *parent = GetCurrentContent();
    if (!parent) {
      parent = mRoot;
    }

    parent->AppendChildTo(content, PR_FALSE);
    PushContent(content);

    if (nodeType == eHTMLTag_table
        || nodeType == eHTMLTag_thead
        || nodeType == eHTMLTag_tbody
        || nodeType == eHTMLTag_tfoot
        || nodeType == eHTMLTag_tr
        || nodeType == eHTMLTag_td
        || nodeType == eHTMLTag_th)
      
      AddBaseTagInfo(content); 
  }
  else if (mProcessing && mIgnoreContainer) {
    mIgnoreContainer = PR_FALSE;
  }

  return result;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::CloseContainer(const nsHTMLTag aTag)
{
  if (aTag == eHTMLTag_html) {
    return NS_OK;
  }
  if (mIgnoreNextCloseHead && aTag == eHTMLTag_head) {
    mIgnoreNextCloseHead = PR_FALSE;
    return NS_OK;
  }

  if (mProcessing && (nsnull != GetCurrentContent())) {
    nsIContent* content;
    FlushText();
    content = PopContent();
    NS_RELEASE(content);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::AddLeaf(const nsIParserNode& aNode)
{
  NS_ENSURE_TRUE(mNodeInfoManager, NS_ERROR_NOT_INITIALIZED);

  nsresult result = NS_OK;

  switch (aNode.GetTokenType()) {
    case eToken_start:
      {
        FlushText();

        
        nsRefPtr<nsGenericHTMLElement> content;
        nsHTMLTag nodeType = nsHTMLTag(aNode.GetNodeType());

        nsIParserService* parserService = nsContentUtils::GetParserService();
        if (!parserService)
          return NS_ERROR_OUT_OF_MEMORY;

        nsCOMPtr<nsINodeInfo> nodeInfo;

        if (nodeType == eHTMLTag_userdefined) {
          NS_ConvertUTF16toUTF8 tmp(aNode.GetText());
          ToLowerCase(tmp);

          nsCOMPtr<nsIAtom> name = do_GetAtom(tmp);
          nodeInfo = mNodeInfoManager->GetNodeInfo(name, nsnull,
                                                   kNameSpaceID_XHTML);
          NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
        }
        else if (mNodeInfoCache[nodeType]) {
          nodeInfo = mNodeInfoCache[nodeType];
        }
        else {
          nsIAtom *name = parserService->HTMLIdToAtomTag(nodeType);
          NS_ASSERTION(name, "This should not happen!");

          nodeInfo = mNodeInfoManager->GetNodeInfo(name, nsnull,
                                                   kNameSpaceID_XHTML);
          NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
          NS_ADDREF(mNodeInfoCache[nodeType] = nodeInfo);
        }

        content = CreateHTMLElement(nodeType, nodeInfo, PR_FALSE);
        NS_ENSURE_TRUE(content, NS_ERROR_OUT_OF_MEMORY);

        result = AddAttributes(aNode, content);
        NS_ENSURE_SUCCESS(result, result);

        nsIContent *parent = GetCurrentContent();
        if (!parent) {
          parent = mRoot;
        }

        parent->AppendChildTo(content, PR_FALSE);

        if (nodeType == eHTMLTag_img || nodeType == eHTMLTag_frame
            || nodeType == eHTMLTag_input)    
            AddBaseTagInfo(content);
        else if (nodeType == eHTMLTag_base)
            ProcessBaseTag(content);
      }
      break;
    case eToken_text:
    case eToken_whitespace:
    case eToken_newline:
      result = AddText(aNode.GetText());
      break;

    case eToken_entity:
      {
        nsAutoString tmp;
        PRInt32 unicode = aNode.TranslateToUnicodeStr(tmp);
        if (unicode < 0) {
          result = AddText(aNode.GetText());
        }
        else {
          result = AddText(tmp);
        }
      }
      break;
  }

  return result;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::AddComment(const nsIParserNode& aNode)
{
  nsCOMPtr<nsIContent> comment;
  nsresult result = NS_OK;

  FlushText();

  result = NS_NewCommentNode(getter_AddRefs(comment), mNodeInfoManager);
  if (NS_SUCCEEDED(result)) {
    comment->SetText(aNode.GetText(), PR_FALSE);

    nsIContent *parent = GetCurrentContent();

    if (nsnull == parent) {
      parent = mRoot;
    }

    parent->AppendChildTo(comment, PR_FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::AddProcessingInstruction(const nsIParserNode& aNode)
{
  return NS_OK;
}






NS_IMETHODIMP
nsHTMLFragmentContentSink::AddDocTypeDecl(const nsIParserNode& aNode)
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::GetFragment(PRBool aWillOwnFragment,
                                       nsIDOMDocumentFragment** aFragment)
{
  if (mRoot) {
    nsresult rv = CallQueryInterface(mRoot, aFragment);
    if (NS_SUCCEEDED(rv) && aWillOwnFragment) {
      mRoot = nsnull;
    }
    return rv;
  }

  *aFragment = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::SetTargetDocument(nsIDocument* aTargetDocument)
{
  NS_ENSURE_ARG_POINTER(aTargetDocument);

  mTargetDocument = aTargetDocument;
  mNodeInfoManager = aTargetDocument->NodeInfoManager();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::WillBuildContent()
{
  mProcessing = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::DidBuildContent()
{
  if (!mAllContent) {
    FlushText();
    DidBuildModel(PR_FALSE); 
    mProcessing = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFragmentContentSink::IgnoreFirstContainer()
{
  mIgnoreContainer = PR_TRUE;
  return NS_OK;
}

nsIContent*
nsHTMLFragmentContentSink::GetCurrentContent()
{
  if (nsnull != mContentStack) {
    PRInt32 indx = mContentStack->Length() - 1;
    if (indx >= 0)
      return mContentStack->ElementAt(indx);
  }
  return nsnull;
}

PRInt32
nsHTMLFragmentContentSink::PushContent(nsIContent *aContent)
{
  if (nsnull == mContentStack) {
    mContentStack = new nsTArray<nsIContent*>();
  }

  mContentStack->AppendElement(aContent);
  return mContentStack->Length();
}

nsIContent*
nsHTMLFragmentContentSink::PopContent()
{
  nsIContent* content = nsnull;
  if (nsnull != mContentStack) {
    PRInt32 indx = mContentStack->Length() - 1;
    if (indx >= 0) {
      content = mContentStack->ElementAt(indx);
      mContentStack->RemoveElementAt(indx);
    }
  }
  return content;
}

#define NS_ACCUMULATION_BUFFER_SIZE 4096

nsresult
nsHTMLFragmentContentSink::AddText(const nsAString& aString)
{
  PRInt32 addLen = aString.Length();
  if (0 == addLen) {
    return NS_OK;
  }

  
  if (0 == mTextSize) {
    mText = (PRUnichar *) PR_MALLOC(sizeof(PRUnichar) * NS_ACCUMULATION_BUFFER_SIZE);
    if (nsnull == mText) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mTextSize = NS_ACCUMULATION_BUFFER_SIZE;
  }

  
  PRInt32 offset = 0;
  PRBool  isLastCharCR = PR_FALSE;
  while (0 != addLen) {
    PRInt32 amount = mTextSize - mTextLength;
    if (amount > addLen) {
      amount = addLen;
    }
    if (0 == amount) {
      nsresult rv = FlushText();
      if (NS_OK != rv) {
        return rv;
      }
    }
    mTextLength +=
      nsContentUtils::CopyNewlineNormalizedUnicodeTo(aString,
                                                     offset,
                                                     &mText[mTextLength],
                                                     amount,
                                                     isLastCharCR);
    offset += amount;
    addLen -= amount;
  }

  return NS_OK;
}

nsresult
nsHTMLFragmentContentSink::AddTextToContent(nsIContent* aContent, const nsAString& aText) {
  NS_ASSERTION(aContent !=nsnull, "can't add text w/o a content");

  nsresult result=NS_OK;

  if(aContent) {
    if (!aText.IsEmpty()) {
      nsCOMPtr<nsIContent> text;
      result = NS_NewTextNode(getter_AddRefs(text), mNodeInfoManager);
      if (NS_SUCCEEDED(result)) {
        text->SetText(aText, PR_TRUE);

        result = aContent->AppendChildTo(text, PR_FALSE);
      }
    }
  }
  return result;
}

nsresult
nsHTMLFragmentContentSink::FlushText()
{
  if (0 == mTextLength) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content;
  nsresult rv = NS_NewTextNode(getter_AddRefs(content), mNodeInfoManager);
  NS_ENSURE_SUCCESS(rv, rv);

  
  content->SetText(mText, mTextLength, PR_FALSE);

  
  nsIContent *parent = GetCurrentContent();

  if (!parent) {
    parent = mRoot;
  }

  rv = parent->AppendChildTo(content, PR_FALSE);

  mTextLength = 0;

  return rv;
}


nsresult
nsHTMLFragmentContentSink::AddAttributes(const nsIParserNode& aNode,
                                         nsIContent* aContent)
{
  

  PRInt32 ac = aNode.GetAttributeCount();

  if (ac == 0) {
    
    

    return NS_OK;
  }

  nsCAutoString k;
  nsHTMLTag nodeType = nsHTMLTag(aNode.GetNodeType());

  
  
  
  
  
  
  
  

  for (PRInt32 i = ac - 1; i >= 0; i--) {
    
    const nsAString& key = aNode.GetKeyAt(i);
    
    
    CopyUTF16toUTF8(key, k);
    ToLowerCase(k);

    nsCOMPtr<nsIAtom> keyAtom = do_GetAtom(k);

    
    static const char* kWhitespace = "\n\r\t\b";
    const nsAString& v =
      nsContentUtils::TrimCharsInSet(kWhitespace, aNode.GetValueAt(i));

    if (nodeType == eHTMLTag_a && keyAtom == nsGkAtoms::name) {
      NS_ConvertUTF16toUTF8 cname(v);
      NS_ConvertUTF8toUTF16 uv(nsUnescape(cname.BeginWriting()));

      
      aContent->SetAttr(kNameSpaceID_None, keyAtom, uv, PR_FALSE);
    } else {
      
      aContent->SetAttr(kNameSpaceID_None, keyAtom, v, PR_FALSE);
    }
  }

  return NS_OK;
}






class nsHTMLParanoidFragmentSink : public nsHTMLFragmentContentSink
{
public:
  nsHTMLParanoidFragmentSink();

  static nsresult Init();
  static void Cleanup();

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD OpenContainer(const nsIParserNode& aNode);
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag);
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode);
  NS_IMETHOD AddComment(const nsIParserNode& aNode);
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode);

  nsresult AddAttributes(const nsIParserNode& aNode,
                         nsIContent* aContent);
protected:
  nsresult NameFromType(const nsHTMLTag aTag,
                        nsIAtom **aResult);

  nsresult NameFromNode(const nsIParserNode& aNode,
                        nsIAtom **aResult);
  
  PRBool mSkip; 

  
  static nsTHashtable<nsISupportsHashKey>* sAllowedTags;
  static nsTHashtable<nsISupportsHashKey>* sAllowedAttributes;
};

nsTHashtable<nsISupportsHashKey>* nsHTMLParanoidFragmentSink::sAllowedTags;
nsTHashtable<nsISupportsHashKey>* nsHTMLParanoidFragmentSink::sAllowedAttributes;

nsHTMLParanoidFragmentSink::nsHTMLParanoidFragmentSink():
  nsHTMLFragmentContentSink(PR_FALSE), mSkip(PR_FALSE)
{
}

nsresult
nsHTMLParanoidFragmentSink::Init()
{
  nsresult rv = NS_ERROR_FAILURE;
  
  if (sAllowedTags) {
    return NS_OK;
  }

  sAllowedTags = new nsTHashtable<nsISupportsHashKey>();
  if (sAllowedTags) {
    rv = sAllowedTags->Init(80);
    for (PRUint32 i = 0; kDefaultAllowedTags[i] && NS_SUCCEEDED(rv); i++) {
      if (!sAllowedTags->PutEntry(*kDefaultAllowedTags[i])) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  sAllowedAttributes = new nsTHashtable<nsISupportsHashKey>();
  if (sAllowedAttributes && NS_SUCCEEDED(rv)) {
    rv = sAllowedAttributes->Init(80);
    for (PRUint32 i = 0;
         kDefaultAllowedAttributes[i] && NS_SUCCEEDED(rv); i++) {
      if (!sAllowedAttributes->PutEntry(*kDefaultAllowedAttributes[i])) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to populate whitelist hash sets");
    Cleanup();
    return rv; 
  }

  return rv;
}

void
nsHTMLParanoidFragmentSink::Cleanup()
{
  if (sAllowedTags) {
    delete sAllowedTags;
    sAllowedTags = nsnull;
  }
  
  if (sAllowedAttributes) {
    delete sAllowedAttributes;
    sAllowedAttributes = nsnull;
  }
}

nsresult
NS_NewHTMLParanoidFragmentSink(nsIFragmentContentSink** aResult)
{
  nsHTMLParanoidFragmentSink* it = new nsHTMLParanoidFragmentSink();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsresult rv = nsHTMLParanoidFragmentSink::Init();
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ADDREF(*aResult = it);
  
  return NS_OK;
}

void
NS_HTMLParanoidFragmentSinkShutdown()
{
  nsHTMLParanoidFragmentSink::Cleanup();
}

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLParanoidFragmentSink,
                             nsHTMLFragmentContentSink)

nsresult
nsHTMLParanoidFragmentSink::NameFromType(const nsHTMLTag aTag,
                                         nsIAtom **aResult)
{
  nsIParserService* parserService = nsContentUtils::GetParserService();
  if (!parserService) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_IF_ADDREF(*aResult = parserService->HTMLIdToAtomTag(aTag));
  
  return NS_OK;
}

nsresult
nsHTMLParanoidFragmentSink::NameFromNode(const nsIParserNode& aNode,
                                         nsIAtom **aResult)
{
  nsresult rv;
  eHTMLTags type = (eHTMLTags)aNode.GetNodeType();
  
  *aResult = nsnull;
  if (type == eHTMLTag_userdefined) {
    nsCOMPtr<nsINodeInfo> nodeInfo;
    rv =
      mNodeInfoManager->GetNodeInfo(aNode.GetText(), nsnull,
                                    kNameSpaceID_XHTML,
                                    getter_AddRefs(nodeInfo));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_IF_ADDREF(*aResult = nodeInfo->NameAtom());
  } else {
    rv = NameFromType(type, aResult);
  }
  return rv;
}



nsresult
nsHTMLParanoidFragmentSink::AddAttributes(const nsIParserNode& aNode,
                                          nsIContent* aContent)
{
  PRInt32 ac = aNode.GetAttributeCount();

  if (ac == 0) {
    return NS_OK;
  }

  nsCAutoString k;
  nsHTMLTag nodeType = nsHTMLTag(aNode.GetNodeType());

  nsresult rv;
  
  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIURI> baseURI;

  for (PRInt32 i = ac - 1; i >= 0; i--) {
    rv = NS_OK;
    const nsAString& key = aNode.GetKeyAt(i);
    CopyUTF16toUTF8(key, k);
    ToLowerCase(k);

    nsCOMPtr<nsIAtom> keyAtom = do_GetAtom(k);

    
    if (!sAllowedAttributes || !sAllowedAttributes->GetEntry(keyAtom)) {
      continue;
    }

    
    static const char* kWhitespace = "\n\r\t\b";
    const nsAString& v =
      nsContentUtils::TrimCharsInSet(kWhitespace, aNode.GetValueAt(i));

    
    if (IsAttrURI(keyAtom)) {
      if (!baseURI) {
        baseURI = aContent->GetBaseURI();
      }
      nsCOMPtr<nsIURI> attrURI;
      rv = NS_NewURI(getter_AddRefs(attrURI), v, nsnull, baseURI);
      if (NS_SUCCEEDED(rv)) {
        rv = secMan->
          CheckLoadURIWithPrincipal(mTargetDocument->NodePrincipal(),
                attrURI,
                nsIScriptSecurityManager::DISALLOW_INHERIT_PRINCIPAL);
      }
    }
    
    
    
    if (NS_FAILED(rv)) {
      continue;
    }

    if (nodeType == eHTMLTag_a && keyAtom == nsGkAtoms::name) {
      NS_ConvertUTF16toUTF8 cname(v);
      NS_ConvertUTF8toUTF16 uv(nsUnescape(cname.BeginWriting()));
      
      aContent->SetAttr(kNameSpaceID_None, keyAtom, uv, PR_FALSE);
    } else {
      
      aContent->SetAttr(kNameSpaceID_None, keyAtom, v, PR_FALSE);
    }

    if (nodeType == eHTMLTag_a || 
        nodeType == eHTMLTag_form ||
        nodeType == eHTMLTag_img ||
        nodeType == eHTMLTag_map ||
        nodeType == eHTMLTag_q ||
        nodeType == eHTMLTag_blockquote ||
        nodeType == eHTMLTag_input) {
      AddBaseTagInfo(aContent);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLParanoidFragmentSink::OpenContainer(const nsIParserNode& aNode)
{
  nsresult rv = NS_OK;
  
  
  eHTMLTags type = (eHTMLTags)aNode.GetNodeType();
  if (type == eHTMLTag_script || type == eHTMLTag_style) {
    mSkip = PR_TRUE;
    return rv;
  }

  nsCOMPtr<nsIAtom> name;
  rv = NameFromNode(aNode, getter_AddRefs(name));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!sAllowedTags || !sAllowedTags->GetEntry(name)) {
    return NS_OK;
  }

  return nsHTMLFragmentContentSink::OpenContainer(aNode);
}

NS_IMETHODIMP
nsHTMLParanoidFragmentSink::CloseContainer(const nsHTMLTag aTag)
{
  nsresult rv = NS_OK;

  if (mSkip) {
    mSkip = PR_FALSE;
    return rv;
  }

  nsCOMPtr<nsIAtom> name;
  rv = NameFromType(aTag, getter_AddRefs(name));
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  if (!sAllowedTags || !sAllowedTags->GetEntry(name)) {
    return NS_OK;
  }

  return nsHTMLFragmentContentSink::CloseContainer(aTag);
}

NS_IMETHODIMP
nsHTMLParanoidFragmentSink::AddLeaf(const nsIParserNode& aNode)
{
  NS_ENSURE_TRUE(mNodeInfoManager, NS_ERROR_NOT_INITIALIZED);
  
  nsresult rv = NS_OK;

  if (mSkip) {
    return rv;
  }
  
  if (aNode.GetTokenType() == eToken_start) {
    nsCOMPtr<nsIAtom> name;
    rv = NameFromNode(aNode, getter_AddRefs(name));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (name == nsGkAtoms::base) {
      nsCOMPtr<nsIContent> content;
      nsCOMPtr<nsINodeInfo> nodeInfo;
      nsIParserService* parserService = nsContentUtils::GetParserService();
      if (!parserService)
        return NS_ERROR_OUT_OF_MEMORY;
      nodeInfo = mNodeInfoManager->GetNodeInfo(name, nsnull,
                                               kNameSpaceID_XHTML);
      NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
      rv = NS_NewHTMLElement(getter_AddRefs(content), nodeInfo, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
      AddAttributes(aNode, content);
      ProcessBaseTag(content);
      return NS_OK;
    }

    if (!sAllowedTags || !sAllowedTags->GetEntry(name)) {
      return NS_OK;
    }
  }

  return nsHTMLFragmentContentSink::AddLeaf(aNode);
}

NS_IMETHODIMP
nsHTMLParanoidFragmentSink::AddComment(const nsIParserNode& aNode)
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLParanoidFragmentSink::AddProcessingInstruction(const nsIParserNode& aNode)
{
  
  return NS_OK;
}
