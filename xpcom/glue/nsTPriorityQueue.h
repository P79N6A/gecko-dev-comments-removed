





#ifndef NS_TPRIORITY_QUEUE_H_
#define NS_TPRIORITY_QUEUE_H_

#include "nsTArray.h"
#include "nsDebug.h"






template<class T, class Compare = nsDefaultComparator<T, T>>
class nsTPriorityQueue
{
public:
  typedef typename nsTArray<T>::size_type size_type;

  



  nsTPriorityQueue() : mCompare(Compare()) {}

  



  explicit nsTPriorityQueue(const Compare& aComp) : mCompare(aComp) {}

  


  nsTPriorityQueue(const nsTPriorityQueue& aOther)
    : mElements(aOther.mElements)
    , mCompare(aOther.mCompare)
  {
  }

  


  bool IsEmpty() const { return mElements.IsEmpty(); }

  


  size_type Length() const { return mElements.Length(); }

  






  const T& Top() const
  {
    NS_ABORT_IF_FALSE(!mElements.IsEmpty(), "Empty queue");
    return mElements[0];
  }

  




  bool Push(const T& aElement)
  {
    T* elem = mElements.AppendElement(aElement);
    if (!elem) {
      return false;  
    }

    
    size_type i = mElements.Length() - 1;
    while (i) {
      size_type parent = (size_type)((i - 1) / 2);
      if (mCompare.LessThan(mElements[parent], mElements[i])) {
        break;
      }
      Swap(i, parent);
      i = parent;
    }

    return true;
  }

  





  T Pop()
  {
    NS_ABORT_IF_FALSE(!mElements.IsEmpty(), "Empty queue");
    T pop = mElements[0];

    
    mElements[0] = mElements[mElements.Length() - 1];
    mElements.TruncateLength(mElements.Length() - 1);

    
    size_type i = 0;
    while (2 * i + 1 < mElements.Length()) {
      size_type swap = i;
      size_type l_child = 2 * i + 1;
      if (mCompare.LessThan(mElements[l_child], mElements[i])) {
        swap = l_child;
      }
      size_type r_child = l_child + 1;
      if (r_child < mElements.Length() &&
          mCompare.LessThan(mElements[r_child], mElements[swap])) {
        swap = r_child;
      }
      if (swap == i) {
        break;
      }
      Swap(i, swap);
      i = swap;
    }

    return pop;
  }

  


  void Clear() { mElements.Clear(); }

  






  const T* Elements() const { return mElements.Elements(); }

protected:
  


  void Swap(size_type aIndexA, size_type aIndexB)
  {
    T temp = mElements[aIndexA];
    mElements[aIndexA] = mElements[aIndexB];
    mElements[aIndexB] = temp;
  }

  nsTArray<T> mElements;
  Compare mCompare; 
};

#endif 
