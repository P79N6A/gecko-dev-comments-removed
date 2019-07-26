




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
#include "nsITextServicesDocument.h"

#include "nsEditor.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventListener.h"
#include "nsICSSLoaderObserver.h"
#include "nsITableLayout.h"

#include "nsEditRules.h"

#include "nsEditProperty.h"
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
struct PropItem;

namespace mozilla {
namespace widget {
struct IMEState;
} 
} 





class nsHTMLEditor : public nsPlaintextEditor,
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
  virtual  ~nsHTMLEditor();

  bool GetReturnInParagraphCreatesNewParagraph();

  
  NS_IMETHOD GetIsDocumentEditable(bool *aIsDocumentEditable);
  NS_IMETHOD BeginningOfDocument();
  virtual nsresult HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent);
  virtual already_AddRefed<nsIContent> GetFocusedContent();
  virtual bool IsActiveInDOMWindow();
  virtual already_AddRefed<nsIDOMEventTarget> GetDOMEventTarget();
  virtual mozilla::dom::Element* GetEditorRoot() MOZ_OVERRIDE;
  virtual already_AddRefed<nsIContent> FindSelectionRoot(nsINode *aNode);
  virtual bool IsAcceptableInputEvent(nsIDOMEvent* aEvent);
  virtual already_AddRefed<nsIContent> GetInputEventTargetContent();
  virtual bool IsEditable(nsIContent *aNode);
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

  

  NS_IMETHOD InsertTableCell(PRInt32 aNumber, bool aAfter);
  NS_IMETHOD InsertTableColumn(PRInt32 aNumber, bool aAfter);
  NS_IMETHOD InsertTableRow(PRInt32 aNumber, bool aAfter);
  NS_IMETHOD DeleteTable();
  NS_IMETHOD DeleteTableCell(PRInt32 aNumber);
  NS_IMETHOD DeleteTableCellContents();
  NS_IMETHOD DeleteTableColumn(PRInt32 aNumber);
  NS_IMETHOD DeleteTableRow(PRInt32 aNumber);
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
                            PRInt32* aRowIndex, PRInt32* aColIndex);
  NS_IMETHOD GetTableSize(nsIDOMElement *aTable,
                          PRInt32* aRowCount, PRInt32* aColCount);
  NS_IMETHOD GetCellAt(nsIDOMElement* aTable, PRInt32 aRowIndex, PRInt32 aColIndex, nsIDOMElement **aCell);
  NS_IMETHOD GetCellDataAt(nsIDOMElement* aTable,
                           PRInt32 aRowIndex, PRInt32 aColIndex,
                           nsIDOMElement **aCell,
                           PRInt32* aStartRowIndex, PRInt32* aStartColIndex,
                           PRInt32* aRowSpan, PRInt32* aColSpan, 
                           PRInt32* aActualRowSpan, PRInt32* aActualColSpan, 
                           bool* aIsSelected);
  NS_IMETHOD GetFirstRow(nsIDOMElement* aTableElement, nsIDOMNode** aRowNode);
  NS_IMETHOD GetNextRow(nsIDOMNode* aCurrentRowNode, nsIDOMNode** aRowNode);
  NS_IMETHOD GetLastCellInRow(nsIDOMNode* aRowNode, nsIDOMNode** aCellNode);

  NS_IMETHOD SetSelectionAfterTableEdit(nsIDOMElement* aTable, PRInt32 aRow, PRInt32 aCol, 
                                        PRInt32 aDirection, bool aSelected);
  NS_IMETHOD GetSelectedOrParentTableElement(nsAString& aTagName,
                                             PRInt32 *aSelectedCount,
                                             nsIDOMElement** aTableElement);
  NS_IMETHOD GetSelectedCellsType(nsIDOMElement *aElement, PRUint32 *aSelectionType);

  nsresult GetCellFromRange(nsIDOMRange *aRange, nsIDOMElement **aCell);

  
  
  
  
  
  NS_IMETHOD GetFirstSelectedCell(nsIDOMRange **aRange, nsIDOMElement **aCell);
  
  
  NS_IMETHOD GetNextSelectedCell(nsIDOMRange **aRange, nsIDOMElement **aCell);

  
  NS_IMETHOD GetFirstSelectedCellInTable(PRInt32 *aRowIndex, PRInt32 *aColIndex, nsIDOMElement **aCell);
    
  
  
  
  NS_IMETHOD SetCSSBackgroundColor(const nsAString& aColor);
  NS_IMETHOD SetHTMLBackgroundColor(const nsAString& aColor);

  
  static already_AddRefed<nsIDOMNode> GetBlockNodeParent(nsIDOMNode *aNode);

  void IsNextCharInNodeWhitespace(nsIContent* aContent,
                                  PRInt32 aOffset,
                                  bool* outIsSpace,
                                  bool* outIsNBSP,
                                  nsIContent** outNode = nullptr,
                                  PRInt32* outOffset = 0);
  void IsPrevCharInNodeWhitespace(nsIContent* aContent,
                                  PRInt32 aOffset,
                                  bool* outIsSpace,
                                  bool* outIsNBSP,
                                  nsIContent** outNode = nullptr,
                                  PRInt32* outOffset = 0);

  

  nsresult EndUpdateViewBatch();

  
  NS_IMETHOD Init(nsIDOMDocument *aDoc, nsIContent *aRoot, nsISelectionController *aSelCon, PRUint32 aFlags);
  NS_IMETHOD PreDestroy(bool aDestroyingFrames);

  
  
  static bool NodeIsBlockStatic(const mozilla::dom::Element* aElement);
  static nsresult NodeIsBlockStatic(nsIDOMNode *aNode, bool *aIsBlock);
protected:
  using nsEditor::IsBlockNode;
  virtual bool IsBlockNode(nsINode *aNode);

public:
  NS_IMETHOD SetFlags(PRUint32 aFlags);

  NS_IMETHOD Paste(PRInt32 aSelectionType);
  NS_IMETHOD CanPaste(PRInt32 aSelectionType, bool *aCanPaste);

  NS_IMETHOD PasteTransferable(nsITransferable *aTransferable);
  NS_IMETHOD CanPasteTransferable(nsITransferable *aTransferable, bool *aCanPaste);

  NS_IMETHOD DebugUnitTests(PRInt32 *outNumTests, PRInt32 *outNumTestsFailed);

  

  NS_IMETHOD StartOperation(OperationID opID,
                            nsIEditor::EDirection aDirection);

  

  NS_IMETHOD EndOperation();

  
  virtual bool TagCanContainTag(nsIAtom* aParentTag, nsIAtom* aChildTag);
  
  
  virtual bool IsContainer(nsIDOMNode *aNode);

  
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
  NS_IMETHODIMP DeleteText(nsIDOMCharacterData *aTextNode,
                           PRUint32             aOffset,
                           PRUint32             aLength);
  NS_IMETHOD InsertTextImpl(const nsAString& aStringToInsert, 
                            nsCOMPtr<nsIDOMNode> *aInOutNode, 
                            PRInt32 *aInOutOffset,
                            nsIDOMDocument *aDoc);
  NS_IMETHOD_(bool) IsModifiableNode(nsIDOMNode *aNode);
  virtual bool IsModifiableNode(nsINode *aNode);

  NS_IMETHOD GetIsSelectionEditable(bool* aIsSelectionEditable);

  NS_IMETHOD SelectAll();

  NS_IMETHOD GetRootElement(nsIDOMElement **aRootElement);

  
  NS_IMETHOD StyleSheetLoaded(nsCSSStyleSheet*aSheet, bool aWasAlternate,
                              nsresult aStatus);

  
  NS_IMETHOD TypedText(const nsAString& aString, ETypingAction aAction);
  nsresult InsertNodeAtPoint( nsIDOMNode *aNode, 
                              nsCOMPtr<nsIDOMNode> *ioParent, 
                              PRInt32 *ioOffset, 
                              bool aNoEmptyNodes);

  
  
  
  
  
  
  nsresult CollapseSelectionToDeepestNonTableFirstChild(nsISelection *aSelection, nsIDOMNode *aNode);

  


  virtual bool IsTextInDirtyFrameVisible(nsIContent *aNode);

  



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
                                 nsCSSStyleSheet **_retval);
  NS_IMETHOD GetURLForStyleSheet(nsCSSStyleSheet *aStyleSheet, nsAString &aURL);

  
  nsresult AddNewStyleSheetToList(const nsAString &aURL,
                                  nsCSSStyleSheet *aStyleSheet);

  nsresult RemoveStyleSheetFromList(const nsAString &aURL);

  bool IsCSSEnabled()
  {
    
    return mCSSAware && mHTMLCSSUtils && mHTMLCSSUtils->IsCSSPrefChecked();
  }

  static bool HasAttributes(mozilla::dom::Element* aElement)
  {
    MOZ_ASSERT(aElement);
    PRUint32 attrCount = aElement->GetAttrCount();
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
  NS_IMETHOD CreateBR(nsIDOMNode *aNode, PRInt32 aOffset, 
                      nsCOMPtr<nsIDOMNode> *outBRNode, nsIEditor::EDirection aSelect = nsIEditor::eNone);



  

  
  
  
  NS_IMETHOD InsertCell(nsIDOMElement *aCell, PRInt32 aRowSpan, PRInt32 aColSpan,
                        bool aAfter, bool aIsHeader, nsIDOMElement **aNewCell);

  
  NS_IMETHOD DeleteRow(nsIDOMElement *aTable, PRInt32 aRowIndex);
  NS_IMETHOD DeleteColumn(nsIDOMElement *aTable, PRInt32 aColIndex);
  NS_IMETHOD DeleteCellContents(nsIDOMElement *aCell);

  
  NS_IMETHOD MergeCells(nsCOMPtr<nsIDOMElement> aTargetCell, nsCOMPtr<nsIDOMElement> aCellToMerge, bool aDeleteCellToMerge);

  NS_IMETHOD DeleteTable2(nsIDOMElement *aTable, nsISelection *aSelection);
  NS_IMETHOD SetColSpan(nsIDOMElement *aCell, PRInt32 aColSpan);
  NS_IMETHOD SetRowSpan(nsIDOMElement *aCell, PRInt32 aRowSpan);

  
  NS_IMETHOD GetTableLayoutObject(nsIDOMElement* aTable, nsITableLayout **tableLayoutObject);
  
  
  PRInt32  GetNumberOfCellsInRow(nsIDOMElement* aTable, PRInt32 rowIndex);
  
  bool AllCellsInRowSelected(nsIDOMElement *aTable, PRInt32 aRowIndex, PRInt32 aNumberOfColumns);
  bool AllCellsInColumnSelected(nsIDOMElement *aTable, PRInt32 aColIndex, PRInt32 aNumberOfRows);

  bool IsEmptyCell(mozilla::dom::Element* aCell);

  
  
  
  
  
  NS_IMETHOD GetCellContext(nsISelection **aSelection,
                            nsIDOMElement   **aTable,
                            nsIDOMElement   **aCell,
                            nsIDOMNode      **aCellParent, PRInt32 *aCellOffset,
                            PRInt32 *aRowIndex, PRInt32 *aColIndex);

  NS_IMETHOD GetCellSpansAt(nsIDOMElement* aTable, PRInt32 aRowIndex, PRInt32 aColIndex, 
                            PRInt32& aActualRowSpan, PRInt32& aActualColSpan);

  NS_IMETHOD SplitCellIntoColumns(nsIDOMElement *aTable, PRInt32 aRowIndex, PRInt32 aColIndex,
                                  PRInt32 aColSpanLeft, PRInt32 aColSpanRight, nsIDOMElement **aNewCell);

  NS_IMETHOD SplitCellIntoRows(nsIDOMElement *aTable, PRInt32 aRowIndex, PRInt32 aColIndex,
                               PRInt32 aRowSpanAbove, PRInt32 aRowSpanBelow, nsIDOMElement **aNewCell);

  nsresult CopyCellBackgroundColor(nsIDOMElement *destCell, nsIDOMElement *sourceCell);

  
  NS_IMETHOD FixBadRowSpan(nsIDOMElement *aTable, PRInt32 aRowIndex, PRInt32& aNewRowCount);
  NS_IMETHOD FixBadColSpan(nsIDOMElement *aTable, PRInt32 aColIndex, PRInt32& aNewColCount);

  
  
  NS_IMETHOD SetSelectionAtDocumentStart(nsISelection *aSelection);


  
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

  
  NS_IMETHOD PasteAsPlaintextQuotation(PRInt32 aSelectionType);

  








  NS_IMETHOD InsertAsPlaintextQuotation(const nsAString & aQuotedText,
                                        bool aAddCites,
                                        nsIDOMNode **aNodeInserted);
  
  
  
  bool IsSafeToInsertData(nsIDOMDocument* aSourceDoc);

  nsresult InsertObject(const char* aType, nsISupports* aObject, bool aIsSafe,
                        nsIDOMDocument *aSourceDoc,
                        nsIDOMNode *aDestinationNode,
                        PRInt32 aDestOffset,
                        bool aDoDeleteSelection);

  
  NS_IMETHOD PrepareTransferable(nsITransferable **transferable);
  NS_IMETHOD PrepareHTMLTransferable(nsITransferable **transferable, bool havePrivFlavor);
  NS_IMETHOD InsertFromTransferable(nsITransferable *transferable, 
                                    nsIDOMDocument *aSourceDoc,
                                    const nsAString & aContextStr,
                                    const nsAString & aInfoStr,
                                    nsIDOMNode *aDestinationNode,
                                    PRInt32 aDestinationOffset,
                                    bool aDoDeleteSelection);
  nsresult InsertFromDataTransfer(nsIDOMDataTransfer *aDataTransfer,
                                  PRInt32 aIndex,
                                  nsIDOMDocument *aSourceDoc,
                                  nsIDOMNode *aDestinationNode,
                                  PRInt32 aDestOffset,
                                  bool aDoDeleteSelection);
  bool HavePrivateHTMLFlavor( nsIClipboard *clipboard );
  nsresult   ParseCFHTML(nsCString & aCfhtml, PRUnichar **aStuffToPaste, PRUnichar **aCfcontext);
  nsresult   DoContentFilterCallback(const nsAString &aFlavor,
                                     nsIDOMDocument *aSourceDoc,
                                     bool aWillDeleteSelection,
                                     nsIDOMNode **aFragmentAsNode,      
                                     nsIDOMNode **aFragStartNode,
                                     PRInt32 *aFragStartOffset,
                                     nsIDOMNode **aFragEndNode,
                                     PRInt32 *aFragEndOffset,
                                     nsIDOMNode **aTargetNode,       
                                     PRInt32 *aTargetOffset,   
                                     bool *aDoContinue);
  nsresult   GetAttributeToModifyOnNode(nsIDOMNode *aNode, nsAString &aAttrib);

  bool       IsInLink(nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *outLink = nullptr);
  nsresult   StripFormattingNodes(nsIDOMNode *aNode, bool aOnlyList = false);
  nsresult   CreateDOMFragmentFromPaste(const nsAString & aInputString,
                                        const nsAString & aContextStr,
                                        const nsAString & aInfoStr,
                                        nsCOMPtr<nsIDOMNode> *outFragNode,
                                        nsCOMPtr<nsIDOMNode> *outStartNode,
                                        nsCOMPtr<nsIDOMNode> *outEndNode,
                                        PRInt32 *outStartOffset,
                                        PRInt32 *outEndOffset,
                                        bool aTrustedInput);
  nsresult   ParseFragment(const nsAString & aStr, nsIAtom* aContextLocalName,
                           nsIDocument* aTargetDoc,
                           nsCOMPtr<nsIDOMNode> *outNode,
                           bool aTrustedInput);
  nsresult   CreateListOfNodesToPaste(nsIDOMNode  *aFragmentAsNode,
                                      nsCOMArray<nsIDOMNode>& outNodeList,
                                      nsIDOMNode *aStartNode,
                                      PRInt32 aStartOffset,
                                      nsIDOMNode *aEndNode,
                                      PRInt32 aEndOffset);
  nsresult CreateTagStack(nsTArray<nsString> &aTagStack,
                          nsIDOMNode *aNode);
  nsresult GetListAndTableParents( bool aEnd, 
                                   nsCOMArray<nsIDOMNode>& aListOfNodes,
                                   nsCOMArray<nsIDOMNode>& outArray);
  nsresult DiscoverPartialListsAndTables(nsCOMArray<nsIDOMNode>& aPasteNodes,
                                         nsCOMArray<nsIDOMNode>& aListsAndTables,
                                         PRInt32 *outHighWaterMark);
  nsresult ScanForListAndTableStructure(bool aEnd,
                                        nsCOMArray<nsIDOMNode>& aNodes,
                                        nsIDOMNode *aListOrTable,
                                        nsCOMPtr<nsIDOMNode> *outReplaceNode);
  nsresult ReplaceOrphanedStructure( bool aEnd,
                                     nsCOMArray<nsIDOMNode>& aNodeArray,
                                     nsCOMArray<nsIDOMNode>& aListAndTableArray,
                                     PRInt32 aHighWaterMark);
  nsIDOMNode* GetArrayEndpoint(bool aEnd, nsCOMArray<nsIDOMNode>& aNodeArray);

  
  bool     IsVisBreak(nsIDOMNode *aNode);

  

  void NormalizeEOLInsertPosition(nsIDOMNode *firstNodeToInsert,
                                  nsCOMPtr<nsIDOMNode> *insertParentNode,
                                  PRInt32 *insertOffset);

  
  bool IsModifiable();

  
  nsresult MakeDefinitionItem(const nsAString & aItemType);
  nsresult InsertBasicBlock(const nsAString & aBlockType);
  
  
  nsresult RelativeFontChange( PRInt32 aSizeChange);
  
  
  nsresult RelativeFontChangeOnTextNode( PRInt32 aSizeChange, 
                                         nsIDOMCharacterData *aTextNode, 
                                         PRInt32 aStartOffset,
                                         PRInt32 aEndOffset);
  nsresult RelativeFontChangeOnNode(PRInt32 aSizeChange, nsINode* aNode);
  nsresult RelativeFontChangeHelper(PRInt32 aSizeChange, nsINode* aNode);

  
  nsresult SetInlinePropertyOnTextNode( nsIDOMCharacterData *aTextNode, 
                                        PRInt32 aStartOffset,
                                        PRInt32 aEndOffset,
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
                                PRInt32 *aOffset,
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
  bool IsAtFrontOfNode(nsIDOMNode *aNode, PRInt32 aOffset);
  bool IsAtEndOfNode(nsIDOMNode *aNode, PRInt32 aOffset);
  bool IsOnlyAttribute(nsIDOMNode *aElement, const nsAString *aAttribute);
  bool IsOnlyAttribute(const nsIContent* aElement, const nsAString& aAttribute);

  nsresult RemoveBlockContainer(nsIDOMNode *inNode);
  nsIContent* GetPriorHTMLSibling(nsINode* aNode);
  nsresult GetPriorHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode);
  nsIContent* GetPriorHTMLSibling(nsINode* aParent, PRInt32 aOffset);
  nsresult GetPriorHTMLSibling(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode);
  nsIContent* GetNextHTMLSibling(nsINode* aNode);
  nsresult GetNextHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode);
  nsIContent* GetNextHTMLSibling(nsINode* aParent, PRInt32 aOffset);
  nsresult GetNextHTMLSibling(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode);
  nsresult GetPriorHTMLNode(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode, bool bNoBlockCrossing = false);
  nsresult GetPriorHTMLNode(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode, bool bNoBlockCrossing = false);
  nsIContent* GetPriorHTMLNode(nsINode* aParent, PRInt32 aOffset,
                               bool aNoBlockCrossing = false);
  nsresult GetNextHTMLNode(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode, bool bNoBlockCrossing = false);
  nsIContent* GetNextHTMLNode(nsINode* aParent, PRInt32 aOffset,
                              bool aNoBlockCrossing = false);
  nsresult GetNextHTMLNode(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode, bool bNoBlockCrossing = false);

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
                                   PRInt32 aDestOffset,
                                   bool aDeleteSelection,
                                   bool aTrustedInput);

  nsresult ClearStyle(nsCOMPtr<nsIDOMNode>* aNode, PRInt32* aOffset,
                      nsIAtom* aProperty, const nsAString* aAttribute);


protected:

  nsCOMArray<nsIContentFilter> mContentFilters;

  nsRefPtr<TypeInState>        mTypeInState;

  bool mCRInParagraphCreatesParagraph;

  bool mCSSAware;
  nsAutoPtr<nsHTMLCSSUtils> mHTMLCSSUtils;

  
  PRInt32  mSelectedCellIndex;

  nsString mLastStyleSheetURL;
  nsString mLastOverrideStyleSheetURL;

  
  nsTArray<nsString> mStyleSheetURLs;
  nsTArray<nsRefPtr<nsCSSStyleSheet> > mStyleSheets;
  
  
  nsTArray<PropItem*> mDefaultStyles;

   
   nsCOMPtr<nsITextServicesDocument> mTextServices;

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

  
  nsresult GetElementOrigin(nsIDOMElement * aElement, PRInt32 & aX, PRInt32 & aY);
  nsresult GetPositionAndDimensions(nsIDOMElement * aElement,
                                    PRInt32 & aX, PRInt32 & aY,
                                    PRInt32 & aW, PRInt32 & aH,
                                    PRInt32 & aBorderLeft,
                                    PRInt32 & aBorderTop,
                                    PRInt32 & aMarginLeft,
                                    PRInt32 & aMarginTop);

  
  

  
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

  PRInt32 mOriginalX;
  PRInt32 mOriginalY;

  PRInt32 mResizedObjectX;
  PRInt32 mResizedObjectY;
  PRInt32 mResizedObjectWidth;
  PRInt32 mResizedObjectHeight;

  PRInt32 mResizedObjectMarginLeft;
  PRInt32 mResizedObjectMarginTop;
  PRInt32 mResizedObjectBorderLeft;
  PRInt32 mResizedObjectBorderTop;

  PRInt32 mXIncrementFactor;
  PRInt32 mYIncrementFactor;
  PRInt32 mWidthIncrementFactor;
  PRInt32 mHeightIncrementFactor;

  PRInt8  mInfoXIncrement;
  PRInt8  mInfoYIncrement;

  nsresult SetAllResizersPosition();

  nsresult CreateResizer(nsIDOMElement ** aReturn, PRInt16 aLocation, nsIDOMNode * aParentNode);
  void     SetAnonymousElementPosition(PRInt32 aX, PRInt32 aY, nsIDOMElement *aResizer);

  nsresult CreateShadow(nsIDOMElement ** aReturn, nsIDOMNode * aParentNode,
                        nsIDOMElement * aOriginalObject);
  nsresult SetShadowPosition(nsIDOMElement * aShadow,
                             nsIDOMElement * aOriginalObject,
                             PRInt32 aOriginalObjectX,
                             PRInt32 aOriginalObjectY);

  nsresult CreateResizingInfo(nsIDOMElement ** aReturn, nsIDOMNode * aParentNode);
  nsresult SetResizingInfoPosition(PRInt32 aX, PRInt32 aY,
                                   PRInt32 aW, PRInt32 aH);

  PRInt32  GetNewResizingIncrement(PRInt32 aX, PRInt32 aY, PRInt32 aID);
  nsresult StartResizing(nsIDOMElement * aHandle);
  PRInt32  GetNewResizingX(PRInt32 aX, PRInt32 aY);
  PRInt32  GetNewResizingY(PRInt32 aX, PRInt32 aY);
  PRInt32  GetNewResizingWidth(PRInt32 aX, PRInt32 aY);
  PRInt32  GetNewResizingHeight(PRInt32 aX, PRInt32 aY);
  void     HideShadowAndInfo();
  void     SetFinalSize(PRInt32 aX, PRInt32 aY);
  void     DeleteRefToAnonymousNode(nsIDOMNode * aNode);
  void     SetResizeIncrements(PRInt32 aX, PRInt32 aY, PRInt32 aW, PRInt32 aH, bool aPreserveRatio);
  void     HideAnonymousEditingUIs();

  

  PRInt32 mPositionedObjectX;
  PRInt32 mPositionedObjectY;
  PRInt32 mPositionedObjectWidth;
  PRInt32 mPositionedObjectHeight;

  PRInt32 mPositionedObjectMarginLeft;
  PRInt32 mPositionedObjectMarginTop;
  PRInt32 mPositionedObjectBorderLeft;
  PRInt32 mPositionedObjectBorderTop;

  nsCOMPtr<nsIDOMElement> mAbsolutelyPositionedObject;
  nsCOMPtr<nsIDOMElement> mGrabber;
  nsCOMPtr<nsIDOMElement> mPositioningShadow;

  PRInt32      mGridSize;

  nsresult CreateGrabber(nsIDOMNode * aParentNode, nsIDOMElement ** aReturn);
  nsresult StartMoving(nsIDOMElement * aHandle);
  nsresult SetFinalPosition(PRInt32 aX, PRInt32 aY);
  void     AddPositioningOffset(PRInt32 & aX, PRInt32 & aY);
  void     SnapToGrid(PRInt32 & newX, PRInt32 & newY);
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

};
#endif

