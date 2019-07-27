



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
class nsIDOMBlob;
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
class DOMFileImpl;
class nsIContentParent;
class PBlobStreamParent;

class BlobParent MOZ_FINAL
  : public PBlobParent
{
  typedef mozilla::ipc::PBackgroundParent PBackgroundParent;

  class IDTableEntry;
  typedef nsDataHashtable<nsIDHashKey, IDTableEntry*> IDTable;

  class OpenStreamRunnable;
  friend class OpenStreamRunnable;

  class RemoteBlobImplBase;
  friend class RemoteBlobImplBase;

  class RemoteBlobImpl;
  class ForwardingRemoteBlobImpl;

  static StaticAutoPtr<IDTable> sIDTable;
  static StaticAutoPtr<Mutex> sIDTableMutex;

  DOMFileImpl* mBlobImpl;
  RemoteBlobImplBase* mRemoteBlobImpl;

  
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
  GetOrCreate(nsIContentParent* aManager, DOMFileImpl* aBlobImpl);

  static BlobParent*
  GetOrCreate(PBackgroundParent* aManager, DOMFileImpl* aBlobImpl);

  
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

  
  already_AddRefed<DOMFileImpl>
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
             const ParentBlobConstructorParams& aParams,
             IDTableEntry* aIDTableEntry);

  BlobParent(PBackgroundParent* aManager,
             const ParentBlobConstructorParams& aParams,
             IDTableEntry* aIDTableEntry);

  
  ~BlobParent();

  void
  CommonInit(IDTableEntry* aIDTableEntry);

  void
  CommonInit(const ParentBlobConstructorParams& aParams,
             IDTableEntry* aIDTableEntry);

  template <class ParentManagerType>
  static BlobParent*
  GetOrCreateFromImpl(ParentManagerType* aManager,
                      DOMFileImpl* aBlobImpl);

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
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual PBlobStreamParent*
  AllocPBlobStreamParent() MOZ_OVERRIDE;

  virtual bool
  RecvPBlobStreamConstructor(PBlobStreamParent* aActor) MOZ_OVERRIDE;

  virtual bool
  DeallocPBlobStreamParent(PBlobStreamParent* aActor) MOZ_OVERRIDE;

  virtual bool
  RecvResolveMystery(const ResolveMysteryParams& aParams) MOZ_OVERRIDE;

  virtual bool
  RecvWaitForSliceCreation() MOZ_OVERRIDE;

  virtual bool
  RecvGetFileId(int64_t* aFileId) MOZ_OVERRIDE;

  virtual bool
  RecvGetFilePath(nsString* aFilePath) MOZ_OVERRIDE;
};



class BlobParent::FriendKey MOZ_FINAL
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
