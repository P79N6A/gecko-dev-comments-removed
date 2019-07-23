





































#ifdef DEBUG
#define ENABLE_STRING_STATS
#endif

#ifdef ENABLE_STRING_STATS
#include <stdio.h>
#endif

#include <stdlib.h>
#include "nsSubstring.h"
#include "nsString.h"
#include "nsStringBuffer.h"
#include "nsDependentString.h"
#include "nsMemory.h"
#include "pratom.h"
#include "prprf.h"



static PRUnichar gNullChar = 0;

char*      nsCharTraits<char>     ::sEmptyBuffer = (char*) &gNullChar;
PRUnichar* nsCharTraits<PRUnichar>::sEmptyBuffer =         &gNullChar;



#ifdef ENABLE_STRING_STATS
class nsStringStats
  {
    public:
      nsStringStats()
        : mAllocCount(0), mReallocCount(0), mFreeCount(0), mShareCount(0) {}

      ~nsStringStats()
        {
          
          
          
          if (!mAllocCount && !mAdoptCount)
            return;

          printf("nsStringStats\n");
          printf(" => mAllocCount:     % 10d\n", mAllocCount);
          printf(" => mReallocCount:   % 10d\n", mReallocCount);
          printf(" => mFreeCount:      % 10d", mFreeCount);
          if (mAllocCount > mFreeCount)
            printf("  --  LEAKED %d !!!\n", mAllocCount - mFreeCount);
          else
            printf("\n");
          printf(" => mShareCount:     % 10d\n", mShareCount);
          printf(" => mAdoptCount:     % 10d\n", mAdoptCount);
          printf(" => mAdoptFreeCount: % 10d", mAdoptFreeCount);
          if (mAdoptCount > mAdoptFreeCount)
            printf("  --  LEAKED %d !!!\n", mAdoptCount - mAdoptFreeCount);
          else
            printf("\n");
        }

      PRInt32 mAllocCount;
      PRInt32 mReallocCount;
      PRInt32 mFreeCount;
      PRInt32 mShareCount;
      PRInt32 mAdoptCount;
      PRInt32 mAdoptFreeCount;
  };
static nsStringStats gStringStats;
#define STRING_STAT_INCREMENT(_s) PR_AtomicIncrement(&gStringStats.m ## _s ## Count)
#else
#define STRING_STAT_INCREMENT(_s)
#endif



inline void
ReleaseData( void* data, PRUint32 flags )
  {
    if (flags & nsSubstring::F_SHARED)
      {
        nsStringBuffer::FromData(data)->Release();
      }
    else if (flags & nsSubstring::F_OWNED)
      {
        nsMemory::Free(data);
        STRING_STAT_INCREMENT(AdoptFree);
#ifdef NS_BUILD_REFCNT_LOGGING
        
        
        NS_LogDtor(data, "StringAdopt", 1);
#endif 
      }
    
  }





class nsAStringAccessor : public nsAString
  {
    private:
      nsAStringAccessor(); 

    public:
      char_type  *data() const   { return mData; }
      size_type   length() const { return mLength; }
      PRUint32    flags() const  { return mFlags; }

      void set(char_type *data, size_type len, PRUint32 flags)
        {
          ReleaseData(mData, mFlags);
          mData = data;
          mLength = len;
          mFlags = flags;
        }
  };

class nsACStringAccessor : public nsACString
  {
    private:
      nsACStringAccessor(); 

    public:
      char_type  *data() const   { return mData; }
      size_type   length() const { return mLength; }
      PRUint32    flags() const  { return mFlags; }

      void set(char_type *data, size_type len, PRUint32 flags)
        {
          ReleaseData(mData, mFlags);
          mData = data;
          mLength = len;
          mFlags = flags;
        }
  };



void
nsStringBuffer::AddRef()
  {
    PR_AtomicIncrement(&mRefCount);
    STRING_STAT_INCREMENT(Share);
    NS_LOG_ADDREF(this, mRefCount, "nsStringBuffer", sizeof(*this));
  }

void
nsStringBuffer::Release()
  {
    PRInt32 count = PR_AtomicDecrement(&mRefCount);
    NS_LOG_RELEASE(this, count, "nsStringBuffer");
    if (count == 0)
      {
        STRING_STAT_INCREMENT(Free);
        free(this); 
      }
  }

  


nsStringBuffer*
nsStringBuffer::Alloc(size_t size)
  {
    NS_ASSERTION(size != 0, "zero capacity allocation not allowed");

    nsStringBuffer *hdr =
        (nsStringBuffer *) malloc(sizeof(nsStringBuffer) + size);
    if (hdr)
      {
        STRING_STAT_INCREMENT(Alloc);

        hdr->mRefCount = 1;
        hdr->mStorageSize = size;
        NS_LOG_ADDREF(hdr, 1, "nsStringBuffer", sizeof(*hdr));
      }
    return hdr;
  }

nsStringBuffer*
nsStringBuffer::Realloc(nsStringBuffer* hdr, size_t size)
  {
    STRING_STAT_INCREMENT(Realloc);

    NS_ASSERTION(size != 0, "zero capacity allocation not allowed");

    
    NS_ASSERTION(!hdr->IsReadonly(), "|Realloc| attempted on readonly string");

    
    
    
    NS_LOG_RELEASE(hdr, 0, "nsStringBuffer");
    
    hdr = (nsStringBuffer*) realloc(hdr, sizeof(nsStringBuffer) + size);
    if (hdr) {
      NS_LOG_ADDREF(hdr, 1, "nsStringBuffer", sizeof(*hdr));
      hdr->mStorageSize = size;
    }

    return hdr;
  }

nsStringBuffer*
nsStringBuffer::FromString(const nsAString& str)
  {
    const nsAStringAccessor* accessor =
        static_cast<const nsAStringAccessor*>(&str);

    if (!(accessor->flags() & nsSubstring::F_SHARED))
      return nsnull;

    return FromData(accessor->data());
  }

nsStringBuffer*
nsStringBuffer::FromString(const nsACString& str)
  {
    const nsACStringAccessor* accessor =
        static_cast<const nsACStringAccessor*>(&str);

    if (!(accessor->flags() & nsCSubstring::F_SHARED))
      return nsnull;

    return FromData(accessor->data());
  }

void
nsStringBuffer::ToString(PRUint32 len, nsAString &str)
  {
    PRUnichar* data = static_cast<PRUnichar*>(Data());

    nsAStringAccessor* accessor = static_cast<nsAStringAccessor*>(&str);
    NS_ASSERTION(data[len] == PRUnichar(0), "data should be null terminated");

    
    PRUint32 flags = accessor->flags();
    flags = (flags & 0xFFFF0000) | nsSubstring::F_SHARED | nsSubstring::F_TERMINATED;

    AddRef();
    accessor->set(data, len, flags);
  }

void
nsStringBuffer::ToString(PRUint32 len, nsACString &str)
  {
    char* data = static_cast<char*>(Data());

    nsACStringAccessor* accessor = static_cast<nsACStringAccessor*>(&str);
    NS_ASSERTION(data[len] == char(0), "data should be null terminated");

    
    PRUint32 flags = accessor->flags();
    flags = (flags & 0xFFFF0000) | nsCSubstring::F_SHARED | nsCSubstring::F_TERMINATED;

    AddRef();
    accessor->set(data, len, flags);
  }




  
#include "string-template-def-unichar.h"
#include "nsTSubstring.cpp"
#include "string-template-undef.h"

  
#include "string-template-def-char.h"
#include "nsTSubstring.cpp"
#include "string-template-undef.h"




#include "prlog.h"
#include "nsXPCOMStrings.h"

PR_STATIC_ASSERT(sizeof(nsStringContainer_base) == sizeof(nsSubstring));
