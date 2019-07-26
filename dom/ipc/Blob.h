



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

class ContentChild;
class ContentParent;
class PBlobStreamChild;
class PBlobStreamParent;

class BlobChild MOZ_FINAL
  : public PBlobChild
{
  friend class ContentChild;

  class RemoteBlob;
  friend class RemoteBlob;

  nsIDOMBlob* mBlob;
  RemoteBlob* mRemoteBlob;
  nsRefPtr<ContentChild> mStrongManager;

  bool mOwnsBlob;
  bool mBlobIsFile;

public:
  
  static BlobChild*
  Create(ContentChild* aManager, nsIDOMBlob* aBlob)
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

private:
  
  BlobChild(ContentChild* aManager, nsIDOMBlob* aBlob);

  
  BlobChild(ContentChild* aManager, const ChildBlobConstructorParams& aParams);

  
  ~BlobChild();

  
  static BlobChild*
  Create(ContentChild* aManager, const ChildBlobConstructorParams& aParams);

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
  friend class ContentParent;

  class OpenStreamRunnable;
  friend class OpenStreamRunnable;

  class RemoteBlob;
  friend class RemoteBlob;

  nsIDOMBlob* mBlob;
  RemoteBlob* mRemoteBlob;
  nsRefPtr<ContentParent> mStrongManager;

  
  
  
  
  
  
  
  nsTArray<nsRevocableEventPtr<OpenStreamRunnable>> mOpenStreamRunnables;

  bool mOwnsBlob;
  bool mBlobIsFile;

public:
  
  static BlobParent*
  Create(ContentParent* aManager, nsIDOMBlob* aBlob)
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

private:
  
  BlobParent(ContentParent* aManager, nsIDOMBlob* aBlob);

  
  BlobParent(ContentParent* aManager,
             const ParentBlobConstructorParams& aParams);

  ~BlobParent();

  
  static BlobParent*
  Create(ContentParent* aManager, const ParentBlobConstructorParams& aParams);

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
