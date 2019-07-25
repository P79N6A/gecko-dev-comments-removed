






































#ifndef _nsHTMLTextAccessible_H_
#define _nsHTMLTextAccessible_H_

#include "nsTextAccessibleWrap.h"
#include "nsAutoPtr.h"
#include "nsBaseWidgetAccessible.h"




class nsHTMLTextAccessible : public nsTextAccessibleWrap
{
public:
  nsHTMLTextAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetName(nsAString& aName);

  
  virtual nsresult GetAttributesInternal(nsIPersistentProperties *aAttributes);
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
};




class nsHTMLHRAccessible : public nsLeafAccessible
{
public:
  nsHTMLHRAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual PRUint32 NativeRole();
};




class nsHTMLBRAccessible : public nsLeafAccessible
{
public:
  nsHTMLBRAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
};




class nsHTMLLabelAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLLabelAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual PRUint32 NativeRole();
};




class nsHTMLOutputAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLOutputAccessible(nsIContent* aContent, nsIWeakReference* aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetRelationByType(PRUint32 aRelationType,
                               nsIAccessibleRelation** aRelation);

  
  virtual PRUint32 NativeRole();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties* aAttributes);
};




class nsHTMLListBulletAccessible : public nsLeafAccessible
{
public:
  nsHTMLListBulletAccessible(nsIContent* aContent, nsIWeakReference* aShell);

  
  NS_IMETHOD GetName(nsAString& aName);

  
  virtual bool IsPrimaryForNode() const;

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
  virtual void AppendTextTo(nsAString& aText, PRUint32 aStartOffset = 0,
                            PRUint32 aLength = PR_UINT32_MAX);
};




class nsHTMLListAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLListAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
};




class nsHTMLLIAccessible : public nsHyperTextAccessibleWrap
{
public:
  nsHTMLLIAccessible(nsIContent* aContent, nsIWeakReference* aShell);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Shutdown();

  
  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();

  
  void UpdateBullet(bool aHasBullet);

protected:
  
  virtual void CacheChildren();

private:
  nsRefPtr<nsHTMLListBulletAccessible> mBullet;
};

inline nsHTMLLIAccessible*
nsAccessible::AsHTMLListItem()
{
  return mFlags & eHTMLListItemAccessible ?
    static_cast<nsHTMLLIAccessible*>(this) : nsnull;
}

#endif
