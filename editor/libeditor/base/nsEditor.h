




#ifndef __editor_h__
#define __editor_h__

#include "mozilla/Assertions.h"         
#include "mozilla/EventForwards.h"      
#include "mozilla/TypedEnum.h"          
#include "nsAutoPtr.h"                  
#include "nsCOMArray.h"                 
#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsEditProperty.h"             
#include "nsIEditor.h"                  
#include "nsIEditorIMESupport.h"        
#include "nsIObserver.h"                
#include "nsIPhonetic.h"                
#include "nsIPlaintextEditor.h"         
#include "nsISupportsImpl.h"            
#include "nsIWeakReferenceUtils.h"      
#include "nsLiteralString.h"            
#include "nsSelectionState.h"           
#include "nsString.h"                   
#include "nsWeakReference.h"            
#include "nscore.h"                     

class AddStyleSheetTxn;
class ChangeAttributeTxn;
class CreateElementTxn;
class DeleteNodeTxn;
class DeleteTextTxn;
class EditAggregateTxn;
class IMETextTxn;
class InsertElementTxn;
class InsertTextTxn;
class JoinElementTxn;
class RemoveStyleSheetTxn;
class SplitElementTxn;
class nsCSSStyleSheet;
class nsIAtom;
class nsIContent;
class nsIDOMCharacterData;
class nsIDOMDataTransfer;
class nsIDOMDocument;
class nsIDOMElement;
class nsIDOMEvent;
class nsIDOMEventListener;
class nsIDOMEventTarget;
class nsIDOMKeyEvent;
class nsIDOMNode;
class nsIDOMRange;
class nsIDocument;
class nsIDocumentStateListener;
class nsIEditActionListener;
class nsIEditorObserver;
class nsIInlineSpellChecker;
class nsINode;
class nsIPresShell;
class nsIPrivateTextRangeList;
class nsISelection;
class nsISupports;
class nsITransaction;
class nsIWidget;
class nsRange;
class nsString;
class nsTransactionManager;

namespace mozilla {
class Selection;

namespace dom {
class Element;
}  
}  

namespace mozilla {
namespace widget {
struct IMEState;
} 
} 

#define kMOZEditorBogusNodeAttrAtom nsEditProperty::mozEditorBogusNode
#define kMOZEditorBogusNodeValue NS_LITERAL_STRING("TRUE")



MOZ_BEGIN_ENUM_CLASS(EditAction, int32_t)
  ignore = -1,
  none = 0,
  undo,
  redo,
  insertNode,
  createNode,
  deleteNode,
  splitNode,
  joinNode,
  deleteText = 1003,

  
  insertText         = 2000,
  insertIMEText      = 2001,
  deleteSelection    = 2002,
  setTextProperty    = 2003,
  removeTextProperty = 2004,
  outputText         = 2005,

  
  insertBreak         = 3000,
  makeList            = 3001,
  indent              = 3002,
  outdent             = 3003,
  align               = 3004,
  makeBasicBlock      = 3005,
  removeList          = 3006,
  makeDefListItem     = 3007,
  insertElement       = 3008,
  insertQuotation     = 3009,
  htmlPaste           = 3012,
  loadHTML            = 3013,
  resetTextProperties = 3014,
  setAbsolutePosition = 3015,
  removeAbsolutePosition = 3016,
  decreaseZIndex      = 3017,
  increaseZIndex      = 3018
MOZ_END_ENUM_CLASS(EditAction)

inline bool operator!(const EditAction& aOp)
{
  return aOp == EditAction::none;
}






class nsEditor : public nsIEditor,
                 public nsIEditorIMESupport,
                 public nsSupportsWeakReference,
                 public nsIObserver,
                 public nsIPhonetic
{
public:

  enum IterDirection
  {
    kIterForward,
    kIterBackward
  };

  


  nsEditor();
  


  virtual ~nsEditor();



  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsEditor,
                                           nsIEditor)

  
  already_AddRefed<nsIDOMDocument> GetDOMDocument();
  already_AddRefed<nsIDocument> GetDocument();
  already_AddRefed<nsIPresShell> GetPresShell();
  void NotifyEditorObservers();

  
  NS_DECL_NSIEDITOR

  
  NS_DECL_NSIEDITORIMESUPPORT

  
  NS_DECL_NSIOBSERVER

  
  NS_DECL_NSIPHONETIC

public:

  nsresult MarkNodeDirty(nsINode* aNode);
  virtual bool IsModifiableNode(nsINode *aNode);

  NS_IMETHOD InsertTextImpl(const nsAString& aStringToInsert, 
                               nsCOMPtr<nsIDOMNode> *aInOutNode, 
                               int32_t *aInOutOffset,
                               nsIDOMDocument *aDoc);
  nsresult InsertTextIntoTextNodeImpl(const nsAString& aStringToInsert, 
                                      nsIDOMCharacterData *aTextNode, 
                                      int32_t aOffset,
                                      bool aSuppressIME = false);
  NS_IMETHOD DeleteSelectionImpl(EDirection aAction,
                                 EStripWrappers aStripWrappers);
  NS_IMETHOD DeleteSelectionAndCreateNode(const nsAString& aTag,
                                           nsIDOMNode ** aNewNode);

  
  nsresult DeleteNode(nsINode* aNode);
  nsresult ReplaceContainer(nsINode* inNode,
                            mozilla::dom::Element** outNode,
                            const nsAString& aNodeType,
                            const nsAString* aAttribute = nullptr,
                            const nsAString* aValue = nullptr,
                            bool aCloneAttributes = false);
  nsresult ReplaceContainer(nsIDOMNode *inNode, 
                            nsCOMPtr<nsIDOMNode> *outNode, 
                            const nsAString &aNodeType,
                            const nsAString *aAttribute = nullptr,
                            const nsAString *aValue = nullptr,
                            bool aCloneAttributes = false);

  nsresult RemoveContainer(nsINode* aNode);
  nsresult RemoveContainer(nsIDOMNode *inNode);
  nsresult InsertContainerAbove(nsIContent* aNode,
                                mozilla::dom::Element** aOutNode,
                                const nsAString& aNodeType,
                                const nsAString* aAttribute = nullptr,
                                const nsAString* aValue = nullptr);
  nsresult InsertContainerAbove(nsIDOMNode *inNode, 
                                nsCOMPtr<nsIDOMNode> *outNode, 
                                const nsAString &aNodeType,
                                const nsAString *aAttribute = nullptr,
                                const nsAString *aValue = nullptr);
  nsresult JoinNodes(nsINode* aNodeToKeep, nsIContent* aNodeToMove);
  nsresult MoveNode(nsINode* aNode, nsINode* aParent, int32_t aOffset);
  nsresult MoveNode(nsIDOMNode *aNode, nsIDOMNode *aParent, int32_t aOffset);

  




  nsresult CreateHTMLContent(const nsAString& aTag,
                             mozilla::dom::Element** aContent);

  
  virtual nsresult BeginIMEComposition();
  virtual nsresult UpdateIMEComposition(const nsAString &aCompositionString,
                                        nsIPrivateTextRangeList *aTextRange)=0;
  void EndIMEComposition();

  void SwitchTextDirectionTo(uint32_t aDirection);

protected:
  nsresult DetermineCurrentDirection();

  

  NS_IMETHOD CreateTxnForSetAttribute(mozilla::dom::Element *aElement,
                                      const nsAString &  aAttribute,
                                      const nsAString &  aValue,
                                      ChangeAttributeTxn ** aTxn);

  

  NS_IMETHOD CreateTxnForRemoveAttribute(mozilla::dom::Element *aElement,
                                         const nsAString &  aAttribute,
                                         ChangeAttributeTxn ** aTxn);

  

  NS_IMETHOD CreateTxnForCreateElement(const nsAString & aTag,
                                       nsINode         *aParent,
                                       int32_t         aPosition,
                                       CreateElementTxn ** aTxn);

  

  NS_IMETHOD CreateTxnForInsertElement(nsINode    * aNode,
                                       nsINode    * aParent,
                                       int32_t      aOffset,
                                       InsertElementTxn ** aTxn);

  

  nsresult CreateTxnForDeleteNode(nsINode* aNode, DeleteNodeTxn** aTxn);


  nsresult CreateTxnForDeleteSelection(EDirection aAction,
                                       EditAggregateTxn** aTxn,
                                       nsINode** aNode,
                                       int32_t* aOffset,
                                       int32_t* aLength);

  nsresult CreateTxnForDeleteInsertionPoint(nsRange* aRange, 
                                            EDirection aAction, 
                                            EditAggregateTxn* aTxn,
                                            nsINode** aNode,
                                            int32_t* aOffset,
                                            int32_t* aLength);


  


  NS_IMETHOD CreateTxnForInsertText(const nsAString & aStringToInsert,
                                    nsIDOMCharacterData *aTextNode,
                                    int32_t aOffset,
                                    InsertTextTxn ** aTxn);

  NS_IMETHOD CreateTxnForIMEText(const nsAString & aStringToInsert,
                                 IMETextTxn ** aTxn);

  

  NS_IMETHOD CreateTxnForAddStyleSheet(nsCSSStyleSheet* aSheet, AddStyleSheetTxn* *aTxn);

  

  NS_IMETHOD CreateTxnForRemoveStyleSheet(nsCSSStyleSheet* aSheet, RemoveStyleSheetTxn* *aTxn);
  
  NS_IMETHOD DeleteText(nsIDOMCharacterData *aElement,
                        uint32_t             aOffset,
                        uint32_t             aLength);



  nsresult CreateTxnForDeleteText(nsIDOMCharacterData* aElement,
                                  uint32_t             aOffset,
                                  uint32_t             aLength,
                                  DeleteTextTxn**      aTxn);

  nsresult CreateTxnForDeleteCharacter(nsIDOMCharacterData* aData,
                                       uint32_t             aOffset,
                                       EDirection           aDirection,
                                       DeleteTextTxn**      aTxn);
	
  NS_IMETHOD CreateTxnForSplitNode(nsINode *aNode,
                                   uint32_t    aOffset,
                                   SplitElementTxn **aTxn);

  NS_IMETHOD CreateTxnForJoinNode(nsINode  *aLeftNode,
                                  nsINode  *aRightNode,
                                  JoinElementTxn **aTxn);

  






  nsresult DeleteSelectionAndPrepareToCreateNode();


  
  void DoAfterDoTransaction(nsITransaction *aTxn);
  
  void DoAfterUndoTransaction();
  
  void DoAfterRedoTransaction();

  typedef enum {
    eDocumentCreated,
    eDocumentToBeDestroyed,
    eDocumentStateChanged
  } TDocumentListenerNotification;
  
  
  NS_IMETHOD NotifyDocumentListeners(TDocumentListenerNotification aNotificationType);
  
  
  NS_IMETHOD SelectEntireDocument(nsISelection *aSelection);

  









  NS_IMETHOD ScrollSelectionIntoView(bool aScrollToAnchor);

  
  bool IsBlockNode(nsIDOMNode* aNode);
  
  virtual bool IsBlockNode(nsINode* aNode);
  
  
  nsIContent* FindNextLeafNode(nsINode  *aCurrentNode,
                               bool      aGoForward,
                               bool      bNoBlockCrossing);

  
  virtual nsresult InstallEventListeners();

  virtual void CreateEventListeners();

  
  virtual void RemoveEventListeners();

  


  bool GetDesiredSpellCheckState();

  mozilla::WidgetKeyboardEvent* GetNativeKeyEvent(nsIDOMKeyEvent* aDOMKeyEvent);

  bool CanEnableSpellCheck()
  {
    
    
    return !IsPasswordEditor() && !IsReadonly() && !IsDisabled() && !ShouldSkipSpellCheck();
  }

public:

  

  NS_IMETHOD StartOperation(EditAction opID,
                            nsIEditor::EDirection aDirection);

  

  NS_IMETHOD EndOperation();

  

  bool     ArePreservingSelection();
  void     PreserveSelectionAcrossActions(mozilla::Selection* aSel);
  nsresult RestorePreservedSelection(nsISelection *aSel);
  void     StopPreservingSelection();

  






  nsresult SplitNodeImpl(nsIDOMNode *aExistingRightNode,
                         int32_t     aOffset,
                         nsIDOMNode *aNewLeftNode,
                         nsIDOMNode *aParent);

  






  nsresult JoinNodesImpl(nsINode* aNodeToKeep,
                         nsINode* aNodeToJoin,
                         nsINode* aParent);

  



  static int32_t GetChildOffset(nsIDOMNode *aChild,
                                nsIDOMNode *aParent);

  



  static already_AddRefed<nsIDOMNode> GetNodeLocation(nsIDOMNode* aChild,
                                                      int32_t* outOffset);
  static nsINode* GetNodeLocation(nsINode* aChild, int32_t* aOffset);

  





  static nsresult GetLengthOfDOMNode(nsIDOMNode *aNode, uint32_t &aCount);

  







  nsresult GetPriorNode(nsIDOMNode  *aCurrentNode, 
                        bool         aEditableNode,
                        nsCOMPtr<nsIDOMNode> *aResultNode,
                        bool         bNoBlockCrossing = false);
  nsIContent* GetPriorNode(nsINode* aCurrentNode, bool aEditableNode,
                           bool aNoBlockCrossing = false);

  
  nsresult GetPriorNode(nsIDOMNode  *aParentNode, 
                        int32_t      aOffset, 
                        bool         aEditableNode, 
                        nsCOMPtr<nsIDOMNode> *aResultNode,
                        bool         bNoBlockCrossing = false);
  nsIContent* GetPriorNode(nsINode* aParentNode,
                           int32_t aOffset,
                           bool aEditableNode,
                           bool aNoBlockCrossing = false);


  






  nsresult GetNextNode(nsIDOMNode  *aCurrentNode, 
                       bool         aEditableNode,
                       nsCOMPtr<nsIDOMNode> *aResultNode,
                       bool         bNoBlockCrossing = false);
  nsIContent* GetNextNode(nsINode* aCurrentNode,
                          bool aEditableNode,
                          bool bNoBlockCrossing = false);

  
  nsresult GetNextNode(nsIDOMNode  *aParentNode, 
                       int32_t      aOffset, 
                       bool         aEditableNode, 
                       nsCOMPtr<nsIDOMNode> *aResultNode,
                       bool         bNoBlockCrossing = false);
  nsIContent* GetNextNode(nsINode* aParentNode,
                          int32_t aOffset,
                          bool aEditableNode,
                          bool aNoBlockCrossing = false);

  
  nsIContent* FindNode(nsINode *aCurrentNode,
                       bool     aGoForward,
                       bool     aEditableNode,
                       bool     bNoBlockCrossing);
  



  nsIDOMNode* GetRightmostChild(nsIDOMNode* aCurrentNode,
                                bool bNoBlockCrossing = false);
  nsIContent* GetRightmostChild(nsINode *aCurrentNode,
                                bool     bNoBlockCrossing = false);

  



  nsIDOMNode* GetLeftmostChild(nsIDOMNode* aCurrentNode,
                               bool bNoBlockCrossing = false);
  nsIContent* GetLeftmostChild(nsINode *aCurrentNode,
                               bool     bNoBlockCrossing = false);

  
  static inline bool NodeIsType(nsIDOMNode *aNode, nsIAtom *aTag)
  {
    return GetTag(aNode) == aTag;
  }

  
  bool CanContain(nsIDOMNode* aParent, nsIDOMNode* aChild);
  bool CanContainTag(nsIDOMNode* aParent, nsIAtom* aTag);
  bool TagCanContain(nsIAtom* aParentTag, nsIDOMNode* aChild);
  virtual bool TagCanContainTag(nsIAtom* aParentTag, nsIAtom* aChildTag);

  
  bool IsRoot(nsIDOMNode* inNode);
  bool IsRoot(nsINode* inNode);
  bool IsEditorRoot(nsINode* aNode);

  
  bool IsDescendantOfRoot(nsIDOMNode* inNode);
  bool IsDescendantOfRoot(nsINode* inNode);
  bool IsDescendantOfEditorRoot(nsIDOMNode* aNode);
  bool IsDescendantOfEditorRoot(nsINode* aNode);

  
  virtual bool IsContainer(nsIDOMNode *aNode);

  
  bool IsEditable(nsIDOMNode *aNode);
  virtual bool IsEditable(nsIContent *aNode);

  
  bool IsMozEditorBogusNode(nsIContent *aNode);

  
  uint32_t CountEditableChildren(nsINode* aNode);
  
  
  nsINode* GetFirstEditableNode(nsINode* aRoot);

  int32_t GetIMEBufferLength();
  bool IsIMEComposing();    
  void SetIsIMEComposing(); 

  
  static nsresult GetTagString(nsIDOMNode *aNode, nsAString& outString);
  static nsIAtom *GetTag(nsIDOMNode *aNode);

  bool NodesSameType(nsIDOMNode *aNode1, nsIDOMNode *aNode2);
  virtual bool AreNodesSameType(nsIContent* aNode1, nsIContent* aNode2);

  static bool IsTextNode(nsIDOMNode *aNode);
  static bool IsTextNode(nsINode *aNode);
  
  static nsCOMPtr<nsIDOMNode> GetChildAt(nsIDOMNode *aParent, int32_t aOffset);
  static nsCOMPtr<nsIDOMNode> GetNodeAtRangeOffsetPoint(nsIDOMNode* aParentOrNode, int32_t aOffset);

  static nsresult GetStartNodeAndOffset(nsISelection *aSelection, nsIDOMNode **outStartNode, int32_t *outStartOffset);
  static nsresult GetStartNodeAndOffset(mozilla::Selection* aSelection,
                                        nsINode** aStartNode,
                                        int32_t* aStartOffset);
  static nsresult GetEndNodeAndOffset(nsISelection *aSelection, nsIDOMNode **outEndNode, int32_t *outEndOffset);
  static nsresult GetEndNodeAndOffset(mozilla::Selection* aSelection,
                                      nsINode** aEndNode,
                                      int32_t* aEndOffset);
#if DEBUG_JOE
  static void DumpNode(nsIDOMNode *aNode, int32_t indent=0);
#endif
  mozilla::Selection* GetSelection();

  
  
  nsresult CreateRange(nsIDOMNode *aStartParent, int32_t aStartOffset,
                       nsIDOMNode *aEndParent, int32_t aEndOffset,
                       nsIDOMRange **aRange);

  
  nsresult AppendNodeToSelectionAsRange(nsIDOMNode *aNode);
  
  nsresult ClearSelection();

  nsresult IsPreformatted(nsIDOMNode *aNode, bool *aResult);

  nsresult SplitNodeDeep(nsIDOMNode *aNode, 
                         nsIDOMNode *aSplitPointParent, 
                         int32_t aSplitPointOffset,
                         int32_t *outOffset,
                         bool    aNoEmptyContainers = false,
                         nsCOMPtr<nsIDOMNode> *outLeftNode = 0,
                         nsCOMPtr<nsIDOMNode> *outRightNode = 0);
  nsresult JoinNodeDeep(nsIDOMNode *aLeftNode, nsIDOMNode *aRightNode, nsCOMPtr<nsIDOMNode> *aOutJoinNode, int32_t *outOffset); 

  nsresult GetString(const nsAString& name, nsAString& value);

  void BeginUpdateViewBatch(void);
  virtual nsresult EndUpdateViewBatch(void);

  bool GetShouldTxnSetSelection();

  virtual nsresult HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent);

  nsresult HandleInlineSpellCheck(EditAction action,
                                    nsISelection *aSelection,
                                    nsIDOMNode *previousSelectedNode,
                                    int32_t previousSelectedOffset,
                                    nsIDOMNode *aStartNode,
                                    int32_t aStartOffset,
                                    nsIDOMNode *aEndNode,
                                    int32_t aEndOffset);

  virtual already_AddRefed<nsIDOMEventTarget> GetDOMEventTarget() = 0;

  
  mozilla::dom::Element *GetRoot();

  
  
  virtual mozilla::dom::Element* GetEditorRoot();

  
  bool IsPlaintextEditor() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorPlaintextMask) != 0;
  }

  bool IsSingleLineEditor() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorSingleLineMask) != 0;
  }

  bool IsPasswordEditor() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorPasswordMask) != 0;
  }

  bool IsReadonly() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorReadonlyMask) != 0;
  }

  bool IsDisabled() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorDisabledMask) != 0;
  }

  bool IsInputFiltered() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorFilterInputMask) != 0;
  }

  bool IsMailEditor() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorMailMask) != 0;
  }

  bool IsWrapHackEnabled() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorEnableWrapHackMask) != 0;
  }

  bool IsFormWidget() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorWidgetMask) != 0;
  }

  bool NoCSS() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorNoCSSMask) != 0;
  }

  bool IsInteractionAllowed() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorAllowInteraction) != 0;
  }

  bool DontEchoPassword() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorDontEchoPassword) != 0;
  }
  
  bool ShouldSkipSpellCheck() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorSkipSpellCheck) != 0;
  }

  bool IsTabbable() const
  {
    return IsSingleLineEditor() || IsPasswordEditor() || IsFormWidget() ||
           IsInteractionAllowed();
  }

  bool HasIndependentSelection() const
  {
    return !!mSelConWeak;
  }

  
  virtual already_AddRefed<nsIContent> GetInputEventTargetContent() = 0;

  
  virtual already_AddRefed<nsIContent> GetFocusedContent();

  
  
  virtual already_AddRefed<nsIContent> GetFocusedContentForIME();

  
  
  
  virtual bool IsActiveInDOMWindow();

  
  
  
  
  virtual bool IsAcceptableInputEvent(nsIDOMEvent* aEvent);

  
  
  
  
  
  virtual already_AddRefed<nsIContent> FindSelectionRoot(nsINode* aNode);

  
  
  
  nsresult InitializeSelection(nsIDOMEventTarget* aFocusEventTarget);

  
  void FinalizeSelection();

  
  
  
  void OnFocus(nsIDOMEventTarget* aFocusEventTarget);

  
  
  
  virtual nsresult InsertFromDataTransfer(nsIDOMDataTransfer *aDataTransfer,
                                          int32_t aIndex,
                                          nsIDOMDocument *aSourceDoc,
                                          nsIDOMNode *aDestinationNode,
                                          int32_t aDestOffset,
                                          bool aDoDeleteSelection) = 0;

  virtual nsresult InsertFromDrop(nsIDOMEvent* aDropEvent) = 0;

  virtual already_AddRefed<nsIDOMNode> FindUserSelectAllNode(nsIDOMNode* aNode) { return nullptr; }

protected:
  enum Tristate {
    eTriUnset,
    eTriFalse,
    eTriTrue
  };
  
  nsCString mContentMIMEType;       

  nsCOMPtr<nsIInlineSpellChecker> mInlineSpellChecker;

  nsRefPtr<nsTransactionManager> mTxnMgr;
  nsCOMPtr<mozilla::dom::Element> mRootElement; 
  nsCOMPtr<nsIPrivateTextRangeList> mIMETextRangeList; 
  nsCOMPtr<nsIDOMCharacterData>     mIMETextNode;      
  nsCOMPtr<nsIDOMEventTarget> mEventTarget; 
  nsCOMPtr<nsIDOMEventListener> mEventListener;
  nsWeakPtr        mSelConWeak;          
  nsWeakPtr        mPlaceHolderTxn;      
  nsWeakPtr        mDocWeak;             
  nsIAtom          *mPlaceHolderName;    
  nsSelectionState *mSelState;           
  nsString         *mPhonetic;

  
  nsCOMArray<nsIEditActionListener> mActionListeners;  
  nsCOMArray<nsIEditorObserver> mEditorObservers;  
  nsCOMArray<nsIDocumentStateListener> mDocStateListeners;

  nsSelectionState  mSavedSel;           
  nsRangeUpdater    mRangeUpdater;       

  uint32_t          mModCount;     
  uint32_t          mFlags;        

  int32_t           mUpdateCount;

  int32_t           mPlaceHolderBatch;   
  EditAction        mAction;             

  uint32_t          mIMETextOffset;    
  uint32_t          mIMEBufferLength;  

  EDirection        mDirection;          
  int8_t            mDocDirtyState;      
  uint8_t           mSpellcheckCheckboxState; 

  bool mInIMEMode;        
  bool mIsIMEComposing;   
                                                       

  bool mShouldTxnSetSelection;  
  bool mDidPreDestroy;    
  bool mDidPostCreate;    
  bool mDispatchInputEvent;

  friend bool NSCanUnload(nsISupports* serviceMgr);
  friend class nsAutoTxnsConserveSelection;
  friend class nsAutoSelectionReset;
  friend class nsAutoRules;
  friend class nsRangeUpdater;
};


#endif
