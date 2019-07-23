







































#ifndef _nsAccessibleEventData_H_
#define _nsAccessibleEventData_H_

#include "nsCOMPtr.h"
#include "nsIAccessibleEvent.h"
#include "nsIAccessible.h"
#include "nsIAccessibleDocument.h"
#include "nsIDOMNode.h"

class nsAccEvent: public nsIAccessibleEvent
{
public:
  
  nsAccEvent(PRUint32 aEventType, nsIAccessible *aAccessible, void *aEventData);
  
  nsAccEvent(PRUint32 aEventType, nsIDOMNode *aDOMNode, void *aEventData);
  virtual ~nsAccEvent() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLEEVENT

protected:
  already_AddRefed<nsIAccessible> GetAccessibleByNode();

private:
  PRUint32 mEventType;
  nsCOMPtr<nsIAccessible> mAccessible;
  nsCOMPtr<nsIDOMNode> mDOMNode;
  nsCOMPtr<nsIAccessibleDocument> mDocAccessible;

public:
  void *mEventData;
};

class nsAccStateChangeEvent: public nsAccEvent,
                             public nsIAccessibleStateChangeEvent
{
public:
  nsAccStateChangeEvent(nsIAccessible *aAccessible,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled);

  nsAccStateChangeEvent(nsIDOMNode *aNode,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled);

  nsAccStateChangeEvent(nsIDOMNode *aNode,
                        PRUint32 aState, PRBool aIsExtraState);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIACCESSIBLEEVENT(nsAccEvent::)
  NS_DECL_NSIACCESSIBLESTATECHANGEEVENT

private:
  PRUint32 mState;
  PRBool mIsExtraState;
  PRBool mIsEnabled;
};

class nsAccTextChangeEvent: public nsAccEvent,
                            public nsIAccessibleTextChangeEvent
{
public:
  nsAccTextChangeEvent(nsIAccessible *aAccessible,
                       PRInt32 aStart, PRUint32 aLength, PRBool aIsInserted);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIACCESSIBLEEVENT(nsAccEvent::)
  NS_DECL_NSIACCESSIBLETEXTCHANGEEVENT

private:
  PRInt32 mStart;
  PRUint32 mLength;
  PRBool mIsInserted;
};

class nsAccCaretMoveEvent: public nsAccEvent,
                           public nsIAccessibleCaretMoveEvent
{
public:
  nsAccCaretMoveEvent(nsIAccessible *aAccessible, PRInt32 aCaretOffset);
  nsAccCaretMoveEvent(nsIDOMNode *aNode);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIACCESSIBLEEVENT(nsAccEvent::)
  NS_DECL_NSIACCESSIBLECARETMOVEEVENT

private:
  PRInt32 mCaretOffset;
};




struct AtkTableChange {
  PRUint32 index;   
  PRUint32 count;   
};

#endif

