











#define READTYPE  int32_t
#include "zlib.h"
#include "nsISupportsUtils.h"
#include "prio.h"
#include "plstr.h"
#include "prlog.h"
#include "stdlib.h"
#include "nsWildCard.h"
#include "nsZipArchive.h"
#include "nsString.h"
#include "prenv.h"
#if defined(XP_WIN)
#include <windows.h>
#endif


#include NEW_H
#define ZIP_ARENABLOCKSIZE (1*1024)

#ifdef XP_UNIX
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <limits.h>
    #include <unistd.h>
#elif defined(XP_WIN) || defined(XP_OS2)
    #include <io.h>
#endif

#ifdef __SYMBIAN32__
    #include <sys/syslimits.h>
#endif 


#ifndef XP_UNIX 
#  ifndef S_IFMT
#    define S_IFMT 0170000
#  endif
#  ifndef S_IFLNK
#    define S_IFLNK  0120000
#  endif
#  ifndef PATH_MAX
#    define PATH_MAX 1024
#  endif
#endif  

#ifdef XP_WIN
#include "private/pprio.h"  
#endif

using namespace mozilla;

static const uint32_t kMaxNameLength = PATH_MAX; 

static const uint16_t kSyntheticTime = 0;
static const uint16_t kSyntheticDate = (1 + (1 << 5) + (0 << 9));

static uint16_t xtoint(const uint8_t *ii);
static uint32_t xtolong(const uint8_t *ll);
static uint32_t HashName(const char* aName, uint16_t nameLen);
#ifdef XP_UNIX
static nsresult ResolveSymlink(const char *path);
#endif

class ZipArchiveLogger {
public:
  void Write(const nsACString &zip, const char *entry) const {
    if (!fd) {
      char *env = PR_GetEnv("MOZ_JAR_LOG_FILE");
      if (!env)
        return;

      nsCOMPtr<nsIFile> logFile;
      nsresult rv = NS_NewLocalFile(NS_ConvertUTF8toUTF16(env), false, getter_AddRefs(logFile));
      if (NS_FAILED(rv))
        return;

      
      logFile->Create(nsIFile::NORMAL_FILE_TYPE, 0644);

      PRFileDesc* file;
#ifdef XP_WIN
      
      
      
      nsAutoString path;
      logFile->GetPath(path);
      if (path.IsEmpty())
        return;
      HANDLE handle = CreateFileW(path.get(), FILE_APPEND_DATA, FILE_SHARE_WRITE,
                                  NULL, OPEN_ALWAYS, 0, NULL);
      if (handle == INVALID_HANDLE_VALUE)
        return;
      file = PR_ImportFile((PROsfd)handle);
      if (!file)
        return;
#else
      rv = logFile->OpenNSPRFileDesc(PR_WRONLY|PR_CREATE_FILE|PR_APPEND, 0644, &file);
      if (NS_FAILED(rv))
        return;
#endif
      fd = file;
    }
    nsCString buf(zip);
    buf.Append(" ");
    buf.Append(entry);
    buf.Append('\n');
    PR_Write(fd, buf.get(), buf.Length());
  }

  void AddRef() {
    MOZ_ASSERT(refCnt >= 0);
    ++refCnt;
  }

  void Release() {
    MOZ_ASSERT(refCnt > 0);
    if ((0 == --refCnt) && fd) {
      PR_Close(fd);
      fd = NULL;
    }
  }
private:
  int refCnt;
  mutable PRFileDesc *fd;
};

static ZipArchiveLogger zipLog;







nsresult gZlibInit(z_stream *zs)
{
  memset(zs, 0, sizeof(z_stream));
  int zerr = inflateInit2(zs, -MAX_WBITS);
  if (zerr != Z_OK) return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

nsZipHandle::nsZipHandle()
  : mFileData(nullptr)
  , mLen(0)
  , mMap(nullptr)
  , mRefCnt(0)
{
  MOZ_COUNT_CTOR(nsZipHandle);
}

NS_IMPL_THREADSAFE_ADDREF(nsZipHandle)
NS_IMPL_THREADSAFE_RELEASE(nsZipHandle)

nsresult nsZipHandle::Init(nsIFile *file, nsZipHandle **ret)
{
  mozilla::AutoFDClose fd;
  nsresult rv = file->OpenNSPRFileDesc(PR_RDONLY, 0000, &fd.rwget());
  if (NS_FAILED(rv))
    return rv;

  int64_t size = PR_Available64(fd);
  if (size >= INT32_MAX)
    return NS_ERROR_FILE_TOO_BIG;

  PRFileMap *map = PR_CreateFileMap(fd, size, PR_PROT_READONLY);
  if (!map)
    return NS_ERROR_FAILURE;
  
  uint8_t *buf = (uint8_t*) PR_MemMap(map, 0, (uint32_t) size);
  
  if (!buf) {
    PR_CloseFileMap(map);
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsZipHandle> handle = new nsZipHandle();
  if (!handle) {
    PR_MemUnmap(buf, (uint32_t) size);
    PR_CloseFileMap(map);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  handle->mMap = map;
  handle->mFile.Init(file);
  handle->mLen = (uint32_t) size;
  handle->mFileData = buf;
  *ret = handle.forget().get();
  return NS_OK;
}

nsresult nsZipHandle::Init(nsZipArchive *zip, const char *entry,
                           nsZipHandle **ret)
{
  nsRefPtr<nsZipHandle> handle = new nsZipHandle();
  if (!handle)
    return NS_ERROR_OUT_OF_MEMORY;

  handle->mBuf = new nsZipItemPtr<uint8_t>(zip, entry);
  if (!handle->mBuf)
    return NS_ERROR_OUT_OF_MEMORY;

  if (!handle->mBuf->Buffer())
    return NS_ERROR_UNEXPECTED;

  handle->mMap = nullptr;
  handle->mFile.Init(zip, entry);
  handle->mLen = handle->mBuf->Length();
  handle->mFileData = handle->mBuf->Buffer();
  *ret = handle.forget().get();
  return NS_OK;
}

int64_t nsZipHandle::SizeOfMapping()
{
    return mLen;
}

nsZipHandle::~nsZipHandle()
{
  if (mMap) {
    PR_MemUnmap((void *)mFileData, mLen);
    PR_CloseFileMap(mMap);
  }
  mFileData = nullptr;
  mMap = nullptr;
  mBuf = nullptr;
  MOZ_COUNT_DTOR(nsZipHandle);
}








nsresult nsZipArchive::OpenArchive(nsZipHandle *aZipHandle)
{
  mFd = aZipHandle;

  
  PL_INIT_ARENA_POOL(&mArena, "ZipArena", ZIP_ARENABLOCKSIZE);

  
  nsresult rv = BuildFileList();
  if (NS_SUCCEEDED(rv)) {
    zipLog.AddRef();
    if (aZipHandle->mFile)
      aZipHandle->mFile.GetURIString(mURI);
  }
  return rv;
}

nsresult nsZipArchive::OpenArchive(nsIFile *aFile)
{
  nsRefPtr<nsZipHandle> handle;
  nsresult rv = nsZipHandle::Init(aFile, getter_AddRefs(handle));
  if (NS_FAILED(rv))
    return rv;

  return OpenArchive(handle);
}




nsresult nsZipArchive::Test(const char *aEntryName)
{
  nsZipItem* currItem;

  if (aEntryName) 
  {
    currItem = GetItem(aEntryName);
    if (!currItem)
      return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
    
    if (currItem->IsDirectory())
      return NS_OK;
    return ExtractFile(currItem, 0, 0);
  }

  
  for (int i = 0; i < ZIP_TABSIZE; i++) {
    for (currItem = mFiles[i]; currItem; currItem = currItem->next) {
      
      if (currItem->IsDirectory())
        continue;
      nsresult rv = ExtractFile(currItem, 0, 0);
      if (rv != NS_OK)
        return rv;
    }
  }

  return NS_OK;
}




nsresult nsZipArchive::CloseArchive()
{
  if (mFd) {
    PL_FinishArenaPool(&mArena);
    mFd = NULL;
  }

  
  
  
  
  
  
  
  memset(mFiles, 0, sizeof(mFiles));
  mBuiltSynthetics = false;
  zipLog.Release();
  return NS_OK;
}




nsZipItem*  nsZipArchive::GetItem(const char * aEntryName)
{
  if (aEntryName) {
    uint32_t len = strlen(aEntryName);
    
    
    if (!mBuiltSynthetics) {
        if ((len > 0) && (aEntryName[len-1] == '/')) {
            if (BuildSynthetics() != NS_OK)
                return 0;
        }
    }
MOZ_WIN_MEM_TRY_BEGIN
    nsZipItem* item = mFiles[ HashName(aEntryName, len) ];
    while (item) {
      if ((len == item->nameLength) && 
          (!memcmp(aEntryName, item->Name(), len))) {
        
        
        zipLog.Write(mURI, aEntryName);
        return item; 
      }
      item = item->next;
    }
MOZ_WIN_MEM_TRY_CATCH(return nullptr)
  }
  return nullptr;
}








nsresult nsZipArchive::ExtractFile(nsZipItem *item, const char *outname,
                                   PRFileDesc* aFd)
{
  if (!item)
    return NS_ERROR_ILLEGAL_VALUE;
  if (!mFd)
    return NS_ERROR_FAILURE;

  
  
  PR_ASSERT(!item->IsDirectory());

  Bytef outbuf[ZIP_BUFLEN];

  nsZipCursor cursor(item, this, outbuf, ZIP_BUFLEN, true);

  nsresult rv = NS_OK;

  while (true) {
    uint32_t count = 0;
    uint8_t* buf = cursor.Read(&count);
    if (!buf) {
      rv = NS_ERROR_FILE_CORRUPTED;
      break;
    } else if (count == 0) {
      break;
    }

    if (aFd && PR_Write(aFd, buf, count) < (READTYPE)count) {
      rv = NS_ERROR_FILE_DISK_FULL;
      break;
    }
  }

  
  if (aFd) {
    PR_Close(aFd);
    if (rv != NS_OK)
      PR_Delete(outname);
#ifdef XP_UNIX
    else if (item->IsSymlink())
      rv = ResolveSymlink(outname);
#endif
  }

  return rv;
}




nsresult
nsZipArchive::FindInit(const char * aPattern, nsZipFind **aFind)
{
  if (!aFind)
    return NS_ERROR_ILLEGAL_VALUE;

  
  *aFind = NULL;

  bool    regExp = false;
  char*   pattern = 0;

  
  nsresult rv = BuildSynthetics();
  if (rv != NS_OK)
    return rv;

  
  if (aPattern)
  {
    switch (NS_WildCardValid((char*)aPattern))
    {
      case INVALID_SXP:
        return NS_ERROR_ILLEGAL_VALUE;

      case NON_SXP:
        regExp = false;
        break;

      case VALID_SXP:
        regExp = true;
        break;

      default:
        
        PR_ASSERT(false);
        return NS_ERROR_ILLEGAL_VALUE;
    }

    pattern = PL_strdup(aPattern);
    if (!pattern)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  *aFind = new nsZipFind(this, pattern, regExp);
  if (!*aFind) {
    PL_strfree(pattern);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}






nsresult nsZipFind::FindNext(const char ** aResult, uint16_t *aNameLen)
{
  if (!mArchive || !aResult || !aNameLen)
    return NS_ERROR_ILLEGAL_VALUE;

  *aResult = 0;
  *aNameLen = 0;
MOZ_WIN_MEM_TRY_BEGIN
  
  while (mSlot < ZIP_TABSIZE)
  {
    
    mItem = mItem ? mItem->next : mArchive->mFiles[mSlot];

    bool found = false;
    if (!mItem)
      ++mSlot;                          
    else if (!mPattern)
      found = true;            
    else if (mRegExp)
    {
      char buf[kMaxNameLength+1];
      memcpy(buf, mItem->Name(), mItem->nameLength);
      buf[mItem->nameLength]='\0';
      found = (NS_WildCardMatch(buf, mPattern, false) == MATCH);
    }
    else
      found = ((mItem->nameLength == strlen(mPattern)) &&
               (memcmp(mItem->Name(), mPattern, mItem->nameLength) == 0));
    if (found) {
      
      *aResult = mItem->Name();
      *aNameLen = mItem->nameLength;
      return NS_OK;
    }
  }
MOZ_WIN_MEM_TRY_CATCH(return NS_ERROR_FAILURE)
  return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
}

#ifdef XP_UNIX



static nsresult ResolveSymlink(const char *path)
{
  PRFileDesc * fIn = PR_Open(path, PR_RDONLY, 0000);
  if (!fIn)
    return NS_ERROR_FILE_DISK_FULL;

  char buf[PATH_MAX+1];
  int32_t length = PR_Read(fIn, (void*)buf, PATH_MAX);
  PR_Close(fIn);

  if ( (length <= 0)
    || ((buf[length] = 0, PR_Delete(path)) != 0)
    || (symlink(buf, path) != 0))
  {
     return NS_ERROR_FILE_DISK_FULL;
  }
  return NS_OK;
}
#endif








nsZipItem* nsZipArchive::CreateZipItem()
{
  
  void *mem;
  PL_ARENA_ALLOCATE(mem, &mArena, sizeof(nsZipItem));
  return (nsZipItem*)mem;
}




nsresult nsZipArchive::BuildFileList()
{
  
  const uint8_t* buf;
  const uint8_t* startp = mFd->mFileData;
  const uint8_t* endp = startp + mFd->mLen;
MOZ_WIN_MEM_TRY_BEGIN
  uint32_t centralOffset = 4;
  if (mFd->mLen > ZIPCENTRAL_SIZE && xtolong(startp + centralOffset) == CENTRALSIG) {
    
  } else {
    for (buf = endp - ZIPEND_SIZE; buf > startp; buf--)
      {
        if (xtolong(buf) == ENDSIG) {
          centralOffset = xtolong(((ZipEnd *)buf)->offset_central_dir);
          break;
        }
      }
  }

  if (!centralOffset)
    return NS_ERROR_FILE_CORRUPTED;

  
  buf = startp + centralOffset;
  uint32_t sig = 0;
  while (buf + int32_t(sizeof(uint32_t)) <= endp &&
         (sig = xtolong(buf)) == CENTRALSIG) {
    
    if (endp - buf < ZIPCENTRAL_SIZE)
      return NS_ERROR_FILE_CORRUPTED;

    
    ZipCentral* central = (ZipCentral*)buf;

    uint16_t namelen = xtoint(central->filename_len);
    uint16_t extralen = xtoint(central->extrafield_len);
    uint16_t commentlen = xtoint(central->commentfield_len);

    
    buf += ZIPCENTRAL_SIZE + namelen + extralen + commentlen;

    
    
    if (namelen < 1 ||
        namelen > kMaxNameLength ||
        buf >= endp) {
      return NS_ERROR_FILE_CORRUPTED;
    }

    nsZipItem* item = CreateZipItem();
    if (!item)
      return NS_ERROR_OUT_OF_MEMORY;

    item->central = central;
    item->nameLength = namelen;
    item->isSynthetic = false;

    
    uint32_t hash = HashName(item->Name(), namelen);
    item->next = mFiles[hash];
    mFiles[hash] = item;

    sig = 0;
  } 

  if (sig != ENDSIG)
    return NS_ERROR_FILE_CORRUPTED;

  
  if (endp - buf >= ZIPEND_SIZE) {
    ZipEnd *zipend = (ZipEnd *)buf;

    buf += ZIPEND_SIZE;
    uint16_t commentlen = xtoint(zipend->commentfield_len);
    if (endp - buf >= commentlen) {
      mCommentPtr = (const char *)buf;
      mCommentLen = commentlen;
    }
  }

MOZ_WIN_MEM_TRY_CATCH(return NS_ERROR_FAILURE)
  return NS_OK;
}




nsresult nsZipArchive::BuildSynthetics()
{
  if (mBuiltSynthetics)
    return NS_OK;
  mBuiltSynthetics = true;

MOZ_WIN_MEM_TRY_BEGIN
  
  
  for (int i = 0; i < ZIP_TABSIZE; ++i)
  {
    for (nsZipItem* item = mFiles[i]; item != nullptr; item = item->next)
    {
      if (item->isSynthetic)
        continue;
    
      
      
      
      
      
      
      uint16_t namelen = item->nameLength;
      MOZ_ASSERT(namelen > 0, "Attempt to build synthetic for zero-length entry name!");
      const char *name = item->Name();
      for (uint16_t dirlen = namelen - 1; dirlen > 0; dirlen--)
      {
        if (name[dirlen-1] != '/')
          continue;

        
        
        if (name[dirlen] == '/')
          continue;

        
        uint32_t hash = HashName(item->Name(), dirlen);
        bool found = false;
        for (nsZipItem* zi = mFiles[hash]; zi != NULL; zi = zi->next)
        {
          if ((dirlen == zi->nameLength) &&
              (0 == memcmp(item->Name(), zi->Name(), dirlen)))
          {
            
            found = true;
            break;
          }
        }
        
        
        
        if (found)
          break;

        nsZipItem* diritem = CreateZipItem();
        if (!diritem)
          return NS_ERROR_OUT_OF_MEMORY;

        
        diritem->central =  item->central;
        diritem->nameLength = dirlen;
        diritem->isSynthetic = true;

        
        diritem->next = mFiles[hash];
        mFiles[hash] = diritem;
      } 
    }
  }
MOZ_WIN_MEM_TRY_CATCH(return NS_ERROR_FAILURE)
  return NS_OK;
}

nsZipHandle* nsZipArchive::GetFD()
{
  if (!mFd)
    return NULL;
  return mFd.get();
}




const uint8_t* nsZipArchive::GetData(nsZipItem* aItem)
{
  PR_ASSERT (aItem);
MOZ_WIN_MEM_TRY_BEGIN
  
  
  uint32_t len = mFd->mLen;
  const uint8_t* data = mFd->mFileData;
  uint32_t offset = aItem->LocalOffset();
  if (offset + ZIPLOCAL_SIZE > len)
    return nullptr;

  
  ZipLocal* Local = (ZipLocal*)(data + offset);
  if ((xtolong(Local->signature) != LOCALSIG))
    return nullptr;

  
  
  
  offset += ZIPLOCAL_SIZE +
            xtoint(Local->filename_len) +
            xtoint(Local->extrafield_len);

  
  if (offset + aItem->Size() > len)
    return nullptr;

  return data + offset;
MOZ_WIN_MEM_TRY_CATCH(return nullptr)
}


bool nsZipArchive::GetComment(nsACString &aComment)
{
MOZ_WIN_MEM_TRY_BEGIN
  aComment.Assign(mCommentPtr, mCommentLen);
MOZ_WIN_MEM_TRY_CATCH(return false)
  return true;
}




int64_t nsZipArchive::SizeOfMapping()
{
    return mFd ? mFd->SizeOfMapping() : 0;
}





nsZipArchive::nsZipArchive()
  : mRefCnt(0)
  , mBuiltSynthetics(false)
{
  MOZ_COUNT_CTOR(nsZipArchive);

  
  memset(mFiles, 0, sizeof(mFiles));
}

NS_IMPL_THREADSAFE_ADDREF(nsZipArchive)
NS_IMPL_THREADSAFE_RELEASE(nsZipArchive)

nsZipArchive::~nsZipArchive()
{
  CloseArchive();

  MOZ_COUNT_DTOR(nsZipArchive);
}






nsZipFind::nsZipFind(nsZipArchive* aZip, char* aPattern, bool aRegExp) : 
  mArchive(aZip),
  mPattern(aPattern),
  mItem(0),
  mSlot(0),
  mRegExp(aRegExp)
{
  MOZ_COUNT_CTOR(nsZipFind);
}

nsZipFind::~nsZipFind()
{
  PL_strfree(mPattern);

  MOZ_COUNT_DTOR(nsZipFind);
}










static uint32_t HashName(const char* aName, uint16_t len)
{
  PR_ASSERT(aName != 0);

  const uint8_t* p = (const uint8_t*)aName;
  const uint8_t* endp = p + len;
  uint32_t val = 0;
  while (p != endp) {
    val = val*37 + *p++;
  }

  return (val % ZIP_TABSIZE);
}







static uint16_t xtoint (const uint8_t *ii)
{
  return (uint16_t) ((ii [0]) | (ii [1] << 8));
}







static uint32_t xtolong (const uint8_t *ll)
{
  return (uint32_t)( (ll [0] <<  0) |
                     (ll [1] <<  8) |
                     (ll [2] << 16) |
                     (ll [3] << 24) );
}






static PRTime GetModTime(uint16_t aDate, uint16_t aTime)
{
  
  
  PRExplodedTime time;

  time.tm_usec = 0;

  time.tm_hour = (aTime >> 11) & 0x1F;
  time.tm_min = (aTime >> 5) & 0x3F;
  time.tm_sec = (aTime & 0x1F) * 2;

  time.tm_year = (aDate >> 9) + 1980;
  time.tm_month = ((aDate >> 5) & 0x0F) - 1;
  time.tm_mday = aDate & 0x1F;

  time.tm_params.tp_gmt_offset = 0;
  time.tm_params.tp_dst_offset = 0;

  PR_NormalizeTime(&time, PR_GMTParameters);
  time.tm_params.tp_gmt_offset = PR_LocalTimeParameters(&time).tp_gmt_offset;
  PR_NormalizeTime(&time, PR_GMTParameters);
  time.tm_params.tp_dst_offset = PR_LocalTimeParameters(&time).tp_dst_offset;

  return PR_ImplodeTime(&time);
}

uint32_t nsZipItem::LocalOffset()
{
  return xtolong(central->localhdr_offset);
}

uint32_t nsZipItem::Size()
{
  return isSynthetic ? 0 : xtolong(central->size);
}

uint32_t nsZipItem::RealSize()
{
  return isSynthetic ? 0 : xtolong(central->orglen);
}

uint32_t nsZipItem::CRC32()
{
  return isSynthetic ? 0 : xtolong(central->crc32);
}

uint16_t nsZipItem::Date()
{
  return isSynthetic ? kSyntheticDate : xtoint(central->date);
}

uint16_t nsZipItem::Time()
{
  return isSynthetic ? kSyntheticTime : xtoint(central->time);
}

uint16_t nsZipItem::Compression()
{
  return isSynthetic ? STORED : xtoint(central->method);
}

bool nsZipItem::IsDirectory()
{
  return isSynthetic || ((nameLength > 0) && ('/' == Name()[nameLength - 1]));
}

uint16_t nsZipItem::Mode()
{
  if (isSynthetic) return 0755;
  return ((uint16_t)(central->external_attributes[2]) | 0x100);
}

const uint8_t * nsZipItem::GetExtraField(uint16_t aTag, uint16_t *aBlockSize)
{
  if (isSynthetic) return nullptr;
MOZ_WIN_MEM_TRY_BEGIN
  const unsigned char *buf = ((const unsigned char*)central) + ZIPCENTRAL_SIZE +
                             nameLength;
  uint32_t buflen = (uint32_t)xtoint(central->extrafield_len);
  uint32_t pos = 0;
  uint16_t tag, blocksize;

  while (buf && (pos + 4) <= buflen) {
    tag = xtoint(buf + pos);
    blocksize = xtoint(buf + pos + 2);

    if (aTag == tag && (pos + 4 + blocksize) <= buflen) {
      *aBlockSize = blocksize;
      return buf + pos;
    }

    pos += blocksize + 4;
  }

MOZ_WIN_MEM_TRY_CATCH(return nullptr)
  return nullptr;
}


PRTime nsZipItem::LastModTime()
{
  if (isSynthetic) return GetModTime(kSyntheticDate, kSyntheticTime);

  
  uint16_t blocksize;
  const uint8_t *tsField = GetExtraField(EXTENDED_TIMESTAMP_FIELD, &blocksize);
  if (tsField && blocksize >= 5 && tsField[4] & EXTENDED_TIMESTAMP_MODTIME) {
    return (PRTime)(xtolong(tsField + 5)) * PR_USEC_PER_SEC;
  }

  return GetModTime(Date(), Time());
}

#ifdef XP_UNIX
bool nsZipItem::IsSymlink()
{
  if (isSynthetic) return false;
  return (xtoint(central->external_attributes+2) & S_IFMT) == S_IFLNK;
}
#endif

nsZipCursor::nsZipCursor(nsZipItem *item, nsZipArchive *aZip, uint8_t* aBuf, uint32_t aBufSize, bool doCRC) :
  mItem(item),
  mBuf(aBuf),
  mBufSize(aBufSize),
  mDoCRC(doCRC)
{
  if (mItem->Compression() == DEFLATED) {
#ifdef DEBUG
    nsresult status =
#endif
      gZlibInit(&mZs);
    NS_ASSERTION(status == NS_OK, "Zlib failed to initialize");
    NS_ASSERTION(aBuf, "Must pass in a buffer for DEFLATED nsZipItem");
  }
  
  mZs.avail_in = item->Size();
  mZs.next_in = (Bytef*)aZip->GetData(item);
  
  if (doCRC)
    mCRC = crc32(0L, Z_NULL, 0);
}

nsZipCursor::~nsZipCursor()
{
  if (mItem->Compression() == DEFLATED) {
    inflateEnd(&mZs);
  }
}

uint8_t* nsZipCursor::ReadOrCopy(uint32_t *aBytesRead, bool aCopy) {
  int zerr;
  uint8_t *buf = nullptr;
  bool verifyCRC = true;

  if (!mZs.next_in)
    return nullptr;
MOZ_WIN_MEM_TRY_BEGIN
  switch (mItem->Compression()) {
  case STORED:
    if (!aCopy) {
      *aBytesRead = mZs.avail_in;
      buf = mZs.next_in;
      mZs.next_in += mZs.avail_in;
      mZs.avail_in = 0;
    } else {
      *aBytesRead = mZs.avail_in > mBufSize ? mBufSize : mZs.avail_in;
      memcpy(mBuf, mZs.next_in, *aBytesRead);
      mZs.avail_in -= *aBytesRead;
      mZs.next_in += *aBytesRead;
    }
    break;
  case DEFLATED:
    buf = mBuf;
    mZs.next_out = buf;
    mZs.avail_out = mBufSize;
    
    zerr = inflate(&mZs, Z_PARTIAL_FLUSH);
    if (zerr != Z_OK && zerr != Z_STREAM_END)
      return nullptr;
    
    *aBytesRead = mZs.next_out - buf;
    verifyCRC = (zerr == Z_STREAM_END);
    break;
  default:
    return nullptr;
  }

  if (mDoCRC) {
    mCRC = crc32(mCRC, (const unsigned char*)buf, *aBytesRead);
    if (verifyCRC && mCRC != mItem->CRC32())
      return nullptr;
  }
MOZ_WIN_MEM_TRY_CATCH(return nullptr)
  return buf;
}

nsZipItemPtr_base::nsZipItemPtr_base(nsZipArchive *aZip, const char * aEntryName, bool doCRC) :
  mReturnBuf(nullptr)
{
  
  mZipHandle = aZip->GetFD();

  nsZipItem* item = aZip->GetItem(aEntryName);
  if (!item)
    return;

  uint32_t size = 0;
  if (item->Compression() == DEFLATED) {
    size = item->RealSize();
    mAutoBuf = new uint8_t[size];
  }

  nsZipCursor cursor(item, aZip, mAutoBuf, size, doCRC);
  mReturnBuf = cursor.Read(&mReadlen);
  if (!mReturnBuf) {
    return;
  }

  if (mReadlen != item->RealSize()) {
    NS_ASSERTION(mReadlen == item->RealSize(), "nsZipCursor underflow");
    mReturnBuf = nullptr;
    return;
  }
}
