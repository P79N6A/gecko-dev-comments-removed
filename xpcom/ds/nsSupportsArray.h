




#ifndef nsSupportsArray_h__
#define nsSupportsArray_h__



#include "nsISupportsArray.h"
#include "mozilla/Attributes.h"

static const uint32_t kAutoArraySize = 8;

class nsSupportsArray MOZ_FINAL : public nsISupportsArray {
public:
  nsSupportsArray(void);
  ~nsSupportsArray(void); 

  static nsresult
  Create(nsISupports *aOuter, REFNSIID aIID, void **aResult);

  NS_DECL_ISUPPORTS

  NS_DECL_NSISERIALIZABLE

  
  NS_IMETHOD Count(uint32_t *result) { *result = mCount; return NS_OK; }
  NS_IMETHOD GetElementAt(uint32_t aIndex, nsISupports* *result) {
    *result = ElementAt(aIndex);
    return NS_OK;
  }
  NS_IMETHOD QueryElementAt(uint32_t aIndex, const nsIID & aIID, void * *aResult) {
    if (aIndex < mCount) {
      nsISupports* element = mArray[aIndex];
      if (nullptr != element)
        return element->QueryInterface(aIID, aResult);
    }
    return NS_ERROR_FAILURE;
  }
  NS_IMETHOD SetElementAt(uint32_t aIndex, nsISupports* value) {
    return ReplaceElementAt(value, aIndex) ? NS_OK : NS_ERROR_FAILURE;
  }
  NS_IMETHOD AppendElement(nsISupports *aElement) {
    
    return (nsresult)InsertElementAt(aElement, mCount);
  }
  
  NS_IMETHOD RemoveElement(nsISupports *aElement) {
    
    return (nsresult)RemoveElement(aElement, 0);
  }
  NS_IMETHOD_(bool) MoveElement(int32_t aFrom, int32_t aTo);
  NS_IMETHOD Enumerate(nsIEnumerator* *result);
  NS_IMETHOD Clear(void);

  
  NS_IMETHOD_(bool) Equals(const nsISupportsArray* aOther);

  NS_IMETHOD_(nsISupports*) ElementAt(uint32_t aIndex);

  NS_IMETHOD_(int32_t) IndexOf(const nsISupports* aPossibleElement);
  NS_IMETHOD_(int32_t) IndexOfStartingAt(const nsISupports* aPossibleElement,
                                         uint32_t aStartIndex = 0);
  NS_IMETHOD_(int32_t) LastIndexOf(const nsISupports* aPossibleElement);

  NS_IMETHOD GetIndexOf(nsISupports *aPossibleElement, int32_t *_retval) {
    *_retval = IndexOf(aPossibleElement);
    return NS_OK;
  }
  
  NS_IMETHOD GetIndexOfStartingAt(nsISupports *aPossibleElement,
                                  uint32_t aStartIndex, int32_t *_retval) {
    *_retval = IndexOfStartingAt(aPossibleElement, aStartIndex);
    return NS_OK;
  }
  
  NS_IMETHOD GetLastIndexOf(nsISupports *aPossibleElement, int32_t *_retval) {
    *_retval = LastIndexOf(aPossibleElement);
    return NS_OK;
  }
  
  NS_IMETHOD_(bool) InsertElementAt(nsISupports* aElement, uint32_t aIndex);

  NS_IMETHOD_(bool) ReplaceElementAt(nsISupports* aElement, uint32_t aIndex);

  NS_IMETHOD_(bool) RemoveElementAt(uint32_t aIndex) {
    return RemoveElementsAt(aIndex,1);
  }
  NS_IMETHOD_(bool) RemoveElement(const nsISupports* aElement, uint32_t aStartIndex = 0);
  NS_IMETHOD_(bool) RemoveLastElement(const nsISupports* aElement);

  NS_IMETHOD DeleteLastElement(nsISupports *aElement) {
    return (RemoveLastElement(aElement) ? NS_OK : NS_ERROR_FAILURE);
  }
  
  NS_IMETHOD DeleteElementAt(uint32_t aIndex) {
    return (RemoveElementAt(aIndex) ? NS_OK : NS_ERROR_FAILURE);
  }
  
  NS_IMETHOD_(bool) AppendElements(nsISupportsArray* aElements) {
    return InsertElementsAt(aElements,mCount);
  }
  
  NS_IMETHOD Compact(void);

  NS_IMETHOD_(bool) EnumerateForwards(nsISupportsArrayEnumFunc aFunc, void* aData);
  NS_IMETHOD_(bool) EnumerateBackwards(nsISupportsArrayEnumFunc aFunc, void* aData);

  NS_IMETHOD Clone(nsISupportsArray **_retval);

  NS_IMETHOD_(bool) InsertElementsAt(nsISupportsArray *aOther, uint32_t aIndex);

  NS_IMETHOD_(bool) RemoveElementsAt(uint32_t aIndex, uint32_t aCount);

  NS_IMETHOD_(bool) SizeTo(int32_t aSize);
protected:
  void DeleteArray(void);

  NS_IMETHOD_(void) GrowArrayBy(int32_t aGrowBy);

  nsISupports** mArray;
  uint32_t mArraySize;
  uint32_t mCount;
  nsISupports*  mAutoArray[kAutoArraySize];
#if DEBUG_SUPPORTSARRAY
  uint32_t mMaxCount;
  uint32_t mMaxSize;
#endif

private:
  
  nsSupportsArray(const nsISupportsArray& other);
};

#endif 
