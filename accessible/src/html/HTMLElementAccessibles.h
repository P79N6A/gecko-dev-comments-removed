




#ifndef mozilla_a11y_HTMLElementAccessibles_h__
#define mozilla_a11y_HTMLElementAccessibles_h__

#include "nsAutoPtr.h"
#include "nsBaseWidgetAccessible.h"

namespace mozilla {
namespace a11y {




class HTMLHRAccessible : public nsLeafAccessible
{
public:

  HTMLHRAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    nsLeafAccessible(aContent, aDoc) {};

  
  virtual a11y::role NativeRole();
};




class HTMLBRAccessible : public nsLeafAccessible
{
public:

  HTMLBRAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    nsLeafAccessible(aContent, aDoc) {};

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();
};




class HTMLLabelAccessible : public HyperTextAccessibleWrap
{
public:

  HTMLLabelAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    HyperTextAccessibleWrap(aContent, aDoc) {};

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult GetNameInternal(nsAString& aName);
  virtual a11y::role NativeRole();
};




class HTMLOutputAccessible : public HyperTextAccessibleWrap
{
public:

  HTMLOutputAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    HyperTextAccessibleWrap(aContent, aDoc) {};

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual a11y::role NativeRole();
  virtual nsresult GetAttributesInternal(nsIPersistentProperties* aAttributes);
  virtual Relation RelationByType(PRUint32 aType);
};

} 
} 

#endif
