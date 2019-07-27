





#ifndef xpcObjectHelper_h
#define xpcObjectHelper_h


#ifdef XP_WIN
#ifdef GetClassInfo
#undef GetClassInfo
#endif
#endif

#include "mozilla/Attributes.h"
#include <stdint.h>
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIClassInfo.h"
#include "nsISupports.h"
#include "nsIXPCScriptable.h"
#include "nsWrapperCache.h"

class xpcObjectHelper
{
public:
    explicit xpcObjectHelper(nsISupports* aObject, nsWrapperCache* aCache = nullptr)
      : mCanonical(nullptr)
      , mObject(aObject)
      , mCache(aCache)
    {
        if (!mCache) {
            if (aObject)
                CallQueryInterface(aObject, &mCache);
            else
                mCache = nullptr;
        }
    }

    nsISupports* Object()
    {
        return mObject;
    }

    nsISupports* GetCanonical()
    {
        if (!mCanonical) {
            mCanonicalStrong = do_QueryInterface(mObject);
            mCanonical = mCanonicalStrong;
        }
        return mCanonical;
    }

    already_AddRefed<nsISupports> forgetCanonical()
    {
        MOZ_ASSERT(mCanonical, "Huh, no canonical to forget?");

        if (!mCanonicalStrong)
            mCanonicalStrong = mCanonical;
        mCanonical = nullptr;
        return mCanonicalStrong.forget();
    }

    nsIClassInfo* GetClassInfo()
    {
        if (mXPCClassInfo)
          return mXPCClassInfo;
        if (!mClassInfo)
            mClassInfo = do_QueryInterface(mObject);
        return mClassInfo;
    }
    nsXPCClassInfo* GetXPCClassInfo()
    {
        if (!mXPCClassInfo) {
            CallQueryInterface(mObject, getter_AddRefs(mXPCClassInfo));
        }
        return mXPCClassInfo;
    }

    already_AddRefed<nsXPCClassInfo> forgetXPCClassInfo()
    {
        GetXPCClassInfo();

        return mXPCClassInfo.forget();
    }

    
    uint32_t GetScriptableFlags()
    {
        
        nsCOMPtr<nsIXPCScriptable> sinfo = GetXPCClassInfo();

        
        if (!sinfo)
            sinfo = do_QueryInterface(GetCanonical());

        
        MOZ_ASSERT(sinfo);

        
        return sinfo->GetScriptableFlags();
    }

    nsWrapperCache* GetWrapperCache()
    {
        return mCache;
    }

protected:
    xpcObjectHelper(nsISupports* aObject, nsISupports* aCanonical,
                    nsWrapperCache* aCache)
      : mCanonical(aCanonical)
      , mObject(aObject)
      , mCache(aCache)
    {
        if (!mCache && aObject)
            CallQueryInterface(aObject, &mCache);
    }

    nsCOMPtr<nsISupports>    mCanonicalStrong;
    nsISupports* MOZ_UNSAFE_REF("xpcObjectHelper has been specifically optimized "
                                "to avoid unnecessary AddRefs and Releases. "
                                "(see bug 565742)") mCanonical;

private:
    xpcObjectHelper(xpcObjectHelper& aOther) = delete;

    nsISupports* MOZ_UNSAFE_REF("xpcObjectHelper has been specifically optimized "
                                "to avoid unnecessary AddRefs and Releases. "
                                "(see bug 565742)") mObject;
    nsWrapperCache*          mCache;
    nsCOMPtr<nsIClassInfo>   mClassInfo;
    nsRefPtr<nsXPCClassInfo> mXPCClassInfo;
};

#endif
