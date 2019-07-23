



















































#define ZFILE_CREATE    PR_WRONLY | PR_CREATE_FILE
#define READTYPE  PRInt32
#include "zlib.h"
#include "nsISupportsUtils.h"
#include "nsRecyclingAllocator.h"
#include "prio.h"
#include "plstr.h"
#include "prlog.h"
#include "stdlib.h"
#include "nsWildCard.h"
#include "zipfile.h"
#include "zipstruct.h"
#include "nsZipArchive.h"






#define NBUCKETS 6
#define BY4ALLOC_ITEMS 320
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

#include "zipfile.h"
#include "zipstruct.h"
#include "nsZipArchive.h"

static PRUint16 xtoint(unsigned char *ii);
static PRUint32 xtolong(unsigned char *ll);
static PRUint16 ExtractMode(unsigned char *ll);
static PRUint32 HashName(const char* aName);
#if defined(XP_UNIX) || defined(XP_BEOS)
static PRBool IsSymlink(unsigned char *ll);
static nsresult ResolveSymlink(const char *path);
#endif















































static void *
zlibAlloc(void *opaque, uInt items, uInt size)
{
  nsRecyclingAllocator *zallocator = (nsRecyclingAllocator *)opaque;
  if (zallocator) {
    
    PRUint32 realitems = items;
    if (size == 4 && items < BY4ALLOC_ITEMS)
      realitems = BY4ALLOC_ITEMS;
     return zallocator->Calloc(realitems, size);
  }
  else
    return calloc(items, size);
}

static void
zlibFree(void *opaque, void *ptr)
{
  nsRecyclingAllocator *zallocator = (nsRecyclingAllocator *)opaque;
  if (zallocator)
    zallocator->Free(ptr);
  else
    free(ptr);
  return;
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
  if (zerr != Z_OK) return ZIP_ERR_MEMORY;

  return ZIP_OK;
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

  nsZipHandle *handle = new nsZipHandle();
  if (!handle)
    return NS_ERROR_OUT_OF_MEMORY;

  handle->mFd = fd;
  handle->mMap = map;
  handle->mLen = (PRUint32) size;
  handle->mFileData = (PRUint8*) PR_MemMap(map, 0, handle->mLen);
  handle->AddRef();
  *ret = handle;
  return NS_OK;
}

PRInt32 nsZipHandle::Read(PRUint32 aPosition, void *aBuffer, PRUint32 aCount)
{
  if (mLen < 0 || aPosition+aCount > (PRUint32) mLen)
    return -1;
  PRInt32 count = PR_MIN(aCount, mLen - aPosition);
  memcpy(aBuffer, mFileData + aPosition, count);
  return count;
}

nsZipHandle::~nsZipHandle()
{
  if (mFd) {
    PR_MemUnmap(mFileData, mLen);
    PR_CloseFileMap(mMap);
    PR_Close(mFd);
    mFd = 0;
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
      return ZIP_ERR_FNF;
    
    if (currItem->isDirectory)
      return ZIP_OK;
    return ExtractFile(currItem, 0, 0);
  }

  
  for (int i = 0; i < ZIP_TABSIZE; i++) {
    for (currItem = mFiles[i]; currItem; currItem = currItem->next) {
      
      if (currItem->isDirectory)
        continue;
      nsresult rv = ExtractFile(currItem, 0, 0);
      if (rv != ZIP_OK)
        return rv;
    }
  }

  return ZIP_OK;
}




nsresult nsZipArchive::CloseArchive()
{
  if (mFd) {
    PL_FinishArenaPool(&mArena);
  }

  
  
  
  
  
  
  
  for (int i = 0; i < ZIP_TABSIZE; i++) {
    mFiles[i] = 0;
  }
  mFd = NULL;
  mBuiltSynthetics = PR_FALSE;
  return ZIP_OK;
}




nsZipItem*  nsZipArchive::GetItem(const char * aEntryName)
{
  if (aEntryName) {
    
    
    if (!mBuiltSynthetics) {
        PRUint32 len = strlen(aEntryName);
        if ((len > 0) && (aEntryName[len-1] == '/')) {
            if (BuildSynthetics() != ZIP_OK)
                return 0;
        }
    }

    nsZipItem* item = mFiles[ HashName(aEntryName) ];
    while (item) {
      if (!strcmp(aEntryName, item->name))
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
    return ZIP_ERR_PARAM;
  if (!mFd)
    return ZIP_ERR_GENERAL;

  
  
  PR_ASSERT(!item->isDirectory);

  
  if (!MaybeReadItem(item))
    return ZIP_ERR_CORRUPT;

  nsSeekableZipHandle fd;
  if (!fd.Open(mFd.get(), item->dataOffset, item->size))
    return ZIP_ERR_CORRUPT;

  nsresult rv;

  
  switch(item->compression)
  {
    case STORED:
      rv = CopyItemToDisk(item->size, item->crc32, fd, aFd);
      break;

    case DEFLATED:
      rv = InflateItem(item, fd, aFd);
      break;

    default:
      
      rv = ZIP_ERR_UNSUPPORTED;
  }

  
  if (aFd) {
    PR_Close(aFd);
    if (rv != ZIP_OK)
      PR_Delete(outname);
#if defined(XP_UNIX) || defined(XP_BEOS)
    else if (item->isSymlink)
      rv = ResolveSymlink(outname);
#endif
  }

  return rv;
}




PRInt32
nsZipArchive::FindInit(const char * aPattern, nsZipFind **aFind)
{
  if (!aFind)
    return ZIP_ERR_PARAM;

  
  *aFind = NULL;

  PRBool  regExp = PR_FALSE;
  char*   pattern = 0;

  
  nsresult rv = BuildSynthetics();
  if (rv != ZIP_OK)
    return rv;

  
  if (aPattern)
  {
    switch (NS_WildCardValid((char*)aPattern))
    {
      case INVALID_SXP:
        return ZIP_ERR_PARAM;

      case NON_SXP:
        regExp = PR_FALSE;
        break;

      case VALID_SXP:
        regExp = PR_TRUE;
        break;

      default:
        
        PR_ASSERT(PR_FALSE);
        return ZIP_ERR_PARAM;
    }

    pattern = PL_strdup(aPattern);
    if (!pattern)
      return ZIP_ERR_MEMORY;
  }

  *aFind = new nsZipFind(this, pattern, regExp);
  if (!*aFind) {
    PL_strfree(pattern);
    return ZIP_ERR_MEMORY;
  }

  return ZIP_OK;
}






nsresult nsZipFind::FindNext(const char ** aResult)
{
  if (!mArchive || !aResult)
    return ZIP_ERR_PARAM;

  *aResult = 0;

  
  while (mSlot < ZIP_TABSIZE)
  {
    
    mItem = mItem ? mItem->next : mArchive->mFiles[mSlot];

    PRBool found = PR_FALSE;
    if (!mItem)
      ++mSlot;                          
    else if (!mPattern)
      found = PR_TRUE;            
    else if (mRegExp)
      found = (NS_WildCardMatch(mItem->name, mPattern, PR_FALSE) == MATCH);
    else
      found = (PL_strcmp(mItem->name, mPattern) == 0);

    if (found) {
      *aResult = mItem->name;
      return ZIP_OK;
    }
  }

  return ZIP_ERR_FNF;
}

#if defined(XP_UNIX) || defined(XP_BEOS)



static nsresult ResolveSymlink(const char *path)
{
  PRFileDesc * fIn = PR_Open(path, PR_RDONLY, 0000);
  if (!fIn)
    return ZIP_ERR_DISK;

  char buf[PATH_MAX+1];
  PRInt32 length = PR_Read(fIn, (void*)buf, PATH_MAX);
  PR_Close(fIn);

  if ( (length <= 0)
    || ((buf[length] = 0, PR_Delete(path)) != 0)
    || (symlink(buf, path) != 0))
  {
     return ZIP_ERR_DISK;
  }
  return ZIP_OK;
}
#endif





#define BR_BUF_SIZE 1024 /* backward read buffer size */




nsZipItem* nsZipArchive::CreateZipItem(PRUint16 namelen)
{
  
  
  void *mem;
  PL_ARENA_ALLOCATE(mem, &mArena, sizeof(nsZipItem)+namelen);
  return (nsZipItem*)mem;
}




nsresult nsZipArchive::BuildFileList()
{
  
  PRUint8* buf;
  PRUint8* endp = mFd->mFileData + mFd->mLen;

  for (buf = endp - ZIPEND_SIZE; xtolong(buf) != ENDSIG; buf--)
  {
    if (buf == mFd->mFileData) {
      
      
      return ZIP_ERR_CORRUPT;
    }
  }
  PRUint32 central = xtolong(((ZipEnd *)buf)->offset_central_dir);

  
  buf = mFd->mFileData + central;
  PRUint32 sig = xtolong(buf);
  while (sig == CENTRALSIG) {
    
    if (endp - buf < ZIPCENTRAL_SIZE)
      return ZIP_ERR_CORRUPT;

    
    ZipCentral* central = (ZipCentral*)buf;

    PRUint16 namelen = xtoint(central->filename_len);
    PRUint16 extralen = xtoint(central->extrafield_len);
    PRUint16 commentlen = xtoint(central->commentfield_len);

    
    
    if (namelen > BR_BUF_SIZE || extralen > BR_BUF_SIZE || commentlen > 2*BR_BUF_SIZE)
      return ZIP_ERR_CORRUPT;

    nsZipItem* item = CreateZipItem(namelen);
    if (!item)
      return ZIP_ERR_MEMORY;

    item->headerOffset  = xtolong(central->localhdr_offset);
    item->dataOffset    = 0;
    item->size          = xtolong(central->size);
    item->realsize      = xtolong(central->orglen);
    item->crc32         = xtolong(central->crc32);
    item->time          = xtoint(central->time);
    item->date          = xtoint(central->date);
    item->isSynthetic   = PR_FALSE;
    item->hasDataOffset = PR_FALSE;
    item->compression   = PR_MIN(xtoint(central->method), UNSUPPORTED);
    item->mode          = ExtractMode(central->external_attributes);
#if defined(XP_UNIX) || defined(XP_BEOS)
    
    item->isSymlink = IsSymlink(central->external_attributes);
#endif

    buf += ZIPCENTRAL_SIZE;

    
    memcpy(item->name, buf, namelen);
    item->name[namelen] = 0;
    
    item->isDirectory = ('/' == item->name[namelen - 1]);

    
    PRUint32 hash = HashName(item->name);
    item->next = mFiles[hash];
    mFiles[hash] = item;

    
    buf += namelen + extralen + commentlen;
    sig = xtolong(buf);
  } 

  if (sig != ENDSIG)
    return ZIP_ERR_CORRUPT;
  return ZIP_OK;
}




nsresult nsZipArchive::BuildSynthetics()
{
  if (mBuiltSynthetics)
    return ZIP_OK;
  mBuiltSynthetics = PR_TRUE;

  
  
  for (int i = 0; i < ZIP_TABSIZE; ++i)
  {
    for (nsZipItem* item = mFiles[i]; item != 0; item = item->next)
    {
      if (item->isSynthetic)
        continue;
    
      
      
      
      
      
      
      PRUint16 namelen = strlen(item->name);
      for (char* p = item->name + namelen - 2; p >= item->name; p--)
      {
        if ('/' != *p)
          continue;

        
        
        
        
        
        
        const PRUint32 dirnamelen = p + 1 - item->name;
        const char savedChar = item->name[dirnamelen];
        item->name[dirnamelen] = 0;

        
        PRUint32 hash = HashName(item->name);
        PRBool found = PR_FALSE;
        for (nsZipItem* zi = mFiles[hash]; zi != NULL; zi = zi->next)
        {
          if (0 == strcmp(item->name, zi->name))
          {
            
            found = PR_TRUE;
            break;
          }
        }

        
        item->name[dirnamelen] = savedChar;

        
        
        
        if (found)
          break;

        nsZipItem* diritem = CreateZipItem(dirnamelen);
        if (!diritem)
          return ZIP_ERR_MEMORY;

        memcpy(diritem->name, item->name, dirnamelen);
        diritem->name[dirnamelen] = 0;

        diritem->isDirectory = PR_TRUE;
        diritem->isSynthetic = PR_TRUE;
        diritem->compression = STORED;
        diritem->size = diritem->realsize = 0;
        diritem->crc32 = 0;
        diritem->mode = 0755;

        
        
        
        
        diritem->time = 0;
        diritem->date = 1 + (1 << 5) + (0 << 9);

        
        diritem->next = mFiles[hash];
        mFiles[hash] = diritem;
      } 
    }
  }
  return ZIP_OK;
}

nsZipHandle* nsZipArchive::GetFD(nsZipItem* aItem)
{
  if (!mFd || !MaybeReadItem(aItem))
    return NULL;
  return mFd.get();
}




bool nsZipArchive::MaybeReadItem(nsZipItem* aItem)
{
  PR_ASSERT (aItem);

  
  if (!aItem->hasDataOffset)
  {
    
    
    
    
    
    
    if (!mFd || !mFd->mLen > aItem->headerOffset + ZIPLOCAL_SIZE)
      return false;

    ZipLocal   *Local =     (ZipLocal*)(mFd->mFileData + aItem->headerOffset);
    
    if ((xtolong(Local->signature) != LOCALSIG))
    {
      
      return false;
    }

    aItem->dataOffset = aItem->headerOffset +
                        ZIPLOCAL_SIZE +
                        xtoint(Local->filename_len) +
                        xtoint(Local->extrafield_len);
    aItem->hasDataOffset = PR_TRUE;
  }

  return true;
}




nsresult
nsZipArchive::CopyItemToDisk(PRUint32 itemSize, PRUint32 itemCrc, 
                             nsSeekableZipHandle &fd, PRFileDesc* outFD)





{
  PRUint32    chunk, pos, crc;
  char buf[ZIP_BUFLEN];

  
  crc = crc32(0L, Z_NULL, 0);

  
  for (pos = 0; pos < itemSize; pos += chunk)
  {
    chunk = (itemSize - pos < ZIP_BUFLEN) ? (itemSize - pos) : ZIP_BUFLEN;
    
    if (fd.Read(buf, chunk) != (READTYPE)chunk)
    {
      
      return ZIP_ERR_CORRUPT;
    }

    
    crc = crc32(crc, (const unsigned char*)buf, chunk);

    if (outFD && PR_Write(outFD, buf, chunk) < (READTYPE)chunk)
    {
      
      return ZIP_ERR_DISK;
    }
  }

  
  if (crc != itemCrc)
      return ZIP_ERR_CORRUPT;

  return ZIP_OK;
}





nsresult nsZipArchive::InflateItem(const nsZipItem* aItem, nsSeekableZipHandle &fd, PRFileDesc* outFD)





{
  PR_ASSERT(aItem);

  
  Bytef inbuf[ZIP_BUFLEN];
  Bytef outbuf[ZIP_BUFLEN];

  
  z_stream    zs;
  nsresult status = gZlibInit(&zs);
  if (status != ZIP_OK)
    return ZIP_ERR_GENERAL;

  
  zs.next_out = outbuf;
  zs.avail_out = ZIP_BUFLEN;

  PRUint32  size = aItem->size;
  PRUint32  outpos = 0;
  PRUint32  crc = crc32(0L, Z_NULL, 0);
  int       zerr = Z_OK;
  while (zerr == Z_OK)
  {
    PRBool      bRead = PR_FALSE;
    PRBool      bWrote= PR_FALSE;

    if (zs.avail_in == 0 && zs.total_in < size)
    {
      
      
      PRUint32 chunk = (size-zs.total_in < ZIP_BUFLEN) ? size-zs.total_in : ZIP_BUFLEN;

      if (fd.Read(inbuf, chunk) != (READTYPE)chunk)
      {
        
        status = ZIP_ERR_CORRUPT;
        break;
      }

      zs.next_in  = inbuf;
      zs.avail_in = chunk;
      bRead       = PR_TRUE;
    }

    if (zs.avail_out == 0)
    {
      
      if (outFD && PR_Write(outFD, outbuf, ZIP_BUFLEN) < ZIP_BUFLEN)
      {
        
        status = ZIP_ERR_DISK;
        break;
      }

      outpos = zs.total_out;
      zs.next_out  = outbuf;
      zs.avail_out = ZIP_BUFLEN;
      bWrote       = PR_TRUE;
    }

    if(bRead || bWrote)
    {
      Bytef* old_next_out = zs.next_out;

      zerr = inflate(&zs, Z_PARTIAL_FLUSH);

      
      crc = crc32(crc, (const unsigned char*)old_next_out, zs.next_out - old_next_out);
    }
    else
      zerr = Z_STREAM_END;

  } 

  
  if ((status == ZIP_OK) && (crc != aItem->crc32))
  {
      status = ZIP_ERR_CORRUPT;
      goto cleanup;
  }

  
  if (zerr == Z_STREAM_END && outpos < zs.total_out)
  {
    PRUint32 chunk = zs.total_out - outpos;
    if (outFD && PR_Write(outFD, outbuf, chunk) < (READTYPE)chunk)
      status = ZIP_ERR_DISK;
  }

  
  if (status == ZIP_OK && zerr != Z_OK && zerr != Z_STREAM_END)
  {
    status = (zerr == Z_MEM_ERROR) ? ZIP_ERR_MEMORY : ZIP_ERR_CORRUPT;
  }

  
  PR_ASSERT(status != ZIP_OK || zs.total_in == aItem->size);
  PR_ASSERT(status != ZIP_OK || zs.total_out == aItem->realsize);

cleanup:
  
  inflateEnd(&zs);

  return status;
}





nsZipArchive::nsZipArchive() :
  mBuiltSynthetics(PR_FALSE)
{
  MOZ_COUNT_CTOR(nsZipArchive);

  
  memset(mFiles, 0, sizeof mFiles);
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










static PRUint32 HashName(const char* aName)
{
  PR_ASSERT(aName != 0);

  PRUint32 val = 0;
  for (PRUint8* c = (PRUint8*)aName; *c != 0; c++) {
    val = val*37 + *c;
  }

  return (val % ZIP_TABSIZE);
}







static PRUint16 xtoint (unsigned char *ii)
{
  return (PRUint16) ((ii [0]) | (ii [1] << 8));
}







static PRUint32 xtolong (unsigned char *ll)
{
  return (PRUint32)( (ll [0] <<  0) |
                     (ll [1] <<  8) |
                     (ll [2] << 16) |
                     (ll [3] << 24) );
}









static PRUint16 ExtractMode(unsigned char *ll)
{
    return ((PRUint16)(ll[2])) | 0x0100;
}

#if defined(XP_UNIX) || defined(XP_BEOS)






static PRBool IsSymlink(unsigned char *ll)
{
  return ((xtoint(ll+2) & S_IFMT) == S_IFLNK);
}
#endif
