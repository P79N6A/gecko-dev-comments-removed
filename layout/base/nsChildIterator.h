










































#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsINodeList.h"







class NS_STACK_CLASS ChildIterator
{
protected:
  nsIContent* mContent;
  
  
  
  union {
    PRUint32 mIndex;
    nsIContent* mChild;
  };
  nsINodeList* mNodes;

public:
  ChildIterator()
    : mContent(nsnull), mChild(0), mNodes(nsnull) {}

  ChildIterator(const ChildIterator& aOther)
    : mContent(aOther.mContent),
      mNodes(aOther.mNodes) {
    if (XBLInvolved()) {
      mIndex = aOther.mIndex;
    } else {
      mChild = aOther.mChild;
    }
  }

  ChildIterator& operator=(const ChildIterator& aOther) {
    mContent = aOther.mContent;
    mNodes = aOther.mNodes;
    if (XBLInvolved()) {
      mIndex = aOther.mIndex;
    } else {
      mChild = aOther.mChild;
    }
    return *this;
  }

  ChildIterator& operator++() {
    if (XBLInvolved()) {
      ++mIndex;
    } else {
      NS_ASSERTION(mChild, "Walking off end of list?");
      mChild = mChild->GetNextSibling();
    }

    return *this;
  }

  ChildIterator operator++(int) {
    ChildIterator result(*this);
    ++(*this);
    return result;
  }

  ChildIterator& operator--() {
    if (XBLInvolved()) {
      --mIndex;
    } else if (mChild) {
      mChild = mChild->GetPreviousSibling();
      NS_ASSERTION(mChild, "Walking off beginning of list");
    } else {
      mChild = mContent->GetLastChild();
    }
    return *this;
  }

  ChildIterator operator--(int) {
    ChildIterator result(*this);
    --(*this);
    return result;
  }

  nsIContent* get() const {
    if (XBLInvolved()) {
      return mNodes->GetNodeAt(mIndex);
    }

    return mChild;
  }

  nsIContent* operator*() const { return get(); }

  bool operator==(const ChildIterator& aOther) const {
    if (XBLInvolved()) {
      return mContent == aOther.mContent && mIndex == aOther.mIndex;
    }

    return mContent == aOther.mContent && mChild == aOther.mChild;
  }

  bool operator!=(const ChildIterator& aOther) const {
    return !aOther.operator==(*this);
  }

  void seek(nsIContent* aContent) {
    if (XBLInvolved()) {
      PRInt32 index = mNodes->IndexOf(aContent);
      
      
      
      
      
      
      if (index != -1) {
        mIndex = index;
      } else {
        
        
        mIndex = length();
      }
    } else if (aContent->GetParent() == mContent) {
      mChild = aContent;
    } else {
      
      
      
      mChild = nsnull;
    }
  }

  bool XBLInvolved() const { return mNodes != nsnull; }

  




  static nsresult Init(nsIContent*    aContent,
                       ChildIterator* aFirst,
                       ChildIterator* aLast);

private:
  PRUint32 length() {
    NS_PRECONDITION(XBLInvolved(), "Don't call me");
    PRUint32 l;
    mNodes->GetLength(&l);
    return l;
  }
};
