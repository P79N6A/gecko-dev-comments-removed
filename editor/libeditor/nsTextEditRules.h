




#ifndef nsTextEditRules_h__
#define nsTextEditRules_h__

#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsEditRules.h"
#include "nsEditor.h"
#include "nsIEditor.h"
#include "nsISupportsImpl.h"
#include "nsITimer.h"
#include "nsPlaintextEditor.h"
#include "nsString.h"
#include "nscore.h"

class nsIDOMElement;
class nsIDOMNode;
namespace mozilla {
namespace dom {
class Selection;
}  
}  












class nsTextEditRules : public nsIEditRules, public nsITimerCallback
{
public:
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsTextEditRules, nsIEditRules)

  nsTextEditRules();

  
  NS_IMETHOD Init(nsPlaintextEditor *aEditor) override;
  NS_IMETHOD SetInitialValue(const nsAString& aValue) override;
  NS_IMETHOD DetachEditor() override;
  NS_IMETHOD BeforeEdit(EditAction action,
                        nsIEditor::EDirection aDirection) override;
  NS_IMETHOD AfterEdit(EditAction action,
                       nsIEditor::EDirection aDirection) override;
  NS_IMETHOD WillDoAction(mozilla::dom::Selection* aSelection,
                          nsRulesInfo* aInfo, bool* aCancel, bool* aHandled) override;
  NS_IMETHOD DidDoAction(mozilla::dom::Selection* aSelection,
                         nsRulesInfo* aInfo, nsresult aResult) override;
  NS_IMETHOD DocumentIsEmpty(bool *aDocumentIsEmpty) override;
  NS_IMETHOD DocumentModified() override;

protected:
  virtual ~nsTextEditRules();

public:
  void ResetIMETextPWBuf();

  





















  static void HandleNewLines(nsString &aString, int32_t aNewLineHandling);

  









  static void FillBufWithPWChars(nsAString *aOutString, int32_t aLength);

protected:

  void InitFields();

  
  nsresult WillInsertText(  EditAction aAction,
                            mozilla::dom::Selection* aSelection,
                            bool            *aCancel,
                            bool            *aHandled,
                            const nsAString *inString,
                            nsAString       *outString,
                            int32_t          aMaxLength);
  nsresult DidInsertText(mozilla::dom::Selection* aSelection,
                         nsresult aResult);
  nsresult GetTopEnclosingPre(nsIDOMNode *aNode, nsIDOMNode** aOutPreNode);

  nsresult WillInsertBreak(mozilla::dom::Selection* aSelection, bool* aCancel,
                           bool *aHandled, int32_t aMaxLength);
  nsresult DidInsertBreak(mozilla::dom::Selection* aSelection,
                          nsresult aResult);

  nsresult WillInsert(mozilla::dom::Selection* aSelection, bool* aCancel);
  nsresult DidInsert(mozilla::dom::Selection* aSelection, nsresult aResult);

  nsresult WillDeleteSelection(mozilla::dom::Selection* aSelection,
                               nsIEditor::EDirection aCollapsedAction,
                               bool *aCancel,
                               bool *aHandled);
  nsresult DidDeleteSelection(mozilla::dom::Selection* aSelection,
                              nsIEditor::EDirection aCollapsedAction,
                              nsresult aResult);

  nsresult WillSetTextProperty(mozilla::dom::Selection* aSelection,
                               bool* aCancel, bool* aHandled);
  nsresult DidSetTextProperty(mozilla::dom::Selection* aSelection,
                              nsresult aResult);

  nsresult WillRemoveTextProperty(mozilla::dom::Selection* aSelection,
                                  bool* aCancel, bool* aHandled);
  nsresult DidRemoveTextProperty(mozilla::dom::Selection* aSelection,
                                 nsresult aResult);

  nsresult WillUndo(mozilla::dom::Selection* aSelection, bool* aCancel,
                    bool* aHandled);
  nsresult DidUndo(mozilla::dom::Selection* aSelection, nsresult aResult);

  nsresult WillRedo(mozilla::dom::Selection* aSelection, bool* aCancel,
                    bool* aHandled);
  nsresult DidRedo(mozilla::dom::Selection* aSelection, nsresult aResult);

  






  nsresult WillOutputText(mozilla::dom::Selection* aSelection,
                          const nsAString  *aInFormat,
                          nsAString *aOutText,
                          bool     *aOutCancel,
                          bool *aHandled);

  nsresult DidOutputText(mozilla::dom::Selection* aSelection, nsresult aResult);


  

  
  nsresult RemoveRedundantTrailingBR();

  
  nsresult CreateTrailingBRIfNeeded();

 
  nsresult CreateBogusNodeIfNeeded(mozilla::dom::Selection* aSelection);

  

  nsresult TruncateInsertionIfNeeded(mozilla::dom::Selection*  aSelection,
                                     const nsAString          *aInString,
                                     nsAString                *aOutString,
                                     int32_t                   aMaxLength,
                                     bool                     *aTruncated);

  
  void RemoveIMETextFromPWBuf(int32_t &aStart, nsAString *aIMEString);

  nsresult CreateMozBR(nsIDOMNode* inParent, int32_t inOffset,
                       nsIDOMNode** outBRNode = nullptr);

  void UndefineCaretBidiLevel(mozilla::dom::Selection* aSelection);

  nsresult CheckBidiLevelForDeletion(mozilla::dom::Selection* aSelection,
                                     nsIDOMNode           *aSelNode,
                                     int32_t               aSelOffset,
                                     nsIEditor::EDirection aAction,
                                     bool                 *aCancel);

  nsresult HideLastPWInput();

  nsresult CollapseSelectionToTrailingBRIfNeeded(mozilla::dom::Selection* aSelection);

  bool IsPasswordEditor() const
  {
    return mEditor ? mEditor->IsPasswordEditor() : false;
  }
  bool IsSingleLineEditor() const
  {
    return mEditor ? mEditor->IsSingleLineEditor() : false;
  }
  bool IsPlaintextEditor() const
  {
    return mEditor ? mEditor->IsPlaintextEditor() : false;
  }
  bool IsReadonly() const
  {
    return mEditor ? mEditor->IsReadonly() : false;
  }
  bool IsDisabled() const
  {
    return mEditor ? mEditor->IsDisabled() : false;
  }
  bool IsMailEditor() const
  {
    return mEditor ? mEditor->IsMailEditor() : false;
  }
  bool DontEchoPassword() const
  {
    return mEditor ? mEditor->DontEchoPassword() : false;
  }

  
  nsPlaintextEditor   *mEditor;        
  nsString             mPasswordText;  
  nsString             mPasswordIMEText;  
  uint32_t             mPasswordIMEIndex;
  nsCOMPtr<nsIDOMNode> mBogusNode;     
  nsCOMPtr<nsIDOMNode> mCachedSelectionNode;    
  int32_t              mCachedSelectionOffset;  
  uint32_t             mActionNesting;
  bool                 mLockRulesSniffing;
  bool                 mDidExplicitlySetInterline;
  bool                 mDeleteBidiImmediately; 
                                               
                                               
                                               
  EditAction mTheAction;     
  nsCOMPtr<nsITimer>   mTimer;
  uint32_t             mLastStart, mLastLength;

  
  friend class nsAutoLockRulesSniffing;

};



class nsTextRulesInfo : public nsRulesInfo
{
 public:

  explicit nsTextRulesInfo(EditAction aAction) :
    nsRulesInfo(aAction),
    inString(0),
    outString(0),
    outputFormat(0),
    maxLength(-1),
    collapsedAction(nsIEditor::eNext),
    stripWrappers(nsIEditor::eStrip),
    bOrdered(false),
    entireList(false),
    bulletType(0),
    alignType(0),
    blockType(0),
    insertElement(0)
    {}

  virtual ~nsTextRulesInfo() {}

  
  const nsAString *inString;
  nsAString *outString;
  const nsAString *outputFormat;
  int32_t maxLength;

  
  nsIEditor::EDirection collapsedAction;
  nsIEditor::EStripWrappers stripWrappers;

  
  bool bOrdered;
  bool entireList;
  const nsAString *bulletType;

  
  const nsAString *alignType;

  
  const nsAString *blockType;

  
  const nsIDOMElement* insertElement;
};







class nsAutoLockRulesSniffing
{
  public:

  explicit nsAutoLockRulesSniffing(nsTextEditRules *rules) : mRules(rules)
                 {if (mRules) mRules->mLockRulesSniffing = true;}
  ~nsAutoLockRulesSniffing()
                 {if (mRules) mRules->mLockRulesSniffing = false;}

  protected:
  nsTextEditRules *mRules;
};






class nsAutoLockListener
{
  public:

  explicit nsAutoLockListener(bool *enabled) : mEnabled(enabled)
                 {if (mEnabled) { mOldState=*mEnabled; *mEnabled = false;}}
  ~nsAutoLockListener()
                 {if (mEnabled) *mEnabled = mOldState;}

  protected:
  bool *mEnabled;
  bool mOldState;
};

#endif 
