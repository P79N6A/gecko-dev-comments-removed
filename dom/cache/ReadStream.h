





#ifndef mozilla_dom_cache_ReadStream_h
#define mozilla_dom_cache_ReadStream_h

#include "mozilla/ipc/FileDescriptor.h"
#include "nsCOMPtr.h"
#include "nsID.h"
#include "nsIInputStream.h"
#include "nsISupportsImpl.h"
#include "nsRefPtr.h"
#include "nsTArrayForwardDeclare.h"

class nsIThread;

namespace mozilla {
namespace dom {
namespace cache {

class PCacheReadStream;
class PCacheReadStreamOrVoid;
class PCacheStreamControlParent;


#define NS_DOM_CACHE_READSTREAM_IID \
{0x8e5da7c9, 0x0940, 0x4f1d, \
  {0x97, 0x25, 0x5c, 0x59, 0x38, 0xdd, 0xb9, 0x9f}}













class ReadStream MOZ_FINAL : public nsIInputStream
{
public:
  
  
  class Controllable
  {
  public:
    
    
    virtual void
    CloseStream() = 0;

    
    
    virtual void
    CloseStreamWithoutReporting() = 0;

    virtual bool
    MatchId(const nsID& aId) const = 0;

    NS_IMETHOD_(MozExternalRefCountType)
    AddRef(void) = 0;

    NS_IMETHOD_(MozExternalRefCountType)
    Release(void) = 0;
  };

  static already_AddRefed<ReadStream>
  Create(const PCacheReadStreamOrVoid& aReadStreamOrVoid);

  static already_AddRefed<ReadStream>
  Create(const PCacheReadStream& aReadStream);

  static already_AddRefed<ReadStream>
  Create(PCacheStreamControlParent* aControl, const nsID& aId,
         nsIInputStream* aStream);

  void Serialize(PCacheReadStreamOrVoid* aReadStreamOut);
  void Serialize(PCacheReadStream* aReadStreamOut);

private:
  class Inner;

  explicit ReadStream(Inner* aInner);
  ~ReadStream();

  
  
  
  
  
  nsRefPtr<Inner> mInner;

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_DOM_CACHE_READSTREAM_IID);
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIINPUTSTREAM
};

NS_DEFINE_STATIC_IID_ACCESSOR(ReadStream, NS_DOM_CACHE_READSTREAM_IID);

} 
} 
} 

#endif
