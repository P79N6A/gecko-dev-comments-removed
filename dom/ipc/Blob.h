





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
struct BlobTraits;

template <>
struct BlobTraits<Parent>
{
  typedef mozilla::dom::PBlobParent ProtocolType;
  typedef mozilla::dom::PBlobStreamParent StreamType;
  typedef mozilla::dom::ParentBlobConstructorParams ConstructorParamsType;
  typedef mozilla::dom::ChildBlobConstructorParams
          OtherSideConstructorParamsType;
  typedef mozilla::dom::ContentParent ConcreteContentManagerType;

  
  
  
  
  
  
  
  
  
  class BaseType : public ProtocolType
  {
  public:
    static const ChildBlobConstructorParams&
    GetBlobConstructorParams(const ConstructorParamsType& aParams)
    {
      return aParams.blobParams();
    }

    static void
    SetBlobConstructorParams(ConstructorParamsType& aParams,
                             const ChildBlobConstructorParams& aBlobParams)
    {
      aParams.blobParams() = aBlobParams;
      aParams.optionalInputStreamParams() = mozilla::void_t();
    }

    static void
    SetBlobConstructorParams(OtherSideConstructorParamsType& aParams,
                             const ChildBlobConstructorParams& aBlobParams)
    {
      aParams = aBlobParams;
    }

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
  typedef mozilla::dom::ChildBlobConstructorParams ConstructorParamsType;
  typedef mozilla::dom::ParentBlobConstructorParams
          OtherSideConstructorParamsType;
  typedef mozilla::dom::ContentChild ConcreteContentManagerType;


  class BaseType : public ProtocolType
  {
  public:
    static const ChildBlobConstructorParams&
    GetBlobConstructorParams(const ConstructorParamsType& aParams)
    {
      return aParams;
    }

    static void
    SetBlobConstructorParams(ConstructorParamsType& aParams,
                             const ChildBlobConstructorParams& aBlobParams)
    {
      aParams = aBlobParams;
    }

    static void
    SetBlobConstructorParams(OtherSideConstructorParamsType& aParams,
                             const ChildBlobConstructorParams& aBlobParams)
    {
      aParams.blobParams() = aBlobParams;
      aParams.optionalInputStreamParams() = mozilla::void_t();
    }

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
  typedef typename BlobTraits<ActorFlavor>::ConstructorParamsType
          ConstructorParamsType;
  typedef typename BlobTraits<ActorFlavor>::OtherSideConstructorParamsType
          OtherSideConstructorParamsType;
  typedef typename BlobTraits<ActorFlavor>::BaseType BaseType;
  typedef RemoteBlob<ActorFlavor> RemoteBlobType;
  typedef mozilla::ipc::IProtocolManager<
                      mozilla::ipc::RPCChannel::RPCListener>::ActorDestroyReason
          ActorDestroyReason;

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
  Create(const ConstructorParamsType& aParams);

  
  
  
  already_AddRefed<nsIDOMBlob>
  GetBlob();

  
  bool
  SetMysteryBlobInfo(const nsString& aName, const nsString& aContentType,
                     uint64_t aLength, uint64_t aLastModifiedDate);

  
  bool
  SetMysteryBlobInfo(const nsString& aContentType, uint64_t aLength);

private:
  
  Blob(nsIDOMBlob* aBlob);

  
  Blob(const ConstructorParamsType& aParams);

  static already_AddRefed<RemoteBlobType>
  CreateRemoteBlob(const ConstructorParamsType& aParams);

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