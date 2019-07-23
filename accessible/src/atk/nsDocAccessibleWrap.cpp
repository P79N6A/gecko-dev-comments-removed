









































#include "nsMai.h"
#include "nsDocAccessibleWrap.h"



nsDocAccessibleWrap::nsDocAccessibleWrap(nsIDOMNode *aDOMNode,
                                         nsIWeakReference *aShell): 
  nsDocAccessible(aDOMNode, aShell), mActivated(PR_FALSE)
{
}

nsDocAccessibleWrap::~nsDocAccessibleWrap()
{
}

void nsDocAccessibleWrap::SetEditor(nsIEditor* aEditor)
{
  
  PRBool needRecreate = mAtkObject && (mEditor != aEditor)
                                   && (!mEditor || !aEditor);
  nsDocAccessible::SetEditor(aEditor);

  if (needRecreate) {
    
    ShutdownAtkObject();

    
    GetAtkObject();

    
    nsCOMPtr<nsIAccessible> accChild;
    while (NextChild(accChild)) {
      if (IsEmbeddedObject(accChild)) {
        AtkObject* childAtkObj = nsAccessibleWrap::GetAtkObject(accChild);
        atk_object_set_parent(childAtkObj, mAtkObject);
      }
    }
  }
}

