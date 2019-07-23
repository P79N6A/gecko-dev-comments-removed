





































#include "mozInlineSpellWordUtil.h"
#include "nsDebug.h"
#include "nsIAtom.h"
#include "nsComponentManagerUtils.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMDocumentView.h"
#include "nsIDOMElement.h"
#include "nsIDOMNSRange.h"
#include "nsIDOMRange.h"
#include "nsIEditor.h"
#include "nsIDOMNode.h"
#include "nsIDOMHTMLBRElement.h"
#include "nsUnicharUtilCIID.h"
#include "nsServiceManagerUtils.h"
#include "nsIContent.h"





inline PRBool IsIgnorableCharacter(PRUnichar ch)
{
  return (ch == 0x200D || 
          ch == 0xAD ||   
          ch == 0x1806);  
}






inline PRBool IsConditionalPunctuation(PRUnichar ch)
{
  return (ch == '\'' ||
          ch == 0x2019); 
}



nsresult
mozInlineSpellWordUtil::Init(nsWeakPtr aWeakEditor)
{
  nsresult rv;

  mCategories = do_GetService(NS_UNICHARCATEGORY_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;
  
  
  
  nsCOMPtr<nsIEditor> editor = do_QueryReferent(aWeakEditor, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = editor->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_SUCCESS(rv, rv);

  mDocument = do_QueryInterface(domDoc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mDOMDocumentRange = do_QueryInterface(domDoc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIDOMDocumentView> docView = do_QueryInterface(domDoc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIDOMAbstractView> abstractView;
  rv = docView->GetDefaultView(getter_AddRefs(abstractView));
  NS_ENSURE_SUCCESS(rv, rv);
  mCSSView = do_QueryInterface(abstractView, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIDOMElement> rootElt;
  rv = editor->GetRootElement(getter_AddRefs(rootElt));
  NS_ENSURE_SUCCESS(rv, rv);
  
  mRootNode = rootElt;
  NS_ASSERTION(mRootNode, "GetRootElement returned null *and* claimed to suceed!");
  return NS_OK;
}

static PRBool
IsTextNode(nsIDOMNode* aNode)
{
  PRUint16 type = 0;
  aNode->GetNodeType(&type);
  return type == nsIDOMNode::TEXT_NODE;
}

typedef void (* OnLeaveNodeFunPtr)(nsIDOMNode* aNode, void* aClosure);




static nsIDOMNode*
FindNextNode(nsIDOMNode* aNode, nsIDOMNode* aRoot,
             OnLeaveNodeFunPtr aOnLeaveNode = nsnull, void* aClosure = nsnull)
{
  NS_PRECONDITION(aNode, "Null starting node?");

  nsCOMPtr<nsIDOMNode> next;
  aNode->GetFirstChild(getter_AddRefs(next));
  if (next)
    return next;
  
  
  if (aNode == aRoot)
    return nsnull;

  aNode->GetNextSibling(getter_AddRefs(next));
  if (next)
    return next;

  
  for (;;) {
    if (aOnLeaveNode) {
      aOnLeaveNode(aNode, aClosure);
    }
    
    aNode->GetParentNode(getter_AddRefs(next));
    if (next == aRoot || ! next)
      return nsnull;
    aNode = next;
    
    aNode->GetNextSibling(getter_AddRefs(next));
    if (next)
      return next;
  }
}



static nsIDOMNode*
FindNextTextNode(nsIDOMNode* aNode, PRInt32 aOffset, nsIDOMNode* aRoot)
{
  NS_PRECONDITION(aNode, "Null starting node?");
  NS_ASSERTION(!IsTextNode(aNode), "FindNextTextNode should start with a non-text node");

  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  nsCOMPtr<nsIDOMNode> checkNode;
  
  nsIContent* child = node->GetChildAt(aOffset);

  if (child) {
    checkNode = do_QueryInterface(child);
  } else {
    
    
    nsINode* next = node->GetSibling(1);
    if (!next) {
      nsCOMPtr<nsINode> root = do_QueryInterface(aRoot);
      while (!next) {
        
        next = node->GetNodeParent();
        if (next == root || !next) {
          return nsnull;
        }
        node = next;
        next = node->GetSibling(1);
      }
    }
    checkNode = do_QueryInterface(next);
  }
  
  while (checkNode && !IsTextNode(checkNode)) {
    checkNode = FindNextNode(checkNode, aRoot);
  }
  return checkNode.get();
}


















nsresult
mozInlineSpellWordUtil::SetEnd(nsIDOMNode* aEndNode, PRInt32 aEndOffset)
{
  NS_PRECONDITION(aEndNode, "Null end node?");

  NS_ASSERTION(mRootNode, "Not initialized");

  InvalidateWords();

  if (!IsTextNode(aEndNode)) {
    
    aEndNode = FindNextTextNode(aEndNode, aEndOffset, mRootNode);
    aEndOffset = 0;
  }
  mSoftEnd = NodeOffset(aEndNode, aEndOffset);
  return NS_OK;
}

nsresult
mozInlineSpellWordUtil::SetPosition(nsIDOMNode* aNode, PRInt32 aOffset)
{
  InvalidateWords();

  if (!IsTextNode(aNode)) {
    
    aNode = FindNextTextNode(aNode, aOffset, mRootNode);
    aOffset = 0;
  }
  mSoftBegin = NodeOffset(aNode, aOffset);

  EnsureWords();
  
  PRInt32 textOffset = MapDOMPositionToSoftTextOffset(mSoftBegin);
  if (textOffset < 0)
    return NS_OK;
  mNextWordIndex = FindRealWordContaining(textOffset, HINT_END, PR_TRUE);
  return NS_OK;
}

void
mozInlineSpellWordUtil::EnsureWords()
{
  if (mSoftTextValid)
    return;
  BuildSoftText();
  BuildRealWords();
  mSoftTextValid = PR_TRUE;
}

nsresult
mozInlineSpellWordUtil::MakeRangeForWord(const RealWord& aWord, nsIDOMRange** aRange)
{
  NodeOffset begin = MapSoftTextOffsetToDOMPosition(aWord.mSoftTextOffset, HINT_BEGIN);
  NodeOffset end = MapSoftTextOffsetToDOMPosition(aWord.EndOffset(), HINT_END);
  return MakeRange(begin, end, aRange);
}



nsresult
mozInlineSpellWordUtil::GetRangeForWord(nsIDOMNode* aWordNode,
                                        PRInt32 aWordOffset,
                                        nsIDOMRange** aRange)
{
  
  NodeOffset pt = NodeOffset(aWordNode, aWordOffset);
  
  InvalidateWords();
  mSoftBegin = mSoftEnd = pt;
  EnsureWords();
  
  PRInt32 offset = MapDOMPositionToSoftTextOffset(pt);
  if (offset < 0)
    return MakeRange(pt, pt, aRange);
  PRInt32 wordIndex = FindRealWordContaining(offset, HINT_BEGIN, PR_FALSE);
  if (wordIndex < 0)
    return MakeRange(pt, pt, aRange);
  return MakeRangeForWord(mRealWords[wordIndex], aRange);
}


static void
NormalizeWord(const nsSubstring& aInput, PRInt32 aPos, PRInt32 aLen, nsAString& aOutput)
{
  aOutput.Truncate();
  for (PRInt32 i = 0; i < aLen; i++) {
    PRUnichar ch = aInput.CharAt(i + aPos);

    
    if (IsIgnorableCharacter(ch))
      continue;

    
    if (ch == 0x2019) { 
      ch = '\'';
    }

    aOutput.Append(ch);
  }
}







nsresult
mozInlineSpellWordUtil::GetNextWord(nsAString& aText, nsIDOMRange** aRange,
                                    PRBool* aSkipChecking)
{
#ifdef DEBUG_SPELLCHECK
  printf("GetNextWord called; mNextWordIndex=%d\n", mNextWordIndex);
#endif

  if (mNextWordIndex < 0 ||
      mNextWordIndex >= PRInt32(mRealWords.Length())) {
    mNextWordIndex = -1;
    *aRange = nsnull;
    *aSkipChecking = PR_TRUE;
    return NS_OK;
  }
  
  const RealWord& word = mRealWords[mNextWordIndex];
  nsresult rv = MakeRangeForWord(word, aRange);
  NS_ENSURE_SUCCESS(rv, rv);
  ++mNextWordIndex;
  *aSkipChecking = !word.mCheckableWord;
  ::NormalizeWord(mSoftText, word.mSoftTextOffset, word.mLength, aText);

#ifdef DEBUG_SPELLCHECK
  printf("GetNextWord returning: %s (skip=%d)\n",
         NS_ConvertUTF16toUTF8(aText).get(), *aSkipChecking);
#endif
  
  return NS_OK;
}





nsresult
mozInlineSpellWordUtil::MakeRange(NodeOffset aBegin, NodeOffset aEnd,
                                  nsIDOMRange** aRange)
{
  if (! mDOMDocumentRange)
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv = mDOMDocumentRange->CreateRange(aRange);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = (*aRange)->SetStart(aBegin.mNode, aBegin.mOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = (*aRange)->SetEnd(aEnd.mNode, aEnd.mOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}












static PRBool
IsDOMWordSeparator(PRUnichar ch)
{
  
  if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
    return PR_TRUE;

  
  if (ch >= 0xA0 &&
      (ch == 0x00A0 ||  
       ch == 0x2002 ||  
       ch == 0x2003 ||  
       ch == 0x2009 ||  
       ch == 0x200C ||  
       ch == 0x3000))   
    return PR_TRUE;

  
  return PR_FALSE;
}

static PRBool
IsBRElement(nsIDOMNode* aNode)
{
  nsresult rv;
  nsCOMPtr<nsIDOMHTMLBRElement> elt = do_QueryInterface(aNode, &rv);
  return NS_SUCCEEDED(rv);
}

static void
GetNodeText(nsIDOMNode* aNode, nsAutoString& aText)
{
  nsresult rv = aNode->GetNodeValue(aText);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to get node text");
}



static nsIDOMNode*
FindPrevNode(nsIDOMNode* aNode, nsIDOMNode* aRoot)
{
  if (aNode == aRoot)
    return nsnull;
  
  nsCOMPtr<nsIDOMNode> prev;
  aNode->GetPreviousSibling(getter_AddRefs(prev));
  if (prev) {
    for (;;) {
      nsCOMPtr<nsIDOMNode> lastChild;
      prev->GetLastChild(getter_AddRefs(lastChild));
      if (!lastChild)
        return prev;
      prev = lastChild;
    }
  }

  
  
  aNode->GetParentNode(getter_AddRefs(prev));
  return prev;
}







static PRBool
ContainsDOMWordSeparator(nsIDOMNode* aNode, PRInt32 aBeforeOffset,
                         PRInt32* aSeparatorOffset)
{
  if (IsBRElement(aNode)) {
    *aSeparatorOffset = 0;
    return PR_TRUE;
  }
  
  if (!IsTextNode(aNode))
    return PR_FALSE;

  nsAutoString str;
  GetNodeText(aNode, str);
  for (PRInt32 i = PR_MIN(aBeforeOffset, PRInt32(str.Length())) - 1; i >= 0; --i) {
    if (IsDOMWordSeparator(str.CharAt(i))) {
      *aSeparatorOffset = i;
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

static PRBool
IsBreakElement(nsIDOMViewCSS* aDocView, nsIDOMNode* aNode)
{
  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aNode);
  if (!element)
    return PR_FALSE;
    
  if (IsBRElement(aNode))
    return PR_TRUE;
  
  nsCOMPtr<nsIDOMCSSStyleDeclaration> style;
  aDocView->GetComputedStyle(element, EmptyString(), getter_AddRefs(style));
  if (!style)
    return PR_FALSE;

#ifdef DEBUG_SPELLCHECK
  printf("    searching element %p\n", (void*)aNode);
#endif

  nsAutoString display;
  style->GetPropertyValue(NS_LITERAL_STRING("display"), display);
#ifdef DEBUG_SPELLCHECK
  printf("      display=\"%s\"\n", NS_ConvertUTF16toUTF8(display).get());
#endif
  if (!display.EqualsLiteral("inline"))
    return PR_TRUE;

  nsAutoString position;
  style->GetPropertyValue(NS_LITERAL_STRING("position"), position);
#ifdef DEBUG_SPELLCHECK
  printf("      position=%s\n", NS_ConvertUTF16toUTF8(position).get());
#endif
  if (!position.EqualsLiteral("static"))
    return PR_TRUE;
    
  
  return PR_FALSE;
}

struct CheckLeavingBreakElementClosure {
  nsIDOMViewCSS* mDocView;
  PRPackedBool   mLeftBreakElement;
};

static void
CheckLeavingBreakElement(nsIDOMNode* aNode, void* aClosure)
{
  CheckLeavingBreakElementClosure* cl =
    static_cast<CheckLeavingBreakElementClosure*>(aClosure);
  if (!cl->mLeftBreakElement && IsBreakElement(cl->mDocView, aNode)) {
    cl->mLeftBreakElement = PR_TRUE;
  }
}

void
mozInlineSpellWordUtil::NormalizeWord(nsSubstring& aWord)
{
  nsAutoString result;
  ::NormalizeWord(aWord, 0, aWord.Length(), result);
  aWord = result;
}

void
mozInlineSpellWordUtil::BuildSoftText()
{
  
  
  
  
  nsIDOMNode* node = mSoftBegin.mNode;
  PRInt32 firstOffsetInNode = 0;
  PRInt32 checkBeforeOffset = mSoftBegin.mOffset;
  while (node) {
    if (ContainsDOMWordSeparator(node, checkBeforeOffset, &firstOffsetInNode))
      break;
    checkBeforeOffset = PR_INT32_MAX;
    if (IsBreakElement(mCSSView, node)) {
      
      
      
      break;
    }
    node = FindPrevNode(node, mRootNode);
  }

  
  
  
  mSoftText.Truncate();
  mSoftTextDOMMapping.Clear();
  PRBool seenSoftEnd = PR_FALSE;
  
  
  nsAutoString str;
  while (node) {
    if (node == mSoftEnd.mNode) {
      seenSoftEnd = PR_TRUE;
    }

    PRBool exit = PR_FALSE;
    if (IsTextNode(node)) {
      GetNodeText(node, str);
      PRInt32 lastOffsetInNode = str.Length();

      if (seenSoftEnd) {
        
        for (PRInt32 i = node == mSoftEnd.mNode ? mSoftEnd.mOffset : 0;
             i < PRInt32(str.Length()); ++i) {
          if (IsDOMWordSeparator(str.CharAt(i))) {
            exit = PR_TRUE;
            
            lastOffsetInNode = i;
            break;
          }
        }
      }
      
      if (firstOffsetInNode < lastOffsetInNode) {
        PRInt32 len = lastOffsetInNode - firstOffsetInNode;
        mSoftTextDOMMapping.AppendElement(
          DOMTextMapping(NodeOffset(node, firstOffsetInNode), mSoftText.Length(), len));
        mSoftText.Append(Substring(str, firstOffsetInNode, len));
      }
      
      firstOffsetInNode = 0;
    }

    if (exit)
      break;

    CheckLeavingBreakElementClosure closure = { mCSSView, PR_FALSE };
    node = FindNextNode(node, mRootNode, CheckLeavingBreakElement, &closure);
    if (closure.mLeftBreakElement || (node && IsBreakElement(mCSSView, node))) {
      
      
      if (seenSoftEnd)
        break;
      
      mSoftText.Append(' ');
    }
  }
  
#ifdef DEBUG_SPELLCHECK
  printf("Got DOM string: %s\n", NS_ConvertUTF16toUTF8(mSoftText).get());
#endif
}

void
mozInlineSpellWordUtil::BuildRealWords()
{
  
  
  
  
  PRInt32 wordStart = -1;
  mRealWords.Clear();
  for (PRInt32 i = 0; i < PRInt32(mSoftText.Length()); ++i) {
    if (IsDOMWordSeparator(mSoftText.CharAt(i))) {
      if (wordStart >= 0) {
        SplitDOMWord(wordStart, i);
        wordStart = -1;
      }
    } else {
      if (wordStart < 0) {
        wordStart = i;
      }
    }
  }
  if (wordStart >= 0) {
    SplitDOMWord(wordStart, mSoftText.Length());
  }
}



PRInt32
mozInlineSpellWordUtil::MapDOMPositionToSoftTextOffset(NodeOffset aNodeOffset)
{
  if (!mSoftTextValid) {
    NS_ERROR("Soft text must be valid if we're to map into it");
    return -1;
  }
  
  for (PRInt32 i = 0; i < PRInt32(mSoftTextDOMMapping.Length()); ++i) {
    const DOMTextMapping& map = mSoftTextDOMMapping[i];
    if (map.mNodeOffset.mNode == aNodeOffset.mNode) {
      
      
      PRInt32 offsetInContributedString =
        aNodeOffset.mOffset - map.mNodeOffset.mOffset;
      if (offsetInContributedString >= 0 &&
          offsetInContributedString <= map.mLength)
        return map.mSoftTextOffset + offsetInContributedString;
      return -1;
    }
  }
  return -1;
}

mozInlineSpellWordUtil::NodeOffset
mozInlineSpellWordUtil::MapSoftTextOffsetToDOMPosition(PRInt32 aSoftTextOffset,
                                                       DOMMapHint aHint)
{
  NS_ASSERTION(mSoftTextValid, "Soft text must be valid if we're to map out of it");
  if (!mSoftTextValid)
    return NodeOffset(nsnull, -1);
  
  
  
  PRInt32 start = 0;
  PRInt32 end = mSoftTextDOMMapping.Length();
  while (end - start >= 2) {
    PRInt32 mid = (start + end)/2;
    const DOMTextMapping& map = mSoftTextDOMMapping[mid];
    if (map.mSoftTextOffset > aSoftTextOffset) {
      end = mid;
    } else {
      start = mid;
    }
  }
  
  if (start >= end)
    return NodeOffset(nsnull, -1);

  
  
  
  
  if (aHint == HINT_END && start > 0) {
    const DOMTextMapping& map = mSoftTextDOMMapping[start - 1];
    if (map.mSoftTextOffset + map.mLength == aSoftTextOffset)
      return NodeOffset(map.mNodeOffset.mNode, map.mNodeOffset.mOffset + map.mLength);
  }
  
  
  
  
  const DOMTextMapping& map = mSoftTextDOMMapping[start];
  PRInt32 offset = aSoftTextOffset - map.mSoftTextOffset;
  if (offset >= 0 && offset <= map.mLength)
    return NodeOffset(map.mNodeOffset.mNode, map.mNodeOffset.mOffset + offset);
    
  return NodeOffset(nsnull, -1);
}

PRInt32
mozInlineSpellWordUtil::FindRealWordContaining(PRInt32 aSoftTextOffset,
    DOMMapHint aHint, PRBool aSearchForward)
{
  NS_ASSERTION(mSoftTextValid, "Soft text must be valid if we're to map out of it");
  if (!mSoftTextValid)
    return -1;

  
  
  PRInt32 start = 0;
  PRInt32 end = mRealWords.Length();
  while (end - start >= 2) {
    PRInt32 mid = (start + end)/2;
    const RealWord& word = mRealWords[mid];
    if (word.mSoftTextOffset > aSoftTextOffset) {
      end = mid;
    } else {
      start = mid;
    }
  }
  
  if (start >= end)
    return -1;

  
  
  
  
  if (aHint == HINT_END && start > 0) {
    const RealWord& word = mRealWords[start - 1];
    if (word.mSoftTextOffset + word.mLength == aSoftTextOffset)
      return start - 1;
  }
  
  
  
  
  const RealWord& word = mRealWords[start];
  PRInt32 offset = aSoftTextOffset - word.mSoftTextOffset;
  if (offset >= 0 && offset <= word.mLength)
    return start;

  if (aSearchForward) {
    if (mRealWords[0].mSoftTextOffset > aSoftTextOffset) {
      
      return 0;
    }
    
    
    
    if (start + 1 < PRInt32(mRealWords.Length()))
      return start + 1;
  }

  return -1;
}




enum CharClass {
  CHAR_CLASS_WORD,
  CHAR_CLASS_SEPARATOR,
  CHAR_CLASS_END_OF_INPUT };


struct WordSplitState
{
  mozInlineSpellWordUtil*    mWordUtil;
  const nsDependentSubstring mDOMWordText;
  PRInt32                    mDOMWordOffset;
  CharClass                  mCurCharClass;

  WordSplitState(mozInlineSpellWordUtil* aWordUtil,
                 const nsString& aString, PRInt32 aStart, PRInt32 aLen)
    : mWordUtil(aWordUtil), mDOMWordText(aString, aStart, aLen),
      mDOMWordOffset(0), mCurCharClass(CHAR_CLASS_END_OF_INPUT) {}

  CharClass ClassifyCharacter(PRInt32 aIndex, PRBool aRecurse) const;
  void Advance();
  void AdvanceThroughSeparators();
  void AdvanceThroughWord();

  
  
  
  
  PRInt32 FindSpecialWord();

  
  
  
  PRBool ShouldSkipWord(PRInt32 aStart, PRInt32 aLength);
};



CharClass
WordSplitState::ClassifyCharacter(PRInt32 aIndex, PRBool aRecurse) const
{
  NS_ASSERTION(aIndex >= 0 && aIndex <= PRInt32(mDOMWordText.Length()),
               "Index out of range");
  if (aIndex == PRInt32(mDOMWordText.Length()))
    return CHAR_CLASS_SEPARATOR;

  
  
  nsIUGenCategory::nsUGenCategory
    charCategory = mWordUtil->GetCategories()->Get(PRUint32(mDOMWordText[aIndex]));
  if (charCategory == nsIUGenCategory::kLetter ||
      IsIgnorableCharacter(mDOMWordText[aIndex]))
    return CHAR_CLASS_WORD;

  
  
  if (IsConditionalPunctuation(mDOMWordText[aIndex])) {
    if (!aRecurse) {
      
      return CHAR_CLASS_SEPARATOR;
    }

    
    if (aIndex == 0)
      return CHAR_CLASS_SEPARATOR;
    if (ClassifyCharacter(aIndex - 1, false) != CHAR_CLASS_WORD)
      return CHAR_CLASS_SEPARATOR;

    
    if (aIndex == PRInt32(mDOMWordText.Length()) - 1)
      return CHAR_CLASS_SEPARATOR;
    if (ClassifyCharacter(aIndex + 1, false) != CHAR_CLASS_WORD)
      return CHAR_CLASS_SEPARATOR;

    
    return CHAR_CLASS_WORD;
  }

  
  if (charCategory == nsIUGenCategory::kSeparator ||
      charCategory == nsIUGenCategory::kOther ||
      charCategory == nsIUGenCategory::kPunctuation ||
      charCategory == nsIUGenCategory::kSymbol)
    return CHAR_CLASS_SEPARATOR;

  
  return CHAR_CLASS_WORD;
}




void
WordSplitState::Advance()
{
  NS_ASSERTION(mDOMWordOffset >= 0, "Negative word index");
  NS_ASSERTION(mDOMWordOffset < (PRInt32)mDOMWordText.Length(),
               "Length beyond end");

  mDOMWordOffset ++;
  if (mDOMWordOffset >= (PRInt32)mDOMWordText.Length())
    mCurCharClass = CHAR_CLASS_END_OF_INPUT;
  else
    mCurCharClass = ClassifyCharacter(mDOMWordOffset, PR_TRUE);
}




void
WordSplitState::AdvanceThroughSeparators()
{
  while (mCurCharClass == CHAR_CLASS_SEPARATOR)
    Advance();
}



void
WordSplitState::AdvanceThroughWord()
{
  while (mCurCharClass == CHAR_CLASS_WORD)
    Advance();
}




PRInt32
WordSplitState::FindSpecialWord()
{
  PRInt32 i;

  
  
  
  
  
  PRBool foundDot = PR_FALSE;
  PRInt32 firstColon = -1;
  for (i = mDOMWordOffset;
       i < PRInt32(mDOMWordText.Length()); i ++) {
    if (mDOMWordText[i] == '@') {
      
      
      

      
      
      
      
      
      
      if (i > 0 && ClassifyCharacter(i - 1, PR_FALSE) == CHAR_CLASS_WORD &&
          i < (PRInt32)mDOMWordText.Length() - 1 &&
          ClassifyCharacter(i + 1, PR_FALSE) == CHAR_CLASS_WORD)

      return mDOMWordText.Length() - mDOMWordOffset;
    } else if (mDOMWordText[i] == '.' && ! foundDot &&
        i > 0 && i < (PRInt32)mDOMWordText.Length() - 1) {
      
      foundDot = PR_TRUE;
    } else if (mDOMWordText[i] == ':' && firstColon < 0) {
      firstColon = i;
    }
  }

  
  
  if (firstColon >= 0 && firstColon < (PRInt32)mDOMWordText.Length() - 1 &&
      mDOMWordText[firstColon + 1] == '/') {
    return mDOMWordText.Length() - mDOMWordOffset;
  }

  
  
  
  
  if (firstColon > mDOMWordOffset) {
    nsString protocol(Substring(mDOMWordText, mDOMWordOffset,
                      firstColon - mDOMWordOffset));
    if (protocol.EqualsIgnoreCase("http") ||
        protocol.EqualsIgnoreCase("https") ||
        protocol.EqualsIgnoreCase("news") ||
        protocol.EqualsIgnoreCase("ftp") ||
        protocol.EqualsIgnoreCase("file") ||
        protocol.EqualsIgnoreCase("javascript") ||
        protocol.EqualsIgnoreCase("ftp")) {
      return mDOMWordText.Length() - mDOMWordOffset;
    }
  }

  
  return -1;
}



PRBool
WordSplitState::ShouldSkipWord(PRInt32 aStart, PRInt32 aLength)
{
  PRInt32 last = aStart + aLength;

  
  for (PRInt32 i = aStart; i < last; i ++) {
    PRUnichar ch = mDOMWordText[i];
    
    if (ch >= '0' && ch <= '9')
      return PR_TRUE;
  }

  
  return PR_FALSE;
}



void
mozInlineSpellWordUtil::SplitDOMWord(PRInt32 aStart, PRInt32 aEnd)
{
  WordSplitState state(this, mSoftText, aStart, aEnd - aStart);
  state.mCurCharClass = state.ClassifyCharacter(0, PR_TRUE);

  while (state.mCurCharClass != CHAR_CLASS_END_OF_INPUT) {
    state.AdvanceThroughSeparators();
    if (state.mCurCharClass == CHAR_CLASS_END_OF_INPUT)
      break;

    PRInt32 specialWordLength = state.FindSpecialWord();
    if (specialWordLength > 0) {
      mRealWords.AppendElement(
        RealWord(aStart + state.mDOMWordOffset, specialWordLength, PR_FALSE));

      
      state.mDOMWordOffset += specialWordLength;
      if (state.mDOMWordOffset + aStart >= aEnd)
        state.mCurCharClass = CHAR_CLASS_END_OF_INPUT;
      else
        state.mCurCharClass = state.ClassifyCharacter(state.mDOMWordOffset, PR_TRUE);
      continue;
    }

    
    PRInt32 wordOffset = state.mDOMWordOffset;

    
    state.AdvanceThroughWord();
    PRInt32 wordLen = state.mDOMWordOffset - wordOffset;
    mRealWords.AppendElement(
      RealWord(aStart + wordOffset, wordLen,
               !state.ShouldSkipWord(wordOffset, wordLen)));
  }
}
