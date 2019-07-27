





#ifndef mozilla_a11y_ProxyAccessible_h
#define mozilla_a11y_ProxyAccessible_h

#include "mozilla/a11y/Role.h"
#include "nsString.h"
#include "nsTArray.h"

namespace mozilla {
namespace a11y {

class Attribute;
class DocAccessibleParent;

class ProxyAccessible
{
public:

  ProxyAccessible(uint64_t aID, ProxyAccessible* aParent,
                  DocAccessibleParent* aDoc, role aRole) :
     mParent(aParent), mDoc(aDoc), mWrapper(0), mID(aID), mRole(aRole),
     mOuterDoc(false)
  {
    MOZ_COUNT_CTOR(ProxyAccessible);
  }
  ~ProxyAccessible()
  {
    MOZ_COUNT_DTOR(ProxyAccessible);
    MOZ_ASSERT(!mWrapper);
  }

  void AddChildAt(uint32_t aIdx, ProxyAccessible* aChild)
  { mChildren.InsertElementAt(aIdx, aChild); }

  uint32_t ChildrenCount() const { return mChildren.Length(); }

  void Shutdown();

  void SetChildDoc(DocAccessibleParent*);

  


  void RemoveChild(ProxyAccessible* aChild)
    { mChildren.RemoveElement(aChild); }

  


  ProxyAccessible* Parent() const { return mParent; }

  


  role Role() const { return mRole; }

  


  uint64_t State() const;

  


  void Name(nsString& aName) const;

  


  void Value(nsString& aValue) const;

  


  void Description(nsString& aDesc) const;

  


  void Attributes(nsTArray<Attribute> *aAttrs) const;

  


  void TextSubstring(int32_t aStartOffset, int32_t aEndOfset,
                     nsString& aText) const;

  


  uintptr_t GetWrapper() const { return mWrapper; }
  void SetWrapper(uintptr_t aWrapper) { mWrapper = aWrapper; }

  


  uint64_t ID() const { return mID; }

protected:
  explicit ProxyAccessible(DocAccessibleParent* aThisAsDoc) :
    mParent(nullptr), mDoc(aThisAsDoc), mWrapper(0), mID(0),
    mRole(roles::DOCUMENT)
  { MOZ_COUNT_CTOR(ProxyAccessible); }

protected:
  ProxyAccessible* mParent;

private:
  nsTArray<ProxyAccessible*> mChildren;
  DocAccessibleParent* mDoc;
  uintptr_t mWrapper;
  uint64_t mID;
  role mRole : 31;
  bool mOuterDoc : 1;
};

enum Interfaces
{
  HYPERTEXT = 1
};

}
}

#endif
