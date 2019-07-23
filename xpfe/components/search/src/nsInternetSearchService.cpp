









































#include "nsInternetSearchService.h"

#include "nscore.h"
#include "nsIEnumerator.h"
#include "nsIRDFObserver.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIServiceManager.h"
#include "nsVoidArray.h"  
#include "nsXPIDLString.h"
#include "plhash.h"
#include "plstr.h"
#include "prmem.h"
#include "prprf.h"
#include "prio.h"
#include "prlog.h"
#include "rdf.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsCOMArray.h"
#include "nsCRT.h"
#include "nsEnumeratorUtils.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"
#include "nsITextToSubURI.h"
#include "nsEscape.h"
#include "nsNetUtil.h"
#include "nsIChannel.h"
#include "nsIFileChannel.h"
#include "nsIHttpChannel.h"
#include "nsIUploadChannel.h"
#include "nsIInputStream.h"
#ifndef MOZ_PLACES_BOOKMARKS
#include "nsIBookmarksService.h"
#endif
#include "nsIStringBundle.h"
#include "nsIObserverService.h"
#include "nsIURL.h"
#include "nsILocalFile.h"
#include "nsUnicharUtils.h"
#include "nsReadableUtils.h"
#include "nsIPrefLocalizedString.h"

#ifdef  XP_WIN
#include "windef.h"
#include "winbase.h"
#endif

#ifdef  DEBUG


#endif

#define MAX_SEARCH_RESULTS_ALLOWED  100

#define POSTHEADER_PREFIX "Content-type: application/x-www-form-urlencoded\r\nContent-Length: "
#define POSTHEADER_SUFFIX "\r\n\r\n"
#define SEARCH_PROPERTIES "chrome://communicator/locale/search/search-panel.properties"
#ifdef MOZ_XUL_APP
#define SEARCHCONFIG_PROPERTIES "chrome://branding/content/searchconfig.properties"
#define INTL_PROPERTIES "chrome://global/locale/intl.properties"
#else
#define SEARCHCONFIG_PROPERTIES "chrome://navigator/content/searchconfig.properties"
#define INTL_PROPERTIES "chrome://navigator/locale/navigator.properties"
#endif

static NS_DEFINE_CID(kTextToSubURICID, NS_TEXTTOSUBURI_CID);

static const char kURINC_SearchEngineRoot[]                   = "NC:SearchEngineRoot";
static const char kURINC_SearchResultsSitesRoot[]             = "NC:SearchResultsSitesRoot";
static const char kURINC_LastSearchRoot[]                     = "NC:LastSearchRoot";
static const char kURINC_SearchCategoryRoot[]                 = "NC:SearchCategoryRoot";
static const char kURINC_SearchCategoryPrefix[]               = "NC:SearchCategory?category=";
static const char kURINC_SearchCategoryEnginePrefix[]         = "NC:SearchCategory?engine=";
static const char kURINC_SearchCategoryEngineBasenamePrefix[] = "NC:SearchCategory?engine=urn:search:engine:";
 
static const char kURINC_FilterSearchURLsRoot[]       = "NC:FilterSearchURLsRoot";
static const char kURINC_FilterSearchSitesRoot[]      = "NC:FilterSearchSitesRoot";
static const char kSearchCommand[]                    = "http://home.netscape.com/NC-rdf#command?";

int PR_CALLBACK searchModePrefCallback(const char *pref, void *aClosure);




static PRInt32 nsString_Find(const nsAString& aPattern,
                             const nsAString& aSource,
                             PRBool aIgnoreCase = PR_FALSE,
                             PRInt32 aOffset = 0, PRInt32 aCount = -1)
{
    nsAString::const_iterator start, end;
    aSource.BeginReading(start);
    aSource.EndReading(end);

    
    start.advance(aOffset);
    if (aCount>0) {
  end = start;    
  end.advance(aCount);
    }
    PRBool found;
    if (aIgnoreCase)
  found = FindInReadable(aPattern, start, end,
             nsCaseInsensitiveStringComparator());
    else
  found = FindInReadable(aPattern, start, end);

    if (!found)
  return kNotFound;

    nsAString::const_iterator originalStart;
    aSource.BeginReading(originalStart);
    return Distance(originalStart, start);
}

class InternetSearchContext : public nsIInternetSearchContext
{
public:
      InternetSearchContext(PRUint32 contextType, nsIRDFResource *aParent, nsIRDFResource *aEngine,
        nsIUnicodeDecoder *aUnicodeDecoder, const PRUnichar *hint);
  virtual   ~InternetSearchContext(void);
  NS_METHOD Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERNETSEARCHCONTEXT

private:
  PRUint32      mContextType;
  nsCOMPtr<nsIRDFResource>  mParent;
  nsCOMPtr<nsIRDFResource>  mEngine;
  nsCOMPtr<nsIUnicodeDecoder> mUnicodeDecoder;
  nsString      mBuffer;
  nsString      mHint;
};



InternetSearchContext::~InternetSearchContext(void)
{
}



InternetSearchContext::InternetSearchContext(PRUint32 contextType, nsIRDFResource *aParent, nsIRDFResource *aEngine,
        nsIUnicodeDecoder *aUnicodeDecoder, const PRUnichar *hint)
  : mContextType(contextType), mParent(aParent), mEngine(aEngine), mUnicodeDecoder(aUnicodeDecoder), mHint(hint)
{
}



NS_IMETHODIMP
InternetSearchContext::Init()
{
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchContext::GetContextType(PRUint32 *aContextType)
{
  *aContextType = mContextType;
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchContext::GetUnicodeDecoder(nsIUnicodeDecoder **decoder)
{
  *decoder = mUnicodeDecoder;
  NS_IF_ADDREF(*decoder);
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchContext::GetEngine(nsIRDFResource **node)
{
  *node = mEngine;
  NS_IF_ADDREF(*node);
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchContext::GetHintConst(const PRUnichar **hint)
{
  *hint = mHint.get();
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchContext::GetParent(nsIRDFResource **node)
{
  *node = mParent;
  NS_IF_ADDREF(*node);
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchContext::AppendBytes(const char *buffer, PRInt32 numBytes)
{
  mBuffer.Append(NS_ConvertASCIItoUTF16(Substring(buffer, buffer + numBytes)));
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchContext::AppendUnicodeBytes(const PRUnichar *buffer, PRInt32 numUniBytes)
{
  mBuffer.Append(buffer, numUniBytes);
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchContext::GetBufferConst(const PRUnichar **buffer)
{
  *buffer = mBuffer.get();
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchContext::GetBufferLength(PRInt32 *bufferLen)
{
  *bufferLen = mBuffer.Length();
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchContext::Truncate()
{
  mBuffer.Truncate();
  return(NS_OK);
}



NS_IMPL_THREADSAFE_ISUPPORTS1(InternetSearchContext, nsIInternetSearchContext)



nsresult
NS_NewInternetSearchContext(PRUint32 contextType, nsIRDFResource *aParent, nsIRDFResource *aEngine,
          nsIUnicodeDecoder *aUnicodeDecoder, const PRUnichar *hint, nsIInternetSearchContext **aResult)
{
   InternetSearchContext *result =
     new InternetSearchContext(contextType, aParent, aEngine, aUnicodeDecoder, hint);

   if (! result)
     return NS_ERROR_OUT_OF_MEMORY;

   nsresult rv = result->Init();
   if (NS_FAILED(rv)) {
     delete result;
     return rv;
   }

   NS_ADDREF(result);
   *aResult = result;
   return NS_OK;
}





static const char   kEngineProtocol[] = "engine://";
static const char   kSearchProtocol[] = "internetsearch:";

#ifdef  DEBUG_SEARCH_UPDATES
#define SEARCH_UPDATE_TIMEOUT 10000   // fire every 10 seconds
#else
#define SEARCH_UPDATE_TIMEOUT 60000   // fire every 60 seconds
#endif


int PR_CALLBACK
searchModePrefCallback(const char *pref, void *aClosure)
{
  InternetSearchDataSource *searchDS = NS_STATIC_CAST(InternetSearchDataSource *, aClosure);
  NS_ASSERTION(searchDS, "No closure?");

  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (!prefs)
    return NS_OK;

  prefs->GetIntPref(pref, &searchDS->mBrowserSearchMode);
#ifdef DEBUG
  printf("searchModePrefCallback: '%s' = %d\n",
         pref, searchDS->mBrowserSearchMode);
#endif
  searchDS->Assert(searchDS->mNC_LastSearchRoot, searchDS->mNC_LastSearchMode,
                   searchDS->mTrueLiteral, PR_TRUE);

  return NS_OK;
}

InternetSearchDataSource::InternetSearchDataSource(void) :
  mBrowserSearchMode(0),
  mEngineListBuilt(PR_FALSE),
#ifdef MOZ_PHOENIX
  mReorderedEngineList(PR_FALSE),
#endif
  mRDFService(do_GetService(NS_RDF_CONTRACTID "/rdf-service;1")),
  mRDFC(do_GetService(NS_RDF_CONTRACTID "/container-utils;1")),
  busySchedule(PR_FALSE)
{
  NS_ASSERTION(mRDFService, "No RDF Service");
  NS_ASSERTION(mRDFC, "No RDF Container Utils");
  
  mRDFService->GetResource(NS_LITERAL_CSTRING(kURINC_SearchEngineRoot),
                           getter_AddRefs(mNC_SearchEngineRoot));
  mRDFService->GetResource(NS_LITERAL_CSTRING(kURINC_LastSearchRoot),
                           getter_AddRefs(mNC_LastSearchRoot));
  mRDFService->GetResource(NS_LITERAL_CSTRING(kURINC_SearchResultsSitesRoot),
                           getter_AddRefs(mNC_SearchResultsSitesRoot));
  mRDFService->GetResource(NS_LITERAL_CSTRING(kURINC_FilterSearchURLsRoot),
                           getter_AddRefs(mNC_FilterSearchURLsRoot));
  mRDFService->GetResource(NS_LITERAL_CSTRING(kURINC_FilterSearchSitesRoot),
                           getter_AddRefs(mNC_FilterSearchSitesRoot));
  mRDFService->GetResource(NS_LITERAL_CSTRING(kURINC_SearchCategoryRoot),
                           getter_AddRefs(mNC_SearchCategoryRoot));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "SearchMode"),
                           getter_AddRefs(mNC_LastSearchMode));
 
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "searchtype"),
                           getter_AddRefs(mNC_SearchType));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "SearchResult"),
                           getter_AddRefs(mNC_SearchResult));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "ref"),
                           getter_AddRefs(mNC_Ref));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "child"),
                           getter_AddRefs(mNC_Child));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "title"),
                           getter_AddRefs(mNC_Title));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "data"),
                           getter_AddRefs(mNC_Data));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Name"),
                           getter_AddRefs(mNC_Name));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Description"),
                           getter_AddRefs(mNC_Description));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Version"),
                           getter_AddRefs(mNC_Version));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "actionButton"),
                           getter_AddRefs(mNC_actionButton));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "actionBar"),
                           getter_AddRefs(mNC_actionBar));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "searchForm"),
                           getter_AddRefs(mNC_searchForm));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "LastText"),
                           getter_AddRefs(mNC_LastText));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "URL"),
                           getter_AddRefs(mNC_URL));
  mRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "instanceOf"),
                           getter_AddRefs(mRDF_InstanceOf));
  mRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "type"),
                           getter_AddRefs(mRDF_type));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "loading"),
                           getter_AddRefs(mNC_loading));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "HTML"),
                           getter_AddRefs(mNC_HTML));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Icon"),
                           getter_AddRefs(mNC_Icon));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "StatusIcon"),
                           getter_AddRefs(mNC_StatusIcon));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Banner"),
                           getter_AddRefs(mNC_Banner));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Site"),
                           getter_AddRefs(mNC_Site));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Relevance"),
                           getter_AddRefs(mNC_Relevance));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Relevance?sort=true"),
                           getter_AddRefs(mNC_RelevanceSort));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Date"),
                           getter_AddRefs(mNC_Date));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "PageRank"),
                           getter_AddRefs(mNC_PageRank));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Engine"),
                           getter_AddRefs(mNC_Engine));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Price"),
                           getter_AddRefs(mNC_Price));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Price?sort=true"),
                           getter_AddRefs(mNC_PriceSort));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Availability"),
                           getter_AddRefs(mNC_Availability));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "BookmarkSeparator"),
                           getter_AddRefs(mNC_BookmarkSeparator));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Update"),
                           getter_AddRefs(mNC_Update));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "UpdateIcon"),
                           getter_AddRefs(mNC_UpdateIcon));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "UpdateCheckDays"),
                           getter_AddRefs(mNC_UpdateCheckDays));
  mRDFService->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastPingDate"),
                           getter_AddRefs(mWEB_LastPingDate));
  mRDFService->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastPingModDate"),
                           getter_AddRefs(mWEB_LastPingModDate));
  mRDFService->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastPingContentLen"),
                           getter_AddRefs(mWEB_LastPingContentLen));
 
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=addtobookmarks"),
                           getter_AddRefs(mNC_SearchCommand_AddToBookmarks));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=addquerytobookmarks"),
                           getter_AddRefs(mNC_SearchCommand_AddQueryToBookmarks));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=filterresult"),
                           getter_AddRefs(mNC_SearchCommand_FilterResult));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=filtersite"),
                           getter_AddRefs(mNC_SearchCommand_FilterSite));
  mRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=clearfilters"),
                           getter_AddRefs(mNC_SearchCommand_ClearFilters));
 
  mRDFService->GetLiteral(NS_LITERAL_STRING("true").get(),
                          getter_AddRefs(mTrueLiteral));

  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
  if (prefs) {
    prefs->RegisterCallback("browser.search.mode", searchModePrefCallback, this);
    prefs->GetIntPref("browser.search.mode", &mBrowserSearchMode);
  }
}



InternetSearchDataSource::~InternetSearchDataSource (void)
{
  if (mTimer) {
    
    
    mTimer->Cancel();
  }

  nsCOMPtr<nsIPref> prefs = do_GetService(NS_PREF_CONTRACTID);
  if (prefs)
    prefs->UnregisterCallback("browser.search.mode", searchModePrefCallback, this);

  if (mRDFService)
    mRDFService->UnregisterDataSource(this);
}



nsresult
InternetSearchDataSource::GetSearchEngineToPing(nsIRDFResource **theEngine, nsCString &updateURL)
{
  nsresult  rv = NS_OK;

  *theEngine = nsnull;
  updateURL.Truncate();

  if (!mUpdateArray)  return(NS_OK);

  PRUint32  numEngines = 0;
  if (NS_FAILED(rv = mUpdateArray->Count(&numEngines))) return(rv);
  if (numEngines < 1) return(NS_OK);

  nsCOMPtr<nsIRDFResource> aRes (do_QueryElementAt(mUpdateArray, 0));

  
  mUpdateArray->RemoveElementAt(0);

  if (aRes)
  {
    if (isSearchCategoryEngineURI(aRes))
    {
      nsCOMPtr<nsIRDFResource>  trueEngine;
      rv = resolveSearchCategoryEngineURI(aRes, getter_AddRefs(trueEngine));
      if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE)) return(rv);
      if (!trueEngine)  return(NS_RDF_NO_VALUE);

      aRes = trueEngine;
    }

    if (!aRes)  return(NS_OK);

    *theEngine = aRes.get();
    NS_ADDREF(*theEngine);

    
    nsCOMPtr<nsIRDFNode>  aNode;
    if (NS_SUCCEEDED(rv = mInner->GetTarget(aRes, mNC_Update, PR_TRUE, getter_AddRefs(aNode)))
      && (rv != NS_RDF_NO_VALUE))
    {
      nsCOMPtr<nsIRDFLiteral> aLiteral (do_QueryInterface(aNode));
      if (aLiteral)
      {
        const PRUnichar *updateUni = nsnull;
        aLiteral->GetValueConst(&updateUni);
        if (updateUni)
        {
          CopyUTF16toUTF8(nsDependentString(updateUni), updateURL);
        }
      }
    }
  }
  return(rv);
}



void
InternetSearchDataSource::FireTimer(nsITimer* aTimer, void* aClosure)
{
  InternetSearchDataSource *search = NS_STATIC_CAST(InternetSearchDataSource *, aClosure);
  if (!search)  return;

  if (!search->busySchedule)
  {
    nsresult      rv;
    nsCOMPtr<nsIRDFResource>  searchURI;
    nsCAutoString     updateURL;
    if (NS_FAILED(rv = search->GetSearchEngineToPing(getter_AddRefs(searchURI), updateURL)))
      return;
    if (!searchURI)     return;
    if (updateURL.IsEmpty())  return;

    search->busyResource = searchURI;

    nsCOMPtr<nsIInternetSearchContext>  engineContext;
    if (NS_FAILED(rv = NS_NewInternetSearchContext(nsIInternetSearchContext::ENGINE_UPDATE_HEAD_CONTEXT,
      nsnull, searchURI, nsnull, nsnull, getter_AddRefs(engineContext))))
      return;
    if (!engineContext) return;

    nsCOMPtr<nsIURI>  uri;
    if (NS_FAILED(rv = NS_NewURI(getter_AddRefs(uri), updateURL.get())))  return;

    nsCOMPtr<nsIChannel>  channel;
    if (NS_FAILED(rv = NS_NewChannel(getter_AddRefs(channel), uri, nsnull)))  return;

    channel->SetLoadFlags(nsIRequest::VALIDATE_ALWAYS);

    nsCOMPtr<nsIHttpChannel> httpChannel (do_QueryInterface(channel));
    if (!httpChannel) return;

    
        httpChannel->SetRequestMethod(NS_LITERAL_CSTRING("HEAD"));
    if (NS_SUCCEEDED(rv = channel->AsyncOpen(search, engineContext)))
    {
      search->busySchedule = PR_TRUE;

#ifdef  DEBUG_SEARCH_UPDATES
      printf("    InternetSearchDataSource::FireTimer - Pinging '%s'\n", (char *)updateURL.get());
#endif
    }
  }
#ifdef  DEBUG_SEARCH_UPDATES
else
  {
  printf("    InternetSearchDataSource::FireTimer - busy pinging.\n");
  }
#endif
}



PRBool
InternetSearchDataSource::isEngineURI(nsIRDFResource *r)
{
  PRBool    isEngineURIFlag = PR_FALSE;
  const char  *uri = nsnull;
  
  r->GetValueConst(&uri);
  if ((uri) && (!strncmp(uri, kEngineProtocol, sizeof(kEngineProtocol) - 1)))
  {
    isEngineURIFlag = PR_TRUE;
  }
  return(isEngineURIFlag);
}



PRBool
InternetSearchDataSource::isSearchURI(nsIRDFResource *r)
{
  PRBool    isSearchURIFlag = PR_FALSE;
  const char  *uri = nsnull;
  
  r->GetValueConst(&uri);
  if ((uri) && (!strncmp(uri, kSearchProtocol, sizeof(kSearchProtocol) - 1)))
  {
    isSearchURIFlag = PR_TRUE;
  }
  return(isSearchURIFlag);
}



PRBool
InternetSearchDataSource::isSearchCategoryURI(nsIRDFResource *r)
{
  PRBool    isSearchCategoryURIFlag = PR_FALSE;
  const char  *uri = nsnull;
  
  r->GetValueConst(&uri);
  if ((uri) && (!strncmp(uri, kURINC_SearchCategoryPrefix, sizeof(kURINC_SearchCategoryPrefix) - 1)))
  {
    isSearchCategoryURIFlag = PR_TRUE;
  }
  return(isSearchCategoryURIFlag);
}



PRBool
InternetSearchDataSource::isSearchCategoryEngineURI(nsIRDFResource *r)
{
  PRBool    isSearchCategoryEngineURIFlag = PR_FALSE;
  const char  *uri = nsnull;
  
  r->GetValueConst(&uri);
  if ((uri) && (!strncmp(uri, kURINC_SearchCategoryEnginePrefix, sizeof(kURINC_SearchCategoryEnginePrefix) - 1)))
  {
    isSearchCategoryEngineURIFlag = PR_TRUE;
  }
  return(isSearchCategoryEngineURIFlag);
}



PRBool
InternetSearchDataSource::isSearchCategoryEngineBasenameURI(nsIRDFNode *r)
{
  PRBool    isSearchCategoryEngineBasenameURIFlag = PR_FALSE;

  nsCOMPtr<nsIRDFResource> aRes (do_QueryInterface(r));
  if (aRes)
  {
    const char  *uri = nsnull;
    aRes->GetValueConst(&uri);
    if ((uri) && (!nsCRT::strncmp(uri, kURINC_SearchCategoryEngineBasenamePrefix,
      (int)sizeof(kURINC_SearchCategoryEngineBasenamePrefix) - 1)))
    {
      isSearchCategoryEngineBasenameURIFlag = PR_TRUE;
    }
  }
  else
  {
    nsCOMPtr<nsIRDFLiteral> aLit (do_QueryInterface(r));
    if (aLit)
    {
      const PRUnichar *uriUni = nsnull;
      aLit->GetValueConst(&uriUni);
      if ((uriUni) && (!nsCRT::strncmp(uriUni,
               NS_ConvertASCIItoUTF16(kURINC_SearchCategoryEngineBasenamePrefix).get(),
        (int)sizeof(kURINC_SearchCategoryEngineBasenamePrefix) - 1)))
      {
        isSearchCategoryEngineBasenameURIFlag = PR_TRUE;
      }
    }
  }
  return(isSearchCategoryEngineBasenameURIFlag);
}



PRBool
InternetSearchDataSource::isSearchCommand(nsIRDFResource *r)
{
  PRBool    isSearchCommandFlag = PR_FALSE;
  const char  *uri = nsnull;
  
  if (NS_SUCCEEDED(r->GetValueConst( &uri )) && (uri))
  {
    if (!strncmp(uri, kSearchCommand, sizeof(kSearchCommand) - 1))
    {
      isSearchCommandFlag = PR_TRUE;
    }
  }
  return(isSearchCommandFlag);
}



nsresult
InternetSearchDataSource::resolveSearchCategoryEngineURI(nsIRDFResource *engine, nsIRDFResource **trueEngine)
{
  *trueEngine = nsnull;

  if ((!categoryDataSource) || (!mInner)) return(NS_ERROR_UNEXPECTED);

  nsresult  rv;
  const char  *uriUni = nsnull;
  if (NS_FAILED(rv = engine->GetValueConst(&uriUni))) return(rv);
  if (!uriUni)  return(NS_ERROR_NULL_POINTER);

  NS_ConvertUTF8toUTF16 uri(uriUni);
  if (uri.Find(kURINC_SearchCategoryEngineBasenamePrefix) !=0)  return(NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIRDFLiteral> basenameLiteral;
  if (NS_FAILED(rv = mRDFService->GetLiteral(uri.get(),
      getter_AddRefs(basenameLiteral))))  return(rv);

  nsCOMPtr<nsIRDFResource>  catSrc;
  rv = mInner->GetSource(mNC_URL, basenameLiteral, PR_TRUE, getter_AddRefs(catSrc));
  if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE)) return(rv);
  if (!catSrc)    return(NS_ERROR_UNEXPECTED);

  *trueEngine = catSrc;
  NS_IF_ADDREF(*trueEngine);
  return(NS_OK);
}





NS_IMPL_ADDREF(InternetSearchDataSource)
NS_IMPL_RELEASE(InternetSearchDataSource)

NS_INTERFACE_MAP_BEGIN(InternetSearchDataSource)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIInternetSearchService)
   NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
   NS_INTERFACE_MAP_ENTRY(nsIStreamListener)
   NS_INTERFACE_MAP_ENTRY(nsIInternetSearchService)
   NS_INTERFACE_MAP_ENTRY(nsIRDFDataSource)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END


NS_METHOD
InternetSearchDataSource::Init()
{
  nsresult rv;

  mInner = do_CreateInstance(NS_RDF_DATASOURCE_CONTRACTID_PREFIX "in-memory-datasource");
  NS_ENSURE_TRUE(mRDFService && mInner, NS_ERROR_FAILURE);

  
  rv = mRDFService->GetDataSource("rdf:local-store",
                                  getter_AddRefs(mLocalstore));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewISupportsArray(getter_AddRefs(mUpdateArray));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mRDFService->RegisterDataSource(this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewLoadGroup(getter_AddRefs(mLoadGroup), nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  busySchedule = PR_FALSE;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mTimer->InitWithFuncCallback(InternetSearchDataSource::FireTimer, this,
                               SEARCH_UPDATE_TIMEOUT,
                               nsITimer::TYPE_REPEATING_SLACK);
  
  

  mEngineListBuilt = PR_FALSE;

  
  nsCOMPtr<nsIObserverService> observerService = 
           do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  observerService->AddObserver(this, "profile-before-change", PR_TRUE);
  observerService->AddObserver(this, "profile-do-change", PR_TRUE);

  return NS_OK;
}

void
InternetSearchDataSource::DeferredInit()
{
  if (mEngineListBuilt)
    return;

  nsresult rv;

  nsCOMPtr<nsIProperties> dirSvc
    (do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID));
  if (!dirSvc)
    return;

  mEngineListBuilt = PR_TRUE;

  
  nsCOMPtr<nsISimpleEnumerator> dirlist;
  rv = dirSvc->Get(NS_APP_SEARCH_DIR_LIST,
                   NS_GET_IID(nsISimpleEnumerator), getter_AddRefs(dirlist));
  if (NS_SUCCEEDED(rv))
  {
    PRBool more;
    while (NS_SUCCEEDED(dirlist->HasMoreElements(&more)) && more) {
      nsCOMPtr<nsISupports> suppfile;
      nsCOMPtr<nsIFile> dir;
      dirlist->GetNext(getter_AddRefs(suppfile));
      dir = do_QueryInterface(suppfile);
      if (dir)
      {
        GetSearchEngineList(dir, PR_FALSE);
      }
    }
#ifdef MOZ_PHOENIX
    if (!mReorderedEngineList)
      ReorderEngineList();
#endif
  }

  
  GetCategoryList();
}

NS_IMETHODIMP
InternetSearchDataSource::GetURI(char **uri)
{
  NS_PRECONDITION(uri != nsnull, "null ptr");
  if (! uri)
    return NS_ERROR_NULL_POINTER;

  if ((*uri = nsCRT::strdup("rdf:internetsearch")) == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}



NS_IMETHODIMP
InternetSearchDataSource::GetSource(nsIRDFResource* property,
                                nsIRDFNode* target,
                                PRBool tv,
                                nsIRDFResource** source )
{
  NS_PRECONDITION(property != nsnull, "null ptr");
  if (! property)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(target != nsnull, "null ptr");
  if (! target)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  *source = nsnull;
  return NS_RDF_NO_VALUE;
}



NS_IMETHODIMP
InternetSearchDataSource::GetSources(nsIRDFResource *property,
                                 nsIRDFNode *target,
                                 PRBool tv,
                                 nsISimpleEnumerator **sources )
{
  nsresult  rv = NS_RDF_NO_VALUE;

  if (mInner)
  {
    rv = mInner->GetSources(property, target, tv, sources);
  }
  else
  {
    rv = NS_NewEmptyEnumerator(sources);
  }
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::GetTarget(nsIRDFResource *source,
                                nsIRDFResource *property,
                                PRBool tv,
                                nsIRDFNode **target )
{
  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(property != nsnull, "null ptr");
  if (! property)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(target != nsnull, "null ptr");
  if (! target)
    return NS_ERROR_NULL_POINTER;

  *target = nsnull;

  nsresult    rv = NS_RDF_NO_VALUE;

  
  if (! tv)
    return(rv);

  if ((isSearchCategoryURI(source)) && (categoryDataSource))
  {
    rv = categoryDataSource->GetTarget(source, property, tv, target);
    return(rv);
  }

  if (isSearchCategoryEngineURI(source))
  {
    nsCOMPtr<nsIRDFResource>  trueEngine;
    rv = resolveSearchCategoryEngineURI(source, getter_AddRefs(trueEngine));
    if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE)) return(rv);
    if (!trueEngine)  return(NS_RDF_NO_VALUE);

    source = trueEngine;
  }

  if (isSearchURI(source) && (property == mNC_Child))
  {
    
    
    *target = source;
    NS_ADDREF(source);
    return(NS_OK);
  }

  if (isSearchCommand(source) && (property == mNC_Name))
  {
    nsresult rv;
    nsCOMPtr<nsIStringBundleService>
    stringService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));

    if (NS_SUCCEEDED(rv) && stringService) {

      nsCOMPtr<nsIStringBundle> bundle;
      rv = stringService->CreateBundle(SEARCH_PROPERTIES, getter_AddRefs(bundle));
      if (NS_SUCCEEDED(rv) && bundle) {

        nsXPIDLString valUni;
        nsAutoString name;

        if (source == mNC_SearchCommand_AddToBookmarks)
          name.AssignLiteral("addtobookmarks");
        else if (source == mNC_SearchCommand_AddQueryToBookmarks)
          name.AssignLiteral("addquerytobookmarks");
        else if (source == mNC_SearchCommand_FilterResult)
          name.AssignLiteral("excludeurl");
        else if (source == mNC_SearchCommand_FilterSite)
          name.AssignLiteral("excludedomain");
        else if (source == mNC_SearchCommand_ClearFilters)
          name.AssignLiteral("clearfilters");

        rv = bundle->GetStringFromName(name.get(), getter_Copies(valUni));
        if (NS_SUCCEEDED(rv) && valUni && *valUni) {
          *target = nsnull;
          nsCOMPtr<nsIRDFLiteral> literal;
          if (NS_FAILED(rv = mRDFService->GetLiteral(valUni, getter_AddRefs(literal))))
            return rv;
          *target = literal;
          NS_IF_ADDREF(*target);
          return rv;
        }
      }
    }
  }

  if (isEngineURI(source))
  {
    
    nsCOMPtr<nsIRDFLiteral> dataLit;
    FindData(source, getter_AddRefs(dataLit));
  }

  if (mInner)
  {
    rv = mInner->GetTarget(source, property, tv, target);
  }

  return(rv);
}

#ifdef MOZ_PHOENIX
void
InternetSearchDataSource::ReorderEngineList()
{
  
  
  
  nsresult rv;
  nsCOMArray<nsIRDFResource> engineList;

  
  nsCOMPtr<nsISimpleEnumerator> engines;
  rv = GetTargets(mNC_SearchEngineRoot, mNC_Child, PR_TRUE, getter_AddRefs(engines));
  if (NS_FAILED(rv)) return; 

  do {
    PRBool hasMore;
    engines->HasMoreElements(&hasMore);
    if (!hasMore)
      break;

    nsCOMPtr<nsISupports> supp;
    engines->GetNext(getter_AddRefs(supp));
    nsCOMPtr<nsIRDFResource> engineResource(do_QueryInterface(supp));

    nsCOMPtr<nsIRDFLiteral> data;
    FindData(engineResource, getter_AddRefs(data));
  }
  while (PR_TRUE);

  
  nsCOMPtr<nsIPrefBranch> pserv(do_GetService(NS_PREFSERVICE_CONTRACTID));
  char prefNameBuf[1096];
  PRInt32 i = 0; 
  do {
    ++i;
    sprintf(prefNameBuf, "browser.search.order.%d", i);

    nsCOMPtr<nsIPrefLocalizedString> engineName;
    rv = pserv->GetComplexValue(prefNameBuf, 
                                NS_GET_IID(nsIPrefLocalizedString),
                                getter_AddRefs(engineName));
    if (NS_FAILED(rv)) break;

    nsXPIDLString data;
    engineName->GetData(getter_Copies(data));

    nsCOMPtr<nsIRDFLiteral> engineNameLiteral;
    mRDFService->GetLiteral(data, getter_AddRefs(engineNameLiteral));

    nsCOMPtr<nsIRDFResource> engineResource;
    rv = mInner->GetSource(mNC_Name, engineNameLiteral, PR_TRUE, getter_AddRefs(engineResource));
    if (NS_FAILED(rv)) continue;

    engineList.AppendObject(engineResource);
  }
  while (PR_TRUE);

  
  rv = GetTargets(mNC_SearchEngineRoot, mNC_Child, PR_TRUE, getter_AddRefs(engines));
  if (NS_FAILED(rv)) return; 

  do {
    PRBool hasMore;
    engines->HasMoreElements(&hasMore);
    if (!hasMore)
      break;

    nsCOMPtr<nsISupports> supp;
    engines->GetNext(getter_AddRefs(supp));
    nsCOMPtr<nsIRDFResource> engineResource(do_QueryInterface(supp));

    if (engineList.IndexOfObject(engineResource) == -1) 
      engineList.AppendObject(engineResource);

    
    
    Unassert(mNC_SearchEngineRoot, mNC_Child, engineResource);
  }
  while (PR_TRUE);

  PRInt32 engineCount = engineList.Count();
  for (i = 0; i < engineCount; ++i)
    Assert(mNC_SearchEngineRoot, mNC_Child, engineList[i], PR_TRUE);

  mReorderedEngineList = PR_TRUE;
}
#endif

NS_IMETHODIMP
InternetSearchDataSource::GetTargets(nsIRDFResource *source,
                           nsIRDFResource *property,
                           PRBool tv,
                           nsISimpleEnumerator **targets )
{
  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(property != nsnull, "null ptr");
  if (! property)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(targets != nsnull, "null ptr");
  if (! targets)
    return NS_ERROR_NULL_POINTER;

  nsresult rv = NS_RDF_NO_VALUE;

  
  if (! tv)
    return(rv);

  if ((isSearchCategoryURI(source)) && (categoryDataSource))
  {
    rv = categoryDataSource->GetTargets(source, property, tv, targets);
    return(rv);
  }

  if (isSearchCategoryEngineURI(source))
  {
    nsCOMPtr<nsIRDFResource>  trueEngine;
    rv = resolveSearchCategoryEngineURI(source, getter_AddRefs(trueEngine));
    if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE)) return(rv);
    if (!trueEngine)  return(NS_RDF_NO_VALUE);

    source = trueEngine;
  }

  if (mInner)
  { 
    
    if ((source == mNC_SearchEngineRoot || isSearchURI(source)) &&
        property == mNC_Child && !mEngineListBuilt)
    {
      DeferredInit();
    }

    rv = mInner->GetTargets(source, property, tv, targets);
  }
  if (isSearchURI(source))
  {
    if (property == mNC_Child)
    {
      PRBool    doNetworkRequest = PR_TRUE;
      if (NS_SUCCEEDED(rv) && (targets))
      {
        
        
        
        PRBool    hasResults = PR_FALSE;
        if (NS_SUCCEEDED((*targets)->HasMoreElements(&hasResults)) &&
            hasResults)
        {
          doNetworkRequest = PR_FALSE;
        }
      }
      BeginSearchRequest(source, doNetworkRequest);
    }
  }
  return(rv);
}



nsresult
InternetSearchDataSource::GetCategoryList()
{
  nsresult rv;

  categoryDataSource =
    do_CreateInstance(NS_RDF_DATASOURCE_CONTRACTID_PREFIX "xml-datasource");
  NS_ENSURE_TRUE(categoryDataSource, NS_ERROR_FAILURE);

  nsCOMPtr<nsIRDFRemoteDataSource> remoteCategoryDataSource (do_QueryInterface(categoryDataSource));
  if (!remoteCategoryDataSource)  return(NS_ERROR_UNEXPECTED);

  
    
  nsCOMPtr<nsIFile> searchFile;
  nsCAutoString searchFileURLSpec;

  rv = NS_GetSpecialDirectory(NS_APP_SEARCH_50_FILE, getter_AddRefs(searchFile));
  if (NS_FAILED(rv)) return rv;
  NS_GetURLSpecFromFile(searchFile, searchFileURLSpec);
  if (NS_FAILED(rv)) return rv;
  rv = remoteCategoryDataSource->Init(searchFileURLSpec.get());
  if (NS_FAILED(rv)) return rv;
    
  
  rv = remoteCategoryDataSource->Refresh(PR_TRUE);
  if (NS_FAILED(rv))  return(rv);

  

  PRBool        isDirtyFlag = PR_FALSE;
  nsCOMPtr<nsIRDFContainer>   categoryRoot;
  rv = mRDFC->MakeSeq(categoryDataSource, mNC_SearchCategoryRoot, getter_AddRefs(categoryRoot));
  if (NS_FAILED(rv))  return(rv);
  if (!categoryRoot)  return(NS_ERROR_UNEXPECTED);

  rv = categoryRoot->Init(categoryDataSource, mNC_SearchCategoryRoot);
  if (NS_FAILED(rv))  return(rv);

  PRInt32   numCategories = 0;
  rv = categoryRoot->GetCount(&numCategories);
  if (NS_FAILED(rv))  return(rv);

  
  for (PRInt32 catLoop=1; catLoop <= numCategories; catLoop++)
  {
    nsCOMPtr<nsIRDFResource>  aCategoryOrdinal;
    rv = mRDFC->IndexToOrdinalResource(catLoop,
      getter_AddRefs(aCategoryOrdinal));
    if (NS_FAILED(rv))  break;
    if (!aCategoryOrdinal)  break;

    nsCOMPtr<nsIRDFNode>    aCategoryNode;
    rv = categoryDataSource->GetTarget(mNC_SearchCategoryRoot, aCategoryOrdinal,
      PR_TRUE, getter_AddRefs(aCategoryNode));
    if (NS_FAILED(rv))  break;
    nsCOMPtr<nsIRDFResource> aCategoryRes (do_QueryInterface(aCategoryNode));
    if (!aCategoryRes)  break;
    const char      *catResURI = nsnull;
    aCategoryRes->GetValueConst(&catResURI);
    if (!catResURI)   break;
    nsCAutoString    categoryStr;
    categoryStr.AssignLiteral(kURINC_SearchCategoryPrefix);
    categoryStr.Append(catResURI);

    nsCOMPtr<nsIRDFResource>  searchCategoryRes;
    rv = mRDFService->GetResource(categoryStr, getter_AddRefs(searchCategoryRes));
    if (NS_FAILED(rv))
      break; 

    nsCOMPtr<nsIRDFContainer> categoryContainer;
    rv = mRDFC->MakeSeq(categoryDataSource, searchCategoryRes,
      getter_AddRefs(categoryContainer));
    if (NS_FAILED(rv))  continue;

    rv = categoryContainer->Init(categoryDataSource, searchCategoryRes);
    if (NS_FAILED(rv))  return(rv);

    PRInt32   numEngines = 0;
    rv = categoryContainer->GetCount(&numEngines);
    if (NS_FAILED(rv))  break;

    
    for (PRInt32 engineLoop=numEngines; engineLoop >= 1; engineLoop--)
    {
      nsCOMPtr<nsIRDFResource>  aEngineOrdinal;
      rv = mRDFC->IndexToOrdinalResource(engineLoop,
        getter_AddRefs(aEngineOrdinal));
      if (NS_FAILED(rv))  break;
      if (!aEngineOrdinal)  break;

      nsCOMPtr<nsIRDFNode>    aEngineNode;
      rv = categoryDataSource->GetTarget(searchCategoryRes, aEngineOrdinal,
        PR_TRUE, getter_AddRefs(aEngineNode));
      if (NS_FAILED(rv))  break;
      nsCOMPtr<nsIRDFResource> aEngineRes (do_QueryInterface(aEngineNode));
      if (!aEngineRes)  break;

      if (isSearchCategoryEngineURI(aEngineRes))
      {
        nsCOMPtr<nsIRDFResource>  trueEngine;
        rv = resolveSearchCategoryEngineURI(aEngineRes,
          getter_AddRefs(trueEngine));
        if (NS_FAILED(rv) || (!trueEngine))
        {
          
#ifdef  DEBUG
          const char    *catEngineURI = nsnull;
          aEngineRes->GetValueConst(&catEngineURI);
          if (catEngineURI)
          {
            printf("**** Stale search engine reference to '%s'\n",
              catEngineURI);
          }
#endif
          nsCOMPtr<nsIRDFNode>  staleCatEngine;
          rv = categoryContainer->RemoveElementAt(engineLoop, PR_TRUE,
            getter_AddRefs(staleCatEngine));
          isDirtyFlag = PR_TRUE;
        }
      }
    }
  }

  if (isDirtyFlag)
  {
    if (remoteCategoryDataSource)
    {
      remoteCategoryDataSource->Flush();
    }
  }

  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::Assert(nsIRDFResource *source,
                       nsIRDFResource *property,
                       nsIRDFNode *target,
                       PRBool tv)
{
  nsresult  rv = NS_RDF_ASSERTION_REJECTED;

  
  if (! tv)
    return(rv);

  if (mInner)
  {
    rv = mInner->Assert(source, property, target, tv);
  }
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::Unassert(nsIRDFResource *source,
                         nsIRDFResource *property,
                         nsIRDFNode *target)
{
  nsresult  rv = NS_RDF_ASSERTION_REJECTED;

  if (mInner)
  {
    rv = mInner->Unassert(source, property, target);
  }
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::Change(nsIRDFResource *source,
      nsIRDFResource *property,
      nsIRDFNode *oldTarget,
      nsIRDFNode *newTarget)
{
  nsresult  rv = NS_RDF_ASSERTION_REJECTED;

  if (mInner)
  {
    rv = mInner->Change(source, property, oldTarget, newTarget);
  }
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::Move(nsIRDFResource *oldSource,
             nsIRDFResource *newSource,
             nsIRDFResource *property,
             nsIRDFNode *target)
{
  nsresult  rv = NS_RDF_ASSERTION_REJECTED;

  if (mInner)
  {
    rv = mInner->Move(oldSource, newSource, property, target);
  }
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::HasAssertion(nsIRDFResource *source,
                             nsIRDFResource *property,
                             nsIRDFNode *target,
                             PRBool tv,
                             PRBool *hasAssertion )
{
  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(property != nsnull, "null ptr");
  if (! property)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(target != nsnull, "null ptr");
  if (! target)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(hasAssertion != nsnull, "null ptr");
  if (! hasAssertion)
    return NS_ERROR_NULL_POINTER;

  *hasAssertion = PR_FALSE;

  
  if (! tv)
  {
    return NS_OK;
        }
        nsresult  rv = NS_RDF_NO_VALUE;
        
        if (mInner)
        {
    rv = mInner->HasAssertion(source, property, target, tv, hasAssertion);
  }
        return(rv);
}

NS_IMETHODIMP 
InternetSearchDataSource::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *result)
{
    if (mInner) {
  return mInner->HasArcIn(aNode, aArc, result);
    }
    else {
  *result = PR_FALSE;
    }
    return NS_OK;
}

NS_IMETHODIMP 
InternetSearchDataSource::HasArcOut(nsIRDFResource *source, nsIRDFResource *aArc, PRBool *result)
{
    NS_PRECONDITION(source != nsnull, "null ptr");
    if (! source)
  return NS_ERROR_NULL_POINTER;

    nsresult rv;

    if ((source == mNC_SearchEngineRoot) || (source == mNC_LastSearchRoot) || isSearchURI(source))
    {
  *result = (aArc == mNC_Child);
  return NS_OK;
    }

    if ((isSearchCategoryURI(source)) && (categoryDataSource))
    {
  const char  *uri = nsnull;
  source->GetValueConst(&uri);
  if (!uri) return(NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIRDFResource>  category;
  if (NS_FAILED(rv = mRDFService->GetResource(nsDependentCString(uri),
                getter_AddRefs(category))))
      return(rv);

  return categoryDataSource->HasArcOut(source, aArc, result);
    }

    if (isSearchCategoryEngineURI(source))
    {
  nsCOMPtr<nsIRDFResource>  trueEngine;
  rv = resolveSearchCategoryEngineURI(source, getter_AddRefs(trueEngine));
  if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE)) return(rv);
  if (!trueEngine) {
      *result = PR_FALSE;
      return NS_OK;
  }
  source = trueEngine;
    }

    if (isEngineURI(source))
    {
  
  nsCOMPtr<nsIRDFLiteral> dataLit;
  FindData(source, getter_AddRefs(dataLit));
    }

    if (mInner)
    {
  return mInner->HasArcOut(source, aArc, result);
    }

    *result = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
InternetSearchDataSource::ArcLabelsIn(nsIRDFNode *node,
                            nsISimpleEnumerator ** labels )
{
  nsresult  rv;

  if (mInner)
  {
    rv = mInner->ArcLabelsIn(node, labels);
    return(rv);
  }
  else
  {
    rv = NS_NewEmptyEnumerator(labels);
  }
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::ArcLabelsOut(nsIRDFResource *source,
                             nsISimpleEnumerator **labels )
{
  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(labels != nsnull, "null ptr");
  if (! labels)
    return NS_ERROR_NULL_POINTER;

  nsresult rv;

  if ((source == mNC_SearchEngineRoot) || (source == mNC_LastSearchRoot) || isSearchURI(source))
  {
            nsCOMPtr<nsISupportsArray> array;
            rv = NS_NewISupportsArray(getter_AddRefs(array));
            if (NS_FAILED(rv)) return rv;

            array->AppendElement(mNC_Child);

            return NS_NewArrayEnumerator(labels, array);
  }

  if ((isSearchCategoryURI(source)) && (categoryDataSource))
  {
    const char  *uri = nsnull;
    source->GetValueConst(&uri);
    if (!uri) return(NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIRDFResource>  category;
    if (NS_FAILED(rv = mRDFService->GetResource(nsDependentCString(uri),
      getter_AddRefs(category))))
      return(rv);

    rv = categoryDataSource->ArcLabelsOut(category, labels);
    return(rv);
  }

  if (isSearchCategoryEngineURI(source))
  {
    nsCOMPtr<nsIRDFResource>  trueEngine;
    rv = resolveSearchCategoryEngineURI(source, getter_AddRefs(trueEngine));
    if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE)) return(rv);
    if (!trueEngine)  return(NS_RDF_NO_VALUE);

    source = trueEngine;
  }

  if (isEngineURI(source))
  {
    
    nsCOMPtr<nsIRDFLiteral> dataLit;
    FindData(source, getter_AddRefs(dataLit));
  }

  if (mInner)
  {
    rv = mInner->ArcLabelsOut(source, labels);
    return(rv);
  }

  return NS_NewEmptyEnumerator(labels);
}



NS_IMETHODIMP
InternetSearchDataSource::GetAllResources(nsISimpleEnumerator** aCursor)
{
  nsresult  rv = NS_RDF_NO_VALUE;

  if (mInner)
  {
    rv = mInner->GetAllResources(aCursor);
  }
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::AddObserver(nsIRDFObserver *aObserver)
{
  nsresult  rv = NS_OK;

  if (mInner)
  {
    rv = mInner->AddObserver(aObserver);
  }
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::RemoveObserver(nsIRDFObserver *aObserver)
{
  nsresult  rv = NS_OK;

  if (mInner)
  {
    rv = mInner->RemoveObserver(aObserver);
  }
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::GetAllCmds(nsIRDFResource* source,
                                     nsISimpleEnumerator** commands)
{
  nsCOMPtr<nsISupportsArray>  cmdArray;
  nsresult      rv;
  rv = NS_NewISupportsArray(getter_AddRefs(cmdArray));
  if (NS_FAILED(rv))  return(rv);

  
  PRBool        haveFilters = PR_FALSE;

  if (mLocalstore)
  {
    nsCOMPtr<nsISimpleEnumerator> cursor;
    PRBool        hasMore = PR_FALSE;

    
    if (NS_SUCCEEDED(rv = mLocalstore->GetTargets(mNC_FilterSearchURLsRoot, mNC_Child,
      PR_TRUE, getter_AddRefs(cursor))))
    {
      if (NS_SUCCEEDED(cursor->HasMoreElements(&hasMore)) && hasMore)
      {
        haveFilters = PR_TRUE;
      }
    }
    if (!haveFilters)
    {
      
      if (NS_SUCCEEDED(rv = mLocalstore->GetTargets(mNC_FilterSearchSitesRoot, mNC_Child,
        PR_TRUE, getter_AddRefs(cursor))))
      {
        if (NS_SUCCEEDED(cursor->HasMoreElements(&hasMore)) && hasMore)
        {
          haveFilters = PR_TRUE;
        }
      }
    }
  }

  PRBool        isSearchResult = PR_FALSE;
  rv = mInner->HasAssertion(source, mRDF_type, mNC_SearchResult, PR_TRUE,
          &isSearchResult);
  if (NS_SUCCEEDED(rv) && isSearchResult)
  {
#ifndef MOZ_PLACES_BOOKMARKS
    nsCOMPtr<nsIRDFDataSource>  datasource;
    if (NS_SUCCEEDED(rv = mRDFService->GetDataSource("rdf:bookmarks", getter_AddRefs(datasource))))
    {
      nsCOMPtr<nsIBookmarksService> bookmarks (do_QueryInterface(datasource));
      if (bookmarks)
      {
        nsAutoString uri;
        if (getSearchURI(source, uri))
        {
          PRBool  isBookmarkedFlag = PR_FALSE;
          rv = bookmarks->IsBookmarked(NS_ConvertUTF16toUTF8(uri).get(), &isBookmarkedFlag);
          if (NS_SUCCEEDED(rv) && !isBookmarkedFlag)
          {
            cmdArray->AppendElement(mNC_SearchCommand_AddToBookmarks);
          }
        }
      }
    }
#endif
    cmdArray->AppendElement(mNC_SearchCommand_AddQueryToBookmarks);
    cmdArray->AppendElement(mNC_BookmarkSeparator);

    
    PRBool        isURLFiltered = PR_FALSE;
    rv = mInner->HasAssertion(mNC_FilterSearchURLsRoot, mNC_Child, source,
                              PR_TRUE, &isURLFiltered);
    if (NS_SUCCEEDED(rv) && !isURLFiltered)
    {
      cmdArray->AppendElement(mNC_SearchCommand_FilterResult);
    }

    
    
    cmdArray->AppendElement(mNC_SearchCommand_FilterSite);

    if (haveFilters)
    {
      cmdArray->AppendElement(mNC_BookmarkSeparator);
      cmdArray->AppendElement(mNC_SearchCommand_ClearFilters);
    }
  }
  else if (isSearchURI(source) || (source == mNC_LastSearchRoot))
  {
    if (haveFilters)
    {
      cmdArray->AppendElement(mNC_SearchCommand_ClearFilters);
    }
  }

  
  cmdArray->AppendElement(mNC_BookmarkSeparator);

  return NS_NewArrayEnumerator(commands, cmdArray);
}



NS_IMETHODIMP
InternetSearchDataSource::IsCommandEnabled(nsISupportsArray* aSources,
                                       nsIRDFResource*   aCommand,
                                       nsISupportsArray* aArguments,
                                       PRBool* aResult)
{
  return(NS_ERROR_NOT_IMPLEMENTED);
}



PRBool
InternetSearchDataSource::getSearchURI(nsIRDFResource *src, nsAString &_retval)
{

  if (src)
  {
    nsresult    rv;
    nsCOMPtr<nsIRDFNode>  srcNode;
    if (NS_SUCCEEDED(rv = mInner->GetTarget(src, mNC_URL, PR_TRUE, getter_AddRefs(srcNode))))
    {
      nsCOMPtr<nsIRDFLiteral> urlLiteral (do_QueryInterface(srcNode));
      if (urlLiteral)
      {
        const PRUnichar *uriUni = nsnull;
        urlLiteral->GetValueConst(&uriUni);
        if (uriUni)
        {
          _retval.Assign(uriUni);
          return PR_TRUE;
        }
      }
    }
  }
  return PR_FALSE;
}



nsresult
InternetSearchDataSource::addToBookmarks(nsIRDFResource *src)
{
  if (!src) return(NS_ERROR_UNEXPECTED);
  if (!mInner)  return(NS_ERROR_UNEXPECTED);

  nsresult    rv;
  nsCOMPtr<nsIRDFNode>  nameNode;
  
  nsCOMPtr<nsIRDFLiteral> nameLiteral;
  const PRUnichar   *name = nsnull;
  if (NS_SUCCEEDED(rv = mInner->GetTarget(src, mNC_Name, PR_TRUE, getter_AddRefs(nameNode))))
  {
    nameLiteral = do_QueryInterface(nameNode);
    if (nameLiteral)
    {
      nameLiteral->GetValueConst(&name);
    }
  }

#ifndef MOZ_PLACES_BOOKMARKS
  nsCOMPtr<nsIRDFDataSource>  datasource;
  if (NS_SUCCEEDED(rv = mRDFService->GetDataSource("rdf:bookmarks", getter_AddRefs(datasource))))
  {
    nsCOMPtr<nsIBookmarksService> bookmarks (do_QueryInterface(datasource));
    if (bookmarks)
    {
      nsAutoString uri;
      if (getSearchURI(src, uri))
      {
        rv = bookmarks->AddBookmarkImmediately(uri.get(),
                                               name, nsIBookmarksService::BOOKMARK_SEARCH_TYPE, nsnull);
      }
    }
  }
#endif

  return(NS_OK);
}



nsresult
InternetSearchDataSource::addQueryToBookmarks(nsIRDFResource *src)
{
  if (!src) return(NS_ERROR_UNEXPECTED);
  if (!mInner)  return(NS_ERROR_UNEXPECTED);

  nsresult rv;
  nsCOMPtr<nsIRDFNode>  refNode;
  if (NS_FAILED(rv = mInner->GetTarget(mNC_LastSearchRoot, mNC_Ref, PR_TRUE,
    getter_AddRefs(refNode))))
    return(rv);
  nsCOMPtr<nsIRDFLiteral> urlLiteral (do_QueryInterface(refNode));
  if (!urlLiteral)
    return(NS_ERROR_UNEXPECTED);
  const PRUnichar *uriUni = nsnull;
  urlLiteral->GetValueConst(&uriUni);

  nsCOMPtr<nsIRDFNode>  textNode;
  if (NS_FAILED(rv = mInner->GetTarget(mNC_LastSearchRoot, mNC_LastText, PR_TRUE,
    getter_AddRefs(textNode))))
    return(rv);
  nsCOMPtr<nsIRDFLiteral> textLiteral = do_QueryInterface(textNode);
  nsXPIDLString value;
  if (textLiteral)
  {
    const PRUnichar *textUni = nsnull;
    textLiteral->GetValueConst(&textUni);
    nsAutoString name;
    name.Assign(textUni);
    
    name.ReplaceChar(PRUnichar('+'), PRUnichar(' '));

    nsCOMPtr<nsIStringBundleService>
    stringService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv) && stringService)
    {
      nsCOMPtr<nsIStringBundle> bundle;
      rv = stringService->CreateBundle(SEARCH_PROPERTIES, getter_AddRefs(bundle));
      if (bundle)
      {
        const PRUnichar *strings[] = { name.get() };
        rv = bundle->FormatStringFromName(NS_LITERAL_STRING("searchTitle").get(), strings, 1, 
          getter_Copies(value));
      }
    }
  }

#ifndef MOZ_PLACES_BOOKMARKS
  nsCOMPtr<nsIRDFDataSource>  datasource;
  if (NS_SUCCEEDED(rv = mRDFService->GetDataSource("rdf:bookmarks", getter_AddRefs(datasource))))
  {
    nsCOMPtr<nsIBookmarksService> bookmarks (do_QueryInterface(datasource));
    if (bookmarks)
      rv = bookmarks->AddBookmarkImmediately(uriUni, value.get(),
                                             nsIBookmarksService::BOOKMARK_SEARCH_TYPE, nsnull);
  }
#endif

  return(NS_OK);
}



nsresult
InternetSearchDataSource::filterResult(nsIRDFResource *aResource)
{
  if (!aResource) return(NS_ERROR_UNEXPECTED);
  if (!mInner)  return(NS_ERROR_UNEXPECTED);

  
  nsAutoString url;
  if (!getSearchURI(aResource, url))
  {
    return NS_ERROR_UNEXPECTED;
  }

  nsresult      rv;
  nsCOMPtr<nsIRDFLiteral> urlLiteral;
  if (NS_FAILED(rv = mRDFService->GetLiteral(url.get(), getter_AddRefs(urlLiteral)))
    || (urlLiteral == nsnull))  return(NS_ERROR_UNEXPECTED);

  
  PRBool  alreadyFiltered = PR_FALSE;
  rv = mLocalstore->HasAssertion(mNC_FilterSearchURLsRoot, mNC_Child,
         urlLiteral, PR_TRUE, &alreadyFiltered);
  if (NS_SUCCEEDED(rv) && alreadyFiltered)
  {
    
    return(rv);
  }

  
  mLocalstore->Assert(mNC_FilterSearchURLsRoot, mNC_Child, urlLiteral, PR_TRUE);

  
  nsCOMPtr<nsIRDFRemoteDataSource> remoteLocalStore (do_QueryInterface(mLocalstore));
  if (remoteLocalStore)
  {
    remoteLocalStore->Flush();
  }

  nsCOMPtr<nsISimpleEnumerator> anonArcs;
  if (NS_SUCCEEDED(rv = mInner->GetSources(mNC_URL, urlLiteral, PR_TRUE,
    getter_AddRefs(anonArcs))))
  {
    PRBool      hasMoreAnonArcs = PR_TRUE;
    while (hasMoreAnonArcs)
    {
      if (NS_FAILED(anonArcs->HasMoreElements(&hasMoreAnonArcs)) ||
          !hasMoreAnonArcs)
        break;
      nsCOMPtr<nsISupports> anonArc;
      if (NS_FAILED(anonArcs->GetNext(getter_AddRefs(anonArc))))
        break;
      nsCOMPtr<nsIRDFResource> anonChild (do_QueryInterface(anonArc));
      if (!anonChild) continue;

      PRBool  isSearchResult = PR_FALSE;
      rv = mInner->HasAssertion(anonChild, mRDF_type, mNC_SearchResult,
                                PR_TRUE, &isSearchResult);
      if (NS_FAILED(rv) || !isSearchResult)
        continue;

      nsCOMPtr<nsIRDFResource>  anonParent;
      if (NS_FAILED(rv = mInner->GetSource(mNC_Child, anonChild, PR_TRUE,
        getter_AddRefs(anonParent)))) continue;
      if (!anonParent)  continue;

      mInner->Unassert(anonParent, mNC_Child, anonChild);
    }
  }

  return(NS_OK);
}



nsresult
InternetSearchDataSource::filterSite(nsIRDFResource *aResource)
{
  if (!aResource) return(NS_ERROR_UNEXPECTED);
  if (!mInner)  return(NS_ERROR_UNEXPECTED);

  nsAutoString host;
  if (!getSearchURI(aResource, host))
  {
    return NS_ERROR_UNEXPECTED;
  }

  
  PRInt32   slashOffset1 = host.Find("://");
  if (slashOffset1 < 1)     return(NS_ERROR_UNEXPECTED);
  PRInt32   slashOffset2 = host.FindChar(PRUnichar('/'), slashOffset1 + 3);
  if (slashOffset2 <= slashOffset1) return(NS_ERROR_UNEXPECTED);
  host.SetLength(slashOffset2 + 1);

  nsresult      rv;
  nsCOMPtr<nsIRDFLiteral> urlLiteral;
  if (NS_FAILED(rv = mRDFService->GetLiteral(host.get(), getter_AddRefs(urlLiteral)))
    || (urlLiteral == nsnull))  return(NS_ERROR_UNEXPECTED);

  
  PRBool  alreadyFiltered = PR_FALSE;
  rv = mLocalstore->HasAssertion(mNC_FilterSearchSitesRoot, mNC_Child,
         urlLiteral, PR_TRUE, &alreadyFiltered);
  if (NS_SUCCEEDED(rv) && alreadyFiltered)
  {
    
    return(rv);
  }

  
  mLocalstore->Assert(mNC_FilterSearchSitesRoot, mNC_Child, urlLiteral, PR_TRUE);

  
  nsCOMPtr<nsIRDFRemoteDataSource> remoteLocalStore (do_QueryInterface(mLocalstore));
  if (remoteLocalStore)
  {
    remoteLocalStore->Flush();
  }

  

  nsCOMPtr<nsISupportsArray>  array;
  nsCOMPtr<nsIRDFResource>  aRes;
  nsCOMPtr<nsISimpleEnumerator> cursor;
  PRBool        hasMore;

  rv = NS_NewISupportsArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) return rv;

  if (NS_FAILED(rv = GetAllResources(getter_AddRefs(cursor))))  return(rv);
  if (!cursor)  return(NS_ERROR_UNEXPECTED);

  hasMore = PR_TRUE;
  while (hasMore)
  {
    if (NS_FAILED(rv = cursor->HasMoreElements(&hasMore)))  return(rv);
    if (!hasMore)
      break;
    
    nsCOMPtr<nsISupports> isupports;
    if (NS_FAILED(rv = cursor->GetNext(getter_AddRefs(isupports))))
        return(rv);
    if (!isupports) return(NS_ERROR_UNEXPECTED);
    aRes = do_QueryInterface(isupports);
    if (!aRes)  return(NS_ERROR_UNEXPECTED);

    if ((aRes.get() == mNC_LastSearchRoot) || (isSearchURI(aRes)))
    {
      array->AppendElement(aRes);
    }
  }

  PRUint32  count;
  if (NS_FAILED(rv = array->Count(&count))) return(rv);
  for (PRUint32 loop=0; loop<count; loop++)
  {
    nsCOMPtr<nsIRDFResource> aSearchRoot (do_QueryElementAt(array, loop));
    if (!aSearchRoot) break;

    if (NS_SUCCEEDED(rv = mInner->GetTargets(aSearchRoot, mNC_Child,
      PR_TRUE, getter_AddRefs(cursor))))
    {
      hasMore = PR_TRUE;
      while (hasMore)
      {
        if (NS_FAILED(cursor->HasMoreElements(&hasMore)) || !hasMore)
          break;

        nsCOMPtr<nsISupports>   arc;
        if (NS_FAILED(cursor->GetNext(getter_AddRefs(arc))))
          break;
        aRes = do_QueryInterface(arc);
        if (!aRes)  break;

        nsAutoString site;
        if (!getSearchURI(aRes, site))
        {
          return NS_ERROR_UNEXPECTED;
        }

        
        slashOffset1 = site.Find("://");
        if (slashOffset1 < 1)     return(NS_ERROR_UNEXPECTED);
        slashOffset2 = site.FindChar(PRUnichar('/'), slashOffset1 + 3);
        if (slashOffset2 <= slashOffset1) return(NS_ERROR_UNEXPECTED);
        site.SetLength(slashOffset2 + 1);

        if (site.Equals(host, nsCaseInsensitiveStringComparator()))
        {
          mInner->Unassert(aSearchRoot, mNC_Child, aRes);
        }
      }
    }
  }

  return(NS_OK);
}



nsresult
InternetSearchDataSource::clearFilters(void)
{
  if (!mInner)  return(NS_ERROR_UNEXPECTED);

  nsresult      rv;
  nsCOMPtr<nsISimpleEnumerator> arcs;
  PRBool        hasMore = PR_TRUE;
  nsCOMPtr<nsISupports>   arc;

  
  if (NS_SUCCEEDED(rv = mLocalstore->GetTargets(mNC_FilterSearchURLsRoot, mNC_Child,
    PR_TRUE, getter_AddRefs(arcs))))
  {
    hasMore = PR_TRUE;
    while (hasMore)
    {
      if (NS_FAILED(arcs->HasMoreElements(&hasMore)) || !hasMore)
        break;
      if (NS_FAILED(arcs->GetNext(getter_AddRefs(arc))))
        break;

      nsCOMPtr<nsIRDFLiteral> filterURL (do_QueryInterface(arc));
      if (!filterURL) continue;
      
      mLocalstore->Unassert(mNC_FilterSearchURLsRoot, mNC_Child, filterURL);
    }
  }

  
  if (NS_SUCCEEDED(rv = mLocalstore->GetTargets(mNC_FilterSearchSitesRoot, mNC_Child,
    PR_TRUE, getter_AddRefs(arcs))))
  {
    hasMore = PR_TRUE;
    while (hasMore)
    {
      if (NS_FAILED(arcs->HasMoreElements(&hasMore)) || !hasMore)
        break;
      if (NS_FAILED(arcs->GetNext(getter_AddRefs(arc))))
        break;

      nsCOMPtr<nsIRDFLiteral> filterSiteLiteral (do_QueryInterface(arc));
      if (!filterSiteLiteral) continue;
      
      mLocalstore->Unassert(mNC_FilterSearchSitesRoot, mNC_Child, filterSiteLiteral);
    }
  }

  
  nsCOMPtr<nsIRDFRemoteDataSource> remoteLocalStore (do_QueryInterface(mLocalstore));
  if (remoteLocalStore)
  {
    remoteLocalStore->Flush();
  }

  return(NS_OK);
}



PRBool
InternetSearchDataSource::isSearchResultFiltered(const nsString &hrefStr)
{
  PRBool    filterStatus = PR_FALSE;
  nsresult  rv;

  const PRUnichar *hrefUni = hrefStr.get();
  if (!hrefUni) return(filterStatus);

  
  nsCOMPtr<nsIRDFLiteral> hrefLiteral;
  if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(hrefUni, getter_AddRefs(hrefLiteral))))
  {
    if (NS_SUCCEEDED(rv = mLocalstore->HasAssertion(mNC_FilterSearchURLsRoot,
      mNC_Child, hrefLiteral, PR_TRUE, &filterStatus)))
    {
      if (filterStatus)
      {
        return(filterStatus);
      }
    }
  }

  

  
  nsAutoString  host(hrefStr);
  PRInt32   slashOffset1 = host.Find("://");
  if (slashOffset1 < 1)     return(NS_ERROR_UNEXPECTED);
  PRInt32   slashOffset2 = host.FindChar(PRUnichar('/'), slashOffset1 + 3);
  if (slashOffset2 <= slashOffset1) return(NS_ERROR_UNEXPECTED);
  host.SetLength(slashOffset2 + 1);

  nsCOMPtr<nsIRDFLiteral> urlLiteral;
  if (NS_FAILED(rv = mRDFService->GetLiteral(host.get(), getter_AddRefs(urlLiteral)))
    || (urlLiteral == nsnull))  return(NS_ERROR_UNEXPECTED);

  rv = mLocalstore->HasAssertion(mNC_FilterSearchSitesRoot, mNC_Child, urlLiteral,
    PR_TRUE, &filterStatus);

  return(filterStatus);
}



NS_IMETHODIMP
InternetSearchDataSource::DoCommand(nsISupportsArray* aSources,
                                nsIRDFResource*   aCommand,
                                nsISupportsArray* aArguments)
{
  nsresult    rv = NS_OK;
  PRInt32     loop;
  PRUint32    numSources;
  if (NS_FAILED(rv = aSources->Count(&numSources))) return(rv);
  if (numSources < 1)
  {
    return(NS_ERROR_ILLEGAL_VALUE);
  }

  for (loop=((PRInt32)numSources)-1; loop>=0; loop--)
  {
    nsCOMPtr<nsIRDFResource> src (do_QueryElementAt(aSources, loop));
    if (!src) return(NS_ERROR_NO_INTERFACE);

    if (aCommand == mNC_SearchCommand_AddToBookmarks)
    {
      if (NS_FAILED(rv = addToBookmarks(src)))
        return(rv);
    }
    else if (aCommand == mNC_SearchCommand_AddQueryToBookmarks)
    {
      if (NS_FAILED(rv = addQueryToBookmarks(src)))
        return(rv);
    }
    else if (aCommand == mNC_SearchCommand_FilterResult)
    {
      if (NS_FAILED(rv = filterResult(src)))
        return(rv);
    }
    else if (aCommand == mNC_SearchCommand_FilterSite)
    {
      if (NS_FAILED(rv = filterSite(src)))
        return(rv);
    }
    else if (aCommand == mNC_SearchCommand_ClearFilters)
    {
      if (NS_FAILED(rv = clearFilters()))
        return(rv);
    }
  }
  return(NS_OK);
}

NS_IMETHODIMP
InternetSearchDataSource::BeginUpdateBatch()
{
        return mInner->BeginUpdateBatch();
}

NS_IMETHODIMP
InternetSearchDataSource::EndUpdateBatch()
{
        return mInner->EndUpdateBatch();
}

NS_IMETHODIMP
InternetSearchDataSource::AddSearchEngine(const char *engineURL, const char *iconURL,
            const PRUnichar *suggestedTitle, const PRUnichar *suggestedCategory)
{
        return AddSearchEngineInternal(engineURL, iconURL, suggestedTitle,
                                       suggestedCategory, nsnull);
}

nsresult
InternetSearchDataSource::AddSearchEngineInternal(const char *engineURL, const char *iconURL,
                                                  const PRUnichar *suggestedTitle,
                                                  const PRUnichar *suggestedCategory,
                                                  nsIRDFResource *aOldEngineResource)
{
  NS_PRECONDITION(engineURL != nsnull, "null ptr");
  if (!engineURL) return(NS_ERROR_NULL_POINTER);
  
  

#ifdef  DEBUG_SEARCH_OUTPUT
  printf("AddSearchEngine: engine='%s'\n", engineURL);
#endif

  nsresult  rv = NS_OK;

  
  if (!mBackgroundLoadGroup)
  {
    if (NS_FAILED(rv = NS_NewLoadGroup(getter_AddRefs(mBackgroundLoadGroup), nsnull)))
      return(rv);
    if (!mBackgroundLoadGroup)
      return(NS_ERROR_UNEXPECTED);
  }

  
  nsCOMPtr<nsIInternetSearchContext>  engineContext;
  rv = NS_NewInternetSearchContext
         (aOldEngineResource? nsIInternetSearchContext::ENGINE_DOWNLOAD_UPDATE_CONTEXT:
                              nsIInternetSearchContext::ENGINE_DOWNLOAD_NEW_CONTEXT,
          nsnull, aOldEngineResource, nsnull, suggestedCategory,
          getter_AddRefs(engineContext));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!engineContext) return(NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIURI>  engineURI;
  if (NS_FAILED(rv = NS_NewURI(getter_AddRefs(engineURI), engineURL)))
    return(rv);

  nsCOMPtr<nsIChannel>  engineChannel;
  if (NS_FAILED(rv = NS_NewChannel(getter_AddRefs(engineChannel), engineURI, nsnull, mBackgroundLoadGroup)))
    return(rv);
    
  if (NS_FAILED(rv = engineChannel->AsyncOpen(this, engineContext)))
    return(rv);

  
  nsCOMPtr<nsIInternetSearchContext>  iconContext;
  rv = NS_NewInternetSearchContext
         (aOldEngineResource? nsIInternetSearchContext::ICON_DOWNLOAD_UPDATE_CONTEXT:
                              nsIInternetSearchContext::ICON_DOWNLOAD_NEW_CONTEXT,
          nsnull, aOldEngineResource, nsnull, suggestedCategory,
          getter_AddRefs(iconContext));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!iconContext) return(NS_ERROR_UNEXPECTED);

  if (iconURL && (*iconURL))
  {
    nsCOMPtr<nsIURI>  iconURI;
    if (NS_FAILED(rv = NS_NewURI(getter_AddRefs(iconURI), iconURL)))
      return(rv);

    nsCOMPtr<nsIChannel>  iconChannel;
    if (NS_FAILED(rv = NS_NewChannel(getter_AddRefs(iconChannel), iconURI, nsnull, mBackgroundLoadGroup)))
      return(rv);
    if (NS_FAILED(rv = iconChannel->AsyncOpen(this, iconContext)))
      return(rv);
  }
  return(NS_OK);
}



nsresult
InternetSearchDataSource::saveContents(nsIChannel* channel, nsIInternetSearchContext *context, PRUint32 contextType)
{
    NS_ASSERTION(contextType == nsIInternetSearchContext::ENGINE_DOWNLOAD_NEW_CONTEXT ||
                 contextType == nsIInternetSearchContext::ICON_DOWNLOAD_NEW_CONTEXT ||
                 contextType == nsIInternetSearchContext::ENGINE_DOWNLOAD_UPDATE_CONTEXT ||
                 contextType == nsIInternetSearchContext::ICON_DOWNLOAD_UPDATE_CONTEXT,
                 "unexpected context");
  nsresult  rv = NS_OK;

  if (!channel) return(NS_ERROR_UNEXPECTED);
  if (!context) return(NS_ERROR_UNEXPECTED);

  
  nsCOMPtr<nsIURI>  channelURI;
  if (NS_FAILED(rv = channel->GetURI(getter_AddRefs(channelURI))))
    return(rv);
  if (!channelURI)
    return(NS_ERROR_NULL_POINTER);

  nsCAutoString baseName;
  if (NS_FAILED(rv = channelURI->GetSpec(baseName)))
    return(rv);

  PRInt32     slashOffset = baseName.RFindChar(PRUnichar('/'));
  if (slashOffset < 0)    return(NS_ERROR_UNEXPECTED);
  baseName.Cut(0, slashOffset+1);
  if (baseName.IsEmpty()) return(NS_ERROR_UNEXPECTED);

  
  PRInt32 extensionOffset;
  if (contextType == nsIInternetSearchContext::ENGINE_DOWNLOAD_NEW_CONTEXT ||
    contextType == nsIInternetSearchContext::ENGINE_DOWNLOAD_UPDATE_CONTEXT)
  {
    extensionOffset = baseName.RFind(".src", PR_TRUE);
    if ((extensionOffset < 0) || (extensionOffset != (PRInt32)(baseName.Length()-4)))
    {
      return(NS_ERROR_UNEXPECTED);
    }
  }

    nsCOMPtr<nsIFile> outFile;
  
  
  nsCOMPtr<nsIRDFResource> oldResource;
  rv = context->GetEngine(getter_AddRefs(oldResource));

  if (oldResource) {
    nsCOMPtr<nsILocalFile> oldEngineFile;
    rv = EngineFileFromResource(oldResource, getter_AddRefs(oldEngineFile));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = oldEngineFile->GetParent(getter_AddRefs(outFile));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = NS_GetSpecialDirectory(NS_APP_USER_SEARCH_DIR, getter_AddRefs(outFile));
    if (NS_FAILED(rv))
        return rv;
  }

    PRBool exists;
    rv = outFile->Exists(&exists);
    if (NS_FAILED(rv)) return(rv);
    if (!exists)
    {
        rv = outFile->Create(nsIFile::DIRECTORY_TYPE, 0755);
        if (NS_FAILED(rv)) return(rv);
    }

  const PRUnichar *dataBuf = nsnull;
  if (NS_FAILED(rv = context->GetBufferConst(&dataBuf)))  return(rv);

  
  
  PRInt32   bufferLength = 0;
  if (NS_FAILED(context->GetBufferLength(&bufferLength))) return(rv);
  if (bufferLength < 1) return(NS_OK);
  
  rv = outFile->Append(NS_ConvertUTF8toUTF16(baseName));
  if (NS_FAILED(rv)) return rv;
  
  
  
  
    
    
    
    
    if (contextType == nsIInternetSearchContext::ENGINE_DOWNLOAD_UPDATE_CONTEXT ||
        contextType == nsIInternetSearchContext::ICON_DOWNLOAD_UPDATE_CONTEXT) {
        
        outFile->Remove(PR_FALSE);
    } else {
        PRBool exists;
        rv = outFile->Exists(&exists);
        if (NS_FAILED(rv) || exists) {
            
            
            return NS_ERROR_UNEXPECTED;
        }
    }

    nsCOMPtr<nsIOutputStream> outputStream, fileOutputStream;
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(fileOutputStream), outFile);
    if (NS_FAILED(rv)) return rv;
    rv = NS_NewBufferedOutputStream(getter_AddRefs(outputStream), fileOutputStream, 4096);
    if (NS_FAILED(rv)) return rv;

    PRUint32 bytesWritten;
    for (PRInt32 loop=0; loop < bufferLength; loop++)
    {
        const char b = (const char) dataBuf[loop];
        outputStream->Write(&b, 1, &bytesWritten);
    }
    outputStream->Flush();    
    outputStream->Close();

    if (contextType == nsIInternetSearchContext::ENGINE_DOWNLOAD_NEW_CONTEXT ||
        contextType == nsIInternetSearchContext::ENGINE_DOWNLOAD_UPDATE_CONTEXT)
    {
        
        const PRUnichar *hintUni = nsnull;
        rv = context->GetHintConst(&hintUni);

        
        SaveEngineInfoIntoGraph(outFile, nsnull, hintUni, dataBuf, PR_FALSE);
    }
    else if (contextType == nsIInternetSearchContext::ICON_DOWNLOAD_NEW_CONTEXT ||
             contextType == nsIInternetSearchContext::ICON_DOWNLOAD_UPDATE_CONTEXT)
    {
        
        SaveEngineInfoIntoGraph(nsnull, outFile, nsnull, nsnull, PR_FALSE);
    }

  
  context->Truncate();

  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::GetInternetSearchURL(const char *searchEngineURI,
  const PRUnichar *searchStr, PRInt16 direction, PRUint16 pageNumber, 
  PRUint16 *whichButtons, char **resultURL)
{
  if (!resultURL) return(NS_ERROR_NULL_POINTER);
  *resultURL = nsnull;

  
  if (!mEngineListBuilt)
    DeferredInit();

  nsresult      rv;
  nsCOMPtr<nsIRDFResource>  engine;
  rv = mRDFService->GetResource(nsDependentCString(searchEngineURI),
                                getter_AddRefs(engine));
  NS_ENSURE_SUCCESS(rv, rv);

  validateEngine(engine);
  
  
  
  if (isSearchCategoryEngineURI(engine))
  {
    nsCOMPtr<nsIRDFResource>  trueEngine;
    rv = resolveSearchCategoryEngineURI(engine, getter_AddRefs(trueEngine));
    if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE)) return(rv);
    if (!trueEngine)  return(NS_RDF_NO_VALUE);

    engine = trueEngine;
  }

  nsCOMPtr<nsIRDFLiteral> dataLit;
  rv = FindData(engine, getter_AddRefs(dataLit));
  if (NS_FAILED(rv) ||
      (rv == NS_RDF_NO_VALUE))  return(rv);
  if (!dataLit) return(NS_ERROR_UNEXPECTED);

  const PRUnichar *dataUni = nsnull;
  dataLit->GetValueConst(&dataUni);
  if (!dataUni) return(NS_RDF_NO_VALUE);

  nsAutoString   text(searchStr), encodingStr, queryEncodingStr;

  
  
  GetData(dataUni, "search", 0, "queryCharset", queryEncodingStr);
       if (queryEncodingStr.IsEmpty())
  {
    GetData(dataUni, "search", 0, "queryEncoding", encodingStr);    
    MapEncoding(encodingStr, queryEncodingStr);
  }

  if (!queryEncodingStr.IsEmpty())
  {
    
    mQueryEncodingStr = queryEncodingStr;
    
    

    char  *utf8data = ToNewUTF8String(text);
    if (utf8data)
    {
      nsCOMPtr<nsITextToSubURI> textToSubURI = 
               do_GetService(kTextToSubURICID, &rv);
      if (NS_SUCCEEDED(rv) && (textToSubURI))
      {
        PRUnichar *uni = nsnull;
        if (NS_SUCCEEDED(rv = textToSubURI->UnEscapeAndConvert("UTF-8", utf8data, &uni)) && (uni))
        {
          char    *charsetData = nsnull;
          rv = textToSubURI->ConvertAndEscape(NS_LossyConvertUTF16toASCII(queryEncodingStr).get(),
                                              uni,
                                              &charsetData);
          if (NS_SUCCEEDED(rv) && charsetData)
          {
            CopyASCIItoUTF16(nsDependentCString(charsetData), text);
            NS_Free(charsetData);
          }
          NS_Free(uni);
        }
      }
      NS_Free(utf8data);
    }
  }
  
  nsAutoString  action, input, method, userVar, name;
  if (NS_FAILED(rv = GetData(dataUni, "search", 0, "action", action)))
      return(rv);

    
    if (!StringBeginsWith(action, NS_LITERAL_STRING("http:")) &&
        !StringBeginsWith(action, NS_LITERAL_STRING("https:")))
        return NS_ERROR_UNEXPECTED;

  if (NS_FAILED(rv = GetData(dataUni, "search", 0, "method", method)))
      return(rv);
  if (NS_FAILED(rv = GetData(dataUni, "search", 0, "name", name)))
      return(rv);
  if (NS_FAILED(rv = GetInputs(dataUni, name, userVar, text, input, direction, pageNumber, whichButtons)))
      return(rv);
  if (input.IsEmpty())        return(NS_ERROR_UNEXPECTED);

  
  if (!method.LowerCaseEqualsLiteral("get"))  return(NS_ERROR_UNEXPECTED);
  
  action += input;

  
  *resultURL = ToNewCString(action);
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchDataSource::RememberLastSearchText(const PRUnichar *escapedSearchStr)
{
  nsresult    rv;

  nsCOMPtr<nsIRDFNode>  textNode;
  if (NS_SUCCEEDED(rv = mInner->GetTarget(mNC_LastSearchRoot, mNC_LastText, PR_TRUE,
    getter_AddRefs(textNode))))
  {
    if (escapedSearchStr != nsnull)
    {
      nsresult    temprv;
      nsCOMPtr<nsIRDFLiteral> textLiteral;
      if (NS_SUCCEEDED(temprv = mRDFService->GetLiteral(escapedSearchStr, getter_AddRefs(textLiteral))))
      {
        if (rv != NS_RDF_NO_VALUE)
        {
          mInner->Change(mNC_LastSearchRoot, mNC_LastText, textNode, textLiteral);
        }
        else
        {
          mInner->Assert(mNC_LastSearchRoot, mNC_LastText, textLiteral, PR_TRUE);
        }
      }
    }
    else if (rv != NS_RDF_NO_VALUE)
    {
      rv = mInner->Unassert(mNC_LastSearchRoot, mNC_LastText, textNode);
    }
  }
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::FindInternetSearchResults(const char *url, PRBool *searchInProgress)
{
  *searchInProgress = PR_FALSE;

  if (!mInner)  return(NS_OK);

  
  
  nsAutoString    shortURL;
  CopyASCIItoUTF16(nsDependentCString(url), shortURL);
  PRInt32     optionsOffset;
  if ((optionsOffset = shortURL.FindChar(PRUnichar('?'))) < 0)  return(NS_OK);
  shortURL.SetLength(optionsOffset);

  
  if (!mEngineListBuilt)
    DeferredInit();

  
  
  PRBool        foundEngine = PR_FALSE;
  nsresult      rv;
  nsCOMPtr<nsIRDFResource>  engine;
  nsCOMPtr<nsISimpleEnumerator> arcs;
  nsAutoString      engineURI;
  nsCOMPtr<nsIRDFLiteral>   dataLit;
  const PRUnichar     *dataUni = nsnull;

  if (NS_SUCCEEDED(rv = mInner->GetTargets(mNC_SearchEngineRoot, mNC_Child,
    PR_TRUE, getter_AddRefs(arcs))))
  {
    PRBool      hasMore = PR_TRUE;
    while (hasMore)
    {
      if (NS_FAILED(arcs->HasMoreElements(&hasMore)) || !hasMore)
        break;
      nsCOMPtr<nsISupports> arc;
      if (NS_FAILED(arcs->GetNext(getter_AddRefs(arc))))
        break;

      engine = do_QueryInterface(arc);
      if (!engine)  continue;

      const char  *uri = nsnull;
      engine->GetValueConst(&uri);
      if (uri)
      {
        CopyUTF8toUTF16(nsDependentCString(uri), engineURI);
      }

      if (NS_FAILED(rv = FindData(engine, getter_AddRefs(dataLit))) ||
        (rv == NS_RDF_NO_VALUE))  continue;
      if (!dataLit) continue;

      dataLit->GetValueConst(&dataUni);
      if (!dataUni) continue;

      nsAutoString    action;
      if (NS_FAILED(rv = GetData(dataUni, "search", 0, "action", action)))  continue;
      if (shortURL.Equals(action, nsCaseInsensitiveStringComparator()))
      {
        foundEngine = PR_TRUE;
        break;
      }

      
      if (NS_FAILED(rv = GetData(dataUni, "browser", 0, "alsomatch", action)))  continue;
      if (nsString_Find(shortURL, action, PR_TRUE) >= 0)
      {
        foundEngine = PR_TRUE;
        break;
      }
    }
  }
  if (foundEngine)
  {
    nsAutoString  searchURL;
    CopyASCIItoUTF16(nsDependentCString(url), searchURL);

    
    nsAutoString  userVar, inputUnused, engineNameStr;
    GetData(dataUni, "search", 0, "name", engineNameStr);

    if (NS_FAILED(rv = GetInputs(dataUni, engineNameStr, userVar, EmptyString(), inputUnused, 0, 0, 0)))  return(rv);
    if (userVar.IsEmpty())  return(NS_RDF_NO_VALUE);

    nsAutoString  queryStr;
    queryStr.Assign(PRUnichar('?'));
    queryStr.Append(userVar);
    queryStr.Append(PRUnichar('='));

    PRInt32   queryOffset;
    if ((queryOffset = nsString_Find(queryStr, searchURL, PR_TRUE )) < 0)
    {
      queryStr.Replace(0, 1, PRUnichar('&'));
      queryOffset = nsString_Find(queryStr, searchURL, PR_TRUE);
    }

    nsAutoString  searchText;
    if (queryOffset >= 0)
    {
      PRInt32   andOffset;
      searchURL.Right(searchText, searchURL.Length() - queryOffset - queryStr.Length());

      if ((andOffset = searchText.FindChar(PRUnichar('&'))) >= 0)
      {
        searchText.SetLength(andOffset);
      }
    }
    if (!searchText.IsEmpty())
    {
      
      if (!mQueryEncodingStr.IsEmpty())
      {
        nsCOMPtr<nsITextToSubURI> textToSubURI = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
        if (NS_SUCCEEDED(rv))
        {
          nsCAutoString escapedSearchText;
          LossyCopyUTF16toASCII(searchText, escapedSearchText);

          
          escapedSearchText.ReplaceSubstring("%25", "%2B25");
          escapedSearchText.ReplaceSubstring("+", "%25");

          PRUnichar *uni = nsnull;
          rv = textToSubURI->UnEscapeAndConvert(NS_LossyConvertUTF16toASCII(mQueryEncodingStr).get(),
                                                escapedSearchText.get(), &uni);
          if (NS_SUCCEEDED(rv) && uni)
          {
            char  *convertedSearchText = nsnull;
            if (NS_SUCCEEDED(rv = textToSubURI->ConvertAndEscape("UTF-8", uni, &convertedSearchText)))
            {

              
              nsCAutoString unescapedSearchText(convertedSearchText);
              unescapedSearchText.ReplaceSubstring("%25", "+");
              unescapedSearchText.ReplaceSubstring("%2B25", "%25");

              CopyUTF8toUTF16(unescapedSearchText, searchText);

              NS_Free(convertedSearchText);
            }
            NS_Free(uni);
          }
        }
      }
      
      RememberLastSearchText(searchText.get());

      
      engineURI.Assign(NS_LITERAL_STRING("internetsearch:engine=") + engineURI +
                       NS_LITERAL_STRING("&text=") + searchText);

#ifdef  DEBUG_SEARCH_OUTPUT
      char  *engineMatch = ToNewCString(searchText);
      if (engineMatch)
      {
        printf("FindInternetSearchResults: search for: '%s'\n\n",
          engineMatch);
        NS_Free(engineMatch);
        engineMatch = nsnull;
      }
#endif
    }
    else
    {
      
      engineURI = searchURL;

      
      RememberLastSearchText(nsnull);
    }

    
    nsCOMPtr<nsIRDFNode>  oldNode;
    if (NS_SUCCEEDED(rv = mInner->GetTarget(mNC_LastSearchRoot, mNC_Ref, PR_TRUE,
      getter_AddRefs(oldNode))))
    {
      if (!engineURI.IsEmpty())
      {
        const PRUnichar *uriUni = engineURI.get();
        nsCOMPtr<nsIRDFLiteral> uriLiteral;
        nsresult    temprv;
        if ((uriUni) && (NS_SUCCEEDED(temprv = mRDFService->GetLiteral(uriUni,
          getter_AddRefs(uriLiteral)))))
        {
          if (rv != NS_RDF_NO_VALUE)
          {
            rv = mInner->Change(mNC_LastSearchRoot, mNC_Ref, oldNode, uriLiteral);
          }
          else
          {
            rv = mInner->Assert(mNC_LastSearchRoot, mNC_Ref, uriLiteral, PR_TRUE);
          }
        }
      }
      else
      {
        rv = mInner->Unassert(mNC_LastSearchRoot, mNC_Ref, oldNode);
      }
    }

    
    ClearResults(PR_FALSE);

    
    DoSearch(nsnull, engine, searchURL, EmptyString());

    *searchInProgress = PR_TRUE;
  }

  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchDataSource::ClearResults(PRBool flushLastSearchRef)
{
  if (!mInner)  return(NS_ERROR_UNEXPECTED);

  
  nsresult      rv;
  nsCOMPtr<nsISimpleEnumerator> arcs;
  if (NS_SUCCEEDED(rv = mInner->GetTargets(mNC_LastSearchRoot, mNC_Child, PR_TRUE, getter_AddRefs(arcs))))
  {
    PRBool      hasMore = PR_TRUE;
    while (hasMore)
    {
      if (NS_FAILED(arcs->HasMoreElements(&hasMore)) || !hasMore)
        break;
      nsCOMPtr<nsISupports> arc;
      if (NS_FAILED(arcs->GetNext(getter_AddRefs(arc))))
        break;
      nsCOMPtr<nsIRDFResource> child (do_QueryInterface(arc));
      if (child)
      {
        mInner->Unassert(mNC_LastSearchRoot, mNC_Child, child);
      }

      
      
      

      PRBool hasInArcs = PR_FALSE;
      nsCOMPtr<nsISimpleEnumerator> inArcs;
      if (NS_FAILED(mInner->ArcLabelsIn(child, getter_AddRefs(inArcs))) ||
        (!inArcs))
        continue;
      if (NS_FAILED(inArcs->HasMoreElements(&hasInArcs)) || hasInArcs)
        continue;

      

      nsCOMPtr<nsISimpleEnumerator> outArcs;
      if (NS_FAILED(mInner->ArcLabelsOut(child, getter_AddRefs(outArcs))) ||
        (!outArcs))
        continue;
      PRBool  hasMoreOutArcs = PR_TRUE;
      while (hasMoreOutArcs)
      {
        if (NS_FAILED(outArcs->HasMoreElements(&hasMoreOutArcs)) ||
            !hasMoreOutArcs)
          break;
        nsCOMPtr<nsISupports> outArc;
        if (NS_FAILED(outArcs->GetNext(getter_AddRefs(outArc))))
          break;
        nsCOMPtr<nsIRDFResource> property (do_QueryInterface(outArc));
        if (!property)
          continue;
        nsCOMPtr<nsIRDFNode> target;
        if (NS_FAILED(mInner->GetTarget(child, property, PR_TRUE,
          getter_AddRefs(target))) || (!target))
          continue;
        mInner->Unassert(child, property, target);
      }
    }
  }

  if (flushLastSearchRef)
  {
    
    nsCOMPtr<nsIRDFNode>  lastTarget;
    if (NS_SUCCEEDED(rv = mInner->GetTarget(mNC_LastSearchRoot, mNC_Ref,
      PR_TRUE, getter_AddRefs(lastTarget))) && (rv != NS_RDF_NO_VALUE))
    {
      nsCOMPtr<nsIRDFLiteral> lastLiteral (do_QueryInterface(lastTarget));
      if (lastLiteral)
      {
        rv = mInner->Unassert(mNC_LastSearchRoot, mNC_Ref, lastLiteral);
      }
    }
  }

  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchDataSource::ClearResultSearchSites(void)
{
  

  if (mInner)
  {
    nsresult      rv;
    nsCOMPtr<nsISimpleEnumerator> arcs;
    if (NS_SUCCEEDED(rv = mInner->GetTargets(mNC_SearchResultsSitesRoot, mNC_Child, PR_TRUE, getter_AddRefs(arcs))))
    {
      PRBool      hasMore = PR_TRUE;
      while (hasMore)
      {
        if (NS_FAILED(arcs->HasMoreElements(&hasMore)) || !hasMore)
          break;
        nsCOMPtr<nsISupports> arc;
        if (NS_FAILED(arcs->GetNext(getter_AddRefs(arc))))
          break;
        nsCOMPtr<nsIRDFResource> child (do_QueryInterface(arc));
        if (child)
        {
          mInner->Unassert(mNC_SearchResultsSitesRoot, mNC_Child, child);
        }
      }
    }
  }
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchDataSource::GetCategoryDataSource(nsIRDFDataSource **ds)
{
  nsresult  rv;

  if (!categoryDataSource)
  {
    if (NS_FAILED(rv = GetCategoryList()))
    {
      *ds = nsnull;
      return(rv);
    }
  }
  if (categoryDataSource)
  {
    *ds = categoryDataSource.get();
    NS_IF_ADDREF(*ds);
    return(NS_OK);
  }
  *ds = nsnull;
  return(NS_ERROR_FAILURE);
}



NS_IMETHODIMP
InternetSearchDataSource::Stop()
{
  nsresult    rv;

  
  if (mLoadGroup)
  {
    nsCOMPtr<nsISimpleEnumerator> requests;
    if (NS_SUCCEEDED(rv = mLoadGroup->GetRequests(getter_AddRefs(requests))))
    {
      PRBool      more;
      while (NS_SUCCEEDED(rv = requests->HasMoreElements(&more)) && more)
      {
        nsCOMPtr<nsISupports> isupports;
        if (NS_FAILED(rv = requests->GetNext(getter_AddRefs(isupports))))
          break;
        nsCOMPtr<nsIRequest> request (do_QueryInterface(isupports));
        if (!request) continue;
        request->Cancel(NS_BINDING_ABORTED);
      }
    }
    mLoadGroup->Cancel(NS_BINDING_ABORTED);
  }

  
  nsCOMPtr<nsISimpleEnumerator> arcs;
  if (NS_SUCCEEDED(rv = mInner->GetSources(mNC_loading, mTrueLiteral, PR_TRUE,
    getter_AddRefs(arcs))))
  {
    PRBool      hasMore = PR_TRUE;
    while (hasMore)
    {
      if (NS_FAILED(arcs->HasMoreElements(&hasMore)) || !hasMore)
        break;
      nsCOMPtr<nsISupports> arc;
      if (NS_FAILED(arcs->GetNext(getter_AddRefs(arc))))
        break;
      nsCOMPtr<nsIRDFResource> src (do_QueryInterface(arc));
      if (src)
      {
        mInner->Unassert(src, mNC_loading, mTrueLiteral);
      }
    }
  }

  return(NS_OK);
}



nsresult
InternetSearchDataSource::BeginSearchRequest(nsIRDFResource *source, PRBool doNetworkRequest)
{
        nsresult    rv = NS_OK;
  const char    *sourceURI = nsnull;

  if (NS_FAILED(rv = source->GetValueConst(&sourceURI)))
    return(rv);
  nsAutoString    uri;
  CopyUTF8toUTF16(nsDependentCString(sourceURI), uri);

  if (uri.Find("internetsearch:") != 0)
    return(NS_ERROR_FAILURE);

  
  ClearResults(PR_TRUE);

  
  ClearResultSearchSites();

  
  const PRUnichar *uriUni = uri.get();
  nsCOMPtr<nsIRDFLiteral> uriLiteral;
  if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(uriUni, getter_AddRefs(uriLiteral))))
  {
    rv = mInner->Assert(mNC_LastSearchRoot, mNC_Ref, uriLiteral, PR_TRUE);
  }

  uri.Cut(0, strlen("internetsearch:"));

  nsVoidArray *engineArray = new nsVoidArray;
  if (!engineArray)
    return(NS_ERROR_FAILURE);

  nsAutoString  text;

  

  while(!uri.IsEmpty())
  {
    nsAutoString  item;

    PRInt32 andOffset = uri.Find("&");
    if (andOffset >= 0)
    {
      uri.Left(item, andOffset);
      uri.Cut(0, andOffset + 1);
    }
    else
    {
      item = uri;
      uri.Truncate();
    }

    PRInt32 equalOffset = item.Find("=");
    if (equalOffset < 0)  break;
    
    nsAutoString  attrib, value;
    item.Left(attrib, equalOffset);
    value = item;
    value.Cut(0, equalOffset + 1);
    
    if (!attrib.IsEmpty() && !value.IsEmpty())
    {
      if (attrib.LowerCaseEqualsLiteral("engine"))
      {
        if ((value.Find(kEngineProtocol) == 0) ||
          (value.Find(kURINC_SearchCategoryEnginePrefix) == 0))
        {
          char  *val = ToNewCString(value);
          if (val)
          {
            engineArray->AppendElement(val);
          }
        }
      }
      else if (attrib.LowerCaseEqualsLiteral("text"))
      {
        text = value;
      }
    }
  }

  mInner->Assert(source, mNC_loading, mTrueLiteral, PR_TRUE);

  PRBool  requestInitiated = PR_FALSE;

  
  while (engineArray->Count() > 0)
  {
    char *baseFilename = (char *)(engineArray->ElementAt(0));
    engineArray->RemoveElementAt(0);
    if (!baseFilename)  continue;

#ifdef  DEBUG_SEARCH_OUTPUT
    printf("Search engine to query: '%s'\n", baseFilename);
#endif

    nsCOMPtr<nsIRDFResource>  engine;
    mRDFService->GetResource(nsDependentCString(baseFilename), getter_AddRefs(engine));
    nsCRT::free(baseFilename);
    baseFilename = nsnull;
    if (!engine)  continue;

    
    
    
    
    if (isSearchCategoryEngineURI(engine))
    {
      nsCOMPtr<nsIRDFResource>  trueEngine;
      rv = resolveSearchCategoryEngineURI(engine, getter_AddRefs(trueEngine));
      if (NS_FAILED(rv) || !trueEngine)
        continue;
      engine = trueEngine;
    }

    
    if (mInner)
    {
      mInner->Assert(mNC_SearchResultsSitesRoot, mNC_Child, engine, PR_TRUE);
    }

    if (doNetworkRequest)
    {
      DoSearch(source, engine, EmptyString(), text);
      requestInitiated = PR_TRUE;
    }
  }
  
  delete engineArray;
  engineArray = nsnull;

  if (!requestInitiated)
  {
    Stop();
  }

  return(rv);
}



nsresult
InternetSearchDataSource::FindData(nsIRDFResource *engine, nsIRDFLiteral **dataLit)
{
  if (!engine)  return(NS_ERROR_NULL_POINTER);
  if (!dataLit) return(NS_ERROR_NULL_POINTER);

  *dataLit = nsnull;

  if (!mInner)  return(NS_RDF_NO_VALUE);

  nsresult    rv;

  nsCOMPtr<nsIRDFNode>  dataTarget = nsnull;
  if (NS_SUCCEEDED((rv = mInner->GetTarget(engine, mNC_Data, PR_TRUE,
    getter_AddRefs(dataTarget)))) && (dataTarget))
  {
    nsCOMPtr<nsIRDFLiteral> aLiteral (do_QueryInterface(dataTarget));
    if (!aLiteral)
      return(NS_ERROR_UNEXPECTED);
    *dataLit = aLiteral;
    NS_IF_ADDREF(*dataLit);
    return(NS_OK);
  }

        nsCOMPtr<nsILocalFile> engineFile;
        rv = EngineFileFromResource(engine, getter_AddRefs(engineFile));
        if (NS_FAILED(rv)) return rv;

        nsString  data;
        rv = ReadFileContents(engineFile, data);

  if (NS_FAILED(rv))
  {
    return(rv);
  }

  
  if (data.IsEmpty()) return(NS_ERROR_UNEXPECTED);

  rv = updateDataHintsInGraph(engine, data.get());

  nsCOMPtr<nsIRDFLiteral> aLiteral;
  if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(data.get(), getter_AddRefs(aLiteral))))
  {
    *dataLit = aLiteral;
    NS_IF_ADDREF(*dataLit);
  }
  
  return(rv);
}

nsresult
InternetSearchDataSource::EngineFileFromResource(nsIRDFResource *aResource,
                                                 nsILocalFile **aResult)
{
  nsresult rv = NS_OK;

  
  const char *engineURI = nsnull;
  rv = aResource->GetValueConst(&engineURI);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString nativePath;
  nativePath.Assign(engineURI);

  NS_ENSURE_TRUE(StringBeginsWith(nativePath,
                                  NS_LITERAL_CSTRING(kEngineProtocol)),
                 NS_ERROR_FAILURE);
  nativePath.Cut(0, sizeof(kEngineProtocol) - 1);

  
  NS_UnescapeURL(nativePath);

#ifdef DEBUG_SEARCH_OUTPUT
  printf("InternetSearchDataSource::EngineFileFromResource\n"
         "File Path: %s\n",
         nativePath.get());
#endif

  rv = NS_NewNativeLocalFile(nativePath, PR_TRUE, aResult);

  return rv;
}

nsresult
InternetSearchDataSource::DecodeData(const char *aCharset, const PRUnichar *aInString, PRUnichar **aOutString)
{
  nsresult rv;
    
  nsCOMPtr <nsICharsetConverterManager> charsetConv = 
          do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder;
  rv = charsetConv->GetUnicodeDecoder(aCharset, getter_AddRefs(unicodeDecoder));

  
  if (NS_FAILED(rv))
    rv = charsetConv->GetUnicodeDecoderRaw("x-mac-roman", getter_AddRefs(unicodeDecoder));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  NS_LossyConvertUTF16toASCII value(aInString);

  PRInt32 srcLength = value.Length();
  PRInt32 outUnicodeLen;
  rv = unicodeDecoder->GetMaxLength(value.get(), srcLength, &outUnicodeLen);
  NS_ENSURE_SUCCESS(rv, rv);

  *aOutString = NS_REINTERPRET_CAST(PRUnichar*, nsMemory::Alloc((outUnicodeLen + 1) * sizeof(PRUnichar)));
  NS_ENSURE_TRUE(*aOutString, NS_ERROR_OUT_OF_MEMORY);

  rv = unicodeDecoder->Convert(value.get(), &srcLength, *aOutString, &outUnicodeLen);
  NS_ENSURE_SUCCESS(rv, rv);
  (*aOutString)[outUnicodeLen] = (PRUnichar)'\0';

  return rv;
}

nsresult
InternetSearchDataSource::updateDataHintsInGraph(nsIRDFResource *engine, const PRUnichar *dataUni)
{
  nsresult  rv = NS_OK;

  
  nsCOMPtr<nsIRDFLiteral> dataLiteral;
  if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(dataUni, getter_AddRefs(dataLiteral))))
  {
    updateAtom(mInner, engine, mNC_Data, dataLiteral, nsnull);
  }

  
  nsAutoString scriptCodeValue;
  const char * charsetName = MapScriptCodeToCharsetName(0);
  nsXPIDLString decodedValue;

  if (NS_SUCCEEDED(rv = GetData(dataUni, "search", 0, "sourceTextEncoding", scriptCodeValue)) && 
    !scriptCodeValue.IsEmpty())
  {
    PRInt32 err;
    PRInt32 scriptCode = scriptCodeValue.ToInteger(&err);
    if (NS_SUCCEEDED(err))
      charsetName = MapScriptCodeToCharsetName(scriptCode);
  }

  nsAutoString  nameValue;
  if (NS_SUCCEEDED(rv = GetData(dataUni, "search", 0, "name", nameValue)))
  {
    rv = DecodeData(charsetName, nameValue.get(), getter_Copies(decodedValue));
    nsCOMPtr<nsIRDFLiteral> nameLiteral;
    if (NS_SUCCEEDED(rv) &&
        NS_SUCCEEDED(rv = mRDFService->GetLiteral(decodedValue.get(),
                                                  getter_AddRefs(nameLiteral))))
    {
      rv = updateAtom(mInner, engine, mNC_Name, nameLiteral, nsnull);
    }
  }

  
  nsAutoString  descValue;
  if (NS_SUCCEEDED(rv = GetData(dataUni, "search", 0, "description", descValue)))
  {
    rv = DecodeData(charsetName, descValue.get(), getter_Copies(decodedValue));
    nsCOMPtr<nsIRDFLiteral> descLiteral;
    if (NS_SUCCEEDED(rv) &&
        NS_SUCCEEDED(rv = mRDFService->GetLiteral(decodedValue.get(),
                                                  getter_AddRefs(descLiteral))))
    {
      rv = updateAtom(mInner, engine, mNC_Description, descLiteral, nsnull);
    }
  }

  
  nsAutoString  versionValue;
  if (NS_SUCCEEDED(rv = GetData(dataUni, "search", 0, "version", versionValue)))
  {
    nsCOMPtr<nsIRDFLiteral> versionLiteral;
    if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(versionValue.get(),
        getter_AddRefs(versionLiteral))))
    {
      rv = updateAtom(mInner, engine, mNC_Version, versionLiteral, nsnull);
    }
  }

  nsAutoString  buttonValue;
  if (NS_SUCCEEDED(rv = GetData(dataUni, "search", 0, "actionButton", buttonValue)))
  {
    nsCOMPtr<nsIRDFLiteral> buttonLiteral;
    if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(buttonValue.get(),
        getter_AddRefs(buttonLiteral))))
    {
      rv = updateAtom(mInner, engine, mNC_actionButton, buttonLiteral, nsnull);
    }
  }

  nsAutoString  barValue;
  if (NS_SUCCEEDED(rv = GetData(dataUni, "search", 0, "actionBar", barValue)))
  {
    nsCOMPtr<nsIRDFLiteral> barLiteral;
    if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(barValue.get(),
        getter_AddRefs(barLiteral))))
    {
      rv = updateAtom(mInner, engine, mNC_actionBar, barLiteral, nsnull);
    }
  }

  nsAutoString  searchFormValue;
  if (NS_SUCCEEDED(rv = GetData(dataUni, "search", 0, "searchForm", searchFormValue)))
  {
    nsCOMPtr<nsIRDFLiteral> searchFormLiteral;
    if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(searchFormValue.get(),
        getter_AddRefs(searchFormLiteral))))
    {
      rv = updateAtom(mInner, engine, mNC_searchForm, searchFormLiteral, nsnull);
    }
  }


  PRBool  updatePrivateFiles = PR_FALSE;

  rv = mInner->HasAssertion(engine, mNC_SearchType, mNC_Engine, PR_TRUE,
                            &updatePrivateFiles);
  if (NS_SUCCEEDED(rv) && updatePrivateFiles)
  {
    
    
    
    
    
    
    
    
    
    

    
    
    nsAutoString  updateStr, updateIconStr, updateCheckDaysStr;

    GetData(dataUni, "browser", 0, "update", updateStr);
    if (updateStr.IsEmpty())
    {
      
      GetData(dataUni, "search", 0, "update", updateStr);

      
      nsAutoString  extension;
      updateStr.Right(extension, 4);
      if (extension.LowerCaseEqualsLiteral(".hqx"))
      {
        updateStr.SetLength(updateStr.Length() - 4);
      }

      
      updateStr.Right(extension, 4);
      if (!extension.LowerCaseEqualsLiteral(".src"))
      {
        
        updateStr.Truncate();
      }
    }
    else
    {
      
      GetData(dataUni, "browser", 0, "updateIcon", updateIconStr);
    }
    if (!updateStr.IsEmpty())
    {
      GetData(dataUni, "browser", 0, "updateCheckDays", updateCheckDaysStr);
      if (updateCheckDaysStr.IsEmpty())
      {
        
        GetData(dataUni, "search", 0, "updateCheckDays", updateCheckDaysStr);
      }
    }

    if (!updateStr.IsEmpty() && !updateCheckDaysStr.IsEmpty())
    {
      nsCOMPtr<nsIRDFLiteral> updateLiteral;
      if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(updateStr.get(),
          getter_AddRefs(updateLiteral))))
      {
        rv = updateAtom(mInner, engine, mNC_Update, updateLiteral, nsnull);
      }

      PRInt32 err;
      PRInt32 updateDays = updateCheckDaysStr.ToInteger(&err);
      if ((err) || (updateDays < 1))
      {
        
        updateDays = 3;
      }

      nsCOMPtr<nsIRDFInt> updateCheckDaysLiteral;
      if (NS_SUCCEEDED(rv = mRDFService->GetIntLiteral(updateDays,
          getter_AddRefs(updateCheckDaysLiteral))))
      {
        rv = updateAtom(mInner, engine, mNC_UpdateCheckDays, updateCheckDaysLiteral, nsnull);
      }

      if (!updateIconStr.IsEmpty())
      {
        nsCOMPtr<nsIRDFLiteral> updateIconLiteral;
        if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(updateIconStr.get(),
            getter_AddRefs(updateIconLiteral))))
        {
          rv = updateAtom(mInner, engine, mNC_UpdateIcon, updateIconLiteral, nsnull);
        }
      }
    }
  }

  return(rv);
}



nsresult
InternetSearchDataSource::updateAtom(nsIRDFDataSource *db, nsIRDFResource *src,
      nsIRDFResource *prop, nsIRDFNode *newValue, PRBool *dirtyFlag)
{
  nsresult    rv;
  nsCOMPtr<nsIRDFNode>  oldValue;

  if (dirtyFlag != nsnull)
  {
    *dirtyFlag = PR_FALSE;
  }

  if (NS_SUCCEEDED(rv = db->GetTarget(src, prop, PR_TRUE, getter_AddRefs(oldValue))) &&
    (rv != NS_RDF_NO_VALUE))
  {
    rv = db->Change(src, prop, oldValue, newValue);

    if ((oldValue.get() != newValue) && (dirtyFlag != nsnull))
    {
      *dirtyFlag = PR_TRUE;
    }
  }
  else
  {
    rv = db->Assert(src, prop, newValue, PR_TRUE);
    if (dirtyFlag != nsnull)
    {
        *dirtyFlag = PR_TRUE;
    }
  }
  return(rv);
}



struct  encodings
{
  const char  *numericEncoding;
  const char  *stringEncoding;
};



nsresult
InternetSearchDataSource::MapEncoding(const nsString &numericEncoding, 
                                      nsString &stringEncoding)
{
  

  struct  encodings encodingList[] =
  {
    { "0", "x-mac-roman"  },
    { "6", "x-mac-greek"  },
    { "35", "x-mac-turkish" },
    { "513", "ISO-8859-1" },
    { "514", "ISO-8859-2" },
    { "517", "ISO-8859-5" },
    { "518", "ISO-8859-6" },
    { "519", "ISO-8859-7" },
    { "520", "ISO-8859-8" },
    { "521", "ISO-8859-9" },
    { "1049", "IBM864"  },
    { "1280", "windows-1252"  },
    { "1281", "windows-1250"  },
    { "1282", "windows-1251"  },
    { "1283", "windows-1253"  },
    { "1284", "windows-1254"  },
    { "1285", "windows-1255"  },
    { "1286", "windows-1256"  },
    { "1536", "us-ascii"  },
    { "1584", "GB2312"  },
    { "1585", "x-gbk"   },
    { "1600", "EUC-KR"  },
    { "2080", "ISO-2022-JP" },
    { "2096", "ISO-2022-CN" },
    { "2112", "ISO-2022-KR" },
    { "2336", "EUC-JP"  },
    { "2352", "GB2312"  },
    { "2353", "x-euc-tw"  },
    { "2368", "EUC-KR"  },
    { "2561", "Shift_JIS" },
    { "2562", "KOI8-R"  },
    { "2563", "Big5"    },
    { "2565", "HZ-GB-2312"  },

    { nsnull, nsnull    }
  };

  if (!numericEncoding.IsEmpty()) {
    for (PRUint32 i = 0; encodingList[i].numericEncoding != nsnull; i++)
    {
      if (numericEncoding.EqualsASCII(encodingList[i].numericEncoding)) 
      {
        stringEncoding.AssignASCII(encodingList[i].stringEncoding);
        return NS_OK;
      }
    }
  }

  
  nsXPIDLString defCharset;
  nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID));
  if (prefs)
    prefs->GetLocalizedUnicharPref("intl.charset.default", getter_Copies(defCharset));

  if (!defCharset.IsEmpty())
    stringEncoding = defCharset;
  else
    
    stringEncoding.AssignLiteral("ISO-8859-1");

  return(NS_OK);
}



const char * const
InternetSearchDataSource::MapScriptCodeToCharsetName(PRUint32 aScriptCode)
{
  
  
  
  
  
  
        static const char* const scriptList[] =
  {
    "x-mac-roman",           
    "Shift_JIS",             
    "Big5",                  
    "EUC-KR",                
    "X-MAC-ARABIC",          
    "X-MAC-HEBREW",          
    "X-MAC-GREEK",           
    "X-MAC-CYRILLIC",        
    "X-MAC-DEVANAGARI" ,     
    "X-MAC-GURMUKHI",        
    "X-MAC-GUJARATI",        
    "X-MAC-ORIYA",           
    "X-MAC-BENGALI",         
    "X-MAC-TAMIL",           
    "X-MAC-TELUGU",          
    "X-MAC-KANNADA",         
    "X-MAC-MALAYALAM",       
    "X-MAC-SINHALESE",       
    "X-MAC-BURMESE",         
    "X-MAC-KHMER",           
    "X-MAC-THAI",            
    "X-MAC-LAOTIAN",         
    "X-MAC-GEORGIAN",        
    "X-MAC-ARMENIAN",        
    "GB2312",                
    "X-MAC-TIBETAN",         
    "X-MAC-MONGOLIAN",       
    "X-MAC-ETHIOPIC",        
    "X-MAC-CENTRALEURROMAN", 
    "X-MAC-VIETNAMESE",      
    "X-MAC-EXTARABIC",       
  };

        if (aScriptCode >= NS_ARRAY_LENGTH(scriptList))
          aScriptCode = 0;

  return scriptList[aScriptCode];
}


nsresult
InternetSearchDataSource::validateEngine(nsIRDFResource *engine)
{
  nsresult  rv;

  
  nsCOMPtr<nsIPrefBranch>
    prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool userAllowed = PR_TRUE;
  rv = prefBranch->GetBoolPref("browser.search.update", &userAllowed);
  
  if (NS_SUCCEEDED(rv) && !userAllowed)
    return NS_OK;

#ifdef  DEBUG_SEARCH_UPDATES
  const char  *engineURI = nsnull;
  engine->GetValueConst(&engineURI);
#endif

  
  nsCOMPtr<nsIRDFNode>  updateCheckDaysNode;
  rv = mInner->GetTarget(engine, mNC_UpdateCheckDays, PR_TRUE, getter_AddRefs(updateCheckDaysNode));
  if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE)) return(rv);
  nsCOMPtr<nsIRDFInt> updateCheckDaysLiteral (do_QueryInterface(updateCheckDaysNode));
  PRInt32   updateCheckDays;
  updateCheckDaysLiteral->GetValue(&updateCheckDays);
  

#ifndef DEBUG_SEARCH_UPDATES
  PRInt32   updateCheckSecs = updateCheckDays * (60 * 60 * 24);
#else
  
  PRInt32   updateCheckSecs = updateCheckDays * 60;
#endif

  nsCOMPtr<nsIRDFNode>  aNode;
  rv = mLocalstore->GetTarget(engine, mWEB_LastPingDate, PR_TRUE, getter_AddRefs(aNode));
  if (NS_FAILED(rv))  return(rv);

  
  if (rv != NS_RDF_NO_VALUE) {
    
    nsCOMPtr<nsIRDFLiteral> lastCheckLiteral(do_QueryInterface(aNode));
    if (!lastCheckLiteral)
      return NS_ERROR_UNEXPECTED;

    const PRUnichar *lastCheckUni = nsnull;
    lastCheckLiteral->GetValueConst(&lastCheckUni);
    if (!lastCheckUni)
      return NS_ERROR_UNEXPECTED;

    PRInt32 lastCheckInt = 0, err = 0;
    lastCheckInt = nsDependentString(lastCheckUni).ToInteger(&err);
    
    rv = (nsresult) err;
    NS_ENSURE_SUCCESS(rv, rv);

    
    PRTime now64 = PR_Now(), temp64, million;
    LL_I2L(million, PR_USEC_PER_SEC);
    LL_DIV(temp64, now64, million);
    PRInt32 now32;
    LL_L2I(now32, temp64);

    
    
    PRInt32 durationSecs = now32 - lastCheckInt;

    if (durationSecs < updateCheckSecs) {
#ifdef  DEBUG_SEARCH_UPDATES
      printf("    Search engine '%s' is valid for %d more seconds.\n",
             engineURI, (updateCheckSecs-durationSecs));
#endif
      return NS_OK;
    }
  }

  
  PRInt32   elementIndex = mUpdateArray->IndexOf(engine);
  if (elementIndex < 0)
  {
    mUpdateArray->AppendElement(engine);

#ifdef  DEBUG_SEARCH_UPDATES
    printf("    Search engine '%s' is now queued to be validated"
           " via HTTP HEAD method.\n",
           engineURI);
#endif
  }
  else
  {
#ifdef  DEBUG_SEARCH_UPDATES
    printf("    Search engine '%s' is already in queue to be validated.\n",
      engineURI);
#endif
  }
  return(NS_OK);
}



nsresult
InternetSearchDataSource::DoSearch(nsIRDFResource *source, nsIRDFResource *engine,
        const nsString &fullURL, const nsString &text)
{
  nsresult  rv;
  nsAutoString  textTemp(text);

  if (!mInner)  return(NS_RDF_NO_VALUE);


  if (!engine)  return(NS_ERROR_NULL_POINTER);

  validateEngine(engine);

  nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder;
  nsAutoString      action, methodStr, input, userVar;

  nsCOMPtr<nsIRDFLiteral>   dataLit;
  if (NS_FAILED(rv = FindData(engine, getter_AddRefs(dataLit))) ||
    (rv == NS_RDF_NO_VALUE))  return(rv);

  const PRUnichar     *dataUni = nsnull;
  dataLit->GetValueConst(&dataUni);
  if (!dataUni)     return(NS_RDF_NO_VALUE);

  if (!fullURL.IsEmpty())
  {
    action.Assign(fullURL);
    methodStr.AssignLiteral("get");
  }
  else
  {
    if (NS_FAILED(rv = GetData(dataUni, "search", 0, "action", action)))  return(rv);
    if (NS_FAILED(rv = GetData(dataUni, "search", 0, "method", methodStr))) return(rv);
  }

  nsAutoString  encodingStr, resultEncodingStr;

  
  
  GetData(dataUni, "interpret", 0, "charset", resultEncodingStr);
  if (resultEncodingStr.IsEmpty())
  {
    GetData(dataUni, "interpret", 0, "resultEncoding", encodingStr);  
    MapEncoding(encodingStr, resultEncodingStr);
  }
  
  
  if (!resultEncodingStr.IsEmpty())
  {
    nsCOMPtr <nsICharsetConverterManager> charsetConv = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
    {
                  NS_LossyConvertUTF16toASCII charset(resultEncodingStr);
                  rv = charsetConv->GetUnicodeDecoder(charset.get(),
                                                      getter_AddRefs(unicodeDecoder));
    }
  }

  
  
  nsAutoString    queryEncodingStr;
  GetData(dataUni, "search", 0, "queryCharset", queryEncodingStr);
  if (queryEncodingStr.IsEmpty())
  {
    GetData(dataUni, "search", 0, "queryEncoding", encodingStr);    
    MapEncoding(encodingStr, queryEncodingStr);
  }
  if (!queryEncodingStr.IsEmpty())
  {
    
    

    char  *utf8data = ToNewUTF8String(textTemp);
    if (utf8data)
    {
      nsCOMPtr<nsITextToSubURI> textToSubURI = 
               do_GetService(kTextToSubURICID, &rv);
      if (NS_SUCCEEDED(rv) && (textToSubURI))
      {
        PRUnichar *uni = nsnull;
        if (NS_SUCCEEDED(rv = textToSubURI->UnEscapeAndConvert("UTF-8", utf8data, &uni)) && (uni))
        {
          char    *charsetData = nsnull;
          rv = textToSubURI->ConvertAndEscape(NS_LossyConvertUTF16toASCII(queryEncodingStr).get(),
                                              uni, &charsetData);
          if (NS_SUCCEEDED(rv) && charsetData)
          {
            CopyASCIItoUTF16(nsDependentCString(charsetData), textTemp);
            NS_Free(charsetData);
          }
          NS_Free(uni);
        }
      }
      NS_Free(utf8data);
    }
  }

  if (fullURL.IsEmpty() && methodStr.LowerCaseEqualsLiteral("get"))
  {
    nsAutoString engineNameStr;
    GetData(dataUni, "search", 0, "name", engineNameStr);

    if (NS_FAILED(rv = GetInputs(dataUni, engineNameStr, userVar, textTemp, input, 0, 0, 0)))  return(rv);
    if (input.IsEmpty())        return(NS_ERROR_UNEXPECTED);

    
    action += input;
  }

  nsCOMPtr<nsIInternetSearchContext>  context;
  if (NS_FAILED(rv = NS_NewInternetSearchContext(nsIInternetSearchContext::WEB_SEARCH_CONTEXT,
    source, engine, unicodeDecoder, nsnull, getter_AddRefs(context))))
    return(rv);
  if (!context) return(NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIURI>  url;
  if (NS_SUCCEEDED(rv = NS_NewURI(getter_AddRefs(url), action)))
  {
    nsCOMPtr<nsIChannel>  channel;
    if (NS_SUCCEEDED(rv = NS_NewChannel(getter_AddRefs(channel), url, nsnull, mLoadGroup)))
    {

      
      nsCOMPtr<nsIHttpChannel> httpMultiChannel (do_QueryInterface(channel));
      if (httpMultiChannel)
      {
                httpMultiChannel->SetRequestHeader(NS_LITERAL_CSTRING("MultiSearch"),
                                                   NS_LITERAL_CSTRING("true"),
                                                   PR_FALSE);
      }

      
      channel->SetLoadFlags(nsIRequest::LOAD_FROM_CACHE);

      if (methodStr.LowerCaseEqualsLiteral("post"))
      {
        nsCOMPtr<nsIHttpChannel> httpChannel (do_QueryInterface(channel));
        if (httpChannel)
        {
            httpChannel->SetRequestMethod(NS_LITERAL_CSTRING("POST"));
            
            
            nsCAutoString  postStr;
            postStr.AssignLiteral(POSTHEADER_PREFIX);
            postStr.AppendInt(input.Length(), 10);
            postStr.AppendLiteral(POSTHEADER_SUFFIX);
            postStr.Append(NS_LossyConvertUTF16toASCII(input));
            
            nsCOMPtr<nsIInputStream>  postDataStream;
            if (NS_SUCCEEDED(rv = NS_NewPostDataStream(getter_AddRefs(postDataStream),
                         PR_FALSE, postStr, 0)))
            {
          nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
          NS_ASSERTION(uploadChannel, "http must support nsIUploadChannel");
          uploadChannel->SetUploadStream(postDataStream, EmptyCString(), -1);
            }
        }
      }
      
      nsCOMPtr<nsIRequest> request;
      rv = channel->AsyncOpen(this, context);
    }
  }

  
  if (mInner)
  {
    nsCOMPtr<nsIRDFNode>  htmlNode;
    if (NS_SUCCEEDED(rv = mInner->GetTarget(engine, mNC_HTML, PR_TRUE, getter_AddRefs(htmlNode)))
      && (rv != NS_RDF_NO_VALUE))
    {
      rv = mInner->Unassert(engine, mNC_HTML, htmlNode);
    }
  }

  
  if (NS_SUCCEEDED(rv) && (mInner))
  {
    
    nsCOMPtr<nsIRDFNode>    engineIconNode = nsnull;
    mInner->GetTarget(engine, mNC_StatusIcon, PR_TRUE, getter_AddRefs(engineIconNode));
    if (engineIconNode)
    {
      rv = mInner->Unassert(engine, mNC_StatusIcon, engineIconNode);
    }

    mInner->Assert(engine, mNC_loading, mTrueLiteral, PR_TRUE);
  }

  return(rv);
}

nsresult
InternetSearchDataSource::SaveEngineInfoIntoGraph(nsIFile *file, nsIFile *icon,
                                                  const PRUnichar *categoryHint,
                                                  const PRUnichar *dataUni,
                                                  PRBool isSystemSearchFile)
{
  nsresult      rv = NS_OK;

  if (!file && !icon) return(NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIRDFResource>  searchRes;
  nsCOMPtr<nsIRDFResource>  categoryRes;
  nsCOMPtr<nsIFile>   native;

  if (icon != nsnull)
  {
    native = icon;
  }

  if (file != nsnull)
  {
    native = file;
  }

  PRBool exists;
  rv = native->Exists(&exists);
  if (NS_FAILED(rv)) return(rv);
  if (!exists) return(NS_ERROR_UNEXPECTED);

  nsAutoString basename;
  rv = native->GetLeafName(basename);
  if (NS_FAILED(rv)) return rv;

  
  PRInt32   extensionOffset;
  if ((extensionOffset = basename.RFindChar(PRUnichar('.'))) > 0)
  {
    basename.SetLength(extensionOffset);
    basename.AppendLiteral(".src");
  }

  nsCAutoString filePath;
  rv = native->GetNativePath(filePath);
  if (NS_FAILED(rv)) return rv;
  
  nsAutoString  searchURL;
  searchURL.AssignASCII(kEngineProtocol);
  char    *uriCescaped = nsEscape(filePath.get(), url_Path);
  if (!uriCescaped) return(NS_ERROR_NULL_POINTER);
  searchURL.AppendASCII(uriCescaped);
  nsCRT::free(uriCescaped);

  if ((extensionOffset = searchURL.RFindChar(PRUnichar('.'))) > 0)
  {
    searchURL.SetLength(extensionOffset);
    searchURL.AppendLiteral(".src");
  }

  rv = mRDFService->GetUnicodeResource(searchURL,
                                       getter_AddRefs(searchRes));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!basename.IsEmpty())
  {
    basename.Insert(NS_ConvertASCIItoUTF16(kURINC_SearchCategoryEngineBasenamePrefix), 0);

    rv = mRDFService->GetUnicodeResource(basename,
                                         getter_AddRefs(categoryRes));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIRDFLiteral> searchLiteral;
    rv = mRDFService->GetLiteral(basename.get(),
                                 getter_AddRefs(searchLiteral));
    NS_ENSURE_SUCCESS(rv, rv);

    if (file)
      updateAtom(mInner, searchRes, mNC_URL, searchLiteral, nsnull);
  }

  if (!searchRes)   return(NS_ERROR_UNEXPECTED);
  if (!categoryRes) return(NS_ERROR_UNEXPECTED);

  nsAutoString  iconURL;
  if (icon)
  {
    nsCAutoString iconFileURL;
    if (NS_FAILED(rv = NS_GetURLSpecFromFile(icon, iconFileURL)))
      return(rv);
    AppendUTF8toUTF16(iconFileURL, iconURL);
  }

  
  if (iconURL.Length() > 0)
  {
    nsCOMPtr<nsIRDFLiteral> iconLiteral;
    if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(iconURL.get(),
        getter_AddRefs(iconLiteral))))
    {
      updateAtom(mInner, searchRes, mNC_Icon, iconLiteral, nsnull);
    }
  }

  if (!isSystemSearchFile)
  {
    
    
    updateAtom(mInner, searchRes, mNC_SearchType, mNC_Engine, nsnull);
  }

  if (dataUni != nsnull)
  {
    updateDataHintsInGraph(searchRes, dataUni);

    
    if (categoryHint && categoryDataSource)
    {
      nsCOMPtr<nsIRDFLiteral> catLiteral;
      rv = mRDFService->GetLiteral(categoryHint, getter_AddRefs(catLiteral));

      nsCOMPtr<nsIRDFResource>  catSrc;
      if (catLiteral)
      {
        rv = categoryDataSource->GetSource(mNC_Title, catLiteral,
          PR_TRUE, getter_AddRefs(catSrc));
      }

      const char    *catURI = nsnull;
      if (catSrc)         
      {
        rv = catSrc->GetValueConst(&catURI);
      }

      nsCOMPtr<nsIRDFResource>  catRes;
      if (catURI)
      {
        nsCAutoString  catList;
        catList.AssignLiteral(kURINC_SearchCategoryPrefix);
        catList.Append(catURI);
        mRDFService->GetResource(catList, getter_AddRefs(catRes));
      }

      nsCOMPtr<nsIRDFContainer> container;
      if (catRes)
        container = do_CreateInstance(NS_RDF_CONTRACTID "/container;1");

      if (container)
      {
        rv = container->Init(categoryDataSource, catRes);
        if (NS_SUCCEEDED(rv))
          rv = mRDFC->MakeSeq(categoryDataSource, catRes, nsnull);

        if (NS_SUCCEEDED(rv))
        {
          PRInt32   searchIndex = -1;
          if (NS_SUCCEEDED(rv = container->IndexOf(categoryRes, &searchIndex))
            && (searchIndex < 0))
          {
            rv = container->AppendElement(categoryRes);
          }
        }
        if (NS_SUCCEEDED(rv))
        {
          
          nsCOMPtr<nsIRDFRemoteDataSource>  remoteCategoryStore;
          remoteCategoryStore = do_QueryInterface(categoryDataSource);
          if (remoteCategoryStore)
          {
            remoteCategoryStore->Flush();
          }
        }
      }
    }
  }

  
  PRBool  hasChildFlag = PR_FALSE;
  rv = mInner->HasAssertion(mNC_SearchEngineRoot, mNC_Child, searchRes,
                            PR_TRUE, &hasChildFlag);
  if (NS_SUCCEEDED(rv) && !hasChildFlag)
  {
    mInner->Assert(mNC_SearchEngineRoot, mNC_Child, searchRes, PR_TRUE);
  }

  return(NS_OK);
}

nsresult
InternetSearchDataSource::GetSearchEngineList(nsIFile *searchDir,
              PRBool isSystemSearchFile)
{
        nsresult      rv = NS_OK;

    if (!mInner)
    {
      return(NS_RDF_NO_VALUE);
    }

    PRBool hasMore = PR_FALSE;
    nsCOMPtr<nsISimpleEnumerator> dirIterator;
    rv = searchDir->GetDirectoryEntries(getter_AddRefs(dirIterator));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIFile> dirEntry;
  while ((rv = dirIterator->HasMoreElements(&hasMore)) == NS_OK && hasMore)
  {
    rv = dirIterator->GetNext((nsISupports**)getter_AddRefs(dirEntry));
        if (NS_FAILED(rv))
          continue;

        
        PRBool isHidden;
        rv = dirEntry->IsHidden(&isHidden);
        if (NS_FAILED(rv) || isHidden)
          continue;

        PRBool isDirectory;
        rv = dirEntry->IsDirectory(&isDirectory);
        if (NS_FAILED(rv))
          continue;
        if (isDirectory)
        {
          GetSearchEngineList(dirEntry, isSystemSearchFile);
          continue;
        }

        
        
        
        PRInt64 fileSize;
        rv = dirEntry->GetFileSize(&fileSize);
        if (NS_FAILED(rv) || (fileSize == 0))
            continue;
    
        nsAutoString uri;
        rv = dirEntry->GetPath(uri);
        if (NS_FAILED(rv))
        continue;

    PRInt32   len = uri.Length();
    if (len < 5)
    {
      continue;
    }

    
    nsAutoString  extension;
    if ((uri.Right(extension, 4) != 4) || (!extension.LowerCaseEqualsLiteral(".src")))
    {
      continue;
    }

    
    PRBool    foundIconFlag = PR_FALSE;
    nsAutoString  temp;
    
    nsCOMPtr<nsILocalFile> iconFile, loopFile;

                static const char *extensions[] = {
                        ".gif",
                        ".jpg",
                        ".jpeg",
                        ".png",
                        nsnull,
                };

                for (int ext_count = 0; extensions[ext_count] != nsnull; ext_count++) {
                        temp = Substring(uri, 0, uri.Length()-4);
                        temp.Append(NS_ConvertASCIItoUTF16(extensions[ext_count]));
                        rv = NS_NewLocalFile(temp, PR_TRUE, getter_AddRefs(loopFile));
                        if (NS_FAILED(rv)) return rv;
                        rv = loopFile->Exists(&foundIconFlag);
                        if (NS_FAILED(rv)) return rv;
                        if (!foundIconFlag) continue;
                        rv = loopFile->IsFile(&foundIconFlag);
                        if (NS_FAILED(rv)) return rv;
                        if (foundIconFlag) 
                        {
                                iconFile = loopFile;
                                break;
                        } 
                }
    
    SaveEngineInfoIntoGraph(dirEntry, iconFile, nsnull, nsnull, isSystemSearchFile);
  }

  return(rv);
}

nsresult
InternetSearchDataSource::ReadFileContents(nsILocalFile *localFile, nsString& sourceContents)
{
  nsresult      rv = NS_ERROR_FAILURE;
  PRInt64       contentsLen, total = 0;
  char        *contents;

        NS_ENSURE_ARG_POINTER(localFile);

        sourceContents.Truncate();

        rv = localFile->GetFileSize(&contentsLen);
        if (NS_FAILED(rv)) return rv;
        if (contentsLen > 0)
        {
                contents = new char [contentsLen + 1];
                if (contents)
                {
                        nsCOMPtr<nsIInputStream> inputStream;
                        rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream), localFile);
                        if (NS_FAILED(rv)) {
                          delete [] contents;
                          return rv;
                        }
                        PRUint32 howMany;
                        while (total < contentsLen) {
                                rv = inputStream->Read(contents+total, 
                                                       PRUint32(contentsLen),
                                                       &howMany);
                                if (NS_FAILED(rv)) {
                                        delete [] contents;
                                        return rv;
                                }
                                total += howMany;
                        }
                        if (total == contentsLen)
            {
        contents[contentsLen] = '\0';
        CopyASCIItoUTF16(Substring(contents, contents + contentsLen), sourceContents);
        rv = NS_OK;
            }
            delete [] contents;
            contents = nsnull;
    }
  }
  return(rv);
}



nsresult
InternetSearchDataSource::GetNumInterpretSections(const PRUnichar *dataUni, PRUint32 &numInterpretSections)
{
  numInterpretSections = 0;

  nsString  buffer(dataUni);

  NS_NAMED_LITERAL_STRING(section, "<interpret");
  PRBool    inSection = PR_FALSE;

  while(!buffer.IsEmpty())
  {
    PRInt32 eol = buffer.FindCharInSet("\r\n", 0);
    if (eol < 0)  break;
    nsAutoString  line;
    if (eol > 0)
    {
      buffer.Left(line, eol);
    }
    buffer.Cut(0, eol+1);
    if (line.IsEmpty()) continue;   
    if (line[0] == PRUnichar('#'))  continue; 
    line.Trim(" \t");
    if (!inSection)
    {
      PRInt32 sectionOffset = nsString_Find(section, line, PR_TRUE);
      if (sectionOffset < 0)  continue;
      line.Cut(0, sectionOffset + section.Length() + 1);
      inSection = PR_TRUE;
      ++numInterpretSections;     
    }
    line.Trim(" \t");
    PRInt32 len = line.Length();
    if (len > 0)
    {
      if (line[len-1] == PRUnichar('>'))
      {
        inSection = PR_FALSE;
        line.SetLength(len-1);
      }
    }
  }
  return(NS_OK);
}


nsresult
InternetSearchDataSource::GetData(const PRUnichar *dataUni, const char *sectionToFind, PRUint32 sectionNum,
          const char *attribToFind, nsString &value)
{
  nsString  buffer(dataUni);

  nsresult  rv = NS_RDF_NO_VALUE;
  PRBool    inSection = PR_FALSE;

  nsAutoString  section;
  section.Assign(PRUnichar('<'));
  section.Append(NS_ConvertASCIItoUTF16(sectionToFind));

  while(!buffer.IsEmpty())
  {
    PRInt32 eol = buffer.FindCharInSet("\r\n", 0);
    if (eol < 0)  break;
    nsAutoString  line;
    if (eol > 0)
    {
      buffer.Left(line, eol);
    }
    buffer.Cut(0, eol+1);
    if (line.IsEmpty()) continue;   
    if (line[0] == PRUnichar('#'))  continue; 
    line.Trim(" \t");
    if (!inSection)
    {
      PRInt32 sectionOffset = nsString_Find(section, line, PR_TRUE);
      if (sectionOffset < 0)  continue;
      if (sectionNum > 0)
      {
        --sectionNum;
        continue;
      }
      line.Cut(0, sectionOffset + section.Length() + 1);
      inSection = PR_TRUE;
    }
    line.Trim(" \t");
    PRInt32 len = line.Length();
    if (len > 0)
    {
      if (line[len-1] == PRUnichar('>'))
      {
        inSection = PR_FALSE;
        line.SetLength(len-1);
      }
    }
    PRInt32 equal = line.FindChar(PRUnichar('='));
    if (equal < 0)  continue;     
    
    nsAutoString  attrib;
    if (equal > 0)
    {
      line.Left(attrib, equal );
    }
    attrib.Trim(" \t");
    if (attrib.EqualsIgnoreCase(attribToFind))
    {
      line.Cut(0, equal+1);
      line.Trim(" \t");
      value = line;

      
      if ((value[0] == PRUnichar('\"')) || (value[0] == PRUnichar('\'')))
      {
        PRUnichar quoteChar = value[0];
        value.Cut(0,1);
        if (!value.IsEmpty())
        {
          PRInt32 quoteEnd = value.FindChar(quoteChar);
          if (quoteEnd >= 0)
          {
            value.SetLength(quoteEnd);
          }
        }
      }
      else
      {
        PRInt32 commentOffset = value.FindCharInSet("# \t", 0);
        if (commentOffset >= 0)
        {
          value.SetLength(commentOffset);
        }
        value.Trim(" \t");
      }
      rv = NS_OK;
      break;
    }
  }
  return(rv);
}



nsresult
InternetSearchDataSource::GetInputs(const PRUnichar *dataUni, nsString &engineName, nsString &userVar,
  const nsString &text, nsString &input, PRInt16 direction, PRUint16 pageNumber,  PRUint16 *whichButtons)
{
  nsString  buffer(dataUni);

  nsresult  rv = NS_OK;
  PRBool    inSection = PR_FALSE;
  PRBool    inDirInput; 
  PRBool    foundInput = PR_FALSE;

  while(!buffer.IsEmpty())
  {
    PRInt32 eol = buffer.FindCharInSet("\r\n", 0);
    if (eol < 0)  break;
    nsAutoString  line;
    if (eol > 0)
    {
      buffer.Left(line, eol);
    }
    buffer.Cut(0, eol+1);
    if (line.IsEmpty()) continue;   
    if (line[0] == PRUnichar('#'))  continue; 
    line.Trim(" \t");
    if (!inSection)
    {
      if (line[0] != PRUnichar('<'))  continue;
      line.Cut(0, 1);
      inSection = PR_TRUE;
    }
    PRInt32 len = line.Length();
    if (len > 0)
    {
      if (line[len-1] == PRUnichar('>'))
      {
        inSection = PR_FALSE;
        line.SetLength(len-1);
      }
    }
    if (inSection)
      continue;

    
    if (line.Find("input", PR_TRUE) == 0)
    {
      line.Cut(0, 5);

      
      inDirInput = PR_FALSE;

      if (line.Find("next", PR_TRUE) == 0)
      {
        inDirInput = PR_TRUE;
        if (whichButtons)
          *whichButtons |= kHaveNext;
      }

      if (line.Find("prev", PR_TRUE) == 0)
      {
        inDirInput = PR_TRUE;
        if (whichButtons)
          *whichButtons |= kHavePrev;
      }

      if (inDirInput)
        line.Cut(0, 4);
      
      line.Trim(" \t");
      
      
      nsAutoString  nameAttrib;

      PRInt32 nameOffset = line.Find("name", PR_TRUE);
      if (nameOffset >= 0)
      {
        PRInt32 equal = line.FindChar(PRUnichar('='), nameOffset);
        if (equal >= 0)
        {
          PRInt32 startQuote = line.FindChar(PRUnichar('\"'), equal + 1);
          if (startQuote >= 0)
          {
            PRInt32 endQuote = line.FindChar(PRUnichar('\"'), startQuote + 1);
            if (endQuote > 0)
            {
              line.Mid(nameAttrib, startQuote+1, endQuote-startQuote-1);
              line.Cut(0, endQuote + 1);
            }
          }
          else
          {
            nameAttrib = line;
            nameAttrib.Cut(0, equal+1);
            nameAttrib.Trim(" \t");
            PRInt32 space = nameAttrib.FindCharInSet(" \t", 0);
            if (space > 0)
            {
              nameAttrib.SetLength(space);
              line.Cut(0, equal+1+space);
            }
            else
            {
              line.Truncate();
            }
          }
        }
      }
      if (foundInput && nameAttrib.IsEmpty()) 
        continue;

      
      nsAutoString  valueAttrib;

      PRInt32  valueOffset;
      if (!inDirInput)
        valueOffset = line.Find("value", PR_TRUE);
      else
        valueOffset = line.Find("factor", PR_TRUE);
      if (valueOffset >= 0)
      {
        PRInt32 equal = line.FindChar(PRUnichar('='), valueOffset);
        if (equal >= 0)
        {
          PRInt32 startQuote = line.FindChar(PRUnichar('\"'), equal + 1);
          if (startQuote >= 0)
          {
            PRInt32 endQuote = line.FindChar(PRUnichar('\"'), startQuote + 1);
            if (endQuote >= 0)
            {
              line.Mid(valueAttrib, startQuote+1, endQuote-startQuote-1);
            }
          }
          else
          {
            
            valueAttrib = line;
            valueAttrib.Cut(0, equal+1);
            valueAttrib.Trim(" \t");
            PRInt32 space = valueAttrib.FindCharInSet(" \t>", 0);
            if (space > 0)
            {
              valueAttrib.SetLength(space);
            }
          }
        }
      }
      else if (line.Find("user", PR_TRUE) >= 0)
      {
        userVar = nameAttrib;
        valueAttrib.Assign(text);
      }
      
      
      
      if (line.RFind("mode=browser", PR_TRUE) >= 0)
        continue;

      if (!valueAttrib.IsEmpty())
      {
        
        
        
        
        
        
        if (!input.IsEmpty())
          input.AppendLiteral("&");
        else if (!nameAttrib.IsEmpty()) 
          input.AppendLiteral("?");

        if (!nameAttrib.IsEmpty()) 
        {
          input += nameAttrib;
          input.AppendLiteral("=");
        }

        
        
        
        
        foundInput = PR_TRUE;

        if (!inDirInput)
          input += valueAttrib;
        else
          input.AppendInt( computeIndex(valueAttrib, pageNumber, direction) );
      }
    }
  }

  
  
  nsCOMPtr<nsIPrefService> pserv(do_GetService(NS_PREFSERVICE_CONTRACTID));
  if (pserv) 
  {
    
    
    
    
    
    PRBool engineIsNotDefault = PR_FALSE;
    nsCOMPtr<nsIPrefBranch> rootBranch(do_QueryInterface(pserv));
    nsCOMPtr<nsIPrefBranch> defaultBranch;
    pserv->GetDefaultBranch("", getter_AddRefs(defaultBranch));

    if (defaultBranch) 
    {
      nsXPIDLString defaultEngineNameStr;
      nsCOMPtr<nsIPrefLocalizedString> defaultEngineName;
      rv = defaultBranch->GetComplexValue("browser.search.defaultenginename", 
                                          NS_GET_IID(nsIPrefLocalizedString),
                                          getter_AddRefs(defaultEngineName));
      if (NS_SUCCEEDED(rv)) {
        defaultEngineName->GetData(getter_Copies(defaultEngineNameStr));

        nsXPIDLString selectedEngineNameStr;
        nsCOMPtr<nsIPrefLocalizedString> selectedEngineName;
        rv = rootBranch->GetComplexValue("browser.search.selectedEngine", 
                                         NS_GET_IID(nsIPrefLocalizedString),
                                         getter_AddRefs(selectedEngineName));
        if (NS_SUCCEEDED(rv) && selectedEngineName) {
          selectedEngineName->GetData(getter_Copies(selectedEngineNameStr));
          engineIsNotDefault = !defaultEngineNameStr.Equals(selectedEngineNameStr);
        }
        else {
          engineIsNotDefault = PR_FALSE; 
                                         
                                         
                                         
        }
      }
    }

    PRInt32 i = 0;
    char prefNameBuf[1096];
    do
    {
      ++i;
      sprintf(prefNameBuf, "browser.search.param.%s.%d.", 
              NS_ConvertUTF16toUTF8(engineName).get(), i);

      nsCOMPtr<nsIPrefBranch> pb;
      rv = pserv->GetBranch(prefNameBuf, getter_AddRefs(pb));
      if (NS_FAILED(rv)) 
        break;

      nsCOMPtr<nsIPrefLocalizedString> parameter;
      nsXPIDLString parameterStr;
      rv = pb->GetComplexValue(engineIsNotDefault ? "custom" : "default", 
                               NS_GET_IID(nsIPrefLocalizedString), 
                               getter_AddRefs(parameter));
      if (NS_FAILED(rv))
        break;

      parameter->GetData(getter_Copies(parameterStr));
      
      if (!parameterStr.IsEmpty()) 
      {
        if (!input.IsEmpty())
          input.Append(NS_LITERAL_STRING("&"));
        input += parameterStr;
      }
    }
    while (1);

    
    nsCOMPtr<nsIStringBundleService> stringService(do_GetService(NS_STRINGBUNDLE_CONTRACTID));
    nsCOMPtr<nsIStringBundle> bundle;
    rv = stringService->CreateBundle(SEARCHCONFIG_PROPERTIES, getter_AddRefs(bundle));
    nsCOMPtr<nsIStringBundle> intlBundle;
    rv = stringService->CreateBundle(INTL_PROPERTIES, getter_AddRefs(intlBundle));

    nsXPIDLString langName;
    intlBundle->GetStringFromName(NS_LITERAL_STRING("general.useragent.locale").get(), 
                                  getter_Copies(langName));

    nsAutoString keyTemplate(NS_LITERAL_STRING("browser.search.param."));
    keyTemplate += engineName;
    keyTemplate.Append(NS_LITERAL_STRING(".release"));

    nsXPIDLString releaseValue;
    NS_NAMED_LITERAL_STRING(distributionID, MOZ_DISTRIBUTION_ID);
    const PRUnichar* strings[] = { distributionID.get(), langName.get() };
    bundle->FormatStringFromName(keyTemplate.get(), strings, 2, getter_Copies(releaseValue));

    if (!releaseValue.IsEmpty()) 
    {
      if (!input.IsEmpty())
        input.Append(NS_LITERAL_STRING("&"));
      input += releaseValue;
    }

    
    nsCOMPtr<nsIPrefBranch> pb;
    rv = pserv->GetBranch("", getter_AddRefs(pb));
    if (NS_FAILED(rv)) return rv;

    i = 0;
    do {
      ++i;
      sprintf(prefNameBuf, "browser.search.order.%d", i);

      nsCOMPtr<nsIPrefLocalizedString> orderEngineName;
      rv = pb->GetComplexValue(prefNameBuf, 
                               NS_GET_IID(nsIPrefLocalizedString),
                               getter_AddRefs(orderEngineName));
      if (NS_FAILED(rv)) 
        break;

      nsXPIDLString orderEngineNameStr;
      orderEngineName->GetData(getter_Copies(orderEngineNameStr));
      if (orderEngineNameStr.Equals(engineName))
        break;
    }
    while (PR_TRUE);

    if (NS_SUCCEEDED(rv))
    {
      sprintf(prefNameBuf, "browser.search.order.%s.%d",
              NS_ConvertUTF16toUTF8(engineName).get(), i);
      
      nsCOMPtr<nsIPrefLocalizedString> orderParam;
      rv = rootBranch->GetComplexValue(prefNameBuf, 
                                       NS_GET_IID(nsIPrefLocalizedString),
                                       getter_AddRefs(orderParam));
      if (NS_FAILED(rv))
      {
        sprintf(prefNameBuf, "browser.search.order.%s",
                NS_ConvertUTF16toUTF8(engineName).get());
        rv = rootBranch->GetComplexValue(prefNameBuf, 
                                         NS_GET_IID(nsIPrefLocalizedString),
                                         getter_AddRefs(orderParam));
      }
    
      if (NS_SUCCEEDED(rv)) 
      {
        nsXPIDLString orderParamStr;
        orderParam->GetData(getter_Copies(orderParamStr));

        if (!orderParamStr.IsEmpty())
        {
          if (!input.IsEmpty())
            input.Append(NS_LITERAL_STRING("&"));
          input += orderParamStr;
        }
      }
    }

    rv = NS_OK;
  }

  return(rv);
}

PRInt32
InternetSearchDataSource::computeIndex(nsAutoString &factor, 
                                       PRUint16 page, PRInt16 direction)
{
  
  PRInt32 errorCode, index = 0;
  PRInt32 factorInt = factor.ToInteger(&errorCode);
  
  if (NS_SUCCEEDED(errorCode))
  {
    
    if (factorInt <= 0)
      factorInt = 10;

    if (direction < 0)
    {
      
      if (0 <= (page - 1))
        --page;
    }
    index = factorInt * page;
  }

  return index;
}



nsresult
InternetSearchDataSource::GetURL(nsIRDFResource *source, nsIRDFLiteral** aResult)
{
  const char *uri = nsnull;
  nsresult rv = source->GetValueConst( &uri );
  NS_ENSURE_SUCCESS(rv, rv);
  return mRDFService->GetLiteral(NS_ConvertUTF8toUTF16(uri).get(), aResult);
}







NS_IMETHODIMP
InternetSearchDataSource::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
#ifdef  DEBUG_SEARCH_OUTPUT
  printf("InternetSearchDataSourceCallback::OnStartRequest entered.\n");
#endif
  return(NS_OK);
}



NS_IMETHODIMP
InternetSearchDataSource::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
        nsIInputStream *aIStream, PRUint32 sourceOffset, PRUint32 aLength)
{
  if (!ctxt)  return(NS_ERROR_NO_INTERFACE);
  nsCOMPtr<nsIInternetSearchContext> context (do_QueryInterface(ctxt));
  if (!context) return(NS_ERROR_NO_INTERFACE);

  nsresult  rv = NS_OK;

  if (aLength < 1)  return(rv);

  PRUint32  count;
  char    *buffer = new char[ aLength ];
  if (!buffer)  return(NS_ERROR_OUT_OF_MEMORY);

  if (NS_FAILED(rv = aIStream->Read(buffer, aLength, &count)) || count == 0)
  {
#ifdef  DEBUG
    printf("Search datasource read failure.\n");
#endif
    delete []buffer;
    return(rv);
  }
  if (count != aLength)
  {
#ifdef  DEBUG
    printf("Search datasource read # of bytes failure.\n");
#endif
    delete []buffer;
    return(NS_ERROR_UNEXPECTED);
  }

  nsCOMPtr<nsIUnicodeDecoder> decoder;
  context->GetUnicodeDecoder(getter_AddRefs(decoder));
  if (decoder)
  {
    char      *aBuffer = buffer;
    PRInt32     unicharBufLen = 0;
    decoder->GetMaxLength(aBuffer, aLength, &unicharBufLen);
    PRUnichar   *unichars = new PRUnichar [ unicharBufLen+1 ];
    do
    {
      PRInt32   srcLength = aLength;
      PRInt32   unicharLength = unicharBufLen;
      rv = decoder->Convert(aBuffer, &srcLength, unichars, &unicharLength);
      unichars[unicharLength]=0;  

      

      
      for(PRInt32 i=0;i < unicharLength; i++)
      {
        if(0x0000 == unichars[i])
        {
          unichars[i] = PRUnichar(' ');
        }
      }
      

      context->AppendUnicodeBytes(unichars, unicharLength);
      
      
      if(NS_FAILED(rv))
      {
        decoder->Reset();
        unsigned char smallBuf[2];
        smallBuf[0] = 0xFF;
        smallBuf[1] = 0xFD;
        context->AppendBytes( (const char *)&smallBuf, 2L);
        if(((PRUint32) (srcLength + 1)) > aLength)
          srcLength = aLength;
        else 
          srcLength++;
        aBuffer += srcLength;
        aLength -= srcLength;
      }
    } while (NS_FAILED(rv) && (aLength > 0));
    delete [] unichars;
    unichars = nsnull;
  }
  else
  {
    context->AppendBytes(buffer, aLength);
  }

  delete [] buffer;
  buffer = nsnull;
  return(rv);
}



NS_IMETHODIMP
InternetSearchDataSource::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                                        nsresult status)
{
  if (!mInner)  return(NS_OK);

  nsCOMPtr<nsIChannel> channel (do_QueryInterface(request));
  nsCOMPtr<nsIInternetSearchContext> context (do_QueryInterface(ctxt));
  if (!ctxt)  return(NS_ERROR_NO_INTERFACE);

  nsresult  rv;
  PRUint32  contextType = 0;
  if (NS_FAILED(rv = context->GetContextType(&contextType)))    return(rv);

  if (contextType == nsIInternetSearchContext::WEB_SEARCH_CONTEXT)
  {
    
    rv = webSearchFinalize(channel, context);
  }
  else if (contextType == nsIInternetSearchContext::ENGINE_DOWNLOAD_NEW_CONTEXT ||
           contextType == nsIInternetSearchContext::ICON_DOWNLOAD_NEW_CONTEXT ||
           contextType == nsIInternetSearchContext::ENGINE_DOWNLOAD_UPDATE_CONTEXT ||
           contextType == nsIInternetSearchContext::ICON_DOWNLOAD_UPDATE_CONTEXT)
  {
    nsCOMPtr<nsIHttpChannel> httpChannel (do_QueryInterface(channel));
    if (!httpChannel) return(NS_ERROR_UNEXPECTED);

    
    PRUint32  httpStatus = 0;
    if (NS_SUCCEEDED(rv = httpChannel->GetResponseStatus(&httpStatus)) &&
      (httpStatus == 200))
    {
      rv = saveContents(channel, context, contextType);
    }
  }
  else if (contextType == nsIInternetSearchContext::ENGINE_UPDATE_HEAD_CONTEXT)
  {
    nsCOMPtr<nsIRDFResource>  theEngine;
    if (NS_FAILED(rv = context->GetEngine(getter_AddRefs(theEngine))))  return(rv);
    if (!theEngine) return(NS_ERROR_NO_INTERFACE);

#ifdef  DEBUG_SEARCH_UPDATES
    const char  *engineURI = nsnull;
    theEngine->GetValueConst(&engineURI);
#endif

    
    busySchedule = PR_FALSE;
    busyResource = nsnull;

    
    
    rv = validateEngineNow(theEngine);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIHttpChannel> httpChannel (do_QueryInterface(channel));
    if (!httpChannel) return(NS_ERROR_UNEXPECTED);

    
    PRUint32  httpStatus = 0;
    if (NS_FAILED(rv = httpChannel->GetResponseStatus(&httpStatus)))
      return(rv);
    if (httpStatus != 200)  return(NS_ERROR_UNEXPECTED);

    
    nsCAutoString     lastModValue, contentLengthValue;

        if (NS_FAILED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Last-Modified"), lastModValue)))
            lastModValue.Truncate();
        if (NS_FAILED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Length"), contentLengthValue)))
            contentLengthValue.Truncate();

    
    PRBool    updateSearchEngineFile = PR_FALSE;

    
    PRBool      tempDirty = PR_FALSE;
    nsCOMPtr<nsIRDFLiteral> newValue;
    if (!lastModValue.IsEmpty())
    {
      mRDFService->GetLiteral(NS_ConvertASCIItoUTF16(lastModValue).get(),
        getter_AddRefs(newValue));
      if (newValue)
      {
        updateAtom(mLocalstore, theEngine, mWEB_LastPingModDate, newValue,
          &tempDirty);
        if (tempDirty)
          updateSearchEngineFile = PR_TRUE;
      }
    }
    if (!contentLengthValue.IsEmpty())
    {
      mRDFService->GetLiteral(NS_ConvertASCIItoUTF16(contentLengthValue).get(),
        getter_AddRefs(newValue));
      if (newValue)
      {
        updateAtom(mLocalstore, theEngine, mWEB_LastPingContentLen, newValue,
          &tempDirty);
        if (tempDirty)
        {
          updateSearchEngineFile = PR_TRUE;
        }
        else
        {
          
          nsCOMPtr<nsIRDFLiteral> dataLit;
          if (NS_SUCCEEDED(rv = FindData(theEngine, getter_AddRefs(dataLit))) &&
            (rv != NS_RDF_NO_VALUE))
          {
            const PRUnichar *dataUni = nsnull;
            dataLit->GetValueConst(&dataUni);
            nsAutoString  dataStr(dataUni);
            PRInt32   dataLen=dataStr.Length();
#ifdef  DEBUG_SEARCH_UPDATES
            printf("    Search engine='%s' data length='%d'\n", engineURI, dataLen);
#endif
            PRInt32 contentLen=0, err=0;
            contentLen = contentLengthValue.ToInteger(&err);
            if ((!err) && (dataLen != contentLen))
            {
#ifdef  DEBUG_SEARCH_UPDATES
              printf("    Search engine '%s' data length != remote content length, so update\n",
                engineURI, dataLen);
#endif
              updateSearchEngineFile = PR_TRUE;
            }
          }
        }
      }
    }

    if (updateSearchEngineFile)
    {
#ifdef  DEBUG_SEARCH_UPDATES
      printf("    Search engine='%s' needs updating, so fetching it\n", engineURI);
#endif
      
      nsCString   updateURL;
      nsCOMPtr<nsIRDFNode>  aNode;
      if (NS_SUCCEEDED(rv = mInner->GetTarget(theEngine, mNC_Update, PR_TRUE, getter_AddRefs(aNode)))
        && (rv != NS_RDF_NO_VALUE))
      {
        nsCOMPtr<nsIRDFLiteral> aLiteral (do_QueryInterface(aNode));
        if (aLiteral)
        {
          const PRUnichar *updateUni = nsnull;
          aLiteral->GetValueConst(&updateUni);
          if (updateUni)
          {
            CopyUTF16toUTF8(nsDependentString(updateUni), updateURL);
          }
        }
      }

      
      nsCString   updateIconURL;
      if (NS_SUCCEEDED(rv = mInner->GetTarget(theEngine, mNC_UpdateIcon, PR_TRUE, getter_AddRefs(aNode)))
        && (rv != NS_RDF_NO_VALUE))
      {
        nsCOMPtr<nsIRDFLiteral> aIconLiteral (do_QueryInterface(aNode));
        if (aIconLiteral)
        {
          const PRUnichar *updateIconUni = nsnull;
          aIconLiteral->GetValueConst(&updateIconUni);
          if (updateIconUni)
          {
            CopyUTF16toUTF8(nsDependentString(updateIconUni), updateIconURL);
          }
        }
      }

      
      AddSearchEngineInternal(updateURL.get(), updateIconURL.get(),
                              nsnull, nsnull, theEngine);
    }
    else
    {
#ifdef  DEBUG_SEARCH_UPDATES
      printf("    Search engine='%s' is current and requires no updating\n", engineURI);
#endif
    }
  }
  else
  {
    rv = NS_ERROR_UNEXPECTED;
  }
  return(rv);
}



nsresult
InternetSearchDataSource::validateEngineNow(nsIRDFResource *engine)
{
  
  
  
  PRTime    now64 = PR_Now(), temp64, million;
  LL_I2L(million, PR_USEC_PER_SEC);
  LL_DIV(temp64, now64, million);
  PRInt32   now32;
  LL_L2I(now32, temp64);

  
  
  nsAutoString  nowStr;
  nowStr.AppendInt(now32);

  nsresult    rv;
  nsCOMPtr<nsIRDFLiteral> nowLiteral;
  if (NS_FAILED(rv = mRDFService->GetLiteral(nowStr.get(),
      getter_AddRefs(nowLiteral)))) return(rv);
  updateAtom(mLocalstore, engine, mWEB_LastPingDate, nowLiteral, nsnull);

  
  nsCOMPtr<nsIRDFRemoteDataSource> remoteLocalStore (do_QueryInterface(mLocalstore));
  if (remoteLocalStore)
  {
    remoteLocalStore->Flush();
  }
  return(NS_OK);
}



nsresult
InternetSearchDataSource::webSearchFinalize(nsIChannel* channel, nsIInternetSearchContext *context)
{
  nsresult      rv;
  nsCOMPtr<nsIRDFResource>  mParent;
  if (NS_FAILED(rv = context->GetParent(getter_AddRefs(mParent))))  return(rv);



  nsCOMPtr<nsIRDFResource>  mEngine;
  if (NS_FAILED(rv = context->GetEngine(getter_AddRefs(mEngine))))  return(rv);
  if (!mEngine) return(NS_ERROR_NO_INTERFACE);

  nsCOMPtr<nsIURI> aURL;
  rv = channel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIRDFNode>    engineIconStatusNode = nsnull;
  mInner->GetTarget(mEngine, mNC_Icon, PR_TRUE, getter_AddRefs(engineIconStatusNode));
  if (engineIconStatusNode)
  {
    rv = mInner->Assert(mEngine, mNC_StatusIcon, engineIconStatusNode, PR_TRUE);
  }

  const PRUnichar *uniBuf = nsnull;
  if (NS_SUCCEEDED(rv = context->GetBufferConst(&uniBuf)) && (uniBuf))
  {
    if (mParent && (mBrowserSearchMode>0))
    {
      
      nsCOMPtr<nsIRDFLiteral> htmlLiteral;
      if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(uniBuf, getter_AddRefs(htmlLiteral))))
      {
        rv = mInner->Assert(mEngine, mNC_HTML, htmlLiteral, PR_TRUE);
      }
    }

    
    PRInt32 uniBufLen = 0L;
    if (NS_SUCCEEDED(rv = context->GetBufferLength(&uniBufLen)))
    {
      rv = ParseHTML(aURL, mParent, mEngine, uniBuf, uniBufLen);
    }
  }
  else
  {
#ifdef  DEBUG_SEARCH_OUTPUT
    printf(" *** InternetSearchDataSourceCallback::OnStopRequest:  no data.\n\n");
#endif
  }

  
  context->Truncate();

  
  mInner->Unassert(mEngine, mNC_loading, mTrueLiteral);

  if (mLoadGroup)
  {
    PRUint32  count = 0;
    if (NS_SUCCEEDED(rv = mLoadGroup->GetActiveCount(&count)))
    {
      
      if (count <= 1)
      {
        Stop();
      }
    }
  }

  return(NS_OK);
}



nsresult
InternetSearchDataSource::ParseHTML(nsIURI *aURL, nsIRDFResource *mParent,
        nsIRDFResource *mEngine, const PRUnichar *htmlPage, PRInt32 htmlPageSize)
{
  
  nsresult  rv;
  nsCOMPtr<nsIRDFNode>  dataNode;
  if (NS_FAILED(rv = mInner->GetTarget(mEngine, mNC_Data, PR_TRUE, getter_AddRefs(dataNode))))
  {
    return(rv);
  }
  nsCOMPtr<nsIRDFLiteral> dataLiteral (do_QueryInterface(dataNode));
  if (!dataLiteral) return(NS_ERROR_NULL_POINTER);

  const PRUnichar *dataUni = nsnull;
  if (NS_FAILED(rv = dataLiteral->GetValueConst(&dataUni)))
    return(rv);
  if (!dataUni) return(NS_ERROR_NULL_POINTER);

  
  nsAutoString  engineStr;
  GetData(dataUni, "search", 0, "name", engineStr);

  
  nsCAutoString hostName;
  aURL->GetAsciiHost(hostName);

  
  nsAutoString  serverPathStr;
  nsCAutoString serverPath;
  aURL->GetPath(serverPath);
  if (!serverPath.IsEmpty())
  {
        AppendUTF8toUTF16(serverPath, serverPathStr);
        serverPath.Truncate();

    PRInt32 serverOptionsOffset = serverPathStr.FindChar(PRUnichar('?'));
    if (serverOptionsOffset >= 0) serverPathStr.SetLength(serverOptionsOffset);
  }

  PRBool    hasPriceFlag = PR_FALSE, hasAvailabilityFlag = PR_FALSE, hasRelevanceFlag = PR_FALSE;
  PRBool    hasDateFlag = PR_FALSE;
  PRBool    skipLocalFlag = PR_FALSE, useAllHREFsFlag = PR_FALSE;
  PRInt32   pageRank = 1;
  PRUint32  numInterpretSections, numResults = 0;

  GetNumInterpretSections(dataUni, numInterpretSections);
  if (numInterpretSections < 1)
  {
    
    numInterpretSections = 1;
    useAllHREFsFlag = PR_TRUE;
    skipLocalFlag = PR_TRUE;
  }

#ifdef  DEBUG
  PRTime    now;
  now = PR_Now();
  printf("\nStart processing search results:   %u bytes \n", htmlPageSize); 
#endif

  
  for (PRUint32 interpretSectionNum=0; interpretSectionNum < numInterpretSections; interpretSectionNum++)
  {
    nsAutoString  resultListStartStr, resultListEndStr;
    nsAutoString  resultItemStartStr, resultItemEndStr;
    nsAutoString  relevanceStartStr, relevanceEndStr;
    nsAutoString  bannerStartStr, bannerEndStr, skiplocalStr;
    nsAutoString  priceStartStr, priceEndStr, availStartStr, availEndStr;
    nsAutoString  dateStartStr, dateEndStr;
    nsAutoString  nameStartStr, nameEndStr;
    nsAutoString  emailStartStr, emailEndStr;
    nsAutoString  browserResultTypeStr;
    browserResultTypeStr.AssignLiteral("result");   

    
    nsDependentString  htmlResults(htmlPage, htmlPageSize);
    PRUint32           startIndex = 0L, stopIndex = htmlPageSize;

    if (!useAllHREFsFlag)
    {
      GetData(dataUni, "interpret", interpretSectionNum, "resultListStart", resultListStartStr);
      GetData(dataUni, "interpret", interpretSectionNum, "resultListEnd", resultListEndStr);
      GetData(dataUni, "interpret", interpretSectionNum, "resultItemStart", resultItemStartStr);
      GetData(dataUni, "interpret", interpretSectionNum, "resultItemEnd", resultItemEndStr);
      GetData(dataUni, "interpret", interpretSectionNum, "relevanceStart", relevanceStartStr);
      GetData(dataUni, "interpret", interpretSectionNum, "relevanceEnd", relevanceEndStr);
      GetData(dataUni, "interpret", interpretSectionNum, "bannerStart", bannerStartStr);
      GetData(dataUni, "interpret", interpretSectionNum, "bannerEnd", bannerEndStr);
      GetData(dataUni, "interpret", interpretSectionNum, "skiplocal", skiplocalStr);
      skipLocalFlag = (skiplocalStr.LowerCaseEqualsLiteral("true")) ? PR_TRUE : PR_FALSE;

      
      GetData(dataUni, "interpret", interpretSectionNum, "priceStart", priceStartStr);
      GetData(dataUni, "interpret", interpretSectionNum, "priceEnd", priceEndStr);
      GetData(dataUni, "interpret", interpretSectionNum, "availStart", availStartStr);
      GetData(dataUni, "interpret", interpretSectionNum, "availEnd", availEndStr);

      
      GetData(dataUni, "interpret", interpretSectionNum, "dateStart", dateStartStr);
      GetData(dataUni, "interpret", interpretSectionNum, "dateEnd", dateEndStr);

      
      GetData(dataUni, "interpret", interpretSectionNum, "nameStart", nameStartStr);
      GetData(dataUni, "interpret", interpretSectionNum, "nameEnd", nameEndStr);
      GetData(dataUni, "interpret", interpretSectionNum, "emailStart", emailStartStr);
      GetData(dataUni, "interpret", interpretSectionNum, "emailEnd", emailEndStr);

      
      GetData(dataUni, "interpret", interpretSectionNum, "browserResultType", browserResultTypeStr);
      if (browserResultTypeStr.IsEmpty())
      {
        browserResultTypeStr.AssignLiteral("result"); 
      }
    }

    
    nsCOMPtr<nsIRDFLiteral> bannerLiteral;
    if ((!bannerStartStr.IsEmpty()) && (!bannerEndStr.IsEmpty()))
    {
      PRInt32 bannerStart = nsString_Find(bannerStartStr, htmlResults, PR_TRUE);
      if (bannerStart >= 0)
      {
        startIndex = bannerStart;

        PRInt32 bannerEnd = nsString_Find(bannerEndStr, htmlResults, PR_TRUE, bannerStart + bannerStartStr.Length());
        if (bannerEnd > bannerStart)
        {
          stopIndex = bannerEnd - 1;

          nsAutoString  htmlBanner;
          htmlResults.Mid(htmlBanner, bannerStart, bannerEnd - bannerStart - 1);
          if (!htmlBanner.IsEmpty())
          {
            const PRUnichar *bannerUni = htmlBanner.get();
            if (bannerUni)
            {
              mRDFService->GetLiteral(bannerUni, getter_AddRefs(bannerLiteral));
            }
          }
        }
      }
    }

    if (!resultListStartStr.IsEmpty())
    {
      PRInt32 resultListStart = nsString_Find(resultListStartStr, htmlResults, PR_TRUE);
      if (resultListStart >= 0)
      {
        startIndex = resultListStart + resultListStartStr.Length();
      }
      else if (!useAllHREFsFlag)
      {
        
        
        continue;
      }
    }
    if (!resultListEndStr.IsEmpty())
    {
      
      

        nsAString::const_iterator originalStart, start, end;
        htmlResults.BeginReading(start);
        htmlResults.EndReading(end);
        originalStart = start;
        
        if (RFindInReadable(resultListEndStr, start, end))
      stopIndex = Distance(originalStart, start);
    }

    PRBool  trimItemStart = PR_TRUE;
    PRBool  trimItemEnd = PR_FALSE;   

    
    if (resultItemStartStr.IsEmpty())
    {
      resultItemStartStr.AssignLiteral("HREF=");
      trimItemStart = PR_FALSE;
    }

    
    if (resultItemEndStr.IsEmpty())
    {
      resultItemEndStr = resultItemStartStr;
      trimItemEnd = PR_TRUE;
    }

    while(startIndex < stopIndex)
    {
      PRInt32 resultItemStart;
      resultItemStart = nsString_Find(resultItemStartStr, htmlResults, PR_TRUE, startIndex);
      if (resultItemStart < 0)  break;

      PRInt32 resultItemEnd;
      if (trimItemStart)
      {
        resultItemStart += resultItemStartStr.Length();
        resultItemEnd = nsString_Find(resultItemEndStr, htmlResults, PR_TRUE, resultItemStart);
      }
      else
      {
        resultItemEnd = nsString_Find(resultItemEndStr, htmlResults, PR_TRUE, resultItemStart + resultItemStartStr.Length());
      }

      if (resultItemEnd < 0)
      {
        resultItemEnd = stopIndex;
      }
      else if (!trimItemEnd && resultItemEnd >= 0)
      {
        resultItemEnd += resultItemEndStr.Length();
      }

      
      
      if (resultItemEnd - resultItemStart - 1 <= 0)    break;
      nsAutoString resultItem(&htmlPage[resultItemStart],
        resultItemEnd - resultItemStart - 1);

      if (resultItem.IsEmpty()) break;

      
      startIndex = resultItemEnd;

#ifdef  DEBUG_SEARCH_OUTPUT
      char  *results = ToNewCString(resultItem);
      if (results)
      {
        printf("\n----- Search result: '%s'\n\n", results);
        nsCRT::free(results);
        results = nsnull;
      }
#endif

      
      PRInt32 hrefOffset = resultItem.Find("HREF=", PR_TRUE);
      if (hrefOffset < 0)
      {
#ifdef  DEBUG_SEARCH_OUTPUT
        printf("\n***** Unable to find HREF!\n\n");
#endif
        continue;
      }

      nsAutoString  hrefStr;
      PRInt32   quoteStartOffset = resultItem.FindCharInSet("\"\'>", hrefOffset);
      PRInt32   quoteEndOffset;
      if (quoteStartOffset < hrefOffset)
      {
        
        quoteStartOffset = hrefOffset + strlen("HREF=");
        quoteEndOffset = resultItem.FindChar('>', quoteStartOffset);
        if (quoteEndOffset < quoteStartOffset)  continue;
      }
      else
      {
        if (resultItem[quoteStartOffset] == PRUnichar('>'))
        {
          
          quoteEndOffset = quoteStartOffset;
          quoteStartOffset = hrefOffset + strlen("HREF=") -1;
        }
        else
        {
          quoteEndOffset = resultItem.FindCharInSet("\"\'", quoteStartOffset + 1);
          if (quoteEndOffset < hrefOffset)  continue;
        }
      }
      resultItem.Mid(hrefStr, quoteStartOffset + 1, quoteEndOffset - quoteStartOffset - 1);

      ConvertEntities(hrefStr, PR_FALSE, PR_TRUE, PR_FALSE);

      if (hrefStr.IsEmpty())  continue;

      nsAutoString absURIStr;

      if (NS_SUCCEEDED(rv = NS_MakeAbsoluteURI(absURIStr, hrefStr, aURL)))
      {
        hrefStr.Assign(absURIStr);

        nsCOMPtr<nsIURI>  absURI;
        rv = NS_NewURI(getter_AddRefs(absURI), absURIStr);

        if (absURI && skipLocalFlag)
        {
          nsCAutoString absPath;
          absURI->GetPath(absPath);
          if (!absPath.IsEmpty())
          {
                        NS_ConvertUTF8toUTF16 absPathStr(absPath);
            PRInt32 pathOptionsOffset = absPathStr.FindChar(PRUnichar('?'));
            if (pathOptionsOffset >= 0)
              absPathStr.SetLength(pathOptionsOffset);
            PRBool  pathsMatchFlag = serverPathStr.Equals(absPathStr, nsCaseInsensitiveStringComparator());
            if (pathsMatchFlag)
              continue;
          }

          if (!hostName.IsEmpty())
          {
            nsCAutoString absHost;
            absURI->GetAsciiHost(absHost);
            if (!absHost.IsEmpty())
            {
              PRBool  hostsMatchFlag = !nsCRT::strcasecmp(hostName.get(), absHost.get());
              if (hostsMatchFlag)
                continue;
            }
          }
        }
      }

      
      if (isSearchResultFiltered(hrefStr))
        continue;

      nsAutoString  site(hrefStr);

#ifdef  DEBUG_SEARCH_OUTPUT
      char *hrefCStr = ToNewCString(hrefStr);
      if (hrefCStr)
      {
        printf("HREF: '%s'\n", hrefCStr);
        nsCRT::free(hrefCStr);
        hrefCStr = nsnull;
      }
#endif

      nsCOMPtr<nsIRDFResource>  res;

      
      if (NS_SUCCEEDED(rv = mRDFService->GetAnonymousResource(getter_AddRefs(res))))
      {
        const PRUnichar *hrefUni = hrefStr.get();
        if (hrefUni)
        {
          nsCOMPtr<nsIRDFLiteral> hrefLiteral;
          if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(hrefUni, getter_AddRefs(hrefLiteral))))
          {
            mInner->Assert(res, mNC_URL, hrefLiteral, PR_TRUE);
          }
        }
      }

      if (NS_FAILED(rv))  continue;
      
      
      const PRUnichar *htmlResponseUni = resultItem.get();
      if (htmlResponseUni && (mBrowserSearchMode > 0))
      {
        nsCOMPtr<nsIRDFLiteral> htmlResponseLiteral;
        if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(htmlResponseUni, getter_AddRefs(htmlResponseLiteral))))
        {
          if (htmlResponseLiteral)
          {
            mInner->Assert(res, mNC_HTML, htmlResponseLiteral, PR_TRUE);
          }
        }
      }
      
      
      if (bannerLiteral)
      {
        mInner->Assert(res, mNC_Banner, bannerLiteral, PR_TRUE);
      }

      
      nsCOMPtr<nsIRDFNode>    oldSiteRes = nsnull;
      mInner->GetTarget(res, mNC_Site, PR_TRUE, getter_AddRefs(oldSiteRes));
      if (!oldSiteRes)
      {
        PRInt32 protocolOffset = site.FindChar(':', 0);
        if (protocolOffset >= 0)
        {
          site.Cut(0, protocolOffset+1);
          while (site[0] == PRUnichar('/'))
          {
            site.Cut(0, 1);
          }
          PRInt32 slashOffset = site.FindChar('/', 0);
          if (slashOffset >= 0)
          {
            site.SetLength(slashOffset);
          }
          if (!site.IsEmpty())
          {
            const PRUnichar *siteUni = site.get();
            if (siteUni)
            {
              nsCOMPtr<nsIRDFLiteral> siteLiteral;
              if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(siteUni, getter_AddRefs(siteLiteral))))
              {
                if (siteLiteral)
                {
                  mInner->Assert(res, mNC_Site, siteLiteral, PR_TRUE);
                }
              }
            }
          }
        }
      }

      
      nsAutoString  nameStr;

      if ((!nameStartStr.IsEmpty()) && (!nameEndStr.IsEmpty()))
      {
        PRInt32   nameStart;
        if ((nameStart = nsString_Find(nameStartStr, resultItem, PR_TRUE)) >= 0)
        {
          nameStart += nameStartStr.Length();
          PRInt32 nameEnd = nsString_Find(nameEndStr, resultItem, PR_TRUE, nameStart);
          if (nameEnd > nameStart)
          {
            resultItem.Mid(nameStr, nameStart, nameEnd - nameStart);
          }
        }
      }

      if (nameStr.IsEmpty())
      {
        PRInt32 anchorEnd = resultItem.FindChar('>', quoteEndOffset);
        if (anchorEnd < quoteEndOffset)
        {
#ifdef  DEBUG_SEARCH_OUTPUT
          printf("\n\nSearch: Unable to find ending > when computing name.\n\n");
#endif
          continue;
        }
        PRInt32 anchorStop = resultItem.Find("</A>", PR_TRUE, quoteEndOffset);
        if (anchorStop < anchorEnd)
        {
#ifdef  DEBUG_SEARCH_OUTPUT
          printf("\n\nSearch: Unable to find </A> tag to compute name.\n\n");
#endif
          continue;
        }
        resultItem.Mid(nameStr, anchorEnd + 1, anchorStop - anchorEnd - 1);
      }

      ConvertEntities(nameStr);

      
      nsCOMPtr<nsIRDFNode>    oldNameRes = nsnull;
      if (!oldNameRes)
      {
        if (!nameStr.IsEmpty())
        {
          const PRUnichar *nameUni = nameStr.get();
          if (nameUni)
          {
            nsCOMPtr<nsIRDFLiteral> nameLiteral;
            if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(nameUni, getter_AddRefs(nameLiteral))))
            {
              if (nameLiteral)
              {
                mInner->Assert(res, mNC_Name, nameLiteral, PR_TRUE);
              }
            }
          }
        }
      }

      
      if (!dateStartStr.IsEmpty())
      {
        nsAutoString  dateItem;
        PRInt32   dateStart;
        if ((dateStart = nsString_Find(dateStartStr, resultItem, PR_TRUE)) >= 0)
        {
          dateStart += dateStartStr.Length();
          PRInt32 dateEnd = nsString_Find(dateEndStr, resultItem, PR_TRUE, dateStart);
          if (dateEnd > dateStart)
          {
            resultItem.Mid(dateItem, dateStart, dateEnd - dateStart);
          }
        }
        
        
        PRInt32   ltOffset, gtOffset;
        while ((ltOffset = dateItem.FindChar(PRUnichar('<'))) >= 0)
        {
          if ((gtOffset = dateItem.FindChar(PRUnichar('>'), ltOffset)) <= ltOffset)
            break;
          dateItem.Cut(ltOffset, gtOffset - ltOffset + 1);
        }

        
        dateItem.Trim("\n\r\t ");

        if (!dateItem.IsEmpty())
        {
          const PRUnichar   *dateUni = dateItem.get();
          nsCOMPtr<nsIRDFLiteral> dateLiteral;
          if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(dateUni, getter_AddRefs(dateLiteral))))
          {
            if (dateLiteral)
            {
              mInner->Assert(res, mNC_Date, dateLiteral, PR_TRUE);
              hasDateFlag = PR_TRUE;
            }
          }
        }
      }

      
      if (!priceStartStr.IsEmpty())
      {
        nsAutoString  priceItem;
        PRInt32   priceStart;
        if ((priceStart = nsString_Find(priceStartStr, resultItem, PR_TRUE)) >= 0)
        {
          priceStart += priceStartStr.Length();
          PRInt32 priceEnd = nsString_Find(priceEndStr, resultItem, PR_TRUE, priceStart);
          if (priceEnd > priceStart)
          {
            resultItem.Mid(priceItem, priceStart, priceEnd - priceStart);
            ConvertEntities(priceItem);
          }
        }
        if (!priceItem.IsEmpty())
        {
          const PRUnichar   *priceUni = priceItem.get();
          nsCOMPtr<nsIRDFLiteral> priceLiteral;
          if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(priceUni, getter_AddRefs(priceLiteral))))
          {
            if (priceLiteral)
            {
              mInner->Assert(res, mNC_Price, priceLiteral, PR_TRUE);
              hasPriceFlag = PR_TRUE;
            }
          }

          PRInt32 priceCharStartOffset = priceItem.FindCharInSet("1234567890");
          if (priceCharStartOffset >= 0)
          {
            priceItem.Cut(0, priceCharStartOffset);
            PRInt32 priceErr;
            float val = priceItem.ToFloat(&priceErr);
            if (priceItem.FindChar(PRUnichar('.')) >= 0)  val *= 100;

            nsCOMPtr<nsIRDFInt> priceSortLiteral;
            if (NS_SUCCEEDED(rv = mRDFService->GetIntLiteral((PRInt32)val, getter_AddRefs(priceSortLiteral))))
            {
              if (priceSortLiteral)
              {
                mInner->Assert(res, mNC_PriceSort, priceSortLiteral, PR_TRUE);
              }
            }
          }
        }
        
        if (!availStartStr.IsEmpty())
        {
          nsAutoString  availItem;
          PRInt32   availStart;
          if ((availStart = nsString_Find(availStartStr, resultItem, PR_TRUE)) >= 0)
          {
            availStart += availStartStr.Length();
            PRInt32 availEnd = nsString_Find(availEndStr, resultItem, PR_TRUE, availStart);
            if (availEnd > availStart)
            {
              resultItem.Mid(availItem, availStart, availEnd - availStart);
              ConvertEntities(availItem);
            }
          }
          if (!availItem.IsEmpty())
          {
            const PRUnichar   *availUni = availItem.get();
            nsCOMPtr<nsIRDFLiteral> availLiteral;
            if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(availUni, getter_AddRefs(availLiteral))))
            {
              if (availLiteral)
              {
                mInner->Assert(res, mNC_Availability, availLiteral, PR_TRUE);
                hasAvailabilityFlag = PR_TRUE;
              }
            }
          }
        }
      }

      
      nsAutoString  relItem;
      PRInt32   relStart;
      if ((relStart = nsString_Find(relevanceStartStr, resultItem, PR_TRUE)) >= 0)
      {
        relStart += relevanceStartStr.Length();
        PRInt32 relEnd = nsString_Find(relevanceEndStr, resultItem, PR_TRUE);
        if (relEnd > relStart)
        {
          resultItem.Mid(relItem, relStart, relEnd - relStart);
        }
      }

      
      nsCOMPtr<nsIRDFNode>    oldRelRes = nsnull;
      if (!oldRelRes)
      {
        if (!relItem.IsEmpty())
        {
          
          const PRUnichar *relUni = relItem.get();
          if (relUni)
          {
            nsAutoString  relStr(relUni);
            
            PRInt32 len = relStr.Length();
            for (PRInt32 x=len-1; x>=0; x--)
            {
              PRUnichar ch;
              ch = relStr.CharAt(x);
              if ((ch != PRUnichar('%')) &&
                ((ch < PRUnichar('0')) || (ch > PRUnichar('9'))))
              {
                relStr.Cut(x, 1);
              }
            }
            
            len = relStr.Length();
            if (len > 0)
            {
              PRUnichar ch;
              ch = relStr.CharAt(len - 1);
              if (ch != PRUnichar('%'))
              {
                relStr += PRUnichar('%');
              }
              relItem = relStr;
              hasRelevanceFlag = PR_TRUE;
            }
            else
            {
              relItem.Truncate();
            }
          }
        }
        if (relItem.IsEmpty())
        {
          relItem.AssignLiteral("-");
        }

        const PRUnichar *relItemUni = relItem.get();
        nsCOMPtr<nsIRDFLiteral> relLiteral;
        if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(relItemUni, getter_AddRefs(relLiteral))))
        {
          if (relLiteral)
          {
            mInner->Assert(res, mNC_Relevance, relLiteral, PR_TRUE);
          }
        }

        if ((!relItem.IsEmpty()) && (!relItem.EqualsLiteral("-")))
        {
          
          if (relItem[relItem.Length()-1] == PRUnichar('%'))
          {
            relItem.Cut(relItem.Length()-1, 1);
          }

          
          nsAutoString  zero;
          zero.AssignLiteral("000");
          if (relItem.Length() < 3)
          {
            relItem.Insert(zero.get(), 0, zero.Length() - relItem.Length()); 
          }
        }
        else
        {
          relItem.AssignLiteral("000");
        }

        const PRUnichar *relSortUni = relItem.get();
        if (relSortUni)
        {
          nsCOMPtr<nsIRDFLiteral> relSortLiteral;
          if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(relSortUni, getter_AddRefs(relSortLiteral))))
          {
            if (relSortLiteral)
            {
              mInner->Assert(res, mNC_RelevanceSort, relSortLiteral, PR_TRUE);
            }
          }
        }
      }

      
      nsCOMPtr<nsIRDFNode>    oldEngineRes = nsnull;
      if (!oldEngineRes)
      {
        if (!engineStr.IsEmpty())
        {
          const PRUnichar   *engineUni = engineStr.get();
          nsCOMPtr<nsIRDFLiteral> engineLiteral;
          if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(engineUni, getter_AddRefs(engineLiteral))))
          {
            if (engineLiteral)
            {
              mInner->Assert(res, mNC_Engine, engineLiteral, PR_TRUE);
            }
          }
        }
      }

      
      nsCOMPtr<nsIRDFNode>    engineIconNode = nsnull;
      mInner->GetTarget(mEngine, mNC_Icon, PR_TRUE, getter_AddRefs(engineIconNode));

      
      nsAutoString  iconChromeDefault;

      if (browserResultTypeStr.LowerCaseEqualsLiteral("category"))
        iconChromeDefault.AssignLiteral("chrome://communicator/skin/search/category.gif");
      else if ((browserResultTypeStr.LowerCaseEqualsLiteral("result")) && (!engineIconNode))
        iconChromeDefault.AssignLiteral("chrome://communicator/skin/search/result.gif");

      if (!iconChromeDefault.IsEmpty())
      {
        const PRUnichar   *iconUni = iconChromeDefault.get();
        nsCOMPtr<nsIRDFLiteral> iconLiteral;
        if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(iconUni, getter_AddRefs(iconLiteral))))
        {
          if (iconLiteral)
          {
            mInner->Assert(res, mNC_Icon, iconLiteral, PR_TRUE);
          }
        }
      }
      else if (engineIconNode)
      {
        rv = mInner->Assert(res, mNC_Icon, engineIconNode, PR_TRUE);
      }

      
      nsCOMPtr<nsIRDFInt> pageRankLiteral;
      if (NS_SUCCEEDED(rv = mRDFService->GetIntLiteral(pageRank++, getter_AddRefs(pageRankLiteral))))
      {
        rv = mInner->Assert(res, mNC_PageRank, pageRankLiteral, PR_TRUE);
      }

      
      if (!browserResultTypeStr.IsEmpty())
      {
        const PRUnichar   *resultTypeUni = browserResultTypeStr.get();
        nsCOMPtr<nsIRDFLiteral> resultTypeLiteral;
        if (NS_SUCCEEDED(rv = mRDFService->GetLiteral(resultTypeUni, getter_AddRefs(resultTypeLiteral))))
        {
          if (resultTypeLiteral)
          {
            mInner->Assert(res, mNC_SearchType, resultTypeLiteral, PR_TRUE);
          }
        }
      }

      
      rv = mInner->Assert(res, mRDF_type, mNC_SearchResult, PR_TRUE);

      
#ifdef  OLDWAY
      PRBool    parentHasChildFlag = PR_FALSE;
      if (mParent)
      {
        mInner->HasAssertion(mParent, mNC_Child, res, PR_TRUE, &parentHasChildFlag);
      }
      if (!parentHasChildFlag)
#endif
      {
        if (mParent)
        {
          rv = mInner->Assert(mParent, mNC_Child, res, PR_TRUE);
        }
      }

      
      if (mInner)
      {
        rv = mInner->Assert(mNC_LastSearchRoot, mNC_Child, res, PR_TRUE);
      }

      ++numResults;

      
      if (numResults >= MAX_SEARCH_RESULTS_ALLOWED)
        break;

    }
  }

  
  if (mParent)
  {
    if (hasPriceFlag)
      SetHint(mParent, mNC_Price);
    if (hasAvailabilityFlag)
      SetHint(mParent, mNC_Availability);
    if (hasRelevanceFlag)
      SetHint(mParent, mNC_Relevance);
    if (hasDateFlag)
      SetHint(mParent, mNC_Date);
  }

#ifdef  DEBUG
  PRTime    now2;
  now2 = PR_Now();
  PRUint64  loadTime64;
  LL_SUB(loadTime64, now2, now);
  PRUint32  loadTime32;
  LL_L2UI(loadTime32, loadTime64);
  printf("Finished processing search results  (%u microseconds)\n", loadTime32);
#endif

  return(NS_OK);
}



nsresult
InternetSearchDataSource::SetHint(nsIRDFResource *mParent, nsIRDFResource *hintRes)
{
  if (!mInner)  return(NS_OK);

  nsresult  rv;
  PRBool    hasAssertionFlag = PR_FALSE;
  rv = mInner->HasAssertion(mParent, hintRes, mTrueLiteral, PR_TRUE,
                            &hasAssertionFlag);
  if (NS_SUCCEEDED(rv) && !hasAssertionFlag)
  {
    rv = mInner->Assert(mParent, hintRes, mTrueLiteral, PR_TRUE);
  }
  return(rv);
}



nsresult
InternetSearchDataSource::ConvertEntities(nsString &nameStr, PRBool removeHTMLFlag,
          PRBool removeCRLFsFlag, PRBool trimWhiteSpaceFlag)
{
  PRInt32 startOffset = 0, ampOffset, semiOffset, offset;

  
  if (removeHTMLFlag)
  {
    
    while ((offset = nameStr.FindChar(PRUnichar('<'), 0)) >= 0)
    {
      PRInt32 offsetEnd = nameStr.FindChar(PRUnichar('>'), offset+1);
      if (offsetEnd <= offset)  break;
      nameStr.Cut(offset, offsetEnd - offset + 1);
    }
  }
  while ((ampOffset = nameStr.FindChar(PRUnichar('&'), startOffset)) >= 0)
  {
    if ((semiOffset = nameStr.FindChar(PRUnichar(';'), ampOffset+1)) <= ampOffset)
      break;

    nsAutoString  entityStr;
    nameStr.Mid(entityStr, ampOffset, semiOffset-ampOffset+1);
    nameStr.Cut(ampOffset, semiOffset-ampOffset+1);

    PRUnichar entityChar = 0;
    if (entityStr.LowerCaseEqualsLiteral("&quot;")) entityChar = PRUnichar('\"');
    else if (entityStr.LowerCaseEqualsLiteral("&amp;")) entityChar = PRUnichar('&');
    else if (entityStr.LowerCaseEqualsLiteral("&nbsp;"))  entityChar = PRUnichar(' ');
    else if (entityStr.LowerCaseEqualsLiteral("&lt;"))    entityChar = PRUnichar('<');
    else if (entityStr.LowerCaseEqualsLiteral("&gt;"))    entityChar = PRUnichar('>');
    else if (entityStr.LowerCaseEqualsLiteral("&iexcl;")) entityChar = PRUnichar(161);
    else if (entityStr.LowerCaseEqualsLiteral("&cent;"))  entityChar = PRUnichar(162);
    else if (entityStr.LowerCaseEqualsLiteral("&pound;")) entityChar = PRUnichar(163);
    else if (entityStr.LowerCaseEqualsLiteral("&curren;"))  entityChar = PRUnichar(164);
    else if (entityStr.LowerCaseEqualsLiteral("&yen;")) entityChar = PRUnichar(165);
    else if (entityStr.LowerCaseEqualsLiteral("&brvbar;"))  entityChar = PRUnichar(166);
    else if (entityStr.LowerCaseEqualsLiteral("&sect;"))  entityChar = PRUnichar(167);
    else if (entityStr.LowerCaseEqualsLiteral("&uml;")) entityChar = PRUnichar(168);
    else if (entityStr.LowerCaseEqualsLiteral("&copy;"))  entityChar = PRUnichar(169);
    else if (entityStr.LowerCaseEqualsLiteral("&ordf;"))  entityChar = PRUnichar(170);
    else if (entityStr.LowerCaseEqualsLiteral("&laquo;")) entityChar = PRUnichar(171);
    else if (entityStr.LowerCaseEqualsLiteral("&not;")) entityChar = PRUnichar(172);
    else if (entityStr.LowerCaseEqualsLiteral("&shy;")) entityChar = PRUnichar(173);
    else if (entityStr.LowerCaseEqualsLiteral("&reg;")) entityChar = PRUnichar(174);
    else if (entityStr.LowerCaseEqualsLiteral("&macr;"))  entityChar = PRUnichar(175);
    else if (entityStr.LowerCaseEqualsLiteral("&deg;")) entityChar = PRUnichar(176);
    else if (entityStr.LowerCaseEqualsLiteral("&plusmn;"))  entityChar = PRUnichar(177);
    else if (entityStr.LowerCaseEqualsLiteral("&sup2;"))  entityChar = PRUnichar(178);
    else if (entityStr.LowerCaseEqualsLiteral("&sup3;"))  entityChar = PRUnichar(179);
    else if (entityStr.LowerCaseEqualsLiteral("&acute;")) entityChar = PRUnichar(180);
    else if (entityStr.LowerCaseEqualsLiteral("&micro;")) entityChar = PRUnichar(181);
    else if (entityStr.LowerCaseEqualsLiteral("&para;"))  entityChar = PRUnichar(182);
    else if (entityStr.LowerCaseEqualsLiteral("&middot;"))  entityChar = PRUnichar(183);
    else if (entityStr.LowerCaseEqualsLiteral("&cedil;")) entityChar = PRUnichar(184);
    else if (entityStr.LowerCaseEqualsLiteral("&sup1;"))  entityChar = PRUnichar(185);
    else if (entityStr.LowerCaseEqualsLiteral("&ordm;"))  entityChar = PRUnichar(186);
    else if (entityStr.LowerCaseEqualsLiteral("&raquo;")) entityChar = PRUnichar(187);
    else if (entityStr.LowerCaseEqualsLiteral("&frac14;"))  entityChar = PRUnichar(188);
    else if (entityStr.LowerCaseEqualsLiteral("&frac12;"))  entityChar = PRUnichar(189);
    else if (entityStr.LowerCaseEqualsLiteral("&frac34;"))  entityChar = PRUnichar(190);
    else if (entityStr.LowerCaseEqualsLiteral("&iquest;"))  entityChar = PRUnichar(191);
    else if (entityStr.LowerCaseEqualsLiteral("&agrave;"))  entityChar = PRUnichar(192);
    else if (entityStr.LowerCaseEqualsLiteral("&aacute;"))  entityChar = PRUnichar(193);
    else if (entityStr.LowerCaseEqualsLiteral("&acirc;")) entityChar = PRUnichar(194);
    else if (entityStr.LowerCaseEqualsLiteral("&atilde;"))  entityChar = PRUnichar(195);
    else if (entityStr.LowerCaseEqualsLiteral("&auml;"))  entityChar = PRUnichar(196);
    else if (entityStr.LowerCaseEqualsLiteral("&aring;")) entityChar = PRUnichar(197);
    else if (entityStr.LowerCaseEqualsLiteral("&aelig;")) entityChar = PRUnichar(198);
    else if (entityStr.LowerCaseEqualsLiteral("&ccedil;"))  entityChar = PRUnichar(199);
    else if (entityStr.LowerCaseEqualsLiteral("&egrave;"))  entityChar = PRUnichar(200);
    else if (entityStr.LowerCaseEqualsLiteral("&eacute;"))  entityChar = PRUnichar(201);
    else if (entityStr.LowerCaseEqualsLiteral("&ecirc;")) entityChar = PRUnichar(202);
    else if (entityStr.LowerCaseEqualsLiteral("&euml;"))  entityChar = PRUnichar(203);
    else if (entityStr.LowerCaseEqualsLiteral("&igrave;"))  entityChar = PRUnichar(204);
    else if (entityStr.LowerCaseEqualsLiteral("&iacute;"))  entityChar = PRUnichar(205);
    else if (entityStr.LowerCaseEqualsLiteral("&icirc;")) entityChar = PRUnichar(206);
    else if (entityStr.LowerCaseEqualsLiteral("&iuml;"))  entityChar = PRUnichar(207);
    else if (entityStr.LowerCaseEqualsLiteral("&eth;")) entityChar = PRUnichar(208);
    else if (entityStr.LowerCaseEqualsLiteral("&ntilde;"))  entityChar = PRUnichar(209);
    else if (entityStr.LowerCaseEqualsLiteral("&ograve;"))  entityChar = PRUnichar(210);
    else if (entityStr.LowerCaseEqualsLiteral("&oacute;"))  entityChar = PRUnichar(211);
    else if (entityStr.LowerCaseEqualsLiteral("&ocirc;")) entityChar = PRUnichar(212);
    else if (entityStr.LowerCaseEqualsLiteral("&otilde;"))  entityChar = PRUnichar(213);
    else if (entityStr.LowerCaseEqualsLiteral("&ouml;"))  entityChar = PRUnichar(214);
    else if (entityStr.LowerCaseEqualsLiteral("&times;")) entityChar = PRUnichar(215);
    else if (entityStr.LowerCaseEqualsLiteral("&oslash;"))  entityChar = PRUnichar(216);
    else if (entityStr.LowerCaseEqualsLiteral("&ugrave;"))  entityChar = PRUnichar(217);
    else if (entityStr.LowerCaseEqualsLiteral("&uacute;"))  entityChar = PRUnichar(218);
    else if (entityStr.LowerCaseEqualsLiteral("&ucirc;")) entityChar = PRUnichar(219);
    else if (entityStr.LowerCaseEqualsLiteral("&uuml;"))  entityChar = PRUnichar(220);
    else if (entityStr.LowerCaseEqualsLiteral("&yacute;"))  entityChar = PRUnichar(221);
    else if (entityStr.LowerCaseEqualsLiteral("&thorn;")) entityChar = PRUnichar(222);
    else if (entityStr.LowerCaseEqualsLiteral("&szlig;")) entityChar = PRUnichar(223);
    else if (entityStr.LowerCaseEqualsLiteral("&agrave;"))  entityChar = PRUnichar(224);
    else if (entityStr.LowerCaseEqualsLiteral("&aacute;"))  entityChar = PRUnichar(225);
    else if (entityStr.LowerCaseEqualsLiteral("&acirc;")) entityChar = PRUnichar(226);
    else if (entityStr.LowerCaseEqualsLiteral("&atilde;"))  entityChar = PRUnichar(227);
    else if (entityStr.LowerCaseEqualsLiteral("&auml;"))  entityChar = PRUnichar(228);
    else if (entityStr.LowerCaseEqualsLiteral("&aring;")) entityChar = PRUnichar(229);
    else if (entityStr.LowerCaseEqualsLiteral("&aelig;")) entityChar = PRUnichar(230);
    else if (entityStr.LowerCaseEqualsLiteral("&ccedil;"))  entityChar = PRUnichar(231);
    else if (entityStr.LowerCaseEqualsLiteral("&egrave;"))  entityChar = PRUnichar(232);
    else if (entityStr.LowerCaseEqualsLiteral("&eacute;"))  entityChar = PRUnichar(233);
    else if (entityStr.LowerCaseEqualsLiteral("&ecirc;")) entityChar = PRUnichar(234);
    else if (entityStr.LowerCaseEqualsLiteral("&euml;"))  entityChar = PRUnichar(235);
    else if (entityStr.LowerCaseEqualsLiteral("&igrave;"))  entityChar = PRUnichar(236);
    else if (entityStr.LowerCaseEqualsLiteral("&iacute;"))  entityChar = PRUnichar(237);
    else if (entityStr.LowerCaseEqualsLiteral("&icirc;")) entityChar = PRUnichar(238);
    else if (entityStr.LowerCaseEqualsLiteral("&iuml;"))  entityChar = PRUnichar(239);
    else if (entityStr.LowerCaseEqualsLiteral("&eth;")) entityChar = PRUnichar(240);
    else if (entityStr.LowerCaseEqualsLiteral("&ntilde;"))  entityChar = PRUnichar(241);
    else if (entityStr.LowerCaseEqualsLiteral("&ograve;"))  entityChar = PRUnichar(242);
    else if (entityStr.LowerCaseEqualsLiteral("&oacute;"))  entityChar = PRUnichar(243);
    else if (entityStr.LowerCaseEqualsLiteral("&ocirc;")) entityChar = PRUnichar(244);
    else if (entityStr.LowerCaseEqualsLiteral("&otilde;"))  entityChar = PRUnichar(245);
    else if (entityStr.LowerCaseEqualsLiteral("&ouml;"))  entityChar = PRUnichar(246);
    else if (entityStr.LowerCaseEqualsLiteral("&divide;"))  entityChar = PRUnichar(247);
    else if (entityStr.LowerCaseEqualsLiteral("&oslash;"))  entityChar = PRUnichar(248);
    else if (entityStr.LowerCaseEqualsLiteral("&ugrave;"))  entityChar = PRUnichar(249);
    else if (entityStr.LowerCaseEqualsLiteral("&uacute;"))  entityChar = PRUnichar(250);
    else if (entityStr.LowerCaseEqualsLiteral("&ucirc;")) entityChar = PRUnichar(251);
    else if (entityStr.LowerCaseEqualsLiteral("&uuml;"))  entityChar = PRUnichar(252);
    else if (entityStr.LowerCaseEqualsLiteral("&yacute;"))  entityChar = PRUnichar(253);
    else if (entityStr.LowerCaseEqualsLiteral("&thorn;")) entityChar = PRUnichar(254);
    else if (entityStr.LowerCaseEqualsLiteral("&yuml;"))  entityChar = PRUnichar(255);

    startOffset = ampOffset;
    if (entityChar != 0)
    {
      nameStr.Insert(entityChar, ampOffset);
      ++startOffset;
    }
  }

  if (removeCRLFsFlag)
  {
    
    while ((offset = nameStr.FindCharInSet("\n\r", 0)) >= 0)
    {
      nameStr.Cut(offset, 1);
    }
  }

  if (trimWhiteSpaceFlag)
  {
    
    nameStr.Trim(" \t");
  }

  return(NS_OK);
}

NS_IMETHODIMP
InternetSearchDataSource::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;

    if (!nsCRT::strcmp(aTopic, "profile-before-change"))
    {
        
        categoryDataSource = nsnull;

        if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("shutdown-cleanse").get()))
        {
            
            nsCOMPtr<nsIFile> searchFile;
            rv = NS_GetSpecialDirectory(NS_APP_SEARCH_50_FILE, getter_AddRefs(searchFile));
            if (NS_SUCCEEDED(rv))
                rv = searchFile->Remove(PR_FALSE);
        }
    }
    else if (!nsCRT::strcmp(aTopic, "profile-do-change"))
    {
        
        if (!categoryDataSource)
          GetCategoryList();
    }

    return rv;
}

