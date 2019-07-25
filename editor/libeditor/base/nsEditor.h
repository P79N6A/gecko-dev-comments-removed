





































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
#include "nsISelection.h"
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
    kOpDeleteSelection,
    
    kOpInsertBreak    = 1000,
    kOpInsertText     = 1001,
    kOpInsertIMEText  = 1002,
    kOpDeleteText     = 1003
  };

  


  nsEditor();
  


  virtual ~nsEditor();



  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsEditor,
                                           nsIEditor)

  
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
  NS_IMETHOD DeleteSelectionImpl(EDirection aAction);
  NS_IMETHOD DeleteSelectionAndCreateNode(const nsAString& aTag,
                                           nsIDOMNode ** aNewNode);

  
  nsresult ReplaceContainer(nsIDOMNode *inNode, 
                            nsCOMPtr<nsIDOMNode> *outNode, 
                            const nsAString &aNodeType,
                            const nsAString *aAttribute = nsnull,
                            const nsAString *aValue = nsnull,
                            bool aCloneAttributes = false);

  nsresult RemoveContainer(nsIDOMNode *inNode);
  nsresult InsertContainerAbove(nsIDOMNode *inNode, 
                                nsCOMPtr<nsIDOMNode> *outNode, 
                                const nsAString &aNodeType,
                                const nsAString *aAttribute = nsnull,
                                const nsAString *aValue = nsnull);
  nsresult MoveNode(nsIDOMNode *aNode, nsIDOMNode *aParent, PRInt32 aOffset);

  




  nsresult CreateHTMLContent(const nsAString& aTag, nsIContent** aContent);

  
  virtual nsresult BeginIMEComposition();
  virtual nsresult UpdateIMEComposition(const nsAString &aCompositionString,
                                        nsIPrivateTextRangeList *aTextRange)=0;
  nsresult EndIMEComposition();

  void SwitchTextDirectionTo(PRUint32 aDirection);

  void BeginKeypressHandling() { mLastKeypressEventWasTrusted = eTriTrue; }
  void BeginKeypressHandling(nsIDOMNSEvent* aEvent);
  void EndKeypressHandling() { mLastKeypressEventWasTrusted = eTriUnset; }

  class FireTrustedInputEvent {
  public:
    explicit FireTrustedInputEvent(nsEditor* aSelf, bool aActive = true)
      : mEditor(aSelf)
      , mShouldAct(aActive && mEditor->mLastKeypressEventWasTrusted == eTriUnset) {
      if (mShouldAct) {
        mEditor->BeginKeypressHandling();
      }
    }
    ~FireTrustedInputEvent() {
      if (mShouldAct) {
        mEditor->EndKeypressHandling();
      }
    }
  private:
    nsEditor* mEditor;
    bool mShouldAct;
  };

protected:
  nsCString mContentMIMEType;       

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


  NS_IMETHOD CreateTxnForDeleteSelection(EDirection aAction,
                                         EditAggregateTxn ** aTxn,
                                         nsIDOMNode ** aNode,
                                         PRInt32 *aOffset,
                                         PRInt32 *aLength);

  NS_IMETHOD CreateTxnForDeleteInsertionPoint(nsIDOMRange         *aRange, 
                                              EDirection aAction, 
                                              EditAggregateTxn *aTxn,
                                              nsIDOMNode ** aNode,
                                              PRInt32 *aOffset,
                                              PRInt32 *aLength);


  


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



  NS_IMETHOD CreateTxnForDeleteText(nsIDOMCharacterData *aElement,
                                    PRUint32             aOffset,
                                    PRUint32             aLength,
                                    DeleteTextTxn      **aTxn);

  nsresult CreateTxnForDeleteCharacter(nsIDOMCharacterData  *aData,
                                       PRUint32              aOffset,
                                       nsIEditor::EDirection aDirection,
                                       DeleteTextTxn       **aTxn);
	
  NS_IMETHOD CreateTxnForSplitNode(nsIDOMNode *aNode,
                                   PRUint32    aOffset,
                                   SplitElementTxn **aTxn);

  NS_IMETHOD CreateTxnForJoinNode(nsIDOMNode  *aLeftNode,
                                  nsIDOMNode  *aRightNode,
                                  JoinElementTxn **aTxn);

  NS_IMETHOD DeleteSelectionAndPrepareToCreateNode(nsCOMPtr<nsIDOMNode> &parentSelectedNode, 
                                                   PRInt32& offsetOfNewNode);

  
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
                               bool      bNoBlockCrossing,
                               nsIContent *aActiveEditorRoot);

  
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

  

  NS_IMETHOD StartOperation(PRInt32 opID, nsIEditor::EDirection aDirection);

  

  NS_IMETHOD EndOperation();

  

  bool     ArePreservingSelection();
  nsresult PreserveSelectionAcrossActions(nsISelection *aSel);
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
                        bool         bNoBlockCrossing = false,
                        nsIContent  *aActiveEditorRoot = nsnull);

  
  nsresult GetPriorNode(nsIDOMNode  *aParentNode, 
                        PRInt32      aOffset, 
                        bool         aEditableNode, 
                        nsCOMPtr<nsIDOMNode> *aResultNode,
                        bool         bNoBlockCrossing = false,
                        nsIContent  *aActiveEditorRoot = nsnull);
                       
  






  nsresult GetNextNode(nsIDOMNode  *aCurrentNode, 
                       bool         aEditableNode,
                       nsCOMPtr<nsIDOMNode> *aResultNode,
                       bool         bNoBlockCrossing = false,
                       nsIContent  *aActiveEditorRoot = nsnull);

  
  nsresult GetNextNode(nsIDOMNode  *aParentNode, 
                       PRInt32      aOffset, 
                       bool         aEditableNode, 
                       nsCOMPtr<nsIDOMNode> *aResultNode,
                       bool         bNoBlockCrossing = false,
                       nsIContent  *aActiveEditorRoot = nsnull);

  
  nsIContent* FindNode(nsINode *aCurrentNode,
                       bool     aGoForward,
                       bool     aEditableNode,
                       bool     bNoBlockCrossing,
                       nsIContent *aActiveEditorRoot);
  



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

  
  static inline bool NodeIsTypeString(nsIDOMNode *aNode, const nsAString &aTag)
  {
    nsIAtom *nodeAtom = GetTag(aNode);
    return nodeAtom && nodeAtom->Equals(aTag);
  }


  
  bool CanContainTag(nsIDOMNode* aParent, const nsAString &aTag);
  bool TagCanContain(const nsAString &aParentTag, nsIDOMNode* aChild);
  virtual bool TagCanContainTag(const nsAString &aParentTag, const nsAString &aChildTag);

  
  bool IsRootNode(nsIDOMNode *inNode);
  bool IsRootNode(nsINode *inNode);

  
  bool IsDescendantOfBody(nsIDOMNode *inNode);
  bool IsDescendantOfBody(nsINode *inNode);

  
  virtual bool IsContainer(nsIDOMNode *aNode);

  
  bool IsEditable(nsIDOMNode *aNode);
  bool IsEditable(nsIContent *aNode);

  virtual bool IsTextInDirtyFrameVisible(nsIContent *aNode);

  
  bool IsMozEditorBogusNode(nsIDOMNode *aNode);
  bool IsMozEditorBogusNode(nsIContent *aNode);

  
  nsresult CountEditableChildren(nsIDOMNode *aNode, PRUint32 &outCount);
  
  
  nsresult GetFirstEditableNode(nsIDOMNode *aRoot, nsCOMPtr<nsIDOMNode> *outFirstNode);
#ifdef XXX_DEAD_CODE
  nsresult GetLastEditableNode(nsIDOMNode *aRoot, nsCOMPtr<nsIDOMNode> *outLastNode);
#endif

  nsresult GetIMEBufferLength(PRInt32* length);
  bool     IsIMEComposing();    
  void     SetIsIMEComposing(); 

  
  static nsresult GetTagString(nsIDOMNode *aNode, nsAString& outString);
  static nsIAtom *GetTag(nsIDOMNode *aNode);
  virtual bool NodesSameType(nsIDOMNode *aNode1, nsIDOMNode *aNode2);
  static bool IsTextOrElementNode(nsIDOMNode *aNode);
  static bool IsTextNode(nsIDOMNode *aNode);
  static bool IsTextNode(nsINode *aNode);
  
  static PRInt32 GetIndexOf(nsIDOMNode *aParent, nsIDOMNode *aChild);
  static nsCOMPtr<nsIDOMNode> GetChildAt(nsIDOMNode *aParent, PRInt32 aOffset);
  static nsCOMPtr<nsIDOMNode> GetNodeAtRangeOffsetPoint(nsIDOMNode* aParentOrNode, PRInt32 aOffset);

  static nsresult GetStartNodeAndOffset(nsISelection *aSelection, nsIDOMNode **outStartNode, PRInt32 *outStartOffset);
  static nsresult GetEndNodeAndOffset(nsISelection *aSelection, nsIDOMNode **outEndNode, PRInt32 *outEndOffset);
#if DEBUG_JOE
  static void DumpNode(nsIDOMNode *aNode, PRInt32 indent=0);
#endif

  
  
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

  nsresult BeginUpdateViewBatch(void);
  virtual nsresult EndUpdateViewBatch(void);

  bool GetShouldTxnSetSelection();

  virtual nsresult HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent);

  nsresult HandleInlineSpellCheck(PRInt32 action,
                                    nsISelection *aSelection,
                                    nsIDOMNode *previousSelectedNode,
                                    PRInt32 previousSelectedOffset,
                                    nsIDOMNode *aStartNode,
                                    PRInt32 aStartOffset,
                                    nsIDOMNode *aEndNode,
                                    PRInt32 aEndOffset);

  virtual already_AddRefed<nsIDOMEventTarget> GetDOMEventTarget() = 0;

  
  mozilla::dom::Element *GetRoot();

  
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

  bool UseAsyncUpdate() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorUseAsyncUpdatesMask) != 0;
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
  
  PRBool ShouldSkipSpellCheck() const
  {
    return (mFlags & nsIPlaintextEditor::eEditorSkipSpellCheck) != 0;
  }

  bool IsTabbable() const
  {
    return IsSingleLineEditor() || IsPasswordEditor() || IsFormWidget() ||
           IsInteractionAllowed();
  }

  
  virtual already_AddRefed<nsIContent> GetFocusedContent();

  
  
  
  virtual bool IsActiveInDOMWindow();

  
  
  
  
  virtual bool IsAcceptableInputEvent(nsIDOMEvent* aEvent);

  
  
  
  
  
  virtual already_AddRefed<nsIContent> FindSelectionRoot(nsINode* aNode);

  
  
  
  nsresult InitializeSelection(nsIDOMEventTarget* aFocusEventTarget);

  
  
  
  void OnFocus(nsIDOMEventTarget* aFocusEventTarget);

protected:

  PRUint32        mModCount;     
  PRUint32        mFlags;        

  nsWeakPtr       mSelConWeak;   
  PRInt32         mUpdateCount;
  nsIViewManager::UpdateViewBatch mBatch;

  
  enum Tristate {
    eTriUnset,
    eTriFalse,
    eTriTrue
  }                 mSpellcheckCheckboxState;
  nsCOMPtr<nsIInlineSpellChecker> mInlineSpellChecker;

  nsCOMPtr<nsITransactionManager> mTxnMgr;
  nsWeakPtr         mPlaceHolderTxn;     
  nsIAtom          *mPlaceHolderName;    
  PRInt32           mPlaceHolderBatch;   
  nsSelectionState *mSelState;           
  nsSelectionState  mSavedSel;           
  nsRangeUpdater    mRangeUpdater;       
  nsCOMPtr<mozilla::dom::Element> mRootElement;   
  PRInt32           mAction;             
  EDirection        mDirection;          
  
  
  nsCOMPtr<nsIPrivateTextRangeList> mIMETextRangeList; 
  nsCOMPtr<nsIDOMCharacterData>     mIMETextNode;      
  PRUint32                          mIMETextOffset;    
  PRUint32                          mIMEBufferLength;  
  bool                              mInIMEMode;        
  bool                              mIsIMEComposing;   
                                                       

  bool                          mShouldTxnSetSelection;  
  bool                          mDidPreDestroy;    
  bool                          mDidPostCreate;    
   
  nsCOMArray<nsIEditActionListener> mActionListeners;  
  nsCOMArray<nsIEditorObserver> mEditorObservers;  
  nsCOMArray<nsIDocumentStateListener> mDocStateListeners;

  PRInt8                        mDocDirtyState;		
  nsWeakPtr        mDocWeak;  
  
  nsCOMPtr<nsIDOMEventTarget> mEventTarget;

  nsString* mPhonetic;

 nsCOMPtr<nsIDOMEventListener> mEventListener;

  Tristate mLastKeypressEventWasTrusted;

  friend bool NSCanUnload(nsISupports* serviceMgr);
  friend class nsAutoTxnsConserveSelection;
  friend class nsAutoSelectionReset;
  friend class nsAutoRules;
  friend class nsRangeUpdater;
};


#endif
