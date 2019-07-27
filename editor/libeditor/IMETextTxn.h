




#ifndef IMETextTxn_h__
#define IMETextTxn_h__

#include "EditTxn.h"                      
#include "nsAutoPtr.h"                    
#include "nsCycleCollectionParticipant.h" 
#include "nsString.h"                     

class nsEditor;

#define NS_IMETEXTTXN_IID \
  { 0xb391355d, 0x346c, 0x43d1, \
    { 0x85, 0xed, 0x9e, 0x65, 0xbe, 0xe7, 0x7e, 0x48 } }

namespace mozilla {

class TextRangeArray;

namespace dom {

class Text;




class IMETextTxn : public EditTxn
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IMETEXTTXN_IID)

  






  IMETextTxn(Text& aTextNode, uint32_t aOffset, uint32_t aReplaceLength,
             TextRangeArray* aTextRangeArray, const nsAString& aString,
             nsEditor& aEditor);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IMETextTxn, EditTxn)

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_EDITTXN

  NS_IMETHOD Merge(nsITransaction* aTransaction, bool* aDidMerge) MOZ_OVERRIDE;

  void MarkFixed();

private:
  ~IMETextTxn();

  nsresult SetSelectionForRanges();

  
  nsRefPtr<Text> mTextNode;

  
  uint32_t mOffset;

  uint32_t mReplaceLength;

  
  nsRefPtr<mozilla::TextRangeArray> mRanges;

  
  nsString mStringToInsert;

  
  nsEditor& mEditor;

  bool mFixed;
};

NS_DEFINE_STATIC_IID_ACCESSOR(IMETextTxn, NS_IMETEXTTXN_IID)

}
}

#endif
