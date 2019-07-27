




#ifndef nsPlaintextEditor_h__
#define nsPlaintextEditor_h__

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsEditor.h"
#include "nsIEditor.h"
#include "nsIEditorMailSupport.h"
#include "nsIPlaintextEditor.h"
#include "nsISupportsImpl.h"
#include "nscore.h"

class nsIContent;
class nsIDOMDataTransfer;
class nsIDOMDocument;
class nsIDOMElement;
class nsIDOMEvent;
class nsIDOMEventTarget;
class nsIDOMKeyEvent;
class nsIDOMNode;
class nsIDocumentEncoder;
class nsIEditRules;
class nsIOutputStream;
class nsISelectionController;
class nsITransferable;

namespace mozilla {
namespace dom {
class Selection;
}
}





class nsPlaintextEditor : public nsEditor,
                          public nsIPlaintextEditor,
                          public nsIEditorMailSupport
{

public:




  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsPlaintextEditor, nsEditor)

  
  enum ETypingAction {
    eTypedText,  
    eTypedBR,    
    eTypedBreak  
  };

  nsPlaintextEditor();

  
  NS_DECL_NSIPLAINTEXTEDITOR

  
  NS_DECL_NSIEDITORMAILSUPPORT

  
  NS_IMETHOD SetAttributeOrEquivalent(nsIDOMElement * aElement,
                                      const nsAString & aAttribute,
                                      const nsAString & aValue,
                                      bool aSuppressTransaction) MOZ_OVERRIDE;
  NS_IMETHOD RemoveAttributeOrEquivalent(nsIDOMElement * aElement,
                                         const nsAString & aAttribute,
                                         bool aSuppressTransaction) MOZ_OVERRIDE;

  
  NS_IMETHOD Init(nsIDOMDocument *aDoc, nsIContent *aRoot,
                  nsISelectionController *aSelCon, uint32_t aFlags,
                  const nsAString& aValue) MOZ_OVERRIDE;
  
  NS_IMETHOD GetDocumentIsEmpty(bool *aDocumentIsEmpty) MOZ_OVERRIDE;
  NS_IMETHOD GetIsDocumentEditable(bool *aIsDocumentEditable) MOZ_OVERRIDE;

  NS_IMETHOD DeleteSelection(EDirection aAction,
                             EStripWrappers aStripWrappers) MOZ_OVERRIDE;

  NS_IMETHOD SetDocumentCharacterSet(const nsACString & characterSet) MOZ_OVERRIDE;

  NS_IMETHOD Undo(uint32_t aCount) MOZ_OVERRIDE;
  NS_IMETHOD Redo(uint32_t aCount) MOZ_OVERRIDE;

  NS_IMETHOD Cut() MOZ_OVERRIDE;
  NS_IMETHOD CanCut(bool *aCanCut) MOZ_OVERRIDE;
  NS_IMETHOD Copy() MOZ_OVERRIDE;
  NS_IMETHOD CanCopy(bool *aCanCopy) MOZ_OVERRIDE;
  NS_IMETHOD CanDelete(bool *aCanDelete) MOZ_OVERRIDE;
  NS_IMETHOD Paste(int32_t aSelectionType) MOZ_OVERRIDE;
  NS_IMETHOD CanPaste(int32_t aSelectionType, bool *aCanPaste) MOZ_OVERRIDE;
  NS_IMETHOD PasteTransferable(nsITransferable *aTransferable) MOZ_OVERRIDE;
  NS_IMETHOD CanPasteTransferable(nsITransferable *aTransferable, bool *aCanPaste) MOZ_OVERRIDE;

  NS_IMETHOD OutputToString(const nsAString& aFormatType,
                            uint32_t aFlags,
                            nsAString& aOutputString) MOZ_OVERRIDE;
                            
  NS_IMETHOD OutputToStream(nsIOutputStream* aOutputStream,
                            const nsAString& aFormatType,
                            const nsACString& aCharsetOverride,
                            uint32_t aFlags) MOZ_OVERRIDE;


  

  NS_IMETHOD StartOperation(EditAction opID,
                            nsIEditor::EDirection aDirection) MOZ_OVERRIDE;

  

  NS_IMETHOD EndOperation() MOZ_OVERRIDE;

  
  virtual nsresult SelectEntireDocument(mozilla::dom::Selection* aSelection) MOZ_OVERRIDE;

  virtual nsresult HandleKeyPressEvent(nsIDOMKeyEvent* aKeyEvent) MOZ_OVERRIDE;

  virtual already_AddRefed<mozilla::dom::EventTarget> GetDOMEventTarget() MOZ_OVERRIDE;

  virtual nsresult BeginIMEComposition(mozilla::WidgetCompositionEvent* aEvent) MOZ_OVERRIDE;
  virtual nsresult UpdateIMEComposition(nsIDOMEvent* aTextEvent) MOZ_OVERRIDE;

  virtual already_AddRefed<nsIContent> GetInputEventTargetContent() MOZ_OVERRIDE;

  
  NS_IMETHOD TypedText(const nsAString& aString, ETypingAction aAction);

  nsresult InsertTextAt(const nsAString &aStringToInsert,
                        nsIDOMNode *aDestinationNode,
                        int32_t aDestOffset,
                        bool aDoDeleteSelection);

  virtual nsresult InsertFromDataTransfer(mozilla::dom::DataTransfer *aDataTransfer,
                                          int32_t aIndex,
                                          nsIDOMDocument *aSourceDoc,
                                          nsIDOMNode *aDestinationNode,
                                          int32_t aDestOffset,
                                          bool aDoDeleteSelection) MOZ_OVERRIDE;

  virtual nsresult InsertFromDrop(nsIDOMEvent* aDropEvent) MOZ_OVERRIDE;

  




  nsresult ExtendSelectionForDelete(mozilla::dom::Selection* aSelection,
                                    nsIEditor::EDirection *aAction);

  
  
  
  bool IsSafeToInsertData(nsIDOMDocument* aSourceDoc);

  static void GetDefaultEditorPrefs(int32_t &aNewLineHandling,
                                    int32_t &aCaretStyle);

protected:
  virtual  ~nsPlaintextEditor();

  NS_IMETHOD  InitRules();
  void        BeginEditorInit();
  nsresult    EndEditorInit();

  
  NS_IMETHOD GetAndInitDocEncoder(const nsAString& aFormatType,
                                  uint32_t aFlags,
                                  const nsACString& aCharset,
                                  nsIDocumentEncoder** encoder);

  
  NS_IMETHOD CreateBR(nsIDOMNode *aNode, int32_t aOffset, 
                      nsCOMPtr<nsIDOMNode> *outBRNode, EDirection aSelect = eNone);
  already_AddRefed<mozilla::dom::Element>
      CreateBRImpl(nsCOMPtr<nsINode>* aInOutParent, int32_t* aInOutOffset,
                   EDirection aSelect);
  nsresult CreateBRImpl(nsCOMPtr<nsIDOMNode>* aInOutParent,
                        int32_t* aInOutOffset,
                        nsCOMPtr<nsIDOMNode>* outBRNode,
                        EDirection aSelect);
  nsresult InsertBR(nsCOMPtr<nsIDOMNode>* outBRNode);

  
  NS_IMETHOD PrepareTransferable(nsITransferable **transferable);
  NS_IMETHOD InsertTextFromTransferable(nsITransferable *transferable,
                                        nsIDOMNode *aDestinationNode,
                                        int32_t aDestOffset,
                                        bool aDoDeleteSelection);

  
  nsresult SharedOutputString(uint32_t aFlags, bool* aIsCollapsed, nsAString& aResult);

  
  bool IsModifiable();

  enum PasswordFieldAllowed {
    ePasswordFieldAllowed,
    ePasswordFieldNotAllowed
  };
  bool CanCutOrCopy(PasswordFieldAllowed aPasswordFieldAllowed);
  bool FireClipboardEvent(int32_t aType, int32_t aSelectionType);

  bool UpdateMetaCharset(nsIDOMDocument* aDocument,
                         const nsACString& aCharacterSet);


protected:

  nsCOMPtr<nsIEditRules>        mRules;
  bool    mWrapToWindow;
  int32_t mWrapColumn;
  int32_t mMaxTextLength;
  int32_t mInitTriggerCounter;
  int32_t mNewlineHandling;
  int32_t mCaretStyle;


friend class nsHTMLEditRules;
friend class nsTextEditRules;
friend class nsAutoEditInitRulesTrigger;

};

#endif 

