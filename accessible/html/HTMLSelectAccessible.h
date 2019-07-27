




#ifndef mozilla_a11y_HTMLSelectAccessible_h__
#define mozilla_a11y_HTMLSelectAccessible_h__

#include "HTMLFormControlAccessible.h"

namespace mozilla {
namespace a11y {



















class HTMLSelectListAccessible : public AccessibleWrap
{
public:

  HTMLSelectListAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLSelectListAccessible() {}

  
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;

  
  virtual bool SelectAll() override;
  virtual bool UnselectAll() override;

  
  virtual bool IsWidget() const override;
  virtual bool IsActiveWidget() const override;
  virtual bool AreItemsOperable() const override;
  virtual Accessible* CurrentItem() override;
  virtual void SetCurrentItem(Accessible* aItem) override;

protected:

  
  virtual void CacheChildren() override;
};




class HTMLSelectOptionAccessible : public HyperTextAccessibleWrap
{
public:
  enum { eAction_Select = 0 };

  HTMLSelectOptionAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLSelectOptionAccessible() {}

  
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
  virtual uint64_t NativeInteractiveState() const override;

  virtual int32_t GetLevelInternal() override;
  virtual nsRect RelativeBounds(nsIFrame** aBoundingFrame) const override;
  virtual void SetSelected(bool aSelect) override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t aIndex) override;

  
  virtual Accessible* ContainerWidget() const override;

protected:
  
  virtual ENameValueFlag NativeName(nsString& aName) override;

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

  
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeInteractiveState() const override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t aIndex) override;
};





class HTMLComboboxListAccessible;




class HTMLComboboxAccessible final : public AccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  HTMLComboboxAccessible(nsIContent* aContent, DocAccessible* aDoc);
  virtual ~HTMLComboboxAccessible() {}

  
  virtual void Shutdown() override;
  virtual void Description(nsString& aDescription) override;
  virtual void Value(nsString& aValue) override;
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
  virtual void InvalidateChildren() override;
  virtual bool RemoveChild(Accessible* aChild) override;

  
  virtual uint8_t ActionCount() override;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) override;
  virtual bool DoAction(uint8_t aIndex) override;

  
  virtual bool IsWidget() const override;
  virtual bool IsActiveWidget() const override;
  virtual bool AreItemsOperable() const override;
  virtual Accessible* CurrentItem() override;
  virtual void SetCurrentItem(Accessible* aItem) override;

protected:
  
  virtual void CacheChildren() override;

  


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

  
  virtual nsIFrame* GetFrame() const override;
  virtual a11y::role NativeRole() override;
  virtual uint64_t NativeState() override;
  virtual nsRect RelativeBounds(nsIFrame** aBoundingFrame) const override;

  
  virtual bool IsActiveWidget() const override;
  virtual bool AreItemsOperable() const override;
};

} 
} 

#endif
