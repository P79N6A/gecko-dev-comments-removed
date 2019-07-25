







































#ifndef _nsAccEvent_H_
#define _nsAccEvent_H_

#include "nsIAccessibleEvent.h"

#include "nsAccessible.h"

class nsDocAccessible;


enum EIsFromUserInput
{
  
  eNoUserInput = 0,
  
  eFromUserInput = 1,
  
  eAutoDetect = -1
};

#define NS_ACCEVENT_IMPL_CID                            \
{  /* 39bde096-317e-4294-b23b-4af4a9b283f7 */           \
  0x39bde096,                                           \
  0x317e,                                               \
  0x4294,                                               \
  { 0xb2, 0x3b, 0x4a, 0xf4, 0xa9, 0xb2, 0x83, 0xf7 }    \
}

class nsAccEvent: public nsIAccessibleEvent
{
public:

  
  
  enum EEventRule {
     
     
     eAllowDupes,

     
     
     
     eCoalesceFromSameSubtree,

    
    
    eCoalesceFromSameDocument,

     
     
     eRemoveDupes,

     
     eDoNotEmit
  };

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCEVENT_IMPL_CID)

  
  nsAccEvent(PRUint32 aEventType, nsAccessible *aAccessible,
             PRBool aIsAsynch = PR_FALSE,
             EIsFromUserInput aIsFromUserInput = eAutoDetect,
             EEventRule aEventRule = eRemoveDupes);
  
  nsAccEvent(PRUint32 aEventType, nsINode *aNode, PRBool aIsAsynch = PR_FALSE,
             EIsFromUserInput aIsFromUserInput = eAutoDetect,
             EEventRule aEventRule = eRemoveDupes);
  virtual ~nsAccEvent() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(nsAccEvent)

  NS_DECL_NSIACCESSIBLEEVENT

  
  PRUint32 GetEventType() const { return mEventType; }
  EEventRule GetEventRule() const { return mEventRule; }
  PRBool IsAsync() const { return mIsAsync; }
  PRBool IsFromUserInput() const { return mIsFromUserInput; }

  nsAccessible *GetAccessible();
  nsINode* GetNode();
  nsDocAccessible* GetDocAccessible();

protected:
  


  nsAccessible *GetAccessibleForNode() const;

  



  void CaptureIsFromUserInput(EIsFromUserInput aIsFromUserInput);

  PRBool mIsFromUserInput;

  PRUint32 mEventType;
  EEventRule mEventRule;
  PRPackedBool mIsAsync;
  nsRefPtr<nsAccessible> mAccessible;
  nsCOMPtr<nsINode> mNode;

  friend class nsAccEventQueue;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccEvent, NS_ACCEVENT_IMPL_CID)


#define NS_ACCREORDEREVENT_IMPL_CID                     \
{  /* f2629eb8-2458-4358-868c-3912b15b767a */           \
  0xf2629eb8,                                           \
  0x2458,                                               \
  0x4358,                                               \
  { 0x86, 0x8c, 0x39, 0x12, 0xb1, 0x5b, 0x76, 0x7a }    \
}

class nsAccReorderEvent : public nsAccEvent
{
public:

  nsAccReorderEvent(nsAccessible *aAccTarget, PRBool aIsAsynch,
                    PRBool aIsUnconditional, nsINode *aReasonNode);

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCREORDEREVENT_IMPL_CID)

  NS_DECL_ISUPPORTS_INHERITED

  


  PRBool IsUnconditionalEvent();

  


  PRBool HasAccessibleInReasonSubtree();

private:
  PRBool mUnconditionalEvent;
  nsCOMPtr<nsINode> mReasonNode;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccReorderEvent, NS_ACCREORDEREVENT_IMPL_CID)


class nsAccStateChangeEvent: public nsAccEvent,
                             public nsIAccessibleStateChangeEvent
{
public:
  nsAccStateChangeEvent(nsAccessible *aAccessible,
                        PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled, PRBool aIsAsynch = PR_FALSE,
                        EIsFromUserInput aIsFromUserInput = eAutoDetect);

  nsAccStateChangeEvent(nsINode *aNode, PRUint32 aState, PRBool aIsExtraState,
                        PRBool aIsEnabled);

  nsAccStateChangeEvent(nsINode *aNode, PRUint32 aState, PRBool aIsExtraState);

  NS_DECL_ISUPPORTS_INHERITED
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
  nsAccTextChangeEvent(nsAccessible *aAccessible, PRInt32 aStart,
                       PRUint32 aLength, nsAString& aModifiedText,
                       PRBool aIsInserted, PRBool aIsAsynch = PR_FALSE,
                       EIsFromUserInput aIsFromUserInput = eAutoDetect);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETEXTCHANGEEVENT

private:
  PRInt32 mStart;
  PRUint32 mLength;
  PRBool mIsInserted;
  nsString mModifiedText;
};

class nsAccCaretMoveEvent: public nsAccEvent,
                           public nsIAccessibleCaretMoveEvent
{
public:
  nsAccCaretMoveEvent(nsAccessible *aAccessible, PRInt32 aCaretOffset);
  nsAccCaretMoveEvent(nsINode *aNode);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLECARETMOVEEVENT

private:
  PRInt32 mCaretOffset;
};

class nsAccTableChangeEvent : public nsAccEvent,
                              public nsIAccessibleTableChangeEvent {
public:
  nsAccTableChangeEvent(nsAccessible *aAccessible, PRUint32 aEventType,
                        PRInt32 aRowOrColIndex, PRInt32 aNumRowsOrCols,
                        PRBool aIsAsynch);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLETABLECHANGEEVENT

private:
  PRUint32 mRowOrColIndex;   
  PRUint32 mNumRowsOrCols;   
};

#endif

