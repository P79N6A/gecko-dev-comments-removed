




#ifndef __editor_h__
#define __editor_h__

#include "nsCOMPtr.h"
#include "nsWeakReference.h"

#include "nsIEditor.h"
#include "nsIPlaintextEditor.h"
#include "nsIEditorIMESupport.h"
#include "nsIPhonetic.h"

#include "nsIAtom.h"
#include "nsIDOMDocument.h"
#include "mozilla/Selection.h"
#include "nsIDOMCharacterData.h"
#include "nsIPrivateTextRange.h"
#include "nsITransactionManager.h"
#include "nsIComponentManager.h"
#include "nsCOMArray.h"
#include "nsIEditActionListener.h"
#include "nsIEditorObserver.h"
#include "nsIDocumentStateListener.h"
#include "nsIDOMElement.h"
#include "nsSelectionState.h"
#include "nsIEditorSpellCheck.h"
#include "nsIInlineSpellChecker.h"
#include "nsIDOMEventTarget.h"
#include "nsStubMutationObserver.h"
#include "nsIViewManager.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIObserver.h"

class nsIDOMCharacterData;
class nsIDOMRange;
class nsIPresShell;
class ChangeAttributeTxn;
class CreateElementTxn;
class InsertElementTxn;
class DeleteElementTxn;
class InsertTextTxn;
class DeleteTextTxn;
class SplitElementTxn;
class JoinElementTxn;
class EditAggregateTxn;
class IMETextTxn;
class AddStyleSheetTxn;
class RemoveStyleSheetTxn;
class nsIFile;
class nsISelectionController;
class nsIDOMEventTarget;
class nsCSSStyleSheet;
class nsKeyEvent;
class nsIDOMNSEvent;

namespace mozilla {
namespace widget {
struct IMEState;
} 
} 

#define kMOZEditorBogusNodeAttrAtom nsEditProperty::mozEditorBogusNode
#define kMOZEditorBogusNodeValue NS_LITERAL_STRING("TRUE")






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

  enum OperationID
  {
    kOpIgnore = -1,
    kOpNone = 0,
    kOpUndo,
    kOpRedo,
    kOpInsertNode,
    kOpCreateNode,
    kOpDeleteNode,
    kOpSplitNode,
    kOpJoinNode,
    kOpDeleteText = 1003,

    
    kOpInsertText         = 2000,
    kOpInsertIMEText      = 2001,
    kOpDeleteSelection    = 2002,
    kOpSetTextProperty    = 2003,
    kOpRemoveTextProperty = 2004,
    kOpOutputText         = 2005,

    
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
    kOpHTMLPaste           = 3012,
    kOpLoadHTML            = 3013,
    kOpResetTextProperties = 3014,
    kOpSetAbsolutePosition = 3015,
    kOpRemoveAbsolutePosition = 3016,
    kOpDecreaseZIndex      = 3017,
    kOpIncreaseZIndex      = 3018
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

  virtual bool IsModifiableNode(nsINode *aNode);
  
  NS_IMETHOD InsertTextImpl(const nsAString& aStringToInsert, 
                               nsCOMPtr<nsIDOMNode> *aInOutNode, 
                               PRInt32 *aInOutOffset,
                               nsIDOMDocument *aDoc);
  nsresult InsertTextIntoTextNodeImpl(const nsAString& aStringToInsert, 
                                      nsIDOMCharacterData *aTextNode, 
                                      PRInt32 aOffset,
                                      bool aSuppressIME = false);
  NS_IMETHOD DeleteSelectionImpl(EDirection aAction,
                                 EStripWrappers aStripWrappers);
  NS_IMETHOD DeleteSelectionAndCreateNode(const nsAString& aTag,
                                           nsIDOMNode ** aNewNode);

  
  nsresult ReplaceContainer(nsINode* inNode,
                            mozilla::dom::Element** outNode,
                            const nsAString& aNodeType,
                            const nsAString* aAttribute = nsnull,
                            const nsAString* aValue = nsnull,
                            bool aCloneAttributes = false);
  nsresult ReplaceContainer(nsIDOMNode *inNode, 
                            nsCOMPtr<nsIDOMNode> *outNode, 
                            const nsAString &aNodeType,
                            const nsAString *aAttribute = nsnull,
                            const nsAString *aValue = nsnull,
                            bool aCloneAttributes = false);

  nsresult RemoveContainer(nsINode* aNode);
  nsresult RemoveContainer(nsIDOMNode *inNode);
  nsresult InsertContainerAbove(nsIContent* aNode,
                                mozilla::dom::Element** aOutNode,
                                const nsAString& aNodeType,
                                const nsAString* aAttribute = nsnull,
                                const nsAString* aValue = nsnull);
  nsresult InsertContainerAbove(nsIDOMNode *inNode, 
                                nsCOMPtr<nsIDOMNode> *outNode, 
                                const nsAString &aNodeType,
                                const nsAString *aAttribute = nsnull,
                                const nsAString *aValue = nsnull);
  nsresult JoinNodes(nsINode* aNodeToKeep, nsIContent* aNodeToMove);
  nsresult MoveNode(nsIContent* aNode, nsINode* aParent, PRInt32 aOffset);
  nsresult MoveNode(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aOffset);

  




  nsresult CreateHTMLContent(const nsAString& aTag,
                             mozilla::dom::Element** aContent);

  
  virtual nsresult BeginIMEComposition();
  virtual nsresult UpdateIMEComposition(const nsAString &aCompositionString,
                                        nsIPrivateTextRangeList *aTextRange)=0;
  nsresult EndIMEComposition();

  void SwitchTextDirectionTo(PRUint32 aDirection);

protected:
  nsresult DetermineCurrentDirection();

  

  NS_IMETHOD CreateTxnForSetAttribute(nsIDOMElement *aElement, 
                                      const nsAString &  aAttribute, 
                                      const nsAString &  aValue,
                                      ChangeAttributeTxn ** aTxn);

  

  NS_IMETHOD CreateTxnForRemoveAttribute(nsIDOMElement *aElement, 
                                         const nsAString &  aAttribute,
                                         ChangeAttributeTxn ** aTxn);

  

  NS_IMETHOD CreateTxnForCreateElement(const nsAString & aTag,
                                       nsIDOMNode     *aParent,
                                       PRInt32         aPosition,
                                       CreateElementTxn ** aTxn);

  

  NS_IMETHOD CreateTxnForInsertElement(nsIDOMNode * aNode,
                                       nsIDOMNode * aParent,
                                       PRInt32      aOffset,
                                       InsertElementTxn ** aTxn);

  

  NS_IMETHOD CreateTxnForDeleteElement(nsIDOMNode * aElement,
                                       DeleteElementTxn ** aTxn);


  nsresult CreateTxnForDeleteSelection(EDirection aAction,
                                       EditAggregateTxn** aTxn,
                                       nsINode** aNode,
                                       PRInt32* aOffset,
                                       PRInt32* aLength);

  nsresult CreateTxnForDeleteInsertionPoint(nsRange* aRange, 
                                            EDirection aAction, 
                                            EditAggregateTxn* aTxn,
                                            nsINode** aNode,
                                            PRInt32* aOffset,
                                            PRInt32* aLength);


  


  NS_IMETHOD CreateTxnForInsertText(const nsAString & aStringToInsert,
                                    nsIDOMCharacterData *aTextNode,
                                    PRInt32 aOffset,
                                    InsertTextTxn ** aTxn);

  NS_IMETHOD CreateTxnForIMEText(const nsAString & aStringToInsert,
                                 IMETextTxn ** aTxn);

  

  NS_IMETHOD CreateTxnForAddStyleSheet(nsCSSStyleSheet* aSheet, AddStyleSheetTxn* *aTxn);

  

  NS_IMETHOD CreateTxnForRemoveStyleSheet(nsCSSStyleSheet* aSheet, RemoveStyleSheetTxn* *aTxn);
  
  NS_IMETHOD DeleteText(nsIDOMCharacterData *aElement,
                        PRUint32             aOffset,
                        PRUint32             aLength);



  nsresult CreateTxnForDeleteText(nsIDOMCharacterData* aElement,
                                  PRUint32             aOffset,
                                  PRUint32             aLength,
                                  DeleteTextTxn**      aTxn);

  nsresult CreateTxnForDeleteCharacter(nsIDOMCharacterData* aData,
                                       PRUint32             aOffset,
                                       EDirection           aDirection,
                                       DeleteTextTxn**      aTxn);
	
  NS_IMETHOD CreateTxnForSplitNode(nsIDOMNode *aNode,
                                   PRUint32    aOffset,
                                   SplitElementTxn **aTxn);

  NS_IMETHOD CreateTxnForJoinNode(nsIDOMNode  *aLeftNode,
                                  nsIDOMNode  *aRightNode,
                                  JoinElementTxn **aTxn);

  






  nsresult DeleteSelectionAndPrepareToCreateNode();


  
  NS_IMETHOD DoAfterDoTransaction(nsITransaction *aTxn);
  
  NS_IMETHOD DoAfterUndoTransaction();
  
  NS_IMETHOD DoAfterRedoTransaction();

  typedef enum {
    eDocumentCreated,
    eDocumentToBeDestroyed,
    eDocumentStateChanged
  } TDocumentListenerNotification;
  
  
  NS_IMETHOD NotifyDocumentListeners(TDocumentListenerNotification aNotificationType);
  
  
  NS_IMETHOD SelectEntireDocument(nsISelection *aSelection);

  









  NS_IMETHOD ScrollSelectionIntoView(bool aScrollToAnchor);

  
  virtual bool IsBlockNode(nsIDOMNode *aNode);
  virtual bool IsBlockNode(nsINode *aNode);
  
  
  nsIContent* FindNextLeafNode(nsINode  *aCurrentNode,
                               bool      aGoForward,
                               bool      bNoBlockCrossing);

  
  nsresult GetWidget(nsIWidget **aWidget);


  
  virtual nsresult InstallEventListeners();

  virtual void CreateEventListeners();

  
  virtual void RemoveEventListeners();

  


  bool GetDesiredSpellCheckState();

  nsKeyEvent* GetNativeKeyEvent(nsIDOMKeyEvent* aDOMKeyEvent);

  bool CanEnableSpellCheck()
  {
    
    
    return !IsPasswordEditor() && !IsReadonly() && !IsDisabled() && !ShouldSkipSpellCheck();
  }

public:

  

  NS_IMETHOD StartOperation(OperationID opID,
                            nsIEditor::EDirection aDirection);

  

  NS_IMETHOD EndOperation();

  

  bool     ArePreservingSelection();
  void     PreserveSelectionAcrossActions(nsISelection *aSel);
  nsresult RestorePreservedSelection(nsISelection *aSel);
  void     StopPreservingSelection();

  






  nsresult SplitNodeImpl(nsIDOMNode *aExistingRightNode,
                         PRInt32     aOffset,
                         nsIDOMNode *aNewLeftNode,
                         nsIDOMNode *aParent);

  








  nsresult JoinNodesImpl(nsIDOMNode *aNodeToKeep,
                         nsIDOMNode *aNodeToJoin,
                         nsIDOMNode *aParent,
                         bool        aNodeToKeepIsFirst);

  



  static nsresult GetChildOffset(nsIDOMNode *aChild, 
                                 nsIDOMNode *aParent, 
                                 PRInt32    &aOffset);

  



  static nsresult GetNodeLocation(nsIDOMNode *aChild, 
                                 nsCOMPtr<nsIDOMNode> *aParent, 
                                 PRInt32    *aOffset);

  





  static nsresult GetLengthOfDOMNode(nsIDOMNode *aNode, PRUint32 &aCount);

  







  nsresult GetPriorNode(nsIDOMNode  *aCurrentNode, 
                        bool         aEditableNode,
                        nsCOMPtr<nsIDOMNode> *aResultNode,
                        bool         bNoBlockCrossing = false);
  nsIContent* GetPriorNode(nsINode* aCurrentNode, bool aEditableNode,
                           bool aNoBlockCrossing = false);

  
  nsresult GetPriorNode(nsIDOMNode  *aParentNode, 
                        PRInt32      aOffset, 
                        bool         aEditableNode, 
                        nsCOMPtr<nsIDOMNode> *aResultNode,
                        bool         bNoBlockCrossing = false);
  nsIContent* GetPriorNode(nsINode* aParentNode,
                           PRInt32 aOffset,
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
                       PRInt32      aOffset, 
                       bool         aEditableNode, 
                       nsCOMPtr<nsIDOMNode> *aResultNode,
                       bool         bNoBlockCrossing = false);
  nsIContent* GetNextNode(nsINode* aParentNode,
                          PRInt32 aOffset,
                          bool aEditableNode,
                          bool aNoBlockCrossing = false);

  
  nsIContent* FindNode(nsINode *aCurrentNode,
                       bool     aGoForward,
                       bool     aEditableNode,
                       bool     bNoBlockCrossing);
  



  already_AddRefed<nsIDOMNode> GetRightmostChild(nsIDOMNode *aCurrentNode, 
                                                 bool        bNoBlockCrossing = false);
  nsIContent* GetRightmostChild(nsINode *aCurrentNode,
                                bool     bNoBlockCrossing = false);

  



  already_AddRefed<nsIDOMNode> GetLeftmostChild(nsIDOMNode  *aCurrentNode, 
                                                bool        bNoBlockCrossing = false);
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

  


  virtual bool IsTextInDirtyFrameVisible(nsIContent *aNode);

  
  bool IsMozEditorBogusNode(nsIContent *aNode);

  
  PRUint32 CountEditableChildren(nsINode* aNode);
  
  
  nsINode* GetFirstEditableNode(nsINode* aRoot);

  PRInt32 GetIMEBufferLength();
  bool IsIMEComposing();    
  void SetIsIMEComposing(); 

  
  static nsresult GetTagString(nsIDOMNode *aNode, nsAString& outString);
  static nsIAtom *GetTag(nsIDOMNode *aNode);

  bool NodesSameType(nsIDOMNode *aNode1, nsIDOMNode *aNode2);
  virtual bool AreNodesSameType(nsIContent* aNode1, nsIContent* aNode2);

  static bool IsTextOrElementNode(nsIDOMNode *aNode);
  static bool IsTextNode(nsIDOMNode *aNode);
  static bool IsTextNode(nsINode *aNode);
  
  static nsCOMPtr<nsIDOMNode> GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset);
  static nsCOMPtr<nsIDOMNode> GetNodeAtRangeOffsetPoint(nsIDOMNode* aParentOrNode, PRInt32 aOffset);

  static nsresult GetStartNodeAndOffset(nsISelection *aSelection, nsIDOMNode **outStartNode, PRInt32 *outStartOffset);
  static nsresult GetEndNodeAndOffset(nsISelection *aSelection, nsIDOMNode **outEndNode, PRInt32 *outEndOffset);
#if DEBUG_JOE
  static void DumpNode(nsIDOMNode *aNode, PRInt32 indent=0);
#endif
  mozilla::Selection* GetSelection();

  
  
  nsresult CreateRange(nsIDOMNode *aStartParent, PRInt32 aStartOffset,
                       nsIDOMNode *aEndParent, PRInt32 aEndOffset,
                       nsIDOMRange **aRange);

  
  nsresult AppendNodeToSelectionAsRange(nsIDOMNode *aNode);
  
  nsresult ClearSelection();

  nsresult IsPreformatted(nsIDOMNode *aNode, bool *aResult);

  nsresult SplitNodeDeep(nsIDOMNode *aNode, 
                         nsIDOMNode *aSplitPointParent, 
                         PRInt32 aSplitPointOffset,
                         PRInt32 *outOffset,
                         bool    aNoEmptyContainers = false,
                         nsCOMPtr<nsIDOMNode> *outLeftNode = 0,
                         nsCOMPtr<nsIDOMNode> *outRightNode = 0);
  nsresult JoinNodeDeep(nsIDOMNode *aLeftNode, nsIDOMNode *aRightNode, nsCOMPtr<nsIDOMNode> *aOutJoinNode, PRInt32 *outOffset); 

  nsresult GetString(const nsAString& name, nsAString& value);

  void BeginUpdateViewBatch(void);
  virtual nsresult EndUpdateViewBatch(void);

  bool GetShouldTxnSetSelection();

  virtual nsresult HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent);

  nsresult HandleInlineSpellCheck(OperationID action,
                                    nsISelection *aSelection,
                                    nsIDOMNode *previousSelectedNode,
                                    PRInt32 previousSelectedOffset,
                                    nsIDOMNode *aStartNode,
                                    PRInt32 aStartOffset,
                                    nsIDOMNode *aEndNode,
                                    PRInt32 aEndOffset);

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

  
  virtual already_AddRefed<nsIContent> GetInputEventTargetContent() = 0;

  
  virtual already_AddRefed<nsIContent> GetFocusedContent();

  
  
  
  virtual bool IsActiveInDOMWindow();

  
  
  
  
  virtual bool IsAcceptableInputEvent(nsIDOMEvent* aEvent);

  
  
  
  
  
  virtual already_AddRefed<nsIContent> FindSelectionRoot(nsINode* aNode);

  
  
  
  nsresult InitializeSelection(nsIDOMEventTarget* aFocusEventTarget);

  
  
  
  void OnFocus(nsIDOMEventTarget* aFocusEventTarget);

  
  
  
  virtual nsresult InsertFromDataTransfer(nsIDOMDataTransfer *aDataTransfer,
                                          PRInt32 aIndex,
                                          nsIDOMDocument *aSourceDoc,
                                          nsIDOMNode *aDestinationNode,
                                          PRInt32 aDestOffset,
                                          bool aDoDeleteSelection) = 0;

  virtual nsresult InsertFromDrop(nsIDOMEvent* aDropEvent) = 0;

  virtual already_AddRefed<nsIDOMNode> FindUserSelectAllNode(nsIDOMNode* aNode) { return nsnull; }

  NS_STACK_CLASS class HandlingTrustedAction
  {
  public:
    explicit HandlingTrustedAction(nsEditor* aSelf, bool aIsTrusted = true)
    {
      Init(aSelf, aIsTrusted);
    }

    HandlingTrustedAction(nsEditor* aSelf, nsIDOMNSEvent* aEvent);

    ~HandlingTrustedAction()
    {
      mEditor->mHandlingTrustedAction = mWasHandlingTrustedAction;
      mEditor->mHandlingActionCount--;
    }

  private:
    nsRefPtr<nsEditor> mEditor;
    bool mWasHandlingTrustedAction;

    void Init(nsEditor* aSelf, bool aIsTrusted)
    {
      MOZ_ASSERT(aSelf);

      mEditor = aSelf;
      mWasHandlingTrustedAction = aSelf->mHandlingTrustedAction;
      if (aIsTrusted) {
        
        
        if (aSelf->mHandlingActionCount == 0) {
          aSelf->mHandlingTrustedAction = true;
        }
      } else {
        aSelf->mHandlingTrustedAction = false;
      }
      aSelf->mHandlingActionCount++;
    }
  };

protected:
  enum Tristate {
    eTriUnset,
    eTriFalse,
    eTriTrue
  };
  
  nsCString mContentMIMEType;       

  nsCOMPtr<nsIInlineSpellChecker> mInlineSpellChecker;

  nsCOMPtr<nsITransactionManager> mTxnMgr;
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

  PRUint32          mModCount;     
  PRUint32          mFlags;        

  PRInt32           mUpdateCount;

  PRInt32           mPlaceHolderBatch;   
  OperationID       mAction;             
  PRUint32          mHandlingActionCount;

  PRUint32          mIMETextOffset;    
  PRUint32          mIMEBufferLength;  

  EDirection        mDirection;          
  PRInt8            mDocDirtyState;      
  PRUint8           mSpellcheckCheckboxState; 

  bool mInIMEMode;        
  bool mIsIMEComposing;   
                                                       

  bool mShouldTxnSetSelection;  
  bool mDidPreDestroy;    
  bool mDidPostCreate;    
  bool mHandlingTrustedAction;
  bool mDispatchInputEvent;

  friend bool NSCanUnload(nsISupports* serviceMgr);
  friend class nsAutoTxnsConserveSelection;
  friend class nsAutoSelectionReset;
  friend class nsAutoRules;
  friend class nsRangeUpdater;
};


#endif
