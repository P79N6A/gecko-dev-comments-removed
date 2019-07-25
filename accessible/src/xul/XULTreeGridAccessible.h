




#ifndef mozilla_a11y_XULTreeGridAccessible_h__
#define mozilla_a11y_XULTreeGridAccessible_h__

#include "XULTreeAccessible.h"
#include "TableAccessible.h"
#include "xpcAccessibleTable.h"

namespace mozilla {
namespace a11y {




class XULTreeGridAccessible : public XULTreeAccessible,
                              public xpcAccessibleTable,
                              public nsIAccessibleTable,
                              public TableAccessible
{
public:
  XULTreeGridAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_OR_FORWARD_NSIACCESSIBLETABLE_WITH_XPCACCESSIBLETABLE

  
  virtual PRUint32 ColCount();
  virtual PRUint32 RowCount();
  virtual Accessible* CellAt(PRUint32 aRowIndex, PRUint32 aColumnIndex);
  virtual void ColDescription(PRUint32 aColIdx, nsString& aDescription);
  virtual bool IsColSelected(PRUint32 aColIdx);
  virtual bool IsRowSelected(PRUint32 aRowIdx);
  virtual bool IsCellSelected(PRUint32 aRowIdx, PRUint32 aColIdx);
  virtual PRUint32 SelectedCellCount();
  virtual PRUint32 SelectedColCount();
  virtual PRUint32 SelectedRowCount();
  virtual void SelectRow(PRUint32 aRowIdx);
  virtual void UnselectRow(PRUint32 aRowIdx);

  
  virtual void Shutdown();

  
  virtual TableAccessible* AsTable() { return this; }
  virtual a11y::role NativeRole();

protected:

  
  virtual already_AddRefed<Accessible> CreateTreeItemAccessible(PRInt32 aRow);
};






class XULTreeGridRowAccessible : public XULTreeItemAccessibleBase
{
public:
  using Accessible::GetChildAt;

  XULTreeGridRowAccessible(nsIContent* aContent, DocAccessible* aDoc,
                           Accessible* aParent, nsITreeBoxObject* aTree,
                           nsITreeView* aTreeView, PRInt32 aRow);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULTreeGridRowAccessible,
                                           XULTreeItemAccessibleBase)

  
  virtual void Shutdown();

  
  virtual a11y::role NativeRole();
  virtual ENameValueFlag Name(nsString& aName);
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







#define XULTREEGRIDCELLACCESSIBLE_IMPL_CID            \
{  /* 84588ad4-549c-4196-a932-4c5ca5de5dff */         \
  0x84588ad4,                                         \
  0x549c,                                             \
  0x4196,                                             \
  { 0xa9, 0x32, 0x4c, 0x5c, 0xa5, 0xde, 0x5d, 0xff }  \
}

class XULTreeGridCellAccessible : public LeafAccessible,
                                  public nsIAccessibleTableCell
{
public:

  XULTreeGridCellAccessible(nsIContent* aContent, DocAccessible* aDoc,
                            XULTreeGridRowAccessible* aRowAcc,
                            nsITreeBoxObject* aTree, nsITreeView* aTreeView,
                            PRInt32 aRow, nsITreeColumn* aColumn);

  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(XULTreeGridCellAccessible,
                                           LeafAccessible)

  

  NS_IMETHOD GetBounds(PRInt32* aX, PRInt32* aY,
                       PRInt32* aWidth, PRInt32* aHeight);

  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 aIndex);

  
  NS_DECL_NSIACCESSIBLETABLECELL

  
  virtual bool Init();
  virtual bool IsPrimaryForNode() const;

  
  virtual ENameValueFlag Name(nsString& aName);
  virtual Accessible* FocusedChild();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties* aAttributes);
  virtual PRInt32 IndexInParent() const;
  virtual Relation RelationByType(PRUint32 aType);
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeInteractiveState() const;

  
  virtual PRUint8 ActionCount();

  
  NS_DECLARE_STATIC_IID_ACCESSOR(XULTREEGRIDCELLACCESSIBLE_IMPL_CID)

  


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

NS_DEFINE_STATIC_IID_ACCESSOR(XULTreeGridCellAccessible,
                              XULTREEGRIDCELLACCESSIBLE_IMPL_CID)

} 
} 

#endif
