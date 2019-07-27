



#ifndef mozilla_dom_ipc_BlobChild_h
#define mozilla_dom_ipc_BlobChild_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/PBlobChild.h"
#include "nsCOMPtr.h"

class nsIDOMBlob;
class nsIEventTarget;
class nsString;

namespace mozilla {
namespace ipc {

class PBackgroundChild;

} 

namespace dom {

class DOMFileImpl;
class nsIContentChild;
class PBlobStreamChild;

class BlobChild MOZ_FINAL
  : public PBlobChild
{
  typedef mozilla::ipc::PBackgroundChild PBackgroundChild;

  class RemoteBlob;
  friend class RemoteBlob;

  nsIDOMBlob* mBlob;
  RemoteBlob* mRemoteBlob;

  
  PBackgroundChild* mBackgroundManager;
  nsCOMPtr<nsIContentChild> mContentManager;

  nsCOMPtr<nsIEventTarget> mEventTarget;

  bool mOwnsBlob;

public:
  
  static BlobChild*
  Create(nsIContentChild* aManager, nsIDOMBlob* aBlob)
  {
    return new BlobChild(aManager, aBlob);
  }

  static BlobChild*
  Create(PBackgroundChild* aManager, nsIDOMBlob* aBlob)
  {
    return new BlobChild(aManager, aBlob);
  }

  
  static BlobChild*
  Create(nsIContentChild* aManager, const ChildBlobConstructorParams& aParams);

  static BlobChild*
  Create(PBackgroundChild* aManager,
         const ChildBlobConstructorParams& aParams);

  static void
  Destroy(PBlobChild* aActor)
  {
    delete static_cast<BlobChild*>(aActor);
  }

  bool
  HasManager() const
  {
    return mBackgroundManager || mContentManager;
  }

  PBackgroundChild*
  GetBackgroundManager() const
  {
    return mBackgroundManager;
  }

  nsIContentChild*
  GetContentManager() const
  {
    return mContentManager;
  }

  
  
  
  already_AddRefed<nsIDOMBlob>
  GetBlob();

  already_AddRefed<DOMFileImpl>
  GetBlobImpl();

  
  bool
  SetMysteryBlobInfo(const nsString& aName,
                     const nsString& aContentType,
                     uint64_t aLength,
                     uint64_t aLastModifiedDate);

  
  bool
  SetMysteryBlobInfo(const nsString& aContentType, uint64_t aLength);

  void
  AssertIsOnOwningThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif

private:
  
  BlobChild(nsIContentChild* aManager, nsIDOMBlob* aBlob);

  BlobChild(PBackgroundChild* aManager, nsIDOMBlob* aBlob);

  
  BlobChild(nsIContentChild* aManager,
            const ChildBlobConstructorParams& aParams);

  BlobChild(PBackgroundChild* aManager,
            const ChildBlobConstructorParams& aParams);

  
  ~BlobChild();

  void
  CommonInit(nsIDOMBlob* aBlob);

  void
  CommonInit(const ChildBlobConstructorParams& aParams);

  template <class ChildManagerType>
  static BlobChild*
  CreateFromParams(ChildManagerType* aManager,
                   const ChildBlobConstructorParams& aParams);

  template <class ChildManagerType>
  static BlobChild*
  SendSliceConstructor(ChildManagerType* aManager,
                       const NormalBlobConstructorParams& aParams,
                       const ParentBlobConstructorParams& aOtherSideParams);

  void
  NoteDyingRemoteBlob();

  nsIEventTarget*
  EventTarget() const
  {
    return mEventTarget;
  }

  bool
  IsOnOwningThread() const;

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual PBlobStreamChild*
  AllocPBlobStreamChild() MOZ_OVERRIDE;

  virtual bool
  RecvPBlobStreamConstructor(PBlobStreamChild* aActor) MOZ_OVERRIDE;

  virtual bool
  DeallocPBlobStreamChild(PBlobStreamChild* aActor) MOZ_OVERRIDE;
};

} 
} 

#endif 
