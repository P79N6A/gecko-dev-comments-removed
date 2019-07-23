




































#ifndef nsSupportsArray_h__
#define nsSupportsArray_h__



#include "nsISupportsArray.h"

static const PRUint32 kAutoArraySize = 8;



#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY

class NS_COM nsSupportsArray : public nsISupportsArray {
public:
  nsSupportsArray(void);
  ~nsSupportsArray(void); 

  static NS_METHOD
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
      if (nsnull != element)
        return element->QueryInterface(aIID, aResult);
    }
    return NS_ERROR_FAILURE;
  }
  NS_IMETHOD SetElementAt(PRUint32 aIndex, nsISupports* value) {
    return ReplaceElementAt(value, aIndex) ? NS_OK : NS_ERROR_FAILURE;
  }
  NS_IMETHOD AppendElement(nsISupports *aElement) {
    return InsertElementAt(aElement, mCount);
  }
  
  NS_IMETHOD RemoveElement(nsISupports *aElement) {
    return RemoveElement(aElement, 0);
  }
  NS_IMETHOD_(PRBool) MoveElement(PRInt32 aFrom, PRInt32 aTo);
  NS_IMETHOD Enumerate(nsIEnumerator* *result);
  NS_IMETHOD Clear(void);

  
  NS_IMETHOD_(PRBool) Equals(const nsISupportsArray* aOther);

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
  
  NS_IMETHOD_(PRBool) InsertElementAt(nsISupports* aElement, PRUint32 aIndex);

  NS_IMETHOD_(PRBool) ReplaceElementAt(nsISupports* aElement, PRUint32 aIndex);

  NS_IMETHOD_(PRBool) RemoveElementAt(PRUint32 aIndex) {
    return RemoveElementsAt(aIndex,1);
  }
  NS_IMETHOD_(PRBool) RemoveElement(const nsISupports* aElement, PRUint32 aStartIndex = 0);
  NS_IMETHOD_(PRBool) RemoveLastElement(const nsISupports* aElement);

  NS_IMETHOD DeleteLastElement(nsISupports *aElement) {
    return (RemoveLastElement(aElement) ? NS_OK : NS_ERROR_FAILURE);
  }
  
  NS_IMETHOD DeleteElementAt(PRUint32 aIndex) {
    return (RemoveElementAt(aIndex) ? NS_OK : NS_ERROR_FAILURE);
  }
  
  NS_IMETHOD_(PRBool) AppendElements(nsISupportsArray* aElements) {
    return InsertElementsAt(aElements,mCount);
  }
  
  NS_IMETHOD Compact(void);

  NS_IMETHOD_(PRBool) EnumerateForwards(nsISupportsArrayEnumFunc aFunc, void* aData);
  NS_IMETHOD_(PRBool) EnumerateBackwards(nsISupportsArrayEnumFunc aFunc, void* aData);

  NS_IMETHOD Clone(nsISupportsArray **_retval);

  NS_IMETHOD_(PRBool) InsertElementsAt(nsISupportsArray *aOther, PRUint32 aIndex);

  NS_IMETHOD_(PRBool) RemoveElementsAt(PRUint32 aIndex, PRUint32 aCount);

  NS_IMETHOD_(PRBool) SizeTo(PRInt32 aSize);
protected:
  void DeleteArray(void);

  NS_IMETHOD_(PRBool) GrowArrayBy(PRInt32 aGrowBy);

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
