





































#ifndef nsPlaintextEditor_h__
#define nsPlaintextEditor_h__

#include "nsCOMPtr.h"

#include "nsIPlaintextEditor.h"
#include "nsIEditorMailSupport.h"

#include "nsEditor.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventListener.h"

#include "nsEditRules.h"
#include "nsCycleCollectionParticipant.h"
 
class nsITransferable;
class nsIDocumentEncoder;





class nsPlaintextEditor : public nsEditor,
                          public nsIPlaintextEditor,
                          public nsIEditorMailSupport
{

public:




  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsPlaintextEditor, nsEditor)

  
  enum {
    eTypedText,  
    eTypedBR,    
    eTypedBreak  
  };

           nsPlaintextEditor();
  virtual  ~nsPlaintextEditor();

  
  NS_DECL_NSIPLAINTEXTEDITOR

  
  NS_DECL_NSIEDITORMAILSUPPORT

  
  NS_IMETHOD SetAttributeOrEquivalent(nsIDOMElement * aElement,
                                      const nsAString & aAttribute,
                                      const nsAString & aValue,
                                      PRBool aSuppressTransaction);
  NS_IMETHOD RemoveAttributeOrEquivalent(nsIDOMElement * aElement,
                                         const nsAString & aAttribute,
                                         PRBool aSuppressTransaction);

  
  NS_IMETHOD Init(nsIDOMDocument *aDoc, nsIContent *aRoot, nsISelectionController *aSelCon, PRUint32 aFlags);
  
  NS_IMETHOD GetDocumentIsEmpty(PRBool *aDocumentIsEmpty);
  NS_IMETHOD GetIsDocumentEditable(PRBool *aIsDocumentEditable);

  NS_IMETHOD DeleteSelection(EDirection aAction);

  NS_IMETHOD SetDocumentCharacterSet(const nsACString & characterSet);

  NS_IMETHOD Undo(PRUint32 aCount);
  NS_IMETHOD Redo(PRUint32 aCount);

  NS_IMETHOD Cut();
  NS_IMETHOD CanCut(PRBool *aCanCut);
  NS_IMETHOD Copy();
  NS_IMETHOD CanCopy(PRBool *aCanCopy);
  NS_IMETHOD Paste(PRInt32 aSelectionType);
  NS_IMETHOD CanPaste(PRInt32 aSelectionType, PRBool *aCanPaste);
  NS_IMETHOD PasteTransferable(nsITransferable *aTransferable);
  NS_IMETHOD CanPasteTransferable(nsITransferable *aTransferable, PRBool *aCanPaste);

  NS_IMETHOD CanDrag(nsIDOMEvent *aDragEvent, PRBool *aCanDrag);
  NS_IMETHOD DoDrag(nsIDOMEvent *aDragEvent);
  NS_IMETHOD InsertFromDrop(nsIDOMEvent* aDropEvent);

  NS_IMETHOD OutputToString(const nsAString& aFormatType,
                            PRUint32 aFlags,
                            nsAString& aOutputString);
                            
  NS_IMETHOD OutputToStream(nsIOutputStream* aOutputStream,
                            const nsAString& aFormatType,
                            const nsACString& aCharsetOverride,
                            PRUint32 aFlags);


  

  NS_IMETHOD StartOperation(PRInt32 opID, nsIEditor::EDirection aDirection);

  

  NS_IMETHOD EndOperation();

  
  NS_IMETHOD SelectEntireDocument(nsISelection *aSelection);

  virtual nsresult HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent);

  virtual already_AddRefed<nsIDOMEventTarget> GetPIDOMEventTarget();

  virtual nsresult BeginIMEComposition();
  virtual nsresult UpdateIMEComposition(const nsAString &aCompositionString,
                                        nsIPrivateTextRangeList *aTextRange);

  
  NS_IMETHOD TypedText(const nsAString& aString, PRInt32 aAction);

  



  nsresult GetTextSelectionOffsets(nsISelection *aSelection,
                                   PRUint32 &aStartOffset, 
                                   PRUint32 &aEndOffset);

  nsresult InsertTextAt(const nsAString &aStringToInsert,
                        nsIDOMNode *aDestinationNode,
                        PRInt32 aDestOffset,
                        PRBool aDoDeleteSelection);

  




  nsresult ExtendSelectionForDelete(nsISelection* aSelection,
                                    nsIEditor::EDirection *aAction);

  static void GetDefaultEditorPrefs(PRInt32 &aNewLineHandling,
                                    PRInt32 &aCaretStyle);

protected:

  NS_IMETHOD  InitRules();
  void        BeginEditorInit();
  nsresult    EndEditorInit();

  
  NS_IMETHOD GetAndInitDocEncoder(const nsAString& aFormatType,
                                  PRUint32 aFlags,
                                  const nsACString& aCharset,
                                  nsIDocumentEncoder** encoder);

  
  NS_IMETHOD CreateBR(nsIDOMNode *aNode, PRInt32 aOffset, 
                      nsCOMPtr<nsIDOMNode> *outBRNode, EDirection aSelect = eNone);
  NS_IMETHOD CreateBRImpl(nsCOMPtr<nsIDOMNode> *aInOutParent, 
                         PRInt32 *aInOutOffset, 
                         nsCOMPtr<nsIDOMNode> *outBRNode, 
                         EDirection aSelect);
  NS_IMETHOD InsertBR(nsCOMPtr<nsIDOMNode> *outBRNode);

  
  NS_IMETHOD PrepareTransferable(nsITransferable **transferable);
  NS_IMETHOD InsertTextFromTransferable(nsITransferable *transferable,
                                        nsIDOMNode *aDestinationNode,
                                        PRInt32 aDestOffset,
                                        PRBool aDoDeleteSelection);
  virtual nsresult SetupDocEncoder(nsIDocumentEncoder **aDocEncoder);
  virtual nsresult PutDragDataInTransferable(nsITransferable **aTransferable);

  
  nsresult SharedOutputString(PRUint32 aFlags, PRBool* aIsCollapsed, nsAString& aResult);

  
  PRBool IsModifiable();

  
  PRBool   mIgnoreSpuriousDragEvent;
  NS_IMETHOD IgnoreSpuriousDragEvent(PRBool aIgnoreSpuriousDragEvent) {mIgnoreSpuriousDragEvent = aIgnoreSpuriousDragEvent; return NS_OK;}

  PRBool CanCutOrCopy();
  PRBool FireClipboardEvent(PRInt32 aType);


protected:

  nsCOMPtr<nsIEditRules>        mRules;
  PRBool  mWrapToWindow;
  PRInt32 mWrapColumn;
  PRInt32 mMaxTextLength;
  PRInt32 mInitTriggerCounter;
  PRInt32 mNewlineHandling;
  PRInt32 mCaretStyle;


friend class nsHTMLEditRules;
friend class nsTextEditRules;
friend class nsAutoEditInitRulesTrigger;

};

#endif 

