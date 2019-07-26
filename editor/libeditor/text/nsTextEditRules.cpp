




#include "mozilla/Assertions.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/Preferences.h"
#include "mozilla/Selection.h"
#include "mozilla/dom/Element.h"
#include "nsAString.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsCRTGlue.h"
#include "nsComponentManagerUtils.h"
#include "nsContentUtils.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsEditorUtils.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeFilter.h"
#include "nsIDOMNodeIterator.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMText.h"
#include "nsINameSpaceManager.h"
#include "nsINode.h"
#include "nsIPlaintextEditor.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsISupportsBase.h"
#include "nsLiteralString.h"
#include "nsNodeIterator.h"
#include "nsTextEditRules.h"
#include "nsTextEditUtils.h"
#include "nsUnicharUtils.h"

using namespace mozilla;

#define CANCEL_OPERATION_IF_READONLY_OR_DISABLED \
  if (IsReadonly() || IsDisabled()) \
  {                     \
    *aCancel = true; \
    return NS_OK;       \
  };






nsTextEditRules::nsTextEditRules()
: mEditor(nullptr)
, mPasswordText()
, mPasswordIMEText()
, mPasswordIMEIndex(0)
, mActionNesting(0)
, mLockRulesSniffing(false)
, mDidExplicitlySetInterline(false)
, mTheAction(EditAction::none)
, mLastStart(0)
, mLastLength(0)
{
}

nsTextEditRules::~nsTextEditRules()
{
   

  if (mTimer)
    mTimer->Cancel();
}





NS_IMPL_CYCLE_COLLECTION_2(nsTextEditRules, mBogusNode, mCachedSelectionNode)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsTextEditRules)
  NS_INTERFACE_MAP_ENTRY(nsIEditRules)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIEditRules)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsTextEditRules)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsTextEditRules)





NS_IMETHODIMP
nsTextEditRules::Init(nsPlaintextEditor *aEditor)
{
  if (!aEditor) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;  
  nsCOMPtr<nsISelection> selection;
  mEditor->GetSelection(getter_AddRefs(selection));
  NS_WARN_IF_FALSE(selection, "editor cannot get selection");

  
  
  nsresult res = CreateBogusNodeIfNeeded(selection);
  NS_ENSURE_SUCCESS(res, res);

  
  
  int32_t rangeCount;
  res = selection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(res, res);
  if (!rangeCount) {
    res = mEditor->EndOfDocument();
    NS_ENSURE_SUCCESS(res, res);
  }

  if (IsPlaintextEditor())
  {
    
    res = CreateTrailingBRIfNeeded();
    NS_ENSURE_SUCCESS(res, res);
  }

  mDeleteBidiImmediately =
    Preferences::GetBool("bidi.edit.delete_immediately", false);

  return res;
}

NS_IMETHODIMP
nsTextEditRules::DetachEditor()
{
  if (mTimer)
    mTimer->Cancel();

  mEditor = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsTextEditRules::BeforeEdit(EditAction action,
                            nsIEditor::EDirection aDirection)
{
  if (mLockRulesSniffing) return NS_OK;
  
  nsAutoLockRulesSniffing lockIt(this);
  mDidExplicitlySetInterline = false;
  if (!mActionNesting)
  {
    
    mTheAction = action;
  }
  mActionNesting++;
  
  
  nsCOMPtr<nsISelection> selection;
  nsresult res = mEditor->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);

  selection->GetAnchorNode(getter_AddRefs(mCachedSelectionNode));
  selection->GetAnchorOffset(&mCachedSelectionOffset);

  return NS_OK;
}


NS_IMETHODIMP
nsTextEditRules::AfterEdit(EditAction action,
                           nsIEditor::EDirection aDirection)
{
  if (mLockRulesSniffing) return NS_OK;
  
  nsAutoLockRulesSniffing lockIt(this);
  
  NS_PRECONDITION(mActionNesting>0, "bad action nesting!");
  nsresult res = NS_OK;
  if (!--mActionNesting)
  {
    nsCOMPtr<nsISelection>selection;
    res = mEditor->GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
  
    res = mEditor->HandleInlineSpellCheck(action, selection,
                                          mCachedSelectionNode, mCachedSelectionOffset,
                                          nullptr, 0, nullptr, 0);
    NS_ENSURE_SUCCESS(res, res);

    
    res = RemoveRedundantTrailingBR();
    if (NS_FAILED(res))
      return res;

    
    res = CreateBogusNodeIfNeeded(selection);
    NS_ENSURE_SUCCESS(res, res);
    
    
    res = CreateTrailingBRIfNeeded();
    NS_ENSURE_SUCCESS(res, res);

    
    CollapseSelectionToTrailingBRIfNeeded(selection);
  }
  return res;
}


NS_IMETHODIMP
nsTextEditRules::WillDoAction(Selection* aSelection,
                              nsRulesInfo* aInfo,
                              bool* aCancel,
                              bool* aHandled)
{
  
  MOZ_ASSERT(aInfo && aCancel && aHandled);

  *aCancel = false;
  *aHandled = false;

  
  nsTextRulesInfo *info = static_cast<nsTextRulesInfo*>(aInfo);

  switch (info->action) {
    case EditAction::insertBreak:
      return WillInsertBreak(aSelection, aCancel, aHandled, info->maxLength);
    case EditAction::insertText:
    case EditAction::insertIMEText:
      return WillInsertText(info->action, aSelection, aCancel, aHandled,
                            info->inString, info->outString, info->maxLength);
    case EditAction::deleteSelection:
      return WillDeleteSelection(aSelection, info->collapsedAction,
                                 aCancel, aHandled);
    case EditAction::undo:
      return WillUndo(aSelection, aCancel, aHandled);
    case EditAction::redo:
      return WillRedo(aSelection, aCancel, aHandled);
    case EditAction::setTextProperty:
      return WillSetTextProperty(aSelection, aCancel, aHandled);
    case EditAction::removeTextProperty:
      return WillRemoveTextProperty(aSelection, aCancel, aHandled);
    case EditAction::outputText:
      return WillOutputText(aSelection, info->outputFormat, info->outString,
                            aCancel, aHandled);
    case EditAction::insertElement:
      
      
      return WillInsert(aSelection, aCancel);
    default:
      return NS_ERROR_FAILURE;
  }
}

NS_IMETHODIMP 
nsTextEditRules::DidDoAction(nsISelection *aSelection,
                             nsRulesInfo *aInfo, nsresult aResult)
{
  
  
  nsAutoTxnsConserveSelection dontSpazMySelection(mEditor);

  NS_ENSURE_TRUE(aSelection && aInfo, NS_ERROR_NULL_POINTER);
    
  
  nsTextRulesInfo *info = static_cast<nsTextRulesInfo*>(aInfo);

  switch (info->action)
  {
    case EditAction::insertBreak:
      return DidInsertBreak(aSelection, aResult);
    case EditAction::insertText:
    case EditAction::insertIMEText:
      return DidInsertText(aSelection, aResult);
    case EditAction::deleteSelection:
      return DidDeleteSelection(aSelection, info->collapsedAction, aResult);
    case EditAction::undo:
      return DidUndo(aSelection, aResult);
    case EditAction::redo:
      return DidRedo(aSelection, aResult);
    case EditAction::setTextProperty:
      return DidSetTextProperty(aSelection, aResult);
    case EditAction::removeTextProperty:
      return DidRemoveTextProperty(aSelection, aResult);
    case EditAction::outputText:
      return DidOutputText(aSelection, aResult);
    default:
      
      return NS_OK;
  }
}


NS_IMETHODIMP
nsTextEditRules::DocumentIsEmpty(bool *aDocumentIsEmpty)
{
  NS_ENSURE_TRUE(aDocumentIsEmpty, NS_ERROR_NULL_POINTER);
  
  *aDocumentIsEmpty = (mBogusNode != nullptr);
  return NS_OK;
}






nsresult
nsTextEditRules::WillInsert(nsISelection *aSelection, bool *aCancel)
{
  NS_ENSURE_TRUE(aSelection && aCancel, NS_ERROR_NULL_POINTER);
  
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED

  
  *aCancel = false;
  
  
  if (mBogusNode)
  {
    mEditor->DeleteNode(mBogusNode);
    mBogusNode = nullptr;
  }

  return NS_OK;
}

nsresult
nsTextEditRules::DidInsert(nsISelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::WillInsertBreak(Selection* aSelection,
                                 bool *aCancel,
                                 bool *aHandled,
                                 int32_t aMaxLength)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED
  *aHandled = false;
  if (IsSingleLineEditor()) {
    *aCancel = true;
  }
  else 
  {
    
    
    NS_NAMED_LITERAL_STRING(inString, "\n");
    nsAutoString outString;
    bool didTruncate;
    nsresult res = TruncateInsertionIfNeeded(aSelection, &inString, &outString,
                                             aMaxLength, &didTruncate);
    NS_ENSURE_SUCCESS(res, res);
    if (didTruncate) {
      *aCancel = true;
      return NS_OK;
    }

    *aCancel = false;

    
    bool bCollapsed;
    res = aSelection->GetIsCollapsed(&bCollapsed);
    NS_ENSURE_SUCCESS(res, res);
    if (!bCollapsed)
    {
      res = mEditor->DeleteSelection(nsIEditor::eNone, nsIEditor::eStrip);
      NS_ENSURE_SUCCESS(res, res);
    }

    res = WillInsert(aSelection, aCancel);
    NS_ENSURE_SUCCESS(res, res);
    
    
    *aCancel = false;
  
  }
  return NS_OK;
}

nsresult
nsTextEditRules::DidInsertBreak(nsISelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::CollapseSelectionToTrailingBRIfNeeded(nsISelection* aSelection)
{
  
  
  
  if (!IsPlaintextEditor()) {
    return NS_OK;
  }

  
  
  int32_t selOffset;
  nsCOMPtr<nsIDOMNode> selNode;
  nsresult res;
  res = mEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMText> nodeAsText = do_QueryInterface(selNode);
  if (!nodeAsText) return NS_OK; 

  uint32_t length;
  res = nodeAsText->GetLength(&length);
  NS_ENSURE_SUCCESS(res, res);

  
  if (selOffset != int32_t(length))
    return NS_OK;

  int32_t parentOffset;
  nsCOMPtr<nsIDOMNode> parentNode = nsEditor::GetNodeLocation(selNode, &parentOffset);

  nsCOMPtr<nsIDOMNode> root = do_QueryInterface(mEditor->GetRoot());
  NS_ENSURE_TRUE(root, NS_ERROR_NULL_POINTER);
  if (parentNode != root) return NS_OK;

  nsCOMPtr<nsIDOMNode> nextNode = mEditor->GetChildAt(parentNode,
                                                      parentOffset + 1);
  if (nextNode && nsTextEditUtils::IsMozBR(nextNode))
  {
    res = aSelection->Collapse(parentNode, parentOffset + 1);
    NS_ENSURE_SUCCESS(res, res);
  }
  return res;
}

static inline already_AddRefed<nsIDOMNode>
GetTextNode(nsISelection *selection, nsEditor *editor) {
  int32_t selOffset;
  nsCOMPtr<nsIDOMNode> selNode;
  nsresult res = editor->GetStartNodeAndOffset(selection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, nullptr);
  if (!editor->IsTextNode(selNode)) {
    
    nsCOMPtr<nsINode> node = do_QueryInterface(selNode);
    
    NS_ENSURE_TRUE(node, nullptr);
    
    nsNodeIterator iter(node, nsIDOMNodeFilter::SHOW_TEXT, nullptr);
    while (!editor->IsTextNode(selNode)) {
      if (NS_FAILED(res = iter.NextNode(getter_AddRefs(selNode))) || !selNode) {
        return nullptr;
      }
    }
  }
  return selNode.forget();
}
#ifdef DEBUG
#define ASSERT_PASSWORD_LENGTHS_EQUAL()                                \
  if (IsPasswordEditor()) {                                            \
    int32_t txtLen;                                                    \
    mEditor->GetTextLength(&txtLen);                                    \
    NS_ASSERTION(mPasswordText.Length() == uint32_t(txtLen),           \
                 "password length not equal to number of asterisks");  \
  }
#else
#define ASSERT_PASSWORD_LENGTHS_EQUAL()
#endif


void
nsTextEditRules::HandleNewLines(nsString &aString,
                                int32_t aNewlineHandling)
{
  if (aNewlineHandling < 0) {
    int32_t caretStyle;
    nsPlaintextEditor::GetDefaultEditorPrefs(aNewlineHandling, caretStyle);
  }

  switch(aNewlineHandling)
  {
  case nsIPlaintextEditor::eNewlinesReplaceWithSpaces:
    
    aString.Trim(CRLF, false, true);
    aString.ReplaceChar(CRLF, ' ');
    break;
  case nsIPlaintextEditor::eNewlinesStrip:
    aString.StripChars(CRLF);
    break;
  case nsIPlaintextEditor::eNewlinesPasteToFirst:
  default:
    {
      int32_t firstCRLF = aString.FindCharInSet(CRLF);

      
      int32_t offset = 0;
      while (firstCRLF == offset)
      {
        offset++;
        firstCRLF = aString.FindCharInSet(CRLF, offset);
      }
      if (firstCRLF > 0)
        aString.Truncate(firstCRLF);
      if (offset > 0)
        aString.Cut(0, offset);
    }
    break;
  case nsIPlaintextEditor::eNewlinesReplaceWithCommas:
    aString.Trim(CRLF, true, true);
    aString.ReplaceChar(CRLF, ',');
    break;
  case nsIPlaintextEditor::eNewlinesStripSurroundingWhitespace:
    {
      nsString result;
      uint32_t offset = 0;
      while (offset < aString.Length())
      {
        int32_t nextCRLF = aString.FindCharInSet(CRLF, offset);
        if (nextCRLF < 0) {
          result.Append(nsDependentSubstring(aString, offset));
          break;
        }
        uint32_t wsBegin = nextCRLF;
        
        while (wsBegin > offset && NS_IS_SPACE(aString[wsBegin - 1]))
          --wsBegin;
        result.Append(nsDependentSubstring(aString, offset, wsBegin - offset));
        offset = nextCRLF + 1;
        while (offset < aString.Length() && NS_IS_SPACE(aString[offset]))
          ++offset;
      }
      aString = result;
    }
    break;
  case nsIPlaintextEditor::eNewlinesPasteIntact:
    
    aString.Trim(CRLF, true, true);
    break;
  }
}

nsresult
nsTextEditRules::WillInsertText(EditAction aAction,
                                Selection* aSelection,
                                bool            *aCancel,
                                bool            *aHandled,
                                const nsAString *inString,
                                nsAString *outString,
                                int32_t          aMaxLength)
{  
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }

  if (inString->IsEmpty() && aAction != EditAction::insertIMEText) {
    
    
    
    
    
    *aCancel = true;
    *aHandled = false;
    return NS_OK;
  }
  
  
  *aCancel = false;
  *aHandled = true;

  
  
  bool truncated = false;
  nsresult res = TruncateInsertionIfNeeded(aSelection, inString, outString,
                                           aMaxLength, &truncated);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if (truncated && outString->IsEmpty() &&
      aAction != EditAction::insertIMEText) {
    *aCancel = true;
    return NS_OK;
  }
  
  int32_t start = 0;
  int32_t end = 0;

  
  if (IsPasswordEditor()) {
    nsContentUtils::GetSelectionInTextControl(aSelection, mEditor->GetRoot(),
                                              start, end);
  }

  
  bool bCollapsed;
  res = aSelection->GetIsCollapsed(&bCollapsed);
  NS_ENSURE_SUCCESS(res, res);
  if (!bCollapsed)
  {
    res = mEditor->DeleteSelection(nsIEditor::eNone, nsIEditor::eStrip);
    NS_ENSURE_SUCCESS(res, res);
  }

  res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);
  
  
  *aCancel = false;
  
  
  
  
  if (IsPasswordEditor())
  {
    if (aAction == EditAction::insertIMEText) {
      RemoveIMETextFromPWBuf(start, outString);
    }
  }

  
  
  
  
  
  
  
  
  
  
  if (IsSingleLineEditor())
  {
    nsAutoString tString(*outString);

    HandleNewLines(tString, mEditor->mNewlineHandling);

    outString->Assign(tString);
  }

  if (IsPasswordEditor())
  {
    
    mPasswordText.Insert(*outString, start);

    if (LookAndFeel::GetEchoPassword() && !DontEchoPassword()) {
      HideLastPWInput();
      mLastStart = start;
      mLastLength = outString->Length();
      if (mTimer)
      {
        mTimer->Cancel();
      }
      else
      {
        mTimer = do_CreateInstance("@mozilla.org/timer;1", &res);
        NS_ENSURE_SUCCESS(res, res);
      }
      mTimer->InitWithCallback(this, LookAndFeel::GetPasswordMaskDelay(),
                               nsITimer::TYPE_ONE_SHOT);
    } 
    else 
    {
      FillBufWithPWChars(outString, outString->Length());
    }
  }

  
  nsCOMPtr<nsIDOMNode> selNode;
  int32_t selOffset;
  res = mEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);

  
  if (!mEditor->IsTextNode(selNode) &&
      !mEditor->CanContainTag(selNode, nsGkAtoms::textTagName)) {
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIDOMDocument> doc = mEditor->GetDOMDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);
    
  if (aAction == EditAction::insertIMEText) {
    res = mEditor->InsertTextImpl(*outString, address_of(selNode), &selOffset, doc);
    NS_ENSURE_SUCCESS(res, res);
  } else {
    
    nsCOMPtr<nsIDOMNode> curNode = selNode;
    int32_t curOffset = selOffset;

    
    nsAutoTxnsConserveSelection dontSpazMySelection(mEditor);

    res = mEditor->InsertTextImpl(*outString, address_of(curNode),
                                  &curOffset, doc);
    NS_ENSURE_SUCCESS(res, res);

    if (curNode) 
    {
      
      
      bool endsWithLF =
        !outString->IsEmpty() && outString->Last() == nsCRT::LF;
      aSelection->SetInterlinePosition(endsWithLF);

      aSelection->Collapse(curNode, curOffset);
    }
  }
  ASSERT_PASSWORD_LENGTHS_EQUAL()
  return res;
}

nsresult
nsTextEditRules::DidInsertText(nsISelection *aSelection, 
                               nsresult aResult)
{
  return DidInsert(aSelection, aResult);
}



nsresult
nsTextEditRules::WillSetTextProperty(nsISelection *aSelection, bool *aCancel, bool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) 
    { return NS_ERROR_NULL_POINTER; }

  
  if (IsPlaintextEditor()) {
    *aCancel = true;
  }
  return NS_OK;
}

nsresult
nsTextEditRules::DidSetTextProperty(nsISelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::WillRemoveTextProperty(nsISelection *aSelection, bool *aCancel, bool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) 
    { return NS_ERROR_NULL_POINTER; }

  
  if (IsPlaintextEditor()) {
    *aCancel = true;
  }
  return NS_OK;
}

nsresult
nsTextEditRules::DidRemoveTextProperty(nsISelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::WillDeleteSelection(Selection* aSelection,
                                     nsIEditor::EDirection aCollapsedAction, 
                                     bool *aCancel,
                                     bool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED

  
  *aCancel = false;
  *aHandled = false;
  
  
  if (mBogusNode) {
    *aCancel = true;
    return NS_OK;
  }

  nsresult res = NS_OK;
  nsAutoScriptBlocker scriptBlocker;

  if (IsPasswordEditor())
  {
    res = mEditor->ExtendSelectionForDelete(aSelection, &aCollapsedAction);
    NS_ENSURE_SUCCESS(res, res);

    
    int32_t start, end;
    nsContentUtils::GetSelectionInTextControl(aSelection, mEditor->GetRoot(),
                                              start, end);

    if (LookAndFeel::GetEchoPassword()) {
      HideLastPWInput();
      mLastStart = start;
      mLastLength = 0;
      if (mTimer)
      {
        mTimer->Cancel();
      }
    }

    if (end == start)
    { 
      if (nsIEditor::ePrevious==aCollapsedAction && 0<start) { 
        mPasswordText.Cut(start-1, 1);
      }
      else if (nsIEditor::eNext==aCollapsedAction) {      
        mPasswordText.Cut(start, 1);
      }
      
    }
    else {  
      mPasswordText.Cut(start, end-start);
    }
  }
  else
  {
    nsCOMPtr<nsIDOMNode> startNode;
    int32_t startOffset;
    res = mEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(startNode), &startOffset);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(startNode, NS_ERROR_FAILURE);
    
    bool bCollapsed;
    res = aSelection->GetIsCollapsed(&bCollapsed);
    NS_ENSURE_SUCCESS(res, res);
  
    if (!bCollapsed)
      return NS_OK;

    
    res = CheckBidiLevelForDeletion(aSelection, startNode, startOffset, aCollapsedAction, aCancel);
    NS_ENSURE_SUCCESS(res, res);
    if (*aCancel) return NS_OK;

    res = mEditor->ExtendSelectionForDelete(aSelection, &aCollapsedAction);
    NS_ENSURE_SUCCESS(res, res);
  }

  res = mEditor->DeleteSelectionImpl(aCollapsedAction, nsIEditor::eStrip);
  NS_ENSURE_SUCCESS(res, res);

  *aHandled = true;
  ASSERT_PASSWORD_LENGTHS_EQUAL()
  return NS_OK;
}

nsresult
nsTextEditRules::DidDeleteSelection(nsISelection *aSelection, 
                                    nsIEditor::EDirection aCollapsedAction, 
                                    nsresult aResult)
{
  nsCOMPtr<nsIDOMNode> startNode;
  int32_t startOffset;
  nsresult res = mEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(startNode), &startOffset);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(startNode, NS_ERROR_FAILURE);
  
  
  if (mEditor->IsTextNode(startNode))
  {
    nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(startNode);
    uint32_t strLength;
    res = textNode->GetLength(&strLength);
    NS_ENSURE_SUCCESS(res, res);
    
    
    if (!strLength)
    {
      res = mEditor->DeleteNode(startNode);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  if (!mDidExplicitlySetInterline)
  {
    
    
    nsCOMPtr<nsISelectionPrivate> selPriv = do_QueryInterface(aSelection);
    if (selPriv) res = selPriv->SetInterlinePosition(true);
  }
  return res;
}

nsresult
nsTextEditRules::WillUndo(nsISelection *aSelection, bool *aCancel, bool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED
  
  *aCancel = false;
  *aHandled = false;
  return NS_OK;
}






nsresult
nsTextEditRules::DidUndo(nsISelection *aSelection, nsresult aResult)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  
  NS_ENSURE_SUCCESS(aResult, aResult);

  dom::Element* theRoot = mEditor->GetRoot();
  NS_ENSURE_TRUE(theRoot, NS_ERROR_FAILURE);
  nsIContent* node = mEditor->GetLeftmostChild(theRoot);
  if (node && mEditor->IsMozEditorBogusNode(node)) {
    mBogusNode = do_QueryInterface(node);
  } else {
    mBogusNode = nullptr;
  }
  return aResult;
}

nsresult
nsTextEditRules::WillRedo(nsISelection *aSelection, bool *aCancel, bool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  CANCEL_OPERATION_IF_READONLY_OR_DISABLED
  
  *aCancel = false;
  *aHandled = false;
  return NS_OK;
}

nsresult
nsTextEditRules::DidRedo(nsISelection *aSelection, nsresult aResult)
{
  nsresult res = aResult;  
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }
  if (NS_SUCCEEDED(res)) 
  {
    nsCOMPtr<nsIDOMElement> theRoot = do_QueryInterface(mEditor->GetRoot());
    NS_ENSURE_TRUE(theRoot, NS_ERROR_FAILURE);
    
    nsCOMPtr<nsIDOMHTMLCollection> nodeList;
    res = theRoot->GetElementsByTagName(NS_LITERAL_STRING("br"),
                                        getter_AddRefs(nodeList));
    NS_ENSURE_SUCCESS(res, res);
    if (nodeList)
    {
      uint32_t len;
      nodeList->GetLength(&len);
      
      if (len != 1) {
        
        mBogusNode = nullptr;
        return NS_OK;  
      }

      nsCOMPtr<nsIDOMNode> node;
      nodeList->Item(0, getter_AddRefs(node));
      nsCOMPtr<nsIContent> content = do_QueryInterface(node);
      MOZ_ASSERT(content);
      if (mEditor->IsMozEditorBogusNode(content)) {
        mBogusNode = node;
      } else {
        mBogusNode = nullptr;
      }
    }
  }
  return res;
}

nsresult
nsTextEditRules::WillOutputText(nsISelection *aSelection, 
                                const nsAString  *aOutputFormat,
                                nsAString *aOutString,                                
                                bool     *aCancel,
                                bool     *aHandled)
{
  
  if (!aOutString || !aOutputFormat || !aCancel || !aHandled) 
    { return NS_ERROR_NULL_POINTER; }

  
  *aCancel = false;
  *aHandled = false;

  nsAutoString outputFormat(*aOutputFormat);
  ToLowerCase(outputFormat);
  if (outputFormat.EqualsLiteral("text/plain"))
  { 
    if (IsPasswordEditor())
    {
      *aOutString = mPasswordText;
      *aHandled = true;
    }
    else if (mBogusNode)
    { 
      aOutString->Truncate();
      *aHandled = true;
    }
  }
  return NS_OK;
}

nsresult
nsTextEditRules::DidOutputText(nsISelection *aSelection, nsresult aResult)
{
  return NS_OK;
}

nsresult
nsTextEditRules::RemoveRedundantTrailingBR()
{
  
  if (mBogusNode)
    return NS_OK;

  
  if (IsSingleLineEditor())
    return NS_OK;

  nsRefPtr<dom::Element> body = mEditor->GetRoot();
  if (!body)
    return NS_ERROR_NULL_POINTER;

  uint32_t childCount = body->GetChildCount();
  if (childCount > 1) {
    
    return NS_OK;
  }

  nsRefPtr<nsIContent> child = body->GetFirstChild();
  if (!child || !child->IsElement()) {
    return NS_OK;
  }

  dom::Element* elem = child->AsElement();
  if (!nsTextEditUtils::IsMozBR(elem)) {
    return NS_OK;
  }

  
  
  elem->UnsetAttr(kNameSpaceID_None, nsGkAtoms::type, true);

  
  mBogusNode = do_QueryInterface(elem);

  
  elem->SetAttr(kNameSpaceID_None, kMOZEditorBogusNodeAttrAtom,
                kMOZEditorBogusNodeValue, false);
  return NS_OK;
}

nsresult
nsTextEditRules::CreateTrailingBRIfNeeded()
{
  
  if (IsSingleLineEditor()) {
    return NS_OK;
  }

  dom::Element* body = mEditor->GetRoot();
  NS_ENSURE_TRUE(body, NS_ERROR_NULL_POINTER);

  nsIContent* lastChild = body->GetLastChild();
  
  NS_ENSURE_TRUE(lastChild, NS_ERROR_NULL_POINTER);

  if (!lastChild->IsHTML(nsGkAtoms::br)) {
    nsAutoTxnsConserveSelection dontSpazMySelection(mEditor);
    nsCOMPtr<nsIDOMNode> domBody = do_QueryInterface(body);
    return CreateMozBR(domBody, body->Length());
  }

  
  
  if (!mEditor->IsMozEditorBogusNode(lastChild)) {
    return NS_OK;
  }

  
  lastChild->UnsetAttr(kNameSpaceID_None, kMOZEditorBogusNodeAttrAtom, false);
  lastChild->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                     NS_LITERAL_STRING("_moz"), true);
  return NS_OK;
}

nsresult
nsTextEditRules::CreateBogusNodeIfNeeded(nsISelection *aSelection)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(mEditor, NS_ERROR_NULL_POINTER);

  if (mBogusNode) {
    
    return NS_OK;
  }

  
  nsAutoRules beginRulesSniffing(mEditor, EditAction::ignore, nsIEditor::eNone);

  nsCOMPtr<dom::Element> body = mEditor->GetRoot();
  if (!body) {
    
    
    return NS_OK;
  }

  
  
  
  for (nsCOMPtr<nsIContent> bodyChild = body->GetFirstChild();
       bodyChild;
       bodyChild = bodyChild->GetNextSibling()) {
    if (mEditor->IsMozEditorBogusNode(bodyChild) ||
        !mEditor->IsEditable(body) || 
        mEditor->IsEditable(bodyChild)) {
      return NS_OK;
    }
  }

  
  if (!mEditor->IsModifiableNode(body)) {
    return NS_OK;
  }

  
  nsCOMPtr<dom::Element> newContent;
  nsresult rv = mEditor->CreateHTMLContent(NS_LITERAL_STRING("br"), getter_AddRefs(newContent));
  NS_ENSURE_SUCCESS(rv, rv);

  
  mBogusNode = do_QueryInterface(newContent);
  NS_ENSURE_TRUE(mBogusNode, NS_ERROR_NULL_POINTER);

  
  newContent->SetAttr(kNameSpaceID_None, kMOZEditorBogusNodeAttrAtom,
                      kMOZEditorBogusNodeValue, false);

  
  nsCOMPtr<nsIDOMNode> bodyNode = do_QueryInterface(body);
  rv = mEditor->InsertNode(mBogusNode, bodyNode, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  
  aSelection->CollapseNative(body, 0);
  return NS_OK;
}


nsresult
nsTextEditRules::TruncateInsertionIfNeeded(Selection* aSelection,
                                           const nsAString  *aInString,
                                           nsAString  *aOutString,
                                           int32_t          aMaxLength,
                                           bool *aTruncated)
{
  if (!aSelection || !aInString || !aOutString) {return NS_ERROR_NULL_POINTER;}
  
  nsresult res = NS_OK;
  *aOutString = *aInString;
  if (aTruncated) {
    *aTruncated = false;
  }
  
  if ((-1 != aMaxLength) && IsPlaintextEditor() && !mEditor->IsIMEComposing() )
  {
    
    
    
    
    
    
    
    
    
    
    
    
    int32_t docLength;
    res = mEditor->GetTextLength(&docLength);
    if (NS_FAILED(res)) { return res; }

    int32_t start, end;
    nsContentUtils::GetSelectionInTextControl(aSelection, mEditor->GetRoot(),
                                              start, end);

    int32_t oldCompStrLength = mEditor->GetIMEBufferLength();

    const int32_t selectionLength = end - start;
    const int32_t resultingDocLength = docLength - selectionLength - oldCompStrLength;
    if (resultingDocLength >= aMaxLength)
    {
      aOutString->Truncate();
      if (aTruncated) {
        *aTruncated = true;
      }
    }
    else
    {
      int32_t inCount = aOutString->Length();
      if (inCount + resultingDocLength > aMaxLength)
      {
        aOutString->Truncate(aMaxLength - resultingDocLength);
        if (aTruncated) {
          *aTruncated = true;
        }
      }
    }
  }
  return res;
}

void
nsTextEditRules::ResetIMETextPWBuf()
{
  mPasswordIMEText.Truncate();
}

void
nsTextEditRules::RemoveIMETextFromPWBuf(int32_t &aStart, nsAString *aIMEString)
{
  MOZ_ASSERT(aIMEString);

  
  if (mPasswordIMEText.IsEmpty()) {
    mPasswordIMEIndex = aStart;
  }
  else {
    
    mPasswordText.Cut(mPasswordIMEIndex, mPasswordIMEText.Length());
    aStart = mPasswordIMEIndex;
  }

  mPasswordIMEText.Assign(*aIMEString);
}

NS_IMETHODIMP nsTextEditRules::Notify(nsITimer *)
{
  MOZ_ASSERT(mTimer);

  
  
  nsresult res = IsPasswordEditor() ? HideLastPWInput() : NS_OK;
  ASSERT_PASSWORD_LENGTHS_EQUAL();
  mLastLength = 0;
  return res;
}

nsresult nsTextEditRules::HideLastPWInput() {
  if (!mLastLength) {
    
    return NS_OK;
  }

  nsAutoString hiddenText;
  FillBufWithPWChars(&hiddenText, mLastLength);

  nsRefPtr<Selection> selection = mEditor->GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  int32_t start, end;
  nsContentUtils::GetSelectionInTextControl(selection, mEditor->GetRoot(),
                                            start, end);

  nsCOMPtr<nsIDOMNode> selNode = GetTextNode(selection, mEditor);
  NS_ENSURE_TRUE(selNode, NS_OK);
  
  nsCOMPtr<nsIDOMCharacterData> nodeAsText(do_QueryInterface(selNode));
  NS_ENSURE_TRUE(nodeAsText, NS_OK);
  
  nodeAsText->ReplaceData(mLastStart, mLastLength, hiddenText);
  selection->Collapse(selNode, start);
  if (start != end)
    selection->Extend(selNode, end);
  return NS_OK;
}


void
nsTextEditRules::FillBufWithPWChars(nsAString *aOutString, int32_t aLength)
{
  MOZ_ASSERT(aOutString);

  
  PRUnichar passwordChar = LookAndFeel::GetPasswordCharacter();

  int32_t i;
  aOutString->Truncate();
  for (i=0; i < aLength; i++)
    aOutString->Append(passwordChar);
}





nsresult 
nsTextEditRules::CreateMozBR(nsIDOMNode* inParent, int32_t inOffset,
                             nsIDOMNode** outBRNode)
{
  NS_ENSURE_TRUE(inParent, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> brNode;
  nsresult res = mEditor->CreateBR(inParent, inOffset, address_of(brNode));
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsIDOMElement> brElem = do_QueryInterface(brNode);
  if (brElem) {
    res = mEditor->SetAttribute(brElem, NS_LITERAL_STRING("type"), NS_LITERAL_STRING("_moz"));
    NS_ENSURE_SUCCESS(res, res);
  }

  if (outBRNode) {
    brNode.forget(outBRNode);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsTextEditRules::DocumentModified()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
