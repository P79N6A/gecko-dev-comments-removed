




#ifndef nsSupportsArray_h__
#define nsSupportsArray_h__



#include "nsISupportsArray.h"
#include "mozilla/Attributes.h"

static const uint32_t kAutoArraySize = 8;

class nsSupportsArray MOZ_FINAL : public nsISupportsArray
{
  ~nsSupportsArray(void); 

public:
  nsSupportsArray(void);
  static nsresult Create(nsISupports* aOuter, REFNSIID aIID, void** aResult);

  NS_DECL_THREADSAFE_ISUPPORTS

  NS_DECL_NSISERIALIZABLE

  
  NS_IMETHOD Count(uint32_t* aResult)
  {
    *aResult = mCount;
    return NS_OK;
  }
  NS_IMETHOD GetElementAt(uint32_t aIndex, nsISupports** aResult);
  NS_IMETHOD QueryElementAt(uint32_t aIndex, const nsIID& aIID, void** aResult)
  {
    if (aIndex < mCount) {
      nsISupports* element = mArray[aIndex];
      if (element) {
        return element->QueryInterface(aIID, aResult);
      }
    }
    return NS_ERROR_FAILURE;
  }
  NS_IMETHOD SetElementAt(uint32_t aIndex, nsISupports* aValue)
  {
    return ReplaceElementAt(aValue, aIndex) ? NS_OK : NS_ERROR_FAILURE;
  }
  NS_IMETHOD AppendElement(nsISupports* aElement)
  {
    
    return (nsresult)InsertElementAt(aElement, mCount);
  }
  
  NS_IMETHOD RemoveElement(nsISupports* aElement)
  {
    
    return (nsresult)RemoveElement(aElement, 0);
  }
  NS_IMETHOD_(bool) MoveElement(int32_t aFrom, int32_t aTo);
  NS_IMETHOD Enumerate(nsIEnumerator** aResult);
  NS_IMETHOD Clear(void);

  
  NS_IMETHOD_(bool) Equals(const nsISupportsArray* aOther);

  NS_IMETHOD_(int32_t) IndexOf(const nsISupports* aPossibleElement);
  NS_IMETHOD_(int32_t) IndexOfStartingAt(const nsISupports* aPossibleElement,
                                         uint32_t aStartIndex = 0);
  NS_IMETHOD_(int32_t) LastIndexOf(const nsISupports* aPossibleElement);

  NS_IMETHOD GetIndexOf(nsISupports* aPossibleElement, int32_t* aResult)
  {
    *aResult = IndexOf(aPossibleElement);
    return NS_OK;
  }

  NS_IMETHOD GetIndexOfStartingAt(nsISupports* aPossibleElement,
                                  uint32_t aStartIndex, int32_t* aResult)
  {
    *aResult = IndexOfStartingAt(aPossibleElement, aStartIndex);
    return NS_OK;
  }

  NS_IMETHOD GetLastIndexOf(nsISupports* aPossibleElement, int32_t* aResult)
  {
    *aResult = LastIndexOf(aPossibleElement);
    return NS_OK;
  }

  NS_IMETHOD_(bool) InsertElementAt(nsISupports* aElement, uint32_t aIndex);

  NS_IMETHOD_(bool) ReplaceElementAt(nsISupports* aElement, uint32_t aIndex);

  NS_IMETHOD_(bool) RemoveElementAt(uint32_t aIndex)
  {
    return RemoveElementsAt(aIndex, 1);
  }
  NS_IMETHOD_(bool) RemoveElement(const nsISupports* aElement,
                                  uint32_t aStartIndex = 0);
  NS_IMETHOD_(bool) RemoveLastElement(const nsISupports* aElement);

  NS_IMETHOD DeleteLastElement(nsISupports* aElement)
  {
    return (RemoveLastElement(aElement) ? NS_OK : NS_ERROR_FAILURE);
  }

  NS_IMETHOD DeleteElementAt(uint32_t aIndex)
  {
    return (RemoveElementAt(aIndex) ? NS_OK : NS_ERROR_FAILURE);
  }

  NS_IMETHOD_(bool) AppendElements(nsISupportsArray* aElements)
  {
    return InsertElementsAt(aElements, mCount);
  }

  NS_IMETHOD Compact(void);

  NS_IMETHOD Clone(nsISupportsArray** aResult);

  NS_IMETHOD_(bool) InsertElementsAt(nsISupportsArray* aOther,
                                     uint32_t aIndex);

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
  
  explicit nsSupportsArray(const nsISupportsArray& aOther);
};

#endif 
