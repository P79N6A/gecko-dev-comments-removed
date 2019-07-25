


















































#define READTYPE  PRInt32
#include "zlib.h"
#include "nsISupportsUtils.h"
#include "nsRecyclingAllocator.h"
#include "prio.h"
#include "plstr.h"
#include "prlog.h"
#include "stdlib.h"
#include "nsWildCard.h"
#include "nsZipArchive.h"
#include "nsString.h"
#include "mozilla/FunctionTimer.h"
#include "prenv.h"
#if defined(XP_WIN)
#include <windows.h>
#endif
#include "mozilla/Telemetry.h"




#define NBUCKETS 6
nsRecyclingAllocator *gZlibAllocator = NULL;


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


using namespace mozilla;

static const PRUint32 kMaxNameLength = PATH_MAX; 

static const PRUint16 kSyntheticTime = 0;
static const PRUint16 kSyntheticDate = (1 + (1 << 5) + (0 << 9));

static PRUint16 xtoint(const PRUint8 *ii);
static PRUint32 xtolong(const PRUint8 *ll);
static PRUint32 HashName(const char* aName, PRUint16 nameLen);
#ifdef XP_UNIX
static nsresult ResolveSymlink(const char *path);
#endif










static void *
zlibAlloc(void *opaque, uInt items, uInt size)
{
  nsRecyclingAllocator *zallocator = (nsRecyclingAllocator *)opaque;
  if (zallocator) {
    return gZlibAllocator->Malloc(items * size);
  }
  return malloc(items * size);
}

static void
zlibFree(void *opaque, void *ptr)
{
  nsRecyclingAllocator *zallocator = (nsRecyclingAllocator *)opaque;
  if (zallocator)
    zallocator->Free(ptr);
  else
    free(ptr);
}

nsresult gZlibInit(z_stream *zs)
{
  memset(zs, 0, sizeof(z_stream));
  
  if (!gZlibAllocator) {
    gZlibAllocator = new nsRecyclingAllocator(NBUCKETS, NS_DEFAULT_RECYCLE_TIMEOUT, "libjar");
  }
  if (gZlibAllocator) {
    zs->zalloc = zlibAlloc;
    zs->zfree = zlibFree;
    zs->opaque = gZlibAllocator;
  }
  int zerr = inflateInit2(zs, -MAX_WBITS);
  if (zerr != Z_OK) return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

nsZipHandle::nsZipHandle()
  : mFileData(nsnull)
  , mLen(0)
  , mMap(nsnull)
  , mRefCnt(0)
{
  MOZ_COUNT_CTOR(nsZipHandle);
}

NS_IMPL_THREADSAFE_ADDREF(nsZipHandle)
NS_IMPL_THREADSAFE_RELEASE(nsZipHandle)

nsresult nsZipHandle::Init(nsILocalFile *file, nsZipHandle **ret)
{
  mozilla::AutoFDClose fd;
  nsresult rv = file->OpenNSPRFileDesc(PR_RDONLY, 0000, &fd);
  if (NS_FAILED(rv))
    return rv;

  PRInt64 size = PR_Available64(fd);
  if (size >= PR_INT32_MAX)
    return NS_ERROR_FILE_TOO_BIG;

  PRFileMap *map = PR_CreateFileMap(fd, size, PR_PROT_READONLY);
  if (!map)
    return NS_ERROR_FAILURE;
  
  PRUint8 *buf = (PRUint8*) PR_MemMap(map, 0, (PRUint32) size);
  
  if (!buf) {
    PR_CloseFileMap(map);
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsZipHandle> handle = new nsZipHandle();
  if (!handle) {
    PR_MemUnmap(buf, (PRUint32) size);
    PR_CloseFileMap(map);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  handle->mMap = map;
  handle->mFile = file;
  handle->mLen = (PRUint32) size;
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

  handle->mBuf = new nsZipItemPtr<PRUint8>(zip, entry);
  if (!handle->mBuf)
    return NS_ERROR_OUT_OF_MEMORY;

  if (!handle->mBuf->Buffer())
    return NS_ERROR_UNEXPECTED;

  handle->mMap = nsnull;
  handle->mLen = handle->mBuf->Length();
  handle->mFileData = handle->mBuf->Buffer();
  *ret = handle.forget().get();
  return NS_OK;
}

nsZipHandle::~nsZipHandle()
{
  if (mMap) {
    PR_MemUnmap((void *)mFileData, mLen);
    PR_CloseFileMap(mMap);
  }
  mFileData = nsnull;
  mMap = nsnull;
  mBuf = nsnull;
  MOZ_COUNT_DTOR(nsZipHandle);
}








nsresult nsZipArchive::OpenArchive(nsZipHandle *aZipHandle)
{
  mFd = aZipHandle;

  
  PL_INIT_ARENA_POOL(&mArena, "ZipArena", ZIP_ARENABLOCKSIZE);

  
  nsresult rv = BuildFileList();
  char *env = PR_GetEnv("MOZ_JAR_LOG_DIR");
  if (env && NS_SUCCEEDED(rv) && aZipHandle->mFile) {
    nsCOMPtr<nsILocalFile> logFile;
    nsresult rv2 = NS_NewLocalFile(NS_ConvertUTF8toUTF16(env), PR_FALSE, getter_AddRefs(logFile));
    
    if (!NS_SUCCEEDED(rv2))
      return rv;

    
    logFile->Create(nsIFile::DIRECTORY_TYPE, 0700);

    nsAutoString name;
    aZipHandle->mFile->GetLeafName(name);
    name.Append(NS_LITERAL_STRING(".log"));
    logFile->Append(name);

    PRFileDesc* fd;
    rv2 = logFile->OpenNSPRFileDesc(PR_WRONLY|PR_CREATE_FILE|PR_APPEND, 0644, &fd);
    if (NS_SUCCEEDED(rv2))
      mLog = fd;
  }
  return rv;
}

nsresult nsZipArchive::OpenArchive(nsIFile *aFile)
{
  nsresult rv;
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(aFile, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsZipHandle> handle;
  rv = nsZipHandle::Init(localFile, getter_AddRefs(handle));
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
  return NS_OK;
}




nsZipItem*  nsZipArchive::GetItem(const char * aEntryName)
{
  if (aEntryName) {
    PRUint32 len = strlen(aEntryName);
    
    
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
        
        if (mLog) {
          
          char *tmp = PL_strdup(aEntryName);
          tmp[len]='\n';
          PR_Write(mLog, tmp, len+1);
          PL_strfree(tmp);
        }
        return item; 
      }
      item = item->next;
    }
MOZ_WIN_MEM_TRY_CATCH(return nsnull)
  }
  return nsnull;
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
    PRUint32 count = 0;
    PRUint8* buf = cursor.Read(&count);
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




PRInt32
nsZipArchive::FindInit(const char * aPattern, nsZipFind **aFind)
{
  if (!aFind)
    return NS_ERROR_ILLEGAL_VALUE;

  
  *aFind = NULL;

  PRBool  regExp = PR_FALSE;
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
        regExp = PR_FALSE;
        break;

      case VALID_SXP:
        regExp = PR_TRUE;
        break;

      default:
        
        PR_ASSERT(PR_FALSE);
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






nsresult nsZipFind::FindNext(const char ** aResult, PRUint16 *aNameLen)
{
  if (!mArchive || !aResult || !aNameLen)
    return NS_ERROR_ILLEGAL_VALUE;

  *aResult = 0;
  *aNameLen = 0;
MOZ_WIN_MEM_TRY_BEGIN
  
  while (mSlot < ZIP_TABSIZE)
  {
    
    mItem = mItem ? mItem->next : mArchive->mFiles[mSlot];

    PRBool found = PR_FALSE;
    if (!mItem)
      ++mSlot;                          
    else if (!mPattern)
      found = PR_TRUE;            
    else if (mRegExp)
    {
      char buf[kMaxNameLength+1];
      memcpy(buf, mItem->Name(), mItem->nameLength);
      buf[mItem->nameLength]='\0';
      found = (NS_WildCardMatch(buf, mPattern, PR_FALSE) == MATCH);
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
  PRInt32 length = PR_Read(fIn, (void*)buf, PATH_MAX);
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
#ifndef XP_WIN
  NS_TIME_FUNCTION;
#endif
  
  const PRUint8* buf;
  const PRUint8* startp = mFd->mFileData;
  const PRUint8* endp = startp + mFd->mLen;
MOZ_WIN_MEM_TRY_BEGIN
  PRUint32 centralOffset = 4;
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
  PRUint32 sig = 0;
  while (buf + PRInt32(sizeof(PRUint32)) <= endp &&
         (sig = xtolong(buf)) == CENTRALSIG) {
    
    if (endp - buf < ZIPCENTRAL_SIZE)
      return NS_ERROR_FILE_CORRUPTED;

    
    ZipCentral* central = (ZipCentral*)buf;

    PRUint16 namelen = xtoint(central->filename_len);
    PRUint16 extralen = xtoint(central->extrafield_len);
    PRUint16 commentlen = xtoint(central->commentfield_len);

    
    buf += ZIPCENTRAL_SIZE + namelen + extralen + commentlen;

    
    
    if (namelen > kMaxNameLength || buf >= endp)
      return NS_ERROR_FILE_CORRUPTED;

    nsZipItem* item = CreateZipItem();
    if (!item)
      return NS_ERROR_OUT_OF_MEMORY;

    item->central = central;
    item->nameLength = namelen;
    item->isSynthetic = false;

    
    PRUint32 hash = HashName(item->Name(), namelen);
    item->next = mFiles[hash];
    mFiles[hash] = item;

    sig = 0;
  } 

  if (sig != ENDSIG)
    return NS_ERROR_FILE_CORRUPTED;

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
    for (nsZipItem* item = mFiles[i]; item != 0; item = item->next)
    {
      if (item->isSynthetic)
        continue;
    
      
      
      
      
      
      
      PRUint16 namelen = item->nameLength;
      const char *name = item->Name();
      for (PRUint16 dirlen = namelen - 1; dirlen > 0; dirlen--)
      {
        if (name[dirlen-1] != '/')
          continue;

        
        PRUint32 hash = HashName(item->Name(), dirlen);
        PRBool found = PR_FALSE;
        for (nsZipItem* zi = mFiles[hash]; zi != NULL; zi = zi->next)
        {
          if ((dirlen == zi->nameLength) &&
              (0 == memcmp(item->Name(), zi->Name(), dirlen)))
          {
            
            found = PR_TRUE;
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




const PRUint8* nsZipArchive::GetData(nsZipItem* aItem)
{
  PR_ASSERT (aItem);
MOZ_WIN_MEM_TRY_BEGIN
  
  
  PRUint32 len = mFd->mLen;
  const PRUint8* data = mFd->mFileData;
  PRUint32 offset = aItem->LocalOffset();
  if (offset + ZIPLOCAL_SIZE > len)
    return nsnull;

  
  ZipLocal* Local = (ZipLocal*)(data + offset);
  if ((xtolong(Local->signature) != LOCALSIG))
    return nsnull;

  
  
  
  offset += ZIPLOCAL_SIZE +
            xtoint(Local->filename_len) +
            xtoint(Local->extrafield_len);

  
  if (offset + aItem->Size() > len)
    return nsnull;

  return data + offset;
MOZ_WIN_MEM_TRY_CATCH(return nsnull)
}





nsZipArchive::nsZipArchive() :
  mBuiltSynthetics(false)
{
  MOZ_COUNT_CTOR(nsZipArchive);

  
  memset(mFiles, 0, sizeof(mFiles));
}

nsZipArchive::~nsZipArchive()
{
  CloseArchive();

  MOZ_COUNT_DTOR(nsZipArchive);
}






nsZipFind::nsZipFind(nsZipArchive* aZip, char* aPattern, PRBool aRegExp) : 
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










static PRUint32 HashName(const char* aName, PRUint16 len)
{
  PR_ASSERT(aName != 0);

  const PRUint8* p = (const PRUint8*)aName;
  const PRUint8* endp = p + len;
  PRUint32 val = 0;
  while (p != endp) {
    val = val*37 + *p++;
  }

  return (val % ZIP_TABSIZE);
}







static PRUint16 xtoint (const PRUint8 *ii)
{
  return (PRUint16) ((ii [0]) | (ii [1] << 8));
}







static PRUint32 xtolong (const PRUint8 *ll)
{
  return (PRUint32)( (ll [0] <<  0) |
                     (ll [1] <<  8) |
                     (ll [2] << 16) |
                     (ll [3] << 24) );
}






static PRTime GetModTime(PRUint16 aDate, PRUint16 aTime)
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

PRUint32 nsZipItem::LocalOffset()
{
  return xtolong(central->localhdr_offset);
}

PRUint32 nsZipItem::Size()
{
  return isSynthetic ? 0 : xtolong(central->size);
}

PRUint32 nsZipItem::RealSize()
{
  return isSynthetic ? 0 : xtolong(central->orglen);
}

PRUint32 nsZipItem::CRC32()
{
  return isSynthetic ? 0 : xtolong(central->crc32);
}

PRUint16 nsZipItem::Date()
{
  return isSynthetic ? kSyntheticDate : xtoint(central->date);
}

PRUint16 nsZipItem::Time()
{
  return isSynthetic ? kSyntheticTime : xtoint(central->time);
}

PRUint16 nsZipItem::Compression()
{
  return isSynthetic ? STORED : xtoint(central->method);
}

bool nsZipItem::IsDirectory()
{
  return isSynthetic || ((nameLength > 0) && ('/' == Name()[nameLength - 1]));
}

PRUint16 nsZipItem::Mode()
{
  if (isSynthetic) return 0755;
  return ((PRUint16)(central->external_attributes[2]) | 0x100);
}

const PRUint8 * nsZipItem::GetExtraField(PRUint16 aTag, PRUint16 *aBlockSize)
{
  if (isSynthetic) return nsnull;
MOZ_WIN_MEM_TRY_BEGIN
  const unsigned char *buf = ((const unsigned char*)central) + ZIPCENTRAL_SIZE +
                             nameLength;
  PRUint32 buflen = (PRUint32)xtoint(central->extrafield_len);
  PRUint32 pos = 0;
  PRUint16 tag, blocksize;

  while (buf && (pos + 4) <= buflen) {
    tag = xtoint(buf + pos);
    blocksize = xtoint(buf + pos + 2);

    if (aTag == tag && (pos + 4 + blocksize) <= buflen) {
      *aBlockSize = blocksize;
      return buf + pos;
    }

    pos += blocksize + 4;
  }

MOZ_WIN_MEM_TRY_CATCH(return nsnull)
  return nsnull;
}


PRTime nsZipItem::LastModTime()
{
  if (isSynthetic) return GetModTime(kSyntheticDate, kSyntheticTime);

  
  PRUint16 blocksize;
  const PRUint8 *tsField = GetExtraField(EXTENDED_TIMESTAMP_FIELD, &blocksize);
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

nsZipCursor::nsZipCursor(nsZipItem *item, nsZipArchive *aZip, PRUint8* aBuf, PRUint32 aBufSize, bool doCRC) :
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

PRUint8* nsZipCursor::Read(PRUint32 *aBytesRead) {
  int zerr;
  PRUint8 *buf = nsnull;
  bool verifyCRC = true;

  if (!mZs.next_in)
    return nsnull;
MOZ_WIN_MEM_TRY_BEGIN
  switch (mItem->Compression()) {
  case STORED:
    *aBytesRead = mZs.avail_in;
    buf = mZs.next_in;
    mZs.next_in += mZs.avail_in;
    mZs.avail_in = 0;
    break;
  case DEFLATED:
    buf = mBuf;
    mZs.next_out = buf;
    mZs.avail_out = mBufSize;
    
    zerr = inflate(&mZs, Z_PARTIAL_FLUSH);
    if (zerr != Z_OK && zerr != Z_STREAM_END)
      return nsnull;
    
    *aBytesRead = mZs.next_out - buf;
    verifyCRC = (zerr == Z_STREAM_END);
    break;
  default:
    return nsnull;
  }

  if (mDoCRC) {
    mCRC = crc32(mCRC, (const unsigned char*)buf, *aBytesRead);
    if (verifyCRC && mCRC != mItem->CRC32())
      return nsnull;
  }
MOZ_WIN_MEM_TRY_CATCH(return nsnull)
  return buf;
}

nsZipItemPtr_base::nsZipItemPtr_base(nsZipArchive *aZip, const char * aEntryName, bool doCRC) :
  mReturnBuf(nsnull)
{
  
  mZipHandle = aZip->GetFD();

  nsZipItem* item = aZip->GetItem(aEntryName);
  if (!item)
    return;

  PRUint32 size = 0;
  if (item->Compression() == DEFLATED) {
    size = item->RealSize();
    mAutoBuf = new PRUint8[size];
  }

  nsZipCursor cursor(item, aZip, mAutoBuf, size, doCRC);
  mReturnBuf = cursor.Read(&mReadlen);
  if (!mReturnBuf) {
    Telemetry::Accumulate(Telemetry::ZIPARCHIVE_CRC, false);
    return;
  }

  if (mReadlen != item->RealSize()) {
    NS_ASSERTION(mReadlen == item->RealSize(), "nsZipCursor underflow");
    mReturnBuf = nsnull;
    return;
  }

  Telemetry::Accumulate(Telemetry::ZIPARCHIVE_CRC, true);
}
