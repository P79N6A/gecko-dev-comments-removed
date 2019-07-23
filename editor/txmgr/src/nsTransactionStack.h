




































#ifndef nsTransactionStack_h__
#define nsTransactionStack_h__

#include "nsDeque.h"

class nsTransactionItem;

class nsTransactionReleaseFunctor : public nsDequeFunctor
{
public:

  nsTransactionReleaseFunctor()          {}
  virtual ~nsTransactionReleaseFunctor() {}
  virtual void *operator()(void *aObject);
};

class nsTransactionStack
{
  nsDeque mQue;

public:

  nsTransactionStack();
  virtual ~nsTransactionStack();

  virtual nsresult Push(nsTransactionItem *aTransactionItem);
  virtual nsresult Pop(nsTransactionItem **aTransactionItem);
  virtual nsresult PopBottom(nsTransactionItem **aTransactionItem);
  virtual nsresult Peek(nsTransactionItem **aTransactionItem);
  virtual nsresult GetItem(PRInt32 aIndex, nsTransactionItem **aTransactionItem);
  virtual nsresult Clear(void);
  virtual nsresult GetSize(PRInt32 *aStackSize);
};

class nsTransactionRedoStack: public nsTransactionStack
{
public:

  virtual ~nsTransactionRedoStack();
  virtual nsresult Clear(void);
};

#endif 
