





#ifndef mozilla_a11y_DocAccessibleParent_h
#define mozilla_a11y_DocAccessibleParent_h

#include "nsAccessibilityService.h"
#include "ProxyAccessible.h"
#include "mozilla/a11y/PDocAccessibleParent.h"
#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace a11y {





class DocAccessibleParent : public ProxyAccessible,
    public PDocAccessibleParent
{
public:
  DocAccessibleParent() :
    ProxyAccessible(this), mParentDoc(nullptr), mShutdown(false)
  { MOZ_COUNT_CTOR_INHERITED(DocAccessibleParent, ProxyAccessible); }
  ~DocAccessibleParent()
  {
    MOZ_COUNT_DTOR_INHERITED(DocAccessibleParent, ProxyAccessible);
    MOZ_ASSERT(mChildDocs.Length() == 0);
    MOZ_ASSERT(!mParentDoc);
  }

  



  virtual bool RecvEvent(const uint64_t& aID, const uint32_t& aType)
    MOZ_OVERRIDE;

  virtual bool RecvShowEvent(const ShowEventData& aData) MOZ_OVERRIDE;
  virtual bool RecvHideEvent(const uint64_t& aRootID) MOZ_OVERRIDE;

  void Destroy();
  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE
  {
    if (!mShutdown)
      Destroy();
  }

  



  DocAccessibleParent* Parent() const { return mParentDoc; }

  



  bool AddChildDoc(DocAccessibleParent* aChildDoc, uint64_t aParentID);

  



  void RemoveChildDoc(DocAccessibleParent* aChildDoc)
  {
    aChildDoc->mParent->SetChildDoc(nullptr);
    mChildDocs.RemoveElement(aChildDoc);
    aChildDoc->mParentDoc = nullptr;
    MOZ_ASSERT(aChildDoc->mChildDocs.Length() == 0);
  }

  void RemoveAccessible(ProxyAccessible* aAccessible)
  {
    MOZ_ASSERT(mAccessibles.GetEntry(aAccessible->ID()));
    mAccessibles.RemoveEntry(aAccessible->ID());
  }

  


  ProxyAccessible* GetAccessible(uintptr_t aID) const
  {
    ProxyEntry* e = mAccessibles.GetEntry(aID);
    return e ? e->mProxy : nullptr;
  }

private:

  class ProxyEntry : public PLDHashEntryHdr
  {
  public:
    explicit ProxyEntry(const void*) : mProxy(nullptr) {}
    ProxyEntry(ProxyEntry&& aOther) :
      mProxy(aOther.mProxy) { aOther.mProxy = nullptr; }
    ~ProxyEntry() { delete mProxy; }

    typedef uint64_t KeyType;
    typedef const void* KeyTypePointer;

    bool KeyEquals(const void* aKey) const
    { return mProxy->ID() == (uint64_t)aKey; }

    static const void* KeyToPointer(uint64_t aKey) { return (void*)aKey; }

    static PLDHashNumber HashKey(const void* aKey) { return (uint64_t)aKey; }

    enum { ALLOW_MEMMOVE = true };

    ProxyAccessible* mProxy;
  };

  uint32_t AddSubtree(ProxyAccessible* aParent,
                      const nsTArray<AccessibleData>& aNewTree, uint32_t aIdx,
                      uint32_t aIdxInParent);
  static PLDHashOperator ShutdownAccessibles(ProxyEntry* entry, void* unused);

  nsTArray<DocAccessibleParent*> mChildDocs;
  DocAccessibleParent* mParentDoc;

  



  nsTHashtable<ProxyEntry> mAccessibles;
  bool mShutdown;
};

}
}

#endif
