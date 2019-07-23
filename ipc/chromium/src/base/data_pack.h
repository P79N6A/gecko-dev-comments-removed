







#ifndef BASE_DATA_PACK_H_
#define BASE_DATA_PACK_H_

#include "base/basictypes.h"
#include "base/scoped_ptr.h"

namespace file_util {
  class MemoryMappedFile;
}
class FilePath;
class StringPiece;

namespace base {

class DataPack {
 public:
  DataPack();
  ~DataPack();

  
  bool Load(const FilePath& path);

  
  
  
  bool Get(uint32_t resource_id, StringPiece* data);

 private:
  
  scoped_ptr<file_util::MemoryMappedFile> mmap_;

  
  size_t resource_count_;

  DISALLOW_COPY_AND_ASSIGN(DataPack);
};

}  

#endif  
