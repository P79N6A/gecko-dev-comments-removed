



#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <cstdlib>
#include <algorithm>
#include "Logging.h"
#include "Zip.h"

mozilla::TemporaryRef<Zip>
Zip::Create(const char *filename)
{
  
  AutoCloseFD fd(open(filename, O_RDONLY));
  if (fd == -1) {
    log("Error opening %s: %s", filename, strerror(errno));
    return NULL;
  }
  struct stat st;
  if (fstat(fd, &st) == -1) {
    log("Error stating %s: %s", filename, strerror(errno));
    return NULL;
  }
  size_t size = st.st_size;
  if (size <= sizeof(CentralDirectoryEnd)) {
    log("Error reading %s: too short", filename);
    return NULL;
  }
  void *mapped = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  if (mapped == MAP_FAILED) {
    log("Error mmapping %s: %s", filename, strerror(errno));
    return NULL;
  }
  debug("Mapped %s @%p", filename, mapped);

  return Create(filename, mapped, size);
}

mozilla::TemporaryRef<Zip>
Zip::Create(const char *filename, void *mapped, size_t size)
{
  mozilla::RefPtr<Zip> zip = new Zip(filename, mapped, size);

  
  
  if (!zip->nextFile && !zip->entries) {
    log("%s - Invalid zip", filename);
    return NULL;
  }

  ZipCollection::Singleton.Register(zip);
  return zip;
}

Zip::Zip(const char *filename, void *mapped, size_t size)
: name(filename ? strdup(filename) : NULL)
, mapped(mapped)
, size(size)
, nextFile(LocalFile::validate(mapped)) 
, nextDir(NULL)
, entries(NULL)
{
  
  
  if (!nextFile)
    GetFirstEntry();
}

Zip::~Zip()
{
  ZipCollection::Forget(this);
  if (name) {
    munmap(mapped, size);
    debug("Unmapped %s @%p", name, mapped);
    free(name);
  }
}

bool
Zip::GetStream(const char *path, Zip::Stream *out) const
{
  debug("%s - GetFile %s", name, path);
  









  if (nextFile && nextFile->GetName().Equals(path) &&
      !entries && (nextFile->compressedSize != 0)) {
    debug("%s - %s was next file: fast path", name, path);
    
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
    debug("%s - Scan directory entries in search for %s", name, path);
    while (entry && !entry->GetName().Equals(path)) {
      entry = entry->GetNext();
    }
    nextDir = entry;
  }
  if (!nextDir) {
    debug("%s - Couldn't find %s", name, path);
    return false;
  }

  

  nextFile = LocalFile::validate(static_cast<const char *>(mapped)
                             + nextDir->offset);
  if (!nextFile) {
    log("%s - Couldn't find the Local File header for %s", name, path);
    return false;
  }

  
  const char *data = reinterpret_cast<const char *>(nextFile->GetData());
  out->compressedBuf = data;
  out->compressedSize = nextDir->compressedSize;
  out->uncompressedSize = nextDir->uncompressedSize;
  out->type = static_cast<Stream::Type>(uint16_t(nextDir->compression));

  
  nextDir = nextDir->GetNext();
  nextFile = NULL;
  return true;
}

const Zip::DirectoryEntry *
Zip::GetFirstEntry() const
{
  if (entries)
    return entries;

  const CentralDirectoryEnd *end = NULL;
  const char *_end = static_cast<const char *>(mapped) + size
                     - sizeof(CentralDirectoryEnd);

  
  for (; _end > mapped && !end; _end--)
    end = CentralDirectoryEnd::validate(_end);
  if (!end) {
    log("%s - Couldn't find end of central directory record", name);
    return NULL;
  }

  entries = DirectoryEntry::validate(static_cast<const char *>(mapped)
                                 + end->offset);
  if (!entries) {
    log("%s - Couldn't find central directory record", name);
  }
  return entries;
}

ZipCollection ZipCollection::Singleton;

mozilla::TemporaryRef<Zip>
ZipCollection::GetZip(const char *path)
{
  
  for (std::vector<Zip *>::iterator it = Singleton.zips.begin();
       it < Singleton.zips.end(); ++it) {
    if ((*it)->GetName() && (strcmp((*it)->GetName(), path) == 0))
      return *it;
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
  debug("ZipCollection::Forget(\"%s\")", zip->GetName());
  std::vector<Zip *>::iterator it = std::find(Singleton.zips.begin(),
                                              Singleton.zips.end(), zip);
  if (*it == zip) {
    Singleton.zips.erase(it);
  } else {
    debug("ZipCollection::Forget: didn't find \"%s\" in bookkeeping", zip->GetName());
  }
}
