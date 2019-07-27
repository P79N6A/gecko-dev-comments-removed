



#ifndef mozilla_dom_ipc_Blob_h
#define mozilla_dom_ipc_Blob_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/PBlobChild.h"
#include "mozilla/dom/PBlobParent.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"

class nsIDOMBlob;
class nsString;
template <class> class nsRevocableEventPtr;

namespace mozilla {
namespace dom {

class nsIContentChild;
class nsIContentParent;
class PBlobStreamChild;
class PBlobStreamParent;

class BlobChild MOZ_FINAL
  : public PBlobChild
{
  friend class nsIContentChild;

  class RemoteBlob;
  friend class RemoteBlob;

  nsIDOMBlob* mBlob;
  RemoteBlob* mRemoteBlob;
  nsRefPtr<nsIContentChild> mStrongManager;

  bool mOwnsBlob;
  bool mBlobIsFile;

public:
  
  static BlobChild*
  Create(nsIContentChild* aManager, nsIDOMBlob* aBlob)
  {
    return new BlobChild(aManager, aBlob);
  }

  
  
  
  already_AddRefed<nsIDOMBlob>
  GetBlob();

  
  bool
  SetMysteryBlobInfo(const nsString& aName,
                     const nsString& aContentType,
                     uint64_t aLength,
                     uint64_t aLastModifiedDate);

  
  bool
  SetMysteryBlobInfo(const nsString& aContentType, uint64_t aLength);

  nsIContentChild* Manager();

private:
  
  BlobChild(nsIContentChild* aManager, nsIDOMBlob* aBlob);

  
  BlobChild(nsIContentChild* aManager, const ChildBlobConstructorParams& aParams);

  
  ~BlobChild();

  
  static BlobChild*
  Create(nsIContentChild* aManager, const ChildBlobConstructorParams& aParams);

  static already_AddRefed<RemoteBlob>
  CreateRemoteBlob(const ChildBlobConstructorParams& aParams);

  void
  NoteDyingRemoteBlob();

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvResolveMystery(const ResolveMysteryParams& aParams) MOZ_OVERRIDE;

  virtual PBlobStreamChild*
  AllocPBlobStreamChild() MOZ_OVERRIDE;

  virtual bool
  RecvPBlobStreamConstructor(PBlobStreamChild* aActor) MOZ_OVERRIDE;

  virtual bool
  DeallocPBlobStreamChild(PBlobStreamChild* aActor) MOZ_OVERRIDE;
};

class BlobParent MOZ_FINAL
  : public PBlobParent
{
  friend class nsIContentParent;

  class OpenStreamRunnable;
  friend class OpenStreamRunnable;

  class RemoteBlob;
  friend class RemoteBlob;

  nsIDOMBlob* mBlob;
  RemoteBlob* mRemoteBlob;
  nsRefPtr<nsIContentParent> mStrongManager;

  
  
  
  
  
  
  
  nsTArray<nsRevocableEventPtr<OpenStreamRunnable>> mOpenStreamRunnables;

  bool mOwnsBlob;
  bool mBlobIsFile;

public:
  
  static BlobParent*
  Create(nsIContentParent* aManager, nsIDOMBlob* aBlob)
  {
    return new BlobParent(aManager, aBlob);
  }

  
  
  
  already_AddRefed<nsIDOMBlob>
  GetBlob();

  
  bool
  SetMysteryBlobInfo(const nsString& aName, const nsString& aContentType,
                     uint64_t aLength, uint64_t aLastModifiedDate);

  
  bool
  SetMysteryBlobInfo(const nsString& aContentType, uint64_t aLength);

  nsIContentParent* Manager();

private:
  
  BlobParent(nsIContentParent* aManager, nsIDOMBlob* aBlob);

  
  BlobParent(nsIContentParent* aManager,
             const ParentBlobConstructorParams& aParams);

  ~BlobParent();

  
  static BlobParent*
  Create(nsIContentParent* aManager, const ParentBlobConstructorParams& aParams);

  static already_AddRefed<RemoteBlob>
  CreateRemoteBlob(const ParentBlobConstructorParams& aParams);

  void
  NoteDyingRemoteBlob();

  void
  NoteRunnableCompleted(OpenStreamRunnable* aRunnable);

  
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
};

} 
} 

#endif 
