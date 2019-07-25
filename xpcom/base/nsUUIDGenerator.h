





































#ifndef _NSUUIDGENERATOR_H_
#define _NSUUIDGENERATOR_H_

#include "mozilla/Mutex.h"
#include "nsIUUIDGenerator.h"

class nsUUIDGenerator : public nsIUUIDGenerator {
public:
    nsUUIDGenerator();

    NS_DECL_ISUPPORTS

    NS_DECL_NSIUUIDGENERATOR

    nsresult Init();

private:
    ~nsUUIDGenerator();

protected:

    mozilla::Mutex mLock;
#if !defined(XP_WIN) && !defined(XP_MACOSX) && !defined(ANDROID)
    char mState[128];
    char *mSavedState;
    PRUint8 mRBytes;
#endif
};

#define NS_UUID_GENERATOR_CONTRACTID "@mozilla.org/uuid-generator;1"
#define NS_UUID_GENERATOR_CLASSNAME "UUID Generator"
#define NS_UUID_GENERATOR_CID \
{ 0x706d36bb, 0xbf79, 0x4293, \
{ 0x81, 0xf2, 0x8f, 0x68, 0x28, 0xc1, 0x8f, 0x9d } }

#endif 
