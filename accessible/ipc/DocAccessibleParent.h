





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
    ProxyAccessible(this), mParentDoc(nullptr),
    mTopLevel(false), mShutdown(false)
  { MOZ_COUNT_CTOR_INHERITED(DocAccessibleParent, ProxyAccessible); }
  ~DocAccessibleParent()
  {
    MOZ_COUNT_DTOR_INHERITED(DocAccessibleParent, ProxyAccessible);
    MOZ_ASSERT(mChildDocs.Length() == 0);
    MOZ_ASSERT(!ParentDoc());
  }

  void SetTopLevel() { mTopLevel = true; }
  bool IsTopLevel() const { return mTopLevel; }

  



  virtual bool RecvEvent(const uint64_t& aID, const uint32_t& aType)
    override;

  virtual bool RecvShowEvent(const ShowEventData& aData) override;
  virtual bool RecvHideEvent(const uint64_t& aRootID) override;
  virtual bool RecvStateChangeEvent(const uint64_t& aID,
                                    const uint64_t& aState,
                                    const bool& aEnabled) override final;

  virtual bool RecvCaretMoveEvent(const uint64_t& aID, const int32_t& aOffset)
    override final;

  virtual bool RecvBindChildDoc(PDocAccessibleParent* aChildDoc, const uint64_t& aID) override;
  void Unbind()
  {
    mParent = nullptr;
    ParentDoc()->mChildDocs.RemoveElement(this);
    mParentDoc = nullptr;
  }

  virtual bool RecvShutdown() override;
  void Destroy();
  virtual void ActorDestroy(ActorDestroyReason aWhy) override
  {
    if (!mShutdown)
      Destroy();
  }

  



  DocAccessibleParent* ParentDoc() const { return mParentDoc; }

  



  bool AddChildDoc(DocAccessibleParent* aChildDoc, uint64_t aParentID,
                   bool aCreating = true);

  



  void RemoveChildDoc(DocAccessibleParent* aChildDoc)
  {
    aChildDoc->Parent()->SetChildDoc(nullptr);
    mChildDocs.RemoveElement(aChildDoc);
    aChildDoc->mParentDoc = nullptr;
    MOZ_ASSERT(aChildDoc->mChildDocs.Length() == 0);
  }

  void RemoveAccessible(ProxyAccessible* aAccessible)
  {
    MOZ_ASSERT(mAccessibles.GetEntry(aAccessible->ID()));
    mAccessibles.RemoveEntry(aAccessible->ID());
  }

  


  ProxyAccessible* GetAccessible(uintptr_t aID)
  {
    if (!aID)
      return this;

    ProxyEntry* e = mAccessibles.GetEntry(aID);
    return e ? e->mProxy : nullptr;
  }

  const ProxyAccessible* GetAccessible(uintptr_t aID) const
    { return const_cast<DocAccessibleParent*>(this)->GetAccessible(aID); }

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
  bool mTopLevel;
  bool mShutdown;
};

}
}

#endif
