






































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

#include "nsIDocumentObserver.h"

#include "nsPoint.h"
#include "nsTArray.h"
#include "nsAutoPtr.h"

class nsIDOMKeyEvent;
class nsITransferable;
class nsIDOMNSRange;
class nsIDocumentEncoder;
class nsIClipboard;
class TypeInState;
class nsIContentFilter;
class nsIURL;
class nsIRangeUtils;
class nsILinkHandler;
struct PropItem;





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

  enum OperationID
  {
    kOpInsertBreak         = 3000,
    kOpMakeList            = 3001,
    kOpIndent              = 3002,
    kOpOutdent             = 3003,
    kOpAlign               = 3004,
    kOpMakeBasicBlock      = 3005,
    kOpRemoveList          = 3006,
    kOpMakeDefListItem     = 3007,
    kOpInsertElement       = 3008,
    kOpInsertQuotation     = 3009,
    kOpSetTextProperty     = 3010,
    kOpRemoveTextProperty  = 3011,
    kOpHTMLPaste           = 3012,
    kOpLoadHTML            = 3013,
    kOpResetTextProperties = 3014,
    kOpSetAbsolutePosition = 3015,
    kOpRemoveAbsolutePosition = 3016,
    kOpDecreaseZIndex      = 3017,
    kOpIncreaseZIndex      = 3018
  };

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

  
  NS_IMETHOD GetIsDocumentEditable(PRBool *aIsDocumentEditable);
  NS_IMETHOD BeginningOfDocument();
  virtual nsresult HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent);
  virtual already_AddRefed<nsIContent> GetFocusedContent();
  virtual PRBool IsActiveInDOMWindow();
  virtual already_AddRefed<nsIDOMEventTarget> GetPIDOMEventTarget();
  virtual already_AddRefed<nsIContent> FindSelectionRoot(nsINode *aNode);
  virtual PRBool IsAcceptableInputEvent(nsIDOMEvent* aEvent);

  
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  
  NS_IMETHOD GetPreferredIMEState(PRUint32 *aState);

  

  NS_DECL_NSIHTMLEDITOR

  
  
  NS_DECL_NSIHTMLOBJECTRESIZER

  
  
  NS_DECL_NSIHTMLABSPOSEDITOR

  
  
  NS_DECL_NSIHTMLINLINETABLEEDITOR

  
  NS_IMETHOD CopyLastEditableChildStyles(nsIDOMNode *aPreviousBlock, nsIDOMNode *aNewBlock,
                                         nsIDOMNode **aOutBrNode);

  NS_IMETHOD LoadHTML(const nsAString &aInputString);

  nsresult GetCSSBackgroundColorState(PRBool *aMixed, nsAString &aOutColor,
                                      PRBool aBlockLevel);
  NS_IMETHOD GetHTMLBackgroundColorState(PRBool *aMixed, nsAString &outColor);

  

  NS_IMETHOD AddStyleSheet(const nsAString & aURL);
  NS_IMETHOD ReplaceStyleSheet(const nsAString& aURL);
  NS_IMETHOD RemoveStyleSheet(const nsAString &aURL);

  NS_IMETHOD AddOverrideStyleSheet(const nsAString & aURL);
  NS_IMETHOD ReplaceOverrideStyleSheet(const nsAString& aURL);
  NS_IMETHOD RemoveOverrideStyleSheet(const nsAString &aURL);

  NS_IMETHOD EnableStyleSheet(const nsAString& aURL, PRBool aEnable);

  

  NS_DECL_NSIEDITORMAILSUPPORT

  

  NS_IMETHOD InsertTableCell(PRInt32 aNumber, PRBool aAfter);
  NS_IMETHOD InsertTableColumn(PRInt32 aNumber, PRBool aAfter);
  NS_IMETHOD InsertTableRow(PRInt32 aNumber, PRBool aAfter);
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
  NS_IMETHOD JoinTableCells(PRBool aMergeNonContiguousContents);
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
                           PRBool* aIsSelected);
  NS_IMETHOD GetFirstRow(nsIDOMElement* aTableElement, nsIDOMNode** aRowNode);
  NS_IMETHOD GetNextRow(nsIDOMNode* aCurrentRowNode, nsIDOMNode** aRowNode);
  NS_IMETHOD GetLastCellInRow(nsIDOMNode* aRowNode, nsIDOMNode** aCellNode);

  NS_IMETHOD SetSelectionAfterTableEdit(nsIDOMElement* aTable, PRInt32 aRow, PRInt32 aCol, 
                                        PRInt32 aDirection, PRBool aSelected);
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

  
  static nsCOMPtr<nsIDOMNode> GetBlockNodeParent(nsIDOMNode *aNode);
  
















  static nsresult GetBlockSection(nsIDOMNode  *aNode,
                                  nsIDOMNode **aLeftNode, 
                                  nsIDOMNode **aRightNode);

  








  static nsresult GetBlockSectionsForRange(nsIDOMRange      *aRange, 
                                           nsCOMArray<nsIDOMRange>& aSections);

  static nsCOMPtr<nsIDOMNode> NextNodeInBlock(nsIDOMNode *aNode, IterDirection aDir);
  nsresult IsNextCharWhitespace(nsIDOMNode *aParentNode, 
                                PRInt32 aOffset, 
                                PRBool *outIsSpace, 
                                PRBool *outIsNBSP,
                                nsCOMPtr<nsIDOMNode> *outNode = 0,
                                PRInt32 *outOffset = 0);
  nsresult IsPrevCharWhitespace(nsIDOMNode *aParentNode, 
                                PRInt32 aOffset, 
                                PRBool *outIsSpace, 
                                PRBool *outIsNBSP,
                                nsCOMPtr<nsIDOMNode> *outNode = 0,
                                PRInt32 *outOffset = 0);

  

  nsresult EndUpdateViewBatch();

  
  NS_IMETHOD Init(nsIDOMDocument *aDoc, nsIContent *aRoot, nsISelectionController *aSelCon, PRUint32 aFlags);
  NS_IMETHOD PreDestroy(PRBool aDestroyingFrames);

  
  static nsresult NodeIsBlockStatic(nsIDOMNode *aNode, PRBool *aIsBlock);

  NS_IMETHOD SetFlags(PRUint32 aFlags);

  NS_IMETHOD Paste(PRInt32 aSelectionType);
  NS_IMETHOD CanPaste(PRInt32 aSelectionType, PRBool *aCanPaste);

  NS_IMETHOD PasteTransferable(nsITransferable *aTransferable);
  NS_IMETHOD CanPasteTransferable(nsITransferable *aTransferable, PRBool *aCanPaste);

  NS_IMETHOD DebugUnitTests(PRInt32 *outNumTests, PRInt32 *outNumTestsFailed);

  

  NS_IMETHOD StartOperation(PRInt32 opID, nsIEditor::EDirection aDirection);

  

  NS_IMETHOD EndOperation();

  
  virtual PRBool TagCanContainTag(const nsAString& aParentTag, const nsAString& aChildTag);
  
  
  virtual PRBool IsContainer(nsIDOMNode *aNode);

  
  NS_IMETHOD SelectEntireDocument(nsISelection *aSelection);

  NS_IMETHOD SetAttributeOrEquivalent(nsIDOMElement * aElement,
                                      const nsAString & aAttribute,
                                      const nsAString & aValue,
                                      PRBool aSuppressTransaction);
  NS_IMETHOD RemoveAttributeOrEquivalent(nsIDOMElement * aElement,
                                         const nsAString & aAttribute,
                                         PRBool aSuppressTransaction);

  
  NS_IMETHOD CollapseAdjacentTextNodes(nsIDOMRange *aInRange);

  virtual PRBool NodesSameType(nsIDOMNode *aNode1, nsIDOMNode *aNode2);

  NS_IMETHODIMP DeleteNode(nsIDOMNode * aNode);
  NS_IMETHODIMP DeleteText(nsIDOMCharacterData *aTextNode,
                           PRUint32             aOffset,
                           PRUint32             aLength);
  NS_IMETHOD InsertTextImpl(const nsAString& aStringToInsert, 
                            nsCOMPtr<nsIDOMNode> *aInOutNode, 
                            PRInt32 *aInOutOffset,
                            nsIDOMDocument *aDoc);
  NS_IMETHOD_(PRBool) IsModifiableNode(nsIDOMNode *aNode);

  NS_IMETHOD SelectAll();

  NS_IMETHOD GetRootElement(nsIDOMElement **aRootElement);

  
  NS_IMETHOD StyleSheetLoaded(nsCSSStyleSheet*aSheet, PRBool aWasAlternate,
                              nsresult aStatus);

  
  NS_IMETHOD TypedText(const nsAString& aString, PRInt32 aAction);
  nsresult InsertNodeAtPoint( nsIDOMNode *aNode, 
                              nsCOMPtr<nsIDOMNode> *ioParent, 
                              PRInt32 *ioOffset, 
                              PRBool aNoEmptyNodes);
  nsCOMPtr<nsIDOMNode> FindUserSelectAllNode(nsIDOMNode *aNode);
                                

  


  nsresult GetTextSelectionOffsets(nsISelection *aSelection,
                                   PRInt32 &aStartOffset, 
                                   PRInt32 &aEndOffset);

  
  
  
  
  
  
  nsresult CollapseSelectionToDeepestNonTableFirstChild(nsISelection *aSelection, nsIDOMNode *aNode);

  virtual PRBool IsTextInDirtyFrameVisible(nsIDOMNode *aNode);

  nsresult IsVisTextNode( nsIDOMNode *aNode, 
                          PRBool *outIsEmptyNode, 
                          PRBool aSafeToAskFrames);
  nsresult IsEmptyNode(nsIDOMNode *aNode, PRBool *outIsEmptyBlock, 
                       PRBool aMozBRDoesntCount = PR_FALSE,
                       PRBool aListOrCellNotEmpty = PR_FALSE,
                       PRBool aSafeToAskFrames = PR_FALSE);
  nsresult IsEmptyNodeImpl(nsIDOMNode *aNode,
                           PRBool *outIsEmptyBlock, 
                           PRBool aMozBRDoesntCount,
                           PRBool aListOrCellNotEmpty,
                           PRBool aSafeToAskFrames,
                           PRBool *aSeenBR);

  
  PRBool   EnableExistingStyleSheet(const nsAString& aURL);

  
  NS_IMETHOD GetStyleSheetForURL(const nsAString &aURL,
                                 nsCSSStyleSheet **_retval);
  NS_IMETHOD GetURLForStyleSheet(nsCSSStyleSheet *aStyleSheet, nsAString &aURL);

  
  nsresult AddNewStyleSheetToList(const nsAString &aURL,
                                  nsCSSStyleSheet *aStyleSheet);

  nsresult RemoveStyleSheetFromList(const nsAString &aURL);
                       
protected:

  NS_IMETHOD  InitRules();

  
  virtual nsresult CreateEventListeners();

  virtual nsresult InstallEventListeners();
  virtual void RemoveEventListeners();

  PRBool ShouldReplaceRootElement();
  void ResetRootElementAndEventTarget();
  nsresult GetBodyElement(nsIDOMHTMLElement** aBody);
  
  
  
  already_AddRefed<nsINode> GetFocusedNode();

  
  PRBool SetCaretInTableCell(nsIDOMElement* aElement);
  PRBool IsElementInBody(nsIDOMElement* aElement);

  
  NS_IMETHOD TabInTable(PRBool inIsShift, PRBool *outHandled);
  NS_IMETHOD CreateBR(nsIDOMNode *aNode, PRInt32 aOffset, 
                      nsCOMPtr<nsIDOMNode> *outBRNode, nsIEditor::EDirection aSelect = nsIEditor::eNone);
  NS_IMETHOD CreateBRImpl(nsCOMPtr<nsIDOMNode> *aInOutParent, 
                         PRInt32 *aInOutOffset, 
                         nsCOMPtr<nsIDOMNode> *outBRNode, 
                         nsIEditor::EDirection aSelect);
  NS_IMETHOD InsertBR(nsCOMPtr<nsIDOMNode> *outBRNode);



  

  
  
  
  NS_IMETHOD InsertCell(nsIDOMElement *aCell, PRInt32 aRowSpan, PRInt32 aColSpan,
                        PRBool aAfter, PRBool aIsHeader, nsIDOMElement **aNewCell);

  
  NS_IMETHOD DeleteRow(nsIDOMElement *aTable, PRInt32 aRowIndex);
  NS_IMETHOD DeleteColumn(nsIDOMElement *aTable, PRInt32 aColIndex);
  NS_IMETHOD DeleteCellContents(nsIDOMElement *aCell);

  
  NS_IMETHOD MergeCells(nsCOMPtr<nsIDOMElement> aTargetCell, nsCOMPtr<nsIDOMElement> aCellToMerge, PRBool aDeleteCellToMerge);

  NS_IMETHOD DeleteTable2(nsIDOMElement *aTable, nsISelection *aSelection);
  NS_IMETHOD SetColSpan(nsIDOMElement *aCell, PRInt32 aColSpan);
  NS_IMETHOD SetRowSpan(nsIDOMElement *aCell, PRInt32 aRowSpan);

  
  NS_IMETHOD GetTableLayoutObject(nsIDOMElement* aTable, nsITableLayout **tableLayoutObject);
  
  
  PRInt32  GetNumberOfCellsInRow(nsIDOMElement* aTable, PRInt32 rowIndex);
  
  PRBool AllCellsInRowSelected(nsIDOMElement *aTable, PRInt32 aRowIndex, PRInt32 aNumberOfColumns);
  PRBool AllCellsInColumnSelected(nsIDOMElement *aTable, PRInt32 aColIndex, PRInt32 aNumberOfRows);

  PRBool   IsEmptyCell(nsIDOMElement *aCell);

  
  
  
  
  
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


  
  NS_IMETHOD IsRootTag(nsString &aTag, PRBool &aIsTag);

  virtual PRBool IsBlockNode(nsIDOMNode *aNode);
  
  static nsCOMPtr<nsIDOMNode> GetEnclosingTable(nsIDOMNode *aNode);

  














  virtual void IsTextPropertySetByContent(nsIDOMNode        *aNode,
                                          nsIAtom           *aProperty, 
                                          const nsAString   *aAttribute,
                                          const nsAString   *aValue,
                                          PRBool            &aIsSet,
                                          nsIDOMNode       **aStyleNode,
                                          nsAString *outValue = nsnull);

  
  NS_IMETHOD PasteAsPlaintextQuotation(PRInt32 aSelectionType);

  








  NS_IMETHOD InsertAsPlaintextQuotation(const nsAString & aQuotedText,
                                        PRBool aAddCites,
                                        nsIDOMNode **aNodeInserted);

  
  NS_IMETHOD PrepareTransferable(nsITransferable **transferable);
  NS_IMETHOD PrepareHTMLTransferable(nsITransferable **transferable, PRBool havePrivFlavor);
  nsresult   PutDragDataInTransferable(nsITransferable **aTransferable);
  NS_IMETHOD InsertFromTransferable(nsITransferable *transferable, 
                                    nsIDOMDocument *aSourceDoc,
                                    const nsAString & aContextStr,
                                    const nsAString & aInfoStr,
                                    nsIDOMNode *aDestinationNode,
                                    PRInt32 aDestinationOffset,
                                    PRBool aDoDeleteSelection);
  PRBool HavePrivateHTMLFlavor( nsIClipboard *clipboard );
  nsresult   ParseCFHTML(nsCString & aCfhtml, PRUnichar **aStuffToPaste, PRUnichar **aCfcontext);
  nsresult   DoContentFilterCallback(const nsAString &aFlavor,
                                     nsIDOMDocument *aSourceDoc,
                                     PRBool aWillDeleteSelection,
                                     nsIDOMNode **aFragmentAsNode,      
                                     nsIDOMNode **aFragStartNode,
                                     PRInt32 *aFragStartOffset,
                                     nsIDOMNode **aFragEndNode,
                                     PRInt32 *aFragEndOffset,
                                     nsIDOMNode **aTargetNode,       
                                     PRInt32 *aTargetOffset,   
                                     PRBool *aDoContinue);
  nsresult   RelativizeURIInFragmentList(const nsCOMArray<nsIDOMNode> &aNodeList,
                                        const nsAString &aFlavor,
                                        nsIDOMDocument *aSourceDoc,
                                        nsIDOMNode *aTargetNode);
  nsresult   RelativizeURIForNode(nsIDOMNode *aNode, nsIURL *aDestURL);
  nsresult   GetAttributeToModifyOnNode(nsIDOMNode *aNode, nsAString &aAttrib);

  PRBool     IsInLink(nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *outLink = nsnull);
  nsresult   StripFormattingNodes(nsIDOMNode *aNode, PRBool aOnlyList = PR_FALSE);
  nsresult   CreateDOMFragmentFromPaste(const nsAString & aInputString,
                                        const nsAString & aContextStr,
                                        const nsAString & aInfoStr,
                                        nsCOMPtr<nsIDOMNode> *outFragNode,
                                        nsCOMPtr<nsIDOMNode> *outStartNode,
                                        nsCOMPtr<nsIDOMNode> *outEndNode,
                                        PRInt32 *outStartOffset,
                                        PRInt32 *outEndOffset,
                                        PRBool aTrustedInput);
  nsresult   ParseFragment(const nsAString & aStr, nsTArray<nsString> &aTagStack,
                           nsIDocument* aTargetDoc,
                           nsCOMPtr<nsIDOMNode> *outNode,
                           PRBool aTrustedInput);
  nsresult   CreateListOfNodesToPaste(nsIDOMNode  *aFragmentAsNode,
                                      nsCOMArray<nsIDOMNode>& outNodeList,
                                      nsIDOMNode *aStartNode,
                                      PRInt32 aStartOffset,
                                      nsIDOMNode *aEndNode,
                                      PRInt32 aEndOffset);
  nsresult CreateTagStack(nsTArray<nsString> &aTagStack,
                          nsIDOMNode *aNode);
  nsresult GetListAndTableParents( PRBool aEnd, 
                                   nsCOMArray<nsIDOMNode>& aListOfNodes,
                                   nsCOMArray<nsIDOMNode>& outArray);
  nsresult DiscoverPartialListsAndTables(nsCOMArray<nsIDOMNode>& aPasteNodes,
                                         nsCOMArray<nsIDOMNode>& aListsAndTables,
                                         PRInt32 *outHighWaterMark);
  nsresult ScanForListAndTableStructure(PRBool aEnd,
                                        nsCOMArray<nsIDOMNode>& aNodes,
                                        nsIDOMNode *aListOrTable,
                                        nsCOMPtr<nsIDOMNode> *outReplaceNode);
  nsresult ReplaceOrphanedStructure( PRBool aEnd,
                                     nsCOMArray<nsIDOMNode>& aNodeArray,
                                     nsCOMArray<nsIDOMNode>& aListAndTableArray,
                                     PRInt32 aHighWaterMark);
  nsIDOMNode* GetArrayEndpoint(PRBool aEnd, nsCOMArray<nsIDOMNode>& aNodeArray);

  
  PRBool   IsVisBreak(nsIDOMNode *aNode);

  

  void NormalizeEOLInsertPosition(nsIDOMNode *firstNodeToInsert,
                                  nsCOMPtr<nsIDOMNode> *insertParentNode,
                                  PRInt32 *insertOffset);

  
  PRBool IsModifiable();

  
  nsresult MakeDefinitionItem(const nsAString & aItemType);
  nsresult InsertBasicBlock(const nsAString & aBlockType);
  
  
  nsresult RelativeFontChange( PRInt32 aSizeChange);
  
  
  nsresult RelativeFontChangeOnTextNode( PRInt32 aSizeChange, 
                                         nsIDOMCharacterData *aTextNode, 
                                         PRInt32 aStartOffset,
                                         PRInt32 aEndOffset);
  nsresult RelativeFontChangeOnNode( PRInt32 aSizeChange, 
                                     nsIDOMNode *aNode);
  nsresult RelativeFontChangeHelper( PRInt32 aSizeChange, 
                                     nsIDOMNode *aNode);

  
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

  nsresult PromoteInlineRange(nsIDOMRange *inRange);
  nsresult PromoteRangeIfStartsOrEndsInNamedAnchor(nsIDOMRange *inRange);
  nsresult SplitStyleAboveRange(nsIDOMRange *aRange, 
                                nsIAtom *aProperty, 
                                const nsAString *aAttribute);
  nsresult SplitStyleAbovePoint(nsCOMPtr<nsIDOMNode> *aNode,
                                PRInt32 *aOffset,
                                nsIAtom *aProperty, 
                                const nsAString *aAttribute,
                                nsCOMPtr<nsIDOMNode> *outLeftNode = nsnull,
                                nsCOMPtr<nsIDOMNode> *outRightNode = nsnull);
  nsresult ApplyDefaultProperties();
  nsresult RemoveStyleInside(nsIDOMNode *aNode, 
                             nsIAtom *aProperty, 
                             const nsAString *aAttribute, 
                             PRBool aChildrenOnly = PR_FALSE);
  nsresult RemoveInlinePropertyImpl(nsIAtom *aProperty, const nsAString *aAttribute);

  PRBool NodeIsProperty(nsIDOMNode *aNode);
  PRBool HasAttr(nsIDOMNode *aNode, const nsAString *aAttribute);
  PRBool HasAttrVal(nsIDOMNode *aNode, const nsAString *aAttribute, const nsAString *aValue);
  PRBool IsAtFrontOfNode(nsIDOMNode *aNode, PRInt32 aOffset);
  PRBool IsAtEndOfNode(nsIDOMNode *aNode, PRInt32 aOffset);
  PRBool IsOnlyAttribute(nsIDOMNode *aElement, const nsAString *aAttribute);

  nsresult RemoveBlockContainer(nsIDOMNode *inNode);
  nsresult GetPriorHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode);
  nsresult GetPriorHTMLSibling(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode);
  nsresult GetNextHTMLSibling(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode);
  nsresult GetNextHTMLSibling(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode);
  nsresult GetPriorHTMLNode(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode, PRBool bNoBlockCrossing = PR_FALSE);
  nsresult GetPriorHTMLNode(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode, PRBool bNoBlockCrossing = PR_FALSE);
  nsresult GetNextHTMLNode(nsIDOMNode *inNode, nsCOMPtr<nsIDOMNode> *outNode, PRBool bNoBlockCrossing = PR_FALSE);
  nsresult GetNextHTMLNode(nsIDOMNode *inParent, PRInt32 inOffset, nsCOMPtr<nsIDOMNode> *outNode, PRBool bNoBlockCrossing = PR_FALSE);

  nsresult IsFirstEditableChild( nsIDOMNode *aNode, PRBool *aOutIsFirst);
  nsresult IsLastEditableChild( nsIDOMNode *aNode, PRBool *aOutIsLast);
  nsresult GetFirstEditableChild( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutFirstChild);
  nsresult GetLastEditableChild( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutLastChild);

  nsresult GetFirstEditableLeaf( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutFirstLeaf);
  nsresult GetLastEditableLeaf( nsIDOMNode *aNode, nsCOMPtr<nsIDOMNode> *aOutLastLeaf);

  
  PRBool   mIgnoreSpuriousDragEvent;

  nsresult GetInlinePropertyBase(nsIAtom *aProperty, 
                             const nsAString *aAttribute,
                             const nsAString *aValue,
                             PRBool *aFirst, 
                             PRBool *aAny, 
                             PRBool *aAll,
                             nsAString *outValue,
                             PRBool aCheckDefaults = PR_TRUE);
  nsresult HasStyleOrIdOrClass(nsIDOMElement * aElement, PRBool *aHasStyleOrIdOrClass);
  nsresult RemoveElementIfNoStyleOrIdOrClass(nsIDOMElement * aElement, nsIAtom * aTag);

  
  PRBool   OurWindowHasFocus();

  
  
  
  
  
  
  
  nsresult DoInsertHTMLWithContext(const nsAString& aInputString,
                                   const nsAString& aContextStr,
                                   const nsAString& aInfoStr,
                                   const nsAString& aFlavor,
                                   nsIDOMDocument* aSourceDoc,
                                   nsIDOMNode* aDestNode,
                                   PRInt32 aDestOffset,
                                   PRBool aDeleteSelection,
                                   PRBool aTrustedInput);


protected:

  nsCOMArray<nsIContentFilter> mContentFilters;

  nsRefPtr<TypeInState>        mTypeInState;

  PRPackedBool mCRInParagraphCreatesParagraph;

  PRPackedBool mCSSAware;
  nsAutoPtr<nsHTMLCSSUtils> mHTMLCSSUtils;

  
  PRInt32  mSelectedCellIndex;

  nsString mLastStyleSheetURL;
  nsString mLastOverrideStyleSheetURL;

  
  nsTArray<nsString> mStyleSheetURLs;
  nsTArray<nsRefPtr<nsCSSStyleSheet> > mStyleSheets;
  
  
  nsTArray<PropItem*> mDefaultStyles;

   
   nsCOMPtr<nsITextServicesDocument> mTextServices;

  
  static nsIRangeUtils* sRangeHelper;

public:
  
  static void Shutdown();

protected:

  
  void     RemoveListenerAndDeleteRef(const nsAString& aEvent,
                                      nsIDOMEventListener* aListener,
                                      PRBool aUseCapture,
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

  
  

  
  PRPackedBool mIsObjectResizingEnabled;
  PRPackedBool mIsResizing;
  PRPackedBool mPreserveRatio;
  PRPackedBool mResizedObjectIsAnImage;

  
  PRPackedBool mIsAbsolutelyPositioningEnabled;
  PRPackedBool mResizedObjectIsAbsolutelyPositioned;

  PRPackedBool mGrabberClicked;
  PRPackedBool mIsMoving;

  PRPackedBool mSnapToGridEnabled;

  
  PRPackedBool mIsInlineTableEditingEnabled;

  

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
  void     SetResizeIncrements(PRInt32 aX, PRInt32 aY, PRInt32 aW, PRInt32 aH, PRBool aPreserveRatio);
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

};
#endif

