






















































#ifndef _NSDEQUE
#define _NSDEQUE

#include "nscore.h"








class nsDequeFunctor{
public:
  virtual void* operator()(void* anObject)=0;
};















class nsDequeIterator;

class NS_COM_GLUE nsDeque {
  friend class nsDequeIterator;
  public:
   nsDeque(nsDequeFunctor* aDeallocator = nsnull);
  ~nsDeque();

  





  inline PRInt32 GetSize() const {return mSize;}

  





  nsDeque& Push(void* aItem);

  





  nsDeque& PushFront(void* aItem);

  




  void* Pop();

  




  void* PopFront();

  





  void* Peek();
  




  void* PeekFront();

  





  void* ObjectAt(int aIndex) const;

  




  nsDeque& Empty();

  






  nsDeque& Erase();

  





  nsDequeIterator Begin() const;

  





  nsDequeIterator End() const;

  void* Last() const;
  







  void ForEach(nsDequeFunctor& aFunctor) const;

  








  const void* FirstThat(nsDequeFunctor& aFunctor) const;

  void SetDeallocator(nsDequeFunctor* aDeallocator);

protected:
  PRInt32         mSize;
  PRInt32         mCapacity;
  PRInt32         mOrigin;
  nsDequeFunctor* mDeallocator;
  void*           mBuffer[8];
  void**          mData;

private:

  




  nsDeque(const nsDeque& other);

  





  nsDeque& operator=(const nsDeque& anOther);

  PRBool GrowCapacity();
};





class NS_COM_GLUE nsDequeIterator {
public:
  
















  nsDequeIterator(const nsDeque& aQueue, int aIndex=0);

  




  nsDequeIterator(const nsDequeIterator& aCopy);

  



  nsDequeIterator& First();

  




  nsDequeIterator& operator=(const nsDequeIterator& aCopy);

  






  PRBool operator!=(nsDequeIterator& aIter);

  








  PRBool operator<(nsDequeIterator& aIter);

  





  PRBool operator==(nsDequeIterator& aIter);

  








  PRBool operator>=(nsDequeIterator& aIter);

  





  void* operator++();

  






  void* operator++(int);

  





  void* operator--();

  






  void* operator--(int);

  














  void* GetCurrent();

  







  void ForEach(nsDequeFunctor& aFunctor) const;

  








  const void* FirstThat(nsDequeFunctor& aFunctor) const;

  protected:

  PRInt32         mIndex;
  const nsDeque&  mDeque;
};
#endif
