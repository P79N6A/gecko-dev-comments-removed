





#ifndef _NSUUIDGENERATOR_H_
#define _NSUUIDGENERATOR_H_

#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"

#include "nsIUUIDGenerator.h"

class nsUUIDGenerator MOZ_FINAL : public nsIUUIDGenerator
{
public:
  nsUUIDGenerator();

  NS_DECL_THREADSAFE_ISUPPORTS

  NS_DECL_NSIUUIDGENERATOR

  nsresult Init();

private:
  ~nsUUIDGenerator();

protected:

  mozilla::Mutex mLock;
#if !defined(XP_WIN) && !defined(XP_MACOSX) && !defined(ANDROID)
  char mState[128];
  char* mSavedState;
  uint8_t mRBytes;
#endif
};

#define NS_UUID_GENERATOR_CONTRACTID "@mozilla.org/uuid-generator;1"
#define NS_UUID_GENERATOR_CID \
{ 0x706d36bb, 0xbf79, 0x4293, \
{ 0x81, 0xf2, 0x8f, 0x68, 0x28, 0xc1, 0x8f, 0x9d } }

#endif 
