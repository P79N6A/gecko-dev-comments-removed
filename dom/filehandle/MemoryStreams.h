





#ifndef mozilla_dom_MemoryStreams_h
#define mozilla_dom_MemoryStreams_h

#include "nsIOutputStream.h"
#include "nsString.h"

template <class> class already_AddRefed;

namespace mozilla {
namespace dom {

class MemoryOutputStream : public nsIOutputStream
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM

  static already_AddRefed<MemoryOutputStream>
  Create(uint64_t aSize);

  const nsCString&
  Data() const
  {
    return mData;
  }

private:
  MemoryOutputStream()
  : mOffset(0)
  { }

  virtual ~MemoryOutputStream()
  { }

  nsCString mData;
  uint64_t mOffset;
};

} 
} 

#endif 
