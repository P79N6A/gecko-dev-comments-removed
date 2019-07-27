



#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>
#include "Logging.h"
#include "Zip.h"

already_AddRefed<Zip>
Zip::Create(const char *filename)
{
  
  AutoCloseFD fd(open(filename, O_RDONLY));
  if (fd == -1) {
    ERROR("Error opening %s: %s", filename, strerror(errno));
    return nullptr;
  }
  struct stat st;
  if (fstat(fd, &st) == -1) {
    ERROR("Error stating %s: %s", filename, strerror(errno));
    return nullptr;
  }
  size_t size = st.st_size;
  if (size <= sizeof(CentralDirectoryEnd)) {
    ERROR("Error reading %s: too short", filename);
    return nullptr;
  }
  void *mapped = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
  if (mapped == MAP_FAILED) {
    ERROR("Error mmapping %s: %s", filename, strerror(errno));
    return nullptr;
  }
  DEBUG_LOG("Mapped %s @%p", filename, mapped);

  return Create(filename, mapped, size);
}

already_AddRefed<Zip>
Zip::Create(const char *filename, void *mapped, size_t size)
{
  mozilla::RefPtr<Zip> zip = new Zip(filename, mapped, size);

  
  
  if (!zip->nextFile && !zip->entries) {
    ERROR("%s - Invalid zip", filename);
    return nullptr;
  }

  ZipCollection::Singleton.Register(zip);
  return zip.forget();
}

Zip::Zip(const char *filename, void *mapped, size_t size)
: name(filename ? strdup(filename) : nullptr)
, mapped(mapped)
, size(size)
, nextFile(LocalFile::validate(mapped)) 
, nextDir(nullptr)
, entries(nullptr)
{
  
  
  if (!nextFile)
    GetFirstEntry();
}

Zip::~Zip()
{
  ZipCollection::Forget(this);
  if (name) {
    munmap(mapped, size);
    DEBUG_LOG("Unmapped %s @%p", name, mapped);
    free(name);
  }
}

bool
Zip::GetStream(const char *path, Zip::Stream *out) const
{
  DEBUG_LOG("%s - GetFile %s", name, path);
  









  if (nextFile && nextFile->GetName().Equals(path) &&
      !entries && (nextFile->compressedSize != 0)) {
    DEBUG_LOG("%s - %s was next file: fast path", name, path);
    
    const char *data = reinterpret_cast<const char *>(nextFile->GetData());
    out->compressedBuf = data;
    out->compressedSize = nextFile->compressedSize;
    out->uncompressedSize = nextFile->uncompressedSize;
    out->type = static_cast<Stream::Type>(uint16_t(nextFile->compression));

    


    data += nextFile->compressedSize;
    if ((nextFile->generalFlag & 0x8) && DataDescriptor::validate(data)) {
      data += sizeof(DataDescriptor);
    }
    nextFile = LocalFile::validate(data);
    return true;
  }

  

  if (!nextDir || !nextDir->GetName().Equals(path)) {
    const DirectoryEntry *entry = GetFirstEntry();
    DEBUG_LOG("%s - Scan directory entries in search for %s", name, path);
    while (entry && !entry->GetName().Equals(path)) {
      entry = entry->GetNext();
    }
    nextDir = entry;
  }
  if (!nextDir) {
    DEBUG_LOG("%s - Couldn't find %s", name, path);
    return false;
  }

  

  nextFile = LocalFile::validate(static_cast<const char *>(mapped)
                             + nextDir->offset);
  if (!nextFile) {
    ERROR("%s - Couldn't find the Local File header for %s", name, path);
    return false;
  }

  
  const char *data = reinterpret_cast<const char *>(nextFile->GetData());
  out->compressedBuf = data;
  out->compressedSize = nextDir->compressedSize;
  out->uncompressedSize = nextDir->uncompressedSize;
  out->type = static_cast<Stream::Type>(uint16_t(nextDir->compression));

  
  nextDir = nextDir->GetNext();
  nextFile = nullptr;
  return true;
}

const Zip::DirectoryEntry *
Zip::GetFirstEntry() const
{
  if (entries)
    return entries;

  const CentralDirectoryEnd *end = nullptr;
  const char *_end = static_cast<const char *>(mapped) + size
                     - sizeof(CentralDirectoryEnd);

  
  for (; _end > mapped && !end; _end--)
    end = CentralDirectoryEnd::validate(_end);
  if (!end) {
    ERROR("%s - Couldn't find end of central directory record", name);
    return nullptr;
  }

  entries = DirectoryEntry::validate(static_cast<const char *>(mapped)
                                 + end->offset);
  if (!entries) {
    ERROR("%s - Couldn't find central directory record", name);
  }
  return entries;
}

ZipCollection ZipCollection::Singleton;

already_AddRefed<Zip>
ZipCollection::GetZip(const char *path)
{
  
  for (std::vector<Zip *>::iterator it = Singleton.zips.begin();
       it < Singleton.zips.end(); ++it) {
    if ((*it)->GetName() && (strcmp((*it)->GetName(), path) == 0)) {
      mozilla::RefPtr<Zip> zip = *it;
      return zip.forget();
    }
  }
  return Zip::Create(path);
}

void
ZipCollection::Register(Zip *zip)
{
  Singleton.zips.push_back(zip);
}

void
ZipCollection::Forget(Zip *zip)
{
  DEBUG_LOG("ZipCollection::Forget(\"%s\")", zip->GetName());
  std::vector<Zip *>::iterator it = std::find(Singleton.zips.begin(),
                                              Singleton.zips.end(), zip);
  if (*it == zip) {
    Singleton.zips.erase(it);
  } else {
    DEBUG_LOG("ZipCollection::Forget: didn't find \"%s\" in bookkeeping", zip->GetName());
  }
}
