


















































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
#elif defined(XP_BEOS)
    #include <unistd.h>
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


static const PRUint32 kMaxNameLength = PATH_MAX; 

static const PRUint16 kSyntheticTime = 0;
static const PRUint16 kSyntheticDate = (1 + (1 << 5) + (0 << 9));

static PRUint16 xtoint(const unsigned char *ii);
static PRUint32 xtolong(const unsigned char *ll);
static PRUint32 HashName(const char* aName, PRUint16 nameLen);
#if defined(XP_UNIX) || defined(XP_BEOS)
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
  : mFd(nsnull)
  , mFileData(nsnull)
  , mLen(0)
  , mMap(nsnull)
  , mRefCnt(0)
{
  MOZ_COUNT_CTOR(nsZipHandle);
}

NS_IMPL_THREADSAFE_ADDREF(nsZipHandle)
NS_IMPL_THREADSAFE_RELEASE(nsZipHandle)

nsresult nsZipHandle::Init(PRFileDesc *fd, nsZipHandle **ret)
{
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

  nsZipHandle *handle = new nsZipHandle();
  if (!handle) {
    PR_MemUnmap(buf, size);
    PR_CloseFileMap(map);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  handle->mFd = fd;
  handle->mMap = map;
  handle->mLen = (PRUint32) size;
  handle->mFileData = buf;
  handle->AddRef();
  *ret = handle;
  return NS_OK;
}

nsZipHandle::~nsZipHandle()
{
  if (mFileData) {
    PR_MemUnmap(mFileData, mLen);
    PR_CloseFileMap(mMap);
    mFileData = nsnull;
    mMap = nsnull;
  }
  if (mFd) {
    PR_Close(mFd);
    mFd = nsnull;
  }
  MOZ_COUNT_DTOR(nsZipHandle);
}









nsresult nsZipArchive::OpenArchive(PRFileDesc * fd)
{
  nsresult rv = nsZipHandle::Init(fd, getter_AddRefs(mFd));
  if (NS_FAILED(rv))
    return rv;

  
  PL_INIT_ARENA_POOL(&mArena, "ZipArena", ZIP_ARENABLOCKSIZE);

  
  return BuildFileList();
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

    nsZipItem* item = mFiles[ HashName(aEntryName, len) ];
    while (item) {
      if ((len == item->nameLength) && 
         (!memcmp(aEntryName, item->Name(), len)))
        return item; 
      item = item->next;
    }
  }
  return 0;
}








nsresult nsZipArchive::ExtractFile(nsZipItem *item, const char *outname,
                                   PRFileDesc* aFd)
{
  if (!item)
    return NS_ERROR_ILLEGAL_VALUE;
  if (!mFd)
    return NS_ERROR_FAILURE;

  
  
  PR_ASSERT(!item->IsDirectory());

  nsresult rv;

  
  switch(item->Compression())
  {
    case STORED:
      rv = CopyItemToDisk(item, aFd);
      break;

    case DEFLATED:
      rv = InflateItem(item, aFd);
      break;

    default:
      
      rv = NS_ERROR_NOT_IMPLEMENTED;
  }

  
  if (aFd) {
    PR_Close(aFd);
    if (rv != NS_OK)
      PR_Delete(outname);
#if defined(XP_UNIX) || defined(XP_BEOS)
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

  return NS_ERROR_FILE_TARGET_DOES_NOT_EXIST;
}

#if defined(XP_UNIX) || defined(XP_BEOS)



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
  
  PRUint8* buf;
  PRUint8* startp = mFd->mFileData;
  PRUint8* endp = startp + mFd->mLen;

  for (buf = endp - ZIPEND_SIZE; xtolong(buf) != ENDSIG; buf--)
  {
    if (buf == startp) {
      
      
      return NS_ERROR_FILE_CORRUPTED;
    }
  }
  PRUint32 centralOffset = xtolong(((ZipEnd *)buf)->offset_central_dir);

  
  buf = startp + centralOffset;
  PRUint32 sig = xtolong(buf);
  while (sig == CENTRALSIG) {
    
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

    sig = xtolong(buf);
  } 

  if (sig != ENDSIG)
    return NS_ERROR_FILE_CORRUPTED;
  return NS_OK;
}




nsresult nsZipArchive::BuildSynthetics()
{
  if (mBuiltSynthetics)
    return NS_OK;
  mBuiltSynthetics = true;

  
  
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
  return NS_OK;
}

nsZipHandle* nsZipArchive::GetFD()
{
  if (!mFd)
    return NULL;
  return mFd.get();
}




PRUint8* nsZipArchive::GetData(nsZipItem* aItem)
{
  PR_ASSERT (aItem);

  
  
  PRUint32 len = mFd->mLen;
  PRUint8* data = mFd->mFileData;
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
}




nsresult
nsZipArchive::CopyItemToDisk(nsZipItem *item, PRFileDesc* outFD)
{
  PR_ASSERT(item);

  
  const PRUint8* itemData = GetData(item);
  if (!itemData)
    return NS_ERROR_FILE_CORRUPTED;

  if (outFD && PR_Write(outFD, itemData, item->Size()) < (READTYPE)item->Size())
  {
    
    return NS_ERROR_FILE_DISK_FULL;
  }

  
  PRUint32 crc = crc32(0L, (const unsigned char*)itemData, item->Size());
  
  if (crc != item->CRC32())
      return NS_ERROR_FILE_CORRUPTED;

  return NS_OK;
}





nsresult nsZipArchive::InflateItem(nsZipItem * item, PRFileDesc* outFD)





{
  PR_ASSERT(item);
  
  Bytef outbuf[ZIP_BUFLEN];

  
  z_stream    zs;
  nsresult status = gZlibInit(&zs);
  if (status != NS_OK)
    return NS_ERROR_FAILURE;

  
  zs.avail_in = item->Size();
  zs.next_in = (Bytef*)GetData(item);
  if (!zs.next_in)
    return NS_ERROR_FILE_CORRUPTED;

  PRUint32  crc = crc32(0L, Z_NULL, 0);
  int zerr = Z_OK;
  while (zerr == Z_OK)
  {
    zs.next_out = outbuf;
    zs.avail_out = ZIP_BUFLEN;

    zerr = inflate(&zs, Z_PARTIAL_FLUSH);
    if (zerr != Z_OK && zerr != Z_STREAM_END)
    {
      status = (zerr == Z_MEM_ERROR) ? NS_ERROR_OUT_OF_MEMORY : NS_ERROR_FILE_CORRUPTED;
      break;
    }
    PRUint32 count = zs.next_out - outbuf;

    
    crc = crc32(crc, (const unsigned char*)outbuf, count);

    if (outFD && PR_Write(outFD, outbuf, count) < (READTYPE)count)
    {
      status = NS_ERROR_FILE_DISK_FULL;
      break;
    }
  } 

  
  inflateEnd(&zs);

  
  if ((status == NS_OK) && (crc != item->CRC32()))
  {
    status = NS_ERROR_FILE_CORRUPTED;
  }
  return status;
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







static PRUint16 xtoint (const unsigned char *ii)
{
  return (PRUint16) ((ii [0]) | (ii [1] << 8));
}







static PRUint32 xtolong (const unsigned char *ll)
{
  return (PRUint32)( (ll [0] <<  0) |
                     (ll [1] <<  8) |
                     (ll [2] << 16) |
                     (ll [3] << 24) );
}

PRUint32 const nsZipItem::LocalOffset()
{
  return xtolong(central->localhdr_offset);
}

PRUint32 const nsZipItem::Size()
{
  return isSynthetic ? 0 : xtolong(central->size);
}

PRUint32 const nsZipItem::RealSize()
{
  return isSynthetic ? 0 : xtolong(central->orglen);
}

PRUint32 const nsZipItem::CRC32()
{
  return isSynthetic ? 0 : xtolong(central->crc32);
}

PRUint16 const nsZipItem::Date()
{
  return isSynthetic ? kSyntheticDate : xtoint(central->date);
}

PRUint16 const nsZipItem::Time()
{
  return isSynthetic ? kSyntheticTime : xtoint(central->time);
}

PRUint16 const nsZipItem::Compression()
{
  return isSynthetic ? STORED : xtoint(central->method);
}

bool const nsZipItem::IsDirectory()
{
  return isSynthetic || ((nameLength > 0) && ('/' == Name()[nameLength - 1]));
}

PRUint16 const nsZipItem::Mode()
{
  if (isSynthetic) return 0755;
  return ((PRUint16)(central->external_attributes[2]) | 0x100);
}

#if defined(XP_UNIX) || defined(XP_BEOS)
bool const nsZipItem::IsSymlink()
{
  if (isSynthetic) return false;
  return (xtoint(central->external_attributes+2) & S_IFMT) == S_IFLNK;
}
#endif
