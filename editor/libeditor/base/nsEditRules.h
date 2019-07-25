




#ifndef nsEditRules_h__
#define nsEditRules_h__


#define NS_IEDITRULES_IID \
{ 0x2cc50d11, 0x9909, 0x433f, \
  { 0xb6, 0xfb, 0x4c, 0xf2, 0x56, 0xe5, 0xe5, 0x71 } }

#include "nsEditor.h"

class nsPlaintextEditor;
class nsISelection;





class nsRulesInfo
{
  public:
  
  nsRulesInfo(nsEditor::OperationID aAction) : action(aAction) {}
  virtual ~nsRulesInfo() {}
  
  nsEditor::OperationID action;
};





class nsIEditRules : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IEDITRULES_IID)
  



  NS_IMETHOD Init(nsPlaintextEditor *aEditor)=0;
  NS_IMETHOD DetachEditor()=0;
  NS_IMETHOD BeforeEdit(nsEditor::OperationID action,
                        nsIEditor::EDirection aDirection) = 0;
  NS_IMETHOD AfterEdit(nsEditor::OperationID action,
                       nsIEditor::EDirection aDirection) = 0;
  NS_IMETHOD WillDoAction(mozilla::Selection* aSelection, nsRulesInfo* aInfo,
                          bool* aCancel, bool* aHandled) = 0;
  NS_IMETHOD DidDoAction(nsISelection *aSelection, nsRulesInfo *aInfo, nsresult aResult)=0;
  NS_IMETHOD DocumentIsEmpty(bool *aDocumentIsEmpty)=0;
  NS_IMETHOD DocumentModified()=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIEditRules, NS_IEDITRULES_IID)

#endif 

