





#ifndef mozilla_dom_ipc_Blob_h
#define mozilla_dom_ipc_Blob_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/PBlobChild.h"
#include "mozilla/dom/PBlobParent.h"
#include "mozilla/dom/PBlobStreamChild.h"
#include "mozilla/dom/PBlobStreamParent.h"
#include "mozilla/dom/PContent.h"

#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"

class nsIDOMBlob;
class nsIIPCSerializableInputStream;
class nsIInputStream;

namespace mozilla {
namespace dom {

class ContentParent;
class ContentChild;

namespace ipc {

enum ActorFlavorEnum
{
  Parent = 0,
  Child
};

template <ActorFlavorEnum>
struct BlobTraits
{ };

template <>
struct BlobTraits<Parent>
{
  typedef mozilla::dom::PBlobParent ProtocolType;
  typedef mozilla::dom::PBlobStreamParent StreamType;
  typedef mozilla::dom::PContentParent ContentManagerType;
  typedef mozilla::dom::ContentParent ConcreteContentManagerType;
  typedef ProtocolType BlobManagerType;

  
  
  
  
  
  
  
  
  
  class BaseType : public ProtocolType
  {
  protected:
    BaseType();
    virtual ~BaseType();

    class OpenStreamRunnable;
    friend class OpenStreamRunnable;

    void
    NoteRunnableCompleted(OpenStreamRunnable* aRunnable);

    nsTArray<nsRevocableEventPtr<OpenStreamRunnable> > mOpenStreamRunnables;
  };

  static void*
  Allocate(size_t aSize)
  {
    
    return moz_malloc(aSize);
  }
};

template <>
struct BlobTraits<Child>
{
  typedef mozilla::dom::PBlobChild ProtocolType;
  typedef mozilla::dom::PBlobStreamChild StreamType;
  typedef mozilla::dom::PContentChild ContentManagerType;
  typedef mozilla::dom::ContentChild ConcreteContentManagerType;
  typedef ProtocolType BlobManagerType;

  class BaseType : public ProtocolType
  {
  protected:
    BaseType()
    { }

    virtual ~BaseType()
    { }
  };

  static void*
  Allocate(size_t aSize)
  {
    
    return moz_xmalloc(aSize);
  }
};

template <ActorFlavorEnum>
class RemoteBlobBase;
template <ActorFlavorEnum>
class RemoteBlob;
template <ActorFlavorEnum>
class RemoteMemoryBlob;
template <ActorFlavorEnum>
class RemoteMultipartBlob;

template <ActorFlavorEnum ActorFlavor>
class Blob : public BlobTraits<ActorFlavor>::BaseType
{
  friend class RemoteBlobBase<ActorFlavor>;

public:
  typedef typename BlobTraits<ActorFlavor>::ProtocolType ProtocolType;
  typedef typename BlobTraits<ActorFlavor>::StreamType StreamType;
  typedef typename BlobTraits<ActorFlavor>::BaseType BaseType;
  typedef typename BlobTraits<ActorFlavor>::ContentManagerType ContentManagerType;
  typedef typename BlobTraits<ActorFlavor>::BlobManagerType BlobManagerType;
  typedef RemoteBlob<ActorFlavor> RemoteBlobType;
  typedef RemoteMemoryBlob<ActorFlavor> RemoteMemoryBlobType;
  typedef RemoteMultipartBlob<ActorFlavor> RemoteMultipartBlobType;
  typedef mozilla::ipc::IProtocolManager<
                      mozilla::ipc::RPCChannel::RPCListener>::ActorDestroyReason
          ActorDestroyReason;
  typedef mozilla::dom::BlobConstructorParams BlobConstructorParams;

protected:
  nsIDOMBlob* mBlob;
  RemoteBlobType* mRemoteBlob;
  RemoteMemoryBlobType* mRemoteMemoryBlob;
  RemoteMultipartBlobType* mRemoteMultipartBlob;

  ContentManagerType* mContentManager;
  BlobManagerType* mBlobManager;

  bool mOwnsBlob;
  bool mBlobIsFile;

public:
  
  static Blob*
  Create(nsIDOMBlob* aBlob)
  {
    return new Blob(aBlob);
  }

  
  static Blob*
  Create(const BlobConstructorParams& aParams);

  
  
  
  already_AddRefed<nsIDOMBlob>
  GetBlob();

  
  bool
  SetMysteryBlobInfo(const nsString& aName, const nsString& aContentType,
                     uint64_t aLength, uint64_t aLastModifiedDate);

  
  bool
  SetMysteryBlobInfo(const nsString& aContentType, uint64_t aLength);

  ProtocolType*
  ConstructPBlobOnManager(ProtocolType* aActor,
                          const BlobConstructorParams& aParams);

  bool
  ManagerIs(const ContentManagerType* aManager) const
  {
    return aManager == mContentManager;
  }

  bool
  ManagerIs(const BlobManagerType* aManager) const
  {
    return aManager == mBlobManager;
  }

#ifdef DEBUG
  bool
  HasManager() const
  {
    return !!mContentManager || !!mBlobManager;
  }
#endif

  void
  SetManager(ContentManagerType* aManager);
  void
  SetManager(BlobManagerType* aManager);

  void
  PropagateManager(Blob<ActorFlavor>* aActor) const;

private:
  
  Blob(nsIDOMBlob* aBlob);

  
  Blob(nsRefPtr<RemoteBlobType>& aBlob,
       bool aIsFile);
  Blob(nsRefPtr<RemoteMemoryBlobType>& aBlob,
       bool aIsFile);
  Blob(nsRefPtr<RemoteMultipartBlobType>& aBlob,
       bool aIsFile);

  void
  NoteDyingRemoteBlob();

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvResolveMystery(const ResolveMysteryParams& aParams) MOZ_OVERRIDE;

  virtual bool
  RecvPBlobStreamConstructor(StreamType* aActor) MOZ_OVERRIDE;

  virtual bool
  RecvPBlobConstructor(ProtocolType* aActor,
                       const BlobConstructorParams& aParams) MOZ_OVERRIDE;

  virtual StreamType*
  AllocPBlobStream() MOZ_OVERRIDE;

  virtual bool
  DeallocPBlobStream(StreamType* aActor) MOZ_OVERRIDE;

  virtual ProtocolType*
  AllocPBlob(const BlobConstructorParams& params) MOZ_OVERRIDE;

  virtual bool
  DeallocPBlob(ProtocolType* actor) MOZ_OVERRIDE;
};

} 

typedef mozilla::dom::ipc::Blob<mozilla::dom::ipc::Child> BlobChild;
typedef mozilla::dom::ipc::Blob<mozilla::dom::ipc::Parent> BlobParent;

} 
} 

#endif 
