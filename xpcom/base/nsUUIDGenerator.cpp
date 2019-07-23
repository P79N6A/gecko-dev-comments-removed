





































#if defined(XP_WIN)
#include <windows.h>
#include <objbase.h>
#elif defined(XP_MACOSX)
#include <CoreFoundation/CoreFoundation.h>
#else
#include <stdlib.h>
#include "prrng.h"
#endif

#include "nsMemory.h"

#include "nsAutoLock.h"

#include "nsUUIDGenerator.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsUUIDGenerator, nsIUUIDGenerator)

nsUUIDGenerator::nsUUIDGenerator()
    : mLock(nsnull)
{
}

nsUUIDGenerator::~nsUUIDGenerator()
{
    if (mLock) {
        PR_DestroyLock(mLock);
    }
}

nsresult
nsUUIDGenerator::Init()
{
    mLock = PR_NewLock();

    NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

    
    
    
#if !defined(XP_WIN) && !defined(XP_MACOSX)
    
    unsigned int seed;

    PRSize bytes = 0;
    while (bytes < sizeof(seed)) {
        PRSize nbytes = PR_GetRandomNoise(((unsigned char *)&seed)+bytes,
                                          sizeof(seed)-bytes);
        if (nbytes == 0) {
            return NS_ERROR_FAILURE;
        }
        bytes += nbytes;
    }

    



    mSavedState = initstate(seed, mState, sizeof(mState));
    setstate(mSavedState);

    mRBytes = 4;
#ifdef RAND_MAX
    if ((unsigned long) RAND_MAX < (unsigned long)0xffffffff)
        mRBytes = 3;
    if ((unsigned long) RAND_MAX < (unsigned long)0x00ffffff)
        mRBytes = 2;
    if ((unsigned long) RAND_MAX < (unsigned long)0x0000ffff)
        mRBytes = 1;
    if ((unsigned long) RAND_MAX < (unsigned long)0x000000ff)
        return NS_ERROR_FAILURE;
#endif

#endif 

    return NS_OK;
}

NS_IMETHODIMP
nsUUIDGenerator::GenerateUUID(nsID** ret)
{
    nsID *id = NS_STATIC_CAST(nsID*, NS_Alloc(sizeof(nsID)));
    if (id == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = GenerateUUIDInPlace(id);
    if (NS_FAILED(rv)) {
        NS_Free(id);
        return rv;
    }

    *ret = id;
    return rv;
}

NS_IMETHODIMP
nsUUIDGenerator::GenerateUUIDInPlace(nsID* id)
{
    
    
    nsAutoLock lock(mLock);
    
#if defined(XP_WIN)
    HRESULT hr = CoCreateGuid((GUID*)id);
    if (NS_FAILED(hr))
        return NS_ERROR_FAILURE;
#elif defined(XP_MACOSX)
    CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
    if (!uuid)
        return NS_ERROR_FAILURE;

    CFUUIDBytes bytes = CFUUIDGetUUIDBytes(uuid);
    memcpy(id, &bytes, sizeof(nsID));

    CFRelease(uuid);
#else 
    



    setstate(mState);

    PRSize bytesLeft = sizeof(nsID);
    while (bytesLeft > 0) {
        long rval = random();

        PRUint8 *src = (PRUint8*)&rval;
        
        
#ifdef IS_BIG_ENDIAN
        src += sizeof(rval) - mRBytes;
#endif
        PRUint8 *dst = ((PRUint8*) id) + (sizeof(nsID) - bytesLeft);
        PRSize toWrite = (bytesLeft < mRBytes ? bytesLeft : mRBytes);
        for (PRSize i = 0; i < toWrite; i++)
            dst[i] = src[i];

        bytesLeft -= toWrite;
    }

    
    id->m2 &= 0x0fff;
    id->m2 |= 0x4000;

    
    id->m3[0] &= 0x3f;
    id->m3[0] |= 0x80;

    
    setstate(mSavedState);
#endif

    return NS_OK;
}
