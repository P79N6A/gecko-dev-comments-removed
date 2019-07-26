



#ifndef xpcObjectHelper_h
#define xpcObjectHelper_h


#ifdef XP_WIN
#ifdef GetClassInfo
#undef GetClassInfo
#endif
#endif

#include "mozilla/Attributes.h"
#include "mozilla/StandardInteger.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIClassInfo.h"
#include "nsINode.h"
#include "nsISupports.h"
#include "nsIXPCScriptable.h"
#include "nsWrapperCache.h"

class xpcObjectHelper
{
public:
    xpcObjectHelper(nsISupports *aObject, nsWrapperCache *aCache = NULL)
      : mCanonical(NULL)
      , mObject(aObject)
      , mCache(aCache)
      , mIsNode(false)
    {
        if (!mCache) {
            if (aObject)
                CallQueryInterface(aObject, &mCache);
            else
                mCache = NULL;
        }
    }

    nsISupports *Object()
    {
        return mObject;
    }

    nsISupports *GetCanonical()
    {
        if (!mCanonical) {
            mCanonicalStrong = do_QueryInterface(mObject);
            mCanonical = mCanonicalStrong;
        }
        return mCanonical;
    }

    already_AddRefed<nsISupports> forgetCanonical()
    {
        NS_ASSERTION(mCanonical, "Huh, no canonical to forget?");

        if (!mCanonicalStrong)
            mCanonicalStrong = mCanonical;
        mCanonical = NULL;
        return mCanonicalStrong.forget();
    }

    nsIClassInfo *GetClassInfo()
    {
        if (mXPCClassInfo)
          return mXPCClassInfo;
        if (!mClassInfo)
            mClassInfo = do_QueryInterface(mObject);
        return mClassInfo;
    }
    nsXPCClassInfo *GetXPCClassInfo()
    {
        if (!mXPCClassInfo) {
            if (mIsNode) {
                mXPCClassInfo =
                    static_cast<nsINode*>(GetCanonical())->GetClassInfo();
#ifdef DEBUG
                AssertGetClassInfoResult();
#endif
            } else {
                CallQueryInterface(mObject, getter_AddRefs(mXPCClassInfo));
            }
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

    nsWrapperCache *GetWrapperCache()
    {
        return mCache;
    }

protected:
    xpcObjectHelper(nsISupports *aObject, nsISupports *aCanonical,
                    nsWrapperCache *aCache, bool aIsNode)
      : mCanonical(aCanonical)
      , mObject(aObject)
      , mCache(aCache)
      , mIsNode(aIsNode)
    {
        if (!mCache && aObject)
            CallQueryInterface(aObject, &mCache);
    }

    nsCOMPtr<nsISupports>    mCanonicalStrong;
    nsISupports*             mCanonical;

private:
    xpcObjectHelper(xpcObjectHelper& aOther) MOZ_DELETE;

#ifdef DEBUG
    void AssertGetClassInfoResult();
#endif

    nsISupports*             mObject;
    nsWrapperCache*          mCache;
    nsCOMPtr<nsIClassInfo>   mClassInfo;
    nsRefPtr<nsXPCClassInfo> mXPCClassInfo;
    bool                     mIsNode;
};

#endif
