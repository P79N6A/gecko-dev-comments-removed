









































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
    
    AtkObject* oldAtkObj = mAtkObject;
    
    AtkObject* parentAtkObj = atk_object_get_parent(oldAtkObj);
    
    PRInt32 index = atk_object_get_index_in_parent(oldAtkObj);

    
    ShutdownAtkObject();

    
    GetAtkObject();

    
    if (parentAtkObj && (index >= 0)) {
      g_signal_emit_by_name(parentAtkObj, "children_changed::remove", index,
                            oldAtkObj, NULL);
      g_signal_emit_by_name(parentAtkObj, "children_changed::add", index,
                            mAtkObject, NULL);
    }

    
    nsCOMPtr<nsIAccessible> accChild;
    while (NextChild(accChild)) {
      if (IsEmbeddedObject(accChild)) {
        AtkObject* childAtkObj = nsAccessibleWrap::GetAtkObject(accChild);
        atk_object_set_parent(childAtkObj, mAtkObject);
      }
    }
  }
}

