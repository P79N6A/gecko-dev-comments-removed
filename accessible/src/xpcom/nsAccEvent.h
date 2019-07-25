





































#ifndef _nsAccEvent_H_
#define _nsAccEvent_H_

#include "nsIAccessibleEvent.h"

#include "AccEvent.h"




class nsAccEvent: public nsIAccessibleEvent
{
public:
  nsAccEvent(AccEvent* aEvent) : mEvent(aEvent) { }
  virtual ~nsAccEvent() { }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLEEVENT

protected:
  nsRefPtr<AccEvent> mEvent;

private:
  nsAccEvent();
  nsAccEvent(const nsAccEvent&);
  nsAccEvent& operator =(const nsAccEvent&);
};





class nsAccStateChangeEvent: public nsAccEvent,
                             public nsIAccessibleStateChangeEvent
{
public:
  nsAccStateChangeEvent(AccStateChangeEvent* aEvent) : nsAccEvent(aEvent) { }
  virtual ~nsAccStateChangeEvent() { }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLESTATECHANGEEVENT

private:
  nsAccStateChangeEvent();
  nsAccStateChangeEvent(const nsAccStateChangeEvent&);
  nsAccStateChangeEvent& operator =(const nsAccStateChangeEvent&);
};





class nsAccTextChangeEvent: public nsAccEvent,
                            public nsIAccessibleTextChangeEvent
{
public:
  nsAccTextChangeEvent(AccTextChangeEvent* aEvent) : nsAccEvent(aEvent) { }
  virtual ~nsAccTextChangeEvent() { }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETEXTCHANGEEVENT

private:
  nsAccTextChangeEvent();
  nsAccTextChangeEvent(const nsAccTextChangeEvent&);
  nsAccTextChangeEvent& operator =(const nsAccTextChangeEvent&);
};





class nsAccCaretMoveEvent: public nsAccEvent,
                           public nsIAccessibleCaretMoveEvent
{
public:
  nsAccCaretMoveEvent(AccCaretMoveEvent* aEvent) : nsAccEvent(aEvent) { }
  virtual ~nsAccCaretMoveEvent() { }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLECARETMOVEEVENT

private:
  nsAccCaretMoveEvent();
  nsAccCaretMoveEvent(const nsAccCaretMoveEvent&);
  nsAccCaretMoveEvent& operator =(const nsAccCaretMoveEvent&);
};





class nsAccTableChangeEvent : public nsAccEvent,
                              public nsIAccessibleTableChangeEvent
{
public:
  nsAccTableChangeEvent(AccTableChangeEvent* aEvent) : nsAccEvent(aEvent) { }
  virtual ~nsAccTableChangeEvent() { }

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETABLECHANGEEVENT

private:
  nsAccTableChangeEvent();
  nsAccTableChangeEvent(const nsAccTableChangeEvent&);
  nsAccTableChangeEvent& operator =(const nsAccTableChangeEvent&);
};

#endif

