




#ifndef mozilla_a11y_HTMLSelectAccessible_h__
#define mozilla_a11y_HTMLSelectAccessible_h__

#include "HTMLFormControlAccessible.h"

class nsIMutableArray;

namespace mozilla {
namespace a11y {



















class HTMLSelectListAccessible : public AccessibleWrap
{
public:

  HTMLSelectListAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLSelectListAccessible() {}

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;

  
  virtual bool SelectAll() MOZ_OVERRIDE;
  virtual bool UnselectAll() MOZ_OVERRIDE;

  
  virtual bool IsWidget() const MOZ_OVERRIDE;
  virtual bool IsActiveWidget() const MOZ_OVERRIDE;
  virtual bool AreItemsOperable() const MOZ_OVERRIDE;
  virtual Accessible* CurrentItem() MOZ_OVERRIDE;
  virtual void SetCurrentItem(Accessible* aItem) MOZ_OVERRIDE;

protected:

  
  virtual void CacheChildren() MOZ_OVERRIDE;
};




class HTMLSelectOptionAccessible : public HyperTextAccessibleWrap
{
public:
  enum { eAction_Select = 0 };

  HTMLSelectOptionAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLSelectOptionAccessible() {}

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual uint64_t NativeInteractiveState() const MOZ_OVERRIDE;

  virtual int32_t GetLevelInternal() MOZ_OVERRIDE;
  virtual nsRect RelativeBounds(nsIFrame** aBoundingFrame) const MOZ_OVERRIDE;
  virtual void SetSelected(bool aSelect) MOZ_OVERRIDE;

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t aIndex) MOZ_OVERRIDE;

  
  virtual Accessible* ContainerWidget() const MOZ_OVERRIDE;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) MOZ_OVERRIDE;

private:

  


  Accessible* GetSelect() const
  {
    Accessible* parent = mParent;
    if (parent && parent->IsHTMLOptGroup())
      parent = parent->Parent();

    if (parent && parent->IsListControl()) {
      Accessible* combobox = parent->Parent();
      return combobox && combobox->IsCombobox() ? combobox : mParent.get();
    }

    return nullptr;
  }

  


  Accessible* GetCombobox() const
  {
    Accessible* parent = mParent;
    if (parent && parent->IsHTMLOptGroup())
      parent = parent->Parent();

    if (parent && parent->IsListControl()) {
      Accessible* combobox = parent->Parent();
      return combobox && combobox->IsCombobox() ? combobox : nullptr;
    }

    return nullptr;
  }
};




class HTMLSelectOptGroupAccessible : public HTMLSelectOptionAccessible
{
public:

  HTMLSelectOptGroupAccessible(nsIContent* aContent, DocAccessible* aDoc) :
    HTMLSelectOptionAccessible(aContent, aDoc)
    { mType = eHTMLOptGroupType; }
  virtual ~HTMLSelectOptGroupAccessible() {}

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeInteractiveState() const MOZ_OVERRIDE;

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t aIndex) MOZ_OVERRIDE;
};





class HTMLComboboxListAccessible;




class HTMLComboboxAccessible MOZ_FINAL : public AccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  HTMLComboboxAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLComboboxAccessible() {}

  
  virtual void Shutdown() MOZ_OVERRIDE;
  virtual void Description(nsString& aDescription) MOZ_OVERRIDE;
  virtual void Value(nsString& aValue) MOZ_OVERRIDE;
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual void InvalidateChildren() MOZ_OVERRIDE;
  virtual bool RemoveChild(Accessible* aChild) MOZ_OVERRIDE;

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t aIndex) MOZ_OVERRIDE;

  
  virtual bool IsWidget() const MOZ_OVERRIDE;
  virtual bool IsActiveWidget() const MOZ_OVERRIDE;
  virtual bool AreItemsOperable() const MOZ_OVERRIDE;
  virtual Accessible* CurrentItem() MOZ_OVERRIDE;
  virtual void SetCurrentItem(Accessible* aItem) MOZ_OVERRIDE;

protected:
  
  virtual void CacheChildren() MOZ_OVERRIDE;

  


  Accessible* SelectedOption() const;

private:
  nsRefPtr<HTMLComboboxListAccessible> mListAccessible;
};






class HTMLComboboxListAccessible : public HTMLSelectListAccessible
{
public:

  HTMLComboboxListAccessible(Accessible* aParent, nsIContent* aContent,
                             DocAccessible* aDoc);
  virtual ~HTMLComboboxListAccessible() {}

  
  virtual nsIFrame* GetFrame() const MOZ_OVERRIDE;
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual nsRect RelativeBounds(nsIFrame** aBoundingFrame) const MOZ_OVERRIDE;

  
  virtual bool IsActiveWidget() const MOZ_OVERRIDE;
  virtual bool AreItemsOperable() const MOZ_OVERRIDE;
};

} 
} 

#endif
