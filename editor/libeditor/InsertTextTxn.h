




#ifndef InsertTextTxn_h__
#define InsertTextTxn_h__

#include "EditTxn.h"                    
#include "nsAutoPtr.h"                  
#include "nsCycleCollectionParticipant.h" 
#include "nsID.h"                       
#include "nsISupportsImpl.h"            
#include "nsString.h"                   
#include "nscore.h"                     

class nsEditor;
class nsITransaction;

#define NS_INSERTTEXTTXN_IID \
{ 0x8c9ad77f, 0x22a7, 0x4d01, \
  { 0xb1, 0x59, 0x8a, 0x0f, 0xdb, 0x1d, 0x08, 0xe9 } }

namespace mozilla {
namespace dom {

class Text;




class InsertTextTxn : public EditTxn
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_INSERTTEXTTXN_IID)

  




  InsertTextTxn(Text& aTextNode, uint32_t aOffset, const nsAString& aString,
                nsEditor& aEditor);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(InsertTextTxn, EditTxn)
	
  NS_DECL_EDITTXN

  NS_IMETHOD Merge(nsITransaction* aTransaction, bool* aDidMerge) MOZ_OVERRIDE;

  
  void GetData(nsString& aResult);

private:
  virtual ~InsertTextTxn();

  
  bool IsSequentialInsert(InsertTextTxn& aOtherTxn);

  
  nsRefPtr<Text> mTextNode;

  
  uint32_t mOffset;

  
  nsString mStringToInsert;

  
  nsEditor& mEditor;
};

NS_DEFINE_STATIC_IID_ACCESSOR(InsertTextTxn, NS_INSERTTEXTTXN_IID)

}
}

#endif
