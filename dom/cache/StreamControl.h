





#ifndef mozilla_dom_cache_StreamControl_h
#define mozilla_dom_cache_StreamControl_h

#include "mozilla/dom/cache/ReadStream.h"
#include "nsRefPtr.h"
#include "nsTObserverArray.h"

struct nsID;

namespace mozilla {
namespace ipc {
 class FileDescriptor;
}
namespace dom {
namespace cache {

class PCacheReadStream;




class StreamControl
{
public:
  
  virtual void
  SerializeControl(PCacheReadStream* aReadStreamOut) = 0;

  virtual void
  SerializeFds(PCacheReadStream* aReadStreamOut,
               const nsTArray<mozilla::ipc::FileDescriptor>& aFds) = 0;

  virtual void
  DeserializeFds(const PCacheReadStream& aReadStream,
                 nsTArray<mozilla::ipc::FileDescriptor>& aFdsOut) = 0;

  

  
  
  
  void
  AddReadStream(ReadStream::Controllable* aReadStream);

  
  void
  ForgetReadStream(ReadStream::Controllable* aReadStream);

  
  void
  NoteClosed(ReadStream::Controllable* aReadStream, const nsID& aId);

protected:
  ~StreamControl();

  void
  CloseReadStreams(const nsID& aId);

  void
  CloseAllReadStreams();

  void
  CloseAllReadStreamsWithoutReporting();

  
  virtual void
  NoteClosedAfterForget(const nsID& aId) = 0;

#ifdef DEBUG
  virtual void
  AssertOwningThread() = 0;
#else
  void AssertOwningThread() { }
#endif

private:
  
  
  typedef nsTObserverArray<nsRefPtr<ReadStream::Controllable>> ReadStreamList;
  ReadStreamList mReadStreamList;
};

} 
} 
} 

#endif 
