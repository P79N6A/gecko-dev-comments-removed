










































#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsINodeList.h"







class NS_STACK_CLASS ChildIterator
{
protected:
  
  
  

  
  nsCOMPtr<nsIContent> mContent;
  PRUint32 mIndex;
  nsCOMPtr<nsINodeList> mNodes;

public:
  ChildIterator()
    : mIndex(0) {}

  ChildIterator(const ChildIterator& aOther)
    : mContent(aOther.mContent),
      mIndex(aOther.mIndex),
      mNodes(aOther.mNodes) {}

  ChildIterator& operator=(const ChildIterator& aOther) {
    mContent = aOther.mContent;
    mIndex = aOther.mIndex;
    mNodes = aOther.mNodes;
    return *this;
  }

  ChildIterator& operator++() {
    ++mIndex;
    return *this;
  }

  ChildIterator operator++(int) {
    ChildIterator result(*this);
    ++mIndex;
    return result;
  }

  ChildIterator& operator--() {
    --mIndex;
    return *this;
  }

  ChildIterator operator--(int) {
    ChildIterator result(*this);
    --mIndex;
    return result;
  }

  nsIContent* get() const {
    if (XBLInvolved()) {
      return mNodes->GetNodeAt(mIndex);
    }

    return mContent->GetChildAt(mIndex);
  }

  nsIContent* operator*() const { return get(); }

  PRBool operator==(const ChildIterator& aOther) const {
    return mContent == aOther.mContent && mIndex == aOther.mIndex;
  }

  PRBool operator!=(const ChildIterator& aOther) const {
    return !aOther.operator==(*this);
  }

  PRUint32 position() {
    return mIndex;
  }

  void seek(PRUint32 aIndex) {
    
    
    
    PRUint32 l = length();

    NS_ASSERTION(PRInt32(aIndex) >= 0 && aIndex <= l, "out of bounds");

    
    if (aIndex > l)
      aIndex = l;

    mIndex = aIndex;
  }

  void seek(nsIContent* aContent) {
    PRInt32 index;
    if (XBLInvolved()) {
      index = mNodes->IndexOf(aContent);
    } else {
      index = mContent->IndexOf(aContent);
    }
    
    
    
    
    
    
    
    
    if (index != -1) {
      mIndex = index;
    } else {
      
      
      mIndex = length();
    }
  }

  PRBool XBLInvolved() const { return mNodes != nsnull; }

  




  static nsresult Init(nsIContent*    aContent,
                       ChildIterator* aFirst,
                       ChildIterator* aLast);

private:
  PRUint32 length() {
    PRUint32 l;
    if (XBLInvolved()) {
      mNodes->GetLength(&l);
    } else {
      l = mContent->GetChildCount();
    }
    return l;
  }
};
