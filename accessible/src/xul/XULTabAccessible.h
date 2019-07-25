




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

  
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);
  NS_IMETHOD DoAction(PRUint8 index);

  
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeInteractiveState() const;
  virtual Relation RelationByType(PRUint32 aType);

  
  virtual PRUint8 ActionCount();
};





class XULTabsAccessible : public XULSelectControlAccessible
{
public:
  XULTabsAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual void Value(nsString& aValue);
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();

  
  virtual PRUint8 ActionCount();
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
  virtual Relation RelationByType(PRUint32 aType);
};

} 
} 

#endif

