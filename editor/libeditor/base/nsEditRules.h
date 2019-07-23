




































#ifndef nsEditRules_h__
#define nsEditRules_h__


#define NS_IEDITRULES_IID \
{ 0x783223c9, 0x153a, 0x4e01, \
  { 0x84, 0x22, 0x39, 0xa3, 0xaf, 0xe4, 0x69, 0xda } }

class nsPlaintextEditor;
class nsISelection;





class nsRulesInfo
{
  public:
  
  nsRulesInfo(int aAction) : action(aAction) {}
  virtual ~nsRulesInfo() {}
  
  int action;
};





class nsIEditRules : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IEDITRULES_IID)
  



  NS_IMETHOD Init(nsPlaintextEditor *aEditor, PRUint32 aFlags)=0;
  NS_IMETHOD DetachEditor()=0;
  NS_IMETHOD BeforeEdit(PRInt32 action, nsIEditor::EDirection aDirection)=0;
  NS_IMETHOD AfterEdit(PRInt32 action, nsIEditor::EDirection aDirection)=0;
  NS_IMETHOD WillDoAction(nsISelection *aSelection, nsRulesInfo *aInfo, PRBool *aCancel, PRBool *aHandled)=0;
  NS_IMETHOD DidDoAction(nsISelection *aSelection, nsRulesInfo *aInfo, nsresult aResult)=0;
  NS_IMETHOD GetFlags(PRUint32 *aFlags)=0;
  NS_IMETHOD SetFlags(PRUint32 aFlags)=0;
  NS_IMETHOD DocumentIsEmpty(PRBool *aDocumentIsEmpty)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEditRules, NS_IEDITRULES_IID)

#endif 

