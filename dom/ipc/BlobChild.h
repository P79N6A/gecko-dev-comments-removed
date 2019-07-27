



#ifndef mozilla_dom_ipc_BlobChild_h
#define mozilla_dom_ipc_BlobChild_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/PBlobChild.h"
#include "nsCOMPtr.h"
#include "nsID.h"

class nsIEventTarget;
class nsIRemoteBlob;
class nsString;

namespace mozilla {
namespace ipc {

class PBackgroundChild;

} 

namespace dom {

class ContentChild;
class FileImpl;
class nsIContentChild;
class PBlobStreamChild;

class BlobChild final
  : public PBlobChild
{
  typedef mozilla::ipc::PBackgroundChild PBackgroundChild;

  class RemoteBlobImpl;
  friend class RemoteBlobImpl;

  class RemoteBlobSliceImpl;
  friend class RemoteBlobSliceImpl;

  FileImpl* mBlobImpl;
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
  GetOrCreate(nsIContentChild* aManager, FileImpl* aBlobImpl);

  static BlobChild*
  GetOrCreate(PBackgroundChild* aManager, FileImpl* aBlobImpl);

  
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

  
  
  
  
  already_AddRefed<FileImpl>
  GetBlobImpl();

  
  bool
  SetMysteryBlobInfo(const nsString& aName,
                     const nsString& aContentType,
                     uint64_t aLength,
                     int64_t aLastModifiedDate);

  
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
  
  BlobChild(nsIContentChild* aManager, FileImpl* aBlobImpl);

  BlobChild(PBackgroundChild* aManager, FileImpl* aBlobImpl);

  BlobChild(nsIContentChild* aManager, BlobChild* aOther);

  BlobChild(PBackgroundChild* aManager, BlobChild* aOther, FileImpl* aBlobImpl);

  
  BlobChild(nsIContentChild* aManager,
            const ChildBlobConstructorParams& aParams);

  BlobChild(PBackgroundChild* aManager,
            const ChildBlobConstructorParams& aParams);

  
  BlobChild(nsIContentChild* aManager,
            const nsID& aParentID,
            RemoteBlobSliceImpl* aRemoteBlobSliceImpl);

  BlobChild(PBackgroundChild* aManager,
            const nsID& aParentID,
            RemoteBlobSliceImpl* aRemoteBlobSliceImpl);

  
  ~BlobChild();

  void
  CommonInit(FileImpl* aBlobImpl);

  void
  CommonInit(BlobChild* aOther, FileImpl* aBlobImpl);

  void
  CommonInit(const ChildBlobConstructorParams& aParams);

  void
  CommonInit(const nsID& aParentID, RemoteBlobImpl* aRemoteBlobImpl);

  template <class ChildManagerType>
  static BlobChild*
  GetOrCreateFromImpl(ChildManagerType* aManager, FileImpl* aBlobImpl);

  template <class ChildManagerType>
  static BlobChild*
  CreateFromParams(ChildManagerType* aManager,
                   const ChildBlobConstructorParams& aParams);

  template <class ChildManagerType>
  static BlobChild*
  SendSliceConstructor(ChildManagerType* aManager,
                       RemoteBlobSliceImpl* aRemoteBlobSliceImpl,
                       const ParentBlobConstructorParams& aParams);

  static BlobChild*
  MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                              nsIContentChild* aManager,
                              FileImpl* aBlobImpl);

  static BlobChild*
  MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                              PBackgroundChild* aManager,
                              FileImpl* aBlobImpl);

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
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual PBlobStreamChild*
  AllocPBlobStreamChild(const uint64_t& aStart,
                        const uint64_t& aLength) override;

  virtual bool
  DeallocPBlobStreamChild(PBlobStreamChild* aActor) override;
};



class BlobChild::FriendKey final
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
