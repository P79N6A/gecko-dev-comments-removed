




#ifndef nsHTMLEditor_h__
#define nsHTMLEditor_h__

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsPlaintextEditor.h"
#include "nsIEditor.h"
#include "nsIHTMLEditor.h"
#include "nsITableEditor.h"
#include "nsIEditorMailSupport.h"
#include "nsIEditorStyleSheets.h"

#include "nsEditor.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventListener.h"
#include "nsICSSLoaderObserver.h"

#include "nsEditRules.h"

#include "nsHTMLCSSUtils.h"

#include "nsHTMLObjectResizer.h"
#include "nsIHTMLAbsPosEditor.h"
#include "nsIHTMLInlineTableEditor.h"
#include "nsIHTMLObjectResizeListener.h"
#include "nsIHTMLObjectResizer.h"

#include "nsIDocumentObserver.h"

#include "nsPoint.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"
#include "nsAttrName.h"
#include "nsStubMutationObserver.h"

#include "mozilla/Attributes.h"
#include "mozilla/dom/Element.h"

class nsIDOMKeyEvent;
class nsITransferable;
class nsIDocumentEncoder;
class nsIClipboard;
class TypeInState;
class nsIContentFilter;
class nsIURL;
class nsILinkHandler;
class nsTableOuterFrame;
struct PropItem;

namespace mozilla {
namespace widget {
struct IMEState;
} 
} 





class nsHTMLEditor MOZ_FINAL : public nsPlaintextEditor,
                               public nsIHTMLEditor,
                               public nsIHTMLObjectResizer,
                               public nsIHTMLAbsPosEditor,
                               public nsITableEditor,
                               public nsIHTMLInlineTableEditor,
                               public nsIEditorStyleSheets,
                               public nsICSSLoaderObserver,
                               public nsStubMutationObserver
{
  typedef enum {eNoOp, eReplaceParent=1, eInsertParent=2} BlockTransformationType;

public:

  enum ResizingRequestID
  {
    kX      = 0,
    kY      = 1,
    kWidth  = 2,
    kHeight = 3
  };

  




  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLEditor, nsPlaintextEditor)


  nsHTMLEditor();

  bool GetReturnInParagraphCreatesNewParagraph();

  
  NS_IMETHOD GetIsDocumentEditable(bool *aIsDocumentEditable);
  NS_IMETHOD BeginningOfDocument();
  virtual nsresult HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent);
  virtual already_AddRefed<nsIContent> GetFocusedContent();
  virtual already_AddRefed<nsIContent> GetFocusedContentForIME();
  virtual bool IsActiveInDOMWindow();
  virtual already_AddRefed<mozilla::dom::EventTarget> GetDOMEventTarget();
  virtual mozilla::dom::Element* GetEditorRoot() MOZ_OVERRIDE;
  virtual already_AddRefed<nsIContent> FindSelectionRoot(nsINode *aNode);
  virtual bool IsAcceptableInputEvent(nsIDOMEvent* aEvent);
  virtual already_AddRefed<nsIContent> GetInputEventTargetContent();
  virtual bool IsEditable(nsINode* aNode) MOZ_OVERRIDE;
  using nsEditor::IsEditable;

  
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  
  NS_IMETHOD GetPreferredIMEState(mozilla::widget::IMEState *aState);

  

  NS_DECL_NSIHTMLEDITOR

  
  
  NS_DECL_NSIHTMLOBJECTRESIZER

  
  
  NS_DECL_NSIHTMLABSPOSEDITOR

  
  
  NS_DECL_NSIHTMLINLINETABLEEDITOR

  
  NS_IMETHOD CopyLastEditableChildStyles(nsIDOMNode *aPreviousBlock, nsIDOMNode *aNewBlock,
                                         nsIDOMNode **aOutBrNode);

  NS_IMETHOD LoadHTML(const nsAString &aInputString);

  nsresult GetCSSBackgroundColorState(bool *aMixed, nsAString &aOutColor,
                                      bool aBlockLevel);
  NS_IMETHOD GetHTMLBackgroundColorState(bool *aMixed, nsAString &outColor);

  

  NS_IMETHOD AddStyleSheet(const nsAString & aURL);
  NS_IMETHOD ReplaceStyleSheet(const nsAString& aURL);
  NS_IMETHOD RemoveStyleSheet(const nsAString &aURL);

  NS_IMETHOD AddOverrideStyleSheet(const nsAString & aURL);
  NS_IMETHOD ReplaceOverrideStyleSheet(const nsAString& aURL);
  NS_IMETHOD RemoveOverrideStyleSheet(const nsAString &aURL);

  NS_IMETHOD EnableStyleSheet(const nsAString& aURL, bool aEnable);

  

  NS_DECL_NSIEDITORMAILSUPPORT

  

  NS_IMETHOD InsertTableCell(int32_t aNumber, bool aAfter);
  NS_IMETHOD InsertTableColumn(int32_t aNumber, bool aAfter);
  NS_IMETHOD InsertTableRow(int32_t aNumber, bool aAfter);
  NS_IMETHOD DeleteTable();
  NS_IMETHOD DeleteTableCell(int32_t aNumber);
  NS_IMETHOD DeleteTableCellContents();
  NS_IMETHOD DeleteTableColumn(int32_t aNumber);
  NS_IMETHOD DeleteTableRow(int32_t aNumber);
  NS_IMETHOD SelectTableCell();
  NS_IMETHOD SelectBlockOfCells(nsIDOMElement *aStartCell, nsIDOMElement *aEndCell);
  NS_IMETHOD SelectTableRow();
  NS_IMETHOD SelectTableColumn();
  NS_IMETHOD SelectTable();
  NS_IMETHOD SelectAllTableCells();
  NS_IMETHOD SwitchTableCellHeaderType(nsIDOMElement *aSourceCell, nsIDOMElement **aNewCell);
  NS_IMETHOD JoinTableCells(bool aMergeNonContiguousContents);
  NS_IMETHOD SplitTableCell();
  NS_IMETHOD NormalizeTable(nsIDOMElement *aTable);
  NS_IMETHOD GetCellIndexes(nsIDOMElement *aCell,
                            int32_t* aRowIndex, int32_t* aColIndex);
  NS_IMETHOD GetTableSize(nsIDOMElement *aTable,
                          int32_t* aRowCount, int32_t* aColCount);
  NS_IMETHOD GetCellAt(nsIDOMElement* aTable, int32_t aRowIndex, int32_t aColIndex, nsIDOMElement **aCell);
  NS_IMETHOD GetCellDataAt(nsIDOMElement* aTable,
                           int32_t aRowIndex, int32_t aColIndex,
                           nsIDOMElement **aCell,
                           int32_t* aStartRowIndex, int32_t* aStartColIndex,
                           int32_t* aRowSpan, int32_t* aColSpan, 
                           int32_t* aActualRowSpan, int32_t* aActualColSpan, 
                           bool* aIsSelected);
  NS_IMETHOD GetFirstRow(nsIDOMElement* aTableElement, nsIDOMNode** aRowNode);
  NS_IMETHOD GetNextRow(nsIDOMNode* aCurrentRowNode, nsIDOMNode** aRowNode);
  NS_IMETHOD GetLastCellInRow(nsIDOMNode* aRowNode, nsIDOMNode** aCellNode);

  NS_IMETHOD SetSelectionAfterTableEdit(nsIDOMElement* aTable, int32_t aRow, int32_t aCol, 
                                        int32_t aDirection, bool aSelected);
  NS_IMETHOD GetSelectedOrParentTableElement(nsAString& aTagName,
                                             int32_t *aSelectedCount,
                                             nsIDOMElement** aTableElement);
  NS_IMETHOD GetSelectedCellsType(nsIDOMElement *aElement, uint32_t *aSelectionType);

  nsresult GetCellFromRange(nsIDOMRange *aRange, nsIDOMElement **aCell);

  
  
  
  
  
  NS_IMETHOD GetFirstSelectedCell(nsIDOMRange **aRange, nsIDOMElement **aCell);
  
  
  NS_IMETHOD GetNextSelectedCell(nsIDOMRange **aRange, nsIDOMElement **aCell);

  
  NS_IMETHOD GetFirstSelectedCellInTable(int32_t *aRowIndex, int32_t *aColIndex, nsIDOMElement **aCell);
    
  
  
  
  NS_IMETHOD SetCSSBackgroundColor(const nsAString& aColor);
  NS_IMETHOD SetHTMLBackgroundColor(const nsAString& aColor);

  
  static already_AddRefed<mozilla::dom::Element> GetBlockNodeParent(nsINode* aNode);
  static already_AddRefed<nsIDOMNode> GetBlockNodeParent(nsIDOMNode *aNode);

  void IsNextCharInNodeWhitespace(nsIContent* aContent,
                                  int32_t aOffset,
                                  bool* outIsSpace,
                                  bool* outIsNBSP,
                                  nsIContent** outNode = nullptr,
                                  int32_t* outOffset = 0);
  void IsPrevCharInNodeWhitespace(nsIContent* aContent,
                                  int32_t aOffset,
                                  bool* outIsSpace,
                                  bool* outIsNBSP,
                                  nsIContent** outNode = nullptr,
                                  int32_t* outOffset = 0);

  

  nsresult EndUpdateViewBatch();

  
  NS_IMETHOD Init(nsIDOMDocument *aDoc, nsIContent *aRoot,
                  nsISelectionController *aSelCon, uint32_t aFlags,
                  const nsAString& aValue);
  NS_IMETHOD PreDestroy(bool aDestroyingFrames);

  
  
  static bool NodeIsBlockStatic(const mozilla::dom::Element* aElement);
  static nsresult NodeIsBlockStatic(nsIDOMNode *aNode, bool *aIsBlock);
protected:
  virtual ~nsHTMLEditor();

  using nsEditor::IsBlockNode;
  virtual bool IsBlockNode(nsINode *aNode);

public:
  NS_IMETHOD SetFlags(uint32_t aFlags);

  NS_IMETHOD Paste(int32_t aSelectionType);
  NS_IMETHOD CanPaste(int32_t aSelectionType, bool *aCanPaste);

  NS_IMETHOD PasteTransferable(nsITransferable *aTransferable);
  NS_IMETHOD CanPasteTransferable(nsITransferable *aTransferable, bool *aCanPaste);

  NS_IMETHOD DebugUnitTests(int32_t *outNumTests, int32_t *outNumTestsFailed);

  

  NS_IMETHOD StartOperation(EditAction opID,
                            nsIEditor::EDirection aDirection);

  

  NS_IMETHOD EndOperation();

  
  virtual bool TagCanContainTag(nsIAtom* aParentTag, nsIAtom* aChildTag);
  
  
  virtual bool IsContainer(nsINode* aNode) MOZ_OVERRIDE;
  virtual bool IsContainer(nsIDOMNode* aNode) MOZ_OVERRIDE;

  
  NS_IMETHOD SelectEntireDocument(nsISelection *aSelection);

  NS_IMETHOD SetAttributeOrEquivalent(nsIDOMElement * aElement,
                                      const nsAString & aAttribute,
                                      const nsAString & aValue,
                                      bool aSuppressTransaction);
  NS_IMETHOD RemoveAttributeOrEquivalent(nsIDOMElement * aElement,
                                         const nsAString & aAttribute,
                                         bool aSuppressTransaction);

  
  NS_IMETHOD CollapseAdjacentTextNodes(nsIDOMRange *aInRange);

  virtual bool AreNodesSameType(nsIContent* aNode1, nsIContent* aNode2)
    MOZ_OVERRIDE;

  NS_IMETHOD DeleteSelectionImpl(EDirection aAction,
                                 EStripWrappers aStripWrappers);
  nsresult DeleteNode(nsINode* aNode);
  NS_IMETHODIMP DeleteNode(nsIDOMNode * aNode);
  nsresult DeleteText(nsGenericDOMDataNode& aTextNode, uint32_t aOffset,
                      uint32_t aLength);
  NS_IMETHOD InsertTextImpl(const nsAString& aStringToInsert, 
                            nsCOMPtr<nsIDOMNode> *aInOutNode, 
                            int32_t *aInOutOffset,
                            nsIDOMDocument *aDoc);
  NS_IMETHOD_(bool) IsModifiableNode(nsIDOMNode *aNode);
  virtual bool IsModifiableNode(nsINode *aNode);

  NS_IMETHOD GetIsSelectionEditable(bool* aIsSelectionEditable);

  NS_IMETHOD SelectAll();

  NS_IMETHOD GetRootElement(nsIDOMElement **aRootElement);

  
  NS_IMETHOD StyleSheetLoaded(mozilla::CSSStyleSheet* aSheet,
                              bool aWasAlternate, nsresult aStatus);

  
  NS_IMETHOD TypedText(const nsAString& aString, ETypingAction aAction);
  nsresult InsertNodeAtPoint( nsIDOMNode *aNode, 
                              nsCOMPtr<nsIDOMNode> *ioParent, 
                              int32_t *ioOffset, 
                              bool aNoEmptyNodes);

  
  
  
  
  
  
  void CollapseSelectionToDeepestNonTableFirstChild(
                          mozilla::dom::Selection* aSelection, nsINode* aNode);

  



  nsresult IsVisTextNode(nsIContent* aNode,
                         bool* outIsEmptyNode,
                         bool aSafeToAskFrames);
  nsresult IsEmptyNode(nsIDOMNode *aNode, bool *outIsEmptyBlock, 
                       bool aMozBRDoesntCount = false,
                       bool aListOrCellNotEmpty = false,
                       bool aSafeToAskFrames = false);
  nsresult IsEmptyNode(nsINode* aNode, bool* outIsEmptyBlock,
                       bool aMozBRDoesntCount = false,
                       bool aListOrCellNotEmpty = false,
                       bool aSafeToAskFrames = false);
  nsresult IsEmptyNodeImpl(nsINode* aNode,
                           bool *outIsEmptyBlock, 
                           bool aMozBRDoesntCount,
                           bool aListOrCellNotEmpty,
                           bool aSafeToAskFrames,
                           bool *aSeenBR);

  
  bool     EnableExistingStyleSheet(const nsAString& aURL);

  
  NS_IMETHOD GetStyleSheetForURL(const nsAString &aURL,
                                 mozilla::CSSStyleSheet** _retval);
  NS_IMETHOD GetURLForStyleSheet(mozilla::CSSStyleSheet* aStyleSheet,
                                 nsAString& aURL);

  
  nsresult AddNewStyleSheetToList(const nsAString &aURL,
                                  mozilla::CSSStyleSheet* aStyleSheet);

  nsresult RemoveStyleSheetFromList(const nsAString &aURL);

  bool IsCSSEnabled()
  {
    
    return mCSSAware && mHTMLCSSUtils && mHTMLCSSUtils->IsCSSPrefChecked();
  }

  static bool HasAttributes(mozilla::dom::Element* aElement)
  {
    MOZ_ASSERT(aElement);
    uint32_t attrCount = aElement->GetAttrCount();
    return attrCount > 1 ||
           (1 == attrCount && !aElement->GetAttrNameAt(0)->Equals(nsGkAtoms::mozdirty));
  }

protected:

  NS_IMETHOD  InitRules();

  
  virtual void CreateEventListeners();

  virtual nsresult InstallEventListeners();
  virtual void RemoveEventListeners();

  bool ShouldReplaceRootElement();
  void ResetRootElementAndEventTarget();
  nsresult GetBodyElement(nsIDOMHTMLElement** aBody);
  
  
  
  already_AddRefed<nsINode> GetFocusedNode();

  
  bool SetCaretInTableCell(nsIDOMElement* aElement);

  
  NS_IMETHOD TabInTable(bool inIsShift, bool *outHandled);
  already_AddRefed<mozilla::dom::Element> CreateBR(nsINode* aNode,
      int32_t aOffset, EDirection aSelect = eNone);
  NS_IMETHOD CreateBR(nsIDOMNode *aNode, int32_t aOffset, 
                      nsCOMPtr<nsIDOMNode> *outBRNode, nsIEditor::EDirection aSelect = nsIEditor::eNone);



  

  
  
  
  NS_IMETHOD InsertCell(nsIDOMElement *aCell, int32_t aRowSpan, int32_t aColSpan,
                        bool aAfter, bool aIsHeader, nsIDOMElement **aNewCell);

  
  NS_IMETHOD DeleteRow(nsIDOMElement *aTable, int32_t aRowIndex);
  NS_IMETHOD DeleteColumn(nsIDOMElement *aTable, int32_t aColIndex);
  NS_IMETHOD DeleteCellContents(nsIDOMElement *aCell);

  
  NS_IMETHOD MergeCells(nsCOMPtr<nsIDOMElement> aTargetCell, nsCOMPtr<nsIDOMElement> aCellToMerge, bool aDeleteCellToMerge);

  NS_IMETHOD DeleteTable2(nsIDOMElement *aTable, nsISelection *aSelection);
  NS_IMETHOD SetColSpan(nsIDOMElement *aCell, int32_t aColSpan);
  NS_IMETHOD SetRowSpan(nsIDOMElement *aCell, int32_t aRowSpan);

  
  nsTableOuterFrame* GetTableFrame(nsIDOMElement* aTable);
  
  
  int32_t  GetNumberOfCellsInRow(nsIDOMElement* aTable, int32_t rowIndex);
  
  bool AllCellsInRowSelected(nsIDOMElement *aTable, int32_t aRowIndex, int32_t aNumberOfColumns);
  bool AllCellsInColumnSelected(nsIDOMElement *aTable, int32_t aColIndex, int32_t aNumberOfRows);

  bool IsEmptyCell(mozilla::dom::Element* aCell);

  
  
  
  
  
  NS_IMETHOD GetCellContext(nsISelection **aSelection,
                            nsIDOMElement   **aTable,
                            nsIDOMElement   **aCell,
                            nsIDOMNode      **aCellParent, int32_t *aCellOffset,
                            int32_t *aRowIndex, int32_t *aColIndex);

  NS_IMETHOD GetCellSpansAt(nsIDOMElement* aTable, int32_t aRowIndex, int32_t aColIndex, 
                            int32_t& aActualRowSpan, int32_t& aActualColSpan);

  NS_IMETHOD SplitCellIntoColumns(nsIDOMElement *aTable, int32_t aRowIndex, int32_t aColIndex,
                                  int32_t aColSpanLeft, int32_t aColSpanRight, nsIDOMElement **aNewCell);

  NS_IMETHOD SplitCellIntoRows(nsIDOMElement *aTable, int32_t aRowIndex, int32_t aColIndex,
                               int32_t aRowSpanAbove, int32_t aRowSpanBelow, nsIDOMElement **aNewCell);

  nsresult CopyCellBackgroundColor(nsIDOMElement *destCell, nsIDOMElement *sourceCell);

  
  NS_IMETHOD FixBadRowSpan(nsIDOMElement *aTable, int32_t aRowIndex, int32_t& aNewRowCount);
  NS_IMETHOD FixBadColSpan(nsIDOMElement *aTable, int32_t aColIndex, int32_t& aNewColCount);

  
  
  NS_IMETHOD SetSelectionAtDocumentStart(nsISelection *aSelection);


  
  static already_AddRefed<mozilla::dom::Element>
    GetEnclosingTable(nsINode* aNode);
  static nsCOMPtr<nsIDOMNode> GetEnclosingTable(nsIDOMNode *aNode);

  















  bool IsTextPropertySetByContent(nsIContent*      aContent,
                                  nsIAtom*         aProperty,
                                  const nsAString* aAttribute,
                                  const nsAString* aValue,
                                  nsAString*       outValue = nullptr);

  void IsTextPropertySetByContent(nsIDOMNode*      aNode,
                                  nsIAtom*         aProperty,
                                  const nsAString* aAttribute,
                                  const nsAString* aValue,
                                  bool&            aIsSet,
                                  nsAString*       outValue = nullptr);

  
  NS_IMETHOD PasteAsPlaintextQuotation(int32_t aSelectionType);

  








  NS_IMETHOD InsertAsPlaintextQuotation(const nsAString & aQuotedText,
                                        bool aAddCites,
                                        nsIDOMNode **aNodeInserted);

  nsresult InsertObject(const char* aType, nsISupports* aObject, bool aIsSafe,
                        nsIDOMDocument *aSourceDoc,
                        nsIDOMNode *aDestinationNode,
                        int32_t aDestOffset,
                        bool aDoDeleteSelection);

  
  NS_IMETHOD PrepareTransferable(nsITransferable **transferable);
  NS_IMETHOD PrepareHTMLTransferable(nsITransferable **transferable, bool havePrivFlavor);
  NS_IMETHOD InsertFromTransferable(nsITransferable *transferable, 
                                    nsIDOMDocument *aSourceDoc,
                                    const nsAString & aContextStr,
                                    const nsAString & aInfoStr,
                                    nsIDOMNode *aDestinationNode,
                                    int32_t aDestinationOffset,
                                    bool aDoDeleteSelection);
  nsresult InsertFromDataTransfer(mozilla::dom::DataTransfer *aDataTransfer,
                                  int32_t aIndex,
                                  nsIDOMDocument *aSourceDoc,
                                  nsIDOMNode *aDestinationNode,
                                  int32_t aDestOffset,
                                  bool aDoDeleteSelection);
  bool HavePrivateHTMLFlavor( nsIClipboard *clipboard );
  nsresult   ParseCFHTML(nsCString & aCfhtml, char16_t **aStuffToPaste, char16_t **aCfcontext);
  nsresult   DoContentFilterCallback(const nsAString &aFlavor,
                                     nsIDOMDocument *aSourceDoc,
                                     bool aWillDeleteSelection,
                                     nsIDOMNode **aFragmentAsNode,      
                                     nsIDOMNode **aFragStartNode,
                                     int32_t *aFragStartOffset,
                                     nsIDOMNode **aFragEndNode,
                                     int32_t *aFragEndOffset,
                                     nsIDOMNode **aTargetNode,       
                                     int32_t *aTargetOffset,   
                                     bool *aDoContinue);

  bool       IsInLink(nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *outLink = nullptr);
  nsresult   StripFormattingNodes(nsIDOMNode *aNode, bool aOnlyList = false);
  nsresult   CreateDOMFragmentFromPaste(const nsAString & aInputString,
                                        const nsAString & aContextStr,
                                        const nsAString & aInfoStr,
                                        nsCOMPtr<nsIDOMNode> *outFragNode,
                                        nsCOMPtr<nsIDOMNode> *outStartNode,
                                        nsCOMPtr<nsIDOMNode> *outEndNode,
                                        int32_t *outStartOffset,
                                        int32_t *outEndOffset,
                                        bool aTrustedInput);
  nsresult   ParseFragment(const nsAString & aStr, nsIAtom* aContextLocalName,
                           nsIDocument* aTargetDoc,
                           nsCOMPtr<nsIDOMNode> *outNode,
                           bool aTrustedInput);
  nsresult   CreateListOfNodesToPaste(nsIDOMNode  *aFragmentAsNode,
                                      nsCOMArray<nsIDOMNode>& outNodeList,
                                      nsIDOMNode *aStartNode,
                                      int32_t aStartOffset,
                                      nsIDOMNode *aEndNode,
                                      int32_t aEndOffset);
  nsresult CreateTagStack(nsTArray<nsString> &aTagStack,
                          nsIDOMNode *aNode);
  nsresult GetListAndTableParents( bool aEnd, 
                                   nsCOMArray<nsIDOMNode>& aListOfNodes,
                                   nsCOMArray<nsIDOMNode>& outArray);
  nsresult DiscoverPartialListsAndTables(nsCOMArray<nsIDOMNode>& aPasteNodes,
                                         nsCOMArray<nsIDOMNode>& aListsAndTables,
                                         int32_t *outHighWaterMark);
  nsresult ScanForListAndTableStructure(bool aEnd,
                                        nsCOMArray<nsIDOMNode>& aNodes,
                                        nsIDOMNode *aListOrTable,
                                        nsCOMPtr<nsIDOMNode> *outReplaceNode);
  nsresult ReplaceOrphanedStructure( bool aEnd,
                                     nsCOMArray<nsIDOMNode>& aNodeArray,
                                     nsCOMArray<nsIDOMNode>& aListAndTableArray,
                                     int32_t aHighWaterMark);
  nsIDOMNode* GetArrayEndpoint(bool aEnd, nsCOMArray<nsIDOMNode>& aNodeArray);

  
  bool     IsVisBreak(nsINode* aNode);
  bool     IsVisBreak(nsIDOMNode *aNode);

  

  void NormalizeEOLInsertPosition(nsIDOMNode *firstNodeToInsert,
                                  nsCOMPtr<nsIDOMNode> *insertParentNode,
                                  int32_t *insertOffset);

  
  bool IsModifiable();

  
  nsresult MakeDefinitionItem(const nsAString & aItemType);
  nsresult InsertBasicBlock(const nsAString & aBlockType);
  
  
  nsresult RelativeFontChange( int32_t aSizeChange);
  
  
  nsresult RelativeFontChangeOnTextNode( int32_t aSizeChange, 
                                         nsIDOMCharacterData *aTextNode, 
                                         int32_t aStartOffset,
                                         int32_t aEndOffset);
  nsresult RelativeFontChangeOnNode(int32_t aSizeChange, nsIContent* aNode);
  nsresult RelativeFontChangeHelper(int32_t aSizeChange, nsINode* aNode);

  
  nsresult SetInlinePropertyOnTextNode( nsIDOMCharacterData *aTextNode, 
                                        int32_t aStartOffset,
                                        int32_t aEndOffset,
                                        nsIAtom *aProperty, 
                                        const nsAString *aAttribute,
                                        const nsAString *aValue);
  nsresult SetInlinePropertyOnNode( nsIDOMNode *aNode,
                                    nsIAtom *aProperty, 
                                    const nsAString *aAttribute,
                                    const nsAString *aValue);
  nsresult SetInlinePropertyOnNode(nsIContent* aNode,
                                   nsIAtom* aProperty,
                                   const nsAString* aAttribute,
                                   const nsAString* aValue);

  nsresult PromoteInlineRange(nsIDOMRange *inRange);
  nsresult PromoteRangeIfStartsOrEndsInNamedAnchor(nsIDOMRange *inRange);
  nsresult SplitStyleAboveRange(nsIDOMRange *aRange, 
                                nsIAtom *aProperty, 
                                const nsAString *aAttribute);
  nsresult SplitStyleAbovePoint(nsCOMPtr<nsIDOMNode> *aNode,
                                int32_t *aOffset,
                                nsIAtom *aProperty, 
                                const nsAString *aAttribute,
                                nsCOMPtr<nsIDOMNode> *outLeftNode = nullptr,
                                nsCOMPtr<nsIDOMNode> *outRightNode = nullptr);
  nsresult ApplyDefaultProperties();
  nsresult RemoveStyleInside(nsIDOMNode *aNode, 
                             nsIAtom *aProperty, 
                             const nsAString *aAttribute, 
                             const bool aChildrenOnly = false);
  nsresult RemoveInlinePropertyImpl(nsIAtom *aProperty, const nsAString *aAttribute);

  bool NodeIsProperty(nsIDOMNode *aNode);
  bool HasAttr(nsIDOMNode *aNode, const nsAString *aAttribute);
  bool IsAtFrontOfNode(nsIDOMNode *aNode, int32_t aOffset);
  bool IsAtEndOfNode(nsIDOMNode *aNode, int32_t aOffset);
  bool IsOnlyAttribute(nsIDOMNode *aElement, const nsAString *aAttribute);
  bool IsOnlyAttribute(const nsIContent* aElement, const nsAString& aAttribute);

  nsresult RemoveBlockContainer(nsIDOMNode *inNode);

  nsIContent* GetPriorHTMLSibling(nsINode* aNode);
  nsresult GetPriorHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode);
  nsIContent* GetPriorHTMLSibling(nsINode* aParent, int32_t aOffset);
  nsresult GetPriorHTMLSibling(nsIDOMNode *inParent, int32_t inOffset, nsCOMPtr<nsIDOMNode> *outNode);

  nsIContent* GetNextHTMLSibling(nsINode* aNode);
  nsresult GetNextHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode);
  nsIContent* GetNextHTMLSibling(nsINode* aParent, int32_t aOffset);
  nsresult GetNextHTMLSibling(nsIDOMNode *inParent, int32_t inOffset, nsCOMPtr<nsIDOMNode> *outNode);

  nsIContent* GetPriorHTMLNode(nsINode* aNode, bool aNoBlockCrossing = false);
  nsresult GetPriorHTMLNode(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode, bool bNoBlockCrossing = false);
  nsIContent* GetPriorHTMLNode(nsINode* aParent, int32_t aOffset,
                               bool aNoBlockCrossing = false);
  nsresult GetPriorHTMLNode(nsIDOMNode *inParent, int32_t inOffset, nsCOMPtr<nsIDOMNode> *outNode, bool bNoBlockCrossing = false);

  nsIContent* GetNextHTMLNode(nsINode* aNode, bool aNoBlockCrossing = false);
  nsresult GetNextHTMLNode(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode, bool bNoBlockCrossing = false);
  nsIContent* GetNextHTMLNode(nsINode* aParent, int32_t aOffset,
                              bool aNoBlockCrossing = false);
  nsresult GetNextHTMLNode(nsIDOMNode *inParent, int32_t inOffset, nsCOMPtr<nsIDOMNode> *outNode, bool bNoBlockCrossing = false);

  nsresult IsFirstEditableChild( nsIDOMNode *aNode, bool *aOutIsFirst);
  nsresult IsLastEditableChild( nsIDOMNode *aNode, bool *aOutIsLast);
  nsresult GetFirstEditableChild( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutFirstChild);
  nsresult GetLastEditableChild( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutLastChild);

  nsresult GetFirstEditableLeaf( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutFirstLeaf);
  nsresult GetLastEditableLeaf( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutLastLeaf);

  nsresult GetInlinePropertyBase(nsIAtom *aProperty, 
                             const nsAString *aAttribute,
                             const nsAString *aValue,
                             bool *aFirst, 
                             bool *aAny, 
                             bool *aAll,
                             nsAString *outValue,
                             bool aCheckDefaults = true);
  bool HasStyleOrIdOrClass(mozilla::dom::Element* aElement);
  nsresult RemoveElementIfNoStyleOrIdOrClass(nsIDOMNode* aElement);

  
  bool     OurWindowHasFocus();

  
  
  
  
  
  
  
  
  
  
  nsresult DoInsertHTMLWithContext(const nsAString& aInputString,
                                   const nsAString& aContextStr,
                                   const nsAString& aInfoStr,
                                   const nsAString& aFlavor,
                                   nsIDOMDocument* aSourceDoc,
                                   nsIDOMNode* aDestNode,
                                   int32_t aDestOffset,
                                   bool aDeleteSelection,
                                   bool aTrustedInput,
                                   bool aClearStyle = true);

  nsresult ClearStyle(nsCOMPtr<nsIDOMNode>* aNode, int32_t* aOffset,
                      nsIAtom* aProperty, const nsAString* aAttribute);


protected:

  nsCOMArray<nsIContentFilter> mContentFilters;

  nsRefPtr<TypeInState>        mTypeInState;

  bool mCRInParagraphCreatesParagraph;

  bool mCSSAware;
  nsAutoPtr<nsHTMLCSSUtils> mHTMLCSSUtils;

  
  int32_t  mSelectedCellIndex;

  nsString mLastStyleSheetURL;
  nsString mLastOverrideStyleSheetURL;

  
  nsTArray<nsString> mStyleSheetURLs;
  nsTArray<nsRefPtr<mozilla::CSSStyleSheet>> mStyleSheets;
  
  
  nsTArray<PropItem*> mDefaultStyles;

protected:

  
  void     RemoveListenerAndDeleteRef(const nsAString& aEvent,
                                      nsIDOMEventListener* aListener,
                                      bool aUseCapture,
                                      nsIDOMElement* aElement,
                                      nsIContent* aParentContent,
                                      nsIPresShell* aShell);
  void     DeleteRefToAnonymousNode(nsIDOMElement* aElement,
                                    nsIContent * aParentContent,
                                    nsIPresShell* aShell);

  nsresult ShowResizersInner(nsIDOMElement *aResizedElement);

  
  nsresult GetElementOrigin(nsIDOMElement * aElement, int32_t & aX, int32_t & aY);
  nsresult GetPositionAndDimensions(nsIDOMElement * aElement,
                                    int32_t & aX, int32_t & aY,
                                    int32_t & aW, int32_t & aH,
                                    int32_t & aBorderLeft,
                                    int32_t & aBorderTop,
                                    int32_t & aMarginLeft,
                                    int32_t & aMarginTop);

  
  

  
  bool mIsObjectResizingEnabled;
  bool mIsResizing;
  bool mPreserveRatio;
  bool mResizedObjectIsAnImage;

  
  bool mIsAbsolutelyPositioningEnabled;
  bool mResizedObjectIsAbsolutelyPositioned;

  bool mGrabberClicked;
  bool mIsMoving;

  bool mSnapToGridEnabled;

  
  bool mIsInlineTableEditingEnabled;

  

  nsCOMPtr<nsIDOMElement> mTopLeftHandle;
  nsCOMPtr<nsIDOMElement> mTopHandle;
  nsCOMPtr<nsIDOMElement> mTopRightHandle;
  nsCOMPtr<nsIDOMElement> mLeftHandle;
  nsCOMPtr<nsIDOMElement> mRightHandle;
  nsCOMPtr<nsIDOMElement> mBottomLeftHandle;
  nsCOMPtr<nsIDOMElement> mBottomHandle;
  nsCOMPtr<nsIDOMElement> mBottomRightHandle;

  nsCOMPtr<nsIDOMElement> mActivatedHandle;

  nsCOMPtr<nsIDOMElement> mResizingShadow;
  nsCOMPtr<nsIDOMElement> mResizingInfo;

  nsCOMPtr<nsIDOMElement> mResizedObject;

  nsCOMPtr<nsIDOMEventListener>  mMouseMotionListenerP;
  nsCOMPtr<nsISelectionListener> mSelectionListenerP;
  nsCOMPtr<nsIDOMEventListener>  mResizeEventListenerP;

  nsCOMArray<nsIHTMLObjectResizeListener> objectResizeEventListeners;

  int32_t mOriginalX;
  int32_t mOriginalY;

  int32_t mResizedObjectX;
  int32_t mResizedObjectY;
  int32_t mResizedObjectWidth;
  int32_t mResizedObjectHeight;

  int32_t mResizedObjectMarginLeft;
  int32_t mResizedObjectMarginTop;
  int32_t mResizedObjectBorderLeft;
  int32_t mResizedObjectBorderTop;

  int32_t mXIncrementFactor;
  int32_t mYIncrementFactor;
  int32_t mWidthIncrementFactor;
  int32_t mHeightIncrementFactor;

  int8_t  mInfoXIncrement;
  int8_t  mInfoYIncrement;

  nsresult SetAllResizersPosition();

  nsresult CreateResizer(nsIDOMElement ** aReturn, int16_t aLocation, nsIDOMNode * aParentNode);
  void     SetAnonymousElementPosition(int32_t aX, int32_t aY, nsIDOMElement *aResizer);

  nsresult CreateShadow(nsIDOMElement ** aReturn, nsIDOMNode * aParentNode,
                        nsIDOMElement * aOriginalObject);
  nsresult SetShadowPosition(nsIDOMElement * aShadow,
                             nsIDOMElement * aOriginalObject,
                             int32_t aOriginalObjectX,
                             int32_t aOriginalObjectY);

  nsresult CreateResizingInfo(nsIDOMElement ** aReturn, nsIDOMNode * aParentNode);
  nsresult SetResizingInfoPosition(int32_t aX, int32_t aY,
                                   int32_t aW, int32_t aH);

  int32_t  GetNewResizingIncrement(int32_t aX, int32_t aY, int32_t aID);
  nsresult StartResizing(nsIDOMElement * aHandle);
  int32_t  GetNewResizingX(int32_t aX, int32_t aY);
  int32_t  GetNewResizingY(int32_t aX, int32_t aY);
  int32_t  GetNewResizingWidth(int32_t aX, int32_t aY);
  int32_t  GetNewResizingHeight(int32_t aX, int32_t aY);
  void     HideShadowAndInfo();
  void     SetFinalSize(int32_t aX, int32_t aY);
  void     DeleteRefToAnonymousNode(nsIDOMNode * aNode);
  void     SetResizeIncrements(int32_t aX, int32_t aY, int32_t aW, int32_t aH, bool aPreserveRatio);
  void     HideAnonymousEditingUIs();

  

  int32_t mPositionedObjectX;
  int32_t mPositionedObjectY;
  int32_t mPositionedObjectWidth;
  int32_t mPositionedObjectHeight;

  int32_t mPositionedObjectMarginLeft;
  int32_t mPositionedObjectMarginTop;
  int32_t mPositionedObjectBorderLeft;
  int32_t mPositionedObjectBorderTop;

  nsCOMPtr<nsIDOMElement> mAbsolutelyPositionedObject;
  nsCOMPtr<nsIDOMElement> mGrabber;
  nsCOMPtr<nsIDOMElement> mPositioningShadow;

  int32_t      mGridSize;

  nsresult CreateGrabber(nsIDOMNode * aParentNode, nsIDOMElement ** aReturn);
  nsresult StartMoving(nsIDOMElement * aHandle);
  nsresult SetFinalPosition(int32_t aX, int32_t aY);
  void     AddPositioningOffset(int32_t & aX, int32_t & aY);
  void     SnapToGrid(int32_t & newX, int32_t & newY);
  nsresult GrabberClicked();
  nsresult EndMoving();
  nsresult CheckPositionedElementBGandFG(nsIDOMElement * aElement,
                                         nsAString & aReturn);

  

  nsCOMPtr<nsIDOMElement> mInlineEditedCell;

  nsCOMPtr<nsIDOMElement> mAddColumnBeforeButton;
  nsCOMPtr<nsIDOMElement> mRemoveColumnButton;
  nsCOMPtr<nsIDOMElement> mAddColumnAfterButton;

  nsCOMPtr<nsIDOMElement> mAddRowBeforeButton;
  nsCOMPtr<nsIDOMElement> mRemoveRowButton;
  nsCOMPtr<nsIDOMElement> mAddRowAfterButton;

  void     AddMouseClickListener(nsIDOMElement * aElement);
  void     RemoveMouseClickListener(nsIDOMElement * aElement);

  nsCOMPtr<nsILinkHandler> mLinkHandler;

public:


friend class nsHTMLEditRules;
friend class nsTextEditRules;
friend class nsWSRunObject;
friend class nsHTMLEditorEventListener;

private:
  
  bool IsSimpleModifiableNode(nsIContent* aContent,
                              nsIAtom* aProperty,
                              const nsAString* aAttribute,
                              const nsAString* aValue);
  nsresult SetInlinePropertyOnNodeImpl(nsIContent* aNode,
                                       nsIAtom* aProperty,
                                       const nsAString* aAttribute,
                                       const nsAString* aValue);
  typedef enum { eInserted, eAppended } InsertedOrAppended;
  void DoContentInserted(nsIDocument* aDocument, nsIContent* aContainer,
                         nsIContent* aChild, int32_t aIndexInContainer,
                         InsertedOrAppended aInsertedOrAppended);
  already_AddRefed<mozilla::dom::Element> GetElementOrParentByTagName(
      const nsAString& aTagName, nsINode* aNode);
  already_AddRefed<mozilla::dom::Element> CreateElementWithDefaults(
      const nsAString& aTagName);
};
#endif

