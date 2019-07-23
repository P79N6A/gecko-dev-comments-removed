




































#ifndef nsJVMConfigManager_h___
#define nsJVMConfigManager_h___

#include "nsIJVMConfigManager.h"
#include "nsString.h"
#include "nsIFile.h"

class nsJVMConfig : public nsIJVMConfig
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIJVMCONFIG

    nsJVMConfig(const nsAString& aVersion, const nsAString& aType,
                const nsAString& aOS, const nsAString& aArch,
                nsIFile* aPath, nsIFile* aMozillaPluginPath,
                const nsAString& aDescription);

    virtual ~nsJVMConfig();
    
protected:
    nsString mVersion;
    nsString mType;
    nsString mOS;
    nsString mArch;
    nsCOMPtr<nsIFile> mPath;
    nsCOMPtr<nsIFile> mMozillaPluginPath;
    nsString mDescription;
};

#endif 
