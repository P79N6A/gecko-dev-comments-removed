



#ifndef mozilla_dom_ipc_BlobChild_h
#define mozilla_dom_ipc_BlobChild_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/PBlobChild.h"
#include "nsCOMPtr.h"
#include "nsID.h"

class nsIDOMBlob;
class nsIEventTarget;
class nsIRemoteBlob;
class nsString;

namespace mozilla {
namespace ipc {

class PBackgroundChild;

} 

namespace dom {

class ContentChild;
class DOMFileImpl;
class nsIContentChild;
class PBlobStreamChild;

class BlobChild MOZ_FINAL
  : public PBlobChild
{
  typedef mozilla::ipc::PBackgroundChild PBackgroundChild;

  class RemoteBlobImpl;
  friend class RemoteBlobImpl;

  DOMFileImpl* mBlobImpl;
  RemoteBlobImpl* mRemoteBlobImpl;

  
  PBackgroundChild* mBackgroundManager;
  nsCOMPtr<nsIContentChild> mContentManager;

  nsCOMPtr<nsIEventTarget> mEventTarget;

  nsID mParentID;

  bool mOwnsBlobImpl;

public:
  class FriendKey;

  static void
  Startup(const FriendKey& aKey);

  
  static BlobChild*
  GetOrCreate(nsIContentChild* aManager, DOMFileImpl* aBlobImpl);

  static BlobChild*
  GetOrCreate(PBackgroundChild* aManager, DOMFileImpl* aBlobImpl);

  
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

  const nsID&
  ParentID() const;

  
  
  
  
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
  
  BlobChild(nsIContentChild* aManager, DOMFileImpl* aBlobImpl);

  BlobChild(PBackgroundChild* aManager, DOMFileImpl* aBlobImpl);

  BlobChild(nsIContentChild* aManager, BlobChild* aOther);

  BlobChild(PBackgroundChild* aManager, BlobChild* aOther);

  
  BlobChild(nsIContentChild* aManager,
            const ChildBlobConstructorParams& aParams);

  BlobChild(PBackgroundChild* aManager,
            const ChildBlobConstructorParams& aParams);

  
  ~BlobChild();

  void
  CommonInit(DOMFileImpl* aBlobImpl);

  void
  CommonInit(BlobChild* aOther);

  void
  CommonInit(const ChildBlobConstructorParams& aParams);

  template <class ChildManagerType>
  static BlobChild*
  GetOrCreateFromImpl(ChildManagerType* aManager, DOMFileImpl* aBlobImpl);

  template <class ChildManagerType>
  static BlobChild*
  CreateFromParams(ChildManagerType* aManager,
                   const ChildBlobConstructorParams& aParams);

  template <class ChildManagerType>
  static BlobChild*
  SendSliceConstructor(ChildManagerType* aManager,
                       const ChildBlobConstructorParams& aParams,
                       const ParentBlobConstructorParams& aOtherSideParams);

  static BlobChild*
  MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                              nsIContentChild* aManager);

  static BlobChild*
  MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                              PBackgroundChild* aManager);

  void
  NoteDyingRemoteBlobImpl();

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
  DeallocPBlobStreamChild(PBlobStreamChild* aActor) MOZ_OVERRIDE;
};



class BlobChild::FriendKey MOZ_FINAL
{
  friend class ContentChild;

private:
  FriendKey()
  { }

  FriendKey(const FriendKey& )
  { }

public:
  ~FriendKey()
  { }
};

} 
} 

#endif 
