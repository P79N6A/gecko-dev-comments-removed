



#include "base/data_pack.h"

#include "base/file_util.h"
#include "base/logging.h"
#include "base/string_piece.h"




namespace {
static const uint32_t kFileFormatVersion = 1;

static const size_t kHeaderLength = 2 * sizeof(uint32_t);

struct DataPackEntry {
  uint32_t resource_id;
  uint32_t file_offset;
  uint32_t length;

  static int CompareById(const void* void_key, const void* void_entry) {
    uint32_t key = *reinterpret_cast<const uint32_t*>(void_key);
    const DataPackEntry* entry =
        reinterpret_cast<const DataPackEntry*>(void_entry);
    if (key < entry->resource_id) {
      return -1;
    } else if (key > entry->resource_id) {
      return 1;
    } else {
      return 0;
    }
  }
}  __attribute((packed));

}  

namespace base {


DataPack::DataPack() : resource_count_(0) {
}
DataPack::~DataPack() {
}

bool DataPack::Load(const FilePath& path) {
  mmap_.reset(new file_util::MemoryMappedFile);
  if (!mmap_->Initialize(path)) {
    mmap_.reset();
    return false;
  }

  
  
  const uint32* ptr = reinterpret_cast<const uint32_t*>(mmap_->data());
  uint32 version = ptr[0];
  if (version != kFileFormatVersion) {
    LOG(ERROR) << "Bad data pack version: got " << version << ", expected "
               << kFileFormatVersion;
    mmap_.reset();
    return false;
  }
  resource_count_ = ptr[1];

  
  
  if (kHeaderLength + resource_count_ * sizeof(DataPackEntry) >
      mmap_->length()) {
    LOG(ERROR) << "Data pack file corruption: too short for number of "
                  "entries specified.";
    mmap_.reset();
    return false;
  }
  
  for (size_t i = 0; i < resource_count_; ++i) {
    const DataPackEntry* entry = reinterpret_cast<const DataPackEntry*>(
        mmap_->data() + kHeaderLength + (i * sizeof(DataPackEntry)));
    if (entry->file_offset + entry->length > mmap_->length()) {
      LOG(ERROR) << "Entry #" << i << " in data pack points off end of file. "
                 << "Was the file corrupted?";
      mmap_.reset();
      return false;
    }
  }

  return true;
}

bool DataPack::Get(uint32_t resource_id, StringPiece* data) {
  
  
#if defined(__BYTE_ORDER)
  
  COMPILE_ASSERT(__BYTE_ORDER == __LITTLE_ENDIAN,
                 datapack_assumes_little_endian);
#elif defined(__BIG_ENDIAN__)
  
  #error DataPack assumes little endian
#endif

  DataPackEntry* target = reinterpret_cast<DataPackEntry*>(
      bsearch(&resource_id, mmap_->data() + kHeaderLength, resource_count_,
              sizeof(DataPackEntry), DataPackEntry::CompareById));
  if (!target) {
    LOG(ERROR) << "No resource found with id: " << resource_id;
    return false;
  }

  data->set(mmap_->data() + target->file_offset, target->length);
  return true;
}

}  
