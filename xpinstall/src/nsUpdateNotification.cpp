








































#include "nsUpdateNotification.h"

#include "nsISupports.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIGenericFactory.h"
#include "nsIRDFContainer.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFService.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsRDFCID.h"
#include "nsIRDFXMLSink.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsISoftwareUpdate.h"

#define NC_RDF_NAME	         	"http://home.netscape.com/NC-rdf#name"
#define NC_RDF_SOURCE			"http://home.netscape.com/NC-rdf#source"
#define NC_RDF_URL				"http://home.netscape.com/NC-rdf#url"
#define NC_RDF_CHILD			"http://home.netscape.com/NC-rdf#child"

#define NC_RDF_NOTIFICATION_ROOT	"http://home.netscape.com/NC-rdf#SoftwareNotificationRoot"
#define NC_XPI_SOURCES              "http://home.netscape.com/NC-rdf#SoftwareUpdateDataSources"
#define NC_XPI_PACKAGES             "http://home.netscape.com/NC-rdf#SoftwarePackages"

#define NC_XPI_TITLE        "http://home.netscape.com/NC-rdf#title"
#define NC_XPI_REGKEY       "http://home.netscape.com/NC-rdf#registryKey"
#define NC_XPI_VERSION      "http://home.netscape.com/NC-rdf#version"
#define NC_XPI_DESCRIPTION	"http://home.netscape.com/NC-rdf#description"

static NS_DEFINE_CID(kRDFServiceCID,   NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerCID, NS_RDFCONTAINER_CID);

nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierSources = nsnull;
nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierPackages = nsnull;
nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierPackage_Title = nsnull;
nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierPackage_Version = nsnull;
nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierPackage_Description = nsnull;
nsIRDFResource* nsXPINotifierImpl::kXPI_NotifierPackage_RegKey = nsnull;

nsIRDFResource* nsXPINotifierImpl::kNC_NotificationRoot = nsnull;
nsIRDFResource* nsXPINotifierImpl::kNC_Source = nsnull;
nsIRDFResource* nsXPINotifierImpl::kNC_Name = nsnull;
nsIRDFResource* nsXPINotifierImpl::kNC_URL = nsnull;
nsIRDFResource* nsXPINotifierImpl::kNC_Child = nsnull;


nsXPINotifierImpl::nsXPINotifierImpl()
    : mRDF(nsnull)
{
    mPendingRefreshes = 0;

    static NS_DEFINE_CID(kRDFInMemoryDataSourceCID, NS_RDFINMEMORYDATASOURCE_CID);
    
    mNotifications = do_CreateInstance(kRDFInMemoryDataSourceCID);
}


nsXPINotifierImpl::~nsXPINotifierImpl()
{
    NS_IF_RELEASE(mRDF);

    NS_IF_RELEASE(kXPI_NotifierSources);
    NS_IF_RELEASE(kXPI_NotifierPackages);
    NS_IF_RELEASE(kXPI_NotifierPackage_Title);
    NS_IF_RELEASE(kXPI_NotifierPackage_Version);
    NS_IF_RELEASE(kXPI_NotifierPackage_Description);
    NS_IF_RELEASE(kXPI_NotifierPackage_RegKey);

   	NS_IF_RELEASE(kNC_NotificationRoot);
    NS_IF_RELEASE(kNC_Source);
	NS_IF_RELEASE(kNC_Name);
	NS_IF_RELEASE(kNC_URL);
	NS_IF_RELEASE(kNC_Child);
}


NS_IMPL_ISUPPORTS2(nsXPINotifierImpl, nsIRDFXMLSinkObserver, nsIUpdateNotification)


nsresult
nsXPINotifierImpl::NotificationEnabled(PRBool* aReturn)
{
    *aReturn = PR_FALSE;

    nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);

    if ( prefBranch )
    {
        PRBool value;
        
        nsresult rv = prefBranch->GetBoolPref( (const char*) XPINSTALL_NOTIFICATIONS_ENABLE, &value);

        if (NS_SUCCEEDED(rv) && value)
        {
            
            
            

            PRInt32 intervalHours = 0;
            
            PRTime now            = 0;
            PRInt32 nowSec        = 0;

            PRInt32 lastTime      = 0;
            
            rv = prefBranch->GetIntPref(XPINSTALL_NOTIFICATIONS_INTERVAL, &intervalHours);

            if (NS_FAILED(rv))
            {
                intervalHours = 7*24;  
                rv = prefBranch->SetIntPref(XPINSTALL_NOTIFICATIONS_INTERVAL, intervalHours);
            }

            rv = prefBranch->GetIntPref(XPINSTALL_NOTIFICATIONS_LASTDATE, &lastTime);
    
            now = PR_Now();

            
            LL_DIV(nowSec, now, 1000000);

            if (NS_FAILED(rv) || lastTime == 0)
            {
                rv = prefBranch->SetIntPref(XPINSTALL_NOTIFICATIONS_LASTDATE, nowSec);
                return NS_OK;
            }
            
            if ((lastTime + (intervalHours*60*24)) <= nowSec)
            {
                *aReturn = PR_TRUE;
            }
        }
    }
    
    return NS_OK;
}

nsresult
nsXPINotifierImpl::Init()
{
    PRBool enabled;

    NotificationEnabled(&enabled);

    if (!enabled)
        return NS_ERROR_FAILURE;
    
    if (mNotifications == nsnull)
        return NS_ERROR_FAILURE;

    nsresult rv;
    nsCOMPtr<nsIRDFDataSource> distributors;
    nsCOMPtr<nsIRDFContainer> distributorsContainer;
    nsCOMPtr <nsISimpleEnumerator> distributorEnumerator;
    PRBool moreElements;
    
    
    rv = CallGetService(kRDFServiceCID, &mRDF);
    if (NS_FAILED(rv)) return rv;
    
    if (! kXPI_NotifierSources)
	{
	   mRDF->GetResource(NC_XPI_SOURCES,       &kXPI_NotifierSources);
       mRDF->GetResource(NC_XPI_PACKAGES,      &kXPI_NotifierPackages);
       mRDF->GetResource(NC_XPI_TITLE,         &kXPI_NotifierPackage_Title);
       mRDF->GetResource(NC_XPI_VERSION,       &kXPI_NotifierPackage_Version);
       mRDF->GetResource(NC_XPI_DESCRIPTION,   &kXPI_NotifierPackage_Description);
       mRDF->GetResource(NC_XPI_REGKEY,        &kXPI_NotifierPackage_RegKey);

       mRDF->GetResource(NC_RDF_NOTIFICATION_ROOT,  &kNC_NotificationRoot);
       
       mRDF->GetResource(NC_RDF_SOURCE,             &kNC_Source);
	   mRDF->GetResource(NC_RDF_NAME,               &kNC_Name);
	   mRDF->GetResource(NC_RDF_URL,                &kNC_URL);
	   mRDF->GetResource(NC_RDF_CHILD,              &kNC_Child);

	}

    rv = OpenRemoteDataSource(BASE_DATASOURCE_URL, PR_TRUE, getter_AddRefs(distributors));
    if (NS_FAILED(rv)) return rv;

    distributorsContainer = do_CreateInstance(kRDFContainerCID, &rv);
    if (NS_SUCCEEDED(rv))
    {
        rv = distributorsContainer->Init(distributors, kXPI_NotifierSources);
        
        if (NS_SUCCEEDED(rv))
        {
            rv = distributorsContainer->GetElements(getter_AddRefs(distributorEnumerator));

            if (NS_SUCCEEDED(rv))
            {
                distributorEnumerator->HasMoreElements(&moreElements);
                while (moreElements) 
                {
                    nsCOMPtr<nsISupports> i;
                    rv = distributorEnumerator->GetNext(getter_AddRefs(i));
                    if (NS_FAILED(rv)) break;

                    nsCOMPtr<nsIRDFResource> aDistributor(do_QueryInterface(i, &rv));
                    if (NS_FAILED(rv)) break;

                    char* uri;
                    nsCOMPtr<nsIRDFDataSource> remoteDatasource;
                    aDistributor->GetValue(&uri);

                    rv = OpenRemoteDataSource(uri, PR_FALSE, getter_AddRefs(remoteDatasource));
					nsMemory::Free(uri);
                    if (NS_FAILED(rv)) break;
                    
                    distributorEnumerator->HasMoreElements(&moreElements);
                }
            }
        }
    }
    return NS_OK;
}


PRBool 
nsXPINotifierImpl::IsNewerOrUninstalled(const char* regKey, const char* versionString)
{
    PRBool needJar = PR_FALSE;

    REGERR status = VR_ValidateComponent( (char*) regKey );

    if ( status == REGERR_NOFIND || status == REGERR_NOFILE )
    {
        
        
        needJar = PR_TRUE;
    }
    else
    {
        VERSION oldVersion;

        status = VR_GetVersion( (char*)regKey, &oldVersion );
        
        if ( status != REGERR_OK )
        {
            needJar = PR_TRUE;
        }
        else 
        {
            VERSION newVersion;

            StringToVersionNumbers(versionString, &(newVersion).major, &(newVersion).minor, &(newVersion).release, &(newVersion).build);
            
            if ( CompareVersions(&oldVersion, &newVersion) < 0 )
                needJar = PR_TRUE;
        }
    }
    return needJar;
}


PRInt32
nsXPINotifierImpl::CompareVersions(VERSION *oldversion, VERSION *newVersion)
{
    PRInt32 diff;
    
    if ( oldversion->major == newVersion->major ) 
    {
        if ( oldversion->minor == newVersion->minor ) 
        {
            if ( oldversion->release == newVersion->release ) 
            {
                if ( oldversion->build == newVersion->build )
                    diff = 0;
                else if ( oldversion->build > newVersion->build )
                    diff = 1;
                else
                    diff = -1;
            }
            else if ( oldversion->release > newVersion->release )
                diff = 1;
            else
                diff = -1;
        }
        else if (  oldversion->minor > newVersion->minor )
            diff = 1;
        else
            diff = -1;
    }
    else if ( oldversion->major > newVersion->major )
        diff = 1;
    else
        diff = -1;

    return diff;
}


void
nsXPINotifierImpl::StringToVersionNumbers(const nsString& version, int32 *aMajor, int32 *aMinor, int32 *aRelease, int32 *aBuild)    
{
    PRInt32 errorCode;

    int dot = version.FindChar('.', 0);
    
    if ( dot == -1 ) 
    {
        *aMajor = version.ToInteger(&errorCode);
    }
    else  
    {
        nsString majorStr;
        version.Mid(majorStr, 0, dot);
        *aMajor  = majorStr.ToInteger(&errorCode);

        int prev = dot+1;
        dot = version.FindChar('.',prev);
        if ( dot == -1 ) 
        {
            nsString minorStr;
            version.Mid(minorStr, prev, version.Length() - prev);
            *aMinor = minorStr.ToInteger(&errorCode);
        }
        else 
        {
            nsString minorStr;
            version.Mid(minorStr, prev, dot - prev);
            *aMinor = minorStr.ToInteger(&errorCode);

            prev = dot+1;
            dot = version.FindChar('.',prev);
            if ( dot == -1 ) 
            {
                nsString releaseStr;
                version.Mid(releaseStr, prev, version.Length() - prev);
                *aRelease = releaseStr.ToInteger(&errorCode);
            }
            else 
            {
                nsString releaseStr;
                version.Mid(releaseStr, prev, dot - prev);
                *aRelease = releaseStr.ToInteger(&errorCode);
    
                prev = dot+1;
                if ( version.Length() > dot ) 
                {
                    nsString buildStr;
                    version.Mid(buildStr, prev, version.Length() - prev);
                    *aBuild = buildStr.ToInteger(&errorCode);
               }
            }
        }
    }
}

nsresult
nsXPINotifierImpl::OpenRemoteDataSource(const char* aURL, PRBool blocking, nsIRDFDataSource** aResult)
{
    static NS_DEFINE_CID(kRDFXMLDataSourceCID, NS_RDFXMLDATASOURCE_CID);
    nsresult rv;

    nsCOMPtr<nsIRDFRemoteDataSource> remote = do_CreateInstance(kRDFXMLDataSourceCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = remote->Init(aURL);
    if (NS_SUCCEEDED(rv)) 
    {
        if (! blocking)
        {
            nsCOMPtr<nsIRDFXMLSink> sink = do_QueryInterface(remote, &rv);
            if (NS_FAILED(rv)) return rv;

            rv = sink->AddXMLSinkObserver(this);
            if (NS_FAILED(rv)) return rv;
        }

        rv = remote->Refresh(blocking);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIRDFDataSource> result = do_QueryInterface(remote, &rv);
        *aResult = result;
        NS_IF_ADDREF(*aResult);
        return rv;
    }
    else 
    {
        
        return mRDF->GetDataSource(aURL, aResult);
    }
}


NS_IMETHODIMP
nsXPINotifierImpl::New(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
    NS_PRECONDITION(aOuter == nsnull, "no aggregation");
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;

    nsXPINotifierImpl* result = new nsXPINotifierImpl();
    if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(result); 

    nsresult rv;
    rv = result->Init();
    if (NS_SUCCEEDED(rv)) {
        rv = result->QueryInterface(aIID, aResult);
    }

    NS_RELEASE(result);
    return rv;
}


NS_IMETHODIMP
nsXPINotifierImpl::OnBeginLoad(nsIRDFXMLSink *aSink)
{
    return NS_OK;
}

NS_IMETHODIMP
nsXPINotifierImpl::OnInterrupt(nsIRDFXMLSink *aSink)
{
    return NS_OK;
}
NS_IMETHODIMP
nsXPINotifierImpl::OnResume(nsIRDFXMLSink *aSink)
{
    return NS_OK;
}

NS_IMETHODIMP
nsXPINotifierImpl::OnEndLoad(nsIRDFXMLSink *aSink)
{
    nsresult rv;

    (void) aSink->RemoveXMLSinkObserver(this);

    nsCOMPtr<nsIRDFDataSource> distributorDataSource = do_QueryInterface(aSink, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr <nsISimpleEnumerator> packageEnumerator;
    PRBool moreElements;

    nsCOMPtr<nsIRDFContainer> distributorContainer =
            do_CreateInstance(kRDFContainerCID, &rv);
    if (NS_SUCCEEDED(rv))
    {
        rv = distributorContainer->Init(distributorDataSource, kXPI_NotifierPackages);
        if (NS_SUCCEEDED(rv))
        {
            rv = distributorContainer->GetElements(getter_AddRefs(packageEnumerator));
            if (NS_SUCCEEDED(rv))
            {
                packageEnumerator->HasMoreElements(&moreElements);
                while (moreElements) 
                {
                    nsCOMPtr<nsISupports> i;

                    rv = packageEnumerator->GetNext(getter_AddRefs(i));
                    if (NS_FAILED(rv)) break;

                    nsCOMPtr<nsIRDFResource> aPackage(do_QueryInterface(i, &rv));
                    if (NS_FAILED(rv)) break;
    
                    
                    
                    nsCOMPtr<nsIRDFNode> versionNode;
                    distributorDataSource->GetTarget(aPackage, 
                                                     kXPI_NotifierPackage_Version, 
                                                     PR_TRUE, 
                                                     getter_AddRefs(versionNode));

                    nsCOMPtr<nsIRDFLiteral> version(do_QueryInterface(versionNode, &rv));
                    if (NS_FAILED(rv)) break;

                    
                    nsCOMPtr<nsIRDFNode> regkeyNode;
                    distributorDataSource->GetTarget(aPackage, 
                                                     kXPI_NotifierPackage_RegKey, 
                                                     PR_TRUE, 
                                                     getter_AddRefs(regkeyNode));

                    nsCOMPtr<nsIRDFLiteral> regkey(do_QueryInterface(regkeyNode, &rv));
                    if (NS_FAILED(rv)) break;

                    
                    PRUnichar* regkeyCString;
                    regkey->GetValue(&regkeyCString);
                    nsString regKeyString(regkeyCString);
                    
                    PRUnichar* versionCString;
                    version->GetValue(&versionCString);
                    nsString versionString(versionCString);
					nsMemory::Free(versionCString);
					nsMemory::Free(regkeyCString);

                    
                    if (IsNewerOrUninstalled(NS_ConvertUTF16toUTF8(regKeyString).get(), NS_ConvertUTF16toUTF8(versionString).get()))
                    {
                        
                        
                        nsCOMPtr<nsIRDFNode> urlNode;
                        distributorDataSource->GetTarget(kXPI_NotifierPackages, 
                                                         kNC_URL, 
                                                         PR_TRUE, 
                                                         getter_AddRefs(urlNode));

                        nsCOMPtr<nsIRDFLiteral> url(do_QueryInterface(urlNode, &rv));
                        if (NS_FAILED(rv)) break;


                        nsCOMPtr<nsIRDFNode> titleNode;
                        distributorDataSource->GetTarget(kXPI_NotifierPackages, 
                                                         kXPI_NotifierPackage_Title, 
                                                         PR_TRUE, 
                                                         getter_AddRefs(titleNode));

                        nsCOMPtr<nsIRDFLiteral> title(do_QueryInterface(titleNode, &rv));
                        if (NS_FAILED(rv)) break;

                        nsCOMPtr<nsIRDFDataSource> ds = do_QueryInterface(mNotifications);

                        ds->Assert(aPackage, kNC_Name, title, PR_TRUE);
                        ds->Assert(aPackage, kNC_URL, url, PR_TRUE);

                        ds->Assert(kNC_NotificationRoot, kNC_Child, aPackage, PR_TRUE);
                        break;

                    }
                }
            }
        }
    }
	VR_Close();
    return NS_OK;
}


NS_IMETHODIMP
nsXPINotifierImpl::OnError(nsIRDFXMLSink *aSink, nsresult aResult, const PRUnichar* aErrorMsg)
{
    (void) aSink->RemoveXMLSinkObserver(this);
    return NS_OK;
}


NS_IMETHODIMP
nsXPINotifierImpl::DisplayUpdateDialog(void)
{
    nsresult rv;
    nsCOMPtr <nsISimpleEnumerator> packages;
    PRBool moreElements;

    nsCOMPtr<nsIRDFDataSource> ds = do_QueryInterface(mNotifications, &rv);

    if (NS_SUCCEEDED(rv))
    {
        rv = ds->GetAllResources(getter_AddRefs(packages));
        if (NS_SUCCEEDED(rv))
        {
            packages->HasMoreElements(&moreElements);
            while (moreElements) 
            {
                nsCOMPtr<nsISupports> i;

                rv = packages->GetNext(getter_AddRefs(i));
                if (NS_FAILED(rv)) break;

                nsCOMPtr<nsIRDFResource> aPackage(do_QueryInterface(i, &rv));
                if (NS_FAILED(rv)) break;

                
                
                nsCOMPtr<nsIRDFNode> name;
                ds->GetTarget(aPackage, 
                              nsXPINotifierImpl::kNC_Name, 
                              PR_TRUE, 
                              getter_AddRefs(name));

                nsCOMPtr<nsIRDFLiteral> nameLiteral = do_QueryInterface(name, &rv);
                if (NS_FAILED(rv)) break;
            }
        }
    }
    return rv;
}



