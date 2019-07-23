




































#include "nsJVMConfigManager.h"

NS_IMPL_ISUPPORTS1(nsJVMConfig, nsIJVMConfig)

nsJVMConfig::nsJVMConfig(const nsAString& aVersion, const nsAString& aType,
                         const nsAString& aOS, const nsAString& aArch,
                         nsIFile* aPath, nsIFile* aMozillaPluginPath,
                         const nsAString& aDescription) :
mVersion(aVersion),
mType(aType),
mOS(aOS),
mArch(aArch),
mPath(aPath),
mMozillaPluginPath(aMozillaPluginPath),
mDescription(aDescription)
{
}

nsJVMConfig::~nsJVMConfig()
{
}


NS_IMETHODIMP
nsJVMConfig::GetVersion(nsAString & aVersion)
{
    aVersion = mVersion;
    return NS_OK;
}


NS_IMETHODIMP
nsJVMConfig::GetType(nsAString & aType)
{
    aType = mType;
    return NS_OK;
}


NS_IMETHODIMP
nsJVMConfig::GetOS(nsAString & aOS)
{
    aOS = mOS;
    return NS_OK;
}


NS_IMETHODIMP
nsJVMConfig::GetArch(nsAString & aArch)
{
    aArch = mArch;
    return NS_OK;
}

NS_IMETHODIMP
nsJVMConfig::GetPath(nsIFile** aPath)
{
    NS_ENSURE_ARG_POINTER(aPath);

    *aPath = mPath;
    NS_IF_ADDREF(*aPath);
    return NS_OK;
}

NS_IMETHODIMP
nsJVMConfig::GetMozillaPluginPath(nsIFile** aMozillaPluginPath)
{
    NS_ENSURE_ARG_POINTER(aMozillaPluginPath);

    *aMozillaPluginPath = mMozillaPluginPath;
    NS_IF_ADDREF(*aMozillaPluginPath);
    return NS_OK;
}


NS_IMETHODIMP
nsJVMConfig::GetDescription(nsAString & aDescription)
{
    aDescription = mDescription;
    return NS_OK;
}

