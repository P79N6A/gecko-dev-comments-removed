




#ifndef __nsXULTreeGridAccessible_h__
#define __nsXULTreeGridAccessible_h__

#include "nsXULTreeAccessible.h"
#include "TableAccessible.h"
#include "xpcAccessibleTable.h"




class nsXULTreeGridAccessible : public nsXULTreeAccessible,
                                public xpcAccessibleTable,
                                public nsIAccessibleTable,
                                public mozilla::a11y::TableAccessible
{
public:
  nsXULTreeGridAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_OR_FORWARD_NSIACCESSIBLETABLE_WITH_XPCACCESSIBLETABLE

  
  virtual PRUint32 ColCount();
  virtual PRUint32 RowCount();
  virtual Accessible* CellAt(PRUint32 aRowIndex, PRUint32 aColumnIndex);
  virtual void UnselectRow(PRUint32 aRowIdx);

  
  virtual void Shutdown();

  
  virtual mozilla::a11y::TableAccessible* AsTable() { return this; }
  virtual mozilla::a11y::role NativeRole();

protected:

  
  virtual already_AddRefed<Accessible> CreateTreeItemAccessible(PRInt32 aRow);
};






class nsXULTreeGridRowAccessible : public nsXULTreeItemAccessibleBase
{
public:
  using Accessible::GetChildAt;

  nsXULTreeGridRowAccessible(nsIContent* aContent, DocAccessible* aDoc,
                             Accessible* aParent, nsITreeBoxObject* aTree,
                             nsITreeView* aTreeView, PRInt32 aRow);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULTreeGridRowAccessible,
                                           nsXULTreeItemAccessibleBase)

  
  virtual void Shutdown();

  
  virtual mozilla::a11y::role NativeRole();
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual Accessible* ChildAtPoint(PRInt32 aX, PRInt32 aY,
                                   EWhichChildAtPoint aWhichChild);

  virtual Accessible* GetChildAt(PRUint32 aIndex);
  virtual PRUint32 ChildCount() const;

  
  virtual Accessible* GetCellAccessible(nsITreeColumn* aColumn);
  virtual void RowInvalidated(PRInt32 aStartColIdx, PRInt32 aEndColIdx);

protected:

  
  virtual void CacheChildren();

  
  AccessibleHashtable mAccessibleCache;
};







#define NS_XULTREEGRIDCELLACCESSIBLE_IMPL_CID         \
{  /* 84588ad4-549c-4196-a932-4c5ca5de5dff */         \
  0x84588ad4,                                         \
  0x549c,                                             \
  0x4196,                                             \
  { 0xa9, 0x32, 0x4c, 0x5c, 0xa5, 0xde, 0x5d, 0xff }  \
}

class nsXULTreeGridCellAccessible : public mozilla::a11y::LeafAccessible,
                                    public nsIAccessibleTableCell
{
public:

  nsXULTreeGridCellAccessible(nsIContent* aContent, DocAccessible* aDoc,
                              nsXULTreeGridRowAccessible* aRowAcc,
                              nsITreeBoxObject* aTree, nsITreeView* aTreeView,
                              PRInt32 aRow, nsITreeColumn* aColumn);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsXULTreeGridCellAccessible,
                                           LeafAccessible)

  

  NS_IMETHOD GetBounds(PRInt32* aX, PRInt32* aY,
                       PRInt32* aWidth, PRInt32* aHeight);

  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual bool Init();
  virtual bool IsPrimaryForNode() const;

  
  virtual mozilla::a11y::ENameValueFlag Name(nsString& aName);
  virtual Accessible* FocusedChild();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties* aAttributes);
  virtual PRInt32 IndexInParent() const;
  virtual Relation RelationByType(PRUint32 aType);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeInteractiveState() const;

  
  virtual PRUint8 ActionCount();

  
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_XULTREEGRIDCELLACCESSIBLE_IMPL_CID)

  


  PRInt32 GetColumnIndex() const;

  



  void CellInvalidated();

protected:
  
  virtual Accessible* GetSiblingAtOffset(PRInt32 aOffset,
                                         nsresult* aError = nsnull) const;
  virtual void DispatchClickEvent(nsIContent* aContent, PRUint32 aActionIndex);

  

  


  bool IsEditable() const;

  enum { eAction_Click = 0 };

  nsCOMPtr<nsITreeBoxObject> mTree;
  nsITreeView* mTreeView;

  PRInt32 mRow;
  nsCOMPtr<nsITreeColumn> mColumn;

  nsString mCachedTextEquiv;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXULTreeGridCellAccessible,
                              NS_XULTREEGRIDCELLACCESSIBLE_IMPL_CID)

#endif
