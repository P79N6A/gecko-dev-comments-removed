





#ifndef mozilla_dom_ipc_BlobParent_h
#define mozilla_dom_ipc_BlobParent_h

#include "mozilla/Attributes.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/dom/PBlobParent.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"

template <class, class> class nsDataHashtable;
class nsIDHashKey;
class nsIEventTarget;
class nsIRemoteBlob;
template <class> class nsRevocableEventPtr;
class nsString;

namespace mozilla {

class Mutex;

namespace ipc {

class PBackgroundParent;

} 

namespace dom {

class ContentParent;
class BlobImpl;
class nsIContentParent;
class PBlobStreamParent;

class BlobParent final
  : public PBlobParent
{
  typedef mozilla::ipc::PBackgroundParent PBackgroundParent;

  class IDTableEntry;
  typedef nsDataHashtable<nsIDHashKey, IDTableEntry*> IDTable;

  class OpenStreamRunnable;
  friend class OpenStreamRunnable;

  class RemoteBlobImpl;

  struct CreateBlobImplMetadata;

  static StaticAutoPtr<IDTable> sIDTable;
  static StaticAutoPtr<Mutex> sIDTableMutex;

  BlobImpl* mBlobImpl;
  RemoteBlobImpl* mRemoteBlobImpl;

  
  PBackgroundParent* mBackgroundManager;
  nsCOMPtr<nsIContentParent> mContentManager;

  nsCOMPtr<nsIEventTarget> mEventTarget;

  
  
  
  
  
  
  
  nsTArray<nsRevocableEventPtr<OpenStreamRunnable>> mOpenStreamRunnables;

  nsRefPtr<IDTableEntry> mIDTableEntry;

  bool mOwnsBlobImpl;

public:
  class FriendKey;

  static void
  Startup(const FriendKey& aKey);

  
  static BlobParent*
  GetOrCreate(nsIContentParent* aManager, BlobImpl* aBlobImpl);

  static BlobParent*
  GetOrCreate(PBackgroundParent* aManager, BlobImpl* aBlobImpl);

  
  static BlobParent*
  Create(nsIContentParent* aManager,
         const ParentBlobConstructorParams& aParams);

  static BlobParent*
  Create(PBackgroundParent* aManager,
         const ParentBlobConstructorParams& aParams);

  static void
  Destroy(PBlobParent* aActor)
  {
    delete static_cast<BlobParent*>(aActor);
  }

  static already_AddRefed<BlobImpl>
  GetBlobImplForID(const nsID& aID);

  bool
  HasManager() const
  {
    return mBackgroundManager || mContentManager;
  }

  PBackgroundParent*
  GetBackgroundManager() const
  {
    return mBackgroundManager;
  }

  nsIContentParent*
  GetContentManager() const
  {
    return mContentManager;
  }

  
  already_AddRefed<BlobImpl>
  GetBlobImpl();

  void
  AssertIsOnOwningThread() const
#ifdef DEBUG
  ;
#else
  { }
#endif

private:
  
  BlobParent(nsIContentParent* aManager, IDTableEntry* aIDTableEntry);

  BlobParent(PBackgroundParent* aManager, IDTableEntry* aIDTableEntry);

  
  BlobParent(nsIContentParent* aManager,
             BlobImpl* aBlobImpl,
             IDTableEntry* aIDTableEntry);

  BlobParent(PBackgroundParent* aManager,
             BlobImpl* aBlobImpl,
             IDTableEntry* aIDTableEntry);

  
  ~BlobParent();

  void
  CommonInit(IDTableEntry* aIDTableEntry);

  void
  CommonInit(BlobImpl* aBlobImpl, IDTableEntry* aIDTableEntry);

  template <class ParentManagerType>
  static BlobParent*
  GetOrCreateFromImpl(ParentManagerType* aManager,
                      BlobImpl* aBlobImpl);

  template <class ParentManagerType>
  static BlobParent*
  CreateFromParams(ParentManagerType* aManager,
                   const ParentBlobConstructorParams& aParams);

  template <class ParentManagerType>
  static BlobParent*
  SendSliceConstructor(ParentManagerType* aManager,
                       const ParentBlobConstructorParams& aParams,
                       const ChildBlobConstructorParams& aOtherSideParams);

  static BlobParent*
  MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                              nsIContentParent* aManager);

  static BlobParent*
  MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                              PBackgroundParent* aManager);

  void
  NoteDyingRemoteBlobImpl();

  void
  NoteRunnableCompleted(OpenStreamRunnable* aRunnable);

  nsIEventTarget*
  EventTarget() const
  {
    return mEventTarget;
  }

  bool
  IsOnOwningThread() const;

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) override;

  virtual PBlobStreamParent*
  AllocPBlobStreamParent(const uint64_t& aStart,
                         const uint64_t& aLength) override;

  virtual bool
  RecvPBlobStreamConstructor(PBlobStreamParent* aActor,
                             const uint64_t& aStart,
                             const uint64_t& aLength) override;

  virtual bool
  DeallocPBlobStreamParent(PBlobStreamParent* aActor) override;

  virtual bool
  RecvResolveMystery(const ResolveMysteryParams& aParams) override;

  virtual bool
  RecvBlobStreamSync(const uint64_t& aStart,
                     const uint64_t& aLength,
                     InputStreamParams* aParams,
                     OptionalFileDescriptorSet* aFDs) override;

  virtual bool
  RecvWaitForSliceCreation() override;

  virtual bool
  RecvGetFileId(int64_t* aFileId) override;

  virtual bool
  RecvGetFilePath(nsString* aFilePath) override;
};



class BlobParent::FriendKey final
{
  friend class ContentParent;

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
