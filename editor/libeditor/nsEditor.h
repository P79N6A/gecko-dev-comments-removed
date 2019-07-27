




#ifndef __editor_h__
#define __editor_h__

#include "mozilla/Assertions.h"         
#include "mozilla/dom/OwningNonNull.h"  
#include "mozilla/dom/Text.h"
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsGkAtoms.h"
#include "nsIEditor.h"                  
#include "nsIEditorIMESupport.h"        
#include "nsIObserver.h"                
#include "nsIPhonetic.h"                
#include "nsIPlaintextEditor.h"         
#include "nsISelectionController.h"     
#include "nsISupportsImpl.h"            
#include "nsIWeakReferenceUtils.h"      
#include "nsLiteralString.h"            
#include "nsSelectionState.h"           
#include "nsString.h"                   
#include "nsWeakReference.h"            
#include "nscore.h"                     

class AddStyleSheetTxn;
class DeleteNodeTxn;
class EditAggregateTxn;
class RemoveStyleSheetTxn;
class nsIAtom;
class nsIContent;
class nsIDOMDocument;
class nsIDOMEvent;
class nsIDOMEventListener;
class nsIDOMEventTarget;
class nsIDOMKeyEvent;
class nsIDOMNode;
class nsIDocument;
class nsIDocumentStateListener;
class nsIEditActionListener;
class nsIEditorObserver;
class nsIInlineSpellChecker;
class nsINode;
class nsIPresShell;
class nsISupports;
class nsITransaction;
class nsIWidget;
class nsRange;
class nsString;
class nsTransactionManager;
struct DOMPoint;

namespace mozilla {
class CSSStyleSheet;
class ErrorResult;
class TextComposition;

namespace dom {
class ChangeAttributeTxn;
class CreateElementTxn;
class DataTransfer;
class DeleteTextTxn;
class Element;
class EventTarget;
class IMETextTxn;
class InsertTextTxn;
class InsertNodeTxn;
class JoinNodeTxn;
class Selection;
class SplitNodeTxn;
class Text;
}  
}  

namespace mozilla {
namespace widget {
struct IMEState;
} 
} 

#define kMOZEditorBogusNodeAttrAtom nsGkAtoms::mozeditorbogusnode
#define kMOZEditorBogusNodeValue NS_LITERAL_STRING("TRUE")



enum class EditAction : int32_t {
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
};

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

protected:
  


  virtual ~nsEditor();

public:


  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsEditor,
                                           nsIEditor)

  
  already_AddRefed<nsIDOMDocument> GetDOMDocument();
  already_AddRefed<nsIDocument> GetDocument();
  already_AddRefed<nsIPresShell> GetPresShell();
  already_AddRefed<nsIWidget> GetWidget();
  enum NotificationForEditorObservers
  {
    eNotifyEditorObserversOfEnd,
    eNotifyEditorObserversOfBefore,
    eNotifyEditorObserversOfCancel
  };
  void NotifyEditorObservers(NotificationForEditorObservers aNotification);

  
  NS_DECL_NSIEDITOR

  
  NS_DECL_NSIEDITORIMESUPPORT

  
  NS_DECL_NSIOBSERVER

  
  NS_DECL_NSIPHONETIC

public:

  virtual bool IsModifiableNode(nsINode *aNode);

  virtual nsresult InsertTextImpl(const nsAString& aStringToInsert,
                                  nsCOMPtr<nsINode>* aInOutNode,
                                  int32_t* aInOutOffset,
                                  nsIDocument* aDoc);
  nsresult InsertTextIntoTextNodeImpl(const nsAString& aStringToInsert,
                                      mozilla::dom::Text& aTextNode,
                                      int32_t aOffset,
                                      bool aSuppressIME = false);
  NS_IMETHOD DeleteSelectionImpl(EDirection aAction,
                                 EStripWrappers aStripWrappers);

  already_AddRefed<mozilla::dom::Element>
  DeleteSelectionAndCreateElement(nsIAtom& aTag);

  
  nsresult DeleteNode(nsINode* aNode);
  nsresult InsertNode(nsIContent& aNode, nsINode& aParent, int32_t aPosition);
  enum ECloneAttributes { eDontCloneAttributes, eCloneAttributes };
  already_AddRefed<mozilla::dom::Element> ReplaceContainer(
                            mozilla::dom::Element* aOldContainer,
                            nsIAtom* aNodeType,
                            nsIAtom* aAttribute = nullptr,
                            const nsAString* aValue = nullptr,
                            ECloneAttributes aCloneAttributes = eDontCloneAttributes);
  void CloneAttributes(mozilla::dom::Element* aDest,
                       mozilla::dom::Element* aSource);

  nsresult RemoveContainer(nsIContent* aNode);
  already_AddRefed<mozilla::dom::Element> InsertContainerAbove(
                                nsIContent* aNode,
                                nsIAtom* aNodeType,
                                nsIAtom* aAttribute = nullptr,
                                const nsAString* aValue = nullptr);
  nsIContent* SplitNode(nsIContent& aNode, int32_t aOffset,
                        mozilla::ErrorResult& aResult);
  nsresult JoinNodes(nsINode& aLeftNode, nsINode& aRightNode);
  nsresult MoveNode(nsIContent* aNode, nsINode* aParent, int32_t aOffset);

  



  already_AddRefed<mozilla::dom::Element> CreateHTMLContent(nsIAtom* aTag);

  
  virtual nsresult BeginIMEComposition(mozilla::WidgetCompositionEvent* aEvent);
  virtual nsresult UpdateIMEComposition(nsIDOMEvent* aDOMTextEvent) = 0;
  void EndIMEComposition();

  void SwitchTextDirectionTo(uint32_t aDirection);

protected:
  nsresult DetermineCurrentDirection();
  void FireInputEvent();

  


  already_AddRefed<mozilla::dom::ChangeAttributeTxn>
  CreateTxnForSetAttribute(mozilla::dom::Element& aElement,
                           nsIAtom& aAttribute, const nsAString& aValue);

  


  already_AddRefed<mozilla::dom::ChangeAttributeTxn>
  CreateTxnForRemoveAttribute(mozilla::dom::Element& aElement,
                              nsIAtom& aAttribute);

  

  already_AddRefed<mozilla::dom::CreateElementTxn>
  CreateTxnForCreateElement(nsIAtom& aTag,
                            nsINode& aParent,
                            int32_t aPosition);

  already_AddRefed<mozilla::dom::Element> CreateNode(nsIAtom* aTag,
                                                     nsINode* aParent,
                                                     int32_t aPosition);

  

  already_AddRefed<mozilla::dom::InsertNodeTxn>
  CreateTxnForInsertNode(nsIContent& aNode, nsINode& aParent, int32_t aOffset);

  

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


  


  already_AddRefed<mozilla::dom::InsertTextTxn>
  CreateTxnForInsertText(const nsAString& aStringToInsert,
                         mozilla::dom::Text& aTextNode, int32_t aOffset);

  
  already_AddRefed<mozilla::dom::IMETextTxn>
  CreateTxnForIMEText(const nsAString & aStringToInsert);

  

  NS_IMETHOD CreateTxnForAddStyleSheet(mozilla::CSSStyleSheet* aSheet,
                                       AddStyleSheetTxn* *aTxn);

  

  NS_IMETHOD CreateTxnForRemoveStyleSheet(mozilla::CSSStyleSheet* aSheet,
                                          RemoveStyleSheetTxn* *aTxn);
  
  nsresult DeleteText(nsGenericDOMDataNode& aElement,
                      uint32_t aOffset, uint32_t aLength);



  already_AddRefed<mozilla::dom::DeleteTextTxn>
  CreateTxnForDeleteText(nsGenericDOMDataNode& aElement,
                         uint32_t aOffset, uint32_t aLength);

  already_AddRefed<mozilla::dom::DeleteTextTxn>
  CreateTxnForDeleteCharacter(nsGenericDOMDataNode& aData, uint32_t aOffset,
                              EDirection aDirection);
	
  already_AddRefed<mozilla::dom::SplitNodeTxn>
  CreateTxnForSplitNode(nsIContent& aNode, uint32_t aOffset);

  already_AddRefed<mozilla::dom::JoinNodeTxn>
  CreateTxnForJoinNode(nsINode& aLeftNode, nsINode& aRightNode);

  






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
  
  
  virtual nsresult SelectEntireDocument(mozilla::dom::Selection* aSelection);

  









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

  bool CanEnableSpellCheck()
  {
    
    
    return !IsPasswordEditor() && !IsReadonly() && !IsDisabled() && !ShouldSkipSpellCheck();
  }

  



  void EnsureComposition(mozilla::WidgetGUIEvent* aEvent);

  nsresult GetSelection(int16_t aSelectionType, nsISelection** aSelection);

public:

  

  NS_IMETHOD StartOperation(EditAction opID,
                            nsIEditor::EDirection aDirection);

  

  NS_IMETHOD EndOperation();

  

  bool     ArePreservingSelection();
  void     PreserveSelectionAcrossActions(mozilla::dom::Selection* aSel);
  nsresult RestorePreservedSelection(mozilla::dom::Selection* aSel);
  void     StopPreservingSelection();

  









  nsresult SplitNodeImpl(nsIContent& aExistingRightNode,
                         int32_t aOffset,
                         nsIContent& aNewLeftNode);

  






  nsresult JoinNodesImpl(nsINode* aNodeToKeep,
                         nsINode* aNodeToJoin,
                         nsINode* aParent);

  



  static int32_t GetChildOffset(nsIDOMNode *aChild,
                                nsIDOMNode *aParent);

  



  static already_AddRefed<nsIDOMNode> GetNodeLocation(nsIDOMNode* aChild,
                                                      int32_t* outOffset);
  static nsINode* GetNodeLocation(nsINode* aChild, int32_t* aOffset);

  





  static nsresult GetLengthOfDOMNode(nsIDOMNode *aNode, uint32_t &aCount);

  







  nsIContent* GetPriorNode(nsINode* aCurrentNode, bool aEditableNode,
                           bool aNoBlockCrossing = false);

  
  nsIContent* GetPriorNode(nsINode* aParentNode,
                           int32_t aOffset,
                           bool aEditableNode,
                           bool aNoBlockCrossing = false);


  






  nsIContent* GetNextNode(nsINode* aCurrentNode,
                          bool aEditableNode,
                          bool bNoBlockCrossing = false);

  
  nsIContent* GetNextNode(nsINode* aParentNode,
                          int32_t aOffset,
                          bool aEditableNode,
                          bool aNoBlockCrossing = false);

  
  nsIContent* FindNode(nsINode *aCurrentNode,
                       bool     aGoForward,
                       bool     aEditableNode,
                       bool     bNoBlockCrossing);
  



  nsIContent* GetRightmostChild(nsINode *aCurrentNode,
                                bool     bNoBlockCrossing = false);

  



  nsIContent* GetLeftmostChild(nsINode *aCurrentNode,
                               bool     bNoBlockCrossing = false);

  
  static inline bool NodeIsType(nsIDOMNode *aNode, nsIAtom *aTag)
  {
    return GetTag(aNode) == aTag;
  }

  
  bool CanContain(nsINode& aParent, nsIContent& aChild);
  bool CanContainTag(nsINode& aParent, nsIAtom& aTag);
  bool TagCanContain(nsIAtom& aParentTag, nsIContent& aChild);
  virtual bool TagCanContainTag(nsIAtom& aParentTag, nsIAtom& aChildTag);

  
  bool IsRoot(nsIDOMNode* inNode);
  bool IsRoot(nsINode* inNode);
  bool IsEditorRoot(nsINode* aNode);

  
  bool IsDescendantOfRoot(nsIDOMNode* inNode);
  bool IsDescendantOfRoot(nsINode* inNode);
  bool IsDescendantOfEditorRoot(nsIDOMNode* aNode);
  bool IsDescendantOfEditorRoot(nsINode* aNode);

  
  virtual bool IsContainer(nsINode* aNode);
  virtual bool IsContainer(nsIDOMNode* aNode);

  
  bool IsEditable(nsIDOMNode *aNode);
  virtual bool IsEditable(nsINode* aNode);

  
  bool IsMozEditorBogusNode(nsINode* aNode);

  
  uint32_t CountEditableChildren(nsINode* aNode);
  
  
  nsINode* GetFirstEditableNode(nsINode* aRoot);

  


  mozilla::TextComposition* GetComposition() const;
  


  bool IsIMEComposing() const;

  
  static nsresult GetTagString(nsIDOMNode *aNode, nsAString& outString);
  static nsIAtom *GetTag(nsIDOMNode *aNode);

  bool NodesSameType(nsIDOMNode *aNode1, nsIDOMNode *aNode2);
  virtual bool AreNodesSameType(nsIContent* aNode1, nsIContent* aNode2);

  static bool IsTextNode(nsIDOMNode *aNode);
  static bool IsTextNode(nsINode *aNode);
  
  static nsCOMPtr<nsIDOMNode> GetChildAt(nsIDOMNode *aParent, int32_t aOffset);
  static nsCOMPtr<nsIDOMNode> GetNodeAtRangeOffsetPoint(nsIDOMNode* aParentOrNode, int32_t aOffset);

  static nsresult GetStartNodeAndOffset(mozilla::dom::Selection* aSelection,
                                        nsIDOMNode** outStartNode,
                                        int32_t* outStartOffset);
  static nsresult GetStartNodeAndOffset(mozilla::dom::Selection* aSelection,
                                        nsINode** aStartNode,
                                        int32_t* aStartOffset);
  static nsresult GetEndNodeAndOffset(mozilla::dom::Selection* aSelection,
                                      nsIDOMNode** outEndNode,
                                      int32_t* outEndOffset);
  static nsresult GetEndNodeAndOffset(mozilla::dom::Selection* aSelection,
                                      nsINode** aEndNode,
                                      int32_t* aEndOffset);
#if DEBUG_JOE
  static void DumpNode(nsIDOMNode *aNode, int32_t indent=0);
#endif
  mozilla::dom::Selection* GetSelection(int16_t aSelectionType =
      nsISelectionController::SELECTION_NORMAL);

  
  
  nsresult CreateRange(nsIDOMNode *aStartParent, int32_t aStartOffset,
                       nsIDOMNode *aEndParent, int32_t aEndOffset,
                       nsRange** aRange);

  
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
  ::DOMPoint JoinNodeDeep(nsIContent& aLeftNode, nsIContent& aRightNode);

  nsresult GetString(const nsAString& name, nsAString& value);

  void BeginUpdateViewBatch(void);
  virtual nsresult EndUpdateViewBatch(void);

  bool GetShouldTxnSetSelection();

  virtual nsresult HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent);

  nsresult HandleInlineSpellCheck(EditAction action,
                                  mozilla::dom::Selection* aSelection,
                                    nsIDOMNode *previousSelectedNode,
                                    int32_t previousSelectedOffset,
                                    nsIDOMNode *aStartNode,
                                    int32_t aStartOffset,
                                    nsIDOMNode *aEndNode,
                                    int32_t aEndOffset);

  virtual already_AddRefed<mozilla::dom::EventTarget> GetDOMEventTarget() = 0;

  
  mozilla::dom::Element *GetRoot();

  
  
  virtual mozilla::dom::Element* GetEditorRoot();

  
  
  mozilla::dom::Element* GetExposedRoot();

  
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

  
  
  
  void OnFocus(nsIDOMEventTarget* aFocusEventTarget);

  
  
  
  virtual nsresult InsertFromDataTransfer(mozilla::dom::DataTransfer *aDataTransfer,
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
  nsRefPtr<mozilla::dom::Text>    mIMETextNode; 
  nsCOMPtr<mozilla::dom::EventTarget> mEventTarget; 
  nsCOMPtr<nsIDOMEventListener> mEventListener;
  nsWeakPtr        mSelConWeak;          
  nsWeakPtr        mPlaceHolderTxn;      
  nsWeakPtr        mDocWeak;             
  nsIAtom          *mPlaceHolderName;    
  nsSelectionState *mSelState;           
  nsString         *mPhonetic;
  
  
  nsRefPtr<mozilla::TextComposition> mComposition;

  
  
  nsTArray<mozilla::dom::OwningNonNull<nsIEditActionListener>> mActionListeners;
  
  nsTArray<mozilla::dom::OwningNonNull<nsIEditorObserver>> mEditorObservers;
  
  nsTArray<mozilla::dom::OwningNonNull<nsIDocumentStateListener>> mDocStateListeners;

  nsSelectionState  mSavedSel;           
  nsRangeUpdater    mRangeUpdater;       

  uint32_t          mModCount;     
  uint32_t          mFlags;        

  int32_t           mUpdateCount;

  int32_t           mPlaceHolderBatch;   
  EditAction        mAction;             

  uint32_t          mIMETextOffset;    

  EDirection        mDirection;          
  int8_t            mDocDirtyState;      
  uint8_t           mSpellcheckCheckboxState; 

  bool mShouldTxnSetSelection;  
  bool mDidPreDestroy;    
  bool mDidPostCreate;    
  bool mDispatchInputEvent;
  bool mIsInEditAction;   

  friend bool NSCanUnload(nsISupports* serviceMgr);
  friend class nsAutoTxnsConserveSelection;
  friend class nsAutoSelectionReset;
  friend class nsAutoRules;
  friend class nsRangeUpdater;
};


#endif
