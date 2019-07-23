






































#ifndef _nsAccessibleEventData_H_
#define _nsAccessibleEventData_H_

#include "nsCOMPtr.h"
#include "nsIAccessibleEvent.h"
#include "nsIAccessible.h"
#include "nsIAccessibleDocument.h"
#include "nsIDOMNode.h"

class nsAccessibleEventData: public nsIAccessibleEvent
{
  public:
    
    nsAccessibleEventData(PRUint32 aEventType, nsIAccessible *aAccessible, 
                          nsIAccessibleDocument *aDocAccessible, 
                          void *aEventData);
    
    nsAccessibleEventData(PRUint32 aEventType, nsIDOMNode *aDOMNode,
                          nsIAccessibleDocument *aDocAccessible,
                          void *aEventData);
    virtual ~nsAccessibleEventData() {};

    NS_DECL_ISUPPORTS

    
    NS_IMETHOD GetEventType(PRUint32 *aEventType) {*aEventType = mEventType; return NS_OK;}
    NS_IMETHOD GetAccessible(nsIAccessible **aAccessible);
    NS_IMETHOD GetAccessibleDocument(nsIAccessibleDocument **aDocAccessible) 
      {NS_ADDREF(*aDocAccessible = mDocAccessible); return NS_OK;}
    NS_IMETHOD GetDOMNode(nsIDOMNode **aDOMNode);

  private:
    PRUint32 mEventType;
    nsCOMPtr<nsIAccessible> mAccessible;
    nsCOMPtr<nsIDOMNode> mDOMNode;
    nsCOMPtr<nsIAccessibleDocument> mDocAccessible;
    void *mEventData;
};





struct StateChange {
  PRUint32 state;
  PRBool   isExtendedState;
  PRBool   enable;
  StateChange() {
    state = 0;
    isExtendedState = PR_FALSE;
    enable = PR_FALSE;
  }
};

enum AtkProperty {
  PROP_0,           
  PROP_NAME,
  PROP_DESCRIPTION,
  PROP_PARENT,      
  PROP_ROLE,
  PROP_LAYER,
  PROP_MDI_ZORDER,
  PROP_TABLE_CAPTION,
  PROP_TABLE_COLUMN_DESCRIPTION,
  PROP_TABLE_COLUMN_HEADER,
  PROP_TABLE_ROW_DESCRIPTION,
  PROP_TABLE_ROW_HEADER,
  PROP_TABLE_SUMMARY,
  PROP_LAST         
};

struct AtkPropertyChange {
  PRInt32 type;     
  void *oldvalue;  
  void *newvalue;
};

struct AtkChildrenChange {
  PRInt32      index;  
  nsIAccessible *child;   
  PRBool        add;    
};

struct AtkTextChange {
  PRInt32  start;
  PRUint32 length;
  PRBool   add;     
};

struct AtkTableChange {
  PRUint32 index;   
  PRUint32 count;   
};

#endif  
