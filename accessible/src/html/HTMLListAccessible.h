





#ifndef mozilla_a11y_HTMLListAccessible_h__
#define mozilla_a11y_HTMLListAccessible_h__

#include "BaseAccessibles.h"
#include "HyperTextAccessibleWrap.h"

namespace mozilla {
namespace a11y {

class HTMLListBulletAccessible;




class HTMLListAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLListAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    HyperTextAccessibleWrap(aContent, aDoc) { }
  virtual ~HTMLListAccessible() { }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();
};





class HTMLLIAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLLIAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLLIAccessible() { }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Shutdown();

  
  NS_IMETHOD GetBounds(PRInt32* aX, PRInt32* aY,
                       PRInt32* aWidth, PRInt32* aHeight);

  
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  void UpdateBullet(bool aHasBullet);

protected:
  
  virtual void CacheChildren();

private:
  nsRefPtr<HTMLListBulletAccessible> mBullet;
};





class HTMLListBulletAccessible : public LeafAccessible
{
public:
  HTMLListBulletAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    LeafAccessible(aContent, aDoc) { }
  virtual ~HTMLListBulletAccessible() { }

  
  virtual nsIFrame* GetFrame() const;
  virtual bool IsPrimaryForNode() const;

  
  virtual ENameValueFlag Name(nsString& aName);
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual void AppendTextTo(nsAString& aText, PRUint32 aStartOffset = 0,
                            PRUint32 aLength = PR_UINT32_MAX);

  

  


  bool IsInside() const;
};

} 
} 


inline mozilla::a11y::HTMLLIAccessible*
Accessible::AsHTMLListItem()
{
  return mFlags & eHTMLListItemAccessible ?
    static_cast<mozilla::a11y::HTMLLIAccessible*>(this) : nullptr;
}

#endif
