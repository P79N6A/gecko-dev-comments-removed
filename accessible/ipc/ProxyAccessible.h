





#ifndef mozilla_a11y_ProxyAccessible_h
#define mozilla_a11y_ProxyAccessible_h

#include "mozilla/a11y/Role.h"
#include "nsString.h"
#include "nsTArray.h"

namespace mozilla {
namespace a11y {

class DocAccessibleParent;

class ProxyAccessible
{
public:

  ProxyAccessible(uint64_t aID, ProxyAccessible* aParent,
                  DocAccessibleParent* aDoc, role aRole,
                  const nsString& aName) :
     mParent(aParent), mDoc(aDoc), mID(aID), mRole(aRole), mOuterDoc(false), mName(aName)
  {
    MOZ_COUNT_CTOR(ProxyAccessible);
  }
  ~ProxyAccessible() { MOZ_COUNT_DTOR(ProxyAccessible); }

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

  


  uintptr_t GetWrapper() const { return mWrapper; }
  void SetWrapper(uintptr_t aWrapper) { mWrapper = aWrapper; }

  


  uint64_t ID() const { return mID; }

protected:
  ProxyAccessible() :
    mParent(nullptr), mDoc(nullptr), mWrapper(0), mID(0)
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
  nsString mName;
};

}
}

#endif
