




































#include <stdlib.h>
#include "tmVector.h"






tmVector::~tmVector() {
  if (mElements)
    free((void*)mElements);
}




nsresult
tmVector::Init() {

  mElements = (void**) calloc (mCapacity, sizeof(void*));
  if (!mElements)
    return NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}




PRInt32
tmVector::Append(void *aElement){
  PR_ASSERT(aElement);

  
  if (mNext == mCapacity)
    if (NS_FAILED(Grow()))
      return -1;

  
  mElements[mNext] = aElement;
  mCount++;

  
  return mNext++; 
}

void
tmVector::Remove(void *aElement) {
  PR_ASSERT(aElement);

  for (PRUint32 index = 0; index < mNext; index++) {
    if (mElements[index] == aElement) {
      mElements[index] = nsnull;
      mCount--;
      if (index == mNext-1) {   
        mNext--;
        
        Shrink();
      }
    }
  }
}

void
tmVector::RemoveAt(PRUint32 aIndex) {
  PR_ASSERT(aIndex < mNext);

  
  if (mElements[aIndex] != nsnull) {
    mElements[aIndex] = nsnull;
    mCount--;
    if (aIndex == mNext-1) {   
      mNext--;
      
      Shrink();
    }
  }
}









void
tmVector::Clear(){
  memset(mElements, 0, mCapacity);
  mCount = 0;
  mNext = 0;
}





nsresult
tmVector::Grow() {

  PRUint32 newcap = mCapacity + GROWTH_INC;
  mElements = (void**) realloc(mElements, (newcap * sizeof(void*)));
  if (mElements) {
    mCapacity = newcap;
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}



nsresult
tmVector::Shrink() {

  PRUint32 newcap = mCapacity - GROWTH_INC;
  if (mNext < newcap) {
    mElements = (void**) realloc(mElements, newcap * sizeof(void*));
    if (!mElements)
      return NS_ERROR_OUT_OF_MEMORY;
    mCapacity = newcap;
  }
  return NS_OK;
}
