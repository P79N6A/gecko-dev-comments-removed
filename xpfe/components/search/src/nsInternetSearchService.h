



































 
#ifndef nsinternetsearchdatasource__h____
#define nsinternetsearchdatasource__h____

#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsISearchService.h"
#include "nsIRDFDataSource.h"
#include "nsIStreamListener.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsIRDFService.h"
#include "nsIRDFContainerUtils.h"
#include "nsITimer.h"
#include "nsIChannel.h"
#include "nsILoadGroup.h"
#include "nsIPref.h"
#include "nsCycleCollectionParticipant.h"

class InternetSearchDataSource : public nsIInternetSearchService,
                                 public nsIRDFDataSource,
                                 public nsIStreamListener,
                                 public nsIObserver,
                                 public nsSupportsWeakReference
{
private:
  PRInt32 mBrowserSearchMode;
  PRBool  mEngineListBuilt;

#ifdef MOZ_PHOENIX
  PRBool mReorderedEngineList;
#endif

  
  nsCOMPtr<nsIRDFResource> mNC_SearchResult;
  nsCOMPtr<nsIRDFResource> mNC_SearchEngineRoot;
  nsCOMPtr<nsIRDFResource> mNC_LastSearchRoot;
  nsCOMPtr<nsIRDFResource> mNC_LastSearchMode;
  nsCOMPtr<nsIRDFResource> mNC_SearchCategoryRoot;
  nsCOMPtr<nsIRDFResource> mNC_SearchResultsSitesRoot;
  nsCOMPtr<nsIRDFResource> mNC_FilterSearchURLsRoot;
  nsCOMPtr<nsIRDFResource> mNC_FilterSearchSitesRoot;
  nsCOMPtr<nsIRDFResource> mNC_SearchType;
  nsCOMPtr<nsIRDFResource> mNC_Ref;
  nsCOMPtr<nsIRDFResource> mNC_Child;
  nsCOMPtr<nsIRDFResource> mNC_Title;
  nsCOMPtr<nsIRDFResource> mNC_Data;
  nsCOMPtr<nsIRDFResource> mNC_Name;
  nsCOMPtr<nsIRDFResource> mNC_Description;
  nsCOMPtr<nsIRDFResource> mNC_Version;
  nsCOMPtr<nsIRDFResource> mNC_actionButton;
  nsCOMPtr<nsIRDFResource> mNC_actionBar;
  nsCOMPtr<nsIRDFResource> mNC_searchForm;
  nsCOMPtr<nsIRDFResource> mNC_LastText;
  nsCOMPtr<nsIRDFResource> mNC_URL;
  nsCOMPtr<nsIRDFResource> mRDF_InstanceOf;
  nsCOMPtr<nsIRDFResource> mRDF_type;
  nsCOMPtr<nsIRDFResource> mNC_loading;
  nsCOMPtr<nsIRDFResource> mNC_HTML;
  nsCOMPtr<nsIRDFResource> mNC_Icon;
  nsCOMPtr<nsIRDFResource> mNC_StatusIcon;
  nsCOMPtr<nsIRDFResource> mNC_Banner;
  nsCOMPtr<nsIRDFResource> mNC_Site;
  nsCOMPtr<nsIRDFResource> mNC_Relevance;
  nsCOMPtr<nsIRDFResource> mNC_Date;
  nsCOMPtr<nsIRDFResource> mNC_RelevanceSort;
  nsCOMPtr<nsIRDFResource> mNC_PageRank;
  nsCOMPtr<nsIRDFResource> mNC_Engine;
  nsCOMPtr<nsIRDFResource> mNC_Price;
  nsCOMPtr<nsIRDFResource> mNC_PriceSort;
  nsCOMPtr<nsIRDFResource> mNC_Availability;
  nsCOMPtr<nsIRDFResource> mNC_BookmarkSeparator;
  nsCOMPtr<nsIRDFResource> mNC_Update;
  nsCOMPtr<nsIRDFResource> mNC_UpdateIcon;
  nsCOMPtr<nsIRDFResource> mNC_UpdateCheckDays;
  nsCOMPtr<nsIRDFResource> mWEB_LastPingDate;
  nsCOMPtr<nsIRDFResource> mWEB_LastPingModDate;
  nsCOMPtr<nsIRDFResource> mWEB_LastPingContentLen;

  nsCOMPtr<nsIRDFResource> mNC_SearchCommand_AddToBookmarks;
  nsCOMPtr<nsIRDFResource> mNC_SearchCommand_AddQueryToBookmarks;
  nsCOMPtr<nsIRDFResource> mNC_SearchCommand_FilterResult;
  nsCOMPtr<nsIRDFResource> mNC_SearchCommand_FilterSite;
  nsCOMPtr<nsIRDFResource> mNC_SearchCommand_ClearFilters;

  nsCOMPtr<nsIRDFLiteral>  mTrueLiteral;

protected:
  nsCOMPtr<nsIRDFService>     mRDFService;
  nsCOMPtr<nsIRDFContainerUtils> mRDFC;
  nsCOMPtr<nsIRDFDataSource>  mInner;
  nsCOMPtr<nsIRDFDataSource>  mLocalstore;
  nsCOMPtr<nsISupportsArray>  mUpdateArray;
  nsCOMPtr<nsITimer>          mTimer;
  nsCOMPtr<nsILoadGroup>      mBackgroundLoadGroup;
  nsCOMPtr<nsILoadGroup>      mLoadGroup;
  nsCOMPtr<nsIRDFDataSource>  categoryDataSource;
  PRBool                      busySchedule;
  nsCOMPtr<nsIRDFResource>    busyResource;
  nsString                    mQueryEncodingStr;

  friend  int  PR_CALLBACK  searchModePrefCallback(const char *pref, void *aClosure);

  
  nsresult  GetSearchEngineToPing(nsIRDFResource **theResource, nsCString &updateURL);
  PRBool    isEngineURI(nsIRDFResource* aResource);
  PRBool    isSearchURI(nsIRDFResource* aResource);
  PRBool    isSearchCategoryURI(nsIRDFResource* aResource);
  PRBool    isSearchCategoryEngineURI(nsIRDFResource* aResource);
  PRBool    isSearchCategoryEngineBasenameURI(nsIRDFNode *aResource);
  PRBool    isSearchCommand(nsIRDFResource* aResource);
  nsresult  resolveSearchCategoryEngineURI(nsIRDFResource *source, nsIRDFResource **trueEngine);
  nsresult  BeginSearchRequest(nsIRDFResource *source, PRBool doNetworkRequest);
  nsresult  FindData(nsIRDFResource *engine, nsIRDFLiteral **data);
  nsresult  EngineFileFromResource(nsIRDFResource *aEngineResource,
                                   nsILocalFile **aResult);
  nsresult  updateDataHintsInGraph(nsIRDFResource *engine, const PRUnichar *data);
  nsresult  updateAtom(nsIRDFDataSource *db, nsIRDFResource *src, nsIRDFResource *prop, nsIRDFNode *newValue, PRBool *dirtyFlag);
  nsresult  validateEngine(nsIRDFResource *engine);
  nsresult  DoSearch(nsIRDFResource *source, nsIRDFResource *engine, const nsString &fullURL, const nsString &text);
  nsresult  MapEncoding(const nsString &numericEncoding, 
                        nsString &stringEncoding);
  const char * const MapScriptCodeToCharsetName(PRUint32 aScriptCode);
  nsresult  SaveEngineInfoIntoGraph(nsIFile *file, nsIFile *icon, const PRUnichar *hint, const PRUnichar *data, PRBool isSystemSearchFile);
  nsresult  GetSearchEngineList(nsIFile *spec, PRBool isSystemSearchFile);
  nsresult  GetCategoryList();
  nsresult  ReadFileContents(nsILocalFile *baseFilename, nsString & sourceContents);
  nsresult  DecodeData(const char *aCharset, const PRUnichar *aInString, PRUnichar **aOutString);
  nsresult  GetData(const PRUnichar *data, const char *sectionToFind, PRUint32 sectionNum, const char *attribToFind, nsString &value);
  nsresult  GetNumInterpretSections(const PRUnichar *data, PRUint32 &numInterpretSections);
  nsresult  GetInputs(const PRUnichar *data, nsString &engineName, nsString &userVar, const nsString &text, nsString &input, PRInt16 direction, PRUint16 pageNumber, PRUint16 *whichButtons);
  PRInt32   computeIndex(nsAutoString &factor, PRUint16 page, PRInt16 direction);
  nsresult  GetURL(nsIRDFResource *source, nsIRDFLiteral** aResult);
  nsresult  validateEngineNow(nsIRDFResource *engine);
  nsresult  webSearchFinalize(nsIChannel *channel, nsIInternetSearchContext *context);
  nsresult  ParseHTML(nsIURI *aURL, nsIRDFResource *mParent, nsIRDFResource *engine, const PRUnichar *htmlPage, PRInt32 htmlPageSize);
  nsresult  SetHint(nsIRDFResource *mParent, nsIRDFResource *hintRes);
  nsresult  ConvertEntities(nsString &str, PRBool removeHTMLFlag = PR_TRUE, PRBool removeCRLFsFlag = PR_TRUE, PRBool trimWhiteSpaceFlag = PR_TRUE);
  nsresult  saveContents(nsIChannel* channel, nsIInternetSearchContext *context, PRUint32 contextType);
  PRBool    getSearchURI(nsIRDFResource *src, nsAString &_retval);  
  nsresult  addToBookmarks(nsIRDFResource *src);
  nsresult  addQueryToBookmarks(nsIRDFResource *src);
  nsresult  filterResult(nsIRDFResource *src);
  nsresult  filterSite(nsIRDFResource *src);
  nsresult  clearFilters(void);
  PRBool    isSearchResultFiltered(const nsString &href);
#ifdef MOZ_PHOENIX
  void      ReorderEngineList();
#endif

static  void    FireTimer(nsITimer* aTimer, void* aClosure);

  
  
  
  nsresult AddSearchEngineInternal(const char *engineURL,
                                   const char *iconURL,
                                   const PRUnichar *suggestedTitle,
                                   const PRUnichar *suggestedCategory,
                                   nsIRDFResource *aOldEngineResource);

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(InternetSearchDataSource,
                                           nsIInternetSearchService)
  NS_DECL_NSIINTERNETSEARCHSERVICE
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIRDFDATASOURCE
  NS_DECL_NSIOBSERVER

  InternetSearchDataSource(void);
  virtual    ~InternetSearchDataSource(void);
  NS_METHOD  Init();
  void DeferredInit();
};

#endif 
