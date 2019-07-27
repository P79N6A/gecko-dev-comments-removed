




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
#include "nsRange.h"
#include "nsContentUtils.h"
#include "nsIFrame.h"
#include <algorithm>
#include "mozilla/BinarySearch.h"

using namespace mozilla;





inline bool IsIgnorableCharacter(char16_t ch)
{
  return (ch == 0xAD ||   
          ch == 0x1806);  
}






inline bool IsConditionalPunctuation(char16_t ch)
{
  return (ch == '\'' ||
          ch == 0x2019 || 
          ch == 0x00B7); 
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
    return nullptr;

  next = aNode->GetNextSibling();
  if (next)
    return next;

  
  for (;;) {
    if (aOnLeaveNode) {
      aOnLeaveNode(aNode, aClosure);
    }
    
    next = aNode->GetParent();
    if (next == aRoot || ! next)
      return nullptr;
    aNode = next;
    
    next = aNode->GetNextSibling();
    if (next)
      return next;
  }
}



static nsINode*
FindNextTextNode(nsINode* aNode, int32_t aOffset, nsINode* aRoot)
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
mozInlineSpellWordUtil::SetEnd(nsINode* aEndNode, int32_t aEndOffset)
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
mozInlineSpellWordUtil::SetPosition(nsINode* aNode, int32_t aOffset)
{
  InvalidateWords();

  if (!IsTextNode(aNode)) {
    
    aNode = FindNextTextNode(aNode, aOffset, mRootNode);
    aOffset = 0;
  }
  mSoftBegin = NodeOffset(aNode, aOffset);

  EnsureWords();
  
  int32_t textOffset = MapDOMPositionToSoftTextOffset(mSoftBegin);
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
                                        int32_t aWordOffset,
                                        nsRange** aRange)
{
  
  nsCOMPtr<nsINode> wordNode = do_QueryInterface(aWordNode);
  NodeOffset pt = NodeOffset(wordNode, aWordOffset);
  
  InvalidateWords();
  mSoftBegin = mSoftEnd = pt;
  EnsureWords();
  
  int32_t offset = MapDOMPositionToSoftTextOffset(pt);
  if (offset < 0)
    return MakeRange(pt, pt, aRange);
  int32_t wordIndex = FindRealWordContaining(offset, HINT_BEGIN, false);
  if (wordIndex < 0)
    return MakeRange(pt, pt, aRange);
  return MakeRangeForWord(mRealWords[wordIndex], aRange);
}


static void
NormalizeWord(const nsSubstring& aInput, int32_t aPos, int32_t aLen, nsAString& aOutput)
{
  aOutput.Truncate();
  for (int32_t i = 0; i < aLen; i++) {
    char16_t ch = aInput.CharAt(i + aPos);

    
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
      mNextWordIndex >= int32_t(mRealWords.Length())) {
    mNextWordIndex = -1;
    *aRange = nullptr;
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
  NS_ENSURE_ARG_POINTER(aBegin.mNode);
  if (!mDOMDocument)
    return NS_ERROR_NOT_INITIALIZED;

  nsRefPtr<nsRange> range = new nsRange(aBegin.mNode);
  nsresult rv = range->Set(aBegin.mNode, aBegin.mOffset,
                           aEnd.mNode, aEnd.mOffset);
  NS_ENSURE_SUCCESS(rv, rv);
  range.forget(aRange);

  return NS_OK;
}












static bool
IsDOMWordSeparator(char16_t ch)
{
  
  if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r')
    return true;

  
  if (ch >= 0xA0 &&
      (ch == 0x00A0 ||  
       ch == 0x2002 ||  
       ch == 0x2003 ||  
       ch == 0x2009 ||  
       ch == 0x3000))   
    return true;

  
  return false;
}

static inline bool
IsBRElement(nsINode* aNode)
{
  return aNode->IsHTMLElement(nsGkAtoms::br);
}














static bool
TextNodeContainsDOMWordSeparator(nsINode* aNode,
                                 int32_t aBeforeOffset,
                                 int32_t* aSeparatorOffset)
{
  
  nsIContent* content = static_cast<nsIContent*>(aNode);
  const nsTextFragment* textFragment = content->GetText();
  NS_ASSERTION(textFragment, "Where is our text?");
  for (int32_t i = std::min(aBeforeOffset, int32_t(textFragment->GetLength())) - 1; i >= 0; --i) {
    if (IsDOMWordSeparator(textFragment->CharAt(i))) {
      
      for (int32_t j = i - 1; j >= 0; --j) {
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
ContainsDOMWordSeparator(nsINode* aNode, int32_t aBeforeOffset,
                         int32_t* aSeparatorOffset)
{
  if (IsBRElement(aNode)) {
    *aSeparatorOffset = 0;
    return true;
  }

  if (!IsTextNode(aNode))
    return false;

  return TextNodeContainsDOMWordSeparator(aNode, aBeforeOffset,
                                          aSeparatorOffset);
}

static bool
IsBreakElement(nsINode* aNode)
{
  if (!aNode->IsElement()) {
    return false;
  }

  dom::Element *element = aNode->AsElement();
    
  if (element->IsHTMLElement(nsGkAtoms::br))
    return true;

  
  
  if (!element->GetPrimaryFrame())
    return false;

  
  
  return element->GetPrimaryFrame()->StyleDisplay()->mDisplay !=
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
  int32_t firstOffsetInNode = 0;
  int32_t checkBeforeOffset = mSoftBegin.mOffset;
  while (node) {
    if (ContainsDOMWordSeparator(node, checkBeforeOffset, &firstOffsetInNode)) {
      if (node == mSoftBegin.mNode) {
        
        
        int32_t newOffset = 0;
        if (firstOffsetInNode > 0) {
          
          
          
          
          
          
          
          
          
          if (!ContainsDOMWordSeparator(node, firstOffsetInNode - 1,
                                        &newOffset)) {
            nsINode* prevNode = node->GetPreviousSibling();
            while (prevNode && IsTextNode(prevNode)) {
              mSoftBegin.mNode = prevNode;
              if (TextNodeContainsDOMWordSeparator(prevNode, INT32_MAX,
                                                   &newOffset)) {
                break;
              }
              prevNode = prevNode->GetPreviousSibling();
            }
          }
        }
        firstOffsetInNode = newOffset;
        mSoftBegin.mOffset = newOffset;
      }
      break;
    }
    checkBeforeOffset = INT32_MAX;
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
      int32_t lastOffsetInNode = textFragment->GetLength();

      if (seenSoftEnd) {
        
        for (int32_t i = node == mSoftEnd.mNode ? mSoftEnd.mOffset : 0;
             i < int32_t(textFragment->GetLength()); ++i) {
          if (IsDOMWordSeparator(textFragment->CharAt(i))) {
            exit = true;
            
            lastOffsetInNode = i;
            break;
          }
        }
      }

      if (firstOffsetInNode < lastOffsetInNode) {
        int32_t len = lastOffsetInNode - firstOffsetInNode;
        mSoftTextDOMMapping.AppendElement(
          DOMTextMapping(NodeOffset(node, firstOffsetInNode), mSoftText.Length(), len));

        bool ok = textFragment->AppendTo(mSoftText, firstOffsetInNode, len,
                                         mozilla::fallible);
        if (!ok) {
            
            mSoftTextDOMMapping.RemoveElementAt(mSoftTextDOMMapping.Length() - 1);
            exit = true;
        }
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
  
  
  
  
  int32_t wordStart = -1;
  mRealWords.Clear();
  for (int32_t i = 0; i < int32_t(mSoftText.Length()); ++i) {
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



int32_t
mozInlineSpellWordUtil::MapDOMPositionToSoftTextOffset(NodeOffset aNodeOffset)
{
  if (!mSoftTextValid) {
    NS_ERROR("Soft text must be valid if we're to map into it");
    return -1;
  }
  
  for (int32_t i = 0; i < int32_t(mSoftTextDOMMapping.Length()); ++i) {
    const DOMTextMapping& map = mSoftTextDOMMapping[i];
    if (map.mNodeOffset.mNode == aNodeOffset.mNode) {
      
      
      int32_t offsetInContributedString =
        aNodeOffset.mOffset - map.mNodeOffset.mOffset;
      if (offsetInContributedString >= 0 &&
          offsetInContributedString <= map.mLength)
        return map.mSoftTextOffset + offsetInContributedString;
      return -1;
    }
  }
  return -1;
}

namespace {

template<class T>
class FirstLargerOffset
{
  int32_t mSoftTextOffset;

public:
  explicit FirstLargerOffset(int32_t aSoftTextOffset) : mSoftTextOffset(aSoftTextOffset) {}
  int operator()(const T& t) const {
  
  
    return mSoftTextOffset < t.mSoftTextOffset ? -1 : 1;
  }
};

template<class T>
bool
FindLastNongreaterOffset(const nsTArray<T>& aContainer, int32_t aSoftTextOffset, size_t* aIndex)
{
  if (aContainer.Length() == 0) {
    return false;
  }

  BinarySearchIf(aContainer, 0, aContainer.Length(),
                 FirstLargerOffset<T>(aSoftTextOffset), aIndex);
  if (*aIndex > 0) {
    
    
    *aIndex -= 1;
  } else {
    
    MOZ_ASSERT(aContainer[*aIndex].mSoftTextOffset > aSoftTextOffset);
  }
  return true;
}

} 

mozInlineSpellWordUtil::NodeOffset
mozInlineSpellWordUtil::MapSoftTextOffsetToDOMPosition(int32_t aSoftTextOffset,
                                                       DOMMapHint aHint)
{
  NS_ASSERTION(mSoftTextValid, "Soft text must be valid if we're to map out of it");
  if (!mSoftTextValid)
    return NodeOffset(nullptr, -1);

  
  size_t index;
  bool found = FindLastNongreaterOffset(mSoftTextDOMMapping, aSoftTextOffset, &index);
  if (!found) {
    return NodeOffset(nullptr, -1);
  }

  
  
  
  
  if (aHint == HINT_END && index > 0) {
    const DOMTextMapping& map = mSoftTextDOMMapping[index - 1];
    if (map.mSoftTextOffset + map.mLength == aSoftTextOffset)
      return NodeOffset(map.mNodeOffset.mNode, map.mNodeOffset.mOffset + map.mLength);
  }

  
  
  
  const DOMTextMapping& map = mSoftTextDOMMapping[index];
  int32_t offset = aSoftTextOffset - map.mSoftTextOffset;
  if (offset >= 0 && offset <= map.mLength)
    return NodeOffset(map.mNodeOffset.mNode, map.mNodeOffset.mOffset + offset);

  return NodeOffset(nullptr, -1);
}

int32_t
mozInlineSpellWordUtil::FindRealWordContaining(int32_t aSoftTextOffset,
    DOMMapHint aHint, bool aSearchForward)
{
  NS_ASSERTION(mSoftTextValid, "Soft text must be valid if we're to map out of it");
  if (!mSoftTextValid)
    return -1;

  
  size_t index;
  bool found = FindLastNongreaterOffset(mRealWords, aSoftTextOffset, &index);
  if (!found) {
    return -1;
  }

  
  
  
  
  if (aHint == HINT_END && index > 0) {
    const RealWord& word = mRealWords[index - 1];
    if (word.mSoftTextOffset + word.mLength == aSoftTextOffset)
      return index - 1;
  }

  
  
  
  const RealWord& word = mRealWords[index];
  int32_t offset = aSoftTextOffset - word.mSoftTextOffset;
  if (offset >= 0 && offset <= word.mLength)
    return index;

  if (aSearchForward) {
    if (mRealWords[0].mSoftTextOffset > aSoftTextOffset) {
      
      return 0;
    }
    
    
    
    if (index + 1 < mRealWords.Length())
      return index + 1;
  }

  return -1;
}




enum CharClass {
  CHAR_CLASS_WORD,
  CHAR_CLASS_SEPARATOR,
  CHAR_CLASS_END_OF_INPUT };


struct MOZ_STACK_CLASS WordSplitState
{
  mozInlineSpellWordUtil*    mWordUtil;
  const nsDependentSubstring mDOMWordText;
  int32_t                    mDOMWordOffset;
  CharClass                  mCurCharClass;

  WordSplitState(mozInlineSpellWordUtil* aWordUtil,
                 const nsString& aString, int32_t aStart, int32_t aLen)
    : mWordUtil(aWordUtil), mDOMWordText(aString, aStart, aLen),
      mDOMWordOffset(0), mCurCharClass(CHAR_CLASS_END_OF_INPUT) {}

  CharClass ClassifyCharacter(int32_t aIndex, bool aRecurse) const;
  void Advance();
  void AdvanceThroughSeparators();
  void AdvanceThroughWord();

  
  
  
  
  bool IsSpecialWord();

  
  
  
  bool ShouldSkipWord(int32_t aStart, int32_t aLength);
};



CharClass
WordSplitState::ClassifyCharacter(int32_t aIndex, bool aRecurse) const
{
  NS_ASSERTION(aIndex >= 0 && aIndex <= int32_t(mDOMWordText.Length()),
               "Index out of range");
  if (aIndex == int32_t(mDOMWordText.Length()))
    return CHAR_CLASS_SEPARATOR;

  
  
  nsIUGenCategory::nsUGenCategory
    charCategory = mozilla::unicode::GetGenCategory(mDOMWordText[aIndex]);
  if (charCategory == nsIUGenCategory::kLetter ||
      IsIgnorableCharacter(mDOMWordText[aIndex]) ||
      mDOMWordText[aIndex] == 0x200C  ||
      mDOMWordText[aIndex] == 0x200D )
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

    
    if (aIndex == int32_t(mDOMWordText.Length()) - 1)
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
      
      
      if (aIndex == int32_t(mDOMWordText.Length()) - 1)
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
  NS_ASSERTION(mDOMWordOffset < (int32_t)mDOMWordText.Length(),
               "Length beyond end");

  mDOMWordOffset ++;
  if (mDOMWordOffset >= (int32_t)mDOMWordText.Length())
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




bool
WordSplitState::IsSpecialWord()
{
  
  
  
  int32_t firstColon = -1;
  for (int32_t i = mDOMWordOffset;
       i < int32_t(mDOMWordText.Length()); i ++) {
    if (mDOMWordText[i] == '@') {
      
      
      

      
      
      
      
      
      
      if (i > 0 && ClassifyCharacter(i - 1, false) == CHAR_CLASS_WORD &&
          i < (int32_t)mDOMWordText.Length() - 1 &&
          ClassifyCharacter(i + 1, false) == CHAR_CLASS_WORD) {
        return true;
      }
    } else if (mDOMWordText[i] == ':' && firstColon < 0) {
      firstColon = i;

      
      
      if (firstColon < (int32_t)mDOMWordText.Length() - 1 &&
          mDOMWordText[firstColon + 1] == '/') {
        return true;
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
      return true;
    }
  }

  
  return false;
}



bool
WordSplitState::ShouldSkipWord(int32_t aStart, int32_t aLength)
{
  int32_t last = aStart + aLength;

  
  for (int32_t i = aStart; i < last; i ++) {
    if (unicode::GetGenCategory(mDOMWordText[i]) == nsIUGenCategory::kNumber) {
      return true;
    }
  }

  
  return false;
}



void
mozInlineSpellWordUtil::SplitDOMWord(int32_t aStart, int32_t aEnd)
{
  WordSplitState state(this, mSoftText, aStart, aEnd - aStart);
  state.mCurCharClass = state.ClassifyCharacter(0, true);

  state.AdvanceThroughSeparators();
  if (state.mCurCharClass != CHAR_CLASS_END_OF_INPUT &&
      state.IsSpecialWord()) {
    int32_t specialWordLength = state.mDOMWordText.Length() - state.mDOMWordOffset;
    mRealWords.AppendElement(
        RealWord(aStart + state.mDOMWordOffset, specialWordLength, false));

    return;
  }

  while (state.mCurCharClass != CHAR_CLASS_END_OF_INPUT) {
    state.AdvanceThroughSeparators();
    if (state.mCurCharClass == CHAR_CLASS_END_OF_INPUT)
      break;

    
    int32_t wordOffset = state.mDOMWordOffset;

    
    state.AdvanceThroughWord();
    int32_t wordLen = state.mDOMWordOffset - wordOffset;
    mRealWords.AppendElement(
      RealWord(aStart + wordOffset, wordLen,
               !state.ShouldSkipWord(wordOffset, wordLen)));
  }
}
