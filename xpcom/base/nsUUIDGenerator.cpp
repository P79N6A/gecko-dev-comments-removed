




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

#include "nsUUIDGenerator.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS1(nsUUIDGenerator, nsIUUIDGenerator)

nsUUIDGenerator::nsUUIDGenerator()
    : mLock("nsUUIDGenerator.mLock")
{
}

nsUUIDGenerator::~nsUUIDGenerator()
{
}

nsresult
nsUUIDGenerator::Init()
{
    
    
    
#if !defined(XP_WIN) && !defined(XP_MACOSX) && !defined(ANDROID)
    
    unsigned int seed;

    size_t bytes = 0;
    while (bytes < sizeof(seed)) {
        size_t nbytes = PR_GetRandomNoise(((unsigned char *)&seed)+bytes,
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
    nsID *id = static_cast<nsID*>(NS_Alloc(sizeof(nsID)));
    if (id == nullptr)
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
    
    
    MutexAutoLock lock(mLock);

#if defined(XP_WIN)
    HRESULT hr = CoCreateGuid((GUID*)id);
    if (FAILED(hr))
        return NS_ERROR_FAILURE;
#elif defined(XP_MACOSX)
    CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
    if (!uuid)
        return NS_ERROR_FAILURE;

    CFUUIDBytes bytes = CFUUIDGetUUIDBytes(uuid);
    memcpy(id, &bytes, sizeof(nsID));

    CFRelease(uuid);
#else 
    



#ifndef ANDROID
    setstate(mState);
#endif

    size_t bytesLeft = sizeof(nsID);
    while (bytesLeft > 0) {
#ifdef ANDROID
        long rval = arc4random();
        const size_t mRBytes = 4;
#else
        long rval = random();
#endif


        uint8_t *src = (uint8_t*)&rval;
        
        
#ifdef IS_BIG_ENDIAN
        src += sizeof(rval) - mRBytes;
#endif
        uint8_t *dst = ((uint8_t*) id) + (sizeof(nsID) - bytesLeft);
        size_t toWrite = (bytesLeft < mRBytes ? bytesLeft : mRBytes);
        for (size_t i = 0; i < toWrite; i++)
            dst[i] = src[i];

        bytesLeft -= toWrite;
    }

    
    id->m2 &= 0x0fff;
    id->m2 |= 0x4000;

    
    id->m3[0] &= 0x3f;
    id->m3[0] |= 0x80;

#ifndef ANDROID
    
    setstate(mSavedState);
#endif
#endif

    return NS_OK;
}
