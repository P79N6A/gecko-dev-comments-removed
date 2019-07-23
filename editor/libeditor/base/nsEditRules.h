




































#ifndef nsEditRules_h__
#define nsEditRules_h__


#define NS_IEDITRULES_IID \
{ 0xfb45ac36, 0xe8f1, 0x44ae, \
  { 0x8f, 0xb7, 0x46, 0x6e, 0x1b, 0xe1, 0x19, 0xb0 } }

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
  



  NS_IMETHOD Init(nsPlaintextEditor *aEditor)=0;
  NS_IMETHOD DetachEditor()=0;
  NS_IMETHOD BeforeEdit(PRInt32 action, nsIEditor::EDirection aDirection)=0;
  NS_IMETHOD AfterEdit(PRInt32 action, nsIEditor::EDirection aDirection)=0;
  NS_IMETHOD WillDoAction(nsISelection *aSelection, nsRulesInfo *aInfo, PRBool *aCancel, PRBool *aHandled)=0;
  NS_IMETHOD DidDoAction(nsISelection *aSelection, nsRulesInfo *aInfo, nsresult aResult)=0;
  NS_IMETHOD DocumentIsEmpty(PRBool *aDocumentIsEmpty)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEditRules, NS_IEDITRULES_IID)

#endif 

