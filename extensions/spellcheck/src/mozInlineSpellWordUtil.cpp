






































#include "mozInlineSpellWordUtil.h"
#include "nsDebug.h"
#include "nsIAtom.h"
#include "nsComponentManagerUtils.h"
#include "nsIDOMCSSStyleDeclaration.h"
#include "nsIDOMElement.h"
#include "nsIDOMRange.h"
#include "nsIEditor.h"
#include "nsIDOMNode.h"
#include "nsIDOMHTMLBRElement.h"
#include "nsUnicharUtilCIID.h"
#include "nsUnicodeProperties.h"
#include "nsServiceManagerUtils.h"
#include "nsIContent.h"
#include "nsTextFragment.h"
#include "mozilla/dom/Element.h"
#include "nsIFrame.h"
#include "nsRange.h"
#include "nsContentUtils.h"

using namespace mozilla;





inline bool IsIgnorableCharacter(PRUnichar ch)
{
  return (ch == 0x200D || 
          ch == 0xAD ||   
          ch == 0x1806);  
}






inline bool IsConditionalPunctuation(PRUnichar ch)
{
  return (ch == '\'' ||
          ch == 0x2019); 
}



nsresult
mozInlineSpellWordUtil::Init(nsWeakPtr aWeakEditor)
{
  nsresult rv;

  
  
  nsCOMPtr<nsIEditor> editor = do_QueryReferent(aWeakEditor, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = editor->GetDocument(getter_AddRefs(domDoc));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(domDoc, NS_ERROR_NULL_POINTER);

  mDOMDocument = domDoc;
  mDocument = do_QueryInterface(domDoc);

  
  
  nsCOMPtr<nsIDOMElement> rootElt;
  rv = editor->GetRootElement(getter_AddRefs(rootElt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsINode> rootNode = do_QueryInterface(rootElt);
  mRootNode = rootNode;
  NS_ASSERTION(mRootNode, "GetRootElement returned null *and* claimed to suceed!");
  return NS_OK;
}

static inline bool
IsTextNode(nsINode* aNode)
{
  return aNode->IsNodeOfType(nsINode::eTEXT);
}

typedef void (* OnLeaveNodeFunPtr)(nsINode* aNode, void* aClosure);




static nsINode*
FindNextNode(nsINode* aNode, nsINode* aRoot,
             OnLeaveNodeFunPtr aOnLeaveNode, void* aClosure)
{
  NS_PRECONDITION(aNode, "Null starting node?");

  nsINode* next = aNode->GetFirstChild();
  if (next)
    return next;
  
  
  if (aNode == aRoot)
    return nsnull;

  next = aNode->GetNextSibling();
  if (next)
    return next;

  
  for (;;) {
    if (aOnLeaveNode) {
      aOnLeaveNode(aNode, aClosure);
    }
    
    next = aNode->GetParent();
    if (next == aRoot || ! next)
      return nsnull;
    aNode = next;
    
    next = aNode->GetNextSibling();
    if (next)
      return next;
  }
}



static nsINode*
FindNextTextNode(nsINode* aNode, PRInt32 aOffset, nsINode* aRoot)
{
  NS_PRECONDITION(aNode, "Null starting node?");
  NS_ASSERTION(!IsTextNode(aNode), "FindNextTextNode should start with a non-text node");

  nsINode* checkNode;
  
  nsIContent* child = aNode->GetChildAt(aOffset);

  if (child) {
    checkNode = child;
  } else {
    
    
    
    checkNode = aNode->GetNextNonChildNode(aRoot);
  }
  
  while (checkNode && !IsTextNode(checkNode)) {
    checkNode = checkNode->GetNextNode(aRoot);
  }
  return checkNode;
}


















nsresult
mozInlineSpellWordUtil::SetEnd(nsINode* aEndNode, PRInt32 aEndOffset)
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
mozInlineSpellWordUtil::SetPosition(nsINode* aNode, PRInt32 aOffset)
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
  mNextWordIndex = FindRealWordContaining(textOffset, HINT_END, true);
  return NS_OK;
}

void
mozInlineSpellWordUtil::EnsureWords()
{
  if (mSoftTextValid)
    return;
  BuildSoftText();
  BuildRealWords();
  mSoftTextValid = true;
}

nsresult
mozInlineSpellWordUtil::MakeRangeForWord(const RealWord& aWord, nsRange** aRange)
{
  NodeOffset begin = MapSoftTextOffsetToDOMPosition(aWord.mSoftTextOffset, HINT_BEGIN);
  NodeOffset end = MapSoftTextOffsetToDOMPosition(aWord.EndOffset(), HINT_END);
  return MakeRange(begin, end, aRange);
}



nsresult
mozInlineSpellWordUtil::GetRangeForWord(nsIDOMNode* aWordNode,
                                        PRInt32 aWordOffset,
                                        nsRange** aRange)
{
  
  nsCOMPtr<nsINode> wordNode = do_QueryInterface(aWordNode);
  NodeOffset pt = NodeOffset(wordNode, aWordOffset);
  
  InvalidateWords();
  mSoftBegin = mSoftEnd = pt;
  EnsureWords();
  
  PRInt32 offset = MapDOMPositionToSoftTextOffset(pt);
  if (offset < 0)
    return MakeRange(pt, pt, aRange);
  PRInt32 wordIndex = FindRealWordContaining(offset, HINT_BEGIN, false);
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
mozInlineSpellWordUtil::GetNextWord(nsAString& aText, nsRange** aRange,
                                    bool* aSkipChecking)
{
#ifdef DEBUG_SPELLCHECK
  printf("GetNextWord called; mNextWordIndex=%d\n", mNextWordIndex);
#endif

  if (mNextWordIndex < 0 ||
      mNextWordIndex >= PRInt32(mRealWords.Length())) {
    mNextWordIndex = -1;
    *aRange = nsnull;
    *aSkipChecking = true;
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
                                  nsRange** aRange)
{
  if (!mDOMDocument)
    return NS_ERROR_NOT_INITIALIZED;

  nsRefPtr<nsRange> range = new nsRange();
  nsresult rv = range->Set(aBegin.mNode, aBegin.mOffset,
                           aEnd.mNode, aEnd.mOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  range.forget(aRange);

  return NS_OK;
}












static bool
IsDOMWordSeparator(PRUnichar ch)
{
  
  if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
    return true;

  
  if (ch >= 0xA0 &&
      (ch == 0x00A0 ||  
       ch == 0x2002 ||  
       ch == 0x2003 ||  
       ch == 0x2009 ||  
       ch == 0x200C ||  
       ch == 0x3000))   
    return true;

  
  return false;
}

static inline bool
IsBRElement(nsINode* aNode)
{
  return aNode->IsElement() &&
         aNode->AsElement()->IsHTML(nsGkAtoms::br);
}









static bool
ContainsDOMWordSeparator(nsINode* aNode, PRInt32 aBeforeOffset,
                         PRInt32* aSeparatorOffset)
{
  if (IsBRElement(aNode)) {
    *aSeparatorOffset = 0;
    return true;
  }
  
  if (!IsTextNode(aNode))
    return false;

  
  nsIContent* content = static_cast<nsIContent*>(aNode);
  const nsTextFragment* textFragment = content->GetText();
  NS_ASSERTION(textFragment, "Where is our text?");
  for (PRInt32 i = NS_MIN(aBeforeOffset, PRInt32(textFragment->GetLength())) - 1; i >= 0; --i) {
    if (IsDOMWordSeparator(textFragment->CharAt(i))) {
      
      for (PRInt32 j = i - 1; j >= 0; --j) {
        if (IsDOMWordSeparator(textFragment->CharAt(j))) {
          i = j;
        } else {
          break;
        }
      }
      *aSeparatorOffset = i;
      return true;
    }
  }
  return false;
}

static bool
IsBreakElement(nsINode* aNode)
{
  if (!aNode->IsElement()) {
    return false;
  }

  dom::Element *element = aNode->AsElement();
    
  if (element->IsHTML(nsGkAtoms::br))
    return true;

  
  
  if (!element->GetPrimaryFrame())
    return false;

  
  
  return element->GetPrimaryFrame()->GetStyleDisplay()->mDisplay !=
    NS_STYLE_DISPLAY_INLINE;
}

struct CheckLeavingBreakElementClosure {
  bool          mLeftBreakElement;
};

static void
CheckLeavingBreakElement(nsINode* aNode, void* aClosure)
{
  CheckLeavingBreakElementClosure* cl =
    static_cast<CheckLeavingBreakElementClosure*>(aClosure);
  if (!cl->mLeftBreakElement && IsBreakElement(aNode)) {
    cl->mLeftBreakElement = true;
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
  
  
  
  
  nsINode* node = mSoftBegin.mNode;
  PRInt32 firstOffsetInNode = 0;
  PRInt32 checkBeforeOffset = mSoftBegin.mOffset;
  while (node) {
    if (ContainsDOMWordSeparator(node, checkBeforeOffset, &firstOffsetInNode)) {
      if (node == mSoftBegin.mNode) {
        
        
        PRInt32 newOffset = 0;
        if (firstOffsetInNode > 0) {
          
          
          
          
          
          
          ContainsDOMWordSeparator(node, firstOffsetInNode - 1, &newOffset);
        }
        firstOffsetInNode = newOffset;
        mSoftBegin.mOffset = newOffset;
      }
      break;
    }
    checkBeforeOffset = PR_INT32_MAX;
    if (IsBreakElement(node)) {
      
      
      
      break;
    }
    
    if (!nsContentUtils::ContentIsDescendantOf(node, mRootNode)) {
      break;
    }
    node = node->GetPreviousContent(mRootNode);
  }

  
  
  
  mSoftText.Truncate();
  mSoftTextDOMMapping.Clear();
  bool seenSoftEnd = false;
  
  
  while (node) {
    if (node == mSoftEnd.mNode) {
      seenSoftEnd = true;
    }

    bool exit = false;
    if (IsTextNode(node)) {
      nsIContent* content = static_cast<nsIContent*>(node);
      NS_ASSERTION(content, "Where is our content?");
      const nsTextFragment* textFragment = content->GetText();
      NS_ASSERTION(textFragment, "Where is our text?");
      PRInt32 lastOffsetInNode = textFragment->GetLength();

      if (seenSoftEnd) {
        
        for (PRInt32 i = node == mSoftEnd.mNode ? mSoftEnd.mOffset : 0;
             i < PRInt32(textFragment->GetLength()); ++i) {
          if (IsDOMWordSeparator(textFragment->CharAt(i))) {
            exit = true;
            
            lastOffsetInNode = i;
            break;
          }
        }
      }
      
      if (firstOffsetInNode < lastOffsetInNode) {
        PRInt32 len = lastOffsetInNode - firstOffsetInNode;
        mSoftTextDOMMapping.AppendElement(
          DOMTextMapping(NodeOffset(node, firstOffsetInNode), mSoftText.Length(), len));
        textFragment->AppendTo(mSoftText, firstOffsetInNode, len);
      }
      
      firstOffsetInNode = 0;
    }

    if (exit)
      break;

    CheckLeavingBreakElementClosure closure = { false };
    node = FindNextNode(node, mRootNode, CheckLeavingBreakElement, &closure);
    if (closure.mLeftBreakElement || (node && IsBreakElement(node))) {
      
      
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
    DOMMapHint aHint, bool aSearchForward)
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

  CharClass ClassifyCharacter(PRInt32 aIndex, bool aRecurse) const;
  void Advance();
  void AdvanceThroughSeparators();
  void AdvanceThroughWord();

  
  
  
  
  PRInt32 FindSpecialWord();

  
  
  
  bool ShouldSkipWord(PRInt32 aStart, PRInt32 aLength);
};



CharClass
WordSplitState::ClassifyCharacter(PRInt32 aIndex, bool aRecurse) const
{
  NS_ASSERTION(aIndex >= 0 && aIndex <= PRInt32(mDOMWordText.Length()),
               "Index out of range");
  if (aIndex == PRInt32(mDOMWordText.Length()))
    return CHAR_CLASS_SEPARATOR;

  
  
  nsIUGenCategory::nsUGenCategory
    charCategory = mozilla::unicode::GetGenCategory(mDOMWordText[aIndex]);
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
    
    
    if (mDOMWordText[aIndex - 1] == '.')
      return CHAR_CLASS_SEPARATOR;

    
    if (aIndex == PRInt32(mDOMWordText.Length()) - 1)
      return CHAR_CLASS_SEPARATOR;
    if (ClassifyCharacter(aIndex + 1, false) != CHAR_CLASS_WORD)
      return CHAR_CLASS_SEPARATOR;
    
    
    if (mDOMWordText[aIndex + 1] == '.')
      return CHAR_CLASS_SEPARATOR;

    
    return CHAR_CLASS_WORD;
  }

  
  
  
  if (aIndex > 0 &&
      mDOMWordText[aIndex] == '.' &&
      mDOMWordText[aIndex - 1] != '.' &&
      ClassifyCharacter(aIndex - 1, false) != CHAR_CLASS_WORD) {
    return CHAR_CLASS_WORD;
  }

  
  if (charCategory == nsIUGenCategory::kSeparator ||
      charCategory == nsIUGenCategory::kOther ||
      charCategory == nsIUGenCategory::kPunctuation ||
      charCategory == nsIUGenCategory::kSymbol) {
    
    if (aIndex > 0 &&
        mDOMWordText[aIndex] == '-' &&
        mDOMWordText[aIndex - 1] != '-' &&
        ClassifyCharacter(aIndex - 1, false) == CHAR_CLASS_WORD) {
      
      
      if (aIndex == PRInt32(mDOMWordText.Length()) - 1)
        return CHAR_CLASS_SEPARATOR;
      if (mDOMWordText[aIndex + 1] != '.' &&
          ClassifyCharacter(aIndex + 1, false) == CHAR_CLASS_WORD)
        return CHAR_CLASS_WORD;
    }
    return CHAR_CLASS_SEPARATOR;
  }

  
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
    mCurCharClass = ClassifyCharacter(mDOMWordOffset, true);
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

  
  
  
  PRInt32 firstColon = -1;
  for (i = mDOMWordOffset;
       i < PRInt32(mDOMWordText.Length()); i ++) {
    if (mDOMWordText[i] == '@') {
      
      
      

      
      
      
      
      
      
      if (i > 0 && ClassifyCharacter(i - 1, false) == CHAR_CLASS_WORD &&
          i < (PRInt32)mDOMWordText.Length() - 1 &&
          ClassifyCharacter(i + 1, false) == CHAR_CLASS_WORD)

      return mDOMWordText.Length() - mDOMWordOffset;
    } else if (mDOMWordText[i] == ':' && firstColon < 0) {
      firstColon = i;

      
      
      if (firstColon < (PRInt32)mDOMWordText.Length() - 1 &&
          mDOMWordText[firstColon + 1] == '/') {
        return mDOMWordText.Length() - mDOMWordOffset;
      }
    }
  }

  
  
  
  
  if (firstColon > mDOMWordOffset) {
    nsString protocol(Substring(mDOMWordText, mDOMWordOffset,
                      firstColon - mDOMWordOffset));
    if (protocol.EqualsIgnoreCase("http") ||
        protocol.EqualsIgnoreCase("https") ||
        protocol.EqualsIgnoreCase("news") ||
        protocol.EqualsIgnoreCase("file") ||
        protocol.EqualsIgnoreCase("javascript") ||
        protocol.EqualsIgnoreCase("data") ||
        protocol.EqualsIgnoreCase("ftp")) {
      return mDOMWordText.Length() - mDOMWordOffset;
    }
  }

  
  return -1;
}



bool
WordSplitState::ShouldSkipWord(PRInt32 aStart, PRInt32 aLength)
{
  PRInt32 last = aStart + aLength;

  
  for (PRInt32 i = aStart; i < last; i ++) {
    PRUnichar ch = mDOMWordText[i];
    
    if (ch >= '0' && ch <= '9')
      return true;
  }

  
  return false;
}



void
mozInlineSpellWordUtil::SplitDOMWord(PRInt32 aStart, PRInt32 aEnd)
{
  WordSplitState state(this, mSoftText, aStart, aEnd - aStart);
  state.mCurCharClass = state.ClassifyCharacter(0, true);

  while (state.mCurCharClass != CHAR_CLASS_END_OF_INPUT) {
    state.AdvanceThroughSeparators();
    if (state.mCurCharClass == CHAR_CLASS_END_OF_INPUT)
      break;

    PRInt32 specialWordLength = state.FindSpecialWord();
    if (specialWordLength > 0) {
      mRealWords.AppendElement(
        RealWord(aStart + state.mDOMWordOffset, specialWordLength, false));

      
      state.mDOMWordOffset += specialWordLength;
      if (state.mDOMWordOffset + aStart >= aEnd)
        state.mCurCharClass = CHAR_CLASS_END_OF_INPUT;
      else
        state.mCurCharClass = state.ClassifyCharacter(state.mDOMWordOffset, true);
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
