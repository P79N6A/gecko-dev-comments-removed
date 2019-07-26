





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
};

template <>
struct BlobTraits<Child>
{
  typedef mozilla::dom::PBlobChild ProtocolType;
  typedef mozilla::dom::PBlobStreamChild StreamType;

  class BaseType : public ProtocolType
  {
  protected:
    BaseType()
    { }

    virtual ~BaseType()
    { }
  };
};

template <ActorFlavorEnum>
class RemoteBlob;

template <ActorFlavorEnum ActorFlavor>
class Blob : public BlobTraits<ActorFlavor>::BaseType
{
  friend class RemoteBlob<ActorFlavor>;

public:
  typedef typename BlobTraits<ActorFlavor>::ProtocolType ProtocolType;
  typedef typename BlobTraits<ActorFlavor>::StreamType StreamType;
  typedef typename BlobTraits<ActorFlavor>::BaseType BaseType;
  typedef RemoteBlob<ActorFlavor> RemoteBlobType;
  typedef mozilla::ipc::IProtocolManager<
                      mozilla::ipc::RPCChannel::RPCListener>::ActorDestroyReason
          ActorDestroyReason;
  typedef mozilla::dom::BlobConstructorParams BlobConstructorParams;

protected:
  nsIDOMBlob* mBlob;
  RemoteBlobType* mRemoteBlob;
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

private:
  
  Blob(nsIDOMBlob* aBlob);

  
  Blob(const BlobConstructorParams& aParams);

  void
  SetRemoteBlob(nsRefPtr<RemoteBlobType>& aRemoteBlob);

  void
  NoteDyingRemoteBlob();

  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvResolveMystery(const ResolveMysteryParams& aParams) MOZ_OVERRIDE;

  virtual bool
  RecvPBlobStreamConstructor(StreamType* aActor) MOZ_OVERRIDE;

  virtual StreamType*
  AllocPBlobStream() MOZ_OVERRIDE;

  virtual bool
  DeallocPBlobStream(StreamType* aActor) MOZ_OVERRIDE;
};

} 

typedef mozilla::dom::ipc::Blob<mozilla::dom::ipc::Child> BlobChild;
typedef mozilla::dom::ipc::Blob<mozilla::dom::ipc::Parent> BlobParent;

} 
} 

#endif 
