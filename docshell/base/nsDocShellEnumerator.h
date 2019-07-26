







#include "nsIEnumerator.h"

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIWeakReferenceUtils.h"

class nsIDocShellTreeItem;


















class nsDocShellEnumerator : public nsISimpleEnumerator
{
protected:

  enum {
    enumerateForwards,
    enumerateBackwards
  };
  
public:

                              nsDocShellEnumerator(int32_t inEnumerationDirection);
  virtual                     ~nsDocShellEnumerator();

  
  NS_DECL_ISUPPORTS
  
  
  NS_DECL_NSISIMPLEENUMERATOR
  
public:

  nsresult                    GetEnumerationRootItem(nsIDocShellTreeItem * *aEnumerationRootItem);
  nsresult                    SetEnumerationRootItem(nsIDocShellTreeItem * aEnumerationRootItem);
  
  nsresult                    GetEnumDocShellType(int32_t *aEnumerationItemType);
  nsresult                    SetEnumDocShellType(int32_t aEnumerationItemType);
    
  nsresult                    First();

protected:

  nsresult                    EnsureDocShellArray();
  nsresult                    ClearState();
  
  nsresult                    BuildDocShellArray(nsTArray<nsWeakPtr>& inItemArray);
  virtual nsresult            BuildArrayRecursive(nsIDocShellTreeItem* inItem, nsTArray<nsWeakPtr>& inItemArray) = 0;
    
protected:

  nsWeakPtr                   mRootItem;      
  
  nsTArray<nsWeakPtr>         mItemArray;     
  uint32_t                    mCurIndex;
  
  int32_t                     mDocShellType;  
  bool                        mArrayValid;    

  const int8_t                mEnumerationDirection;
};


class nsDocShellForwardsEnumerator : public nsDocShellEnumerator
{
public:

                              nsDocShellForwardsEnumerator()
                              : nsDocShellEnumerator(enumerateForwards)
                              {                              
                              }

protected:

  virtual nsresult            BuildArrayRecursive(nsIDocShellTreeItem* inItem, nsTArray<nsWeakPtr>& inItemArray);

};

class nsDocShellBackwardsEnumerator : public nsDocShellEnumerator
{
public:

                              nsDocShellBackwardsEnumerator()
                              : nsDocShellEnumerator(enumerateBackwards)
                              {                              
                              }
protected:

  virtual nsresult            BuildArrayRecursive(nsIDocShellTreeItem* inItem, nsTArray<nsWeakPtr>& inItemArray);

};
