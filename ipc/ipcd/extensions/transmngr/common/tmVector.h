




































#ifndef _tmVector_H_
#define _tmVector_H_

#include "tmUtils.h"

#define GROWTH_INC 5















class tmVector
{
public:

  
  

  




  tmVector() : mNext(0), mCount(0), mCapacity(10), mElements(nsnull) {;}

  


  virtual ~tmVector();

  
  

  





  nsresult Init();

  

  



  PRInt32 Append(void *aElement);

  



  void Remove(void *aElement);

  



  void RemoveAt(PRUint32 aIndex);

  



  void Clear();

  



  void* operator[](PRUint32 index) { 
    PR_ASSERT(index < mNext);
    return mElements[index]; 
  }

  


  PRUint32 Count() { return mCount; }

  








  PRUint32 Size() { return mNext; }

protected:

  nsresult Grow();     
  nsresult Shrink();   

  
  

  
  PRUint32 mNext;             
  PRUint32 mCount;            
  PRUint32 mCapacity;         

  
  void **mElements;

private:

};

#endif
