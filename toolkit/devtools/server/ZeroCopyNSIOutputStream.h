




#ifndef mozilla_devtools_ZeroCopyNSIOutputStream__
#define mozilla_devtools_ZeroCopyNSIOutputStream__

#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/stubs/common.h>

#include "nsCOMPtr.h"
#include "nsIOutputStream.h"

namespace mozilla {
namespace devtools {








class MOZ_STACK_CLASS ZeroCopyNSIOutputStream
  : public ::google::protobuf::io::ZeroCopyOutputStream
{
  static const int BUFFER_SIZE = 8192;

  
  nsCOMPtr<nsIOutputStream>& out;

  
  char buffer[BUFFER_SIZE];

  
  nsresult result_;

  
  int amountUsed;

  
  
  
  int64_t writtenCount;

  
  nsresult writeBuffer();

public:
  explicit ZeroCopyNSIOutputStream(nsCOMPtr<nsIOutputStream>& out);

  nsresult flush() { return writeBuffer(); }

  
  bool failed() const { return NS_FAILED(result_); }

  nsresult result() const { return result_; }

  
  virtual ~ZeroCopyNSIOutputStream() override;
  virtual bool Next(void** data, int* size) override;
  virtual void BackUp(int count) override;
  virtual ::google::protobuf::int64 ByteCount() const override;
};

} 
} 

#endif 
