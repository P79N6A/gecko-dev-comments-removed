





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
    HyperTextAccessibleWrap(aContent, aDoc) { mGenericTypes |= eList; }
  virtual ~HTMLListAccessible() { }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
};





class HTMLLIAccessible : public HyperTextAccessibleWrap
{
public:
  HTMLLIAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLLIAccessible() { }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual void Shutdown();

  
  NS_IMETHOD GetBounds(int32_t* aX, int32_t* aY,
                       int32_t* aWidth, int32_t* aHeight);

  
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();

  
  void UpdateBullet(bool aHasBullet);

protected:
  
  virtual void CacheChildren();

private:
  nsRefPtr<HTMLListBulletAccessible> mBullet;
};





class HTMLListBulletAccessible : public LeafAccessible
{
public:
  HTMLListBulletAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLListBulletAccessible() { }

  
  virtual nsIFrame* GetFrame() const;

  
  virtual ENameValueFlag Name(nsString& aName);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual void AppendTextTo(nsAString& aText, uint32_t aStartOffset = 0,
                            uint32_t aLength = UINT32_MAX);

  

  


  bool IsInside() const;
};


inline HTMLLIAccessible*
Accessible::AsHTMLListItem()
{
  return IsHTMLListItem() ? static_cast<HTMLLIAccessible*>(this) : nullptr;
}

} 
} 

#endif
