





#ifndef mozilla_dom_cache_ReadStream_h
#define mozilla_dom_cache_ReadStream_h

#include "mozilla/ipc/FileDescriptor.h"
#include "nsCOMPtr.h"
#include "nsID.h"
#include "nsIInputStream.h"
#include "nsISupportsImpl.h"
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












class ReadStream : public nsIInputStream
{
public:
  static already_AddRefed<ReadStream>
  Create(const PCacheReadStreamOrVoid& aReadStreamOrVoid);

  static already_AddRefed<ReadStream>
  Create(const PCacheReadStream& aReadStream);

  static already_AddRefed<ReadStream>
  Create(PCacheStreamControlParent* aControl, const nsID& aId,
         nsIInputStream* aStream);

  void Serialize(PCacheReadStreamOrVoid* aReadStreamOut);
  void Serialize(PCacheReadStream* aReadStreamOut);

  
  void CloseStream();
  void CloseStreamWithoutReporting();
  bool MatchId(const nsID& aId) const;

protected:
  class NoteClosedRunnable;
  class ForgetRunnable;

  ReadStream(const nsID& aId, nsIInputStream* aStream);
  virtual ~ReadStream();

  void NoteClosed();
  void Forget();

  virtual void NoteClosedOnOwningThread() = 0;
  virtual void ForgetOnOwningThread() = 0;
  virtual void SerializeControl(PCacheReadStream* aReadStreamOut) = 0;

  virtual void
  SerializeFds(PCacheReadStream* aReadStreamOut,
               const nsTArray<mozilla::ipc::FileDescriptor>& fds) = 0;

  const nsID mId;
  nsCOMPtr<nsIInputStream> mStream;
  nsCOMPtr<nsIInputStream> mSnappyStream;
  nsCOMPtr<nsIThread> mOwningThread;
  bool mClosed;

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
