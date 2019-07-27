






#include "nsFind.h"
#include "nsContentCID.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsIFrame.h"
#include "nsITextControlFrame.h"
#include "nsIFormControl.h"
#include "nsIEditor.h"
#include "nsIPlaintextEditor.h"
#include "nsTextFragment.h"
#include "nsString.h"
#include "nsIAtom.h"
#include "nsServiceManagerUtils.h"
#include "nsUnicharUtils.h"
#include "nsIDOMElement.h"
#include "nsIWordBreaker.h"
#include "nsCRT.h"
#include "nsRange.h"
#include "nsContentUtils.h"
#include "mozilla/DebugOnly.h"

using namespace mozilla;


#define CHAR_TO_UNICHAR(c) ((char16_t)(const unsigned char)c)

static NS_DEFINE_CID(kCContentIteratorCID, NS_CONTENTITERATOR_CID);
static NS_DEFINE_CID(kCPreContentIteratorCID, NS_PRECONTENTITERATOR_CID);

#define CH_QUOTE ((char16_t) 0x22)
#define CH_APOSTROPHE ((char16_t) 0x27)
#define CH_LEFT_SINGLE_QUOTE ((char16_t) 0x2018)
#define CH_RIGHT_SINGLE_QUOTE ((char16_t) 0x2019)
#define CH_LEFT_DOUBLE_QUOTE ((char16_t) 0x201C)
#define CH_RIGHT_DOUBLE_QUOTE ((char16_t) 0x201D)

#define CH_SHY ((char16_t) 0xAD)



PR_STATIC_ASSERT(CH_SHY <= 255);






























class nsFindContentIterator : public nsIContentIterator
{
public:
  explicit nsFindContentIterator(bool aFindBackward)
    : mStartOffset(0),
      mEndOffset(0),
      mFindBackward(aFindBackward)
  {
  }

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(nsFindContentIterator)

  
  virtual nsresult Init(nsINode* aRoot)
  {
    NS_NOTREACHED("internal error");
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  virtual nsresult Init(nsIDOMRange* aRange)
  {
    NS_NOTREACHED("internal error");
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  
  nsresult Init(nsIDOMNode* aStartNode, int32_t aStartOffset,
                nsIDOMNode* aEndNode, int32_t aEndOffset);
  virtual void First();
  virtual void Last();
  virtual void Next();
  virtual void Prev();
  virtual nsINode* GetCurrentNode();
  virtual bool IsDone();
  virtual nsresult PositionAt(nsINode* aCurNode);

protected:
  virtual ~nsFindContentIterator()
  {
  }

private:
  nsCOMPtr<nsIContentIterator> mOuterIterator;
  nsCOMPtr<nsIContentIterator> mInnerIterator;
  
  
  nsCOMPtr<nsIDOMNode> mStartNode;
  int32_t mStartOffset;
  nsCOMPtr<nsIDOMNode> mEndNode;
  int32_t mEndOffset;

  nsCOMPtr<nsIContent> mStartOuterContent;
  nsCOMPtr<nsIContent> mEndOuterContent;
  bool mFindBackward;

  void Reset();
  void MaybeSetupInnerIterator();
  void SetupInnerIterator(nsIContent* aContent);
};

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFindContentIterator)
  NS_INTERFACE_MAP_ENTRY(nsIContentIterator)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFindContentIterator)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFindContentIterator)

NS_IMPL_CYCLE_COLLECTION(nsFindContentIterator, mOuterIterator, mInnerIterator,
                         mStartOuterContent, mEndOuterContent, mEndNode, mStartNode)


nsresult
nsFindContentIterator::Init(nsIDOMNode* aStartNode, int32_t aStartOffset,
                            nsIDOMNode* aEndNode, int32_t aEndOffset)
{
  NS_ENSURE_ARG_POINTER(aStartNode);
  NS_ENSURE_ARG_POINTER(aEndNode);
  if (!mOuterIterator) {
    if (mFindBackward) {
      
      
      
      mOuterIterator = do_CreateInstance(kCContentIteratorCID);
    }
    else {
      
      
      
      mOuterIterator = do_CreateInstance(kCPreContentIteratorCID);
    }
    NS_ENSURE_ARG_POINTER(mOuterIterator);
  }

  
  mStartNode = aStartNode;
  mStartOffset = aStartOffset;
  mEndNode = aEndNode;
  mEndOffset = aEndOffset;

  return NS_OK;
}

void
nsFindContentIterator::First()
{
  Reset();
}

void
nsFindContentIterator::Last()
{
  Reset();
}

void
nsFindContentIterator::Next()
{
  if (mInnerIterator) {
    mInnerIterator->Next();
    if (!mInnerIterator->IsDone())
      return;

    
  }
  else {
    mOuterIterator->Next();
  }
  MaybeSetupInnerIterator();
}

void
nsFindContentIterator::Prev()
{
  if (mInnerIterator) {
    mInnerIterator->Prev();
    if (!mInnerIterator->IsDone())
      return;

    
  }
  else {
    mOuterIterator->Prev();
  }
  MaybeSetupInnerIterator();
}

nsINode*
nsFindContentIterator::GetCurrentNode()
{
  if (mInnerIterator && !mInnerIterator->IsDone()) {
    return mInnerIterator->GetCurrentNode();
  }
  return mOuterIterator->GetCurrentNode();
}

bool
nsFindContentIterator::IsDone() {
  if (mInnerIterator && !mInnerIterator->IsDone()) {
    return false;
  }
  return mOuterIterator->IsDone();
}

nsresult
nsFindContentIterator::PositionAt(nsINode* aCurNode)
{
  nsINode* oldNode = mOuterIterator->GetCurrentNode();
  nsresult rv = mOuterIterator->PositionAt(aCurNode);
  if (NS_SUCCEEDED(rv)) {
    MaybeSetupInnerIterator();
  }
  else {
    mOuterIterator->PositionAt(oldNode);
    if (mInnerIterator)
      rv = mInnerIterator->PositionAt(aCurNode);
  }
  return rv;
}

void
nsFindContentIterator::Reset()
{
  mInnerIterator = nullptr;
  mStartOuterContent = nullptr;
  mEndOuterContent = nullptr;

  
  

  
  nsCOMPtr<nsIContent> startContent(do_QueryInterface(mStartNode));
  if (startContent) {
    mStartOuterContent = startContent->FindFirstNonChromeOnlyAccessContent();
  }

  
  nsCOMPtr<nsIContent> endContent(do_QueryInterface(mEndNode));
  if (endContent) {
    mEndOuterContent = endContent->FindFirstNonChromeOnlyAccessContent();
  }

  
  
  
  nsCOMPtr<nsINode> node = do_QueryInterface(mStartNode);
  NS_ENSURE_TRUE_VOID(node);

  nsCOMPtr<nsIDOMRange> range = nsFind::CreateRange(node);
  range->SetStart(mStartNode, mStartOffset);
  range->SetEnd(mEndNode, mEndOffset);
  mOuterIterator->Init(range);

  if (!mFindBackward) {
    if (mStartOuterContent != startContent) {
      
      SetupInnerIterator(mStartOuterContent);
      if (mInnerIterator)
        mInnerIterator->First();
    }
    if (!mOuterIterator->IsDone())
      mOuterIterator->First();
  }
  else {
    if (mEndOuterContent != endContent) {
      
      SetupInnerIterator(mEndOuterContent);
      if (mInnerIterator)
        mInnerIterator->Last();
    }
    if (!mOuterIterator->IsDone())
      mOuterIterator->Last();
  }

  
  
  if (!mInnerIterator) {
    MaybeSetupInnerIterator();
  }
}

void
nsFindContentIterator::MaybeSetupInnerIterator()
{
  mInnerIterator = nullptr;

  nsCOMPtr<nsIContent> content =
    do_QueryInterface(mOuterIterator->GetCurrentNode());
  if (!content || !content->IsNodeOfType(nsINode::eHTML_FORM_CONTROL))
    return;

  nsCOMPtr<nsIFormControl> formControl(do_QueryInterface(content));
  if (!formControl->IsTextControl(true)) {
    return;
  }

  SetupInnerIterator(content);
  if (mInnerIterator) {
    if (!mFindBackward) {
      mInnerIterator->First();
      
      
      if (!mOuterIterator->IsDone())
        mOuterIterator->First();
    }
    else {
      mInnerIterator->Last();
      
      
      if (!mOuterIterator->IsDone())
        mOuterIterator->Last();
    }
  }
}

void
nsFindContentIterator::SetupInnerIterator(nsIContent* aContent)
{
  if (!aContent) {
    return;
  }
  NS_ASSERTION(!aContent->IsRootOfNativeAnonymousSubtree(), "invalid call");

  nsITextControlFrame* tcFrame = do_QueryFrame(aContent->GetPrimaryFrame());
  if (!tcFrame)
    return;

  nsCOMPtr<nsIEditor> editor;
  tcFrame->GetEditor(getter_AddRefs(editor));
  if (!editor)
    return;

  
  uint32_t editorFlags = 0;
  editor->GetFlags(&editorFlags);
  if (editorFlags & nsIPlaintextEditor::eEditorDisabledMask)
    return;

  nsCOMPtr<nsIDOMElement> rootElement;
  editor->GetRootElement(getter_AddRefs(rootElement));

  nsCOMPtr<nsIDOMRange> innerRange = nsFind::CreateRange(aContent);
  nsCOMPtr<nsIDOMRange> outerRange = nsFind::CreateRange(aContent);
  if (!innerRange || !outerRange) {
    return;
  }

  
  mInnerIterator = do_CreateInstance(kCPreContentIteratorCID);

  if (mInnerIterator) {
    innerRange->SelectNodeContents(rootElement);

    
    
    if (aContent == mStartOuterContent) {
      innerRange->SetStart(mStartNode, mStartOffset);
    }
    if (aContent == mEndOuterContent) {
      innerRange->SetEnd(mEndNode, mEndOffset);
    }
    
    mInnerIterator->Init(innerRange);

    
    
    nsresult res1, res2;
    nsCOMPtr<nsIDOMNode> outerNode(do_QueryInterface(aContent));
    if (!mFindBackward) { 
      
      res1 = outerRange->SetEnd(mEndNode, mEndOffset);
      res2 = outerRange->SetStartAfter(outerNode);
    }
    else { 
      
      res1 = outerRange->SetStart(mStartNode, mStartOffset);
      res2 = outerRange->SetEndBefore(outerNode);
    }
    if (NS_FAILED(res1) || NS_FAILED(res2)) {
      
      
      outerRange->Collapse(true);
    }

    
    
    
    
    mOuterIterator->Init(outerRange);
  }
}

nsresult
NS_NewFindContentIterator(bool aFindBackward,
                          nsIContentIterator** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }

  nsFindContentIterator* it = new nsFindContentIterator(aFindBackward);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return it->QueryInterface(NS_GET_IID(nsIContentIterator), (void **)aResult);
}


NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFind)
  NS_INTERFACE_MAP_ENTRY(nsIFind)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFind)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFind)

  NS_IMPL_CYCLE_COLLECTION(nsFind, mLastBlockParent, mIterNode, mIterator)

nsFind::nsFind()
  : mFindBackward(false)
  , mCaseSensitive(false)
  , mIterOffset(0)
{
}

nsFind::~nsFind()
{
}

#ifdef DEBUG_FIND
static void DumpNode(nsIDOMNode* aNode)
{
  if (!aNode)
  {
    printf(">>>> Node: NULL\n");
    return;
  }
  nsAutoString nodeName;
  aNode->GetNodeName(nodeName);
  nsCOMPtr<nsIContent> textContent (do_QueryInterface(aNode));
  if (textContent && textContent->IsNodeOfType(nsINode::eTEXT))
  {
    nsAutoString newText;
    textContent->AppendTextTo(newText);
    printf(">>>> Text node (node name %s): '%s'\n",
           NS_LossyConvertUTF16toASCII(nodeName).get(),
           NS_LossyConvertUTF16toASCII(newText).get());
  }
  else
    printf(">>>> Node: %s\n", NS_LossyConvertUTF16toASCII(nodeName).get());
}
#endif

nsresult
nsFind::InitIterator(nsIDOMNode* aStartNode, int32_t aStartOffset,
                     nsIDOMNode* aEndNode, int32_t aEndOffset)
{
  if (!mIterator)
  {
    mIterator = new nsFindContentIterator(mFindBackward);
    NS_ENSURE_TRUE(mIterator, NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ENSURE_ARG_POINTER(aStartNode);
  NS_ENSURE_ARG_POINTER(aEndNode);

#ifdef DEBUG_FIND
  printf("InitIterator search range:\n");
  printf(" -- start %d, ", aStartOffset); DumpNode(aStartNode);
  printf(" -- end %d, ", aEndOffset); DumpNode(aEndNode);
#endif

  nsresult rv =
    mIterator->Init(aStartNode, aStartOffset, aEndNode, aEndOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  if (mFindBackward) {
    mIterator->Last();
  }
  else {
    mIterator->First();
  }
  return NS_OK;
}


NS_IMETHODIMP
nsFind::GetFindBackwards(bool *aFindBackward)
{
  if (!aFindBackward)
    return NS_ERROR_NULL_POINTER;

  *aFindBackward = mFindBackward;
  return NS_OK;
}
NS_IMETHODIMP
nsFind::SetFindBackwards(bool aFindBackward)
{
  mFindBackward = aFindBackward;
  return NS_OK;
}


NS_IMETHODIMP
nsFind::GetCaseSensitive(bool *aCaseSensitive)
{
  if (!aCaseSensitive)
    return NS_ERROR_NULL_POINTER;

  *aCaseSensitive = mCaseSensitive;
  return NS_OK;
}
NS_IMETHODIMP
nsFind::SetCaseSensitive(bool aCaseSensitive)
{
  mCaseSensitive = aCaseSensitive;
  return NS_OK;
}

NS_IMETHODIMP
nsFind::GetWordBreaker(nsIWordBreaker** aWordBreaker)
{
  *aWordBreaker = mWordBreaker;
  NS_IF_ADDREF(*aWordBreaker);
  return NS_OK;
}

NS_IMETHODIMP
nsFind::SetWordBreaker(nsIWordBreaker* aWordBreaker)
{
  mWordBreaker = aWordBreaker;
  return NS_OK;
}


















nsresult
nsFind::NextNode(nsIDOMRange* aSearchRange,
                 nsIDOMRange* aStartPoint, nsIDOMRange* aEndPoint,
                 bool aContinueOk)
{
  nsresult rv;

  nsCOMPtr<nsIContent> content;

  if (!mIterator || aContinueOk)
  {
    
    
    
    nsCOMPtr<nsIDOMNode> startNode;
    nsCOMPtr<nsIDOMNode> endNode;
    int32_t startOffset, endOffset;
    if (aContinueOk)
    {
#ifdef DEBUG_FIND
      printf("Match in progress: continuing past endpoint\n");
#endif
      if (mFindBackward) {
        aSearchRange->GetStartContainer(getter_AddRefs(startNode));
        aSearchRange->GetStartOffset(&startOffset);
        aEndPoint->GetStartContainer(getter_AddRefs(endNode));
        aEndPoint->GetStartOffset(&endOffset);
      } else {     
        aEndPoint->GetEndContainer(getter_AddRefs(startNode));
        aEndPoint->GetEndOffset(&startOffset);
        aSearchRange->GetEndContainer(getter_AddRefs(endNode));
        aSearchRange->GetEndOffset(&endOffset);
      }
    }
    else  
    {
      if (mFindBackward) {
        aSearchRange->GetStartContainer(getter_AddRefs(startNode));
        aSearchRange->GetStartOffset(&startOffset);
        aStartPoint->GetEndContainer(getter_AddRefs(endNode));
        aStartPoint->GetEndOffset(&endOffset);
        
        
        
        
        
        
        
      } else {     
        aStartPoint->GetStartContainer(getter_AddRefs(startNode));
        aStartPoint->GetStartOffset(&startOffset);
        aEndPoint->GetEndContainer(getter_AddRefs(endNode));
        aEndPoint->GetEndOffset(&endOffset);
      }
    }

    rv = InitIterator(startNode, startOffset, endNode, endOffset);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!aStartPoint)
      aStartPoint = aSearchRange;

    content = do_QueryInterface(mIterator->GetCurrentNode());
#ifdef DEBUG_FIND
    nsCOMPtr<nsIDOMNode> dnode (do_QueryInterface(content));
    printf(":::::: Got the first node "); DumpNode(dnode);
#endif
    if (content && content->IsNodeOfType(nsINode::eTEXT) &&
        !SkipNode(content))
    {
      mIterNode = do_QueryInterface(content);
      
      nsCOMPtr<nsIDOMNode> node;
      if (mFindBackward) {
        aStartPoint->GetEndContainer(getter_AddRefs(node));
        if (mIterNode.get() == node.get())
          aStartPoint->GetEndOffset(&mIterOffset);
        else
          mIterOffset = -1;   
      }
      else
      {
        aStartPoint->GetStartContainer(getter_AddRefs(node));
        if (mIterNode.get() == node.get())
          aStartPoint->GetStartOffset(&mIterOffset);
        else
          mIterOffset = 0;
      }
#ifdef DEBUG_FIND
      printf("Setting initial offset to %d\n", mIterOffset);
#endif
      return NS_OK;
    }
  }

  while (1)
  {
    if (mFindBackward)
      mIterator->Prev();
    else
      mIterator->Next();

    content = do_QueryInterface(mIterator->GetCurrentNode());
    if (!content)
      break;

#ifdef DEBUG_FIND
    nsCOMPtr<nsIDOMNode> dnode (do_QueryInterface(content));
    printf(":::::: Got another node "); DumpNode(dnode);
#endif

    
    
    
    
    
    
    

    
    
    
    
    if (SkipNode(content))
      continue;

    if (content->IsNodeOfType(nsINode::eTEXT))
      break;
#ifdef DEBUG_FIND
    dnode = do_QueryInterface(content);
    printf("Not a text node: "); DumpNode(dnode);
#endif
  }

  if (content)
    mIterNode = do_QueryInterface(content);
  else
    mIterNode = nullptr;
  mIterOffset = -1;

#ifdef DEBUG_FIND
  printf("Iterator gave: "); DumpNode(mIterNode);
#endif
  return NS_OK;
}

bool nsFind::IsBlockNode(nsIContent* aContent)
{
  if (!aContent->IsHTML()) {
    return false;
  }

  nsIAtom *atom = aContent->Tag();

  if (atom == nsGkAtoms::img ||
      atom == nsGkAtoms::hr ||
      atom == nsGkAtoms::th ||
      atom == nsGkAtoms::td)
    return true;

  return nsContentUtils::IsHTMLBlock(atom);
}

bool nsFind::IsTextNode(nsIDOMNode* aNode)
{
  uint16_t nodeType;
  aNode->GetNodeType(&nodeType);

  return nodeType == nsIDOMNode::TEXT_NODE ||
         nodeType == nsIDOMNode::CDATA_SECTION_NODE;
}

bool nsFind::IsVisibleNode(nsIDOMNode *aDOMNode)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aDOMNode));
  if (!content)
    return false;

  nsIFrame *frame = content->GetPrimaryFrame();
  if (!frame) {
    
    return false;
  }

  return frame->StyleVisibility()->IsVisible();
}

bool nsFind::SkipNode(nsIContent* aContent)
{
  nsIAtom *atom;

#ifdef HAVE_BIDI_ITERATOR
  atom = aContent->Tag();

  
  
  return (aContent->IsNodeOfType(nsINode::eCOMMENT) ||
          (aContent->IsHTML() &&
           (atom == sScriptAtom ||
            atom == sNoframesAtom ||
            atom == sSelectAtom)));

#else 
  
  
  
  

  nsIContent *content = aContent;
  while (content)
  {
    atom = content->Tag();

    if (aContent->IsNodeOfType(nsINode::eCOMMENT) ||
        (content->IsHTML() &&
         (atom == nsGkAtoms::script ||
          atom == nsGkAtoms::noframes ||
          atom == nsGkAtoms::select)))
    {
#ifdef DEBUG_FIND
      printf("Skipping node: ");
      nsCOMPtr<nsIDOMNode> node (do_QueryInterface(content));
      DumpNode(node);
#endif

      return true;
    }

    
    if (IsBlockNode(content))
      return false;

    content = content->GetParent();
  }

  return false;
#endif 
}

nsresult nsFind::GetBlockParent(nsIDOMNode* aNode, nsIDOMNode** aParent)
{
  while (aNode)
  {
    nsCOMPtr<nsIDOMNode> parent;
    nsresult rv = aNode->GetParentNode(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIContent> content (do_QueryInterface(parent));
    if (content && IsBlockNode(content))
    {
      *aParent = parent;
      NS_ADDREF(*aParent);
      return NS_OK;
    }
    aNode = parent;
  }
  return NS_ERROR_FAILURE;
}



void nsFind::ResetAll()
{
  mIterator = nullptr;
  mLastBlockParent = nullptr;
}

#define NBSP_CHARCODE (CHAR_TO_UNICHAR(160))
#define IsSpace(c) (nsCRT::IsAsciiSpace(c) || (c) == NBSP_CHARCODE)
#define OVERFLOW_PINDEX (mFindBackward ? pindex < 0 : pindex > patLen)
#define DONE_WITH_PINDEX (mFindBackward ? pindex <= 0 : pindex >= patLen)
#define ALMOST_DONE_WITH_PINDEX (mFindBackward ? pindex <= 0 : pindex >= patLen-1)






NS_IMETHODIMP
nsFind::Find(const char16_t *aPatText, nsIDOMRange* aSearchRange,
             nsIDOMRange* aStartPoint, nsIDOMRange* aEndPoint,
             nsIDOMRange** aRangeRet)
{
#ifdef DEBUG_FIND
  printf("============== nsFind::Find('%s'%s, %p, %p, %p)\n",
         NS_LossyConvertUTF16toASCII(aPatText).get(),
         mFindBackward ? " (backward)" : " (forward)",
         (void*)aSearchRange, (void*)aStartPoint, (void*)aEndPoint);
#endif

  NS_ENSURE_ARG(aSearchRange);
  NS_ENSURE_ARG(aStartPoint);
  NS_ENSURE_ARG(aEndPoint);
  NS_ENSURE_ARG_POINTER(aRangeRet);
  *aRangeRet = 0;

  if (!aPatText)
    return NS_ERROR_NULL_POINTER;

  ResetAll();

  nsAutoString patAutoStr(aPatText);
  if (!mCaseSensitive)
    ToLowerCase(patAutoStr);

  
  static const char kShy[] = { char(CH_SHY), 0 };
  patAutoStr.StripChars(kShy);

  const char16_t* patStr = patAutoStr.get();
  int32_t patLen = patAutoStr.Length() - 1;

  
  int32_t pindex = (mFindBackward ? patLen : 0);

  
  int32_t findex = 0;

  
  int incr = (mFindBackward ? -1 : 1);

  nsCOMPtr<nsIContent> tc;
  const nsTextFragment *frag = nullptr;
  int32_t fragLen = 0;

  
  const char16_t *t2b = nullptr;
  const char      *t1b = nullptr;

  
  
  bool inWhitespace = false;

  
  nsCOMPtr<nsIDOMNode> matchAnchorNode;
  int32_t matchAnchorOffset = 0;

  
  nsCOMPtr<nsIDOMNode> endNode;
  int32_t endOffset;
  aEndPoint->GetEndContainer(getter_AddRefs(endNode));
  aEndPoint->GetEndOffset(&endOffset);

  char16_t prevChar = 0;
  while (1)
  {
#ifdef DEBUG_FIND
    printf("Loop ...\n");
#endif

    
    if (!frag)
    {

      tc = nullptr;
      NextNode(aSearchRange, aStartPoint, aEndPoint, false);
      if (!mIterNode)    
      {
        
        
        if (matchAnchorNode)
          NextNode(aSearchRange, aStartPoint, aEndPoint, true);

        
        
        ResetAll();
        return NS_OK;
      }

      
      
      
      
      nsCOMPtr<nsIDOMNode> blockParent;
      GetBlockParent(mIterNode, getter_AddRefs(blockParent));
#ifdef DEBUG_FIND
      printf("New node: old blockparent = %p, new = %p\n",
             (void*)mLastBlockParent.get(), (void*)blockParent.get());
#endif
      if (blockParent != mLastBlockParent)
      {
#ifdef DEBUG_FIND
        printf("Different block parent!\n");
#endif
        mLastBlockParent = blockParent;
        
        matchAnchorNode = nullptr;
        matchAnchorOffset = 0;
        pindex = (mFindBackward ? patLen : 0);
        inWhitespace = false;
      }

      
      tc = do_QueryInterface(mIterNode);
      if (!tc || !(frag = tc->GetText())) 
      {
        mIterator = nullptr;
        mLastBlockParent = 0;
        ResetAll();
        return NS_OK;
      }

      fragLen = frag->GetLength();

      
      
      
      if (mIterNode == matchAnchorNode)
        findex = matchAnchorOffset + (mFindBackward ? 1 : 0);

      
      
      
      
      
      else if (mIterOffset >= 0)
        findex = mIterOffset - (mFindBackward ? 1 : 0);

      
      else if (mFindBackward)
        findex = fragLen - 1;
      else
        findex = 0;

      
      mIterOffset = -1;

      
      if (findex < 0 || findex > fragLen-1)
      {
#ifdef DEBUG_FIND
        printf("At the end of a text node -- skipping to the next\n");
#endif
        frag = 0;
        continue;
      }

#ifdef DEBUG_FIND
      printf("Starting from offset %d\n", findex);
#endif
      if (frag->Is2b())
      {
        t2b = frag->Get2b();
        t1b = nullptr;
#ifdef DEBUG_FIND
        nsAutoString str2(t2b, fragLen);
        printf("2 byte, '%s'\n", NS_LossyConvertUTF16toASCII(str2).get());
#endif
      }
      else
      {
        t1b = frag->Get1b();
        t2b = nullptr;
#ifdef DEBUG_FIND
        nsAutoCString str1(t1b, fragLen);
        printf("1 byte, '%s'\n", str1.get());
#endif
      }
    }
    else 
    {
      
      
      findex += incr;
#ifdef DEBUG_FIND
      printf("Same node -- (%d, %d)\n", pindex, findex);
#endif
      if (mFindBackward ? (findex < 0) : (findex >= fragLen))
      {
#ifdef DEBUG_FIND
        printf("Will need to pull a new node: mAO = %d, frag len=%d\n",
               matchAnchorOffset, fragLen);
#endif
        
        frag = nullptr;
        continue;
      }
    }

    
    
    if (mIterNode == endNode &&
        ((mFindBackward && (findex < endOffset)) ||
         (!mFindBackward && (findex > endOffset))))
    {
      ResetAll();
      return NS_OK;
    }

    
    char16_t c = (t2b ? t2b[findex] : CHAR_TO_UNICHAR(t1b[findex]));
    char16_t patc = patStr[pindex];

#ifdef DEBUG_FIND
    printf("Comparing '%c'=%x to '%c' (%d of %d), findex=%d%s\n",
           (char)c, (int)c, patc, pindex, patLen, findex,
           inWhitespace ? " (inWhitespace)" : "");
#endif

    
    
    
    if (inWhitespace && !IsSpace(c))
    {
      inWhitespace = false;
      pindex += incr;
#ifdef DEBUG
      
      
      
      if (OVERFLOW_PINDEX)
        NS_ASSERTION(false, "Missed a whitespace match");
#endif
      patc = patStr[pindex];
    }
    if (!inWhitespace && IsSpace(patc))
      inWhitespace = true;

    
    else if (!inWhitespace && !mCaseSensitive && IsUpperCase(c))
      c = ToLowerCase(c);

    switch (c) {
      
      case CH_SHY:
        continue;
      
      case CH_LEFT_SINGLE_QUOTE:
      case CH_RIGHT_SINGLE_QUOTE:
        c = CH_APOSTROPHE;
        break;
      case CH_LEFT_DOUBLE_QUOTE:
      case CH_RIGHT_DOUBLE_QUOTE:
        c = CH_QUOTE;
        break;
    }

    switch (patc) {
      
      case CH_LEFT_SINGLE_QUOTE:
      case CH_RIGHT_SINGLE_QUOTE:
        patc = CH_APOSTROPHE;
        break;
      case CH_LEFT_DOUBLE_QUOTE:
      case CH_RIGHT_DOUBLE_QUOTE:
        patc = CH_QUOTE;
        break;
    }

    
    if (pindex != (mFindBackward ? patLen : 0) && c != patc && !inWhitespace) {
      if (c == '\n' && t2b && IS_CJ_CHAR(prevChar)) {
        int32_t nindex = findex + incr;
        if (mFindBackward ? (nindex >= 0) : (nindex < fragLen)) {
          if (IS_CJ_CHAR(t2b[nindex]))
            continue;
        }
      }
    }

    
    if ((c == patc) || (inWhitespace && IsSpace(c)))
    {
      prevChar = c;
#ifdef DEBUG_FIND
      if (inWhitespace)
        printf("YES (whitespace)(%d of %d)\n", pindex, patLen);
      else
        printf("YES! '%c' == '%c' (%d of %d)\n", c, patc, pindex, patLen);
#endif

      
      if (!matchAnchorNode) {
        matchAnchorNode = mIterNode;
        matchAnchorOffset = findex;
      }

      
      if (DONE_WITH_PINDEX)
        
      {
#ifdef DEBUG_FIND
        printf("Found a match!\n");
#endif

        
        nsCOMPtr<nsIDOMNode> startParent;
        nsCOMPtr<nsIDOMNode> endParent;
        nsCOMPtr<nsIDOMRange> range = CreateRange(tc);
        if (range)
        {
          int32_t matchStartOffset, matchEndOffset;
          
          int32_t mao = matchAnchorOffset + (mFindBackward ? 1 : 0);
          if (mFindBackward)
          {
            startParent = do_QueryInterface(tc);
            endParent = matchAnchorNode;
            matchStartOffset = findex;
            matchEndOffset = mao;
          }
          else
          {
            startParent = matchAnchorNode;
            endParent = do_QueryInterface(tc);
            matchStartOffset = mao;
            matchEndOffset = findex+1;
          }
          if (startParent && endParent &&
              IsVisibleNode(startParent) && IsVisibleNode(endParent))
          {
            range->SetStart(startParent, matchStartOffset);
            range->SetEnd(endParent, matchEndOffset);
            *aRangeRet = range.get();
            NS_ADDREF(*aRangeRet);
          }
          else {
            startParent = nullptr; 
          }
        }

        if (startParent) {
          
          
          
          mIterOffset = findex + (mFindBackward ? 1 : 0);
  #ifdef DEBUG_FIND
          printf("mIterOffset = %d, mIterNode = ", mIterOffset);
          DumpNode(mIterNode);
  #endif

          ResetAll();
          return NS_OK;
        }
        matchAnchorNode = nullptr;  
      }

      if (matchAnchorNode) {
        
        
        
        if (!inWhitespace || DONE_WITH_PINDEX || IsSpace(patStr[pindex+incr]))
        {
          pindex += incr;
          inWhitespace = false;
#ifdef DEBUG_FIND
          printf("Advancing pindex to %d\n", pindex);
#endif
        }

        continue;
      }
    }

#ifdef DEBUG_FIND
    printf("NOT: %c == %c\n", c, patc);
#endif

    
    
    
    if (matchAnchorNode)    
    {
      findex = matchAnchorOffset;
      mIterOffset = matchAnchorOffset;
          

      
      if (matchAnchorNode != mIterNode)
      {
        nsCOMPtr<nsIContent> content (do_QueryInterface(matchAnchorNode));
        DebugOnly<nsresult> rv = NS_ERROR_UNEXPECTED;
        if (content)
          rv = mIterator->PositionAt(content);
        frag = 0;
        NS_ASSERTION(NS_SUCCEEDED(rv), "Text content wasn't nsIContent!");
#ifdef DEBUG_FIND
        printf("Repositioned anchor node\n");
#endif
      }
#ifdef DEBUG_FIND
      printf("Ending a partial match; findex -> %d, mIterOffset -> %d\n",
             findex, mIterOffset);
#endif
    }
    matchAnchorNode = nullptr;
    matchAnchorOffset = 0;
    inWhitespace = false;
    pindex = (mFindBackward ? patLen : 0);
#ifdef DEBUG_FIND
    printf("Setting findex back to %d, pindex to %d\n", findex, pindex);

#endif
  } 

  
  ResetAll();
  return NS_OK;
}


already_AddRefed<nsIDOMRange>
nsFind::CreateRange(nsINode* aNode)
{
  nsRefPtr<nsRange> range = new nsRange(aNode);
  range->SetMaySpanAnonymousSubtrees(true);
  return range.forget();
}
