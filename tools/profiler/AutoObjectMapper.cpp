





#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "mozilla/Assertions.h"

#include "PlatformMacros.h"
#include "AutoObjectMapper.h"

#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)
# include <dlfcn.h>
# include "mozilla/Types.h"
  
  
  
  extern "C" {
    MFBT_API size_t
    __dl_get_mappable_length(void *handle);
    MFBT_API void *
    __dl_mmap(void *handle, void *addr, size_t length, off_t offset);
    MFBT_API void
    __dl_munmap(void *handle, void *addr, size_t length);
  }
  
# include "nsString.h"
# include "nsDirectoryServiceUtils.h"
# include "nsDirectoryServiceDefs.h"
#endif




static void
failedToMessage(void(*aLog)(const char*),
                const char* aHowFailed, std::string aFileName)
{
  char buf[300];
  snprintf(buf, sizeof(buf), "AutoObjectMapper::Map: Failed to %s \'%s\'",
           aHowFailed, aFileName.c_str());
  buf[sizeof(buf)-1] = 0;
  aLog(buf);
}


AutoObjectMapperPOSIX::AutoObjectMapperPOSIX(void(*aLog)(const char*))
  : mImage(nullptr)
  , mSize(0)
  , mLog(aLog)
  , mIsMapped(false)
{}

AutoObjectMapperPOSIX::~AutoObjectMapperPOSIX() {
  if (!mIsMapped) {
    
    MOZ_ASSERT(!mImage);
    MOZ_ASSERT(mSize == 0);
    return;
  }
  MOZ_ASSERT(mSize > 0);
  
  
  
  MOZ_ASSERT(mImage);
  munmap(mImage, mSize);
}

bool AutoObjectMapperPOSIX::Map(void** start, size_t* length,
                                std::string fileName)
{
  MOZ_ASSERT(!mIsMapped);

  int fd = open(fileName.c_str(), O_RDONLY);
  if (fd == -1) {
    failedToMessage(mLog, "open", fileName);
    return false;
  }

  struct stat st;
  int    err = fstat(fd, &st);
  size_t sz  = (err == 0) ? st.st_size : 0;
  if (err != 0 || sz == 0) {
    failedToMessage(mLog, "fstat", fileName);
    close(fd);
    return false;
  }

  void* image = mmap(nullptr, sz, PROT_READ, MAP_SHARED, fd, 0);
  if (image == MAP_FAILED) {
    failedToMessage(mLog, "mmap", fileName);
    close(fd);
    return false;
  }

  close(fd);
  mIsMapped = true;
  mImage = *start  = image;
  mSize  = *length = sz;
  return true;
}


#if defined(SPS_OS_android) && !defined(MOZ_WIDGET_GONK)




static char*
get_installation_lib_dir()
{
  nsCOMPtr<nsIProperties>
    directoryService(do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID));
  if (!directoryService) {
    return nullptr;
  }
  nsCOMPtr<nsIFile> greDir;
  nsresult rv = directoryService->Get(NS_GRE_DIR, NS_GET_IID(nsIFile),
                                      getter_AddRefs(greDir));
  if (NS_FAILED(rv)) return nullptr;
  nsCString path;
  rv = greDir->GetNativePath(path);
  if (NS_FAILED(rv)) {
    return nullptr;
  }
  return strdup(path.get());
}

AutoObjectMapperFaultyLib::AutoObjectMapperFaultyLib(void(*aLog)(const char*))
  : AutoObjectMapperPOSIX(aLog)
  , mHdl(nullptr)
{}

AutoObjectMapperFaultyLib::~AutoObjectMapperFaultyLib() {
  if (mHdl) {
    
    MOZ_ASSERT(mSize > 0);
    
    MOZ_ASSERT(mImage);
    __dl_munmap(mHdl, mImage, mSize);
    dlclose(mHdl);
    
    mImage = nullptr;
    mSize  = 0;
  }
  
  
  
  
  
}

bool AutoObjectMapperFaultyLib::Map(void** start, size_t* length,
                                    std::string fileName)
{
  MOZ_ASSERT(!mHdl);

  if (fileName == "libmozglue.so") {

    
    char* libdir = get_installation_lib_dir();
    if (libdir) {
      fileName = std::string(libdir) + "/lib/" + fileName;
      free(libdir);
    }
    
    return AutoObjectMapperPOSIX::Map(start, length, fileName);

  } else {

    
    
    void* hdl = dlopen(fileName.c_str(), RTLD_GLOBAL | RTLD_LAZY);
    if (!hdl) {
      failedToMessage(mLog, "get handle for ELF file", fileName);
      return false;
    }

    size_t sz = __dl_get_mappable_length(hdl);
    if (sz == 0) {
      dlclose(hdl);
      failedToMessage(mLog, "get size for ELF file", fileName);
      return false;
    }

    void* image = __dl_mmap(hdl, nullptr, sz, 0);
    if (image == MAP_FAILED) {
      dlclose(hdl);
      failedToMessage(mLog, "mmap ELF file", fileName);
      return false;
    }

    mHdl   = hdl;
    mImage = *start  = image;
    mSize  = *length = sz;
    return true;
  }
}

#endif 
