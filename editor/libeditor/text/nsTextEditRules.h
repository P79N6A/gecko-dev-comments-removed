




































#ifndef nsTextEditRules_h__
#define nsTextEditRules_h__

#include "nsCOMPtr.h"

#include "nsPlaintextEditor.h"
#include "nsIDOMNode.h"

#include "nsEditRules.h"
#include "nsITimer.h"












class nsTextEditRules : public nsIEditRules, public nsITimerCallback
{
public:
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsTextEditRules, nsIEditRules)
  
              nsTextEditRules();
  virtual     ~nsTextEditRules();

  
  NS_IMETHOD Init(nsPlaintextEditor *aEditor);
  NS_IMETHOD DetachEditor();
  NS_IMETHOD BeforeEdit(PRInt32 action, nsIEditor::EDirection aDirection);
  NS_IMETHOD AfterEdit(PRInt32 action, nsIEditor::EDirection aDirection);
  NS_IMETHOD WillDoAction(nsISelection *aSelection, nsRulesInfo *aInfo, bool *aCancel, bool *aHandled);
  NS_IMETHOD DidDoAction(nsISelection *aSelection, nsRulesInfo *aInfo, nsresult aResult);
  NS_IMETHOD DocumentIsEmpty(bool *aDocumentIsEmpty);
  NS_IMETHOD DocumentModified();

  
  enum 
  {
    kDefault             = 0,
    
    kUndo                = 1000,
    kRedo                = 1001,
    
    kInsertText          = 2000,
    kInsertTextIME       = 2001,
    kDeleteSelection     = 2002,
    kSetTextProperty     = 2003,
    kRemoveTextProperty  = 2004,
    kOutputText          = 2005,
    
    kInsertBreak         = 3000,
    kMakeList            = 3001,
    kIndent              = 3002,
    kOutdent             = 3003,
    kAlign               = 3004,
    kMakeBasicBlock      = 3005,
    kRemoveList          = 3006,
    kMakeDefListItem     = 3007,
    kInsertElement       = 3008,
    kLoadHTML            = 3013,
    kSetAbsolutePosition = 3015,
    kRemoveAbsolutePosition = 3016,
    kDecreaseZIndex      = 3017,
    kIncreaseZIndex      = 3018

  };
  
public:
  nsresult ResetIMETextPWBuf();

  





















  static void HandleNewLines(nsString &aString, PRInt32 aNewLineHandling);

  









  static nsresult FillBufWithPWChars(nsAString *aOutString, PRInt32 aLength);

protected:

  
  nsresult WillInsertText(  PRInt32          aAction,
                            nsISelection *aSelection, 
                            bool            *aCancel,
                            bool            *aHandled,
                            const nsAString *inString,
                            nsAString       *outString,
                            PRInt32          aMaxLength);
  nsresult DidInsertText(nsISelection *aSelection, nsresult aResult);
  nsresult GetTopEnclosingPre(nsIDOMNode *aNode, nsIDOMNode** aOutPreNode);

  nsresult WillInsertBreak(nsISelection *aSelection, bool *aCancel,
                           bool *aHandled, PRInt32 aMaxLength);
  nsresult DidInsertBreak(nsISelection *aSelection, nsresult aResult);

  nsresult WillInsert(nsISelection *aSelection, bool *aCancel);
  nsresult DidInsert(nsISelection *aSelection, nsresult aResult);

  nsresult WillDeleteSelection(nsISelection *aSelection, 
                               nsIEditor::EDirection aCollapsedAction, 
                               bool *aCancel,
                               bool *aHandled);
  nsresult DidDeleteSelection(nsISelection *aSelection, 
                              nsIEditor::EDirection aCollapsedAction, 
                              nsresult aResult);

  nsresult WillSetTextProperty(nsISelection *aSelection, bool *aCancel, bool *aHandled);
  nsresult DidSetTextProperty(nsISelection *aSelection, nsresult aResult);

  nsresult WillRemoveTextProperty(nsISelection *aSelection, bool *aCancel, bool *aHandled);
  nsresult DidRemoveTextProperty(nsISelection *aSelection, nsresult aResult);

  nsresult WillUndo(nsISelection *aSelection, bool *aCancel, bool *aHandled);
  nsresult DidUndo(nsISelection *aSelection, nsresult aResult);

  nsresult WillRedo(nsISelection *aSelection, bool *aCancel, bool *aHandled);
  nsresult DidRedo(nsISelection *aSelection, nsresult aResult);

  






  nsresult WillOutputText(nsISelection *aSelection,
                          const nsAString  *aInFormat,
                          nsAString *aOutText, 
                          bool     *aOutCancel, 
                          bool *aHandled);

  nsresult DidOutputText(nsISelection *aSelection, nsresult aResult);


  

  
  nsresult RemoveRedundantTrailingBR();
  
  
  nsresult CreateTrailingBRIfNeeded();
  
 
  nsresult CreateBogusNodeIfNeeded(nsISelection *aSelection);

  

  nsresult TruncateInsertionIfNeeded(nsISelection             *aSelection, 
                                     const nsAString          *aInString,
                                     nsAString                *aOutString,
                                     PRInt32                   aMaxLength,
                                     bool                     *aTruncated);

  
  nsresult RemoveIMETextFromPWBuf(PRUint32 &aStart, nsAString *aIMEString);

  nsresult CreateMozBR(nsIDOMNode* inParent, PRInt32 inOffset,
                       nsIDOMNode** outBRNode = nsnull);

  nsresult CheckBidiLevelForDeletion(nsISelection         *aSelection,
                                     nsIDOMNode           *aSelNode, 
                                     PRInt32               aSelOffset, 
                                     nsIEditor::EDirection aAction,
                                     bool                 *aCancel);

  nsresult HideLastPWInput();

  nsresult CollapseSelectionToTrailingBRIfNeeded(nsISelection *aSelection);

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
  PRUint32             mPasswordIMEIndex;
  nsCOMPtr<nsIDOMNode> mBogusNode;     
  nsCOMPtr<nsIDOMNode> mCachedSelectionNode;    
  PRInt32              mCachedSelectionOffset;  
  PRUint32             mActionNesting;
  bool                 mLockRulesSniffing;
  bool                 mDidExplicitlySetInterline;
  bool                 mDeleteBidiImmediately; 
                                               
                                               
                                               
  PRInt32              mTheAction;     
  nsCOMPtr<nsITimer>   mTimer;
  PRUint32             mLastStart, mLastLength;

  
  friend class nsAutoLockRulesSniffing;

};



class nsTextRulesInfo : public nsRulesInfo
{
 public:
 
  nsTextRulesInfo(int aAction) : 
    nsRulesInfo(aAction),
    inString(0),
    outString(0),
    outputFormat(0),
    maxLength(-1),
    collapsedAction(nsIEditor::eNext),
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
  PRInt32 maxLength;
  
  
  nsIEditor::EDirection collapsedAction;
  
  
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
  
  nsAutoLockRulesSniffing(nsTextEditRules *rules) : mRules(rules) 
                 {if (mRules) mRules->mLockRulesSniffing = true;}
  ~nsAutoLockRulesSniffing() 
                 {if (mRules) mRules->mLockRulesSniffing = false;}
  
  protected:
  nsTextEditRules *mRules;
};






class nsAutoLockListener
{
  public:
  
  nsAutoLockListener(bool *enabled) : mEnabled(enabled)
                 {if (mEnabled) { mOldState=*mEnabled; *mEnabled = false;}}
  ~nsAutoLockListener() 
                 {if (mEnabled) *mEnabled = mOldState;}
  
  protected:
  bool *mEnabled;
  bool mOldState;
};

#endif 
