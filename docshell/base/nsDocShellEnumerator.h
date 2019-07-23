








































#include "nsIEnumerator.h"

#include "nsCOMPtr.h"
#include "nsTArray.h"

class nsIDocShellTreeItem;


















class nsDocShellEnumerator : public nsISimpleEnumerator
{
protected:

  enum {
    enumerateForwards,
    enumerateBackwards
  };
  
public:

                              nsDocShellEnumerator(PRInt32 inEnumerationDirection);
  virtual                     ~nsDocShellEnumerator();

  
  NS_DECL_ISUPPORTS
  
  
  NS_DECL_NSISIMPLEENUMERATOR
  
public:

  nsresult                    GetEnumerationRootItem(nsIDocShellTreeItem * *aEnumerationRootItem);
  nsresult                    SetEnumerationRootItem(nsIDocShellTreeItem * aEnumerationRootItem);
  
  nsresult                    GetEnumDocShellType(PRInt32 *aEnumerationItemType);
  nsresult                    SetEnumDocShellType(PRInt32 aEnumerationItemType);
    
  nsresult                    First();

protected:

  nsresult                    EnsureDocShellArray();
  nsresult                    ClearState();
  
  nsresult                    BuildDocShellArray(nsTArray<nsIDocShellTreeItem*>& inItemArray);
  virtual nsresult            BuildArrayRecursive(nsIDocShellTreeItem* inItem, nsTArray<nsIDocShellTreeItem*>& inItemArray) = 0;
    
protected:

  nsIDocShellTreeItem*        mRootItem;      
  
  nsTArray<nsIDocShellTreeItem*> mItemArray; 
  PRUint32                    mCurIndex;
  
  PRInt32                     mDocShellType;  
  PRPackedBool                mArrayValid;    

  const PRInt8                mEnumerationDirection;
};


class nsDocShellForwardsEnumerator : public nsDocShellEnumerator
{
public:

                              nsDocShellForwardsEnumerator()
                              : nsDocShellEnumerator(enumerateForwards)
                              {                              
                              }

protected:

  virtual nsresult            BuildArrayRecursive(nsIDocShellTreeItem* inItem, nsTArray<nsIDocShellTreeItem*>& inItemArray);

};

class nsDocShellBackwardsEnumerator : public nsDocShellEnumerator
{
public:

                              nsDocShellBackwardsEnumerator()
                              : nsDocShellEnumerator(enumerateBackwards)
                              {                              
                              }
protected:

  virtual nsresult            BuildArrayRecursive(nsIDocShellTreeItem* inItem, nsTArray<nsIDocShellTreeItem*>& inItemArray);

};
