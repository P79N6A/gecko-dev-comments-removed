




#ifndef mozilla_a11y_XULTabAccessible_h__
#define mozilla_a11y_XULTabAccessible_h__


#include "XULMenuAccessible.h"
#include "XULSelectControlAccessible.h"

namespace mozilla {
namespace a11y {




class XULTabAccessible : public AccessibleWrap
{
public:
  enum { eAction_Switch = 0 };

  XULTabAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);
  NS_IMETHOD DoAction(uint8_t index);

  
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual uint64_t NativeInteractiveState() const;
  virtual Relation RelationByType(uint32_t aType);

  
  virtual uint8_t ActionCount();
};





class XULTabsAccessible : public XULSelectControlAccessible
{
public:
  XULTabsAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Value(nsString& aValue);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();

  
  virtual uint8_t ActionCount();
};





class XULDeckAccessible : public AccessibleWrap
{
public:
  XULDeckAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    AccessibleWrap(aContent, aDoc)
    { mFlags |= eXULDeckAccessible; }

  
  virtual a11y::role NativeRole();
};












class XULTabpanelAccessible : public AccessibleWrap
{
public:
  XULTabpanelAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole();
  virtual Relation RelationByType(uint32_t aType);
};

} 
} 

#endif

