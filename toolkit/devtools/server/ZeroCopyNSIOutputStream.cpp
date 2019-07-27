




#include "mozilla/devtools/ZeroCopyNSIOutputStream.h"

#include "mozilla/DebugOnly.h"

namespace mozilla {
namespace devtools {

ZeroCopyNSIOutputStream::ZeroCopyNSIOutputStream(nsCOMPtr<nsIOutputStream>& out)
  : out(out)
  , result_(NS_OK)
  , amountUsed(0)
  , writtenCount(0)
{
  DebugOnly<bool> nonBlocking = false;
  MOZ_ASSERT(out->IsNonBlocking(&nonBlocking) == NS_OK);
  MOZ_ASSERT(!nonBlocking);
}

ZeroCopyNSIOutputStream::~ZeroCopyNSIOutputStream()
{
  if (!failed())
    NS_WARN_IF(NS_FAILED(writeBuffer()));
}

nsresult
ZeroCopyNSIOutputStream::writeBuffer()
{
  if (failed())
    return result_;

  if (amountUsed == 0)
    return NS_OK;

  int32_t amountWritten = 0;
  while (amountWritten < amountUsed) {
    uint32_t justWritten = 0;

    result_ = out->Write(buffer + amountWritten,
                         amountUsed - amountWritten,
                         &justWritten);
    if (NS_WARN_IF(NS_FAILED(result_)))
      return result_;

    amountWritten += justWritten;
  }

  writtenCount += amountUsed;
  amountUsed = 0;
  return NS_OK;
}



bool
ZeroCopyNSIOutputStream::Next(void** data, int* size)
{
  MOZ_ASSERT(data != nullptr);
  MOZ_ASSERT(size != nullptr);

  if (failed())
    return false;

  if (amountUsed == BUFFER_SIZE) {
    if (NS_FAILED(writeBuffer()))
      return false;
  }

  *data = buffer + amountUsed;
  *size = BUFFER_SIZE - amountUsed;
  amountUsed = BUFFER_SIZE;
  return true;
}

void
ZeroCopyNSIOutputStream::BackUp(int count)
{
  MOZ_ASSERT(count > 0,
             "Must back up a positive number of bytes.");
  MOZ_ASSERT(amountUsed == BUFFER_SIZE,
             "Can only call BackUp directly after calling Next.");
  MOZ_ASSERT(count <= amountUsed,
             "Can't back up further than we've given out.");

  amountUsed -= count;
}

::google::protobuf::int64
ZeroCopyNSIOutputStream::ByteCount() const
{
  return writtenCount + amountUsed;
}

} 
} 
