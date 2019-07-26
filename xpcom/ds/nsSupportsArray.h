




#ifndef nsSupportsArray_h__
#define nsSupportsArray_h__



#include "nsISupportsArray.h"
#include "mozilla/Attributes.h"

static const PRUint32 kAutoArraySize = 8;

class nsSupportsArray MOZ_FINAL : public nsISupportsArray {
public:
  nsSupportsArray(void);
  ~nsSupportsArray(void); 

  static nsresult
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  NS_DECL_ISUPPORTS

  NS_DECL_NSISERIALIZABLE

  
  NS_IMETHOD Count(PRUint32 *result) { *result = mCount; return NS_OK; }
  NS_IMETHOD GetElementAt(PRUint32 aIndex, nsISupports* *result) {
    *result = ElementAt(aIndex);
    return NS_OK;
  }
  NS_IMETHOD QueryElementAt(PRUint32 aIndex, const nsIID & aIID, void * *aResult) {
    if (aIndex < mCount) {
      nsISupports* element = mArray[aIndex];
      if (nullptr != element)
        return element->QueryInterface(aIID, aResult);
    }
    return NS_ERROR_FAILURE;
  }
  NS_IMETHOD SetElementAt(PRUint32 aIndex, nsISupports* value) {
    return ReplaceElementAt(value, aIndex) ? NS_OK : NS_ERROR_FAILURE;
  }
  NS_IMETHOD AppendElement(nsISupports *aElement) {
    
    return (nsresult)InsertElementAt(aElement, mCount);
  }
  
  NS_IMETHOD RemoveElement(nsISupports *aElement) {
    
    return (nsresult)RemoveElement(aElement, 0);
  }
  NS_IMETHOD_(bool) MoveElement(PRInt32 aFrom, PRInt32 aTo);
  NS_IMETHOD Enumerate(nsIEnumerator* *result);
  NS_IMETHOD Clear(void);

  
  NS_IMETHOD_(bool) Equals(const nsISupportsArray* aOther);

  NS_IMETHOD_(nsISupports*) ElementAt(PRUint32 aIndex);

  NS_IMETHOD_(PRInt32) IndexOf(const nsISupports* aPossibleElement);
  NS_IMETHOD_(PRInt32) IndexOfStartingAt(const nsISupports* aPossibleElement,
                                         PRUint32 aStartIndex = 0);
  NS_IMETHOD_(PRInt32) LastIndexOf(const nsISupports* aPossibleElement);

  NS_IMETHOD GetIndexOf(nsISupports *aPossibleElement, PRInt32 *_retval) {
    *_retval = IndexOf(aPossibleElement);
    return NS_OK;
  }
  
  NS_IMETHOD GetIndexOfStartingAt(nsISupports *aPossibleElement,
                                  PRUint32 aStartIndex, PRInt32 *_retval) {
    *_retval = IndexOfStartingAt(aPossibleElement, aStartIndex);
    return NS_OK;
  }
  
  NS_IMETHOD GetLastIndexOf(nsISupports *aPossibleElement, PRInt32 *_retval) {
    *_retval = LastIndexOf(aPossibleElement);
    return NS_OK;
  }
  
  NS_IMETHOD_(bool) InsertElementAt(nsISupports* aElement, PRUint32 aIndex);

  NS_IMETHOD_(bool) ReplaceElementAt(nsISupports* aElement, PRUint32 aIndex);

  NS_IMETHOD_(bool) RemoveElementAt(PRUint32 aIndex) {
    return RemoveElementsAt(aIndex,1);
  }
  NS_IMETHOD_(bool) RemoveElement(const nsISupports* aElement, PRUint32 aStartIndex = 0);
  NS_IMETHOD_(bool) RemoveLastElement(const nsISupports* aElement);

  NS_IMETHOD DeleteLastElement(nsISupports *aElement) {
    return (RemoveLastElement(aElement) ? NS_OK : NS_ERROR_FAILURE);
  }
  
  NS_IMETHOD DeleteElementAt(PRUint32 aIndex) {
    return (RemoveElementAt(aIndex) ? NS_OK : NS_ERROR_FAILURE);
  }
  
  NS_IMETHOD_(bool) AppendElements(nsISupportsArray* aElements) {
    return InsertElementsAt(aElements,mCount);
  }
  
  NS_IMETHOD Compact(void);

  NS_IMETHOD_(bool) EnumerateForwards(nsISupportsArrayEnumFunc aFunc, void* aData);
  NS_IMETHOD_(bool) EnumerateBackwards(nsISupportsArrayEnumFunc aFunc, void* aData);

  NS_IMETHOD Clone(nsISupportsArray **_retval);

  NS_IMETHOD_(bool) InsertElementsAt(nsISupportsArray *aOther, PRUint32 aIndex);

  NS_IMETHOD_(bool) RemoveElementsAt(PRUint32 aIndex, PRUint32 aCount);

  NS_IMETHOD_(bool) SizeTo(PRInt32 aSize);
protected:
  void DeleteArray(void);

  NS_IMETHOD_(bool) GrowArrayBy(PRInt32 aGrowBy);

  nsISupports** mArray;
  PRUint32 mArraySize;
  PRUint32 mCount;
  nsISupports*  mAutoArray[kAutoArraySize];
#if DEBUG_SUPPORTSARRAY
  PRUint32 mMaxCount;
  PRUint32 mMaxSize;
#endif

private:
  
  nsSupportsArray(const nsISupportsArray& other);
};

#endif 
