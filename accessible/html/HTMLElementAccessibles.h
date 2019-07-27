




#ifndef mozilla_a11y_HTMLElementAccessibles_h__
#define mozilla_a11y_HTMLElementAccessibles_h__

#include "BaseAccessibles.h"
#include "nsAutoPtr.h"

namespace mozilla {
namespace a11y {




class HTMLHRAccessible : public LeafAccessible
{
public:

  HTMLHRAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    LeafAccessible(aContent, aDoc) {}

  
  virtual a11y::role NativeRole() override;
};




class HTMLBRAccessible : public LeafAccessible
{
public:
  HTMLBRAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    LeafAccessible(aContent, aDoc)
  {
    mType = eHTMLBRType;
  }

  
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) override;
};




class HTMLLabelAccessible : public HyperTextAccessibleWrap
{
public:

  HTMLLabelAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    HyperTextAccessibleWrap(aContent, aDoc) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual Relation RelationByType(RelationType aType) override;

protected:
  virtual ~HTMLLabelAccessible() {}
  virtual ENameValueFlag NativeName(nsString& aName) override;
};




class HTMLOutputAccessible : public HyperTextAccessibleWrap
{
public:

  HTMLOutputAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    HyperTextAccessibleWrap(aContent, aDoc) {}

  NS_DECL_ISUPPORTS_INHERITED

  
  virtual Relation RelationByType(RelationType aType) override;

protected:
  virtual ~HTMLOutputAccessible() {}
};

} 
} 

#endif
