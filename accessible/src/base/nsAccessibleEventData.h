







































#ifndef _nsAccessibleEventData_H_
#define _nsAccessibleEventData_H_

#include "nsCOMPtr.h"
#include "nsIAccessibleEvent.h"
#include "nsIAccessible.h"
#include "nsIAccessibleDocument.h"
#include "nsIDOMNode.h"
class nsIPresShell;

class nsAccEvent: public nsIAccessibleEvent
{
public:
  
  nsAccEvent(PRUint32 aEventType, nsIAccessible *aAccessible, void *aEventData, PRBool aIsAsynch = PR_FALSE);
  
  nsAccEvent(PRUint32 aEventType, nsIDOMNode *aDOMNode, void *aEventData, PRBool aIsAsynch = PR_FALSE);
  virtual ~nsAccEvent() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLEEVENT

  static void GetLastEventAttributes(nsIDOMNode *aNode,
                                      nsIPersistentProperties *aAttributes);
  static nsIDOMNode* GetLastEventAtomicRegion(nsIDOMNode *aNode);

protected:
  already_AddRefed<nsIAccessible> GetAccessibleByNode();

  void CaptureIsFromUserInput(PRBool aIsAsynch);
  PRBool mIsFromUserInput;

private:
  PRUint32 mEventType;
  nsCOMPtr<nsIAccessible> mAccessible;
  nsCOMPtr<nsIDOMNode> mDOMNode;
  nsCOMPtr<nsIAccessibleDocument> mDocAccessible;

  static PRBool gLastEventFromUserInput;
  static nsIDOMNode* gLastEventNodeWeak;

public:
  





  static void PrepareForEvent(nsIDOMNode *aChangeNode,
                              PRBool aForceIsFromUserInput = PR_FALSE);

  




  static void PrepareForEvent(nsIAccessibleEvent *aEvent);

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

