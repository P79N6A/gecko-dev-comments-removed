





#include "nsHTMLEditRules.h"

#include <stdlib.h>

#include "mozilla/Assertions.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/Preferences.h"
#include "mozilla/dom/Selection.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/OwningNonNull.h"
#include "mozilla/mozalloc.h"
#include "nsAString.h"
#include "nsAlgorithm.h"
#include "nsCRT.h"
#include "nsCRTGlue.h"
#include "nsComponentManagerUtils.h"
#include "nsContentUtils.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsEditorUtils.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsHTMLCSSUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsHTMLEditor.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsID.h"
#include "nsIDOMCharacterData.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMText.h"
#include "nsIHTMLAbsPosEditor.h"
#include "nsIHTMLDocument.h"
#include "nsINode.h"
#include "nsLiteralString.h"
#include "nsPlaintextEditor.h"
#include "nsRange.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsStringFwd.h"
#include "nsTArray.h"
#include "nsTextEditUtils.h"
#include "nsThreadUtils.h"
#include "nsUnicharUtils.h"
#include "nsWSRunObject.h"
#include <algorithm>

class nsISupports;
class nsRulesInfo;

using namespace mozilla;
using namespace mozilla::dom;




enum
{
  kLonely = 0,
  kPrevSib = 1,
  kNextSib = 2,
  kBothSibs = 3
};





static bool IsBlockNode(nsIDOMNode* node)
{
  bool isBlock (false);
  nsHTMLEditor::NodeIsBlockStatic(node, &isBlock);
  return isBlock;
}

static bool IsInlineNode(nsIDOMNode* node)
{
  return !IsBlockNode(node);
}

static bool
IsStyleCachePreservingAction(EditAction action)
{
  return action == EditAction::deleteSelection ||
         action == EditAction::insertBreak ||
         action == EditAction::makeList ||
         action == EditAction::indent ||
         action == EditAction::outdent ||
         action == EditAction::align ||
         action == EditAction::makeBasicBlock ||
         action == EditAction::removeList ||
         action == EditAction::makeDefListItem ||
         action == EditAction::insertElement ||
         action == EditAction::insertQuotation;
}
 
class nsTableCellAndListItemFunctor : public nsBoolDomIterFunctor
{
  public:
    
    virtual bool operator()(nsIDOMNode* aNode) const
    {
      if (nsHTMLEditUtils::IsTableCell(aNode)) return true;
      if (nsHTMLEditUtils::IsListItem(aNode)) return true;
      return false;
    }
};

class nsBRNodeFunctor : public nsBoolDomIterFunctor
{
  public:
    virtual bool operator()(nsIDOMNode* aNode) const
    {
      if (nsTextEditUtils::IsBreak(aNode)) return true;
      return false;
    }
};

class nsEmptyEditableFunctor : public nsBoolDomIterFunctor
{
  public:
    explicit nsEmptyEditableFunctor(nsHTMLEditor* editor) : mHTMLEditor(editor) {}
    virtual bool operator()(nsIDOMNode* aNode) const
    {
      if (mHTMLEditor->IsEditable(aNode) &&
        (nsHTMLEditUtils::IsListItem(aNode) ||
        nsHTMLEditUtils::IsTableCellOrCaption(aNode)))
      {
        bool bIsEmptyNode;
        nsresult res = mHTMLEditor->IsEmptyNode(aNode, &bIsEmptyNode, false, false);
        NS_ENSURE_SUCCESS(res, false);
        if (bIsEmptyNode)
          return true;
      }
      return false;
    }
  protected:
    nsHTMLEditor* mHTMLEditor;
};

class nsEditableTextFunctor : public nsBoolDomIterFunctor
{
  public:
    explicit nsEditableTextFunctor(nsHTMLEditor* editor) : mHTMLEditor(editor) {}
    virtual bool operator()(nsIDOMNode* aNode) const
    {
      if (nsEditor::IsTextNode(aNode) && mHTMLEditor->IsEditable(aNode)) 
      {
        return true;
      }
      return false;
    }
  protected:
    nsHTMLEditor* mHTMLEditor;
};






nsHTMLEditRules::nsHTMLEditRules()
{
  InitFields();
}

void
nsHTMLEditRules::InitFields()
{
  mHTMLEditor = nullptr;
  mDocChangeRange = nullptr;
  mListenerEnabled = true;
  mReturnInEmptyLIKillsList = true;
  mDidDeleteSelection = false;
  mDidRangedDelete = false;
  mRestoreContentEditableCount = false;
  mUtilRange = nullptr;
  mJoinOffset = 0;
  mNewBlock = nullptr;
  mRangeItem = new nsRangeStore();
  
  mCachedStyles[0] = StyleCache(nsGkAtoms::b, EmptyString(), EmptyString());
  mCachedStyles[1] = StyleCache(nsGkAtoms::i, EmptyString(), EmptyString());
  mCachedStyles[2] = StyleCache(nsGkAtoms::u, EmptyString(), EmptyString());
  mCachedStyles[3] = StyleCache(nsGkAtoms::font, NS_LITERAL_STRING("face"), EmptyString());
  mCachedStyles[4] = StyleCache(nsGkAtoms::font, NS_LITERAL_STRING("size"), EmptyString());
  mCachedStyles[5] = StyleCache(nsGkAtoms::font, NS_LITERAL_STRING("color"), EmptyString());
  mCachedStyles[6] = StyleCache(nsGkAtoms::tt, EmptyString(), EmptyString());
  mCachedStyles[7] = StyleCache(nsGkAtoms::em, EmptyString(), EmptyString());
  mCachedStyles[8] = StyleCache(nsGkAtoms::strong, EmptyString(), EmptyString());
  mCachedStyles[9] = StyleCache(nsGkAtoms::dfn, EmptyString(), EmptyString());
  mCachedStyles[10] = StyleCache(nsGkAtoms::code, EmptyString(), EmptyString());
  mCachedStyles[11] = StyleCache(nsGkAtoms::samp, EmptyString(), EmptyString());
  mCachedStyles[12] = StyleCache(nsGkAtoms::var, EmptyString(), EmptyString());
  mCachedStyles[13] = StyleCache(nsGkAtoms::cite, EmptyString(), EmptyString());
  mCachedStyles[14] = StyleCache(nsGkAtoms::abbr, EmptyString(), EmptyString());
  mCachedStyles[15] = StyleCache(nsGkAtoms::acronym, EmptyString(), EmptyString());
  mCachedStyles[16] = StyleCache(nsGkAtoms::backgroundColor, EmptyString(), EmptyString());
  mCachedStyles[17] = StyleCache(nsGkAtoms::sub, EmptyString(), EmptyString());
  mCachedStyles[18] = StyleCache(nsGkAtoms::sup, EmptyString(), EmptyString());
}

nsHTMLEditRules::~nsHTMLEditRules()
{
  
  
  
  
  
  if (mHTMLEditor)
    mHTMLEditor->RemoveEditActionListener(this);
}





NS_IMPL_ADDREF_INHERITED(nsHTMLEditRules, nsTextEditRules)
NS_IMPL_RELEASE_INHERITED(nsHTMLEditRules, nsTextEditRules)
NS_IMPL_QUERY_INTERFACE_INHERITED(nsHTMLEditRules, nsTextEditRules, nsIEditActionListener)






NS_IMETHODIMP
nsHTMLEditRules::Init(nsPlaintextEditor *aEditor)
{
  InitFields();

  mHTMLEditor = static_cast<nsHTMLEditor*>(aEditor);
  nsresult res;

  
  res = nsTextEditRules::Init(aEditor);
  NS_ENSURE_SUCCESS(res, res);

  
  static const char kPrefName[] =
    "editor.html.typing.returnInEmptyListItemClosesList";
  nsAdoptingCString returnInEmptyLIKillsList =
    Preferences::GetCString(kPrefName);

  
  
  mReturnInEmptyLIKillsList = !returnInEmptyLIKillsList.EqualsLiteral("false");

  
  nsCOMPtr<nsINode> node = mHTMLEditor->GetRoot();
  if (!node) {
    node = mHTMLEditor->GetDocument();
  }

  NS_ENSURE_STATE(node);

  mUtilRange = new nsRange(node);
   
  
  
  nsAutoLockRulesSniffing lockIt((nsTextEditRules*)this);
  if (!mDocChangeRange) {
    mDocChangeRange = new nsRange(node);
  }

  if (node->IsElement()) {
    ErrorResult rv;
    mDocChangeRange->SelectNode(*node, rv);
    AdjustSpecialBreaks();
  }

  
  res = mHTMLEditor->AddEditActionListener(this);

  return res;
}

NS_IMETHODIMP
nsHTMLEditRules::DetachEditor()
{
  if (mHTMLEditor) {
    mHTMLEditor->RemoveEditActionListener(this);
  }
  mHTMLEditor = nullptr;
  return nsTextEditRules::DetachEditor();
}

NS_IMETHODIMP
nsHTMLEditRules::BeforeEdit(EditAction action,
                            nsIEditor::EDirection aDirection)
{
  if (mLockRulesSniffing) return NS_OK;

  nsAutoLockRulesSniffing lockIt((nsTextEditRules*)this);
  mDidExplicitlySetInterline = false;

  if (!mActionNesting++)
  {
    
    mDidRangedDelete = false;
    
    
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();

    
    NS_ENSURE_STATE(selection->RangeCount());
    mRangeItem->startNode = selection->GetRangeAt(0)->GetStartParent();
    mRangeItem->startOffset = selection->GetRangeAt(0)->StartOffset();
    mRangeItem->endNode = selection->GetRangeAt(0)->GetEndParent();
    mRangeItem->endOffset = selection->GetRangeAt(0)->EndOffset();
    nsCOMPtr<nsIDOMNode> selStartNode = GetAsDOMNode(mRangeItem->startNode);
    nsCOMPtr<nsIDOMNode> selEndNode = GetAsDOMNode(mRangeItem->endNode);

    
    NS_ENSURE_STATE(mHTMLEditor);
    (mHTMLEditor->mRangeUpdater).RegisterRangeItem(mRangeItem);

    
    mDidDeleteSelection = false;
    
    
    if(mDocChangeRange)
    {
      
      mDocChangeRange->Reset(); 
    }
    if(mUtilRange)
    {
      
      mUtilRange->Reset(); 
    }

    
    if (action == EditAction::insertText ||
        action == EditAction::insertIMEText ||
        action == EditAction::deleteSelection ||
        IsStyleCachePreservingAction(action)) {
      nsCOMPtr<nsIDOMNode> selNode = selStartNode;
      if (aDirection == nsIEditor::eNext)
        selNode = selEndNode;
      nsresult res = CacheInlineStyles(selNode);
      NS_ENSURE_SUCCESS(res, res);
    }

    
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<nsIDOMDocument> doc = mHTMLEditor->GetDOMDocument();
    NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(doc);
    NS_ENSURE_TRUE(htmlDoc, NS_ERROR_FAILURE);
    if (htmlDoc->GetEditingState() == nsIHTMLDocument::eContentEditable) {
      htmlDoc->ChangeContentEditableCount(nullptr, +1);
      mRestoreContentEditableCount = true;
    }

    
    ConfirmSelectionInBody();
    
    mTheAction = action;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLEditRules::AfterEdit(EditAction action,
                           nsIEditor::EDirection aDirection)
{
  if (mLockRulesSniffing) return NS_OK;

  nsAutoLockRulesSniffing lockIt(this);

  NS_PRECONDITION(mActionNesting>0, "bad action nesting!");
  nsresult res = NS_OK;
  if (!--mActionNesting)
  {
    
    res = AfterEditInner(action, aDirection);

    
    NS_ENSURE_STATE(mHTMLEditor);
    (mHTMLEditor->mRangeUpdater).DropRangeItem(mRangeItem);

    
    if (mRestoreContentEditableCount) {
      NS_ENSURE_STATE(mHTMLEditor);
      nsCOMPtr<nsIDOMDocument> doc = mHTMLEditor->GetDOMDocument();
      NS_ENSURE_TRUE(doc, NS_ERROR_NOT_INITIALIZED);
      nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(doc);
      NS_ENSURE_TRUE(htmlDoc, NS_ERROR_FAILURE);
      if (htmlDoc->GetEditingState() == nsIHTMLDocument::eContentEditable) {
        htmlDoc->ChangeContentEditableCount(nullptr, -1);
      }
      mRestoreContentEditableCount = false;
    }
  }

  return res;
}


nsresult
nsHTMLEditRules::AfterEditInner(EditAction action,
                                nsIEditor::EDirection aDirection)
{
  ConfirmSelectionInBody();
  if (action == EditAction::ignore) return NS_OK;
  
  NS_ENSURE_STATE(mHTMLEditor);
  nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();
  NS_ENSURE_STATE(selection);
  
  nsCOMPtr<nsIDOMNode> rangeStartParent, rangeEndParent;
  int32_t rangeStartOffset = 0, rangeEndOffset = 0;
  
  bool bDamagedRange = false;  
  if (mDocChangeRange)
  {  
    mDocChangeRange->GetStartContainer(getter_AddRefs(rangeStartParent));
    mDocChangeRange->GetEndContainer(getter_AddRefs(rangeEndParent));
    mDocChangeRange->GetStartOffset(&rangeStartOffset);
    mDocChangeRange->GetEndOffset(&rangeEndOffset);
    if (rangeStartParent && rangeEndParent) 
      bDamagedRange = true; 
  }
  
  nsresult res;
  if (bDamagedRange && !((action == EditAction::undo) || (action == EditAction::redo)))
  {
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
   
    
    PromoteRange(*mDocChangeRange, action);

    
    
    
    
    
    if ((action == EditAction::deleteSelection) && mDidRangedDelete)
    {
      res = InsertBRIfNeeded(selection);
      NS_ENSURE_SUCCESS(res, res);
    }  
    
    
    AdjustSpecialBreaks();
    
    
    if ( (action != EditAction::insertText &&
         action != EditAction::insertIMEText) )
    {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->CollapseAdjacentTextNodes(mDocChangeRange);
      NS_ENSURE_SUCCESS(res, res);
    }

    
    res = RemoveEmptyNodes();
    NS_ENSURE_SUCCESS(res, res);

    
    if ((action == EditAction::insertText) || 
        (action == EditAction::insertIMEText) ||
        (action == EditAction::deleteSelection) ||
        (action == EditAction::insertBreak) || 
        (action == EditAction::htmlPaste ||
        (action == EditAction::loadHTML)))
    {
      res = AdjustWhitespace(selection);
      NS_ENSURE_SUCCESS(res, res);
      
      
      NS_ENSURE_STATE(mHTMLEditor);
      nsWSRunObject(mHTMLEditor, mRangeItem->startNode,
                    mRangeItem->startOffset).AdjustWhitespace();
      
      if (mRangeItem->startNode != mRangeItem->endNode ||
          mRangeItem->startOffset != mRangeItem->endOffset) {
        NS_ENSURE_STATE(mHTMLEditor);
        nsWSRunObject(mHTMLEditor, mRangeItem->endNode,
                      mRangeItem->endOffset).AdjustWhitespace();
      }
    }
    
    
    if (mNewBlock)
    {
      res = PinSelectionToNewBlock(selection);
      mNewBlock = 0;
    }

    
    if ((action == EditAction::insertText) || 
        (action == EditAction::insertIMEText) ||
        (action == EditAction::deleteSelection) ||
        (action == EditAction::insertBreak) || 
        (action == EditAction::htmlPaste ||
        (action == EditAction::loadHTML)))
    {
      res = AdjustSelection(selection, aDirection);
      NS_ENSURE_SUCCESS(res, res);
    }

    
    if (action == EditAction::insertText ||
        action == EditAction::insertIMEText ||
        action == EditAction::deleteSelection ||
        IsStyleCachePreservingAction(action)) {
      NS_ENSURE_STATE(mHTMLEditor);
      mHTMLEditor->mTypeInState->UpdateSelState(selection);
      res = ReapplyCachedStyles();
      NS_ENSURE_SUCCESS(res, res);
      ClearCachedStyles();
    }    
  }

  NS_ENSURE_STATE(mHTMLEditor);
  
  res = mHTMLEditor->HandleInlineSpellCheck(action, selection, 
                                            GetAsDOMNode(mRangeItem->startNode),
                                            mRangeItem->startOffset,
                                            rangeStartParent, rangeStartOffset,
                                            rangeEndParent, rangeEndOffset);
  NS_ENSURE_SUCCESS(res, res);

  
  res = CreateBogusNodeIfNeeded(selection);
  
  
  NS_ENSURE_SUCCESS(res, res);
  
  if (!mDidExplicitlySetInterline)
  {
    res = CheckInterlinePosition(selection);
  }
  
  return res;
}


NS_IMETHODIMP
nsHTMLEditRules::WillDoAction(Selection* aSelection,
                              nsRulesInfo* aInfo,
                              bool* aCancel,
                              bool* aHandled)
{
  MOZ_ASSERT(aInfo && aCancel && aHandled);

  *aCancel = false;
  *aHandled = false;

  
  nsTextRulesInfo *info = static_cast<nsTextRulesInfo*>(aInfo);

  
  
  if (info->action == EditAction::outputText ||
      info->action == EditAction::undo ||
      info->action == EditAction::redo) {
    return nsTextEditRules::WillDoAction(aSelection, aInfo, aCancel, aHandled);
  }

  
  if (!aSelection) {
    return NS_OK;
  }
  NS_ENSURE_TRUE(aSelection->RangeCount(), NS_OK);

  nsRefPtr<nsRange> range = aSelection->GetRangeAt(0);
  nsCOMPtr<nsINode> selStartNode = range->GetStartParent();

  NS_ENSURE_STATE(mHTMLEditor);
  if (!mHTMLEditor->IsModifiableNode(selStartNode)) {
    *aCancel = true;
    return NS_OK;
  }

  nsCOMPtr<nsINode> selEndNode = range->GetEndParent();

  if (selStartNode != selEndNode) {
    NS_ENSURE_STATE(mHTMLEditor);
    if (!mHTMLEditor->IsModifiableNode(selEndNode)) {
      *aCancel = true;
      return NS_OK;
    }

    NS_ENSURE_STATE(mHTMLEditor);
    if (!mHTMLEditor->IsModifiableNode(range->GetCommonAncestor())) {
      *aCancel = true;
      return NS_OK;
    }
  }

  switch (info->action) {
    case EditAction::insertText:
    case EditAction::insertIMEText:
      return WillInsertText(info->action, aSelection, aCancel, aHandled,
                            info->inString, info->outString, info->maxLength);
    case EditAction::loadHTML:
      return WillLoadHTML(aSelection, aCancel);
    case EditAction::insertBreak:
      return WillInsertBreak(aSelection, aCancel, aHandled);
    case EditAction::deleteSelection:
      return WillDeleteSelection(aSelection, info->collapsedAction,
                                 info->stripWrappers, aCancel, aHandled);
    case EditAction::makeList:
      return WillMakeList(aSelection, info->blockType, info->entireList,
                          info->bulletType, aCancel, aHandled);
    case EditAction::indent:
      return WillIndent(aSelection, aCancel, aHandled);
    case EditAction::outdent:
      return WillOutdent(aSelection, aCancel, aHandled);
    case EditAction::setAbsolutePosition:
      return WillAbsolutePosition(aSelection, aCancel, aHandled);
    case EditAction::removeAbsolutePosition:
      return WillRemoveAbsolutePosition(aSelection, aCancel, aHandled);
    case EditAction::align:
      return WillAlign(aSelection, info->alignType, aCancel, aHandled);
    case EditAction::makeBasicBlock:
      return WillMakeBasicBlock(aSelection, info->blockType, aCancel, aHandled);
    case EditAction::removeList:
      return WillRemoveList(aSelection, info->bOrdered, aCancel, aHandled);
    case EditAction::makeDefListItem:
      return WillMakeDefListItem(aSelection, info->blockType, info->entireList,
                                 aCancel, aHandled);
    case EditAction::insertElement:
      return WillInsert(aSelection, aCancel);
    case EditAction::decreaseZIndex:
      return WillRelativeChangeZIndex(aSelection, -1, aCancel, aHandled);
    case EditAction::increaseZIndex:
      return WillRelativeChangeZIndex(aSelection, 1, aCancel, aHandled);
    default:
      return nsTextEditRules::WillDoAction(aSelection, aInfo,
                                           aCancel, aHandled);
  }
}


NS_IMETHODIMP
nsHTMLEditRules::DidDoAction(Selection* aSelection, nsRulesInfo* aInfo,
                             nsresult aResult)
{
  nsTextRulesInfo *info = static_cast<nsTextRulesInfo*>(aInfo);
  switch (info->action)
  {
    case EditAction::insertBreak:
      return DidInsertBreak(aSelection, aResult);
    case EditAction::deleteSelection:
      return DidDeleteSelection(aSelection, info->collapsedAction, aResult);
    case EditAction::makeBasicBlock:
    case EditAction::indent:
    case EditAction::outdent:
    case EditAction::align:
      return DidMakeBasicBlock(aSelection, aInfo, aResult);
    case EditAction::setAbsolutePosition: {
      nsresult rv = DidMakeBasicBlock(aSelection, aInfo, aResult);
      NS_ENSURE_SUCCESS(rv, rv);
      return DidAbsolutePosition();
    }
    default:
      
      return nsTextEditRules::DidDoAction(aSelection, aInfo, aResult);
  }
}
  
nsresult
nsHTMLEditRules::GetListState(bool *aMixed, bool *aOL, bool *aUL, bool *aDL)
{
  NS_ENSURE_TRUE(aMixed && aOL && aUL && aDL, NS_ERROR_NULL_POINTER);
  *aMixed = false;
  *aOL = false;
  *aUL = false;
  *aDL = false;
  bool bNonList = false;
  
  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  nsresult res = GetListActionNodes(arrayOfNodes, EntireList::no,
                                    TouchContent::no);
  NS_ENSURE_SUCCESS(res, res);

  
  for (const auto& curNode : arrayOfNodes) {
    if (!curNode->IsElement()) {
      bNonList = true;
    } else if (curNode->IsHTMLElement(nsGkAtoms::ul)) {
      *aUL = true;
    } else if (curNode->IsHTMLElement(nsGkAtoms::ol)) {
      *aOL = true;
    } else if (curNode->IsHTMLElement(nsGkAtoms::li)) {
      if (dom::Element* parent = curNode->GetParentElement()) {
        if (parent->IsHTMLElement(nsGkAtoms::ul)) {
          *aUL = true;
        } else if (parent->IsHTMLElement(nsGkAtoms::ol)) {
          *aOL = true;
        }
      }
    } else if (curNode->IsAnyOfHTMLElements(nsGkAtoms::dl,
                                            nsGkAtoms::dt,
                                            nsGkAtoms::dd)) {
      *aDL = true;
    } else {
      bNonList = true;
    }
  }  
  
  
  if ((*aUL + *aOL + *aDL + bNonList) > 1) {
    *aMixed = true;
  }

  return NS_OK;
}

nsresult 
nsHTMLEditRules::GetListItemState(bool *aMixed, bool *aLI, bool *aDT, bool *aDD)
{
  NS_ENSURE_TRUE(aMixed && aLI && aDT && aDD, NS_ERROR_NULL_POINTER);
  *aMixed = false;
  *aLI = false;
  *aDT = false;
  *aDD = false;
  bool bNonList = false;
  
  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  nsresult res = GetListActionNodes(arrayOfNodes, EntireList::no,
                                    TouchContent::no);
  NS_ENSURE_SUCCESS(res, res);

  
  for (const auto& node : arrayOfNodes) {
    if (!node->IsElement()) {
      bNonList = true;
    } else if (node->IsAnyOfHTMLElements(nsGkAtoms::ul,
                                         nsGkAtoms::ol,
                                         nsGkAtoms::li)) {
      *aLI = true;
    } else if (node->IsHTMLElement(nsGkAtoms::dt)) {
      *aDT = true;
    } else if (node->IsHTMLElement(nsGkAtoms::dd)) {
      *aDD = true;
    } else if (node->IsHTMLElement(nsGkAtoms::dl)) {
      
      bool bDT, bDD;
      GetDefinitionListItemTypes(node->AsElement(), &bDT, &bDD);
      *aDT |= bDT;
      *aDD |= bDD;
    } else {
      bNonList = true;
    }
  }  
  
  
  if ( (*aDT + *aDD + bNonList) > 1) *aMixed = true;
  
  return NS_OK;
}

nsresult 
nsHTMLEditRules::GetAlignment(bool *aMixed, nsIHTMLEditor::EAlignment *aAlign)
{
  
  
  
  
  

  

  
  NS_ENSURE_TRUE(aMixed && aAlign, NS_ERROR_NULL_POINTER);
  *aMixed = false;
  *aAlign = nsIHTMLEditor::eLeft;

  
  NS_ENSURE_STATE(mHTMLEditor);  
  nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();
  NS_ENSURE_STATE(selection);

  
  NS_ENSURE_STATE(mHTMLEditor);  
  nsCOMPtr<Element> rootElem = mHTMLEditor->GetRoot();
  NS_ENSURE_TRUE(rootElem, NS_ERROR_FAILURE);

  int32_t offset, rootOffset;
  nsCOMPtr<nsINode> parent = nsEditor::GetNodeLocation(rootElem, &rootOffset);
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(selection,
                                                    getter_AddRefs(parent),
                                                    &offset);
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsINode> nodeToExamine;
  if (selection->Collapsed()) {
    
    
    nodeToExamine = parent;
  }
  else if (!mHTMLEditor) {
    return NS_ERROR_UNEXPECTED;
  }
  else if (mHTMLEditor->IsTextNode(parent)) 
  {
    
    nodeToExamine = parent;
  } else if (parent->IsHTMLElement(nsGkAtoms::html) && offset == rootOffset) {
    
    NS_ENSURE_STATE(mHTMLEditor);
    nodeToExamine = mHTMLEditor->GetNextNode(parent, offset, true);
  }
  else
  {
    nsTArray<nsRefPtr<nsRange>> arrayOfRanges;
    GetPromotedRanges(*selection, arrayOfRanges, EditAction::align);

    
    nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
    res = GetNodesForOperation(arrayOfRanges, arrayOfNodes,
                               EditAction::align, TouchContent::no);
    NS_ENSURE_SUCCESS(res, res);                                 
    nodeToExamine = arrayOfNodes.SafeElementAt(0);
  }

  NS_ENSURE_TRUE(nodeToExamine, NS_ERROR_NULL_POINTER);

  NS_NAMED_LITERAL_STRING(typeAttrName, "align");
  nsIAtom  *dummyProperty = nullptr;
  nsCOMPtr<Element> blockParent;
  NS_ENSURE_STATE(mHTMLEditor);
  if (mHTMLEditor->IsBlockNode(nodeToExamine))
    blockParent = nodeToExamine->AsElement();
  else {
    NS_ENSURE_STATE(mHTMLEditor);
    blockParent = mHTMLEditor->GetBlockNodeParent(nodeToExamine);
  }

  NS_ENSURE_TRUE(blockParent, NS_ERROR_FAILURE);

  NS_ENSURE_STATE(mHTMLEditor);
  if (mHTMLEditor->IsCSSEnabled())
  {
    NS_ENSURE_STATE(mHTMLEditor);
    if (mHTMLEditor->mHTMLCSSUtils->IsCSSEditableProperty(blockParent,
                                                          dummyProperty,
                                                          &typeAttrName)) {
      
      nsAutoString value;
      
      NS_ENSURE_STATE(mHTMLEditor);
      mHTMLEditor->mHTMLCSSUtils->GetCSSEquivalentToHTMLInlineStyleSet(
        blockParent, dummyProperty, &typeAttrName, value,
        nsHTMLCSSUtils::eComputed);
      if (value.EqualsLiteral("center") ||
          value.EqualsLiteral("-moz-center") ||
          value.EqualsLiteral("auto auto"))
      {
        *aAlign = nsIHTMLEditor::eCenter;
        return NS_OK;
      }
      if (value.EqualsLiteral("right") ||
          value.EqualsLiteral("-moz-right") ||
          value.EqualsLiteral("auto 0px"))
      {
        *aAlign = nsIHTMLEditor::eRight;
        return NS_OK;
      }
      if (value.EqualsLiteral("justify"))
      {
        *aAlign = nsIHTMLEditor::eJustify;
        return NS_OK;
      }
      *aAlign = nsIHTMLEditor::eLeft;
      return NS_OK;
    }
  }

  
  bool isFirstNodeToExamine = true;
  while (nodeToExamine)
  {
    if (!isFirstNodeToExamine && nsHTMLEditUtils::IsTable(nodeToExamine))
    {
      
      
      
      return NS_OK;
    }
    if (nsHTMLEditUtils::SupportsAlignAttr(GetAsDOMNode(nodeToExamine))) {
      
      nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(nodeToExamine);
      if (elem)
      {
        nsAutoString typeAttrVal;
        res = elem->GetAttribute(NS_LITERAL_STRING("align"), typeAttrVal);
        ToLowerCase(typeAttrVal);
        if (NS_SUCCEEDED(res) && typeAttrVal.Length())
        {
          if (typeAttrVal.EqualsLiteral("center"))
            *aAlign = nsIHTMLEditor::eCenter;
          else if (typeAttrVal.EqualsLiteral("right"))
            *aAlign = nsIHTMLEditor::eRight;
          else if (typeAttrVal.EqualsLiteral("justify"))
            *aAlign = nsIHTMLEditor::eJustify;
          else
            *aAlign = nsIHTMLEditor::eLeft;
          return res;
        }
      }
    }
    isFirstNodeToExamine = false;
    nodeToExamine = nodeToExamine->GetParentNode(); 
  }
  return NS_OK;
}

static nsIAtom* MarginPropertyAtomForIndent(nsHTMLCSSUtils* aHTMLCSSUtils,
                                            nsIDOMNode* aNode) {
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node || !aNode, nsGkAtoms::marginLeft);
  nsAutoString direction;
  aHTMLCSSUtils->GetComputedProperty(*node, *nsGkAtoms::direction, direction);
  return direction.EqualsLiteral("rtl") ?
    nsGkAtoms::marginRight : nsGkAtoms::marginLeft;
}

nsresult 
nsHTMLEditRules::GetIndentState(bool *aCanIndent, bool *aCanOutdent)
{
  NS_ENSURE_TRUE(aCanIndent && aCanOutdent, NS_ERROR_FAILURE);
  *aCanIndent = true;    
  *aCanOutdent = false;

  
  NS_ENSURE_STATE(mHTMLEditor && mHTMLEditor->GetSelection());
  OwningNonNull<Selection> selection = *mHTMLEditor->GetSelection();

  
  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  nsresult res = GetNodesFromSelection(*selection, EditAction::indent,
                                       arrayOfNodes, TouchContent::no);
  NS_ENSURE_SUCCESS(res, res);

  
  
  
  NS_ENSURE_STATE(mHTMLEditor);
  bool useCSS = mHTMLEditor->IsCSSEnabled();
  for (auto& curNode : Reversed(arrayOfNodes)) {
    if (nsHTMLEditUtils::IsNodeThatCanOutdent(GetAsDOMNode(curNode))) {
      *aCanOutdent = true;
      break;
    }
    else if (useCSS) {
      
      NS_ENSURE_STATE(mHTMLEditor);
      nsIAtom* marginProperty =
        MarginPropertyAtomForIndent(mHTMLEditor->mHTMLCSSUtils,
                                    GetAsDOMNode(curNode));
      nsAutoString value;
      
      NS_ENSURE_STATE(mHTMLEditor);
      mHTMLEditor->mHTMLCSSUtils->GetSpecifiedProperty(*curNode,
                                                       *marginProperty, value);
      float f;
      nsCOMPtr<nsIAtom> unit;
      
      NS_ENSURE_STATE(mHTMLEditor);
      mHTMLEditor->mHTMLCSSUtils->ParseLength(value, &f, getter_AddRefs(unit));
      
      if (0 < f) {
        *aCanOutdent = true;
        break;
      }
    }
  }  
  
  if (!*aCanOutdent)
  {
    
    
    
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<nsIDOMNode> parent, tmp, root = do_QueryInterface(mHTMLEditor->GetRoot());
    NS_ENSURE_TRUE(root, NS_ERROR_NULL_POINTER);
    int32_t selOffset;
    NS_ENSURE_STATE(mHTMLEditor);
    nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->GetStartNodeAndOffset(selection, getter_AddRefs(parent), &selOffset);
    NS_ENSURE_SUCCESS(res, res);
    while (parent && (parent!=root))
    {
      if (nsHTMLEditUtils::IsNodeThatCanOutdent(parent))
      {
        *aCanOutdent = true;
        break;
      }
      tmp=parent;
      tmp->GetParentNode(getter_AddRefs(parent));
    }

    
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->GetEndNodeAndOffset(selection, getter_AddRefs(parent), &selOffset);
    NS_ENSURE_SUCCESS(res, res);
    while (parent && (parent!=root))
    {
      if (nsHTMLEditUtils::IsNodeThatCanOutdent(parent))
      {
        *aCanOutdent = true;
        break;
      }
      tmp=parent;
      tmp->GetParentNode(getter_AddRefs(parent));
    }
  }
  return res;
}


nsresult 
nsHTMLEditRules::GetParagraphState(bool *aMixed, nsAString &outFormat)
{
  
  
  NS_ENSURE_TRUE(aMixed, NS_ERROR_NULL_POINTER);
  *aMixed = true;
  outFormat.Truncate(0);
  
  bool bMixed = false;
  
  nsAutoString formatStr(NS_LITERAL_STRING("x")); 
  
  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  nsresult res = GetParagraphFormatNodes(arrayOfNodes, TouchContent::no);
  NS_ENSURE_SUCCESS(res, res);

  
  
  
  for (int32_t i = arrayOfNodes.Length() - 1; i >= 0; i--) {
    auto& curNode = arrayOfNodes[i];
    nsAutoString format;
    
    if (IsBlockNode(GetAsDOMNode(curNode)) &&
        !nsHTMLEditUtils::IsFormatNode(curNode)) {
      
      res = AppendInnerFormatNodes(arrayOfNodes, curNode);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  
  
  
  if (!arrayOfNodes.Length()) {
    nsCOMPtr<nsINode> selNode;
    int32_t selOffset;
    NS_ENSURE_STATE(mHTMLEditor);
    nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();
    NS_ENSURE_STATE(selection);
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->GetStartNodeAndOffset(selection, getter_AddRefs(selNode), &selOffset);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selNode, NS_ERROR_NULL_POINTER);
    arrayOfNodes.AppendElement(*selNode);
  }

  
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIDOMElement> rootElem = do_QueryInterface(mHTMLEditor->GetRoot());
  NS_ENSURE_TRUE(rootElem, NS_ERROR_NULL_POINTER);

  
  for (auto& curNode : Reversed(arrayOfNodes)) {
    nsAutoString format;
    
    if (nsHTMLEditUtils::IsFormatNode(curNode)) {
      GetFormatString(GetAsDOMNode(curNode), format);
    } else if (IsBlockNode(GetAsDOMNode(curNode))) {
      
      
      
      
      continue;
    }
    else
    {
      nsCOMPtr<nsIDOMNode> node, tmp = GetAsDOMNode(curNode);
      tmp->GetParentNode(getter_AddRefs(node));
      while (node)
      {
        if (node == rootElem)
        {
          format.Truncate(0);
          break;
        }
        else if (nsHTMLEditUtils::IsFormatNode(node))
        {
          GetFormatString(node, format);
          break;
        }
        
        tmp = node;
        tmp->GetParentNode(getter_AddRefs(node));
      }
    }
    
    
    if (formatStr.EqualsLiteral("x"))
      formatStr = format;
    
    else if (format != formatStr) 
    {
      bMixed = true;
      break; 
    }
  }  
  
  *aMixed = bMixed;
  outFormat = formatStr;
  return res;
}

nsresult
nsHTMLEditRules::AppendInnerFormatNodes(nsTArray<OwningNonNull<nsINode>>& aArray,
                                        nsINode* aNode)
{
  MOZ_ASSERT(aNode);

  
  
  
  
  bool foundInline = false;
  for (nsIContent* child = aNode->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    bool isBlock = IsBlockNode(child->AsDOMNode());
    bool isFormat = nsHTMLEditUtils::IsFormatNode(child);
    if (isBlock && !isFormat) {
      
      AppendInnerFormatNodes(aArray, child);
    } else if (isFormat) {
      aArray.AppendElement(*child);
    } else if (!foundInline) {
      
      foundInline = true;      
      aArray.AppendElement(*child);
    }
  }
  return NS_OK;
}

nsresult 
nsHTMLEditRules::GetFormatString(nsIDOMNode *aNode, nsAString &outFormat)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  if (nsHTMLEditUtils::IsFormatNode(aNode))
  {
    nsCOMPtr<nsIAtom> atom = nsEditor::GetTag(aNode);
    atom->ToString(outFormat);
  }
  else
    outFormat.Truncate();

  return NS_OK;
}    





nsresult
nsHTMLEditRules::WillInsert(Selection* aSelection, bool* aCancel)
{
  nsresult res = nsTextEditRules::WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res); 
  
  
  
  
  
  
  if (!aSelection->Collapsed()) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIDOMNode> selNode, priorNode;
  int32_t selOffset;
  
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(selNode),
                                           &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->GetPriorHTMLNode(selNode, selOffset,
                                      address_of(priorNode));
  if (NS_SUCCEEDED(res) && priorNode && nsTextEditUtils::IsMozBR(priorNode))
  {
    nsCOMPtr<nsIDOMNode> block1, block2;
    if (IsBlockNode(selNode)) {
      block1 = selNode;
    }
    else {
      NS_ENSURE_STATE(mHTMLEditor);
      block1 = mHTMLEditor->GetBlockNodeParent(selNode);
    }
    NS_ENSURE_STATE(mHTMLEditor);
    block2 = mHTMLEditor->GetBlockNodeParent(priorNode);
  
    if (block1 && block1 == block2) {
      
      
      
      selNode = nsEditor::GetNodeLocation(priorNode, &selOffset);
      res = aSelection->Collapse(selNode,selOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
  }

  if (mDidDeleteSelection &&
      (mTheAction == EditAction::insertText ||
       mTheAction == EditAction::insertIMEText ||
       mTheAction == EditAction::deleteSelection)) {
    res = ReapplyCachedStyles();
    NS_ENSURE_SUCCESS(res, res);
  }
  
  
  if (!IsStyleCachePreservingAction(mTheAction)) {
    ClearCachedStyles();
  }

  return NS_OK;
}    

nsresult
nsHTMLEditRules::WillInsertText(EditAction aAction,
                                Selection*       aSelection,
                                bool            *aCancel,
                                bool            *aHandled,
                                const nsAString *inString,
                                nsAString       *outString,
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
  nsresult res;
  
  
  if (!aSelection->Collapsed()) {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->DeleteSelection(nsIEditor::eNone, nsIEditor::eNoStrip);
    NS_ENSURE_SUCCESS(res, res);
  }

  res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);
  
  
  *aCancel = false;

  
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIDocument> doc = mHTMLEditor->GetDocument();
  nsCOMPtr<nsIDOMDocument> domDoc = mHTMLEditor->GetDOMDocument();
  NS_ENSURE_TRUE(doc && domDoc, NS_ERROR_NOT_INITIALIZED);

  
  res = CreateStyleForInsertText(aSelection, domDoc);
  NS_ENSURE_SUCCESS(res, res);
  
  
  NS_ENSURE_STATE(mHTMLEditor);
  NS_ENSURE_STATE(aSelection->GetRangeAt(0));
  nsCOMPtr<nsINode> selNode = aSelection->GetRangeAt(0)->GetStartParent();
  int32_t selOffset = aSelection->GetRangeAt(0)->StartOffset();
  NS_ENSURE_STATE(selNode);

  
  NS_ENSURE_STATE(mHTMLEditor);
  if (!mHTMLEditor->IsTextNode(selNode) &&
      (!mHTMLEditor || !mHTMLEditor->CanContainTag(*selNode,
                                                   *nsGkAtoms::textTagName))) {
    return NS_ERROR_FAILURE;
  }
    
  if (aAction == EditAction::insertIMEText) {
    
    
    if (inString->IsEmpty())
    {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->InsertTextImpl(*inString, address_of(selNode),
                                        &selOffset, doc);
    }
    else
    {
      NS_ENSURE_STATE(mHTMLEditor);
      nsWSRunObject wsObj(mHTMLEditor, selNode, selOffset);
      res = wsObj.InsertText(*inString, address_of(selNode), &selOffset, doc);
    }
    NS_ENSURE_SUCCESS(res, res);
  }
  else 
  {
    
    nsCOMPtr<nsINode> curNode = selNode;
    int32_t curOffset = selOffset;
    
    
    
    bool isPRE;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->IsPreformatted(GetAsDOMNode(selNode), &isPRE);
    NS_ENSURE_SUCCESS(res, res);    
    
    
    
    
    
    nsAutoLockListener lockit(&mListenerEnabled); 
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
    nsAutoString tString(*inString);
    const char16_t *unicodeBuf = tString.get();
    int32_t pos = 0;
    NS_NAMED_LITERAL_STRING(newlineStr, LFSTR);
        
    
    
    
    if (isPRE || IsPlaintextEditor())
    {
      while (unicodeBuf && (pos != -1) && (pos < (int32_t)(*inString).Length()))
      {
        int32_t oldPos = pos;
        int32_t subStrLen;
        pos = tString.FindChar(nsCRT::LF, oldPos);

        if (pos != -1) 
        {
          subStrLen = pos - oldPos;
          
          if (subStrLen == 0)
            subStrLen = 1;
        }
        else
        {
          subStrLen = tString.Length() - oldPos;
          pos = tString.Length();
        }

        nsDependentSubstring subStr(tString, oldPos, subStrLen);
        
        
        if (subStr.Equals(newlineStr))
        {
          NS_ENSURE_STATE(mHTMLEditor);
          nsCOMPtr<Element> br =
            mHTMLEditor->CreateBRImpl(address_of(curNode), &curOffset,
                                      nsIEditor::eNone);
          NS_ENSURE_STATE(br);
          pos++;
        }
        else
        {
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->InsertTextImpl(subStr, address_of(curNode),
                                            &curOffset, doc);
          NS_ENSURE_SUCCESS(res, res);
        }
      }
    }
    else
    {
      NS_NAMED_LITERAL_STRING(tabStr, "\t");
      NS_NAMED_LITERAL_STRING(spacesStr, "    ");
      char specialChars[] = {TAB, nsCRT::LF, 0};
      while (unicodeBuf && (pos != -1) && (pos < (int32_t)inString->Length()))
      {
        int32_t oldPos = pos;
        int32_t subStrLen;
        pos = tString.FindCharInSet(specialChars, oldPos);
        
        if (pos != -1) 
        {
          subStrLen = pos - oldPos;
          
          if (subStrLen == 0)
            subStrLen = 1;
        }
        else
        {
          subStrLen = tString.Length() - oldPos;
          pos = tString.Length();
        }

        nsDependentSubstring subStr(tString, oldPos, subStrLen);
        NS_ENSURE_STATE(mHTMLEditor);
        nsWSRunObject wsObj(mHTMLEditor, curNode, curOffset);

        
        if (subStr.Equals(tabStr))
        {
          res =
            wsObj.InsertText(spacesStr, address_of(curNode), &curOffset, doc);
          NS_ENSURE_SUCCESS(res, res);
          pos++;
        }
        
        else if (subStr.Equals(newlineStr))
        {
          nsCOMPtr<Element> br = wsObj.InsertBreak(address_of(curNode),
                                                   &curOffset,
                                                   nsIEditor::eNone);
          NS_ENSURE_TRUE(br, NS_ERROR_FAILURE);
          pos++;
        }
        else
        {
          res = wsObj.InsertText(subStr, address_of(curNode), &curOffset, doc);
          NS_ENSURE_SUCCESS(res, res);
        }
        NS_ENSURE_SUCCESS(res, res);
      }
    }
    aSelection->SetInterlinePosition(false);
    if (curNode) aSelection->Collapse(curNode, curOffset);
    
    
    if (!mDocChangeRange)
    {
      mDocChangeRange = new nsRange(selNode);
    }
    res = mDocChangeRange->SetStart(selNode, selOffset);
    NS_ENSURE_SUCCESS(res, res);
    if (curNode)
      res = mDocChangeRange->SetEnd(curNode, curOffset);
    else
      res = mDocChangeRange->SetEnd(selNode, selOffset);
    NS_ENSURE_SUCCESS(res, res);
  }
  return res;
}

nsresult
nsHTMLEditRules::WillLoadHTML(Selection* aSelection, bool* aCancel)
{
  NS_ENSURE_TRUE(aSelection && aCancel, NS_ERROR_NULL_POINTER);

  *aCancel = false;

  
  

  if (mBogusNode)
  {
    mEditor->DeleteNode(mBogusNode);
    mBogusNode = nullptr;
  }

  return NS_OK;
}

nsresult
nsHTMLEditRules::WillInsertBreak(Selection* aSelection,
                                 bool* aCancel, bool* aHandled)
{
  if (!aSelection || !aCancel || !aHandled) {
    return NS_ERROR_NULL_POINTER;
  }
  
  *aCancel = false;
  *aHandled = false;

  
  nsresult res = NS_OK;
  if (!aSelection->Collapsed()) {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->DeleteSelection(nsIEditor::eNone, nsIEditor::eStrip);
    NS_ENSURE_SUCCESS(res, res);
  }

  res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);

  
  
  *aCancel = false;

  
  
  if (IsMailEditor()) {
    res = SplitMailCites(aSelection, aHandled);
    NS_ENSURE_SUCCESS(res, res);
    if (*aHandled) {
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIDOMNode> node;
  int32_t offset;

  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(node),
                                           &offset);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  
  NS_ENSURE_STATE(mHTMLEditor);
  if (!mHTMLEditor->IsModifiableNode(node)) {
    *aCancel = true;
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNode> blockParent;
  if (IsBlockNode(node)) {
    blockParent = node;
  } else {
    NS_ENSURE_STATE(mHTMLEditor);
    blockParent = mHTMLEditor->GetBlockNodeParent(node);
  }
  NS_ENSURE_TRUE(blockParent, NS_ERROR_FAILURE);

  
  
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIContent> hostContent = mHTMLEditor->GetActiveEditingHost();
  nsCOMPtr<nsIDOMNode> hostNode = do_QueryInterface(hostContent);
  if (!nsEditorUtils::IsDescendantOf(blockParent, hostNode)) {
    res = StandardBreakImpl(node, offset, aSelection);
    NS_ENSURE_SUCCESS(res, res);
    *aHandled = true;
    return NS_OK;
  }

  
  
  
  
  bool isEmpty;
  IsEmptyBlock(blockParent, &isEmpty);
  if (isEmpty) {
    uint32_t blockLen;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->GetLengthOfDOMNode(blockParent, blockLen);
    NS_ENSURE_SUCCESS(res, res);
    nsCOMPtr<nsIDOMNode> brNode;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->CreateBR(blockParent, blockLen, address_of(brNode));
    NS_ENSURE_SUCCESS(res, res);
  }

  nsCOMPtr<nsIDOMNode> listItem = IsInListItem(blockParent);
  if (listItem && listItem != hostNode) {
    ReturnInListItem(aSelection, listItem, node, offset);
    *aHandled = true;
    return NS_OK;
  } else if (nsHTMLEditUtils::IsHeader(blockParent)) {
    
    ReturnInHeader(aSelection, blockParent, node, offset);
    *aHandled = true;
    return NS_OK;
  } else if (nsHTMLEditUtils::IsParagraph(blockParent)) {
    
    res = ReturnInParagraph(aSelection, blockParent, node, offset,
                            aCancel, aHandled);
    NS_ENSURE_SUCCESS(res, res);
    
  }

  
  if (!(*aHandled)) {
    *aHandled = true;
    return StandardBreakImpl(node, offset, aSelection);
  }
  return NS_OK;
}

nsresult
nsHTMLEditRules::StandardBreakImpl(nsIDOMNode* aNode, int32_t aOffset,
                                   Selection* aSelection)
{
  nsCOMPtr<nsIDOMNode> brNode;
  bool bAfterBlock = false;
  bool bBeforeBlock = false;
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> node(aNode);

  if (IsPlaintextEditor()) {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->CreateBR(node, aOffset, address_of(brNode));
  } else {
    NS_ENSURE_STATE(mHTMLEditor);
    nsWSRunObject wsObj(mHTMLEditor, node, aOffset);
    int32_t visOffset = 0, newOffset;
    WSType wsType;
    nsCOMPtr<nsINode> node_(do_QueryInterface(node)), visNode;
    wsObj.PriorVisibleNode(node_, aOffset, address_of(visNode),
                           &visOffset, &wsType);
    if (wsType & WSType::block) {
      bAfterBlock = true;
    }
    wsObj.NextVisibleNode(node_, aOffset, address_of(visNode),
                          &visOffset, &wsType);
    if (wsType & WSType::block) {
      bBeforeBlock = true;
    }
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<nsIDOMNode> linkNode;
    if (mHTMLEditor->IsInLink(node, address_of(linkNode))) {
      
      nsCOMPtr<nsIDOMNode> linkParent;
      res = linkNode->GetParentNode(getter_AddRefs(linkParent));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->SplitNodeDeep(linkNode, node, aOffset,
                                       &newOffset, true);
      NS_ENSURE_SUCCESS(res, res);
      
      node = linkParent;
      aOffset = newOffset;
    }
    node_ = do_QueryInterface(node);
    nsCOMPtr<Element> br =
      wsObj.InsertBreak(address_of(node_), &aOffset, nsIEditor::eNone);
    node = GetAsDOMNode(node_);
    brNode = GetAsDOMNode(br);
    NS_ENSURE_TRUE(brNode, NS_ERROR_FAILURE);
  }
  NS_ENSURE_SUCCESS(res, res);
  node = nsEditor::GetNodeLocation(brNode, &aOffset);
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);
  if (bAfterBlock && bBeforeBlock) {
    
    
    
    aSelection->SetInterlinePosition(true);
    res = aSelection->Collapse(node, aOffset);
  } else {
    NS_ENSURE_STATE(mHTMLEditor);
    nsWSRunObject wsObj(mHTMLEditor, node, aOffset+1);
    nsCOMPtr<nsINode> secondBR;
    int32_t visOffset = 0;
    WSType wsType;
    nsCOMPtr<nsINode> node_(do_QueryInterface(node));
    wsObj.NextVisibleNode(node_, aOffset+1, address_of(secondBR),
                          &visOffset, &wsType);
    if (wsType == WSType::br) {
      
      
      
      
      
      
      int32_t brOffset;
      nsCOMPtr<nsIDOMNode> brParent = nsEditor::GetNodeLocation(GetAsDOMNode(secondBR), &brOffset);
      if (brParent != node || brOffset != aOffset + 1) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(secondBR->AsContent(), node_, aOffset + 1);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
    
    
    
    

    
    
    nsCOMPtr<nsIDOMNode> siblingNode;
    brNode->GetNextSibling(getter_AddRefs(siblingNode));
    if (siblingNode && IsBlockNode(siblingNode)) {
      aSelection->SetInterlinePosition(false);
    } else {
      aSelection->SetInterlinePosition(true);
    }
    res = aSelection->Collapse(node, aOffset+1);
  }
  return res;
}

nsresult
nsHTMLEditRules::DidInsertBreak(Selection* aSelection, nsresult aResult)
{
  return NS_OK;
}


nsresult
nsHTMLEditRules::SplitMailCites(Selection* aSelection, bool* aHandled)
{
  NS_ENSURE_TRUE(aSelection && aHandled, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> leftCite, rightCite;
  nsCOMPtr<nsINode> selNode;
  nsCOMPtr<Element> citeNode;
  int32_t selOffset, newOffset;
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  citeNode = GetTopEnclosingMailCite(*selNode);
  NS_ENSURE_SUCCESS(res, res);
  if (citeNode)
  {
    
    
    
    
    
    
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsWSRunObject wsObj(mHTMLEditor, selNode, selOffset);
    nsCOMPtr<nsINode> visNode;
    int32_t visOffset=0;
    WSType wsType;
    wsObj.NextVisibleNode(selNode, selOffset, address_of(visNode),
                          &visOffset, &wsType);
    if (wsType == WSType::br) {
      
      if (visNode != citeNode && citeNode->Contains(visNode)) {
        
        NS_ENSURE_STATE(mHTMLEditor);
        selNode = mHTMLEditor->GetNodeLocation(visNode, &selOffset);
        ++selOffset;
      }
    }
     
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->SplitNodeDeep(GetAsDOMNode(citeNode),
                                     GetAsDOMNode(selNode), selOffset,
                                     &newOffset, true, address_of(leftCite),
                                     address_of(rightCite));
    NS_ENSURE_SUCCESS(res, res);
    selNode = citeNode->GetParentNode();
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<Element> brNode = mHTMLEditor->CreateBR(selNode, newOffset);
    NS_ENSURE_STATE(brNode);
    
    aSelection->SetInterlinePosition(true);
    res = aSelection->Collapse(selNode, newOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    
    
    if (IsInlineNode(GetAsDOMNode(citeNode))) {
      NS_ENSURE_STATE(mHTMLEditor);
      nsWSRunObject wsObj(mHTMLEditor, selNode, newOffset);
      nsCOMPtr<nsINode> visNode;
      int32_t visOffset=0;
      WSType wsType;
      wsObj.PriorVisibleNode(selNode, newOffset, address_of(visNode),
                             &visOffset, &wsType);
      if (wsType == WSType::normalWS || wsType == WSType::text ||
          wsType == WSType::special) {
        NS_ENSURE_STATE(mHTMLEditor);
        nsWSRunObject wsObjAfterBR(mHTMLEditor, selNode, newOffset+1);
        wsObjAfterBR.NextVisibleNode(selNode, newOffset + 1,
                                     address_of(visNode), &visOffset, &wsType);
        if (wsType == WSType::normalWS || wsType == WSType::text ||
            wsType == WSType::special) {
          NS_ENSURE_STATE(mHTMLEditor);
          brNode = mHTMLEditor->CreateBR(selNode, newOffset);
          NS_ENSURE_STATE(brNode);
        }
      }
    }
    
    bool bEmptyCite = false;
    if (leftCite)
    {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->IsEmptyNode(leftCite, &bEmptyCite, true, false);
      if (NS_SUCCEEDED(res) && bEmptyCite) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->DeleteNode(leftCite);
      }
      NS_ENSURE_SUCCESS(res, res);
    }
    if (rightCite)
    {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->IsEmptyNode(rightCite, &bEmptyCite, true, false);
      if (NS_SUCCEEDED(res) && bEmptyCite) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->DeleteNode(rightCite);
      }
      NS_ENSURE_SUCCESS(res, res);
    }
    *aHandled = true;
  }
  return NS_OK;
}


nsresult
nsHTMLEditRules::WillDeleteSelection(Selection* aSelection,
                                     nsIEditor::EDirection aAction,
                                     nsIEditor::EStripWrappers aStripWrappers,
                                     bool* aCancel,
                                     bool* aHandled)
{
  MOZ_ASSERT(aStripWrappers == nsIEditor::eStrip ||
             aStripWrappers == nsIEditor::eNoStrip);

  if (!aSelection || !aCancel || !aHandled) {
    return NS_ERROR_NULL_POINTER;
  }
  
  *aCancel = false;
  *aHandled = false;

  
  mDidDeleteSelection = true;

  
  if (mBogusNode) {
    *aCancel = true;
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMElement> cell;
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->GetFirstSelectedCell(nullptr, getter_AddRefs(cell));
  if (NS_SUCCEEDED(res) && cell) {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->DeleteTableCellContents();
    *aHandled = true;
    return res;
  }
  cell = nullptr;

  
  
  
  
  
  
  bool bCollapsed = aSelection->Collapsed();
  bool join = false;
  bool origCollapsed = bCollapsed;

  nsCOMPtr<nsINode> selNode;
  int32_t selOffset;

  NS_ENSURE_STATE(aSelection->GetRangeAt(0));
  nsCOMPtr<nsINode> startNode = aSelection->GetRangeAt(0)->GetStartParent();
  int32_t startOffset = aSelection->GetRangeAt(0)->StartOffset();
  NS_ENSURE_TRUE(startNode, NS_ERROR_FAILURE);


  if (bCollapsed) {
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<Element> host = mHTMLEditor->GetActiveEditingHost();
    NS_ENSURE_TRUE(host, NS_ERROR_FAILURE);
    res = CheckForEmptyBlock(startNode, host, aSelection, aHandled);
    NS_ENSURE_SUCCESS(res, res);
    if (*aHandled) {
      return NS_OK;
    }

    
    res = CheckBidiLevelForDeletion(aSelection, GetAsDOMNode(startNode),
                                    startOffset, aAction, aCancel);
    NS_ENSURE_SUCCESS(res, res);
    if (*aCancel) {
      return NS_OK;
    }

    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->ExtendSelectionForDelete(aSelection, &aAction);
    NS_ENSURE_SUCCESS(res, res);

    
    if (aAction == nsIEditor::eNone) {
      return NS_OK;
    }

    
    NS_ENSURE_STATE(aSelection->GetRangeAt(0));
    startNode = aSelection->GetRangeAt(0)->GetStartParent();
    startOffset = aSelection->GetRangeAt(0)->StartOffset();
    NS_ENSURE_TRUE(startNode, NS_ERROR_FAILURE);

    bCollapsed = aSelection->Collapsed();
  }

  if (bCollapsed) {
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsWSRunObject wsObj(mHTMLEditor, startNode, startOffset);
    nsCOMPtr<nsINode> visNode;
    int32_t visOffset;
    WSType wsType;

    
    if (aAction == nsIEditor::eNext) {
      wsObj.NextVisibleNode(startNode, startOffset, address_of(visNode),
                            &visOffset, &wsType);
    } else {
      wsObj.PriorVisibleNode(startNode, startOffset, address_of(visNode),
                             &visOffset, &wsType);
    }

    if (!visNode) {
      
      *aCancel = true;
      return res;
    }

    if (wsType == WSType::normalWS) {
      
      if (aAction == nsIEditor::eNext) {
        res = wsObj.DeleteWSForward();
      } else {
        res = wsObj.DeleteWSBackward();
      }
      *aHandled = true;
      NS_ENSURE_SUCCESS(res, res);
      res = InsertBRIfNeeded(aSelection);
      return res;
    }
    
    if (wsType == WSType::text) {
      
      OwningNonNull<Text> nodeAsText = *visNode->GetAsText();
      int32_t so = visOffset;
      int32_t eo = visOffset + 1;
      if (aAction == nsIEditor::ePrevious) {
        if (so == 0) {
          return NS_ERROR_UNEXPECTED;
        }
        so--;
        eo--;
        
        if (so > 0) {
          const nsTextFragment *text = nodeAsText->GetText();
          if (NS_IS_LOW_SURROGATE(text->CharAt(so)) &&
              NS_IS_HIGH_SURROGATE(text->CharAt(so - 1))) {
            so--;
          }
        }
      } else {
        nsRefPtr<nsRange> range = aSelection->GetRangeAt(0);
        NS_ENSURE_STATE(range);

        NS_ASSERTION(range->GetStartParent() == visNode,
                     "selection start not in visNode");
        NS_ASSERTION(range->GetEndParent() == visNode,
                     "selection end not in visNode");

        so = range->StartOffset();
        eo = range->EndOffset();
      }
      NS_ENSURE_STATE(mHTMLEditor);
      res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor,
          address_of(visNode), &so, address_of(visNode), &eo);
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteText(nodeAsText, std::min(so, eo),
                                    DeprecatedAbs(eo - so));
      *aHandled = true;
      NS_ENSURE_SUCCESS(res, res);
      res = InsertBRIfNeeded(aSelection);
      NS_ENSURE_SUCCESS(res, res);

      
      
      mDidRangedDelete = true;

      return NS_OK;
    }
    
    if (wsType == WSType::special || wsType == WSType::br ||
        visNode->IsHTMLElement(nsGkAtoms::hr)) {
      
      if (visNode->IsHTMLElement(nsGkAtoms::br) &&
          (!mHTMLEditor || !mHTMLEditor->IsVisBreak(visNode))) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->DeleteNode(visNode);
        NS_ENSURE_SUCCESS(res, res);
        return WillDeleteSelection(aSelection, aAction, aStripWrappers,
                                   aCancel, aHandled);
      }

      
      if (aAction == nsIEditor::ePrevious &&
          visNode->IsHTMLElement(nsGkAtoms::hr)) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        bool moveOnly = true;

        selNode = visNode->GetParentNode();
        selOffset = selNode ? selNode->IndexOf(visNode) : -1;

        bool interLineIsRight;
        res = aSelection->GetInterlinePosition(&interLineIsRight);
        NS_ENSURE_SUCCESS(res, res);

        if (startNode == selNode && startOffset - 1 == selOffset &&
            !interLineIsRight) {
          moveOnly = false;
        }

        if (moveOnly) {
          
          
          ++selOffset;
          aSelection->Collapse(selNode, selOffset);
          aSelection->SetInterlinePosition(false);
          mDidExplicitlySetInterline = true;
          *aHandled = true;

          
          

          WSType otherWSType;
          nsCOMPtr<nsINode> otherNode;
          int32_t otherOffset;

          wsObj.NextVisibleNode(startNode, startOffset, address_of(otherNode),
                                &otherOffset, &otherWSType);

          if (otherWSType == WSType::br) {
            

            NS_ENSURE_STATE(mHTMLEditor);
            nsCOMPtr<nsIContent> otherContent(do_QueryInterface(otherNode));
            res = nsWSRunObject::PrepareToDeleteNode(mHTMLEditor, otherContent);
            NS_ENSURE_SUCCESS(res, res);
            NS_ENSURE_STATE(mHTMLEditor);
            res = mHTMLEditor->DeleteNode(otherNode);
            NS_ENSURE_SUCCESS(res, res);
          }

          return NS_OK;
        }
        
      }

      
      NS_ENSURE_STATE(mHTMLEditor);
      NS_ENSURE_STATE(visNode->IsContent());
      res = nsWSRunObject::PrepareToDeleteNode(mHTMLEditor,
                                               visNode->AsContent());
      NS_ENSURE_SUCCESS(res, res);
      
      NS_ENSURE_STATE(mHTMLEditor);
      nsCOMPtr<nsIContent> sibling = mHTMLEditor->GetPriorHTMLSibling(visNode);
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(visNode);
      NS_ENSURE_SUCCESS(res, res);
      
      *aHandled = true;
      
      nsCOMPtr<nsINode> stepbrother;
      if (sibling) {
        NS_ENSURE_STATE(mHTMLEditor);
        stepbrother = mHTMLEditor->GetNextHTMLSibling(sibling);
      }
      
      if (startNode == stepbrother && startNode->GetAsText() &&
          sibling->GetAsText()) {
        ::DOMPoint pt = JoinNodesSmart(*sibling, *startNode->AsContent());
        NS_ENSURE_STATE(pt.node);
        
        res = aSelection->Collapse(pt.node, pt.offset);
        NS_ENSURE_SUCCESS(res, res);
      }
      res = InsertBRIfNeeded(aSelection);
      NS_ENSURE_SUCCESS(res, res);
      return NS_OK;
    }
    
    if (wsType == WSType::otherBlock) {
      
      
      if (nsHTMLEditUtils::IsTableElement(visNode)) {
        *aCancel = true;
        return NS_OK;
      }

      
      
      
      bool bDeletedBR = false;
      WSType otherWSType;
      nsCOMPtr<nsINode> otherNode;
      int32_t otherOffset;

      
      if (aAction == nsIEditor::eNext) {
        wsObj.PriorVisibleNode(startNode, startOffset, address_of(otherNode),
                               &otherOffset, &otherWSType);
      } else {
        wsObj.NextVisibleNode(startNode, startOffset, address_of(otherNode),
                              &otherOffset, &otherWSType);
      }

      
      nsCOMPtr<nsIContent> leafNode;
      nsCOMPtr<nsINode> leftNode, rightNode;
      if (aAction == nsIEditor::ePrevious) {
        NS_ENSURE_STATE(mHTMLEditor);
        leafNode = mHTMLEditor->GetLastEditableLeaf(*visNode);
        leftNode = leafNode;
        rightNode = startNode;
      } else {
        NS_ENSURE_STATE(mHTMLEditor);
        leafNode = mHTMLEditor->GetFirstEditableLeaf(*visNode);
        leftNode = startNode;
        rightNode = leafNode;
      }

      if (otherNode->IsHTMLElement(nsGkAtoms::br)) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->DeleteNode(otherNode);
        NS_ENSURE_SUCCESS(res, res);
        *aHandled = true;
        bDeletedBR = true;
      }

      
      if (leftNode && rightNode &&
          InDifferentTableElements(leftNode, rightNode)) {
        return NS_OK;
      }

      if (bDeletedBR) {
        
        NS_ENSURE_STATE(leafNode);
        ::DOMPoint newSel = GetGoodSelPointForNode(*leafNode, aAction);
        NS_ENSURE_STATE(newSel.node);
        aSelection->Collapse(newSel.node, newSel.offset);
        return NS_OK;
      }

      

      nsCOMPtr<nsINode> selPointNode = startNode;
      int32_t selPointOffset = startOffset;
      {
        NS_ENSURE_STATE(mHTMLEditor);
        nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater,
                                    address_of(selPointNode), &selPointOffset);
        res = JoinBlocks(GetAsDOMNode(leftNode), GetAsDOMNode(rightNode),
                         aCancel);
        *aHandled = true;
        NS_ENSURE_SUCCESS(res, res);
      }
      aSelection->Collapse(selPointNode, selPointOffset);
      return NS_OK;
    }
    
    if (wsType == WSType::thisBlock) {
      
      

      
      
      if (nsHTMLEditUtils::IsTableElement(visNode)) {
        *aCancel = true;
        return NS_OK;
      }

      
      nsCOMPtr<nsINode> leftNode, rightNode;
      if (aAction == nsIEditor::ePrevious) {
        NS_ENSURE_STATE(mHTMLEditor);
        leftNode = mHTMLEditor->GetPriorHTMLNode(visNode);
        rightNode = startNode;
      } else {
        NS_ENSURE_STATE(mHTMLEditor);
        rightNode = mHTMLEditor->GetNextHTMLNode(visNode);
        leftNode = startNode;
      }

      
      if (!leftNode || !rightNode) {
        *aCancel = true;
        return NS_OK;
      }

      
      if (InDifferentTableElements(leftNode, rightNode)) {
        *aCancel = true;
        return NS_OK;
      }

      nsCOMPtr<nsINode> selPointNode = startNode;
      int32_t selPointOffset = startOffset;
      {
        NS_ENSURE_STATE(mHTMLEditor);
        nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater,
                                    address_of(selPointNode), &selPointOffset);
        res = JoinBlocks(GetAsDOMNode(leftNode), GetAsDOMNode(rightNode),
                         aCancel);
        *aHandled = true;
        NS_ENSURE_SUCCESS(res, res);
      }
      aSelection->Collapse(selPointNode, selPointOffset);
      return NS_OK;
    }
  }


  
  res = ExpandSelectionForDeletion(aSelection);
  NS_ENSURE_SUCCESS(res, res);

  
  mDidRangedDelete = true;

  
  NS_ENSURE_STATE(aSelection->GetRangeAt(0));
  startNode = aSelection->GetRangeAt(0)->GetStartParent();
  startOffset = aSelection->GetRangeAt(0)->StartOffset();
  NS_ENSURE_TRUE(startNode, NS_ERROR_FAILURE);
  nsCOMPtr<nsINode> endNode = aSelection->GetRangeAt(0)->GetEndParent();
  int32_t endOffset = aSelection->GetRangeAt(0)->EndOffset();
  NS_ENSURE_TRUE(endNode, NS_ERROR_FAILURE);

  
  
  if (!IsPlaintextEditor()) {
    NS_ENSURE_STATE(mHTMLEditor);
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
    res = nsWSRunObject::PrepareToDeleteRange(mHTMLEditor,
                                           address_of(startNode), &startOffset,
                                           address_of(endNode), &endOffset);
    NS_ENSURE_SUCCESS(res, res);
  }

  {
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsAutoTrackDOMPoint startTracker(mHTMLEditor->mRangeUpdater,
                                     address_of(startNode), &startOffset);
    nsAutoTrackDOMPoint endTracker(mHTMLEditor->mRangeUpdater,
                                   address_of(endNode), &endOffset);
    
    *aHandled = true;

    if (endNode == startNode) {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteSelectionImpl(aAction, aStripWrappers);
      NS_ENSURE_SUCCESS(res, res);
    } else {
      
      nsCOMPtr<Element> startCiteNode = GetTopEnclosingMailCite(*startNode);
      nsCOMPtr<Element> endCiteNode = GetTopEnclosingMailCite(*endNode);

      
      
      
      if (startCiteNode && !endCiteNode) {
        aAction = nsIEditor::eNext;
      } else if (!startCiteNode && endCiteNode) {
        aAction = nsIEditor::ePrevious;
      }

      
      nsCOMPtr<Element> leftParent;
      if (IsBlockNode(GetAsDOMNode(startNode))) {
        leftParent = startNode->AsElement();
      } else {
        NS_ENSURE_STATE(mHTMLEditor);
        leftParent = mHTMLEditor->GetBlockNodeParent(startNode);
      }

      nsCOMPtr<Element> rightParent;
      if (IsBlockNode(GetAsDOMNode(endNode))) {
        rightParent = endNode->AsElement();
      } else {
        NS_ENSURE_STATE(mHTMLEditor);
        rightParent = mHTMLEditor->GetBlockNodeParent(endNode);
      }

      
      if (leftParent && leftParent == rightParent) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->DeleteSelectionImpl(aAction, aStripWrappers);
      } else {
        
        NS_ENSURE_STATE(leftParent && rightParent);

        
        nsCOMPtr<nsINode> leftBlockParent = leftParent->GetParentNode();
        nsCOMPtr<nsINode> rightBlockParent = rightParent->GetParentNode();

        
        NS_ENSURE_STATE(mHTMLEditor);
        if (leftBlockParent == rightBlockParent &&
            mHTMLEditor->NodesSameType(GetAsDOMNode(leftParent),
                                       GetAsDOMNode(rightParent))) {
          if (leftParent->IsHTMLElement(nsGkAtoms::p)) {
            
            NS_ENSURE_STATE(mHTMLEditor);
            res = mHTMLEditor->DeleteSelectionImpl(aAction, aStripWrappers);
            NS_ENSURE_SUCCESS(res, res);
            
            NS_ENSURE_STATE(mHTMLEditor);
            ::DOMPoint pt = mHTMLEditor->JoinNodeDeep(*leftParent, *rightParent);
            NS_ENSURE_STATE(pt.node);
            
            res = aSelection->Collapse(pt.node, pt.offset);
            NS_ENSURE_SUCCESS(res, res);
            return NS_OK;
          }
          if (nsHTMLEditUtils::IsListItem(leftParent) ||
              nsHTMLEditUtils::IsHeader(*leftParent)) {
            
            NS_ENSURE_STATE(mHTMLEditor);
            res = mHTMLEditor->DeleteSelectionImpl(aAction, aStripWrappers);
            NS_ENSURE_SUCCESS(res, res);
            
            NS_ENSURE_STATE(mHTMLEditor);
            ::DOMPoint pt = mHTMLEditor->JoinNodeDeep(*leftParent, *rightParent);
            NS_ENSURE_STATE(pt.node);
            
            res = aSelection->Collapse(pt.node, pt.offset);
            NS_ENSURE_SUCCESS(res, res);
            return NS_OK;
          }
        }

        
        
        join = true;

        uint32_t rangeCount = aSelection->RangeCount();
        for (uint32_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
          OwningNonNull<nsRange> range = *aSelection->GetRangeAt(rangeIdx);

          
          nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
          nsTrivialFunctor functor;
          nsDOMSubtreeIterator iter(*range);
          iter.AppendList(functor, arrayOfNodes);

          
          int32_t listCount = arrayOfNodes.Length();
          for (int32_t j = 0; j < listCount; j++) {
            nsCOMPtr<nsINode> somenode = do_QueryInterface(arrayOfNodes[0]);
            NS_ENSURE_STATE(somenode);
            DeleteNonTableElements(somenode);
            arrayOfNodes.RemoveElementAt(0);
            
            
            if (join && origCollapsed) {
              if (!somenode->IsContent()) {
                join = false;
                continue;
              }
              nsCOMPtr<nsIContent> content = somenode->AsContent();
              if (content->NodeType() == nsIDOMNode::TEXT_NODE) {
                NS_ENSURE_STATE(mHTMLEditor);
                mHTMLEditor->IsVisTextNode(content, &join, true);
              } else {
                NS_ENSURE_STATE(mHTMLEditor);
                join = content->IsHTMLElement(nsGkAtoms::br) &&
                       !mHTMLEditor->IsVisBreak(somenode);
              }
            }
          }
        }

        
        
        
        
        if (startNode->GetAsText() &&
            startNode->Length() > uint32_t(startOffset)) {
          
          OwningNonNull<nsGenericDOMDataNode> dataNode =
            *static_cast<nsGenericDOMDataNode*>(startNode.get());
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->DeleteText(dataNode, startOffset,
              startNode->Length() - startOffset);
          NS_ENSURE_SUCCESS(res, res);
        }
        if (endNode->GetAsText() && endOffset) {
          
          NS_ENSURE_STATE(mHTMLEditor);
          OwningNonNull<nsGenericDOMDataNode> dataNode =
            *static_cast<nsGenericDOMDataNode*>(endNode.get());
          res = mHTMLEditor->DeleteText(dataNode, 0, endOffset);
          NS_ENSURE_SUCCESS(res, res);
        }

        if (join) {
          res = JoinBlocks(GetAsDOMNode(leftParent), GetAsDOMNode(rightParent),
                           aCancel);
          NS_ENSURE_SUCCESS(res, res);
        }
      }
    }
  }
  
  
  
  
  
  
  
  
  if (aAction == (join ? nsIEditor::eNext : nsIEditor::ePrevious)) {
    res = aSelection->Collapse(endNode, endOffset);
  } else {
    res = aSelection->Collapse(startNode, startOffset);
  }
  NS_ENSURE_SUCCESS(res, res);
  return NS_OK;
}







nsresult
nsHTMLEditRules::InsertBRIfNeeded(Selection* aSelection)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  
  
  nsCOMPtr<nsINode> node;
  int32_t offset;
  nsresult res = mEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(node), &offset);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(node, NS_ERROR_FAILURE);

  
  if (!IsBlockNode(GetAsDOMNode(node))) {
    return NS_OK;
  }

  
  NS_ENSURE_STATE(mHTMLEditor);
  nsWSRunObject wsObj(mHTMLEditor, node, offset);
  if (((wsObj.mStartReason & WSType::block) ||
       (wsObj.mStartReason & WSType::br)) &&
      (wsObj.mEndReason & WSType::block)) {
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    if (mHTMLEditor->CanContainTag(*node, *nsGkAtoms::br)) {
      NS_ENSURE_STATE(mHTMLEditor);
      nsCOMPtr<Element> br =
        mHTMLEditor->CreateBR(node, offset, nsIEditor::ePrevious);
      return br ? NS_OK : NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}








::DOMPoint
nsHTMLEditRules::GetGoodSelPointForNode(nsINode& aNode,
                                        nsIEditor::EDirection aAction)
{
  NS_ENSURE_TRUE(mHTMLEditor, ::DOMPoint());
  if (aNode.GetAsText() || mHTMLEditor->IsContainer(&aNode)) {
    return ::DOMPoint(&aNode,
                      aAction == nsIEditor::ePrevious ? aNode.Length() : 0);
  }

  ::DOMPoint ret;
  ret.node = aNode.GetParentNode();
  ret.offset = ret.node ? ret.node->IndexOf(&aNode) : -1;
  NS_ENSURE_TRUE(mHTMLEditor, ::DOMPoint());
  if ((!aNode.IsHTMLElement(nsGkAtoms::br) ||
       mHTMLEditor->IsVisBreak(&aNode)) &&
      aAction == nsIEditor::ePrevious) {
    ret.offset++;
  }
  return ret;
}












nsresult
nsHTMLEditRules::JoinBlocks(nsIDOMNode *aLeftNode,
                            nsIDOMNode *aRightNode,
                            bool *aCanceled)
{
  NS_ENSURE_ARG_POINTER(aLeftNode && aRightNode);

  nsCOMPtr<nsIDOMNode> aLeftBlock, aRightBlock;

  if (IsBlockNode(aLeftNode)) {
    aLeftBlock = aLeftNode;
  } else if (aLeftNode) {
    NS_ENSURE_STATE(mHTMLEditor);
    aLeftBlock = mHTMLEditor->GetBlockNodeParent(aLeftNode);
  }

  if (IsBlockNode(aRightNode)) {
    aRightBlock = aRightNode;
  } else if (aRightNode) {
    NS_ENSURE_STATE(mHTMLEditor);
    aRightBlock = mHTMLEditor->GetBlockNodeParent(aRightNode);
  }

  
  NS_ENSURE_TRUE(aLeftBlock && aRightBlock, NS_ERROR_NULL_POINTER);
  NS_ENSURE_STATE(aLeftBlock != aRightBlock);

  if (nsHTMLEditUtils::IsTableElement(aLeftBlock) ||
      nsHTMLEditUtils::IsTableElement(aRightBlock)) {
    
    *aCanceled = true;
    return NS_OK;
  }

  
  if (nsHTMLEditUtils::IsHR(aLeftBlock)) {
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<nsIDOMNode> realLeft = mHTMLEditor->GetBlockNodeParent(aLeftBlock);
    aLeftBlock = realLeft;
  }
  if (nsHTMLEditUtils::IsHR(aRightBlock)) {
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<nsIDOMNode> realRight = mHTMLEditor->GetBlockNodeParent(aRightBlock);
    aRightBlock = realRight;
  }
  NS_ENSURE_STATE(aLeftBlock && aRightBlock);

  
  if (aLeftBlock == aRightBlock) {
    *aCanceled = true;
    return NS_OK;
  }
  
  
  if (nsHTMLEditUtils::IsList(aLeftBlock) &&
      nsHTMLEditUtils::IsListItem(aRightBlock)) {
    nsCOMPtr<nsIDOMNode> rightParent;
    aRightBlock->GetParentNode(getter_AddRefs(rightParent));
    if (rightParent == aLeftBlock) {
      return NS_OK;
    }
  }

  
  
  bool bMergeLists = false;
  nsIAtom* existingList = nsGkAtoms::_empty;
  int32_t theOffset;
  nsCOMPtr<nsIDOMNode> leftList, rightList;
  if (nsHTMLEditUtils::IsListItem(aLeftBlock) &&
      nsHTMLEditUtils::IsListItem(aRightBlock)) {
    aLeftBlock->GetParentNode(getter_AddRefs(leftList));
    aRightBlock->GetParentNode(getter_AddRefs(rightList));
    if (leftList && rightList && (leftList!=rightList))
    {
      
      
      
      
      if (!nsEditorUtils::IsDescendantOf(leftList, aRightBlock, &theOffset) &&
          !nsEditorUtils::IsDescendantOf(rightList, aLeftBlock, &theOffset))
      {
        aLeftBlock = leftList;
        aRightBlock = rightList;
        bMergeLists = true;
        NS_ENSURE_STATE(mHTMLEditor);
        existingList = mHTMLEditor->GetTag(leftList);
      }
    }
  }
  
  NS_ENSURE_STATE(mHTMLEditor);
  nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
  
  nsresult res = NS_OK;
  int32_t  rightOffset = 0;
  int32_t  leftOffset  = -1;

  
  
  if (nsEditorUtils::IsDescendantOf(aLeftBlock, aRightBlock, &rightOffset)) {
    
    
    rightOffset++;
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<nsINode> leftBlock(do_QueryInterface(aLeftBlock));
    res = nsWSRunObject::ScrubBlockBoundary(mHTMLEditor,
                                            nsWSRunObject::kBlockEnd,
                                            leftBlock);
    NS_ENSURE_SUCCESS(res, res);

    {
      nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater,
                                  address_of(aRightBlock), &rightOffset);
      nsCOMPtr<nsINode> rightBlock(do_QueryInterface(aRightBlock));
      res = nsWSRunObject::ScrubBlockBoundary(mHTMLEditor,
                                              nsWSRunObject::kAfterBlock,
                                              rightBlock, rightOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    nsCOMPtr<nsIDOMNode> brNode;
    res = CheckForInvisibleBR(aLeftBlock, kBlockEnd, address_of(brNode));
    NS_ENSURE_SUCCESS(res, res);
    if (bMergeLists)
    {
      
      
      nsCOMPtr<nsIContent> parent(do_QueryInterface(rightList));
      NS_ENSURE_TRUE(parent, NS_ERROR_NULL_POINTER);

      nsIContent *child = parent->GetChildAt(theOffset);
      nsCOMPtr<nsINode> leftList_ = do_QueryInterface(leftList);
      NS_ENSURE_STATE(leftList_);
      while (child)
      {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(child, leftList_, -1);
        NS_ENSURE_SUCCESS(res, res);

        child = parent->GetChildAt(rightOffset);
      }
    }
    else
    {
      res = MoveBlock(aLeftBlock, aRightBlock, leftOffset, rightOffset);
    }
    NS_ENSURE_STATE(mHTMLEditor);
    if (brNode) mHTMLEditor->DeleteNode(brNode);
  
  
  } else if (nsEditorUtils::IsDescendantOf(aRightBlock, aLeftBlock, &leftOffset)) {
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<nsINode> rightBlock(do_QueryInterface(aRightBlock));
    res = nsWSRunObject::ScrubBlockBoundary(mHTMLEditor,
                                            nsWSRunObject::kBlockStart,
                                            rightBlock);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_STATE(mHTMLEditor);
    {
      nsAutoTrackDOMPoint tracker(mHTMLEditor->mRangeUpdater,
                                  address_of(aLeftBlock), &leftOffset);
      nsCOMPtr<nsINode> leftBlock(do_QueryInterface(aLeftBlock));
      res = nsWSRunObject::ScrubBlockBoundary(mHTMLEditor,
                                              nsWSRunObject::kBeforeBlock,
                                              leftBlock, leftOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    nsCOMPtr<nsIDOMNode> brNode;
    res = CheckForInvisibleBR(aLeftBlock, kBeforeBlock, address_of(brNode),
                              leftOffset);
    NS_ENSURE_SUCCESS(res, res);
    if (bMergeLists)
    {
      res = MoveContents(rightList, leftList, &leftOffset);
    }
    else
    {
      
      
      

      int32_t previousContentOffset;
      nsCOMPtr<nsIDOMNode> previousContentParent;

      if (aLeftNode == aLeftBlock) {
        
        
        
        previousContentParent = aLeftBlock;
        previousContentOffset = leftOffset;
      } else {
        
        
        
        
        
        
        
        
        

        previousContentParent =
          nsEditor::GetNodeLocation(aLeftNode, &previousContentOffset);

        
        previousContentOffset++;
      }

      
      

      NS_ENSURE_STATE(mHTMLEditor);
      nsCOMPtr<nsINode> editorRoot = mHTMLEditor->GetEditorRoot();
      if (!editorRoot || aLeftNode != editorRoot->AsDOMNode()) {
        nsCOMPtr<nsIDOMNode> splittedPreviousContent;
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->SplitStyleAbovePoint(address_of(previousContentParent),
                                                &previousContentOffset,
                                                nullptr, nullptr, nullptr,
                                                address_of(splittedPreviousContent));
        NS_ENSURE_SUCCESS(res, res);

        if (splittedPreviousContent) {
          previousContentParent =
            nsEditor::GetNodeLocation(splittedPreviousContent,
                                      &previousContentOffset);
        }
      }

      res = MoveBlock(previousContentParent, aRightBlock,
                      previousContentOffset, rightOffset);
    }
    NS_ENSURE_STATE(mHTMLEditor);
    if (brNode) mHTMLEditor->DeleteNode(brNode);
  }
  else
  {
    
    
    
    
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<Element> leftBlock(do_QueryInterface(aLeftBlock));
    nsCOMPtr<Element> rightBlock(do_QueryInterface(aRightBlock));
    res = nsWSRunObject::PrepareToJoinBlocks(mHTMLEditor, leftBlock, rightBlock);
    NS_ENSURE_SUCCESS(res, res);
    
    nsCOMPtr<nsIDOMNode> brNode;
    res = CheckForInvisibleBR(aLeftBlock, kBlockEnd, address_of(brNode));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_STATE(mHTMLEditor);
    if (bMergeLists || mHTMLEditor->NodesSameType(aLeftBlock, aRightBlock)) {
      
      ::DOMPoint pt = JoinNodesSmart(*leftBlock, *rightBlock);
      if (pt.node && bMergeLists) {
        nsCOMPtr<nsIDOMNode> newBlock;
        res = ConvertListType(aRightBlock, address_of(newBlock),
                              existingList, nsGkAtoms::li);
      }
    }
    else
    {
      
      res = MoveBlock(aLeftBlock, aRightBlock, leftOffset, rightOffset);
    }
    if (NS_SUCCEEDED(res) && brNode)
    {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(brNode);
    }
  }
  return res;
}











nsresult
nsHTMLEditRules::MoveBlock(nsIDOMNode *aLeftBlock, nsIDOMNode *aRightBlock, int32_t aLeftOffset, int32_t aRightOffset)
{
  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  
  nsresult res = GetNodesFromPoint(::DOMPoint(aRightBlock,aRightOffset),
                                   EditAction::makeList, arrayOfNodes,
                                   TouchContent::no);
  NS_ENSURE_SUCCESS(res, res);
  for (auto& curNode : arrayOfNodes) {
    
    if (IsBlockNode(GetAsDOMNode(curNode))) {
      
      res = MoveContents(GetAsDOMNode(curNode), aLeftBlock, &aLeftOffset); 
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(curNode);
    }
    else
    {
      
      res = MoveNodeSmart(GetAsDOMNode(curNode), aLeftBlock, &aLeftOffset);
    }
  }
  return res;
}









nsresult
nsHTMLEditRules::MoveNodeSmart(nsIDOMNode *aSource, nsIDOMNode *aDest, int32_t *aOffset)
{
  nsCOMPtr<nsIContent> source = do_QueryInterface(aSource);
  nsCOMPtr<nsINode> dest = do_QueryInterface(aDest);
  NS_ENSURE_TRUE(source && dest && aOffset, NS_ERROR_NULL_POINTER);

  nsresult res;
  
  NS_ENSURE_STATE(mHTMLEditor);
  if (mHTMLEditor->CanContain(*dest, *source)) {
    
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->MoveNode(source, dest, *aOffset);
    NS_ENSURE_SUCCESS(res, res);
    if (*aOffset != -1) ++(*aOffset);
  }
  else
  {
    
    res = MoveContents(aSource, aDest, aOffset);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->DeleteNode(aSource);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}









nsresult
nsHTMLEditRules::MoveContents(nsIDOMNode *aSource, nsIDOMNode *aDest, int32_t *aOffset)
{
  NS_ENSURE_TRUE(aSource && aDest && aOffset, NS_ERROR_NULL_POINTER);
  if (aSource == aDest) return NS_ERROR_ILLEGAL_VALUE;
  NS_ENSURE_STATE(mHTMLEditor);
  NS_ASSERTION(!mHTMLEditor->IsTextNode(aSource), "#text does not have contents");
  
  nsCOMPtr<nsIDOMNode> child;
  nsAutoString tag;
  nsresult res;
  aSource->GetFirstChild(getter_AddRefs(child));
  while (child)
  {
    res = MoveNodeSmart(child, aDest, aOffset);
    NS_ENSURE_SUCCESS(res, res);
    aSource->GetFirstChild(getter_AddRefs(child));
  }
  return NS_OK;
}


nsresult
nsHTMLEditRules::DeleteNonTableElements(nsINode* aNode)
{
  MOZ_ASSERT(aNode);
  if (!nsHTMLEditUtils::IsTableElementButNotTable(aNode)) {
    NS_ENSURE_STATE(mHTMLEditor);
    return mHTMLEditor->DeleteNode(aNode->AsDOMNode());
  }

  for (int32_t i = aNode->GetChildCount() - 1; i >= 0; --i) {
    nsresult rv = DeleteNonTableElements(aNode->GetChildAt(i));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
nsHTMLEditRules::DidDeleteSelection(Selection* aSelection, 
                                    nsIEditor::EDirection aDir, 
                                    nsresult aResult)
{
  if (!aSelection) { return NS_ERROR_NULL_POINTER; }
  
  
  nsCOMPtr<nsINode> startNode;
  int32_t startOffset;
  nsresult res = mEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(startNode), &startOffset);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(startNode, NS_ERROR_FAILURE);
  
  
  nsCOMPtr<Element> citeNode = GetTopEnclosingMailCite(*startNode);
  if (citeNode) {
    bool isEmpty = true, seenBR = false;
    NS_ENSURE_STATE(mHTMLEditor);
    mHTMLEditor->IsEmptyNodeImpl(citeNode, &isEmpty, true, true, false,
                                 &seenBR);
    if (isEmpty)
    {
      int32_t offset;
      nsCOMPtr<nsINode> parent = nsEditor::GetNodeLocation(citeNode, &offset);
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(citeNode);
      NS_ENSURE_SUCCESS(res, res);
      if (parent && seenBR)
      {
        NS_ENSURE_STATE(mHTMLEditor);
        nsCOMPtr<Element> brNode = mHTMLEditor->CreateBR(parent, offset);
        NS_ENSURE_STATE(brNode);
        aSelection->Collapse(parent, offset);
      }
    }
  }
  
  
  return nsTextEditRules::DidDeleteSelection(aSelection, aDir, aResult);
}

nsresult
nsHTMLEditRules::WillMakeList(Selection* aSelection,
                              const nsAString* aListType,
                              bool aEntireList,
                              const nsAString* aBulletType,
                              bool* aCancel,
                              bool* aHandled,
                              const nsAString* aItemType)
{
  if (!aSelection || !aListType || !aCancel || !aHandled) {
    return NS_ERROR_NULL_POINTER;
  }
  OwningNonNull<nsIAtom> listType = do_GetAtom(*aListType);

  nsresult res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);

  
  
  *aCancel = false;
  *aHandled = false;

  
  nsCOMPtr<nsIAtom> itemType;
  if (aItemType) { 
    itemType = do_GetAtom(*aItemType);
    NS_ENSURE_TRUE(itemType, NS_ERROR_OUT_OF_MEMORY);
  } else if (listType == nsGkAtoms::dl) {
    itemType = nsGkAtoms::dd;
  } else {
    itemType = nsGkAtoms::li;
  }

  
  
  
  

  *aHandled = true;

  res = NormalizeSelection(aSelection);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_STATE(mHTMLEditor);
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);

  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  res = GetListActionNodes(arrayOfNodes,
                           aEntireList ? EntireList::yes : EntireList::no);
  NS_ENSURE_SUCCESS(res, res);

  
  bool bOnlyBreaks = true;
  for (auto& curNode : arrayOfNodes) {
    
    if (!nsTextEditUtils::IsBreak(curNode) &&
        !IsEmptyInline(GetAsDOMNode(curNode))) {
      bOnlyBreaks = false;
      break;
    }
  }

  
  
  if (!arrayOfNodes.Length() || bOnlyBreaks) {
    
    if (bOnlyBreaks) {
      for (auto& node : arrayOfNodes) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->DeleteNode(node);
        NS_ENSURE_SUCCESS(res, res);
      }
    }

    
    NS_ENSURE_STATE(aSelection->RangeCount());
    nsCOMPtr<nsINode> parent = aSelection->GetRangeAt(0)->GetStartParent();
    int32_t offset = aSelection->GetRangeAt(0)->StartOffset();
    NS_ENSURE_STATE(parent);

    
    NS_ENSURE_STATE(mHTMLEditor);
    if (!mHTMLEditor->CanContainTag(*parent, listType)) {
      *aCancel = true;
      return NS_OK;
    }
    res = SplitAsNeeded(listType, parent, offset);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<Element> theList =
      mHTMLEditor->CreateNode(listType, parent, offset);
    NS_ENSURE_STATE(theList);

    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<Element> theListItem =
      mHTMLEditor->CreateNode(itemType, theList, 0);
    NS_ENSURE_STATE(theListItem);

    
    mNewBlock = GetAsDOMNode(theListItem);
    
    res = aSelection->Collapse(theListItem, 0);
    
    selectionResetter.Abort();
    *aHandled = true;
    return res;
  }

  
  

  LookInsideDivBQandList(arrayOfNodes);

  
  

  uint32_t listCount = arrayOfNodes.Length();
  nsCOMPtr<nsINode> curParent;
  nsCOMPtr<Element> curList, prevListItem;

  for (uint32_t i = 0; i < listCount; i++) {
    
    nsCOMPtr<nsIDOMNode> newBlock;
    NS_ENSURE_STATE(arrayOfNodes[i]->IsContent());
    nsCOMPtr<nsIContent> curNode = arrayOfNodes[i]->AsContent();
    int32_t offset;
    curParent = nsEditor::GetNodeLocation(curNode, &offset);

    
    
    if (curList && InDifferentTableElements(curList, curNode)) {
      curList = nullptr;
    }

    
    if (nsTextEditUtils::IsBreak(curNode)) {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(curNode);
      NS_ENSURE_SUCCESS(res, res);
      prevListItem = 0;
      continue;
    } else if (IsEmptyInline(GetAsDOMNode(curNode))) {
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(curNode);
      NS_ENSURE_SUCCESS(res, res);
      continue;
    }

    if (nsHTMLEditUtils::IsList(curNode)) {
      
      if (curList && !nsEditorUtils::IsDescendantOf(curNode, curList)) {
        
        
        
        
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(curNode, curList, -1);
        NS_ENSURE_SUCCESS(res, res);
        res = ConvertListType(GetAsDOMNode(curNode), address_of(newBlock),
                              listType, itemType);
        NS_ENSURE_SUCCESS(res, res);
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->RemoveBlockContainer(newBlock);
        NS_ENSURE_SUCCESS(res, res);
      } else {
        
        res = ConvertListType(GetAsDOMNode(curNode), address_of(newBlock),
                              listType, itemType);
        NS_ENSURE_SUCCESS(res, res);
        curList = do_QueryInterface(newBlock);
      }
      prevListItem = 0;
      continue;
    }

    if (nsHTMLEditUtils::IsListItem(curNode)) {
      NS_ENSURE_STATE(mHTMLEditor);
      if (!curParent->IsHTMLElement(listType)) {
        
        
        if (!curList || nsEditorUtils::IsDescendantOf(curNode, curList)) {
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->SplitNode(curParent->AsDOMNode(), offset,
                                       getter_AddRefs(newBlock));
          NS_ENSURE_SUCCESS(res, res);
          int32_t offset;
          nsCOMPtr<nsINode> parent = nsEditor::GetNodeLocation(curParent,
                                                               &offset);
          NS_ENSURE_STATE(mHTMLEditor);
          curList = mHTMLEditor->CreateNode(listType, parent, offset);
          NS_ENSURE_STATE(curList);
        }
        
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(curNode, curList, -1);
        NS_ENSURE_SUCCESS(res, res);
        
        NS_ENSURE_STATE(mHTMLEditor);
        if (!curNode->IsHTMLElement(itemType)) {
          NS_ENSURE_STATE(mHTMLEditor);
          newBlock = dont_AddRef(GetAsDOMNode(
            mHTMLEditor->ReplaceContainer(curNode->AsElement(),
                                          itemType).take()));
          NS_ENSURE_STATE(newBlock);
        }
      } else {
        
        
        if (!curList) {
          curList = curParent->AsElement();
        } else if (curParent != curList) {
          
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->MoveNode(curNode, curList, -1);
          NS_ENSURE_SUCCESS(res, res);
        }
        NS_ENSURE_STATE(mHTMLEditor);
        if (!curNode->IsHTMLElement(itemType)) {
          NS_ENSURE_STATE(mHTMLEditor);
          newBlock = dont_AddRef(GetAsDOMNode(
            mHTMLEditor->ReplaceContainer(curNode->AsElement(),
                                          itemType).take()));
          NS_ENSURE_STATE(newBlock);
        }
      }
      nsCOMPtr<nsIDOMElement> curElement = do_QueryInterface(curNode);
      NS_NAMED_LITERAL_STRING(typestr, "type");
      if (aBulletType && !aBulletType->IsEmpty()) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->SetAttribute(curElement, typestr, *aBulletType);
      } else {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->RemoveAttribute(curElement, typestr);
      }
      NS_ENSURE_SUCCESS(res, res);
      continue;
    }

    
    
    if (curNode->IsHTMLElement(nsGkAtoms::div)) {
      prevListItem = nullptr;
      int32_t j = i + 1;
      GetInnerContent(*curNode, arrayOfNodes, &j);
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->RemoveContainer(curNode);
      NS_ENSURE_SUCCESS(res, res);
      listCount = arrayOfNodes.Length();
      continue;
    }

    
    if (!curList) {
      res = SplitAsNeeded(listType, curParent, offset);
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_STATE(mHTMLEditor);
      curList = mHTMLEditor->CreateNode(listType, curParent, offset);
      NS_ENSURE_SUCCESS(res, res);
      
      mNewBlock = GetAsDOMNode(curList);
      
      prevListItem = 0;
    }

    
    nsCOMPtr<Element> listItem;
    if (!nsHTMLEditUtils::IsListItem(curNode)) {
      if (IsInlineNode(GetAsDOMNode(curNode)) && prevListItem) {
        
        
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(curNode, prevListItem, -1);
        NS_ENSURE_SUCCESS(res, res);
      } else {
        
        if (curNode->IsHTMLElement(nsGkAtoms::p)) {
          NS_ENSURE_STATE(mHTMLEditor);
          listItem = mHTMLEditor->ReplaceContainer(curNode->AsElement(),
                                                   itemType);
          NS_ENSURE_STATE(listItem);
        } else {
          NS_ENSURE_STATE(mHTMLEditor);
          listItem = mHTMLEditor->InsertContainerAbove(curNode, itemType);
          NS_ENSURE_STATE(listItem);
        }
        if (IsInlineNode(GetAsDOMNode(curNode))) {
          prevListItem = listItem;
        } else {
          prevListItem = nullptr;
        }
      }
    } else {
      listItem = curNode->AsElement();
    }

    if (listItem) {
      
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->MoveNode(listItem, curList, -1);
      NS_ENSURE_SUCCESS(res, res);
    }
  }

  return res;
}


nsresult
nsHTMLEditRules::WillRemoveList(Selection* aSelection,
                                bool aOrdered, 
                                bool *aCancel,
                                bool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  
  *aCancel = false;
  *aHandled = true;
  
  nsresult res = NormalizeSelection(aSelection);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_STATE(mHTMLEditor);
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  
  nsTArray<nsRefPtr<nsRange>> arrayOfRanges;
  GetPromotedRanges(*aSelection, arrayOfRanges, EditAction::makeList);
  
  
  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  res = GetListActionNodes(arrayOfNodes, EntireList::no);
  NS_ENSURE_SUCCESS(res, res);                                 
                                     
  
  int32_t listCount = arrayOfNodes.Length();
  int32_t i;
  for (i=listCount-1; i>=0; i--)
  {
    OwningNonNull<nsINode> testNode = arrayOfNodes[i];
    NS_ENSURE_STATE(mHTMLEditor);
    if (!mHTMLEditor->IsEditable(testNode))
    {
      arrayOfNodes.RemoveElementAt(i);
    }
  }
  
  
  listCount = arrayOfNodes.Length();
  
  
  for (auto& curNode : arrayOfNodes) {
    
    if (nsHTMLEditUtils::IsListItem(curNode))  
    {
      bool bOutOfList;
      do
      {
        res = PopListItem(GetAsDOMNode(curNode), &bOutOfList);
        NS_ENSURE_SUCCESS(res, res);
      } while (!bOutOfList); 
    }
    else if (nsHTMLEditUtils::IsList(curNode)) 
    {
      res = RemoveListStructure(GetAsDOMNode(curNode));
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return res;
}


nsresult
nsHTMLEditRules::WillMakeDefListItem(Selection* aSelection,
                                     const nsAString *aItemType, 
                                     bool aEntireList, 
                                     bool *aCancel,
                                     bool *aHandled)
{
  
  NS_NAMED_LITERAL_STRING(listType, "dl");
  return WillMakeList(aSelection, &listType, aEntireList, nullptr, aCancel, aHandled, aItemType);
}

nsresult
nsHTMLEditRules::WillMakeBasicBlock(Selection* aSelection,
                                    const nsAString *aBlockType, 
                                    bool *aCancel,
                                    bool *aHandled)
{
  OwningNonNull<nsIAtom> blockType = do_GetAtom(*aBlockType);
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  
  *aCancel = false;
  *aHandled = false;
  
  nsresult res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);
  
  
  *aCancel = false;
  res = NormalizeSelection(aSelection);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_STATE(mHTMLEditor);
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  NS_ENSURE_STATE(mHTMLEditor);
  nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);
  *aHandled = true;
  nsString tString(*aBlockType);

  
  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  res = GetNodesFromSelection(*aSelection, EditAction::makeBasicBlock,
                              arrayOfNodes);
  NS_ENSURE_SUCCESS(res, res);

  
  int32_t listCount = arrayOfNodes.Length();
  int32_t i;
  for (i=listCount-1; i>=0; i--)
  {
    NS_ENSURE_STATE(mHTMLEditor);
    if (!mHTMLEditor->IsEditable(arrayOfNodes[i])) {
      arrayOfNodes.RemoveElementAt(i);
    }
  }

  
  listCount = arrayOfNodes.Length();
  
  
  if (ListIsEmptyLine(arrayOfNodes))
  {
    nsCOMPtr<nsIDOMNode> theBlock;
    
    
    NS_ENSURE_STATE(aSelection->RangeCount());
    nsCOMPtr<nsINode> parent = aSelection->GetRangeAt(0)->GetStartParent();
    int32_t offset = aSelection->GetRangeAt(0)->StartOffset();
    NS_ENSURE_STATE(parent);
    if (tString.EqualsLiteral("normal") ||
        tString.IsEmpty() ) 
    {
      nsCOMPtr<nsIDOMNode> curBlock = parent->AsDOMNode();
      if (!IsBlockNode(curBlock)) {
        NS_ENSURE_STATE(mHTMLEditor);
        curBlock = dont_AddRef(GetAsDOMNode(
          mHTMLEditor->GetBlockNodeParent(parent).take()));
      }
      nsCOMPtr<nsIDOMNode> curBlockPar;
      NS_ENSURE_TRUE(curBlock, NS_ERROR_NULL_POINTER);
      curBlock->GetParentNode(getter_AddRefs(curBlockPar));
      if (nsHTMLEditUtils::IsFormatNode(curBlock))
      {
        
        
        nsCOMPtr<nsIDOMNode> brNode;
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->GetNextHTMLNode(parent->AsDOMNode(), offset,
                                           address_of(brNode));
        NS_ENSURE_SUCCESS(res, res);        
        if (brNode && nsTextEditUtils::IsBreak(brNode))
        {
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->DeleteNode(brNode);
          NS_ENSURE_SUCCESS(res, res); 
        }
        
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->SplitNodeDeep(curBlock, parent->AsDOMNode(), offset,
                                         &offset, true);
        NS_ENSURE_SUCCESS(res, res);
        
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->CreateBR(curBlockPar, offset, address_of(brNode));
        NS_ENSURE_SUCCESS(res, res);
        
        res = aSelection->Collapse(curBlockPar, offset);
        selectionResetter.Abort();  
        *aHandled = true;
      }
      
    }
    else  
    {   
      
      NS_ENSURE_STATE(mHTMLEditor);
      nsCOMPtr<nsIContent> brNode =
        mHTMLEditor->GetNextHTMLNode(parent, offset, true);
      NS_ENSURE_SUCCESS(res, res);
      if (brNode && nsTextEditUtils::IsBreak(brNode))
      {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->DeleteNode(brNode);
        NS_ENSURE_SUCCESS(res, res);
        
        arrayOfNodes.RemoveElement(brNode);
      }
      
      res = SplitAsNeeded(blockType, parent, offset);
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_STATE(mHTMLEditor);
      theBlock = dont_AddRef(GetAsDOMNode(
        mHTMLEditor->CreateNode(blockType, parent, offset).take()));
      NS_ENSURE_STATE(theBlock);
      
      mNewBlock = theBlock;
      
      while (!arrayOfNodes.IsEmpty()) {
        OwningNonNull<nsINode> curNode = arrayOfNodes[0];
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->DeleteNode(curNode);
        NS_ENSURE_SUCCESS(res, res);
        arrayOfNodes.RemoveElementAt(0);
      }
      
      res = aSelection->Collapse(theBlock,0);
      selectionResetter.Abort();  
      *aHandled = true;
    }
    return res;    
  }
  else
  {
    
    
    
    if (tString.EqualsLiteral("blockquote")) {
      res = MakeBlockquote(arrayOfNodes);
    } else if (tString.EqualsLiteral("normal") || tString.IsEmpty()) {
      res = RemoveBlockStyle(arrayOfNodes);
    } else {
      res = ApplyBlockStyle(arrayOfNodes, *blockType);
    }
    return res;
  }
  return res;
}

nsresult 
nsHTMLEditRules::DidMakeBasicBlock(Selection* aSelection,
                                   nsRulesInfo *aInfo, nsresult aResult)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  
  if (!aSelection->Collapsed()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMNode> parent;
  int32_t offset;
  nsresult res = nsEditor::GetStartNodeAndOffset(aSelection, getter_AddRefs(parent), &offset);
  NS_ENSURE_SUCCESS(res, res);
  res = InsertMozBRIfNeeded(parent);
  return res;
}

nsresult
nsHTMLEditRules::WillIndent(Selection* aSelection,
                            bool* aCancel, bool* aHandled)
{
  nsresult res;
  NS_ENSURE_STATE(mHTMLEditor);
  if (mHTMLEditor->IsCSSEnabled()) {
    res = WillCSSIndent(aSelection, aCancel, aHandled);
  }
  else {
    res = WillHTMLIndent(aSelection, aCancel, aHandled);
  }
  return res;
}

nsresult
nsHTMLEditRules::WillCSSIndent(Selection* aSelection,
                               bool* aCancel, bool* aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  
  nsresult res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);

  
  
  *aCancel = false;
  *aHandled = true;

  res = NormalizeSelection(aSelection);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_STATE(mHTMLEditor);
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  nsTArray<OwningNonNull<nsRange>> arrayOfRanges;
  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  
  
  
  
  nsCOMPtr<Element> liNode;
  if (aSelection->Collapsed()) {
    nsCOMPtr<nsINode> node;
    nsCOMPtr<Element> block;
    int32_t offset;
    NS_ENSURE_STATE(mHTMLEditor);
    nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(node), &offset);
    NS_ENSURE_SUCCESS(res, res);
    if (IsBlockNode(GetAsDOMNode(node))) {
      block = node->AsElement();
    } else {
      NS_ENSURE_STATE(mHTMLEditor);
      block = mHTMLEditor->GetBlockNodeParent(node);
    }
    if (block && nsHTMLEditUtils::IsListItem(block))
      liNode = block;
  }
  
  if (liNode)
  {
    arrayOfNodes.AppendElement(*liNode);
  }
  else
  {
    
    
    
    
    res = GetNodesFromSelection(*aSelection, EditAction::indent, arrayOfNodes);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  if (ListIsEmptyLine(arrayOfNodes))
  {
    
    NS_ENSURE_STATE(aSelection->RangeCount());
    nsCOMPtr<nsINode> parent = aSelection->GetRangeAt(0)->GetStartParent();
    int32_t offset = aSelection->GetRangeAt(0)->StartOffset();
    NS_ENSURE_STATE(parent);

    
    res = SplitAsNeeded(*nsGkAtoms::div, parent, offset);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<Element> theBlock = mHTMLEditor->CreateNode(nsGkAtoms::div,
                                                         parent, offset);
    NS_ENSURE_STATE(theBlock);
    
    mNewBlock = theBlock->AsDOMNode();
    RelativeChangeIndentationOfElementNode(theBlock->AsDOMNode(), +1);
    
    while (!arrayOfNodes.IsEmpty()) {
      OwningNonNull<nsINode> curNode = arrayOfNodes[0];
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(curNode);
      NS_ENSURE_SUCCESS(res, res);
      arrayOfNodes.RemoveElementAt(0);
    }
    
    res = aSelection->Collapse(theBlock,0);
    selectionResetter.Abort();  
    *aHandled = true;
    return res;
  }
  
  
  
  int32_t i;
  nsCOMPtr<nsINode> curParent;
  nsCOMPtr<Element> curList, curQuote;
  nsCOMPtr<nsIContent> sibling;
  int32_t listCount = arrayOfNodes.Length();
  for (i=0; i<listCount; i++)
  {
    
    NS_ENSURE_STATE(arrayOfNodes[i]->IsContent());
    nsCOMPtr<nsIContent> curNode = arrayOfNodes[i]->AsContent();

    
    NS_ENSURE_STATE(mHTMLEditor);
    if (!mHTMLEditor->IsEditable(curNode)) continue;

    curParent = curNode->GetParentNode();
    int32_t offset = curParent ? curParent->IndexOf(curNode) : -1;
    
    
    if (nsHTMLEditUtils::IsList(curParent))
    {
      sibling = nullptr;

      
      
      
      NS_ENSURE_STATE(mHTMLEditor);
      sibling = mHTMLEditor->GetNextHTMLSibling(curNode);
      if (sibling && nsHTMLEditUtils::IsList(sibling))
      {
        if (curParent->NodeInfo()->NameAtom() == sibling->NodeInfo()->NameAtom() &&
            curParent->NodeInfo()->NamespaceID() == sibling->NodeInfo()->NamespaceID()) {
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->MoveNode(curNode, sibling, 0);
          NS_ENSURE_SUCCESS(res, res);
          continue;
        }
      }
      
      
      
      NS_ENSURE_STATE(mHTMLEditor);
      sibling = mHTMLEditor->GetPriorHTMLSibling(curNode);
      if (sibling && nsHTMLEditUtils::IsList(sibling))
      {
        if (curParent->NodeInfo()->NameAtom() == sibling->NodeInfo()->NameAtom() &&
            curParent->NodeInfo()->NamespaceID() == sibling->NodeInfo()->NamespaceID()) {
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->MoveNode(curNode, sibling, -1);
          NS_ENSURE_SUCCESS(res, res);
          continue;
        }
      }
      sibling = nullptr;
      
      
      
      if (curList) {
        NS_ENSURE_STATE(mHTMLEditor);
        sibling = mHTMLEditor->GetPriorHTMLSibling(curNode);
      }

      if (!curList || (sibling && sibling != curList)) {
        
        res = SplitAsNeeded(*curParent->NodeInfo()->NameAtom(), curParent,
                            offset);
        NS_ENSURE_SUCCESS(res, res);
        NS_ENSURE_STATE(mHTMLEditor);
        curList = mHTMLEditor->CreateNode(curParent->NodeInfo()->NameAtom(),
                                          curParent, offset);
        NS_ENSURE_STATE(curList);
        
        
        mNewBlock = curList->AsDOMNode();
      }
      
      uint32_t listLen = curList->Length();
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->MoveNode(curNode, curList, listLen);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    else 
    {
      if (IsBlockNode(curNode->AsDOMNode())) {
        RelativeChangeIndentationOfElementNode(curNode->AsDOMNode(), +1);
        curQuote = nullptr;
      }
      else {
        if (!curQuote)
        {
          
          if (!mEditor->CanContainTag(*curParent, *nsGkAtoms::div)) {
            return NS_OK; 
          }

          res = SplitAsNeeded(*nsGkAtoms::div, curParent, offset);
          NS_ENSURE_SUCCESS(res, res);
          NS_ENSURE_STATE(mHTMLEditor);
          curQuote = mHTMLEditor->CreateNode(nsGkAtoms::div, curParent,
                                             offset);
          NS_ENSURE_STATE(curQuote);
          RelativeChangeIndentationOfElementNode(curQuote->AsDOMNode(), +1);
          
          mNewBlock = curQuote->AsDOMNode();
          
        }
        
        
        uint32_t quoteLen = curQuote->Length();
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(curNode, curQuote, quoteLen);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
  return res;
}

nsresult
nsHTMLEditRules::WillHTMLIndent(Selection* aSelection,
                                bool* aCancel, bool* aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  nsresult res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);

  
  
  *aCancel = false;
  *aHandled = true;

  res = NormalizeSelection(aSelection);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_STATE(mHTMLEditor);
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  
  
  
  
  
  
  nsTArray<nsRefPtr<nsRange>> arrayOfRanges;
  GetPromotedRanges(*aSelection, arrayOfRanges, EditAction::indent);
  
  
  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  res = GetNodesForOperation(arrayOfRanges, arrayOfNodes, EditAction::indent);
  NS_ENSURE_SUCCESS(res, res);                                 
                                     
  
  if (ListIsEmptyLine(arrayOfNodes))
  {
    
    NS_ENSURE_STATE(aSelection->RangeCount());
    nsCOMPtr<nsINode> parent = aSelection->GetRangeAt(0)->GetStartParent();
    int32_t offset = aSelection->GetRangeAt(0)->StartOffset();
    NS_ENSURE_STATE(parent);

    
    res = SplitAsNeeded(*nsGkAtoms::blockquote, parent, offset);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<Element> theBlock = mHTMLEditor->CreateNode(nsGkAtoms::blockquote,
                                                         parent, offset);
    NS_ENSURE_STATE(theBlock);
    
    mNewBlock = theBlock->AsDOMNode();
    
    while (!arrayOfNodes.IsEmpty()) {
      OwningNonNull<nsINode> curNode = arrayOfNodes[0];
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(curNode);
      NS_ENSURE_SUCCESS(res, res);
      arrayOfNodes.RemoveElementAt(0);
    }
    
    res = aSelection->Collapse(theBlock,0);
    selectionResetter.Abort();  
    *aHandled = true;
    return res;
  }

  
  
  int32_t i;
  nsCOMPtr<nsINode> curParent;
  nsCOMPtr<nsIContent> sibling;
  nsCOMPtr<Element> curList, curQuote, indentedLI;
  int32_t listCount = arrayOfNodes.Length();
  for (i=0; i<listCount; i++)
  {
    
    NS_ENSURE_STATE(arrayOfNodes[i]->IsContent());
    nsCOMPtr<nsIContent> curNode = arrayOfNodes[i]->AsContent();

    
    NS_ENSURE_STATE(mHTMLEditor);
    if (!mHTMLEditor->IsEditable(curNode)) continue;

    curParent = curNode->GetParentNode();
    int32_t offset = curParent ? curParent->IndexOf(curNode) : -1;
     
    
    if (nsHTMLEditUtils::IsList(curParent))
    {
      sibling = nullptr;

      
      
      
      NS_ENSURE_STATE(mHTMLEditor);
      sibling = mHTMLEditor->GetNextHTMLSibling(curNode);
      if (sibling && nsHTMLEditUtils::IsList(sibling) &&
          curParent->NodeInfo()->NameAtom() == sibling->NodeInfo()->NameAtom() &&
          curParent->NodeInfo()->NamespaceID() == sibling->NodeInfo()->NamespaceID()) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(curNode, sibling, 0);
        NS_ENSURE_SUCCESS(res, res);
        continue;
      }

      
      
      
      NS_ENSURE_STATE(mHTMLEditor);
      sibling = mHTMLEditor->GetPriorHTMLSibling(curNode);
      if (sibling && nsHTMLEditUtils::IsList(sibling) &&
          curParent->NodeInfo()->NameAtom() == sibling->NodeInfo()->NameAtom() &&
          curParent->NodeInfo()->NamespaceID() == sibling->NodeInfo()->NamespaceID()) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(curNode, sibling, -1);
        NS_ENSURE_SUCCESS(res, res);
        continue;
      }

      sibling = nullptr;

      
      
      if (curList) {
        NS_ENSURE_STATE(mHTMLEditor);
        sibling = mHTMLEditor->GetPriorHTMLSibling(curNode);
      }

      if (!curList || (sibling && sibling != curList) )
      {
        
        res = SplitAsNeeded(*curParent->NodeInfo()->NameAtom(), curParent,
                            offset);
        NS_ENSURE_SUCCESS(res, res);
        NS_ENSURE_STATE(mHTMLEditor);
        curList = mHTMLEditor->CreateNode(curParent->NodeInfo()->NameAtom(),
                                          curParent, offset);
        NS_ENSURE_STATE(curList);
        
        
        mNewBlock = curList->AsDOMNode();
      }
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->MoveNode(curNode, curList, -1);
      NS_ENSURE_SUCCESS(res, res);
      
      curQuote = nullptr;
    }
    
    else 
    {
      
      
      
      
      
      
      nsCOMPtr<Element> listItem = IsInListItem(curNode);
      if (listItem) {
        if (indentedLI == listItem) {
          
          continue;
        }
        curParent = listItem->GetParentNode();
        offset = curParent ? curParent->IndexOf(listItem) : -1;
        
        
        if (curList)
        {
          sibling = nullptr;
          NS_ENSURE_STATE(mHTMLEditor);
          sibling = mHTMLEditor->GetPriorHTMLSibling(curNode);
        }
         
        if (!curList || (sibling && sibling != curList) )
        {
          
          res = SplitAsNeeded(*curParent->NodeInfo()->NameAtom(), curParent,
                              offset);
          NS_ENSURE_SUCCESS(res, res);
          NS_ENSURE_STATE(mHTMLEditor);
          curList = mHTMLEditor->CreateNode(curParent->NodeInfo()->NameAtom(),
                                            curParent, offset);
          NS_ENSURE_STATE(curList);
        }
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(listItem, curList, -1);
        NS_ENSURE_SUCCESS(res, res);
        
        indentedLI = listItem;
      }
      
      else
      {
        
        
        
        
        if (curQuote && InDifferentTableElements(curQuote, curNode)) {
          curQuote = nullptr;
        }
        
        if (!curQuote) 
        {
          
          if (!mEditor->CanContainTag(*curParent, *nsGkAtoms::blockquote)) {
            return NS_OK; 
          }

          res = SplitAsNeeded(*nsGkAtoms::blockquote, curParent, offset);
          NS_ENSURE_SUCCESS(res, res);
          NS_ENSURE_STATE(mHTMLEditor);
          curQuote = mHTMLEditor->CreateNode(nsGkAtoms::blockquote, curParent,
                                             offset);
          NS_ENSURE_STATE(curQuote);
          
          mNewBlock = curQuote->AsDOMNode();
          
        }
          
        
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(curNode, curQuote, -1);
        NS_ENSURE_SUCCESS(res, res);
        
        curList = nullptr;
      }
    }
  }
  return res;
}


nsresult
nsHTMLEditRules::WillOutdent(Selection* aSelection,
                             bool* aCancel, bool* aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  
  *aCancel = false;
  *aHandled = true;
  nsresult res = NS_OK;
  nsCOMPtr<nsIDOMNode> rememberedLeftBQ, rememberedRightBQ;
  NS_ENSURE_STATE(mHTMLEditor);
  bool useCSS = mHTMLEditor->IsCSSEnabled();

  res = NormalizeSelection(aSelection);
  NS_ENSURE_SUCCESS(res, res);
  
  {
    NS_ENSURE_STATE(mHTMLEditor);
    nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
    
    
    
    
    
    nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
    res = GetNodesFromSelection(*aSelection, EditAction::outdent,
                                arrayOfNodes);
    NS_ENSURE_SUCCESS(res, res);

    
    

    nsCOMPtr<nsIDOMNode> curBlockQuote, firstBQChild, lastBQChild;
    bool curBlockQuoteIsIndentedWithCSS = false;
    for (auto& curNode : arrayOfNodes) {
      
      nsCOMPtr<nsINode> curParent = curNode->GetParentNode();
      int32_t offset = curParent ? curParent->IndexOf(curNode) : -1;
      
      
      if (curNode->IsHTMLElement(nsGkAtoms::blockquote)) {
        
        
        if (curBlockQuote)
        {
          res = OutdentPartOfBlock(curBlockQuote, firstBQChild, lastBQChild,
                                   curBlockQuoteIsIndentedWithCSS,
                                   address_of(rememberedLeftBQ),
                                   address_of(rememberedRightBQ));
          NS_ENSURE_SUCCESS(res, res);
          curBlockQuote = 0;  firstBQChild = 0;  lastBQChild = 0;
          curBlockQuoteIsIndentedWithCSS = false;
        }
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->RemoveBlockContainer(GetAsDOMNode(curNode));
        NS_ENSURE_SUCCESS(res, res);
        continue;
      }
      
      if (useCSS && IsBlockNode(GetAsDOMNode(curNode))) {
        NS_ENSURE_STATE(mHTMLEditor);
        nsIAtom* marginProperty =
          MarginPropertyAtomForIndent(mHTMLEditor->mHTMLCSSUtils,
                                      GetAsDOMNode(curNode));
        nsAutoString value;
        NS_ENSURE_STATE(mHTMLEditor);
        mHTMLEditor->mHTMLCSSUtils->GetSpecifiedProperty(*curNode,
                                                         *marginProperty,
                                                         value);
        float f;
        nsCOMPtr<nsIAtom> unit;
        NS_ENSURE_STATE(mHTMLEditor);
        mHTMLEditor->mHTMLCSSUtils->ParseLength(value, &f, getter_AddRefs(unit));
        if (f > 0)
        {
          RelativeChangeIndentationOfElementNode(GetAsDOMNode(curNode), -1);
          continue;
        }
      }
      
      if (nsHTMLEditUtils::IsListItem(curNode)) 
      {
        
        
        
        if (curBlockQuote)
        {
          res = OutdentPartOfBlock(curBlockQuote, firstBQChild, lastBQChild,
                                   curBlockQuoteIsIndentedWithCSS,
                                   address_of(rememberedLeftBQ),
                                   address_of(rememberedRightBQ));
          NS_ENSURE_SUCCESS(res, res);
          curBlockQuote = 0;  firstBQChild = 0;  lastBQChild = 0;
          curBlockQuoteIsIndentedWithCSS = false;
        }
        bool bOutOfList;
        res = PopListItem(GetAsDOMNode(curNode), &bOutOfList);
        NS_ENSURE_SUCCESS(res, res);
        continue;
      }
      
      if (curBlockQuote)
      {
        
        if (nsEditorUtils::IsDescendantOf(GetAsDOMNode(curNode),
                                          curBlockQuote)) {
          lastBQChild = GetAsDOMNode(curNode);
          continue;  
        }
        else
        {
          
          
          
          res = OutdentPartOfBlock(curBlockQuote, firstBQChild, lastBQChild,
                                   curBlockQuoteIsIndentedWithCSS,
                                   address_of(rememberedLeftBQ),
                                   address_of(rememberedRightBQ));
          NS_ENSURE_SUCCESS(res, res);
          curBlockQuote = 0;  firstBQChild = 0;  lastBQChild = 0;
          curBlockQuoteIsIndentedWithCSS = false;
          
        }
      }
      
      
      nsCOMPtr<nsINode> n = curNode;
      curBlockQuoteIsIndentedWithCSS = false;
      
      
      while (!n->IsHTMLElement(nsGkAtoms::body) && mHTMLEditor &&
             mHTMLEditor->IsDescendantOfEditorRoot(n) &&
             (n->IsHTMLElement(nsGkAtoms::table) ||
              !nsHTMLEditUtils::IsTableElement(n))) {
        if (!n->GetParentNode()) {
          break;
        }
        n = n->GetParentNode();
        if (n->IsHTMLElement(nsGkAtoms::blockquote)) {
          
          curBlockQuote = GetAsDOMNode(n);
          firstBQChild  = GetAsDOMNode(curNode);
          lastBQChild   = GetAsDOMNode(curNode);
          break;
        }
        else if (useCSS)
        {
          NS_ENSURE_STATE(mHTMLEditor);
          nsIAtom* marginProperty =
            MarginPropertyAtomForIndent(mHTMLEditor->mHTMLCSSUtils,
                                        GetAsDOMNode(curNode));
          nsAutoString value;
          NS_ENSURE_STATE(mHTMLEditor);
          mHTMLEditor->mHTMLCSSUtils->GetSpecifiedProperty(*n, *marginProperty,
                                                           value);
          float f;
          nsCOMPtr<nsIAtom> unit;
          NS_ENSURE_STATE(mHTMLEditor);
          mHTMLEditor->mHTMLCSSUtils->ParseLength(value, &f, getter_AddRefs(unit));
          if (f > 0 && !(nsHTMLEditUtils::IsList(curParent) && nsHTMLEditUtils::IsList(curNode)))
          {
            curBlockQuote = GetAsDOMNode(n);
            firstBQChild  = GetAsDOMNode(curNode);
            lastBQChild   = GetAsDOMNode(curNode);
            curBlockQuoteIsIndentedWithCSS = true;
            break;
          }
        }
      }

      if (!curBlockQuote)
      {
        
        if (nsHTMLEditUtils::IsList(curParent))  
        {
          if (nsHTMLEditUtils::IsList(curNode))  
          {
            NS_ENSURE_STATE(mHTMLEditor);
            res = mHTMLEditor->RemoveBlockContainer(GetAsDOMNode(curNode));
            NS_ENSURE_SUCCESS(res, res);
          }
          
        }
        else if (nsHTMLEditUtils::IsList(curNode)) 
        {
          nsCOMPtr<nsIContent> child = curNode->GetLastChild();
          while (child)
          {
            if (nsHTMLEditUtils::IsListItem(child))
            {
              bool bOutOfList;
              res = PopListItem(GetAsDOMNode(child), &bOutOfList);
              NS_ENSURE_SUCCESS(res, res);
            }
            else if (nsHTMLEditUtils::IsList(child))
            {
              
              
              
              

              NS_ENSURE_STATE(mHTMLEditor);
              res = mHTMLEditor->MoveNode(child, curParent, offset + 1);
              NS_ENSURE_SUCCESS(res, res);
            }
            else
            {
              
              NS_ENSURE_STATE(mHTMLEditor);
              res = mHTMLEditor->DeleteNode(child);
              NS_ENSURE_SUCCESS(res, res);
            }
            child = curNode->GetLastChild();
          }
          
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->RemoveBlockContainer(GetAsDOMNode(curNode));
          NS_ENSURE_SUCCESS(res, res);
        }
        else if (useCSS) {
          nsCOMPtr<nsIDOMElement> element;
          nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(curNode);
          if (textNode) {
            
            nsCOMPtr<nsIDOMNode> parent;
            textNode->GetParentNode(getter_AddRefs(parent));
            element = do_QueryInterface(parent);
          } else {
            element = do_QueryInterface(curNode);
          }
          if (element) {
            RelativeChangeIndentationOfElementNode(element, -1);
          }
        }
      }
    }
    if (curBlockQuote)
    {
      
      res = OutdentPartOfBlock(curBlockQuote, firstBQChild, lastBQChild,
                               curBlockQuoteIsIndentedWithCSS,
                               address_of(rememberedLeftBQ),
                               address_of(rememberedRightBQ));
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  
  
  if (rememberedLeftBQ || rememberedRightBQ) {
    if (aSelection->Collapsed()) {
      
      nsCOMPtr<nsIDOMNode> sNode;
      int32_t sOffset;
      NS_ENSURE_STATE(mHTMLEditor);
      mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(sNode), &sOffset);
      if (rememberedLeftBQ &&
          ((sNode == rememberedLeftBQ) || nsEditorUtils::IsDescendantOf(sNode, rememberedLeftBQ)))
      {
        
        sNode = nsEditor::GetNodeLocation(rememberedLeftBQ, &sOffset);
        sOffset++;
        aSelection->Collapse(sNode, sOffset);
      }
      
      NS_ENSURE_STATE(mHTMLEditor);
      mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(sNode), &sOffset);
      if (rememberedRightBQ &&
          ((sNode == rememberedRightBQ) || nsEditorUtils::IsDescendantOf(sNode, rememberedRightBQ)))
      {
        
        sNode = nsEditor::GetNodeLocation(rememberedRightBQ, &sOffset);
        aSelection->Collapse(sNode, sOffset);
      }
    }
    return NS_OK;
  }
  return res;
}





nsresult
nsHTMLEditRules::RemovePartOfBlock(Element& aBlock,
                                   nsIContent& aStartChild,
                                   nsIContent& aEndChild)
{
  nsresult res = SplitBlock(aBlock.AsDOMNode(), aStartChild.AsDOMNode(),
                            aEndChild.AsDOMNode());
  NS_ENSURE_SUCCESS(res, res);
  

  NS_ENSURE_STATE(mHTMLEditor);
  return mHTMLEditor->RemoveBlockContainer(aBlock.AsDOMNode());
}

nsresult 
nsHTMLEditRules::SplitBlock(nsIDOMNode *aBlock, 
                            nsIDOMNode *aStartChild, 
                            nsIDOMNode *aEndChild,
                            nsCOMPtr<nsIDOMNode> *aLeftNode,
                            nsCOMPtr<nsIDOMNode> *aRightNode,
                            nsCOMPtr<nsIDOMNode> *aMiddleNode)
{
  NS_ENSURE_TRUE(aBlock && aStartChild && aEndChild, NS_ERROR_NULL_POINTER);
  
  nsCOMPtr<nsIDOMNode> leftNode, rightNode;
  int32_t startOffset, endOffset, offset;
  nsresult res;

  
  nsCOMPtr<nsIDOMNode> startParent = nsEditor::GetNodeLocation(aStartChild, &startOffset);
  
  
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->SplitNodeDeep(aBlock, startParent, startOffset, &offset, 
                                   true, address_of(leftNode), address_of(rightNode));
  NS_ENSURE_SUCCESS(res, res);
  if (rightNode)  aBlock = rightNode;

  
  if (aLeftNode) 
    *aLeftNode = leftNode;

  
  nsCOMPtr<nsIDOMNode> endParent = nsEditor::GetNodeLocation(aEndChild, &endOffset);
  endOffset++;  

  
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->SplitNodeDeep(aBlock, endParent, endOffset, &offset, 
                                   true, address_of(leftNode), address_of(rightNode));
  NS_ENSURE_SUCCESS(res, res);
  if (leftNode)  aBlock = leftNode;
  
  
  if (aRightNode) 
    *aRightNode = rightNode;

  if (aMiddleNode)
    *aMiddleNode = aBlock;

  return NS_OK;
}

nsresult
nsHTMLEditRules::OutdentPartOfBlock(nsIDOMNode *aBlock, 
                                    nsIDOMNode *aStartChild, 
                                    nsIDOMNode *aEndChild,
                                    bool aIsBlockIndentedWithCSS,
                                    nsCOMPtr<nsIDOMNode> *aLeftNode,
                                    nsCOMPtr<nsIDOMNode> *aRightNode)
{
  nsCOMPtr<nsIDOMNode> middleNode;
  nsresult res = SplitBlock(aBlock, aStartChild, aEndChild, 
                            aLeftNode,
                            aRightNode,
                            address_of(middleNode));
  NS_ENSURE_SUCCESS(res, res);
  if (aIsBlockIndentedWithCSS) {
    res = RelativeChangeIndentationOfElementNode(middleNode, -1);
  } else {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->RemoveBlockContainer(middleNode);
  }
  return res;
}





nsresult
nsHTMLEditRules::ConvertListType(nsIDOMNode* aList,
                                 nsCOMPtr<nsIDOMNode>* outList,
                                 nsIAtom* aListType,
                                 nsIAtom* aItemType)
{
  MOZ_ASSERT(aListType);
  MOZ_ASSERT(aItemType);

  NS_ENSURE_TRUE(aList && outList, NS_ERROR_NULL_POINTER);
  nsCOMPtr<Element> list = do_QueryInterface(aList);
  NS_ENSURE_STATE(list);

  nsCOMPtr<dom::Element> outNode;
  nsresult rv = ConvertListType(list, getter_AddRefs(outNode), aListType, aItemType);
  *outList = outNode ? outNode->AsDOMNode() : nullptr;
  return rv;
}

nsresult
nsHTMLEditRules::ConvertListType(Element* aList,
                                 dom::Element** aOutList,
                                 nsIAtom* aListType,
                                 nsIAtom* aItemType)
{
  MOZ_ASSERT(aList);
  MOZ_ASSERT(aOutList);
  MOZ_ASSERT(aListType);
  MOZ_ASSERT(aItemType);

  nsCOMPtr<nsINode> child = aList->GetFirstChild();
  while (child)
  {
    if (child->IsElement()) {
      dom::Element* element = child->AsElement();
      if (nsHTMLEditUtils::IsListItem(element) &&
          !element->IsHTMLElement(aItemType)) {
        child = mHTMLEditor->ReplaceContainer(element, aItemType);
        NS_ENSURE_STATE(child);
      } else if (nsHTMLEditUtils::IsList(element) &&
                 !element->IsHTMLElement(aListType)) {
        nsCOMPtr<dom::Element> temp;
        nsresult rv = ConvertListType(child->AsElement(), getter_AddRefs(temp),
                                      aListType, aItemType);
        NS_ENSURE_SUCCESS(rv, rv);
        child = temp.forget();
      }
    }
    child = child->GetNextSibling();
  }

  if (aList->IsHTMLElement(aListType)) {
    nsCOMPtr<dom::Element> list = aList->AsElement();
    list.forget(aOutList);
    return NS_OK;
  }

  *aOutList = mHTMLEditor->ReplaceContainer(aList, aListType).take();
  NS_ENSURE_STATE(aOutList);

  return NS_OK;
}







nsresult
nsHTMLEditRules::CreateStyleForInsertText(Selection* aSelection,
                                          nsIDOMDocument *aDoc)
{
  MOZ_ASSERT(aSelection && aDoc && mHTMLEditor->mTypeInState);

  bool weDidSomething = false;
  nsCOMPtr<nsIDOMNode> node, tmp;
  int32_t offset;
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection,
                                                    getter_AddRefs(node),
                                                    &offset);
  NS_ENSURE_SUCCESS(res, res);

  
  
  
  int32_t length = mHTMLEditor->mDefaultStyles.Length();
  for (int32_t j = 0; j < length; j++) {
    PropItem* propItem = mHTMLEditor->mDefaultStyles[j];
    MOZ_ASSERT(propItem);
    bool bFirst, bAny, bAll;

    
    
    
    
    
    
    
    
    nsAutoString curValue;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->GetInlinePropertyBase(*propItem->tag, &propItem->attr,
                                             nullptr, &bFirst, &bAny, &bAll,
                                             &curValue, false);
    NS_ENSURE_SUCCESS(res, res);

    if (!bAny) {
      
      mHTMLEditor->mTypeInState->SetProp(propItem->tag, propItem->attr,
                                         propItem->value);
    }
  }

  nsCOMPtr<nsIDOMElement> rootElement;
  res = aDoc->GetDocumentElement(getter_AddRefs(rootElement));
  NS_ENSURE_SUCCESS(res, res);

  
  nsAutoPtr<PropItem> item(mHTMLEditor->mTypeInState->TakeClearProperty());
  while (item && node != rootElement) {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->ClearStyle(address_of(node), &offset,
                                  item->tag, &item->attr);
    NS_ENSURE_SUCCESS(res, res);
    item = mHTMLEditor->mTypeInState->TakeClearProperty();
    weDidSomething = true;
  }

  
  int32_t relFontSize = mHTMLEditor->mTypeInState->TakeRelativeFontSize();
  item = mHTMLEditor->mTypeInState->TakeSetProperty();

  if (item || relFontSize) {
    
    
    if (mHTMLEditor->IsTextNode(node)) {
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->SplitNodeDeep(node, node, offset, &offset);
      NS_ENSURE_SUCCESS(res, res);
      node->GetParentNode(getter_AddRefs(tmp));
      node = tmp;
    }
    if (!mHTMLEditor->IsContainer(node)) {
      return NS_OK;
    }
    nsCOMPtr<nsIDOMNode> newNode;
    nsCOMPtr<nsIDOMText> nodeAsText;
    res = aDoc->CreateTextNode(EmptyString(), getter_AddRefs(nodeAsText));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(nodeAsText, NS_ERROR_NULL_POINTER);
    newNode = do_QueryInterface(nodeAsText);
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->InsertNode(newNode, node, offset);
    NS_ENSURE_SUCCESS(res, res);
    node = newNode;
    offset = 0;
    weDidSomething = true;

    if (relFontSize) {
      
      int32_t dir = relFontSize > 0 ? 1 : -1;
      for (int32_t j = 0; j < DeprecatedAbs(relFontSize); j++) {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->RelativeFontChangeOnTextNode(dir, nodeAsText,
                                                        0, -1);
        NS_ENSURE_SUCCESS(res, res);
      }
    }

    while (item) {
      NS_ENSURE_STATE(mHTMLEditor);
      nsCOMPtr<nsIContent> content = do_QueryInterface(node);
      NS_ENSURE_STATE(content || !node);
      res = mHTMLEditor->SetInlinePropertyOnNode(*content, *item->tag,
                                                 &item->attr, item->value);
      NS_ENSURE_SUCCESS(res, res);
      item = mHTMLEditor->mTypeInState->TakeSetProperty();
    }
  }
  if (weDidSomething) {
    return aSelection->Collapse(node, offset);
  }

  return NS_OK;
}







nsresult 
nsHTMLEditRules::IsEmptyBlock(nsIDOMNode *aNode, 
                              bool *outIsEmptyBlock, 
                              bool aMozBRDoesntCount,
                              bool aListItemsNotEmpty) 
{
  NS_ENSURE_TRUE(aNode && outIsEmptyBlock, NS_ERROR_NULL_POINTER);
  *outIsEmptyBlock = true;
  

  nsCOMPtr<nsIDOMNode> nodeToTest;
  if (IsBlockNode(aNode)) nodeToTest = do_QueryInterface(aNode);



  NS_ENSURE_TRUE(nodeToTest, NS_ERROR_NULL_POINTER);
  return mHTMLEditor->IsEmptyNode(nodeToTest, outIsEmptyBlock,
                     aMozBRDoesntCount, aListItemsNotEmpty);
}


nsresult
nsHTMLEditRules::WillAlign(Selection* aSelection,
                           const nsAString *alignType, 
                           bool *aCancel,
                           bool *aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }

  nsresult res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);

  
  
  *aCancel = false;
  *aHandled = false;

  res = NormalizeSelection(aSelection);
  NS_ENSURE_SUCCESS(res, res);
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);

  
  
  
  
  *aHandled = true;
  nsTArray<OwningNonNull<nsINode>> nodeArray;
  res = GetNodesFromSelection(*aSelection, EditAction::align, nodeArray);
  NS_ENSURE_SUCCESS(res, res);

  
  
  bool emptyDiv = false;
  int32_t listCount = nodeArray.Length();
  if (!listCount) emptyDiv = true;
  if (listCount == 1)
  {
    OwningNonNull<nsINode> theNode = nodeArray[0];

    if (nsHTMLEditUtils::SupportsAlignAttr(GetAsDOMNode(theNode))) {
      
      
      
      
      nsCOMPtr<nsIDOMElement> theElem = do_QueryInterface(theNode);
      res = AlignBlock(theElem, alignType, true);
      NS_ENSURE_SUCCESS(res, res);
      return NS_OK;
    }

    if (nsTextEditUtils::IsBreak(theNode))
    {
      
      
      
      
      
      
      
      
      
      
      
      

      nsCOMPtr<nsIDOMNode> parent;
      int32_t offset;
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(parent), &offset);

      if (!nsHTMLEditUtils::IsTableElement(parent) || nsHTMLEditUtils::IsTableCellOrCaption(parent))
        emptyDiv = true;
    }
  }
  if (emptyDiv)
  {
    nsCOMPtr<nsIDOMNode> brNode, sib;
    NS_NAMED_LITERAL_STRING(divType, "div");

    NS_ENSURE_STATE(aSelection->GetRangeAt(0));
    nsCOMPtr<nsINode> parent = aSelection->GetRangeAt(0)->GetStartParent();
    int32_t offset = aSelection->GetRangeAt(0)->StartOffset();
    NS_ENSURE_STATE(parent);

    res = SplitAsNeeded(*nsGkAtoms::div, parent, offset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->GetNextHTMLNode(parent->AsDOMNode(), offset,
                                       address_of(brNode));
    NS_ENSURE_SUCCESS(res, res);
    if (brNode && nsTextEditUtils::IsBreak(brNode))
    {
      
      
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetNextHTMLSibling(parent->AsDOMNode(), offset,
                                            address_of(sib));
      NS_ENSURE_SUCCESS(res, res);
      if (!IsBlockNode(sib))
      {
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->DeleteNode(brNode);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<Element> theDiv = mHTMLEditor->CreateNode(nsGkAtoms::div, parent,
                                                       offset);
    NS_ENSURE_STATE(theDiv);
    
    mNewBlock = theDiv->AsDOMNode();
    
    nsCOMPtr<nsIDOMElement> divElem = do_QueryInterface(theDiv);
    res = AlignBlock(divElem, alignType, true);
    NS_ENSURE_SUCCESS(res, res);
    *aHandled = true;
    
    res = CreateMozBR(theDiv->AsDOMNode(), 0);
    NS_ENSURE_SUCCESS(res, res);
    res = aSelection->Collapse(theDiv, 0);
    selectionResetter.Abort();  
    return res;
  }

  
  

  nsTArray<bool> transitionList;
  MakeTransitionList(nodeArray, transitionList);

  
  

  nsCOMPtr<nsINode> curParent;
  nsCOMPtr<Element> curDiv;
  bool useCSS = mHTMLEditor->IsCSSEnabled();
  for (int32_t i = 0; i < listCount; ++i) {
    
    nsCOMPtr<nsIDOMNode> curNode = nodeArray[i]->AsDOMNode();
    nsCOMPtr<nsIContent> curContent = do_QueryInterface(curNode);
    NS_ENSURE_STATE(curContent);

    
    if (!mHTMLEditor->IsEditable(curNode)) continue;

    curParent = curContent->GetParentNode();
    int32_t offset = curParent ? curParent->IndexOf(curContent) : -1;

    
    
    
    
    if (nsHTMLEditUtils::SupportsAlignAttr(curNode))
    {
      nsCOMPtr<nsIDOMElement> curElem = do_QueryInterface(curNode);
      res = AlignBlock(curElem, alignType, false);
      NS_ENSURE_SUCCESS(res, res);
      
      curDiv = 0;
      continue;
    }

    
    
    bool isEmptyTextNode = false;
    if (nsEditor::IsTextNode(curNode) &&
       ((nsHTMLEditUtils::IsTableElement(curParent) &&
         !nsHTMLEditUtils::IsTableCellOrCaption(GetAsDOMNode(curParent))) ||
        nsHTMLEditUtils::IsList(curParent) ||
        (NS_SUCCEEDED(mHTMLEditor->IsEmptyNode(curNode, &isEmptyTextNode)) && isEmptyTextNode)))
      continue;

    
    
    
    if ( nsHTMLEditUtils::IsListItem(curNode)
         || nsHTMLEditUtils::IsList(curNode))
    {
      res = RemoveAlignment(curNode, *alignType, true);
      NS_ENSURE_SUCCESS(res, res);
      if (useCSS) {
        nsCOMPtr<nsIDOMElement> curElem = do_QueryInterface(curNode);
        NS_NAMED_LITERAL_STRING(attrName, "align");
        int32_t count;
        mHTMLEditor->mHTMLCSSUtils->SetCSSEquivalentToHTMLStyle(curNode, nullptr,
                                                                &attrName, alignType,
                                                                &count, false);
        curDiv = 0;
        continue;
      }
      else if (nsHTMLEditUtils::IsList(curParent)) {
        
        
        res = AlignInnerBlocks(*curContent, alignType);
        NS_ENSURE_SUCCESS(res, res);
        curDiv = 0;
        continue;
      }
      
    }      

    
    
    if (!curDiv || transitionList[i])
    {
      
      if (!mEditor->CanContainTag(*curParent, *nsGkAtoms::div)) {
        return NS_OK; 
      }

      res = SplitAsNeeded(*nsGkAtoms::div, curParent, offset);
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_STATE(mHTMLEditor);
      curDiv = mHTMLEditor->CreateNode(nsGkAtoms::div, curParent, offset);
      NS_ENSURE_STATE(curDiv);
      
      mNewBlock = curDiv->AsDOMNode();
      
      nsCOMPtr<nsIDOMElement> divElem = do_QueryInterface(curDiv);
      res = AlignBlock(divElem, alignType, true);
      
      
      
      
    }

    
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->MoveNode(curContent, curDiv, -1);
    NS_ENSURE_SUCCESS(res, res);
  }

  return res;
}





nsresult
nsHTMLEditRules::AlignInnerBlocks(nsINode& aNode, const nsAString* alignType)
{
  NS_ENSURE_TRUE(alignType, NS_ERROR_NULL_POINTER);

  
  nsTArray<OwningNonNull<nsINode>> nodeArray;
  nsTableCellAndListItemFunctor functor;
  nsDOMIterator iter(aNode);
  iter.AppendList(functor, nodeArray);

  
  for (auto& node : nodeArray) {
    nsresult res = AlignBlockContents(node->AsDOMNode(), alignType);
    NS_ENSURE_SUCCESS(res, res);
  }

  return NS_OK;
}





nsresult
nsHTMLEditRules::AlignBlockContents(nsIDOMNode *aNode, const nsAString *alignType)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node && alignType, NS_ERROR_NULL_POINTER);
  nsresult res = NS_OK;
  nsCOMPtr<nsIContent> firstChild, lastChild;
  nsCOMPtr<Element> divNode;
  
  bool useCSS = mHTMLEditor->IsCSSEnabled();

  NS_ENSURE_STATE(mHTMLEditor);
  firstChild = mHTMLEditor->GetFirstEditableChild(*node);
  NS_ENSURE_STATE(mHTMLEditor);
  lastChild = mHTMLEditor->GetLastEditableChild(*node);
  NS_NAMED_LITERAL_STRING(attr, "align");
  if (!firstChild)
  {
    
  } else if (firstChild == lastChild &&
             firstChild->IsHTMLElement(nsGkAtoms::div)) {
    
    
    nsCOMPtr<nsIDOMElement> divElem = do_QueryInterface(firstChild);
    if (useCSS) {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->SetAttributeOrEquivalent(divElem, attr, *alignType, false); 
    }
    else {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->SetAttribute(divElem, attr, *alignType);
    }
    NS_ENSURE_SUCCESS(res, res);
  }
  else
  {
    
    NS_ENSURE_STATE(mHTMLEditor);
    divNode = mHTMLEditor->CreateNode(nsGkAtoms::div, node, 0);
    NS_ENSURE_STATE(divNode);
    
    nsCOMPtr<nsIDOMElement> divElem = do_QueryInterface(divNode);
    if (useCSS) {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->SetAttributeOrEquivalent(divElem, attr, *alignType, false); 
    }
    else {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->SetAttribute(divElem, attr, *alignType);
    }
    NS_ENSURE_SUCCESS(res, res);
    
    while (lastChild && (lastChild != divNode))
    {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->MoveNode(lastChild, divNode, 0);
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_STATE(mHTMLEditor);
      lastChild = mHTMLEditor->GetLastEditableChild(*node);
    }
  }
  return res;
}





nsresult
nsHTMLEditRules::CheckForEmptyBlock(nsINode* aStartNode,
                                    Element* aBodyNode,
                                    Selection* aSelection,
                                    bool* aHandled)
{
  
  if (IsInlineNode(GetAsDOMNode(aBodyNode))) {
    return NS_OK;
  }
  
  
  nsCOMPtr<Element> block;
  if (IsBlockNode(GetAsDOMNode(aStartNode))) {
    block = aStartNode->AsElement();
  } else {
    block = mHTMLEditor->GetBlockNodeParent(aStartNode);
  }
  bool bIsEmptyNode;
  nsCOMPtr<Element> emptyBlock;
  nsresult res;
  if (block && block != aBodyNode) {
    
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->IsEmptyNode(block, &bIsEmptyNode, true, false);
    NS_ENSURE_SUCCESS(res, res);
    while (block && bIsEmptyNode && !nsHTMLEditUtils::IsTableElement(block) &&
           block != aBodyNode) {
      emptyBlock = block;
      block = mHTMLEditor->GetBlockNodeParent(emptyBlock);
      NS_ENSURE_STATE(mHTMLEditor);
      if (block) {
        res = mHTMLEditor->IsEmptyNode(block, &bIsEmptyNode, true, false);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }

  if (emptyBlock && emptyBlock->IsEditable()) {
    nsCOMPtr<nsINode> blockParent = emptyBlock->GetParentNode();
    NS_ENSURE_TRUE(blockParent, NS_ERROR_FAILURE);
    int32_t offset = blockParent->IndexOf(emptyBlock);

    if (nsHTMLEditUtils::IsListItem(emptyBlock)) {
      
      bool bIsFirst;
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->IsFirstEditableChild(GetAsDOMNode(emptyBlock),
                                              &bIsFirst);
      NS_ENSURE_SUCCESS(res, res);
      if (bIsFirst) {
        nsCOMPtr<nsINode> listParent = blockParent->GetParentNode();
        NS_ENSURE_TRUE(listParent, NS_ERROR_FAILURE);
        int32_t listOffset = listParent->IndexOf(blockParent);
        
        if (!nsHTMLEditUtils::IsList(listParent)) {
          
          NS_ENSURE_STATE(mHTMLEditor);
          nsCOMPtr<Element> br =
            mHTMLEditor->CreateBR(listParent, listOffset);
          NS_ENSURE_STATE(br);
          
          res = aSelection->Collapse(listParent, listOffset);
          NS_ENSURE_SUCCESS(res, res);
        }
        
        
      }
    } else {
      
      res = aSelection->Collapse(blockParent, offset + 1);
      NS_ENSURE_SUCCESS(res, res);
    }
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->DeleteNode(emptyBlock);
    *aHandled = true;
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}

nsresult
nsHTMLEditRules::CheckForInvisibleBR(nsIDOMNode *aBlock, 
                                     BRLocation aWhere, 
                                     nsCOMPtr<nsIDOMNode> *outBRNode,
                                     int32_t aOffset)
{
  nsCOMPtr<nsINode> block = do_QueryInterface(aBlock);
  NS_ENSURE_TRUE(block && outBRNode, NS_ERROR_NULL_POINTER);
  *outBRNode = nullptr;

  nsCOMPtr<nsIDOMNode> testNode;
  int32_t testOffset = 0;
  bool runTest = false;

  if (aWhere == kBlockEnd)
  {
    nsCOMPtr<nsIDOMNode> rightmostNode =
      
      GetAsDOMNode(mHTMLEditor->GetRightmostChild(block, true));

    if (rightmostNode)
    {
      int32_t nodeOffset;
      nsCOMPtr<nsIDOMNode> nodeParent = nsEditor::GetNodeLocation(rightmostNode,
                                                                  &nodeOffset);
      runTest = true;
      testNode = nodeParent;
      
      
      testOffset = nodeOffset + 1;
    }
  }
  else if (aOffset)
  {
    runTest = true;
    testNode = aBlock;
    
    testOffset = aOffset;
  }

  if (runTest)
  {
    nsWSRunObject wsTester(mHTMLEditor, testNode, testOffset);
    if (WSType::br == wsTester.mStartReason) {
      *outBRNode = GetAsDOMNode(wsTester.mStartReasonNode);
    }
  }

  return NS_OK;
}










void
nsHTMLEditRules::GetInnerContent(nsINode& aNode,
                                 nsTArray<OwningNonNull<nsINode>>& aOutArrayOfNodes,
                                 int32_t* aIndex, Lists aLists, Tables aTables)
{
  MOZ_ASSERT(aIndex);

  for (nsCOMPtr<nsIContent> node = mHTMLEditor->GetFirstEditableChild(aNode);
       node; node = node->GetNextSibling()) {
    if ((aLists == Lists::yes && (nsHTMLEditUtils::IsList(node) ||
                                  nsHTMLEditUtils::IsListItem(node))) ||
        (aTables == Tables::yes && nsHTMLEditUtils::IsTableElement(node))) {
      GetInnerContent(*node, aOutArrayOfNodes, aIndex, aLists, aTables);
    } else {
      aOutArrayOfNodes.InsertElementAt(*aIndex, *node);
      (*aIndex)++;
    }
  }
}





nsresult
nsHTMLEditRules::ExpandSelectionForDeletion(Selection* aSelection)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  
  
  if (aSelection->Collapsed()) {
    return NS_OK;
  }

  int32_t rangeCount;
  nsresult res = aSelection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if (rangeCount != 1) return NS_OK;
  
  
  nsRefPtr<nsRange> range = aSelection->GetRangeAt(0);
  NS_ENSURE_TRUE(range, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> selStartNode, selEndNode, selCommon;
  int32_t selStartOffset, selEndOffset;
  
  res = range->GetStartContainer(getter_AddRefs(selStartNode));
  NS_ENSURE_SUCCESS(res, res);
  res = range->GetStartOffset(&selStartOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = range->GetEndContainer(getter_AddRefs(selEndNode));
  NS_ENSURE_SUCCESS(res, res);
  res = range->GetEndOffset(&selEndOffset);
  NS_ENSURE_SUCCESS(res, res);

  
  res = range->GetCommonAncestorContainer(getter_AddRefs(selCommon));
  NS_ENSURE_SUCCESS(res, res);
  if (!IsBlockNode(selCommon))
    selCommon = nsHTMLEditor::GetBlockNodeParent(selCommon);
  NS_ENSURE_STATE(selCommon);

  
  bool stillLooking = true;
  nsCOMPtr<nsIDOMNode> firstBRParent;
  nsCOMPtr<nsINode> unused;
  int32_t visOffset=0, firstBROffset=0;
  WSType wsType;
  nsCOMPtr<nsIContent> rootContent = mHTMLEditor->GetActiveEditingHost();
  nsCOMPtr<nsIDOMNode> rootElement = do_QueryInterface(rootContent);
  NS_ENSURE_TRUE(rootElement, NS_ERROR_FAILURE);

  
  if ((selStartNode!=selCommon) && (selStartNode!=rootElement))
  {
    while (stillLooking)
    {
      nsWSRunObject wsObj(mHTMLEditor, selStartNode, selStartOffset);
      nsCOMPtr<nsINode> selStartNode_(do_QueryInterface(selStartNode));
      wsObj.PriorVisibleNode(selStartNode_, selStartOffset, address_of(unused),
                             &visOffset, &wsType);
      if (wsType == WSType::thisBlock) {
        
        
        if (nsHTMLEditUtils::IsTableElement(wsObj.mStartReasonNode) ||
            selCommon == GetAsDOMNode(wsObj.mStartReasonNode) ||
            rootElement == GetAsDOMNode(wsObj.mStartReasonNode)) {
          stillLooking = false;
        }
        else
        { 
          selStartNode = nsEditor::GetNodeLocation(GetAsDOMNode(wsObj.mStartReasonNode),
                                                   &selStartOffset);
        }
      }
      else
      {
        stillLooking = false;
      }
    }
  }
  
  stillLooking = true;
  
  if ((selEndNode!=selCommon) && (selEndNode!=rootElement))
  {
    while (stillLooking)
    {
      nsWSRunObject wsObj(mHTMLEditor, selEndNode, selEndOffset);
      nsCOMPtr<nsINode> selEndNode_(do_QueryInterface(selEndNode));
      wsObj.NextVisibleNode(selEndNode_, selEndOffset, address_of(unused),
                            &visOffset, &wsType);
      if (wsType == WSType::br) {
        if (mHTMLEditor->IsVisBreak(wsObj.mEndReasonNode))
        {
          stillLooking = false;
        }
        else
        { 
          if (!firstBRParent)
          {
            firstBRParent = selEndNode;
            firstBROffset = selEndOffset;
          }
          selEndNode = nsEditor::GetNodeLocation(GetAsDOMNode(wsObj.mEndReasonNode), &selEndOffset);
          ++selEndOffset;
        }
      } else if (wsType == WSType::thisBlock) {
        
        
        if (nsHTMLEditUtils::IsTableElement(wsObj.mEndReasonNode) ||
            selCommon == GetAsDOMNode(wsObj.mEndReasonNode) ||
            rootElement == GetAsDOMNode(wsObj.mEndReasonNode)) {
          stillLooking = false;
        }
        else
        { 
          selEndNode = nsEditor::GetNodeLocation(GetAsDOMNode(wsObj.mEndReasonNode), &selEndOffset);
          ++selEndOffset;
        }
       }
      else
      {
        stillLooking = false;
      }
    }
  }
  
  aSelection->Collapse(selStartNode, selStartOffset);
  
  
  
  
  bool doEndExpansion = true;
  if (firstBRParent)
  {
    
    nsCOMPtr<nsIDOMNode> brBlock = firstBRParent;
    if (!IsBlockNode(brBlock))
      brBlock = nsHTMLEditor::GetBlockNodeParent(brBlock);
    bool nodeBefore=false, nodeAfter=false;
    
    
    nsCOMPtr<nsINode> node = do_QueryInterface(selStartNode);
    NS_ENSURE_STATE(node);
    nsRefPtr<nsRange> range = new nsRange(node);
    res = range->SetStart(selStartNode, selStartOffset);
    NS_ENSURE_SUCCESS(res, res);
    res = range->SetEnd(selEndNode, selEndOffset);
    NS_ENSURE_SUCCESS(res, res);
    
    
    nsCOMPtr<nsIContent> brContentBlock = do_QueryInterface(brBlock);
    if (brContentBlock) {
      res = nsRange::CompareNodeToRange(brContentBlock, range, &nodeBefore,
                                        &nodeAfter);
    }
    
    
    if (nodeBefore || nodeAfter)
      doEndExpansion = false;
  }
  if (doEndExpansion)
  {
    res = aSelection->Extend(selEndNode, selEndOffset);
  }
  else
  {
    
    res = aSelection->Extend(firstBRParent, firstBROffset);
  }
  
  return res;
}









nsresult 
nsHTMLEditRules::NormalizeSelection(Selection* inSelection)
{
  NS_ENSURE_TRUE(inSelection, NS_ERROR_NULL_POINTER);

  
  if (inSelection->Collapsed()) {
    return NS_OK;
  }

  int32_t rangeCount;
  nsresult res = inSelection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(res, res);
  
  
  if (rangeCount != 1) return NS_OK;
  
  nsRefPtr<nsRange> range = inSelection->GetRangeAt(0);
  NS_ENSURE_TRUE(range, NS_ERROR_NULL_POINTER);
  nsCOMPtr<nsIDOMNode> startNode, endNode;
  int32_t startOffset, endOffset;
  nsCOMPtr<nsIDOMNode> newStartNode, newEndNode;
  int32_t newStartOffset, newEndOffset;
  
  res = range->GetStartContainer(getter_AddRefs(startNode));
  NS_ENSURE_SUCCESS(res, res);
  res = range->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = range->GetEndContainer(getter_AddRefs(endNode));
  NS_ENSURE_SUCCESS(res, res);
  res = range->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  
  newStartNode = startNode; 
  newStartOffset = startOffset;
  newEndNode = endNode; 
  newEndOffset = endOffset;
  
  
  nsCOMPtr<nsINode> unused;
  int32_t offset;
  WSType wsType;

  
  nsWSRunObject wsEndObj(mHTMLEditor, endNode, endOffset);
  
  
  nsCOMPtr<nsINode> endNode_(do_QueryInterface(endNode));
  wsEndObj.PriorVisibleNode(endNode_, endOffset, address_of(unused),
                            &offset, &wsType);
  if (wsType != WSType::text && wsType != WSType::normalWS) {
    
    
    if (wsEndObj.mStartReason == WSType::otherBlock) {
      
      nsCOMPtr<nsIDOMNode> child =
        GetAsDOMNode(mHTMLEditor->GetRightmostChild(wsEndObj.mStartReasonNode,
                                                    true));
      if (child)
      {
        newEndNode = nsEditor::GetNodeLocation(child, &newEndOffset);
        ++newEndOffset; 
      }
      
    } else if (wsEndObj.mStartReason == WSType::thisBlock) {
      
      nsCOMPtr<nsIDOMNode> child;
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetPriorHTMLNode(endNode, endOffset, address_of(child));
      if (child)
      {
        newEndNode = nsEditor::GetNodeLocation(child, &newEndOffset);
        ++newEndOffset; 
      }
      
    } else if (wsEndObj.mStartReason == WSType::br) {
      
      newEndNode = nsEditor::GetNodeLocation(GetAsDOMNode(wsEndObj.mStartReasonNode),
                                             &newEndOffset);
    }
  }
  
  
  
  nsWSRunObject wsStartObj(mHTMLEditor, startNode, startOffset);
  
  
  nsCOMPtr<nsINode> startNode_(do_QueryInterface(startNode));
  wsStartObj.NextVisibleNode(startNode_, startOffset, address_of(unused),
                             &offset, &wsType);
  if (wsType != WSType::text && wsType != WSType::normalWS) {
    
    
    if (wsStartObj.mEndReason == WSType::otherBlock) {
      
      nsCOMPtr<nsIDOMNode> child =
        GetAsDOMNode(mHTMLEditor->GetLeftmostChild(wsStartObj.mEndReasonNode,
                                                   true));
      if (child)
      {
        newStartNode = nsEditor::GetNodeLocation(child, &newStartOffset);
      }
      
    } else if (wsStartObj.mEndReason == WSType::thisBlock) {
      
      nsCOMPtr<nsIDOMNode> child;
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetNextHTMLNode(startNode, startOffset, address_of(child));
      if (child)
      {
        newStartNode = nsEditor::GetNodeLocation(child, &newStartOffset);
      }
      
    } else if (wsStartObj.mEndReason == WSType::br) {
      
      newStartNode = nsEditor::GetNodeLocation(GetAsDOMNode(wsStartObj.mEndReasonNode),
                                               &newStartOffset);
      ++newStartOffset; 
    }
  }
  
  
  
  
  
  
  
  
  
  int16_t comp;
  comp = nsContentUtils::ComparePoints(startNode, startOffset,
                                       newEndNode, newEndOffset);
  if (comp == 1) return NS_OK;  
  comp = nsContentUtils::ComparePoints(newStartNode, newStartOffset,
                                       endNode, endOffset);
  if (comp == 1) return NS_OK;  
  
  
  inSelection->Collapse(newStartNode, newStartOffset);
  inSelection->Extend(newEndNode, newEndOffset);
  return NS_OK;
}





void
nsHTMLEditRules::GetPromotedPoint(RulesEndpoint aWhere, nsIDOMNode* aNode,
                                  int32_t aOffset,
                                  EditAction actionID,
                                  nsCOMPtr<nsIDOMNode>* outNode,
                                  int32_t* outOffset)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  MOZ_ASSERT(node && outNode && outOffset);

  
  *outNode = node->AsDOMNode();
  *outOffset = aOffset;

  
  
  if (actionID == EditAction::insertText ||
      actionID == EditAction::insertIMEText ||
      actionID == EditAction::insertBreak ||
      actionID == EditAction::deleteText) {
    bool isSpace, isNBSP;
    nsCOMPtr<nsIContent> content = do_QueryInterface(node), temp;
    
    
    
    
    while (content) {
      int32_t offset;
      if (aWhere == kStart) {
        NS_ENSURE_TRUE(mHTMLEditor, );
        mHTMLEditor->IsPrevCharInNodeWhitespace(content, *outOffset,
                                                &isSpace, &isNBSP,
                                                getter_AddRefs(temp), &offset);
      } else {
        NS_ENSURE_TRUE(mHTMLEditor, );
        mHTMLEditor->IsNextCharInNodeWhitespace(content, *outOffset,
                                                &isSpace, &isNBSP,
                                                getter_AddRefs(temp), &offset);
      }
      if (isSpace || isNBSP) {
        content = temp;
        *outOffset = offset;
      } else {
        break;
      }
    }

    *outNode = content->AsDOMNode();
    return;
  }

  int32_t offset = aOffset;

  
  
  if (aWhere == kStart) {
    
    if (node->IsNodeOfType(nsINode::eTEXT)) {
      if (!node->GetParentNode()) {
        
        return;
      }
      offset = node->GetParentNode()->IndexOf(node);
      node = node->GetParentNode();
    }

    
    
    NS_ENSURE_TRUE(mHTMLEditor, );
    nsCOMPtr<nsINode> priorNode =
      mHTMLEditor->GetPriorHTMLNode(node, offset, true);

    while (priorNode && priorNode->GetParentNode() &&
           mHTMLEditor && !mHTMLEditor->IsVisBreak(priorNode->AsDOMNode()) &&
           !IsBlockNode(priorNode->AsDOMNode())) {
      offset = priorNode->GetParentNode()->IndexOf(priorNode);
      node = priorNode->GetParentNode();
      NS_ENSURE_TRUE(mHTMLEditor, );
      priorNode = mHTMLEditor->GetPriorHTMLNode(node, offset, true);
    }

    
    
    
    NS_ENSURE_TRUE(mHTMLEditor, );
    nsCOMPtr<nsIContent> nearNode =
      mHTMLEditor->GetPriorHTMLNode(node, offset, true);
    while (!nearNode && !node->IsHTMLElement(nsGkAtoms::body) &&
           node->GetParentNode()) {
      
      
      
      
      if (actionID == EditAction::outdent &&
          node->IsHTMLElement(nsGkAtoms::blockquote)) {
        break;
      }

      int32_t parentOffset = node->GetParentNode()->IndexOf(node);
      nsCOMPtr<nsINode> parent = node->GetParentNode();

      
      
      
      
      bool blockLevelAction = actionID == EditAction::indent ||
                              actionID == EditAction::outdent ||
                              actionID == EditAction::align ||
                              actionID == EditAction::makeBasicBlock;
      NS_ENSURE_TRUE(mHTMLEditor, );
      if (!mHTMLEditor->IsDescendantOfEditorRoot(parent) &&
          (blockLevelAction || !mHTMLEditor ||
           !mHTMLEditor->IsDescendantOfEditorRoot(node))) {
        NS_ENSURE_TRUE(mHTMLEditor, );
        break;
      }

      node = parent;
      offset = parentOffset;
      NS_ENSURE_TRUE(mHTMLEditor, );
      nearNode = mHTMLEditor->GetPriorHTMLNode(node, offset, true);
    }
    *outNode = node->AsDOMNode();
    *outOffset = offset;
    return;
  }

  
  
  if (node->IsNodeOfType(nsINode::eTEXT)) {
    if (!node->GetParentNode()) {
      
      return;
    }
    
    offset = 1 + node->GetParentNode()->IndexOf(node);
    node = node->GetParentNode();
  }

  
  
  NS_ENSURE_TRUE(mHTMLEditor, );
  nsCOMPtr<nsIContent> nextNode =
    mHTMLEditor->GetNextHTMLNode(node, offset, true);

  while (nextNode && !IsBlockNode(nextNode->AsDOMNode()) &&
         nextNode->GetParentNode()) {
    offset = 1 + nextNode->GetParentNode()->IndexOf(nextNode);
    node = nextNode->GetParentNode();
    NS_ENSURE_TRUE(mHTMLEditor, );
    if (mHTMLEditor->IsVisBreak(nextNode->AsDOMNode())) {
      break;
    }
    NS_ENSURE_TRUE(mHTMLEditor, );
    nextNode = mHTMLEditor->GetNextHTMLNode(node, offset, true);
  }

  
  
  
  NS_ENSURE_TRUE(mHTMLEditor, );
  nsCOMPtr<nsIContent> nearNode =
    mHTMLEditor->GetNextHTMLNode(node, offset, true);
  while (!nearNode && !node->IsHTMLElement(nsGkAtoms::body) &&
         node->GetParentNode()) {
    int32_t parentOffset = node->GetParentNode()->IndexOf(node);
    nsCOMPtr<nsINode> parent = node->GetParentNode();

    
    
    
    if ((!mHTMLEditor || !mHTMLEditor->IsDescendantOfEditorRoot(node)) &&
        (!mHTMLEditor || !mHTMLEditor->IsDescendantOfEditorRoot(parent))) {
      NS_ENSURE_TRUE(mHTMLEditor, );
      break;
    }

    node = parent;
    
    offset = parentOffset + 1;
    NS_ENSURE_TRUE(mHTMLEditor, );
    nearNode = mHTMLEditor->GetNextHTMLNode(node, offset, true);
  }
  *outNode = node->AsDOMNode();
  *outOffset = offset;
}






void
nsHTMLEditRules::GetPromotedRanges(Selection& aSelection,
                                   nsTArray<nsRefPtr<nsRange>>& outArrayOfRanges,
                                   EditAction inOperationType)
{
  uint32_t rangeCount = aSelection.RangeCount();

  for (uint32_t i = 0; i < rangeCount; i++) {
    nsRefPtr<nsRange> selectionRange = aSelection.GetRangeAt(i);
    MOZ_ASSERT(selectionRange);

    
    nsRefPtr<nsRange> opRange = selectionRange->CloneRange();

    
    
    
    PromoteRange(*opRange, inOperationType);

    
    outArrayOfRanges.AppendElement(opRange);
  }
}






void
nsHTMLEditRules::PromoteRange(nsRange& aRange, EditAction aOperationType)
{
  NS_ENSURE_TRUE(mHTMLEditor, );
  nsCOMPtr<nsIEditor> kungFuDeathGrip(mHTMLEditor);

  nsCOMPtr<nsINode> startNode = aRange.GetStartParent();
  nsCOMPtr<nsINode> endNode = aRange.GetEndParent();
  int32_t startOffset = aRange.StartOffset();
  int32_t endOffset = aRange.EndOffset();

  
  
  
  
  if (startNode == endNode && startOffset == endOffset) {
    nsCOMPtr<Element> block;
    if (IsBlockNode(GetAsDOMNode(startNode))) {
      block = startNode->AsElement();
    } else {
      block = mHTMLEditor->GetBlockNodeParent(startNode);
    }
    if (block) {
      bool bIsEmptyNode = false;
      nsCOMPtr<nsIContent> root = mHTMLEditor->GetActiveEditingHost();
      
      NS_ENSURE_TRUE(root, );
      if (!nsContentUtils::ContentIsDescendantOf(root, block)) {
        mHTMLEditor->IsEmptyNode(block, &bIsEmptyNode, true, false);
      }
      if (bIsEmptyNode) {
        startNode = block;
        endNode = block;
        startOffset = 0;
        endOffset = block->Length();
      }
    }
  }

  
  
  

  nsCOMPtr<nsIDOMNode> opStartNode;
  nsCOMPtr<nsIDOMNode> opEndNode;
  int32_t opStartOffset, opEndOffset;
  nsRefPtr<nsRange> opRange;

  GetPromotedPoint(kStart, GetAsDOMNode(startNode), startOffset,
                   aOperationType, address_of(opStartNode), &opStartOffset);
  GetPromotedPoint(kEnd, GetAsDOMNode(endNode), endOffset, aOperationType,
                   address_of(opEndNode), &opEndOffset);

  
  if (!mHTMLEditor->IsDescendantOfEditorRoot(
        nsEditor::GetNodeAtRangeOffsetPoint(opStartNode, opStartOffset)) ||
      !mHTMLEditor->IsDescendantOfEditorRoot(
        nsEditor::GetNodeAtRangeOffsetPoint(opEndNode, opEndOffset - 1))) {
    return;
  }

  DebugOnly<nsresult> res = aRange.SetStart(opStartNode, opStartOffset);
  MOZ_ASSERT(NS_SUCCEEDED(res));
  res = aRange.SetEnd(opEndNode, opEndOffset);
  MOZ_ASSERT(NS_SUCCEEDED(res));
}

class NodeComparator
{
  public:
    bool Equals(const nsINode* node, const nsIDOMNode* domNode) const
    {
      return domNode == GetAsDOMNode(const_cast<nsINode*>(node));
    }
};

class nsUniqueFunctor : public nsBoolDomIterFunctor
{
public:
  explicit nsUniqueFunctor(nsTArray<OwningNonNull<nsINode>> &aArray) : mArray(aArray)
  {
  }
  
  virtual bool operator()(nsIDOMNode* aNode) const
  {
    return !mArray.Contains(aNode, NodeComparator());
  }

private:
  nsTArray<OwningNonNull<nsINode>>& mArray;
};





nsresult
nsHTMLEditRules::GetNodesForOperation(nsTArray<nsRefPtr<nsRange>>& aArrayOfRanges,
                                      nsTArray<OwningNonNull<nsINode>>& aOutArrayOfNodes,
                                      EditAction aOperationType,
                                      TouchContent aTouchContent)
{
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIEditor> kungFuDeathGrip(mHTMLEditor);

  int32_t rangeCount = aArrayOfRanges.Length();
  nsresult res;

  
  

  if (aTouchContent == TouchContent::yes) {
    nsTArray<OwningNonNull<nsRangeStore>> rangeItemArray;
    rangeItemArray.AppendElements(rangeCount);

    
    for (int32_t i = 0; i < rangeCount; i++) {
      rangeItemArray[i] = new nsRangeStore();
      rangeItemArray[i]->StoreRange(aArrayOfRanges[0]);
      mHTMLEditor->mRangeUpdater.RegisterRangeItem(rangeItemArray[i]);
      aArrayOfRanges.RemoveElementAt(0);
    }
    
    for (auto& item : Reversed(rangeItemArray)) {
      res = BustUpInlinesAtRangeEndpoints(*item);
      if (NS_FAILED(res)) {
        break;
      }
    }
    
    for (auto& item : rangeItemArray) {
      mHTMLEditor->mRangeUpdater.DropRangeItem(item);
      aArrayOfRanges.AppendElement(item->GetRange());
    }
    NS_ENSURE_SUCCESS(res, res);
  }
  
  for (auto& range : aArrayOfRanges) {
    nsDOMSubtreeIterator iter(*range);
    if (aOutArrayOfNodes.Length() == 0) {
      iter.AppendList(nsTrivialFunctor(), aOutArrayOfNodes);
    } else {
      
      
      
      nsTArray<OwningNonNull<nsINode>> nodes;
      iter.AppendList(nsUniqueFunctor(aOutArrayOfNodes), nodes);
      aOutArrayOfNodes.AppendElements(nodes);
    }
  }

  
  
  if (aOperationType == EditAction::makeBasicBlock) {
    for (int32_t i = aOutArrayOfNodes.Length() - 1; i >= 0; i--) {
      OwningNonNull<nsINode> node = aOutArrayOfNodes[i];
      if (nsHTMLEditUtils::IsListItem(node)) {
        int32_t j = i;
        aOutArrayOfNodes.RemoveElementAt(i);
        GetInnerContent(*node, aOutArrayOfNodes, &j);
      }
    }
  
  
  } else if (aOperationType == EditAction::outdent ||
             aOperationType == EditAction::indent ||
             aOperationType == EditAction::setAbsolutePosition) {
    for (int32_t i = aOutArrayOfNodes.Length() - 1; i >= 0; i--) {
      OwningNonNull<nsINode> node = aOutArrayOfNodes[i];
      if (nsHTMLEditUtils::IsTableElementButNotTable(node)) {
        int32_t j = i;
        aOutArrayOfNodes.RemoveElementAt(i);
        GetInnerContent(*node, aOutArrayOfNodes, &j);
      }
    }
  }
  
  if (aOperationType == EditAction::outdent &&
      !mHTMLEditor->IsCSSEnabled()) {
    for (int32_t i = aOutArrayOfNodes.Length() - 1; i >= 0; i--) {
      OwningNonNull<nsINode> node = aOutArrayOfNodes[i];
      if (node->IsHTMLElement(nsGkAtoms::div)) {
        int32_t j = i;
        aOutArrayOfNodes.RemoveElementAt(i);
        GetInnerContent(*node, aOutArrayOfNodes, &j, Lists::no, Tables::no);
      }
    }
  }


  
  
  if (aOperationType == EditAction::makeBasicBlock ||
      aOperationType == EditAction::makeList ||
      aOperationType == EditAction::align ||
      aOperationType == EditAction::setAbsolutePosition ||
      aOperationType == EditAction::indent ||
      aOperationType == EditAction::outdent) {
    for (int32_t i = aOutArrayOfNodes.Length() - 1; i >= 0; i--) {
      OwningNonNull<nsINode> node = aOutArrayOfNodes[i];
      if (aTouchContent == TouchContent::yes &&
          IsInlineNode(GetAsDOMNode(node)) && mHTMLEditor->IsContainer(node) &&
          !mHTMLEditor->IsTextNode(node)) {
        nsTArray<OwningNonNull<nsINode>> arrayOfInlines;
        res = BustUpInlinesAtBRs(*node, arrayOfInlines);
        NS_ENSURE_SUCCESS(res, res);

        
        aOutArrayOfNodes.RemoveElementAt(i);
        aOutArrayOfNodes.InsertElementsAt(i, arrayOfInlines);
      }
    }
  }
  return NS_OK;
}


void
nsHTMLEditRules::GetChildNodesForOperation(nsINode& aNode,
    nsTArray<OwningNonNull<nsINode>>& outArrayOfNodes)
{
  for (nsCOMPtr<nsIContent> child = aNode.GetFirstChild();
       child; child = child->GetNextSibling()) {
    outArrayOfNodes.AppendElement(*child);
  }
}


nsresult
nsHTMLEditRules::GetListActionNodes(nsTArray<OwningNonNull<nsINode>>& aOutArrayOfNodes,
                                    EntireList aEntireList,
                                    TouchContent aTouchContent)
{
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIEditor> kungFuDeathGrip(mHTMLEditor);

  nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  
  
  if (aEntireList == EntireList::yes) {
    uint32_t rangeCount = selection->RangeCount();
    for (uint32_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
      nsRefPtr<nsRange> range = selection->GetRangeAt(rangeIdx);
      for (nsCOMPtr<nsINode> parent = range->GetCommonAncestor();
           parent; parent = parent->GetParentNode()) {
        if (nsHTMLEditUtils::IsList(parent)) {
          aOutArrayOfNodes.AppendElement(*parent);
          break;
        }
      }
    }
    
    
    if (aOutArrayOfNodes.Length()) {
      return NS_OK;
    }
  }

  {
    
    nsAutoTxnsConserveSelection dontSpazMySelection(mHTMLEditor);

    
    nsresult res = GetNodesFromSelection(*selection, EditAction::makeList,
                                         aOutArrayOfNodes, aTouchContent);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  for (int32_t i = aOutArrayOfNodes.Length() - 1; i >= 0; i--) {
    OwningNonNull<nsINode> testNode = aOutArrayOfNodes[i];

    
    if (!mHTMLEditor->IsEditable(testNode)) {
      aOutArrayOfNodes.RemoveElementAt(i);
      continue;
    }

    
    
    if (nsHTMLEditUtils::IsTableElementButNotTable(testNode)) {
      int32_t j = i;
      aOutArrayOfNodes.RemoveElementAt(i);
      GetInnerContent(*testNode, aOutArrayOfNodes, &j, Lists::no);
    }
  }

  
  
  LookInsideDivBQandList(aOutArrayOfNodes);

  return NS_OK;
}


void
nsHTMLEditRules::LookInsideDivBQandList(nsTArray<OwningNonNull<nsINode>>& aNodeArray)
{
  NS_ENSURE_TRUE(mHTMLEditor, );
  nsCOMPtr<nsIEditor> kungFuDeathGrip(mHTMLEditor);

  
  
  int32_t listCount = aNodeArray.Length();
  if (listCount != 1) {
    return;
  }

  OwningNonNull<nsINode> curNode = aNodeArray[0];

  while (curNode->IsHTMLElement(nsGkAtoms::div) ||
         nsHTMLEditUtils::IsList(curNode) ||
         curNode->IsHTMLElement(nsGkAtoms::blockquote)) {
    
    uint32_t numChildren = mHTMLEditor->CountEditableChildren(curNode);
    if (numChildren != 1) {
      break;
    }

    
    nsCOMPtr<nsIContent> child = curNode->GetFirstChild();
    if (!child->IsHTMLElement(nsGkAtoms::div) &&
        !nsHTMLEditUtils::IsList(child) &&
        !child->IsHTMLElement(nsGkAtoms::blockquote)) {
      break;
    }

    
    curNode = child;
  }

  
  
  aNodeArray.RemoveElementAt(0);
  if (curNode->IsAnyOfHTMLElements(nsGkAtoms::div,
                                   nsGkAtoms::blockquote)) {
    int32_t j = 0;
    GetInnerContent(*curNode, aNodeArray, &j, Lists::no, Tables::no);
    return;
  }

  aNodeArray.AppendElement(*curNode);
}





void
nsHTMLEditRules::GetDefinitionListItemTypes(dom::Element* aElement, bool* aDT, bool* aDD)
{
  MOZ_ASSERT(aElement);
  MOZ_ASSERT(aElement->IsHTMLElement(nsGkAtoms::dl));
  MOZ_ASSERT(aDT);
  MOZ_ASSERT(aDD);

  *aDT = *aDD = false;
  for (nsIContent* child = aElement->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsHTMLElement(nsGkAtoms::dt)) {
      *aDT = true;
    } else if (child->IsHTMLElement(nsGkAtoms::dd)) {
      *aDD = true;
    }
  }
}

nsresult
nsHTMLEditRules::GetParagraphFormatNodes(nsTArray<OwningNonNull<nsINode>>& outArrayOfNodes,
                                         TouchContent aTouchContent)
{
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIEditor> kungFuDeathGrip(mHTMLEditor);

  nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();
  NS_ENSURE_STATE(selection);

  
  nsresult res = GetNodesFromSelection(*selection, EditAction::makeBasicBlock,
                                       outArrayOfNodes, aTouchContent);
  NS_ENSURE_SUCCESS(res, res);

  
  for (int32_t i = outArrayOfNodes.Length() - 1; i >= 0; i--) {
    OwningNonNull<nsINode> testNode = outArrayOfNodes[i];

    
    if (!mHTMLEditor->IsEditable(testNode)) {
      outArrayOfNodes.RemoveElementAt(i);
    }

    
    
    
    if (nsHTMLEditUtils::IsTableElement(testNode) ||
        nsHTMLEditUtils::IsList(testNode) ||
        nsHTMLEditUtils::IsListItem(testNode)) {
      int32_t j = i;
      outArrayOfNodes.RemoveElementAt(i);
      GetInnerContent(testNode, outArrayOfNodes, &j);
    }
  }
  return NS_OK;
}





nsresult 
nsHTMLEditRules::BustUpInlinesAtRangeEndpoints(nsRangeStore &item)
{
  nsresult res = NS_OK;
  bool isCollapsed = ((item.startNode == item.endNode) && (item.startOffset == item.endOffset));

  nsCOMPtr<nsIDOMNode> endInline =
    GetHighestInlineParent(GetAsDOMNode(item.endNode));
  
  
  if (endInline && !isCollapsed)
  {
    nsCOMPtr<nsIDOMNode> resultEndNode;
    int32_t resultEndOffset;
    endInline->GetParentNode(getter_AddRefs(resultEndNode));
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->SplitNodeDeep(endInline, GetAsDOMNode(item.endNode),
                                     item.endOffset, &resultEndOffset, true);
    NS_ENSURE_SUCCESS(res, res);
    
    item.endNode = do_QueryInterface(resultEndNode);
    item.endOffset = resultEndOffset;
  }

  nsCOMPtr<nsIDOMNode> startInline =
    GetHighestInlineParent(GetAsDOMNode(item.startNode));

  if (startInline)
  {
    nsCOMPtr<nsIDOMNode> resultStartNode;
    int32_t resultStartOffset;
    startInline->GetParentNode(getter_AddRefs(resultStartNode));
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->SplitNodeDeep(startInline, GetAsDOMNode(item.startNode),
                                     item.startOffset, &resultStartOffset,
                                     true);
    NS_ENSURE_SUCCESS(res, res);
    
    item.startNode = do_QueryInterface(resultStartNode);
    item.startOffset = resultStartOffset;
  }
  
  return res;
}



nsresult
nsHTMLEditRules::BustUpInlinesAtBRs(nsINode& aNode,
                                    nsTArray<OwningNonNull<nsINode>>& aOutArrayOfNodes)
{
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIEditor> kungFuDeathGrip(mHTMLEditor);

  
  nsTArray<OwningNonNull<nsINode>> arrayOfBreaks;
  nsBRNodeFunctor functor;
  nsDOMIterator iter(aNode);
  iter.AppendList(functor, arrayOfBreaks);

  
  if (!arrayOfBreaks.Length()) {
    aOutArrayOfNodes.AppendElement(aNode);
    return NS_OK;
  }

  
  nsCOMPtr<nsINode> inlineParentNode = aNode.GetParentNode();
  nsCOMPtr<nsIDOMNode> splitDeepNode = GetAsDOMNode(&aNode);
  nsCOMPtr<nsIDOMNode> leftDOMNode, rightDOMNode;

  for (uint32_t i = 0; i < arrayOfBreaks.Length(); i++) {
    OwningNonNull<Element> breakNode = *arrayOfBreaks[i]->AsElement();
    NS_ENSURE_TRUE(splitDeepNode, NS_ERROR_NULL_POINTER);
    nsCOMPtr<nsINode> splitParentNode = breakNode->GetParentNode();
    int32_t splitOffset = splitParentNode ?
      splitParentNode->IndexOf(breakNode) : -1;

    int32_t resultOffset;
    nsresult res = mHTMLEditor->SplitNodeDeep(splitDeepNode,
        GetAsDOMNode(splitParentNode), splitOffset, &resultOffset, false,
        address_of(leftDOMNode), address_of(rightDOMNode));
    NS_ENSURE_SUCCESS(res, res);

    
    if (leftDOMNode) {
      
      
      
      nsCOMPtr<nsINode> leftNode = do_QueryInterface(leftDOMNode);
      NS_ENSURE_STATE(leftNode);
      aOutArrayOfNodes.AppendElement(*leftNode);
    }
    
    res = mHTMLEditor->MoveNode(breakNode, inlineParentNode, resultOffset);
    NS_ENSURE_SUCCESS(res, res);
    aOutArrayOfNodes.AppendElement(*breakNode);

    
    splitDeepNode = rightDOMNode;
  }
  
  if (rightDOMNode) {
    nsCOMPtr<nsINode> rightNode = do_QueryInterface(rightDOMNode);
    NS_ENSURE_STATE(rightNode);
    aOutArrayOfNodes.AppendElement(*rightNode);
  }
  return NS_OK;
}


nsCOMPtr<nsIDOMNode> 
nsHTMLEditRules::GetHighestInlineParent(nsIDOMNode* aNode)
{
  NS_ENSURE_TRUE(aNode, nullptr);
  if (IsBlockNode(aNode)) return nullptr;
  nsCOMPtr<nsIDOMNode> inlineNode, node=aNode;

  while (node && IsInlineNode(node))
  {
    inlineNode = node;
    inlineNode->GetParentNode(getter_AddRefs(node));
  }
  return inlineNode;
}






nsresult
nsHTMLEditRules::GetNodesFromPoint(::DOMPoint aPoint,
                                   EditAction aOperation,
                                   nsTArray<OwningNonNull<nsINode>>& outArrayOfNodes,
                                   TouchContent aTouchContent)
{
  NS_ENSURE_STATE(aPoint.node);
  nsRefPtr<nsRange> range = new nsRange(aPoint.node);
  nsresult res = range->SetStart(aPoint.node, aPoint.offset);
  MOZ_ASSERT(NS_SUCCEEDED(res));

  
  PromoteRange(*range, aOperation);

  
  nsTArray<nsRefPtr<nsRange>> arrayOfRanges;

  
  arrayOfRanges.AppendElement(range);

  
  res = GetNodesForOperation(arrayOfRanges, outArrayOfNodes, aOperation,
                             aTouchContent);
  NS_ENSURE_SUCCESS(res, res);

  return NS_OK;
}






nsresult
nsHTMLEditRules::GetNodesFromSelection(Selection& aSelection,
                                       EditAction aOperation,
                                       nsTArray<OwningNonNull<nsINode>>& outArrayOfNodes,
                                       TouchContent aTouchContent)
{
  
  nsTArray<nsRefPtr<nsRange>> arrayOfRanges;
  GetPromotedRanges(aSelection, arrayOfRanges, aOperation);

  
  nsresult res = GetNodesForOperation(arrayOfRanges, outArrayOfNodes,
                                      aOperation, aTouchContent);
  NS_ENSURE_SUCCESS(res, res);

  return NS_OK;
}






void
nsHTMLEditRules::MakeTransitionList(nsTArray<OwningNonNull<nsINode>>& aNodeArray,
                                    nsTArray<bool>& aTransitionArray)
{
  nsCOMPtr<nsINode> prevParent;

  aTransitionArray.EnsureLengthAtLeast(aNodeArray.Length());
  for (uint32_t i = 0; i < aNodeArray.Length(); i++) {
    if (aNodeArray[i]->GetParentNode() != prevParent) {
      
      aTransitionArray[i] = true;
    } else {
      
      aTransitionArray[i] = false;
    }
    prevParent = aNodeArray[i]->GetParentNode();
  }
}






 






already_AddRefed<nsIDOMNode>
nsHTMLEditRules::IsInListItem(nsIDOMNode* aNode)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  nsCOMPtr<nsIDOMNode> retval = do_QueryInterface(IsInListItem(node));
  return retval.forget();
}

Element*
nsHTMLEditRules::IsInListItem(nsINode* aNode)
{
  NS_ENSURE_TRUE(aNode, nullptr);
  if (nsHTMLEditUtils::IsListItem(aNode)) {
    return aNode->AsElement();
  }

  Element* parent = aNode->GetParentElement();
  while (parent && mHTMLEditor && mHTMLEditor->IsDescendantOfEditorRoot(parent) &&
         !nsHTMLEditUtils::IsTableElement(parent)) {
    if (nsHTMLEditUtils::IsListItem(parent)) {
      return parent;
    }
    parent = parent->GetParentElement();
  }
  return nullptr;
}





nsresult 
nsHTMLEditRules::ReturnInHeader(Selection* aSelection, 
                                nsIDOMNode *aHeader, 
                                nsIDOMNode *aNode, 
                                int32_t aOffset)
{
  NS_ENSURE_TRUE(aSelection && aHeader && aNode, NS_ERROR_NULL_POINTER);  
  
  
  int32_t offset;
  nsCOMPtr<nsIDOMNode> headerParent = nsEditor::GetNodeLocation(aHeader, &offset);

  
  nsCOMPtr<nsINode> selNode(do_QueryInterface(aNode));
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = nsWSRunObject::PrepareToSplitAcrossBlocks(mHTMLEditor,
                                                           address_of(selNode),
                                                           &aOffset);
  NS_ENSURE_SUCCESS(res, res);

  
  int32_t newOffset;
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->SplitNodeDeep(aHeader, GetAsDOMNode(selNode), aOffset, &newOffset);
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsIDOMNode> prevItem;
  NS_ENSURE_STATE(mHTMLEditor);
  mHTMLEditor->GetPriorHTMLSibling(aHeader, address_of(prevItem));
  if (prevItem && nsHTMLEditUtils::IsHeader(prevItem))
  {
    bool bIsEmptyNode;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->IsEmptyNode(prevItem, &bIsEmptyNode);
    NS_ENSURE_SUCCESS(res, res);
    if (bIsEmptyNode) {
      res = CreateMozBR(prevItem, 0);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  
  
  bool isEmpty;
  res = IsEmptyBlock(aHeader, &isEmpty, true);
  NS_ENSURE_SUCCESS(res, res);
  if (isEmpty)
  {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->DeleteNode(aHeader);
    NS_ENSURE_SUCCESS(res, res);
    
    
    nsCOMPtr<nsIDOMNode> sibling;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->GetNextHTMLSibling(headerParent, offset+1, address_of(sibling));
    NS_ENSURE_SUCCESS(res, res);
    if (!sibling || !nsTextEditUtils::IsBreak(sibling))
    {
      ClearCachedStyles();
      NS_ENSURE_STATE(mHTMLEditor);
      mHTMLEditor->mTypeInState->ClearAllProps();

      
      NS_NAMED_LITERAL_STRING(pType, "p");
      nsCOMPtr<nsIDOMNode> pNode;
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->CreateNode(pType, headerParent, offset+1, getter_AddRefs(pNode));
      NS_ENSURE_SUCCESS(res, res);

      
      nsCOMPtr<nsIDOMNode> brNode;
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->CreateBR(pNode, 0, address_of(brNode));
      NS_ENSURE_SUCCESS(res, res);

      
      res = aSelection->Collapse(pNode, 0);
    }
    else
    {
      headerParent = nsEditor::GetNodeLocation(sibling, &offset);
      
      res = aSelection->Collapse(headerParent,offset+1);
    }
  }
  else
  {
    
    res = aSelection->Collapse(aHeader,0);
  }
  return res;
}




nsresult
nsHTMLEditRules::ReturnInParagraph(Selection* aSelection,
                                   nsIDOMNode* aPara,
                                   nsIDOMNode* aNode,
                                   int32_t aOffset,
                                   bool* aCancel,
                                   bool* aHandled)
{
  if (!aSelection || !aPara || !aNode || !aCancel || !aHandled) {
    return NS_ERROR_NULL_POINTER;
  }
  *aCancel = false;
  *aHandled = false;
  nsresult res;

  int32_t offset;
  nsCOMPtr<nsIDOMNode> parent = nsEditor::GetNodeLocation(aNode, &offset);

  NS_ENSURE_STATE(mHTMLEditor);
  bool doesCRCreateNewP = mHTMLEditor->GetReturnInParagraphCreatesNewParagraph();

  bool newBRneeded = false;
  nsCOMPtr<nsIDOMNode> sibling;

  NS_ENSURE_STATE(mHTMLEditor);
  if (aNode == aPara && doesCRCreateNewP) {
    
    sibling = aNode;
  } else if (mHTMLEditor->IsTextNode(aNode)) {
    nsCOMPtr<nsIDOMText> textNode = do_QueryInterface(aNode);
    uint32_t strLength;
    res = textNode->GetLength(&strLength);
    NS_ENSURE_SUCCESS(res, res);

    
    if (!aOffset) {
      
      NS_ENSURE_STATE(mHTMLEditor);
      mHTMLEditor->GetPriorHTMLSibling(aNode, address_of(sibling));
      if (!sibling || !mHTMLEditor || !mHTMLEditor->IsVisBreak(sibling) ||
          nsTextEditUtils::HasMozAttr(sibling)) {
        NS_ENSURE_STATE(mHTMLEditor);
        newBRneeded = true;
      }
    } else if (aOffset == (int32_t)strLength) {
      
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetNextHTMLSibling(aNode, address_of(sibling));
      if (!sibling || !mHTMLEditor || !mHTMLEditor->IsVisBreak(sibling) ||
          nsTextEditUtils::HasMozAttr(sibling)) {
        NS_ENSURE_STATE(mHTMLEditor);
        newBRneeded = true;
        offset++;
      }
    } else {
      if (doesCRCreateNewP) {
        nsCOMPtr<nsIDOMNode> tmp;
        res = mEditor->SplitNode(aNode, aOffset, getter_AddRefs(tmp));
        NS_ENSURE_SUCCESS(res, res);
        aNode = tmp;
      }

      newBRneeded = true;
      offset++;
    }
  } else {
    
    
    nsCOMPtr<nsIDOMNode> nearNode, selNode = aNode;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->GetPriorHTMLNode(aNode, aOffset, address_of(nearNode));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_STATE(mHTMLEditor);
    if (!nearNode || !mHTMLEditor->IsVisBreak(nearNode) ||
        nsTextEditUtils::HasMozAttr(nearNode)) {
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetNextHTMLNode(aNode, aOffset, address_of(nearNode));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_STATE(mHTMLEditor);
      if (!nearNode || !mHTMLEditor->IsVisBreak(nearNode) ||
          nsTextEditUtils::HasMozAttr(nearNode)) {
        newBRneeded = true;
      }
    }
    if (!newBRneeded) {
      sibling = nearNode;
    }
  }
  if (newBRneeded) {
    
    NS_ENSURE_TRUE(doesCRCreateNewP, NS_OK);

    nsCOMPtr<nsIDOMNode> brNode;
    NS_ENSURE_STATE(mHTMLEditor);
    res =  mHTMLEditor->CreateBR(parent, offset, address_of(brNode));
    sibling = brNode;
  }
  nsCOMPtr<nsIDOMNode> selNode = aNode;
  *aHandled = true;
  return SplitParagraph(aPara, sibling, aSelection, address_of(selNode), &aOffset);
}




nsresult 
nsHTMLEditRules::SplitParagraph(nsIDOMNode *aPara,
                                nsIDOMNode *aBRNode, 
                                Selection* aSelection,
                                nsCOMPtr<nsIDOMNode> *aSelNode, 
                                int32_t *aOffset)
{
  NS_ENSURE_TRUE(aPara && aBRNode && aSelNode && *aSelNode && aOffset && aSelection, NS_ERROR_NULL_POINTER);
  nsresult res = NS_OK;
  
  
  int32_t newOffset;
  
  nsCOMPtr<nsIDOMNode> leftPara, rightPara;
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsINode> selNode(do_QueryInterface(*aSelNode));
  res = nsWSRunObject::PrepareToSplitAcrossBlocks(mHTMLEditor, address_of(selNode), aOffset);
  *aSelNode = GetAsDOMNode(selNode);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->SplitNodeDeep(aPara, *aSelNode, *aOffset, &newOffset, false,
                                   address_of(leftPara), address_of(rightPara));
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_STATE(mHTMLEditor);
  if (mHTMLEditor->IsVisBreak(aBRNode))
  {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->DeleteNode(aBRNode);  
    NS_ENSURE_SUCCESS(res, res);
  }

  
  nsCOMPtr<nsIDOMElement> rightElt = do_QueryInterface(rightPara);
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->RemoveAttribute(rightElt, NS_LITERAL_STRING("id"));
  NS_ENSURE_SUCCESS(res, res);

  
  res = InsertMozBRIfNeeded(leftPara);
  NS_ENSURE_SUCCESS(res, res);
  res = InsertMozBRIfNeeded(rightPara);
  NS_ENSURE_SUCCESS(res, res);

  
  
  nsCOMPtr<nsINode> rightParaNode = do_QueryInterface(rightPara);
  NS_ENSURE_STATE(mHTMLEditor && rightParaNode);
  nsCOMPtr<nsIDOMNode> child =
    GetAsDOMNode(mHTMLEditor->GetLeftmostChild(rightParaNode, true));
  NS_ENSURE_STATE(mHTMLEditor);
  if (mHTMLEditor->IsTextNode(child) || !mHTMLEditor ||
      mHTMLEditor->IsContainer(child))
  {
    NS_ENSURE_STATE(mHTMLEditor);
    aSelection->Collapse(child,0);
  }
  else
  {
    int32_t offset;
    nsCOMPtr<nsIDOMNode> parent = nsEditor::GetNodeLocation(child, &offset);
    aSelection->Collapse(parent,offset);
  }
  return res;
}





nsresult 
nsHTMLEditRules::ReturnInListItem(Selection* aSelection, 
                                  nsIDOMNode *aListItem, 
                                  nsIDOMNode *aNode, 
                                  int32_t aOffset)
{
  nsCOMPtr<Element> listItem = do_QueryInterface(aListItem);
  NS_ENSURE_TRUE(aSelection && listItem && aNode, NS_ERROR_NULL_POINTER);
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIDOMNode> listitem;
  
  
  NS_PRECONDITION(true == nsHTMLEditUtils::IsListItem(aListItem),
                  "expected a list item and didn't get one");
  
  
  NS_ENSURE_STATE(mHTMLEditor);
  nsIContent* rootContent = mHTMLEditor->GetActiveEditingHost();
  nsCOMPtr<nsIDOMNode> rootNode = do_QueryInterface(rootContent);
  nsCOMPtr<nsINode> list = listItem->GetParentNode();
  int32_t itemOffset = list ? list->IndexOf(listItem) : -1;

  
  
  bool isEmpty;
  res = IsEmptyBlock(aListItem, &isEmpty, true, false);
  NS_ENSURE_SUCCESS(res, res);
  if (isEmpty && (rootNode != GetAsDOMNode(list)) &&
      mReturnInEmptyLIKillsList) {
    
    nsCOMPtr<nsINode> listParent = list->GetParentNode();
    int32_t offset = listParent ? listParent->IndexOf(list) : -1;

    
    bool bIsLast;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->IsLastEditableChild(aListItem, &bIsLast);
    NS_ENSURE_SUCCESS(res, res);
    if (!bIsLast)
    {
      
      nsCOMPtr<nsIDOMNode> tempNode;
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->SplitNode(GetAsDOMNode(list), itemOffset,
                                   getter_AddRefs(tempNode));
      NS_ENSURE_SUCCESS(res, res);
    }

    
    if (nsHTMLEditUtils::IsList(listParent)) {
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->MoveNode(listItem, listParent, offset + 1);
      NS_ENSURE_SUCCESS(res, res);
      res = aSelection->Collapse(aListItem,0);
    }
    else
    {
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(aListItem);
      NS_ENSURE_SUCCESS(res, res);

      
      NS_NAMED_LITERAL_STRING(pType, "p");
      nsCOMPtr<nsIDOMNode> pNode;
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->CreateNode(pType, GetAsDOMNode(listParent),
                                    offset + 1, getter_AddRefs(pNode));
      NS_ENSURE_SUCCESS(res, res);

      
      nsCOMPtr<nsIDOMNode> brNode;
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->CreateBR(pNode, 0, address_of(brNode));
      NS_ENSURE_SUCCESS(res, res);

      
      res = aSelection->Collapse(pNode, 0);
    }
    return res;
  }
  
  
  
  nsCOMPtr<nsINode> selNode(do_QueryInterface(aNode));
  NS_ENSURE_STATE(mHTMLEditor);
  res = nsWSRunObject::PrepareToSplitAcrossBlocks(mHTMLEditor, address_of(selNode), &aOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  int32_t newOffset;
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->SplitNodeDeep(aListItem, GetAsDOMNode(selNode), aOffset, &newOffset, false);
  NS_ENSURE_SUCCESS(res, res);
  
  
  
  nsCOMPtr<nsIDOMNode> prevItem;
  NS_ENSURE_STATE(mHTMLEditor);
  mHTMLEditor->GetPriorHTMLSibling(aListItem, address_of(prevItem));

  if (prevItem && nsHTMLEditUtils::IsListItem(prevItem))
  {
    bool bIsEmptyNode;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->IsEmptyNode(prevItem, &bIsEmptyNode);
    NS_ENSURE_SUCCESS(res, res);
    if (bIsEmptyNode) {
      res = CreateMozBR(prevItem, 0);
      NS_ENSURE_SUCCESS(res, res);
    } else {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->IsEmptyNode(aListItem, &bIsEmptyNode, true);
      NS_ENSURE_SUCCESS(res, res);
      if (bIsEmptyNode) 
      {
        nsCOMPtr<nsIAtom> nodeAtom = nsEditor::GetTag(aListItem);
        if (nodeAtom == nsGkAtoms::dd || nodeAtom == nsGkAtoms::dt) {
          int32_t itemOffset;
          nsCOMPtr<nsIDOMNode> list = nsEditor::GetNodeLocation(aListItem, &itemOffset);

          nsAutoString listTag(nodeAtom == nsGkAtoms::dt
            ? NS_LITERAL_STRING("dd") : NS_LITERAL_STRING("dt"));
          nsCOMPtr<nsIDOMNode> newListItem;
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->CreateNode(listTag, list, itemOffset+1, getter_AddRefs(newListItem));
          NS_ENSURE_SUCCESS(res, res);
          res = mEditor->DeleteNode(aListItem);
          NS_ENSURE_SUCCESS(res, res);
          return aSelection->Collapse(newListItem, 0);
        }

        nsCOMPtr<nsIDOMNode> brNode;
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->CopyLastEditableChildStyles(prevItem, aListItem, getter_AddRefs(brNode));
        NS_ENSURE_SUCCESS(res, res);
        if (brNode) 
        {
          int32_t offset;
          nsCOMPtr<nsIDOMNode> brParent = nsEditor::GetNodeLocation(brNode, &offset);
          return aSelection->Collapse(brParent, offset);
        }
      }
      else
      {
        NS_ENSURE_STATE(mHTMLEditor);
        nsWSRunObject wsObj(mHTMLEditor, aListItem, 0);
        nsCOMPtr<nsINode> visNode_;
        int32_t visOffset = 0;
        WSType wsType;
        nsCOMPtr<nsINode> aListItem_(do_QueryInterface(aListItem));
        wsObj.NextVisibleNode(aListItem_, 0, address_of(visNode_),
                              &visOffset, &wsType);
        nsCOMPtr<nsIDOMNode> visNode(GetAsDOMNode(visNode_));
        if (wsType == WSType::special || wsType == WSType::br ||
            nsHTMLEditUtils::IsHR(visNode)) {
          int32_t offset;
          nsCOMPtr<nsIDOMNode> parent = nsEditor::GetNodeLocation(visNode, &offset);
          return aSelection->Collapse(parent, offset);
        }
        else
        {
          return aSelection->Collapse(visNode, visOffset);
        }
      }
    }
  }
  res = aSelection->Collapse(aListItem,0);
  return res;
}





nsresult
nsHTMLEditRules::MakeBlockquote(nsTArray<OwningNonNull<nsINode>>& aNodeArray)
{
  
  
  
  
  nsresult res;
  nsCOMPtr<Element> curBlock;
  nsCOMPtr<nsINode> prevParent;

  for (auto& curNode : aNodeArray) {
    
    NS_ENSURE_STATE(curNode->IsContent());

    
    if (nsHTMLEditUtils::IsTableElementButNotTable(curNode) ||
        nsHTMLEditUtils::IsListItem(curNode)) {
      
      curBlock = nullptr;
      
      nsTArray<OwningNonNull<nsINode>> childArray;
      GetChildNodesForOperation(*curNode, childArray);
      res = MakeBlockquote(childArray);
      NS_ENSURE_SUCCESS(res, res);
    }

    
    
    if (prevParent) {
      if (prevParent != curNode->GetParentNode()) {
        
        curBlock = nullptr;
        prevParent = curNode->GetParentNode();
      }
    } else {
      prevParent = curNode->GetParentNode();
    }

    
    if (!curBlock) {
      nsCOMPtr<nsINode> curParent = curNode->GetParentNode();
      int32_t offset = curParent ? curParent->IndexOf(curNode) : -1;
      res = SplitAsNeeded(*nsGkAtoms::blockquote, curParent, offset);
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_STATE(mHTMLEditor);
      curBlock = mHTMLEditor->CreateNode(nsGkAtoms::blockquote, curParent,
                                         offset);
      NS_ENSURE_STATE(curBlock);
      
      mNewBlock = curBlock->AsDOMNode();
      
    }

    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->MoveNode(curNode->AsContent(), curBlock, -1);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}




nsresult
nsHTMLEditRules::RemoveBlockStyle(nsTArray<OwningNonNull<nsINode>>& aNodeArray)
{
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIEditor> kungFuDeathGrip(mHTMLEditor);

  
  
  
  nsresult res;

  nsCOMPtr<Element> curBlock;
  nsCOMPtr<nsIContent> firstNode, lastNode;
  for (auto& curNode : aNodeArray) {
    
    if (nsHTMLEditUtils::IsFormatNode(curNode)) {
      
      if (curBlock) {
        res = RemovePartOfBlock(*curBlock, *firstNode, *lastNode);
        NS_ENSURE_SUCCESS(res, res);
        firstNode = lastNode = curBlock = nullptr;
      }
      
      res = mHTMLEditor->RemoveBlockContainer(curNode->AsDOMNode());
      NS_ENSURE_SUCCESS(res, res);
    } else if (curNode->IsAnyOfHTMLElements(nsGkAtoms::table,
                                            nsGkAtoms::tr,
                                            nsGkAtoms::tbody,
                                            nsGkAtoms::td,
                                            nsGkAtoms::li,
                                            nsGkAtoms::blockquote,
                                            nsGkAtoms::div) ||
                nsHTMLEditUtils::IsList(curNode)) {
      
      if (curBlock) {
        res = RemovePartOfBlock(*curBlock, *firstNode, *lastNode);
        NS_ENSURE_SUCCESS(res, res);
        firstNode = lastNode = curBlock = nullptr;
      }
      
      nsTArray<OwningNonNull<nsINode>> childArray;
      GetChildNodesForOperation(*curNode, childArray);
      res = RemoveBlockStyle(childArray);
      NS_ENSURE_SUCCESS(res, res);
    } else if (IsInlineNode(GetAsDOMNode(curNode))) {
      if (curBlock) {
        
        if (nsEditorUtils::IsDescendantOf(curNode, curBlock)) {
          
          lastNode = curNode->AsContent();
          continue;
        } else {
          
          
          
          res = RemovePartOfBlock(*curBlock, *firstNode, *lastNode);
          NS_ENSURE_SUCCESS(res, res);
          firstNode = lastNode = curBlock = nullptr;
          
        }
      }
      curBlock = mHTMLEditor->GetBlockNodeParent(curNode);
      if (curBlock && nsHTMLEditUtils::IsFormatNode(curBlock)) {
        firstNode = lastNode = curNode->AsContent();
      } else {
        
        curBlock = nullptr;
      }
    } else if (curBlock) {
      
      
      res = RemovePartOfBlock(*curBlock, *firstNode, *lastNode);
      NS_ENSURE_SUCCESS(res, res);
      firstNode = lastNode = curBlock = nullptr;
    }
  }
  
  if (curBlock) {
    res = RemovePartOfBlock(*curBlock, *firstNode, *lastNode);
    NS_ENSURE_SUCCESS(res, res);
    firstNode = lastNode = curBlock = nullptr;
  }
  return NS_OK;
}





nsresult
nsHTMLEditRules::ApplyBlockStyle(nsTArray<OwningNonNull<nsINode>>& aNodeArray,
                                 nsIAtom& aBlockTag)
{
  
  
  
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIEditor> kungFuDeathGrip(mHTMLEditor);

  nsresult res;

  
  for (int32_t i = aNodeArray.Length() - 1; i >= 0; i--) {
    if (!mHTMLEditor->IsEditable(aNodeArray[i])) {
      aNodeArray.RemoveElementAt(i);
    }
  }

  nsCOMPtr<Element> newBlock;

  nsCOMPtr<Element> curBlock;
  for (auto& curNode : aNodeArray) {
    nsCOMPtr<nsINode> curParent = curNode->GetParentNode();
    int32_t offset = curParent ? curParent->IndexOf(curNode) : -1;

    
    if (curNode->IsHTMLElement(&aBlockTag)) {
      
      curBlock = nullptr;
      
      continue;
    }

    
    
    
    if (nsHTMLEditUtils::IsMozDiv(curNode) ||
        nsHTMLEditUtils::IsFormatNode(curNode)) {
      
      curBlock = nullptr;
      newBlock = mHTMLEditor->ReplaceContainer(curNode->AsElement(),
                                               &aBlockTag, nullptr, nullptr,
                                               nsEditor::eCloneAttributes);
      NS_ENSURE_STATE(newBlock);
    } else if (nsHTMLEditUtils::IsTable(curNode) ||
               nsHTMLEditUtils::IsList(curNode) ||
               curNode->IsAnyOfHTMLElements(nsGkAtoms::tbody,
                                            nsGkAtoms::tr,
                                            nsGkAtoms::td,
                                            nsGkAtoms::li,
                                            nsGkAtoms::blockquote,
                                            nsGkAtoms::div)) {
      
      curBlock = nullptr;
      
      nsTArray<OwningNonNull<nsINode>> childArray;
      GetChildNodesForOperation(*curNode, childArray);
      if (childArray.Length()) {
        res = ApplyBlockStyle(childArray, aBlockTag);
        NS_ENSURE_SUCCESS(res, res);
      } else {
        
        res = SplitAsNeeded(aBlockTag, curParent, offset);
        NS_ENSURE_SUCCESS(res, res);
        nsCOMPtr<Element> theBlock =
          mHTMLEditor->CreateNode(&aBlockTag, curParent, offset);
        NS_ENSURE_STATE(theBlock);
        
        mNewBlock = theBlock->AsDOMNode();
      }
    } else if (curNode->IsHTMLElement(nsGkAtoms::br)) {
      
      
      if (curBlock) {
        
        curBlock = nullptr;
        res = mHTMLEditor->DeleteNode(curNode);
        NS_ENSURE_SUCCESS(res, res);
      } else {
        
        
        res = SplitAsNeeded(aBlockTag, curParent, offset);
        NS_ENSURE_SUCCESS(res, res);
        curBlock = mHTMLEditor->CreateNode(&aBlockTag, curParent, offset);
        NS_ENSURE_STATE(curBlock);
        
        mNewBlock = curBlock->AsDOMNode();
        
        res = mHTMLEditor->MoveNode(curNode->AsContent(), curBlock, -1);
        NS_ENSURE_SUCCESS(res, res);
      }
    } else if (IsInlineNode(GetAsDOMNode(curNode))) {
      
      
      
      
      
      
      
      if (&aBlockTag == nsGkAtoms::pre && !mHTMLEditor->IsEditable(curNode)) {
        
        continue;
      }

      
      if (!curBlock) {
        res = SplitAsNeeded(aBlockTag, curParent, offset);
        NS_ENSURE_SUCCESS(res, res);
        curBlock = mHTMLEditor->CreateNode(&aBlockTag, curParent, offset);
        NS_ENSURE_STATE(curBlock);
        
        mNewBlock = curBlock->AsDOMNode();
        
      }

      

      
      
      res = mHTMLEditor->MoveNode(curNode->AsContent(), curBlock, -1);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}






nsresult
nsHTMLEditRules::SplitAsNeeded(nsIAtom& aTag,
                               nsCOMPtr<nsINode>& inOutParent,
                               int32_t& inOutOffset)
{
  NS_ENSURE_TRUE(inOutParent, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsINode> tagParent, splitNode;
  for (nsCOMPtr<nsINode> parent = inOutParent; parent;
       parent = parent->GetParentNode()) {
    

    
    NS_ENSURE_STATE(mHTMLEditor);
    if (!mHTMLEditor->IsDescendantOfEditorRoot(parent)) {
      NS_ENSURE_STATE(mHTMLEditor);
      if (parent != mHTMLEditor->GetActiveEditingHost()) {
        return NS_ERROR_FAILURE;
      }
    }

    NS_ENSURE_STATE(mHTMLEditor);
    if (mHTMLEditor->CanContainTag(*parent, aTag)) {
      
      tagParent = parent;
      break;
    }

    splitNode = parent;
  }
  if (!tagParent) {
    
    return NS_ERROR_FAILURE;
  }
  if (splitNode) {
    
    NS_ENSURE_STATE(mHTMLEditor);
    nsresult res = mHTMLEditor->SplitNodeDeep(splitNode->AsDOMNode(),
                                              inOutParent->AsDOMNode(),
                                              inOutOffset, &inOutOffset);
    NS_ENSURE_SUCCESS(res, res);
    inOutParent = tagParent;
  }
  return NS_OK;
}








::DOMPoint
nsHTMLEditRules::JoinNodesSmart(nsIContent& aNodeLeft, nsIContent& aNodeRight)
{
  
  nsCOMPtr<nsINode> parent = aNodeLeft.GetParentNode();
  NS_ENSURE_TRUE(parent, ::DOMPoint());
  int32_t parOffset = parent->IndexOf(&aNodeLeft);
  nsCOMPtr<nsINode> rightParent = aNodeRight.GetParentNode();

  
  
  nsresult res;
  if (parent != rightParent) {
    NS_ENSURE_TRUE(mHTMLEditor, ::DOMPoint());
    res = mHTMLEditor->MoveNode(&aNodeRight, parent, parOffset);
    NS_ENSURE_SUCCESS(res, ::DOMPoint());
  }

  ::DOMPoint ret(&aNodeRight, aNodeLeft.Length());

  
  if (nsHTMLEditUtils::IsList(&aNodeLeft) || aNodeLeft.GetAsText()) {
    
    res = mHTMLEditor->JoinNodes(aNodeLeft, aNodeRight);
    NS_ENSURE_SUCCESS(res, ::DOMPoint());
    return ret;
  }

  
  NS_ENSURE_TRUE(mHTMLEditor, ::DOMPoint());
  nsCOMPtr<nsIContent> lastLeft = mHTMLEditor->GetLastEditableChild(aNodeLeft);
  NS_ENSURE_TRUE(lastLeft, ::DOMPoint());

  NS_ENSURE_TRUE(mHTMLEditor, ::DOMPoint());
  nsCOMPtr<nsIContent> firstRight = mHTMLEditor->GetFirstEditableChild(aNodeRight);
  NS_ENSURE_TRUE(firstRight, ::DOMPoint());

  
  NS_ENSURE_TRUE(mHTMLEditor, ::DOMPoint());
  res = mHTMLEditor->JoinNodes(aNodeLeft, aNodeRight);
  NS_ENSURE_SUCCESS(res, ::DOMPoint());

  if (lastLeft && firstRight && mHTMLEditor &&
      mHTMLEditor->AreNodesSameType(lastLeft, firstRight) &&
      (lastLeft->GetAsText() || !mHTMLEditor ||
       (lastLeft->IsElement() && firstRight->IsElement() &&
        mHTMLEditor->mHTMLCSSUtils->ElementsSameStyle(lastLeft->AsElement(),
                                                  firstRight->AsElement())))) {
    NS_ENSURE_TRUE(mHTMLEditor, ::DOMPoint());
    return JoinNodesSmart(*lastLeft, *firstRight);
  }
  return ret;
}


Element*
nsHTMLEditRules::GetTopEnclosingMailCite(nsINode& aNode)
{
  nsCOMPtr<Element> ret;

  for (nsCOMPtr<nsINode> node = &aNode; node; node = node->GetParentNode()) {
    if ((IsPlaintextEditor() && node->IsHTMLElement(nsGkAtoms::pre)) ||
        nsHTMLEditUtils::IsMailCite(node)) {
      ret = node->AsElement();
    }
    if (node->IsHTMLElement(nsGkAtoms::body)) {
      break;
    }
  }

  return ret;
}


nsresult 
nsHTMLEditRules::CacheInlineStyles(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  NS_ENSURE_STATE(mHTMLEditor);
  bool useCSS = mHTMLEditor->IsCSSEnabled();

  for (int32_t j = 0; j < SIZE_STYLE_TABLE; ++j)
  {
    bool isSet = false;
    nsAutoString outValue;
    
    if (!useCSS || (mCachedStyles[j].tag == nsGkAtoms::font &&
                    mCachedStyles[j].attr.EqualsLiteral("size"))) {
      NS_ENSURE_STATE(mHTMLEditor);
      mHTMLEditor->IsTextPropertySetByContent(aNode, mCachedStyles[j].tag,
                                              &(mCachedStyles[j].attr), nullptr,
                                              isSet, &outValue);
    }
    else
    {
      NS_ENSURE_STATE(mHTMLEditor);
      mHTMLEditor->mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(aNode,
        mCachedStyles[j].tag, &(mCachedStyles[j].attr), isSet, outValue,
        nsHTMLCSSUtils::eComputed);
    }
    if (isSet)
    {
      mCachedStyles[j].mPresent = true;
      mCachedStyles[j].value.Assign(outValue);
    }
  }
  return NS_OK;
}


nsresult
nsHTMLEditRules::ReapplyCachedStyles()
{
  
  
  

  
  NS_ENSURE_STATE(mHTMLEditor);
  bool useCSS = mHTMLEditor->IsCSSEnabled();

  
  NS_ENSURE_STATE(mHTMLEditor);
  nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();
  MOZ_ASSERT(selection);
  if (!selection->RangeCount()) {
    
    return NS_OK;
  }
  nsCOMPtr<nsIContent> selNode =
    do_QueryInterface(selection->GetRangeAt(0)->GetStartParent());
  if (!selNode) {
    
    return NS_OK;
  }

  for (int32_t i = 0; i < SIZE_STYLE_TABLE; ++i) {
    if (mCachedStyles[i].mPresent) {
      bool bFirst, bAny, bAll;
      bFirst = bAny = bAll = false;

      nsAutoString curValue;
      if (useCSS) {
        
        NS_ENSURE_STATE(mHTMLEditor);
        bAny = mHTMLEditor->mHTMLCSSUtils->IsCSSEquivalentToHTMLInlineStyleSet(
          selNode, mCachedStyles[i].tag, &(mCachedStyles[i].attr), curValue,
          nsHTMLCSSUtils::eComputed);
      }
      if (!bAny) {
        
        NS_ENSURE_STATE(mHTMLEditor);
        nsresult res = mHTMLEditor->GetInlinePropertyBase(
                                                     *mCachedStyles[i].tag,
                                                     &(mCachedStyles[i].attr),
                                                     &(mCachedStyles[i].value),
                                                     &bFirst, &bAny, &bAll,
                                                     &curValue, false);
        NS_ENSURE_SUCCESS(res, res);
      }
      
      if (!bAny || IsStyleCachePreservingAction(mTheAction)) {
        NS_ENSURE_STATE(mHTMLEditor);
        mHTMLEditor->mTypeInState->SetProp(mCachedStyles[i].tag,
                                           mCachedStyles[i].attr,
                                           mCachedStyles[i].value);
      }
    }
  }

  return NS_OK;
}


void
nsHTMLEditRules::ClearCachedStyles()
{
  
  for (uint32_t j = 0; j < SIZE_STYLE_TABLE; j++) {
    mCachedStyles[j].mPresent = false;
    mCachedStyles[j].value.Truncate();
  }
}


void
nsHTMLEditRules::AdjustSpecialBreaks()
{
  NS_ENSURE_TRUE(mHTMLEditor, );

  
  nsTArray<OwningNonNull<nsINode>> nodeArray;
  nsEmptyEditableFunctor functor(mHTMLEditor);
  nsDOMIterator iter(*mDocChangeRange);
  iter.AppendList(functor, nodeArray);

  
  for (auto& node : nodeArray) {
    
    
    
    
    nsresult res = CreateMozBR(node->AsDOMNode(), (int32_t)node->Length());
    NS_ENSURE_SUCCESS(res, );
  }
}

nsresult 
nsHTMLEditRules::AdjustWhitespace(Selection* aSelection)
{
  
  nsCOMPtr<nsIDOMNode> selNode;
  int32_t selOffset;
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  
  NS_ENSURE_STATE(mHTMLEditor);
  return nsWSRunObject(mHTMLEditor, selNode, selOffset).AdjustWhitespace();
}

nsresult 
nsHTMLEditRules::PinSelectionToNewBlock(Selection* aSelection)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
  if (!aSelection->Collapsed()) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNode> selNode, temp;
  int32_t selOffset;
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  temp = selNode;
  
  
  nsCOMPtr<nsINode> node = do_QueryInterface(selNode);
  NS_ENSURE_STATE(node);
  nsRefPtr<nsRange> range = new nsRange(node);
  res = range->SetStart(selNode, selOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = range->SetEnd(selNode, selOffset);
  NS_ENSURE_SUCCESS(res, res);
  nsCOMPtr<nsIContent> block (do_QueryInterface(mNewBlock));
  NS_ENSURE_TRUE(block, NS_ERROR_NO_INTERFACE);
  bool nodeBefore, nodeAfter;
  res = nsRange::CompareNodeToRange(block, range, &nodeBefore, &nodeAfter);
  NS_ENSURE_SUCCESS(res, res);
  
  if (nodeBefore && nodeAfter)
    return NS_OK;  
  else if (nodeBefore)
  {
    
    nsCOMPtr<nsIDOMNode> tmp = mNewBlock;
    NS_ENSURE_STATE(mHTMLEditor);
    tmp = GetAsDOMNode(mHTMLEditor->GetLastEditableChild(*block));
    uint32_t endPoint;
    NS_ENSURE_STATE(mHTMLEditor);
    if (mHTMLEditor->IsTextNode(tmp) || !mHTMLEditor ||
        mHTMLEditor->IsContainer(tmp))
    {
      NS_ENSURE_STATE(mHTMLEditor);
      res = nsEditor::GetLengthOfDOMNode(tmp, endPoint);
      NS_ENSURE_SUCCESS(res, res);
    }
    else
    {
      tmp = nsEditor::GetNodeLocation(tmp, (int32_t*)&endPoint);
      endPoint++;  
    }
    return aSelection->Collapse(tmp, (int32_t)endPoint);
  }
  else
  {
    
    nsCOMPtr<nsIDOMNode> tmp = mNewBlock;
    NS_ENSURE_STATE(mHTMLEditor);
    tmp = GetAsDOMNode(mHTMLEditor->GetFirstEditableChild(*block));
    int32_t offset;
    if (!(mHTMLEditor->IsTextNode(tmp) || !mHTMLEditor ||
          mHTMLEditor->IsContainer(tmp)))
    {
      tmp = nsEditor::GetNodeLocation(tmp, &offset);
    }
    NS_ENSURE_STATE(mHTMLEditor);
    return aSelection->Collapse(tmp, 0);
  }
}

nsresult 
nsHTMLEditRules::CheckInterlinePosition(Selection* aSelection)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);

  
  if (!aSelection->Collapsed()) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNode> selNode, node;
  int32_t selOffset;
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);

  
  
  
  NS_ENSURE_STATE(mHTMLEditor);
  mHTMLEditor->GetPriorHTMLNode(selNode, selOffset, address_of(node), true);
  if (node && nsTextEditUtils::IsBreak(node))
  {
    aSelection->SetInterlinePosition(true);
    return NS_OK;
  }

  
  NS_ENSURE_STATE(mHTMLEditor);
  mHTMLEditor->GetPriorHTMLSibling(selNode, selOffset, address_of(node));
  if (node && IsBlockNode(node))
  {
    aSelection->SetInterlinePosition(true);
    return NS_OK;
  }

  
  NS_ENSURE_STATE(mHTMLEditor);
  mHTMLEditor->GetNextHTMLSibling(selNode, selOffset, address_of(node));
  if (node && IsBlockNode(node))
    aSelection->SetInterlinePosition(false);
  return NS_OK;
}

nsresult 
nsHTMLEditRules::AdjustSelection(Selection* aSelection,
                                 nsIEditor::EDirection aAction)
{
  NS_ENSURE_TRUE(aSelection, NS_ERROR_NULL_POINTER);
 
  
  
  
  if (!aSelection->Collapsed()) {
    return NS_OK;
  }

  
  nsCOMPtr<nsINode> selNode, temp;
  int32_t selOffset;
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(aSelection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  temp = selNode;
  
  
  NS_ENSURE_STATE(mHTMLEditor);
  while (!mHTMLEditor->IsEditable(selNode))
  {
    
    selNode = nsEditor::GetNodeLocation(temp, &selOffset);
    NS_ENSURE_TRUE(selNode, NS_ERROR_FAILURE);
    temp = selNode;
    NS_ENSURE_STATE(mHTMLEditor);
  }
  
  
  
  nsCOMPtr<nsINode> theblock;
  if (IsBlockNode(GetAsDOMNode(selNode))) {
    theblock = selNode;
  } else {
    NS_ENSURE_STATE(mHTMLEditor);
    theblock = mHTMLEditor->GetBlockNodeParent(selNode);
  }
  NS_ENSURE_STATE(mHTMLEditor);
  if (theblock && mHTMLEditor->IsEditable(theblock)) {
    bool bIsEmptyNode;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->IsEmptyNode(theblock, &bIsEmptyNode, false, false);
    NS_ENSURE_SUCCESS(res, res);
    
    NS_ENSURE_STATE(mHTMLEditor);
    if (bIsEmptyNode && mHTMLEditor->CanContainTag(*selNode, *nsGkAtoms::br)) {
      NS_ENSURE_STATE(mHTMLEditor);
      nsCOMPtr<Element> rootNode = mHTMLEditor->GetRoot();
      NS_ENSURE_TRUE(rootNode, NS_ERROR_FAILURE);
      if (selNode == rootNode)
      {
        
        
        
        return NS_OK;
      }

      
      return CreateMozBR(GetAsDOMNode(selNode), selOffset);
    }
  }
  
  
  nsCOMPtr<nsIDOMCharacterData> textNode = do_QueryInterface(selNode);
  if (textNode)
    return NS_OK; 
  
  
  
  
  

  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIContent> nearNode =
    mHTMLEditor->GetPriorHTMLNode(selNode, selOffset);
  if (nearNode) 
  {
    
    nsCOMPtr<nsINode> block, nearBlock;
    if (IsBlockNode(GetAsDOMNode(selNode))) {
      block = selNode;
    } else {
      NS_ENSURE_STATE(mHTMLEditor);
      block = mHTMLEditor->GetBlockNodeParent(selNode);
    }
    NS_ENSURE_STATE(mHTMLEditor);
    nearBlock = mHTMLEditor->GetBlockNodeParent(nearNode);
    if (block && block == nearBlock) {
      if (nearNode && nsTextEditUtils::IsBreak(nearNode) )
      {   
        NS_ENSURE_STATE(mHTMLEditor);
        if (!mHTMLEditor->IsVisBreak(nearNode))
        {
          
          
          
          nsCOMPtr<nsIDOMNode> brNode;
          res = CreateMozBR(GetAsDOMNode(selNode), selOffset,
                            getter_AddRefs(brNode));
          NS_ENSURE_SUCCESS(res, res);
          nsCOMPtr<nsIDOMNode> brParent =
            nsEditor::GetNodeLocation(brNode, &selOffset);
          
          aSelection->SetInterlinePosition(true);
          res = aSelection->Collapse(brParent, selOffset);
          NS_ENSURE_SUCCESS(res, res);
        }
        else
        {
          NS_ENSURE_STATE(mHTMLEditor);
          nsCOMPtr<nsIContent> nextNode =
            mHTMLEditor->GetNextHTMLNode(nearNode, true);
          if (nextNode && nsTextEditUtils::IsMozBR(nextNode))
          {
            
            
            aSelection->SetInterlinePosition(true);
          }
        }
      }
    }
  }

  
  NS_ENSURE_STATE(mHTMLEditor);
  nearNode = mHTMLEditor->GetPriorHTMLNode(selNode, selOffset, true);
  if (nearNode && (nsTextEditUtils::IsBreak(nearNode) ||
                   nsEditor::IsTextNode(nearNode) ||
                   nsHTMLEditUtils::IsImage(nearNode) ||
                   nearNode->IsHTMLElement(nsGkAtoms::hr))) {
    
    return NS_OK;
  }
  NS_ENSURE_STATE(mHTMLEditor);
  nearNode = mHTMLEditor->GetNextHTMLNode(selNode, selOffset, true);
  if (nearNode && (nsTextEditUtils::IsBreak(nearNode) ||
                   nsEditor::IsTextNode(nearNode) ||
                   nearNode->IsAnyOfHTMLElements(nsGkAtoms::img,
                                                 nsGkAtoms::hr))) {
    return NS_OK; 
  }

  
  
  nsCOMPtr<nsIDOMNode> nearNodeDOM = GetAsDOMNode(nearNode);
  res = FindNearSelectableNode(GetAsDOMNode(selNode), selOffset, aAction,
                               address_of(nearNodeDOM));
  NS_ENSURE_SUCCESS(res, res);
  nearNode = do_QueryInterface(nearNodeDOM);

  if (nearNode)
  {
    
    textNode = do_QueryInterface(nearNode);
    if (textNode)
    {
      int32_t offset = 0;
      
      if (aAction == nsIEditor::ePrevious)
        textNode->GetLength((uint32_t*)&offset);
      res = aSelection->Collapse(nearNode,offset);
    }
    else  
    {
      selNode = nsEditor::GetNodeLocation(nearNode, &selOffset);
      if (aAction == nsIEditor::ePrevious) selOffset++;  
      res = aSelection->Collapse(selNode, selOffset);
    }
  }
  return res;
}


nsresult
nsHTMLEditRules::FindNearSelectableNode(nsIDOMNode *aSelNode, 
                                        int32_t aSelOffset, 
                                        nsIEditor::EDirection &aDirection,
                                        nsCOMPtr<nsIDOMNode> *outSelectableNode)
{
  NS_ENSURE_TRUE(aSelNode && outSelectableNode, NS_ERROR_NULL_POINTER);
  *outSelectableNode = nullptr;
  nsresult res = NS_OK;
  
  nsCOMPtr<nsIDOMNode> nearNode, curNode;
  if (aDirection == nsIEditor::ePrevious) {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->GetPriorHTMLNode(aSelNode, aSelOffset, address_of(nearNode));
  } else {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->GetNextHTMLNode(aSelNode, aSelOffset, address_of(nearNode));
  }
  NS_ENSURE_SUCCESS(res, res);
  
  if (!nearNode) 
  {
    if (aDirection == nsIEditor::ePrevious)
      aDirection = nsIEditor::eNext;
    else
      aDirection = nsIEditor::ePrevious;
    
    if (aDirection == nsIEditor::ePrevious) {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetPriorHTMLNode(aSelNode, aSelOffset, address_of(nearNode));
    } else {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetNextHTMLNode(aSelNode, aSelOffset, address_of(nearNode));
    }
    NS_ENSURE_SUCCESS(res, res);
  }
  
  
  
  NS_ENSURE_STATE(mHTMLEditor);
  while (nearNode && !(mHTMLEditor->IsTextNode(nearNode)
                       || nsTextEditUtils::IsBreak(nearNode)
                       || nsHTMLEditUtils::IsImage(nearNode)))
  {
    curNode = nearNode;
    if (aDirection == nsIEditor::ePrevious) {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetPriorHTMLNode(curNode, address_of(nearNode));
    } else {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetNextHTMLNode(curNode, address_of(nearNode));
    }
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_STATE(mHTMLEditor);
  }
  
  if (nearNode)
  {
    
    if (InDifferentTableElements(nearNode, aSelNode)) {
      return NS_OK;
    }
    
    
    *outSelectableNode = do_QueryInterface(nearNode);
  }
  return res;
}


bool nsHTMLEditRules::InDifferentTableElements(nsIDOMNode* aNode1,
                                               nsIDOMNode* aNode2)
{
  nsCOMPtr<nsINode> node1 = do_QueryInterface(aNode1);
  nsCOMPtr<nsINode> node2 = do_QueryInterface(aNode2);
  return InDifferentTableElements(node1, node2);
}

bool
nsHTMLEditRules::InDifferentTableElements(nsINode* aNode1, nsINode* aNode2)
{
  MOZ_ASSERT(aNode1 && aNode2);

  while (aNode1 && !nsHTMLEditUtils::IsTableElement(aNode1)) {
    aNode1 = aNode1->GetParentNode();
  }

  while (aNode2 && !nsHTMLEditUtils::IsTableElement(aNode2)) {
    aNode2 = aNode2->GetParentNode();
  }

  return aNode1 != aNode2;
}


nsresult
nsHTMLEditRules::RemoveEmptyNodes()
{
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIEditor> kungFuDeathGrip(mHTMLEditor);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  nsCOMPtr<nsIContentIterator> iter = NS_NewContentIterator();

  nsresult res = iter->Init(mDocChangeRange);
  NS_ENSURE_SUCCESS(res, res);

  nsTArray<OwningNonNull<nsINode>> arrayOfEmptyNodes, arrayOfEmptyCites, skipList;

  
  while (!iter->IsDone()) {
    OwningNonNull<nsINode> node = *iter->GetCurrentNode();

    nsCOMPtr<nsINode> parent = node->GetParentNode();

    size_t idx = skipList.IndexOf(node);
    if (idx != skipList.NoIndex) {
      
      
      skipList[idx] = parent;
    } else {
      bool bIsCandidate = false;
      bool bIsEmptyNode = false;
      bool bIsMailCite = false;

      if (node->IsElement()) {
        if (node->IsHTMLElement(nsGkAtoms::body)) {
          
        } else if ((bIsMailCite = nsHTMLEditUtils::IsMailCite(node)) ||
                   node->IsHTMLElement(nsGkAtoms::a) ||
                   nsHTMLEditUtils::IsInlineStyle(node) ||
                   nsHTMLEditUtils::IsList(node) ||
                   node->IsHTMLElement(nsGkAtoms::div)) {
          
          bIsCandidate = true;
        } else if (nsHTMLEditUtils::IsFormatNode(node) ||
                   nsHTMLEditUtils::IsListItem(node) ||
                   node->IsHTMLElement(nsGkAtoms::blockquote)) {
          
          
          
          bool bIsSelInNode;
          res = SelectionEndpointInNode(node, &bIsSelInNode);
          NS_ENSURE_SUCCESS(res, res);
          if (!bIsSelInNode) {
            bIsCandidate = true;
          }
        }
      }

      if (bIsCandidate) {
        
        
        res = mHTMLEditor->IsEmptyNode(node->AsDOMNode(), &bIsEmptyNode,
                                       bIsMailCite, true);
        NS_ENSURE_SUCCESS(res, res);
        if (bIsEmptyNode) {
          if (bIsMailCite) {
            
            arrayOfEmptyCites.AppendElement(*node);
          } else {
            arrayOfEmptyNodes.AppendElement(*node);
          }
        }
      }

      if (!bIsEmptyNode) {
        
        skipList.AppendElement(*parent);
      }
    }

    iter->Next();
  }

  
  for (auto& delNode : arrayOfEmptyNodes) {
    if (mHTMLEditor->IsModifiableNode(delNode)) {
      res = mHTMLEditor->DeleteNode(delNode);
      NS_ENSURE_SUCCESS(res, res);
    }
  }

  
  
  for (auto& delNode : arrayOfEmptyCites) {
    bool bIsEmptyNode;
    res = mHTMLEditor->IsEmptyNode(delNode, &bIsEmptyNode, false, true);
    NS_ENSURE_SUCCESS(res, res);
    if (!bIsEmptyNode) {
      
      
      nsCOMPtr<nsINode> parent = delNode->GetParentNode();
      int32_t offset = parent ? parent->IndexOf(delNode) : -1;
      nsCOMPtr<Element> br = mHTMLEditor->CreateBR(parent, offset);
      NS_ENSURE_STATE(br);
    }
    res = mHTMLEditor->DeleteNode(delNode);
    NS_ENSURE_SUCCESS(res, res);
  }

  return NS_OK;
}

nsresult
nsHTMLEditRules::SelectionEndpointInNode(nsINode* aNode, bool* aResult)
{
  NS_ENSURE_TRUE(aNode && aResult, NS_ERROR_NULL_POINTER);

  nsIDOMNode* node = aNode->AsDOMNode();
  
  *aResult = false;
  
  NS_ENSURE_STATE(mHTMLEditor);
  nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();
  NS_ENSURE_STATE(selection);
  
  uint32_t rangeCount = selection->RangeCount();
  for (uint32_t rangeIdx = 0; rangeIdx < rangeCount; ++rangeIdx) {
    nsRefPtr<nsRange> range = selection->GetRangeAt(rangeIdx);
    nsCOMPtr<nsIDOMNode> startParent, endParent;
    range->GetStartContainer(getter_AddRefs(startParent));
    if (startParent)
    {
      if (node == startParent) {
        *aResult = true;
        return NS_OK;
      }
      if (nsEditorUtils::IsDescendantOf(startParent, node)) {
        *aResult = true;
        return NS_OK;
      }
    }
    range->GetEndContainer(getter_AddRefs(endParent));
    if (startParent == endParent) continue;
    if (endParent)
    {
      if (node == endParent) {
        *aResult = true;
        return NS_OK;
      }
      if (nsEditorUtils::IsDescendantOf(endParent, node)) {
        *aResult = true;
        return NS_OK;
      }
    }
  }
  return NS_OK;
}





bool 
nsHTMLEditRules::IsEmptyInline(nsIDOMNode *aNode)
{
  if (aNode && IsInlineNode(aNode) && mHTMLEditor &&
      mHTMLEditor->IsContainer(aNode)) 
  {
    bool bEmpty;
    NS_ENSURE_TRUE(mHTMLEditor, false);
    mHTMLEditor->IsEmptyNode(aNode, &bEmpty);
    return bEmpty;
  }
  return false;
}


bool
nsHTMLEditRules::ListIsEmptyLine(nsTArray<OwningNonNull<nsINode>>& aArrayOfNodes)
{
  
  
  
  NS_ENSURE_TRUE(aArrayOfNodes.Length(), true);

  NS_ENSURE_TRUE(mHTMLEditor, false);
  nsCOMPtr<nsIEditor> kungFuDeathGrip(mHTMLEditor);

  int32_t brCount = 0;

  for (auto& node : aArrayOfNodes) {
    if (!mHTMLEditor->IsEditable(node)) {
      continue;
    }
    if (nsTextEditUtils::IsBreak(node)) {
      
      if (brCount) {
        return false;
      }
      brCount++;
    } else if (IsEmptyInline(GetAsDOMNode(node))) {
      
    } else {
      return false;
    }
  }
  return true;
}


nsresult 
nsHTMLEditRules::PopListItem(nsIDOMNode *aListItem, bool *aOutOfList)
{
  nsCOMPtr<Element> listItem = do_QueryInterface(aListItem);
  
  NS_ENSURE_TRUE(listItem && aOutOfList, NS_ERROR_NULL_POINTER);
  
  
  *aOutOfList = false;
  
  nsCOMPtr<nsINode> curParent = listItem->GetParentNode();
  int32_t offset = curParent ? curParent->IndexOf(listItem) : -1;
    
  if (!nsHTMLEditUtils::IsListItem(listItem)) {
    return NS_ERROR_FAILURE;
  }
    
  
  
  nsCOMPtr<nsINode> curParPar = curParent->GetParentNode();
  int32_t parOffset = curParPar ? curParPar->IndexOf(curParent) : -1;
  
  bool bIsFirstListItem;
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->IsFirstEditableChild(aListItem,
                                                   &bIsFirstListItem);
  NS_ENSURE_SUCCESS(res, res);

  bool bIsLastListItem;
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->IsLastEditableChild(aListItem, &bIsLastListItem);
  NS_ENSURE_SUCCESS(res, res);
    
  if (!bIsFirstListItem && !bIsLastListItem)
  {
    
    nsCOMPtr<nsIDOMNode> newBlock;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->SplitNode(GetAsDOMNode(curParent), offset,
                                 getter_AddRefs(newBlock));
    NS_ENSURE_SUCCESS(res, res);
  }
  
  if (!bIsFirstListItem) parOffset++;
  
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->MoveNode(listItem, curParPar, parOffset);
  NS_ENSURE_SUCCESS(res, res);
    
  
  if (!nsHTMLEditUtils::IsList(curParPar) &&
      nsHTMLEditUtils::IsListItem(listItem)) {
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->RemoveBlockContainer(GetAsDOMNode(listItem));
    NS_ENSURE_SUCCESS(res, res);
    *aOutOfList = true;
  }
  return res;
}

nsresult
nsHTMLEditRules::RemoveListStructure(nsIDOMNode *aList)
{
  NS_ENSURE_ARG_POINTER(aList);

  nsresult res;

  nsCOMPtr<nsIDOMNode> child;
  aList->GetFirstChild(getter_AddRefs(child));

  while (child)
  {
    if (nsHTMLEditUtils::IsListItem(child))
    {
      bool bOutOfList;
      do
      {
        res = PopListItem(child, &bOutOfList);
        NS_ENSURE_SUCCESS(res, res);
      } while (!bOutOfList);   
    }
    else if (nsHTMLEditUtils::IsList(child))
    {
      res = RemoveListStructure(child);
      NS_ENSURE_SUCCESS(res, res);
    }
    else
    {
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(child);
      NS_ENSURE_SUCCESS(res, res);
    }
    aList->GetFirstChild(getter_AddRefs(child));
  }
  
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->RemoveBlockContainer(aList);
  NS_ENSURE_SUCCESS(res, res);

  return res;
}


nsresult 
nsHTMLEditRules::ConfirmSelectionInBody()
{
  
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIDOMElement> rootElement = do_QueryInterface(mHTMLEditor->GetRoot());
  NS_ENSURE_TRUE(rootElement, NS_ERROR_UNEXPECTED);

  
  NS_ENSURE_STATE(mHTMLEditor);
  nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();
  NS_ENSURE_STATE(selection);
  
  
  nsCOMPtr<nsIDOMNode> selNode, temp, parent;
  int32_t selOffset;
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(selection,
                                                    getter_AddRefs(selNode),
                                                    &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  temp = selNode;
  
  
  while (temp && !nsTextEditUtils::IsBody(temp))
  {
    res = temp->GetParentNode(getter_AddRefs(parent));
    temp = parent;
  }
  
  
  if (!temp) 
  {


    selection->Collapse(rootElement, 0);
  }
  
  
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->GetEndNodeAndOffset(selection, getter_AddRefs(selNode), &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  temp = selNode;
  
  
  while (temp && !nsTextEditUtils::IsBody(temp))
  {
    res = temp->GetParentNode(getter_AddRefs(parent));
    temp = parent;
  }
  
  
  if (!temp) 
  {


    selection->Collapse(rootElement, 0);
  }
  
  return res;
}


nsresult 
nsHTMLEditRules::UpdateDocChangeRange(nsRange* aRange)
{
  nsresult res = NS_OK;

  
  
  
  
  nsCOMPtr<nsIDOMNode> startNode;
  res = aRange->GetStartContainer(getter_AddRefs(startNode));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_STATE(mHTMLEditor);
  if (!mHTMLEditor->IsDescendantOfRoot(startNode)) {
    
    return NS_OK;
  }
  
  if (!mDocChangeRange)
  {
    
    mDocChangeRange = aRange->CloneRange();
  }
  else
  {
    int16_t result;
    
    
    res = mDocChangeRange->CompareBoundaryPoints(nsIDOMRange::START_TO_START, aRange, &result);
    if (res == NS_ERROR_NOT_INITIALIZED) {
      
      
      
      
      result = 1;
      res = NS_OK;
    }
    NS_ENSURE_SUCCESS(res, res);
    if (result > 0)  
    {
      int32_t startOffset;
      res = aRange->GetStartOffset(&startOffset);
      NS_ENSURE_SUCCESS(res, res);
      res = mDocChangeRange->SetStart(startNode, startOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
    
    
    res = mDocChangeRange->CompareBoundaryPoints(nsIDOMRange::END_TO_END, aRange, &result);
    NS_ENSURE_SUCCESS(res, res);
    if (result < 0)  
    {
      nsCOMPtr<nsIDOMNode> endNode;
      int32_t endOffset;
      res = aRange->GetEndContainer(getter_AddRefs(endNode));
      NS_ENSURE_SUCCESS(res, res);
      res = aRange->GetEndOffset(&endOffset);
      NS_ENSURE_SUCCESS(res, res);
      res = mDocChangeRange->SetEnd(endNode, endOffset);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return res;
}

nsresult 
nsHTMLEditRules::InsertMozBRIfNeeded(nsIDOMNode *aNode)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);
  if (!IsBlockNode(aNode)) return NS_OK;
  
  bool isEmpty;
  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->IsEmptyNode(aNode, &isEmpty);
  NS_ENSURE_SUCCESS(res, res);
  if (!isEmpty) {
    return NS_OK;
  }

  return CreateMozBR(aNode, 0);
}

NS_IMETHODIMP 
nsHTMLEditRules::WillCreateNode(const nsAString& aTag, nsIDOMNode *aParent, int32_t aPosition)
{
  return NS_OK;  
}

NS_IMETHODIMP 
nsHTMLEditRules::DidCreateNode(const nsAString& aTag, 
                               nsIDOMNode *aNode, 
                               nsIDOMNode *aParent, 
                               int32_t aPosition, 
                               nsresult aResult)
{
  if (!mListenerEnabled) {
    return NS_OK;
  }
  
  nsresult res = mUtilRange->SelectNode(aNode);
  NS_ENSURE_SUCCESS(res, res);
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillInsertNode(nsIDOMNode *aNode, nsIDOMNode *aParent, int32_t aPosition)
{
  return NS_OK;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidInsertNode(nsIDOMNode *aNode, 
                               nsIDOMNode *aParent, 
                               int32_t aPosition, 
                               nsresult aResult)
{
  if (!mListenerEnabled) {
    return NS_OK;
  }
  nsresult res = mUtilRange->SelectNode(aNode);
  NS_ENSURE_SUCCESS(res, res);
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillDeleteNode(nsIDOMNode *aChild)
{
  if (!mListenerEnabled) {
    return NS_OK;
  }
  nsresult res = mUtilRange->SelectNode(aChild);
  NS_ENSURE_SUCCESS(res, res);
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidDeleteNode(nsIDOMNode *aChild, nsresult aResult)
{
  return NS_OK;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillSplitNode(nsIDOMNode *aExistingRightNode, int32_t aOffset)
{
  return NS_OK;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidSplitNode(nsIDOMNode *aExistingRightNode, 
                              int32_t aOffset, 
                              nsIDOMNode *aNewLeftNode, 
                              nsresult aResult)
{
  if (!mListenerEnabled) {
    return NS_OK;
  }
  nsresult res = mUtilRange->SetStart(aNewLeftNode, 0);
  NS_ENSURE_SUCCESS(res, res);
  res = mUtilRange->SetEnd(aExistingRightNode, 0);
  NS_ENSURE_SUCCESS(res, res);
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillJoinNodes(nsIDOMNode *aLeftNode, nsIDOMNode *aRightNode, nsIDOMNode *aParent)
{
  if (!mListenerEnabled) {
    return NS_OK;
  }
  
  nsresult res = nsEditor::GetLengthOfDOMNode(aLeftNode, mJoinOffset);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidJoinNodes(nsIDOMNode  *aLeftNode, 
                              nsIDOMNode *aRightNode, 
                              nsIDOMNode *aParent, 
                              nsresult aResult)
{
  if (!mListenerEnabled) {
    return NS_OK;
  }
  
  nsresult res = mUtilRange->SetStart(aRightNode, mJoinOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = mUtilRange->SetEnd(aRightNode, mJoinOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillInsertText(nsIDOMCharacterData *aTextNode, int32_t aOffset, const nsAString &aString)
{
  return NS_OK;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidInsertText(nsIDOMCharacterData *aTextNode, 
                                  int32_t aOffset, 
                                  const nsAString &aString, 
                                  nsresult aResult)
{
  if (!mListenerEnabled) {
    return NS_OK;
  }
  int32_t length = aString.Length();
  nsCOMPtr<nsIDOMNode> theNode = do_QueryInterface(aTextNode);
  nsresult res = mUtilRange->SetStart(theNode, aOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = mUtilRange->SetEnd(theNode, aOffset+length);
  NS_ENSURE_SUCCESS(res, res);
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}


NS_IMETHODIMP 
nsHTMLEditRules::WillDeleteText(nsIDOMCharacterData *aTextNode, int32_t aOffset, int32_t aLength)
{
  return NS_OK;  
}


NS_IMETHODIMP 
nsHTMLEditRules::DidDeleteText(nsIDOMCharacterData *aTextNode, 
                                  int32_t aOffset, 
                                  int32_t aLength, 
                                  nsresult aResult)
{
  if (!mListenerEnabled) {
    return NS_OK;
  }
  nsCOMPtr<nsIDOMNode> theNode = do_QueryInterface(aTextNode);
  nsresult res = mUtilRange->SetStart(theNode, aOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = mUtilRange->SetEnd(theNode, aOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}

NS_IMETHODIMP
nsHTMLEditRules::WillDeleteSelection(nsISelection* aSelection)
{
  if (!mListenerEnabled) {
    return NS_OK;
  }
  nsRefPtr<Selection> selection = static_cast<Selection*>(aSelection);
  
  nsCOMPtr<nsIDOMNode> selNode;
  int32_t selOffset;

  NS_ENSURE_STATE(mHTMLEditor);
  nsresult res = mHTMLEditor->GetStartNodeAndOffset(selection,
                                                    getter_AddRefs(selNode),
                                                    &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = mUtilRange->SetStart(selNode, selOffset);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->GetEndNodeAndOffset(selection, getter_AddRefs(selNode),
                                         &selOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = mUtilRange->SetEnd(selNode, selOffset);
  NS_ENSURE_SUCCESS(res, res);
  res = UpdateDocChangeRange(mUtilRange);
  return res;  
}

NS_IMETHODIMP
nsHTMLEditRules::DidDeleteSelection(nsISelection *aSelection)
{
  return NS_OK;
}





nsresult
nsHTMLEditRules::RemoveAlignment(nsIDOMNode * aNode, const nsAString & aAlignType, bool aChildrenOnly)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  NS_ENSURE_STATE(mHTMLEditor);
  if (mHTMLEditor->IsTextNode(aNode) || nsHTMLEditUtils::IsTable(aNode)) return NS_OK;
  nsresult res = NS_OK;

  nsCOMPtr<nsIDOMNode> child = aNode,tmp;
  if (aChildrenOnly)
  {
    aNode->GetFirstChild(getter_AddRefs(child));
  }
  NS_ENSURE_STATE(mHTMLEditor);
  bool useCSS = mHTMLEditor->IsCSSEnabled();

  while (child)
  {
    if (aChildrenOnly) {
      
      child->GetNextSibling(getter_AddRefs(tmp));
    }
    else
    {
      tmp = nullptr;
    }
    bool isBlock;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->NodeIsBlockStatic(child, &isBlock);
    NS_ENSURE_SUCCESS(res, res);

    if (nsEditor::NodeIsType(child, nsGkAtoms::center)) {
      
      
      res = RemoveAlignment(child, aAlignType, true);
      NS_ENSURE_SUCCESS(res, res);

      
      
      res = MakeSureElemStartsOrEndsOnCR(child);
      NS_ENSURE_SUCCESS(res, res);

      
      NS_ENSURE_STATE(mHTMLEditor);
      nsCOMPtr<Element> childAsElement = do_QueryInterface(child);
      NS_ENSURE_STATE(childAsElement);
      res = mHTMLEditor->RemoveContainer(childAsElement);
      NS_ENSURE_SUCCESS(res, res);
    }
    else if (isBlock || nsHTMLEditUtils::IsHR(child))
    {
      
      nsCOMPtr<nsIDOMElement> curElem = do_QueryInterface(child);
      if (nsHTMLEditUtils::SupportsAlignAttr(child))
      {
        
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->RemoveAttribute(curElem, NS_LITERAL_STRING("align"));
        NS_ENSURE_SUCCESS(res, res);
      }
      if (useCSS)
      {
        if (nsHTMLEditUtils::IsTable(child) || nsHTMLEditUtils::IsHR(child))
        {
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->SetAttributeOrEquivalent(curElem, NS_LITERAL_STRING("align"), aAlignType, false); 
        }
        else
        {
          nsAutoString dummyCssValue;
          NS_ENSURE_STATE(mHTMLEditor);
          res = mHTMLEditor->mHTMLCSSUtils->RemoveCSSInlineStyle(child,
            nsGkAtoms::textAlign, dummyCssValue);
        }
        NS_ENSURE_SUCCESS(res, res);
      }
      if (!nsHTMLEditUtils::IsTable(child))
      {
        
        res = RemoveAlignment(child, aAlignType, true);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
    child = tmp;
  }
  return NS_OK;
}




nsresult
nsHTMLEditRules::MakeSureElemStartsOrEndsOnCR(nsIDOMNode *aNode, bool aStarts)
{
  nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
  NS_ENSURE_TRUE(node, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> child;
  nsresult res;
  if (aStarts)
  {
    NS_ENSURE_STATE(mHTMLEditor);
    child = GetAsDOMNode(mHTMLEditor->GetFirstEditableChild(*node));
  }
  else
  {
    NS_ENSURE_STATE(mHTMLEditor);
    child = GetAsDOMNode(mHTMLEditor->GetLastEditableChild(*node));
  }
  NS_ENSURE_TRUE(child, NS_OK);
  bool isChildBlock;
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->NodeIsBlockStatic(child, &isChildBlock);
  NS_ENSURE_SUCCESS(res, res);
  bool foundCR = false;
  if (isChildBlock || nsTextEditUtils::IsBreak(child))
  {
    foundCR = true;
  }
  else
  {
    nsCOMPtr<nsIDOMNode> sibling;
    if (aStarts)
    {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetPriorHTMLSibling(aNode, address_of(sibling));
    }
    else
    {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->GetNextHTMLSibling(aNode, address_of(sibling));
    }
    NS_ENSURE_SUCCESS(res, res);
    if (sibling)
    {
      bool isBlock;
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->NodeIsBlockStatic(sibling, &isBlock);
      NS_ENSURE_SUCCESS(res, res);
      if (isBlock || nsTextEditUtils::IsBreak(sibling))
      {
        foundCR = true;
      }
    }
    else
    {
      foundCR = true;
    }
  }
  if (!foundCR)
  {
    int32_t offset = 0;
    if (!aStarts) {
      nsCOMPtr<nsINode> node = do_QueryInterface(aNode);
      NS_ENSURE_STATE(node);
      offset = node->GetChildCount();
    }
    nsCOMPtr<nsIDOMNode> brNode;
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->CreateBR(aNode, offset, address_of(brNode));
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}

nsresult
nsHTMLEditRules::MakeSureElemStartsOrEndsOnCR(nsIDOMNode *aNode)
{
  nsresult res = MakeSureElemStartsOrEndsOnCR(aNode, false);
  NS_ENSURE_SUCCESS(res, res);
  res = MakeSureElemStartsOrEndsOnCR(aNode, true);
  return res;
}

nsresult
nsHTMLEditRules::AlignBlock(nsIDOMElement * aElement, const nsAString * aAlignType, bool aContentsOnly)
{
  NS_ENSURE_TRUE(aElement, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
  bool isBlock = IsBlockNode(node);
  if (!isBlock && !nsHTMLEditUtils::IsHR(node)) {
    
    return NS_OK;
  }

  nsresult res = RemoveAlignment(node, *aAlignType, aContentsOnly);
  NS_ENSURE_SUCCESS(res, res);
  NS_NAMED_LITERAL_STRING(attr, "align");
  NS_ENSURE_STATE(mHTMLEditor);
  if (mHTMLEditor->IsCSSEnabled()) {
    
    
    NS_ENSURE_STATE(mHTMLEditor);
    res = mHTMLEditor->SetAttributeOrEquivalent(aElement, attr, *aAlignType, false); 
    NS_ENSURE_SUCCESS(res, res);
  }
  else {
    
    
    if (nsHTMLEditUtils::SupportsAlignAttr(node)) {
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->SetAttribute(aElement, attr, *aAlignType);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}

nsresult
nsHTMLEditRules::RelativeChangeIndentationOfElementNode(nsIDOMNode *aNode, int8_t aRelativeChange)
{
  NS_ENSURE_ARG_POINTER(aNode);

  if (aRelativeChange != 1 && aRelativeChange != -1) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsCOMPtr<Element> element = do_QueryInterface(aNode);
  if (!element) {
    return NS_OK;
  }

  NS_ENSURE_STATE(mHTMLEditor);
  nsIAtom* marginProperty =
    MarginPropertyAtomForIndent(mHTMLEditor->mHTMLCSSUtils,
                                GetAsDOMNode(element));
  nsAutoString value;
  NS_ENSURE_STATE(mHTMLEditor);
  mHTMLEditor->mHTMLCSSUtils->GetSpecifiedProperty(*element, *marginProperty,
                                                   value);
  float f;
  nsCOMPtr<nsIAtom> unit;
  NS_ENSURE_STATE(mHTMLEditor);
  mHTMLEditor->mHTMLCSSUtils->ParseLength(value, &f, getter_AddRefs(unit));
  if (0 == f) {
    nsAutoString defaultLengthUnit;
    NS_ENSURE_STATE(mHTMLEditor);
    mHTMLEditor->mHTMLCSSUtils->GetDefaultLengthUnit(defaultLengthUnit);
    unit = do_GetAtom(defaultLengthUnit);
  }
  if        (nsGkAtoms::in == unit) {
            f += NS_EDITOR_INDENT_INCREMENT_IN * aRelativeChange;
  } else if (nsGkAtoms::cm == unit) {
            f += NS_EDITOR_INDENT_INCREMENT_CM * aRelativeChange;
  } else if (nsGkAtoms::mm == unit) {
            f += NS_EDITOR_INDENT_INCREMENT_MM * aRelativeChange;
  } else if (nsGkAtoms::pt == unit) {
            f += NS_EDITOR_INDENT_INCREMENT_PT * aRelativeChange;
  } else if (nsGkAtoms::pc == unit) {
            f += NS_EDITOR_INDENT_INCREMENT_PC * aRelativeChange;
  } else if (nsGkAtoms::em == unit) {
            f += NS_EDITOR_INDENT_INCREMENT_EM * aRelativeChange;
  } else if (nsGkAtoms::ex == unit) {
            f += NS_EDITOR_INDENT_INCREMENT_EX * aRelativeChange;
  } else if (nsGkAtoms::px == unit) {
            f += NS_EDITOR_INDENT_INCREMENT_PX * aRelativeChange;
  } else if (nsGkAtoms::percentage == unit) {
            f += NS_EDITOR_INDENT_INCREMENT_PERCENT * aRelativeChange;    
  }

  if (0 < f) {
    nsAutoString newValue;
    newValue.AppendFloat(f);
    newValue.Append(nsDependentAtomString(unit));
    NS_ENSURE_STATE(mHTMLEditor);
    mHTMLEditor->mHTMLCSSUtils->SetCSSProperty(*element, *marginProperty,
                                               newValue);
    return NS_OK;
  }

  NS_ENSURE_STATE(mHTMLEditor);
  mHTMLEditor->mHTMLCSSUtils->RemoveCSSProperty(*element, *marginProperty,
                                                value);

  
  
  
  
  
  nsCOMPtr<dom::Element> node = do_QueryInterface(aNode);
  if (!node || !node->IsHTMLElement(nsGkAtoms::div) ||
      !mHTMLEditor ||
      node == mHTMLEditor->GetActiveEditingHost() ||
      !mHTMLEditor->IsDescendantOfEditorRoot(node) ||
      nsHTMLEditor::HasAttributes(node)) {
    NS_ENSURE_STATE(mHTMLEditor);
    return NS_OK;
  }

  NS_ENSURE_STATE(mHTMLEditor);
  return mHTMLEditor->RemoveContainer(node);
}





nsresult
nsHTMLEditRules::WillAbsolutePosition(Selection* aSelection,
                                      bool* aCancel, bool* aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  nsresult res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);

  
  
  *aCancel = false;
  *aHandled = true;
  
  nsCOMPtr<nsIDOMElement> focusElement;
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->GetSelectionContainer(getter_AddRefs(focusElement));
  if (focusElement) {
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(focusElement);
    if (nsHTMLEditUtils::IsImage(node)) {
      mNewBlock = node;
      return NS_OK;
    }
  }

  res = NormalizeSelection(aSelection);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_STATE(mHTMLEditor);
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);
  
  
  
  
  
  
  nsTArray<nsRefPtr<nsRange>> arrayOfRanges;
  GetPromotedRanges(*aSelection, arrayOfRanges,
                    EditAction::setAbsolutePosition);
  
  
  nsTArray<OwningNonNull<nsINode>> arrayOfNodes;
  res = GetNodesForOperation(arrayOfRanges, arrayOfNodes,
                             EditAction::setAbsolutePosition);
  NS_ENSURE_SUCCESS(res, res);                                 
                                     
  
  if (ListIsEmptyLine(arrayOfNodes))
  {
    
    NS_ENSURE_STATE(aSelection->RangeCount());
    nsCOMPtr<nsINode> parent = aSelection->GetRangeAt(0)->GetStartParent();
    int32_t offset = aSelection->GetRangeAt(0)->StartOffset();
    NS_ENSURE_STATE(parent);

    
    res = SplitAsNeeded(*nsGkAtoms::div, parent, offset);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_STATE(mHTMLEditor);
    nsCOMPtr<Element> thePositionedDiv =
      mHTMLEditor->CreateNode(nsGkAtoms::div, parent, offset);
    NS_ENSURE_STATE(thePositionedDiv);
    
    mNewBlock = thePositionedDiv->AsDOMNode();
    
    while (!arrayOfNodes.IsEmpty()) {
      OwningNonNull<nsINode> curNode = arrayOfNodes[0];
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->DeleteNode(curNode);
      NS_ENSURE_SUCCESS(res, res);
      arrayOfNodes.RemoveElementAt(0);
    }
    
    res = aSelection->Collapse(thePositionedDiv,0);
    selectionResetter.Abort();  
    *aHandled = true;
    return res;
  }

  
  
  nsCOMPtr<nsINode> curParent;
  nsCOMPtr<nsIDOMNode> indentedLI, sibling;
  nsCOMPtr<Element> curList, curPositionedDiv;
  for (uint32_t i = 0; i < arrayOfNodes.Length(); i++) {
    
    NS_ENSURE_STATE(arrayOfNodes[i]->IsContent());
    nsCOMPtr<nsIContent> curNode = arrayOfNodes[i]->AsContent();

    
    NS_ENSURE_STATE(mHTMLEditor);
    if (!mHTMLEditor->IsEditable(curNode)) continue;

    curParent = curNode->GetParentNode();
    int32_t offset = curParent ? curParent->IndexOf(curNode) : -1;
     
    
    if (nsHTMLEditUtils::IsList(curParent))
    {
      
      
      if (curList)
      {
        NS_ENSURE_STATE(mHTMLEditor);
        sibling = GetAsDOMNode(mHTMLEditor->GetPriorHTMLSibling(curNode));
      }
      
      if (!curList || (sibling && sibling != GetAsDOMNode(curList))) {
        
        res = SplitAsNeeded(*curParent->NodeInfo()->NameAtom(), curParent,
                            offset);
        NS_ENSURE_SUCCESS(res, res);
        if (!curPositionedDiv) {
          nsCOMPtr<nsINode> curParentParent = curParent->GetParentNode();
          int32_t parentOffset = curParentParent
            ? curParentParent->IndexOf(curParent) : -1;
          NS_ENSURE_STATE(mHTMLEditor);
          curPositionedDiv = mHTMLEditor->CreateNode(nsGkAtoms::div, curParentParent,
                                                     parentOffset);
          mNewBlock = GetAsDOMNode(curPositionedDiv);
        }
        NS_ENSURE_STATE(mHTMLEditor);
        curList = mHTMLEditor->CreateNode(curParent->NodeInfo()->NameAtom(),
                                          curPositionedDiv, -1);
        NS_ENSURE_STATE(curList);
        
        
        
      }
      
      NS_ENSURE_STATE(mHTMLEditor);
      res = mHTMLEditor->MoveNode(curNode, curList, -1);
      NS_ENSURE_SUCCESS(res, res);
      
      
    }
    
    else 
    {
      
      
      
      
      
      
      nsCOMPtr<Element> listItem = IsInListItem(curNode);
      if (listItem) {
        if (indentedLI == GetAsDOMNode(listItem)) {
          
          continue;
        }
        curParent = listItem->GetParentNode();
        offset = curParent ? curParent->IndexOf(listItem) : -1;
        
        
        if (curList)
        {
          NS_ENSURE_STATE(mHTMLEditor);
          sibling = GetAsDOMNode(mHTMLEditor->GetPriorHTMLSibling(curNode));
        }
         
        if (!curList || (sibling && sibling != GetAsDOMNode(curList))) {
          
          res = SplitAsNeeded(*curParent->NodeInfo()->NameAtom(), curParent,
                              offset);
          NS_ENSURE_SUCCESS(res, res);
          if (!curPositionedDiv) {
            nsCOMPtr<nsINode> curParentParent = curParent->GetParentNode();
            int32_t parentOffset = curParentParent ?
              curParentParent->IndexOf(curParent) : -1;
            NS_ENSURE_STATE(mHTMLEditor);
            curPositionedDiv = mHTMLEditor->CreateNode(nsGkAtoms::div,
                                                       curParentParent,
                                                       parentOffset);
            mNewBlock = GetAsDOMNode(curPositionedDiv);
          }
          NS_ENSURE_STATE(mHTMLEditor);
          curList = mHTMLEditor->CreateNode(curParent->NodeInfo()->NameAtom(),
                                            curPositionedDiv, -1);
          NS_ENSURE_STATE(curList);
        }
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(listItem, curList, -1);
        NS_ENSURE_SUCCESS(res, res);
        
        indentedLI = GetAsDOMNode(listItem);
      }
      
      else
      {
        

        if (!curPositionedDiv) 
        {
          if (curNode->IsHTMLElement(nsGkAtoms::div)) {
            curPositionedDiv = curNode->AsElement();
            mNewBlock = GetAsDOMNode(curPositionedDiv);
            curList = nullptr;
            continue;
          }
          res = SplitAsNeeded(*nsGkAtoms::div, curParent, offset);
          NS_ENSURE_SUCCESS(res, res);
          NS_ENSURE_STATE(mHTMLEditor);
          curPositionedDiv = mHTMLEditor->CreateNode(nsGkAtoms::div, curParent,
                                                     offset);
          NS_ENSURE_STATE(curPositionedDiv);
          
          mNewBlock = GetAsDOMNode(curPositionedDiv);
          
        }
          
        
        NS_ENSURE_STATE(mHTMLEditor);
        res = mHTMLEditor->MoveNode(curNode, curPositionedDiv, -1);
        NS_ENSURE_SUCCESS(res, res);
        
        curList = nullptr;
      }
    }
  }
  return res;
}

nsresult
nsHTMLEditRules::DidAbsolutePosition()
{
  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIHTMLAbsPosEditor> absPosHTMLEditor = mHTMLEditor;
  nsCOMPtr<nsIDOMElement> elt = do_QueryInterface(mNewBlock);
  return absPosHTMLEditor->AbsolutelyPositionElement(elt, true);
}

nsresult
nsHTMLEditRules::WillRemoveAbsolutePosition(Selection* aSelection,
                                            bool* aCancel, bool* aHandled) {
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  nsresult res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);

  
  
  *aCancel = false;
  *aHandled = true;

  nsCOMPtr<nsIDOMElement>  elt;
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->GetAbsolutelyPositionedSelectionContainer(getter_AddRefs(elt));
  NS_ENSURE_SUCCESS(res, res);

  NS_ENSURE_STATE(mHTMLEditor);
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);

  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIHTMLAbsPosEditor> absPosHTMLEditor = mHTMLEditor;
  return absPosHTMLEditor->AbsolutelyPositionElement(elt, false);
}

nsresult
nsHTMLEditRules::WillRelativeChangeZIndex(Selection* aSelection,
                                          int32_t aChange,
                                          bool *aCancel,
                                          bool * aHandled)
{
  if (!aSelection || !aCancel || !aHandled) { return NS_ERROR_NULL_POINTER; }
  nsresult res = WillInsert(aSelection, aCancel);
  NS_ENSURE_SUCCESS(res, res);

  
  
  *aCancel = false;
  *aHandled = true;

  nsCOMPtr<nsIDOMElement>  elt;
  NS_ENSURE_STATE(mHTMLEditor);
  res = mHTMLEditor->GetAbsolutelyPositionedSelectionContainer(getter_AddRefs(elt));
  NS_ENSURE_SUCCESS(res, res);

  NS_ENSURE_STATE(mHTMLEditor);
  nsAutoSelectionReset selectionResetter(aSelection, mHTMLEditor);

  NS_ENSURE_STATE(mHTMLEditor);
  nsCOMPtr<nsIHTMLAbsPosEditor> absPosHTMLEditor = mHTMLEditor;
  int32_t zIndex;
  return absPosHTMLEditor->RelativeChangeElementZIndex(elt, aChange, &zIndex);
}

NS_IMETHODIMP
nsHTMLEditRules::DocumentModified()
{
  nsContentUtils::AddScriptRunner(NS_NewRunnableMethod(this, &nsHTMLEditRules::DocumentModifiedWorker));
  return NS_OK;
}

void
nsHTMLEditRules::DocumentModifiedWorker()
{
  if (!mHTMLEditor) {
    return;
  }

  
  nsAutoScriptBlockerSuppressNodeRemoved scriptBlocker;

  nsCOMPtr<nsIHTMLEditor> kungFuDeathGrip(mHTMLEditor);
  nsRefPtr<Selection> selection = mHTMLEditor->GetSelection();
  if (!selection) {
    return;
  }

  
  
  if (mBogusNode) {
    mEditor->DeleteNode(mBogusNode);
    mBogusNode = nullptr;
  }

  
  CreateBogusNodeIfNeeded(selection);
}
