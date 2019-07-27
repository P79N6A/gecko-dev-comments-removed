



#ifndef mozilla_dom_ipc_BlobParent_h
#define mozilla_dom_ipc_BlobParent_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/PBlobParent.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"

class nsIDOMBlob;
class nsIEventTarget;
class nsString;
template <class> class nsRevocableEventPtr;

namespace mozilla {
namespace ipc {

class PBackgroundParent;

} 

namespace dom {

class DOMFileImpl;
class nsIContentParent;
class PBlobStreamParent;

class BlobParent MOZ_FINAL
  : public PBlobParent
{
  typedef mozilla::ipc::PBackgroundParent PBackgroundParent;

  class OpenStreamRunnable;
  friend class OpenStreamRunnable;

  class RemoteBlob;
  friend class RemoteBlob;

  nsIDOMBlob* mBlob;
  RemoteBlob* mRemoteBlob;

  
  PBackgroundParent* mBackgroundManager;
  nsCOMPtr<nsIContentParent> mContentManager;

  
  nsRefPtr<DOMFileImpl> mBlobImpl;

  nsCOMPtr<nsIEventTarget> mEventTarget;

  
  
  
  
  
  
  
  nsTArray<nsRevocableEventPtr<OpenStreamRunnable>> mOpenStreamRunnables;

  bool mOwnsBlob;
  bool mOwnsRemoteBlob;

public:
  
  static BlobParent*
  Create(nsIContentParent* aManager, nsIDOMBlob* aBlob)
  {
    return new BlobParent(aManager, aBlob);
  }

  static BlobParent*
  Create(PBackgroundParent* aManager, nsIDOMBlob* aBlob)
  {
    return new BlobParent(aManager, aBlob);
  }

  static BlobParent*
  Create(PBackgroundParent* aManager, DOMFileImpl* aBlobImpl)
  {
    return new BlobParent(aManager, aBlobImpl);
  }

  
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

  
  
  
  
  already_AddRefed<nsIDOMBlob>
  GetBlob();

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
  
  BlobParent(nsIContentParent* aManager, nsIDOMBlob* aBlob);

  BlobParent(PBackgroundParent* aManager, nsIDOMBlob* aBlob);

  BlobParent(PBackgroundParent* aManager, DOMFileImpl* aBlobImpl);

  
  BlobParent(nsIContentParent* aManager,
             const ParentBlobConstructorParams& aParams);

  BlobParent(PBackgroundParent* aManager,
             const ParentBlobConstructorParams& aParams);

  
  ~BlobParent();

  void
  CommonInit(nsIDOMBlob* aBlob);

  void
  CommonInit(const ParentBlobConstructorParams& aParams);

  static BlobParent*
  CreateFromParams(nsIContentParent* aManager,
                   const ParentBlobConstructorParams& aParams);

  static BlobParent*
  CreateFromParams(PBackgroundParent* aManager,
                   const ParentBlobConstructorParams& aParams);

  template <class ParentManagerType>
  static BlobParent*
  SendSliceConstructor(ParentManagerType* aManager,
                       const ParentBlobConstructorParams& aParams,
                       const ChildBlobConstructorParams& aOtherSideParams);

  void
  NoteDyingRemoteBlob();

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
  RecvGetFileId(int64_t* aFileId) MOZ_OVERRIDE;

  virtual bool
  RecvGetFilePath(nsString* aFilePath) MOZ_OVERRIDE;
};

} 
} 

#endif 
