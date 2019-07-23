




































#include "nsSoftwareUpdate.h"

#include "nsInstall.h"
#include "nsInstallVersion.h"
#include "nsIDOMInstallVersion.h"

#include "nscore.h"
#include "nsString.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIScriptGlobalObject.h"

#include "prprf.h"

static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
static NS_DEFINE_IID(kIScriptObjectOwnerIID, NS_ISCRIPTOBJECTOWNER_IID);

static NS_DEFINE_IID(kIInstallVersion_IID, NS_IDOMINSTALLVERSION_IID);


nsInstallVersion::nsInstallVersion()
{
    mScriptObject   = nsnull;
}

nsInstallVersion::~nsInstallVersion()
{
}

NS_IMETHODIMP
nsInstallVersion::QueryInterface(REFNSIID aIID,void** aInstancePtr)
{
    if (aInstancePtr == NULL)
    {
        return NS_ERROR_NULL_POINTER;
    }

    
    *aInstancePtr = NULL;

    if ( aIID.Equals(kIScriptObjectOwnerIID))
    {
        *aInstancePtr = (void*) ((nsIScriptObjectOwner*)this);
        AddRef();
        return NS_OK;
    }
    else if ( aIID.Equals(kIInstallVersion_IID) )
    {
        *aInstancePtr = (void*) ((nsIDOMInstallVersion*)this);
        AddRef();
        return NS_OK;
    }
    else if ( aIID.Equals(kISupportsIID) )
    {
        *aInstancePtr = (void*)(nsISupports*)(nsIScriptObjectOwner*)this;
        AddRef();
        return NS_OK;
    }

     return NS_NOINTERFACE;
}


NS_IMPL_ADDREF(nsInstallVersion)
NS_IMPL_RELEASE(nsInstallVersion)



NS_IMETHODIMP
nsInstallVersion::GetScriptObject(nsIScriptContext *aContext, void** aScriptObject)
{
    NS_PRECONDITION(nsnull != aScriptObject, "null arg");
    nsresult res = NS_OK;

    if (!mScriptObject)
    {
        res = NS_NewScriptInstallVersion(aContext,
                                         (nsISupports *)(nsIDOMInstallVersion*)this,
                                         nsnull,
                                         &mScriptObject);
    }


    *aScriptObject = mScriptObject;
    return res;
}

NS_IMETHODIMP
nsInstallVersion::SetScriptObject(void *aScriptObject)
{
  mScriptObject = aScriptObject;
  return NS_OK;
}


NS_IMETHODIMP
nsInstallVersion::Init(PRInt32 aMajor, PRInt32 aMinor, PRInt32 aRelease, PRInt32 aBuild)
{
    mMajor   = aMajor;
    mMinor   = aMinor;
    mRelease = aRelease;
    mBuild   = aBuild;

    return NS_OK;
}

NS_IMETHODIMP
nsInstallVersion::Init(const nsString& version)
{
    mMajor = mMinor = mRelease = mBuild = 0;

    
    if (PR_sscanf(NS_ConvertUTF16toUTF8(version).get(),"%d.%d.%d.%d",&mMajor,&mMinor,&mRelease,&mBuild) < 1)
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}


NS_IMETHODIMP
nsInstallVersion::GetMajor(PRInt32* aMajor)
{
    *aMajor = mMajor;
    return NS_OK;
}

NS_IMETHODIMP
nsInstallVersion::SetMajor(PRInt32 aMajor)
{
    mMajor = aMajor;
    return NS_OK;
}

NS_IMETHODIMP
nsInstallVersion::GetMinor(PRInt32* aMinor)
{
    *aMinor = mMinor;
    return NS_OK;
}

NS_IMETHODIMP
nsInstallVersion::SetMinor(PRInt32 aMinor)
{
    mMinor = aMinor;
    return NS_OK;
}

NS_IMETHODIMP
nsInstallVersion::GetRelease(PRInt32* aRelease)
{
    *aRelease = mRelease;
    return NS_OK;
}

NS_IMETHODIMP
nsInstallVersion::SetRelease(PRInt32 aRelease)
{
    mRelease = aRelease;
    return NS_OK;
}

NS_IMETHODIMP
nsInstallVersion::GetBuild(PRInt32* aBuild)
{
    *aBuild = mBuild;
    return NS_OK;
}

NS_IMETHODIMP
nsInstallVersion::SetBuild(PRInt32 aBuild)
{
    mBuild = aBuild;
    return NS_OK;
}


NS_IMETHODIMP
nsInstallVersion::CompareTo(nsIDOMInstallVersion* aVersion, PRInt32* aReturn)
{
    PRInt32 aMajor, aMinor, aRelease, aBuild;

    aVersion->GetMajor(&aMajor);
    aVersion->GetMinor(&aMinor);
    aVersion->GetRelease(&aRelease);
    aVersion->GetBuild(&aBuild);

    CompareTo(aMajor, aMinor, aRelease, aBuild, aReturn);

    return NS_OK;
}


NS_IMETHODIMP
nsInstallVersion::CompareTo(const nsString& aString, PRInt32* aReturn)
{
    nsInstallVersion inVersion;
    inVersion.Init(aString);

    return CompareTo(&inVersion, aReturn);
}

NS_IMETHODIMP
nsInstallVersion::CompareTo(PRInt32 aMajor, PRInt32 aMinor, PRInt32 aRelease, PRInt32 aBuild, PRInt32* aReturn)
{
    int diff;

    if ( mMajor == aMajor )
    {
        if ( mMinor == aMinor )
        {
            if ( mRelease == aRelease )
            {
                if ( mBuild == aBuild )
                    diff = EQUAL;
                else if ( mBuild > aBuild )
                    diff = BLD_DIFF;
                else
                    diff = BLD_DIFF_MINUS;
            }
            else if ( mRelease > aRelease )
                diff = REL_DIFF;
            else
                diff = REL_DIFF_MINUS;
        }
        else if (  mMinor > aMinor )
            diff = MINOR_DIFF;
        else
            diff = MINOR_DIFF_MINUS;
    }
    else if ( mMajor > aMajor )
        diff = MAJOR_DIFF;
    else
        diff = MAJOR_DIFF_MINUS;

    *aReturn = diff;

    return NS_OK;
}

NS_IMETHODIMP
nsInstallVersion::ToString(nsString& aReturn)
{
    char buf[128];
    PRUint32 len;

    len=PR_snprintf(buf, sizeof(buf), "%d.%d.%d.%d", mMajor, mMinor, mRelease, mBuild);
    aReturn.AssignASCII(buf,len);

    return NS_OK;
}
