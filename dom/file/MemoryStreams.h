





#ifndef mozilla_dom_file_memorystreams_h__
#define mozilla_dom_file_memorystreams_h__

#include "FileCommon.h"

#include "nsIOutputStream.h"

BEGIN_FILE_NAMESPACE

class MemoryOutputStream : public nsIOutputStream
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOUTPUTSTREAM

  static already_AddRefed<MemoryOutputStream>
  Create(PRUint64 aSize);


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
  PRUint64 mOffset;
};

END_FILE_NAMESPACE

#endif 
