










































#include "nsCOMPtr.h"
#include "nsIContent.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMNode.h"







class ChildIterator
{
protected:
  nsCOMPtr<nsIContent> mContent;
  PRUint32 mIndex;
  nsCOMPtr<nsIDOMNodeList> mNodes;

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

  already_AddRefed<nsIContent> get() const {
    nsIContent* result = nsnull;
    if (mNodes) {
      nsCOMPtr<nsIDOMNode> node;
      mNodes->Item(mIndex, getter_AddRefs(node));
      CallQueryInterface(node, &result);
    } else {
      result = mContent->GetChildAt(PRInt32(mIndex));
      NS_IF_ADDREF(result);
    }

    return result;
  }

  already_AddRefed<nsIContent> operator*() const { return get(); }

  PRBool operator==(const ChildIterator& aOther) const {
    return mContent == aOther.mContent && mIndex == aOther.mIndex;
  }

  PRBool operator!=(const ChildIterator& aOther) const {
    return !aOther.operator==(*this);
  }

  PRUint32 index() {
    return mIndex;
  }

  void seek(PRUint32 aIndex) {
    
    
    
    PRUint32 length;
    if (mNodes)
      mNodes->GetLength(&length);
    else
      length = mContent->GetChildCount();

    NS_ASSERTION(PRInt32(aIndex) >= 0 && aIndex <= length, "out of bounds");

    
    if (aIndex > length)
      aIndex = length;

    mIndex = aIndex;
  }

  




  static nsresult Init(nsIContent*    aContent,
                       ChildIterator* aFirst,
                       ChildIterator* aLast);
};
