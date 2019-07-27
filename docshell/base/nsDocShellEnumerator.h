





#ifndef nsDocShellEnumerator_h___
#define nsDocShellEnumerator_h___

#include "nsISimpleEnumerator.h"
#include "nsTArray.h"
#include "nsIWeakReferenceUtils.h"

class nsIDocShellTreeItem;

















class nsDocShellEnumerator : public nsISimpleEnumerator
{
protected:
  enum
  {
    enumerateForwards,
    enumerateBackwards
  };

  virtual ~nsDocShellEnumerator();

public:
  explicit nsDocShellEnumerator(int32_t aEnumerationDirection);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSISIMPLEENUMERATOR

public:
  nsresult GetEnumerationRootItem(nsIDocShellTreeItem** aEnumerationRootItem);
  nsresult SetEnumerationRootItem(nsIDocShellTreeItem* aEnumerationRootItem);

  nsresult GetEnumDocShellType(int32_t* aEnumerationItemType);
  nsresult SetEnumDocShellType(int32_t aEnumerationItemType);

  nsresult First();

protected:
  nsresult EnsureDocShellArray();
  nsresult ClearState();

  nsresult BuildDocShellArray(nsTArray<nsWeakPtr>& aItemArray);
  virtual nsresult BuildArrayRecursive(nsIDocShellTreeItem* aItem,
                                       nsTArray<nsWeakPtr>& aItemArray) = 0;

protected:
  nsWeakPtr mRootItem;  

  nsTArray<nsWeakPtr> mItemArray;  
  uint32_t mCurIndex;

  int32_t mDocShellType;  
  bool mArrayValid;  

  const int8_t mEnumerationDirection;
};

class nsDocShellForwardsEnumerator : public nsDocShellEnumerator
{
public:
  nsDocShellForwardsEnumerator()
    : nsDocShellEnumerator(enumerateForwards)
  {
  }

protected:
  virtual nsresult BuildArrayRecursive(nsIDocShellTreeItem* aItem,
                                       nsTArray<nsWeakPtr>& aItemArray);

};

class nsDocShellBackwardsEnumerator : public nsDocShellEnumerator
{
public:
  nsDocShellBackwardsEnumerator()
    : nsDocShellEnumerator(enumerateBackwards)
  {
  }

protected:
  virtual nsresult BuildArrayRecursive(nsIDocShellTreeItem* aItem,
                                       nsTArray<nsWeakPtr>& aItemArray);
};

#endif
