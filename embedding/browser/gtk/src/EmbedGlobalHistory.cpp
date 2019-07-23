










































#include "EmbedGlobalHistory.h"
#include "nsIObserverService.h"
#include "nsAutoPtr.h"
#include "nsIURI.h"
#include "nsInt64.h"
#include "nsIIOService.h"
#include "nsNetUtil.h"
#include "gtkmozembed_common.h"
#include "nsISeekableStream.h"
#ifndef MOZILLA_INTERNAL_API
#include "nsCRT.h"
#endif
#include "nsILineInputStream.h"


#define defaultSeparator 1

static const PRInt32 kNewEntriesBetweenFlush = 10;
static const PRInt32 kMaxSafeReadEntriesCount = 2000;

static const PRUint32 kDefaultExpirationIntervalDays = 7;

static const PRInt64 kMSecsPerDay = LL_INIT(0, 60 * 60 * 24 * 1000);
static const PRInt64 kOneThousand = LL_INIT(0, 1000);

static GList *mURLList;                 
static PRInt64 mExpirationInterval;     
static EmbedGlobalHistory *sEmbedGlobalHistory = nsnull;




class HistoryEntry
{
public:
  PRInt64         mLastVisitTime;     
  PRPackedBool    mWritten;           
  nsCString       mTitle;             
  nsCString       mUrl;               
};

#define CLOSE_FILE_HANDLE(file_handle) \
  PR_BEGIN_MACRO \
    if (file_handle) { \
      close_output_stream(file_handle); \
      NS_RELEASE(file_handle); \
    } \
  PR_END_MACRO

static void close_output_stream(OUTPUT_STREAM *file_handle)
{
  file_handle->Close();
  return;
}

static bool file_handle_uri_exists(LOCAL_FILE *uri)
{
  g_return_val_if_fail(uri, false);
  PRBool exists = PR_FALSE;
  uri->Exists(&exists);
  return exists;
}

static LOCAL_FILE* file_handle_uri_new(const char *uri)
{
  g_return_val_if_fail(uri, nsnull);
  nsresult rv;
  LOCAL_FILE *historyFile = nsnull;
  rv = NS_NewNativeLocalFile(nsDependentCString(uri), 1, &historyFile);
  return historyFile;
}

static void file_handle_uri_release(LOCAL_FILE *uri)
{
  g_return_if_fail(uri);
  NS_RELEASE(uri);
}


static bool file_handle_create_uri(OUTPUT_STREAM **file_handle, LOCAL_FILE *uri)
{
  g_return_val_if_fail(file_handle, false);
  nsresult rv;
  rv = NS_NewLocalFileOutputStream(file_handle, uri, PR_RDWR | PR_APPEND | PR_CREATE_FILE, 0660);

  return NS_SUCCEEDED(rv);
}

static bool file_handle_open_uri(OUTPUT_STREAM **file_handle, LOCAL_FILE *uri)
{
  g_return_val_if_fail(file_handle, false);
  nsresult rv;
  rv = NS_NewLocalFileOutputStream(file_handle, uri, PR_RDWR, 0660);

  return NS_SUCCEEDED(rv);
}

static bool file_handle_seek(OUTPUT_STREAM *file_handle, gboolean end)
{
  g_return_val_if_fail(file_handle, false);
  nsresult rv;
  nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(file_handle, &rv);
  rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, end ? -1 : 0);
  return NS_SUCCEEDED(rv);
}

static bool file_handle_truncate(OUTPUT_STREAM *file_handle)
{
  g_return_val_if_fail(file_handle, false);
  nsresult rv;
  nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(file_handle, &rv);
  rv = seekable->SetEOF();
  return NS_SUCCEEDED(rv);
}

static guint64 file_handle_write(OUTPUT_STREAM *file_handle, gpointer line)
{
  g_return_val_if_fail(file_handle, 0);
  PRUint32 amt = 0;
  nsresult rv;
  rv = file_handle->Write((char*)line, strlen((char*)line), &amt);
  
  return NS_SUCCEEDED(rv);
}



static nsresult writeEntry(OUTPUT_STREAM *file_handle, HistoryEntry *entry);

nsresult OnVisited(HistoryEntry *entry)
{
  NS_ENSURE_ARG(entry);
  entry->mLastVisitTime = PR_Now();
  LL_DIV(entry->mLastVisitTime, entry->mLastVisitTime, kOneThousand);
  return NS_OK;
}


PRInt64 GetLastVisitTime(HistoryEntry *entry)
{
  NS_ENSURE_ARG(entry);
  return entry->mLastVisitTime;
}


nsresult SetLastVisitTime(HistoryEntry *entry, const PRInt64& aTime)
{
  NS_ENSURE_ARG(entry);
  NS_ENSURE_ARG_POINTER(aTime);
  entry->mLastVisitTime = aTime;
  return NS_OK;
}


PRBool GetIsWritten(HistoryEntry *entry)
{
  NS_ENSURE_ARG(entry);
  return entry->mWritten;
}


nsresult SetIsWritten(HistoryEntry *entry)
{
  NS_ENSURE_ARG(entry);
  entry->mWritten = PR_TRUE;
  return NS_OK;
}


#define SET_TITLE(entry, aTitle) (entry->mTitle.Assign(aTitle))


#define GET_TITLE(entry) (entry && !entry->mTitle.IsEmpty() ? entry->mTitle.get() : "")


nsresult SET_URL(HistoryEntry *aEntry, const char *aUrl)
{
  NS_ENSURE_ARG(aEntry);
  NS_ENSURE_ARG(aUrl);
  aEntry->mUrl.Assign(aUrl);
  return NS_OK;
}


const char* GET_URL(HistoryEntry *aEntry)
{
  return (aEntry && !aEntry->mUrl.IsEmpty()) ? aEntry->mUrl.get() : "";
}


int history_entry_find_exist(gconstpointer a, gconstpointer b)
{
  return g_ascii_strcasecmp((char*)GET_URL((HistoryEntry *)a), (char *) b);
}


int find_insertion_place(gconstpointer a, gconstpointer b)
{
  PRInt64 lastVisitTime = GetLastVisitTime((HistoryEntry *) a);
  PRInt64 tempTime = GetLastVisitTime((HistoryEntry *) b);
  return LL_CMP(lastVisitTime, <, tempTime);
}


PRBool entryHasExpired(HistoryEntry *entry)
{
  
  PRInt64 nowInMilliSecs = PR_Now();
  LL_DIV(nowInMilliSecs, nowInMilliSecs, kOneThousand);
  
  PRInt64 expirationIntervalAgo;
  LL_SUB(expirationIntervalAgo, nowInMilliSecs, mExpirationInterval);
  PRInt64 lastVisitTime = GetLastVisitTime(entry);
  return LL_CMP(lastVisitTime, <, expirationIntervalAgo);
}


void history_entry_foreach_to_remove(gpointer data, gpointer user_data)
{
  HistoryEntry *entry = (HistoryEntry *) data;
  if (entry) {
    entry->mLastVisitTime = 0;
    delete entry;
  }
}




NS_IMPL_ISUPPORTS2(EmbedGlobalHistory, nsIGlobalHistory2, nsIObserver)

EmbedGlobalHistory*
EmbedGlobalHistory::GetInstance()
{
  if (!sEmbedGlobalHistory)
  {
    sEmbedGlobalHistory = new EmbedGlobalHistory();
    if (!sEmbedGlobalHistory)
      return nsnull;
    NS_ADDREF(sEmbedGlobalHistory);   
    if (NS_FAILED(sEmbedGlobalHistory->Init()))
    {
      NS_RELEASE(sEmbedGlobalHistory);
      return nsnull;
    }
  }
  else
    NS_ADDREF(sEmbedGlobalHistory);   
  return sEmbedGlobalHistory;
}


void
EmbedGlobalHistory::DeleteInstance()
{
  if (sEmbedGlobalHistory)
  {
    delete sEmbedGlobalHistory;
  }
}


EmbedGlobalHistory::EmbedGlobalHistory()
: mFileHandle(nsnull)
{
  if (!mURLList) {
    mDataIsLoaded = PR_FALSE;
    mFlushModeFullWriteNeeded = PR_FALSE;
    mEntriesAddedSinceFlush = 0;
    mHistoryFile = nsnull;
    LL_I2L(mExpirationInterval, kDefaultExpirationIntervalDays);
    LL_MUL(mExpirationInterval, mExpirationInterval, kMSecsPerDay);
  }
}


EmbedGlobalHistory::~EmbedGlobalHistory()
{
  LoadData();
  FlushData(kFlushModeFullWrite);
  if (mURLList) {
    g_list_foreach(mURLList, (GFunc) history_entry_foreach_to_remove, NULL);
    g_list_free(mURLList);
    mURLList = NULL;
  }
  if (mFileHandle) {
    CLOSE_FILE_HANDLE(mFileHandle);
  }
  if (mHistoryFile) {
    g_free(mHistoryFile);
    mHistoryFile = nsnull;
  }
  if (sEmbedGlobalHistory)
    sEmbedGlobalHistory = nsnull;
}


NS_IMETHODIMP EmbedGlobalHistory::Init()
{
  if (mURLList) return NS_OK;
  

  PRInt32 expireDays;
  int success = gtk_moz_embed_common_get_pref(G_TYPE_INT, EMBED_HISTORY_PREF_EXPIRE_DAYS, &expireDays);
  if (success) {
    LL_I2L(mExpirationInterval, expireDays);
    LL_MUL(mExpirationInterval, mExpirationInterval, kMSecsPerDay);
  }
  
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
  NS_ASSERTION(observerService, "failed to get observer service");
  if (observerService) {
    observerService->AddObserver(this, "quit-application", PR_FALSE);
    observerService->AddObserver(this, "RemoveEntries", PR_FALSE);
  }
  nsresult rv = InitFile();
  if (NS_FAILED(rv))
    return NS_ERROR_FAILURE;
  rv = LoadData();
  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

#define BROKEN_RV_HANDLING_CODE(rv) PR_BEGIN_MACRO                        \
  if (NS_FAILED(rv)) {                                                    \
    /* OOPS the coder (not timeless) didn't handle this case well at all. \
     * unfortunately the coder will remain anonymous.                     \
     * XXX please fix me.                                                 \
     */                                                                   \
  }                                                                       \
  PR_END_MACRO

#define BROKEN_STRING_GETTER(out) PR_BEGIN_MACRO                      \
  /* OOPS the coder (not timeless) decided not to do anything in this \
   * method, but to return NS_OK anyway. That's not very polite.      \
   */                                                                 \
  out.Truncate();                                                     \
  PR_END_MACRO

#define BROKEN_STRING_BUILDER(var) PR_BEGIN_MACRO \
  /* This is just wrong */                        \
  PR_END_MACRO

#define UNACCEPTABLE_CRASHY_GLIB_ALLOCATION(newed) PR_BEGIN_MACRO \
  /* OOPS this code is using a glib allocation function which     \
   * will cause the application to crash when it runs out of      \
   * memory. This is not cool. either g_try methods should be     \
   * used or malloc, or new (note that gecko /will/ be replacing  \
   * its operator new such that new will not throw exceptions).   \
   * XXX please fix me.                                           \
   */                                                             \
  if (!newed) {                                                   \
  }                                                               \
  PR_END_MACRO

#define ALLOC_NOT_CHECKED(newed) PR_BEGIN_MACRO               \
  /* This might not crash, but the code probably isn't really \
   * designed to handle it, perhaps the code should be fixed? \
   */                                                         \
  if (!newed) {                                               \
  }                                                           \
  PR_END_MACRO





NS_IMETHODIMP EmbedGlobalHistory::AddURI(nsIURI *aURI, PRBool aRedirect, PRBool aToplevel, nsIURI *aReferrer)
{
  NS_ENSURE_ARG(aURI);
  nsCAutoString URISpec;
  aURI->GetSpec(URISpec);
  const char *aURL = URISpec.get();
  if (!aToplevel)
    return NS_OK;
  PRBool isHTTP, isHTTPS;
  nsresult rv = NS_OK;
  rv |= aURI->SchemeIs("http", &isHTTP);
  rv |= aURI->SchemeIs("https", &isHTTPS);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
  
  if (!isHTTP && !isHTTPS)
  {
    



    PRBool isAbout, isImap, isNews, isMailbox, isViewSource, isChrome, isData, isJavascript;
    rv  = aURI->SchemeIs("about", &isAbout);
    rv |= aURI->SchemeIs("imap", &isImap);
    rv |= aURI->SchemeIs("news", &isNews);
    rv |= aURI->SchemeIs("mailbox", &isMailbox);
    rv |= aURI->SchemeIs("view-source", &isViewSource);
    rv |= aURI->SchemeIs("chrome", &isChrome);
    rv |= aURI->SchemeIs("data", &isData);
    rv |= aURI->SchemeIs("javascript", &isJavascript);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);
    if (isAbout ||
        isImap ||
        isNews ||
        isMailbox ||
        isViewSource ||
        isChrome ||
        isData ||
        isJavascript) {
      return NS_OK;
    }
  }
#ifdef DEBUG
  
#endif
  rv = LoadData();
  NS_ENSURE_SUCCESS(rv, rv);
  GList *node = g_list_find_custom(mURLList, aURL, (GCompareFunc) history_entry_find_exist);
  HistoryEntry *entry = NULL;
  if (node && node->data)
    entry = (HistoryEntry *)(node->data);
  nsCAutoString hostname;
  aURI->GetHost(hostname);

  
  if (!entry) {
    entry = new HistoryEntry;
    ALLOC_NOT_CHECKED(entry);
    rv |= OnVisited(entry);
    SET_TITLE(entry, hostname);
    rv |= SET_URL(entry, aURL);
    BROKEN_RV_HANDLING_CODE(rv);
    unsigned int listSize = g_list_length(mURLList);
    if (listSize+1 > kDefaultMaxSize) {
      GList *last = g_list_last(mURLList);
      mURLList = g_list_remove(mURLList, last->data);
    }
    mURLList = g_list_insert_sorted(mURLList, entry,
                                    (GCompareFunc) find_insertion_place);
    
    BROKEN_RV_HANDLING_CODE(rv);
    if (++mEntriesAddedSinceFlush >= kNewEntriesBetweenFlush)
      rv |= FlushData(kFlushModeAppend);
    
  } else {
    
    rv |= OnVisited(entry);
    SET_TITLE(entry, hostname);
    
    BROKEN_RV_HANDLING_CODE(rv);
    mURLList = g_list_remove(mURLList, entry);
    mURLList = g_list_insert_sorted(mURLList, entry, (GCompareFunc) find_insertion_place);
    
    BROKEN_RV_HANDLING_CODE(rv);
    
    mFlushModeFullWriteNeeded = PR_TRUE;
    if (++mEntriesAddedSinceFlush >= kNewEntriesBetweenFlush)
      rv |= FlushData(kFlushModeFullWrite);
  }
  return rv;
}


NS_IMETHODIMP EmbedGlobalHistory::IsVisited(nsIURI *aURI, PRBool *_retval)
{
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(_retval);
  nsCAutoString URISpec;
  aURI->GetSpec(URISpec);
  const char *aURL = URISpec.get();
  nsresult rv = LoadData();
  NS_ENSURE_SUCCESS(rv, rv);
  GList *node = g_list_find_custom(mURLList, aURL,
                                   (GCompareFunc) history_entry_find_exist);
  *_retval = (node && node->data);
  return rv;
}


NS_IMETHODIMP EmbedGlobalHistory::SetPageTitle(nsIURI *aURI,
                                               const nsAString & aTitle)
{
  NS_ENSURE_ARG(aURI);
  nsresult rv;
  
  PRBool isAbout;
  rv = aURI->SchemeIs("about", &isAbout);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isAbout)
    return NS_OK;
  nsCAutoString URISpec;
  aURI->GetSpec(URISpec);
  const char *aURL = URISpec.get();
  rv |= LoadData();
  BROKEN_RV_HANDLING_CODE(rv);
  NS_ENSURE_SUCCESS(rv, rv);

  GList *node = g_list_find_custom(mURLList, aURL,
                                   (GCompareFunc) history_entry_find_exist);
  HistoryEntry *entry = NULL;
  if (node)
    entry = (HistoryEntry *)(node->data);
  if (entry) {
    SET_TITLE(entry, NS_ConvertUTF16toUTF8(aTitle).get());
    BROKEN_RV_HANDLING_CODE(rv);
    
    mFlushModeFullWriteNeeded = PR_TRUE;
    if (++mEntriesAddedSinceFlush >= kNewEntriesBetweenFlush)
      rv |= FlushData(kFlushModeFullWrite);
    BROKEN_RV_HANDLING_CODE(rv);
  }
  return rv;
}

nsresult EmbedGlobalHistory::RemoveEntries(const PRUnichar *url, int time)
{
  nsresult rv = NS_ERROR_FAILURE;
  if (!mURLList)
    return rv;

  if (url) {
    GList *node = g_list_find_custom(mURLList, NS_ConvertUTF16toUTF8(url).get(), (GCompareFunc) history_entry_find_exist);
    if (!node) return rv;
    if (node->data) {
      HistoryEntry *entry = static_cast<HistoryEntry *>
                                       (node->data);

      entry->mLastVisitTime = 0;
      delete entry;
      mURLList = g_list_remove(mURLList, entry);
    }
  } else {
    g_list_foreach (mURLList, (GFunc) history_entry_foreach_to_remove, NULL);
    g_list_free(mURLList);
    mURLList = NULL;
  }

  mFlushModeFullWriteNeeded = PR_TRUE;
  mEntriesAddedSinceFlush++;
  rv = FlushData(kFlushModeFullWrite);

  return rv;
}




NS_IMETHODIMP EmbedGlobalHistory::Observe(nsISupports *aSubject,
                                          const char *aTopic,
                                          const PRUnichar *aData)
{
  nsresult rv = NS_OK;
  
  if (strcmp(aTopic, "quit-application") == 0) {
    rv = LoadData();
    
    rv |= FlushData(kFlushModeFullWrite);
    if (mURLList) {
      g_list_foreach(mURLList, (GFunc) history_entry_foreach_to_remove, NULL);
      g_list_free(mURLList);
      mURLList = NULL;
    }
    if (mFileHandle) {
      CLOSE_FILE_HANDLE(mFileHandle);
    }
  } else if (strcmp(aTopic, "RemoveEntries") == 0) {
    rv |= RemoveEntries(aData, 0);
  }
  return rv;
}

static nsresult
GetHistoryFileName(char **aHistoryFile)
{
  NS_ENSURE_ARG_POINTER(aHistoryFile);
  
  
  
  if (EmbedPrivate::sProfileDir) {
    nsCString path;
    EmbedPrivate::sProfileDir->GetNativePath(path);
    *aHistoryFile = g_strdup_printf("%s/history.dat", path.get());
    BROKEN_STRING_BUILDER(aHistoryFile);
  } else {
    *aHistoryFile = g_strdup_printf("%s/history.dat", g_get_tmp_dir());
    BROKEN_STRING_BUILDER(aHistoryFile);
  }
  return NS_OK;
}




nsresult EmbedGlobalHistory::InitFile()
{
  if (!mHistoryFile) {
    if (NS_FAILED(GetHistoryFileName(&mHistoryFile)))
      return NS_ERROR_FAILURE;
  }

  LOCAL_FILE *uri = file_handle_uri_new(mHistoryFile);
  if (!uri)
    return NS_ERROR_FAILURE;

  gboolean rs = FALSE;
  if (!file_handle_uri_exists(uri)) {
    if (!file_handle_create_uri(&mFileHandle, uri)) {
      NS_WARNING("Could not create a history file\n");
      file_handle_uri_release(uri);
      return NS_ERROR_FAILURE;
    }
    CLOSE_FILE_HANDLE(mFileHandle);
  }
  rs = file_handle_open_uri(&mFileHandle, uri);

  file_handle_uri_release(uri);

  if (!rs) {
    NS_WARNING("Could not open a history file\n");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}


nsresult EmbedGlobalHistory::LoadData()
{
  nsresult rv = NS_OK;
  if (!mDataIsLoaded) {
    mDataIsLoaded = PR_TRUE;
    LOCAL_FILE *uri = file_handle_uri_new(mHistoryFile);
    if (uri) {
      rv |= ReadEntries(uri);
      file_handle_uri_release(uri);
    }
  }
  return rv;
}


nsresult EmbedGlobalHistory::WriteEntryIfWritten(GList *list, OUTPUT_STREAM *file_handle)
{
  if (!file_handle)
    return NS_ERROR_FAILURE;

  unsigned int counter = g_list_length(list);
  while (counter > 0) {
    HistoryEntry *entry = static_cast<HistoryEntry*>(g_list_nth_data(list, counter-1));
    counter--;
    if (!entry || entryHasExpired(entry)) {
      continue;
    }
    writeEntry(file_handle, entry);
  }
  return NS_OK;
}


nsresult EmbedGlobalHistory::WriteEntryIfUnwritten(GList *list, OUTPUT_STREAM *file_handle)
{
  if (!file_handle)
    return NS_ERROR_FAILURE;
  unsigned int counter = g_list_length(list);
  while (counter > 0) {
    HistoryEntry *entry = static_cast<HistoryEntry*>(g_list_nth_data(list, counter-1));
    if (!entry || entryHasExpired(entry)) {
      counter--;
      continue;
    }
    if (!GetIsWritten(entry))
      writeEntry(file_handle, entry);
    counter--;
  }
  return NS_OK;
}


nsresult EmbedGlobalHistory::FlushData(PRIntn mode)
{
  nsresult rv = NS_OK;
  if (mEntriesAddedSinceFlush == 0)
    return NS_OK;
  if (!mHistoryFile)
  {
    rv = InitFile();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = FlushData(kFlushModeFullWrite);
    return rv;
  }
  LOCAL_FILE *uri = file_handle_uri_new(mHistoryFile);
  if (!uri) return NS_ERROR_FAILURE;

  gboolean rs = file_handle_uri_exists(uri);
  file_handle_uri_release(uri);

  if (!rs && NS_FAILED(rv))
    return NS_ERROR_FAILURE;

  if (mode == kFlushModeFullWrite || mFlushModeFullWriteNeeded == PR_TRUE)
  {
    if (!file_handle_seek(mFileHandle, FALSE))
      return NS_ERROR_FAILURE;
    if (!file_handle_truncate(mFileHandle))
      return NS_ERROR_FAILURE;
    WriteEntryIfWritten(mURLList, mFileHandle);
    mFlushModeFullWriteNeeded = PR_FALSE;
  }
  else
  {
    if (!file_handle_seek(mFileHandle, TRUE))
      return NS_ERROR_FAILURE;
    WriteEntryIfUnwritten(mURLList, mFileHandle);
  }

  mEntriesAddedSinceFlush = 0;
  return NS_OK;
}



nsresult EmbedGlobalHistory::GetEntry(const char *entry)
{
  char separator = (char) defaultSeparator;
  int pos = 0;
  nsInt64 outValue = 0;
  while (PR_TRUE) {
    PRInt32 digit;
    if (entry[pos] == separator) {
      pos++;
      break;
    }
    if (entry[pos] == '\0' || !isdigit(entry[pos]))
      return NS_ERROR_FAILURE;
    digit = entry[pos] - '0';
    outValue *= nsInt64(10);
    outValue += nsInt64(digit);
    pos++;
  }
  char url[1024], title[1024];
  int urlLength= 0, titleLength= 0, numStrings=1;
  
  
  while(PR_TRUE) {
    if (entry[pos] == separator) {
      numStrings++;
      pos++;
      continue;
    }
    if (numStrings > 2)
      break;
    if (numStrings==1) {
      url[urlLength++] = entry[pos];
    } else {
      title[titleLength++] = entry[pos];
  }
  pos++;
  }
  url[urlLength]='\0';
  title[titleLength]='\0';
  HistoryEntry *newEntry = new HistoryEntry;
  if (!newEntry)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = NS_OK;
  SET_TITLE(newEntry, title);
  rv |= SetLastVisitTime(newEntry, outValue);
  rv |= SetIsWritten(newEntry);
  rv |= SET_URL(newEntry, url);
  BROKEN_RV_HANDLING_CODE(rv);
  
  if (!entryHasExpired(newEntry)) {
    mURLList = g_list_prepend(mURLList, newEntry);
  }
  return rv;
}



nsresult EmbedGlobalHistory::ReadEntries(LOCAL_FILE *file_uri)
{
  if (!file_uri)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;

  nsCOMPtr<nsIInputStream> fileStream;
  NS_NewLocalFileInputStream(getter_AddRefs(fileStream), file_uri);
  if (!fileStream)
    return NS_ERROR_OUT_OF_MEMORY;
  nsCOMPtr<nsILineInputStream> lineStream = do_QueryInterface(fileStream, &rv);
  NS_ASSERTION(lineStream, "File stream is not an nsILineInputStream");
  
  nsCString utf8Buffer;
  PRBool moreData = PR_FALSE;

  PRInt32 safe_limit = 0;
  do {
    rv = lineStream->ReadLine(utf8Buffer, &moreData);
    safe_limit++;
    if (NS_FAILED(rv))
      return NS_OK;

    if (utf8Buffer.IsEmpty())
      continue;
    rv = GetEntry(utf8Buffer.get());
  } while (moreData && safe_limit < kMaxSafeReadEntriesCount);
  fileStream->Close();

  return rv;
}





static nsresult writePRInt64(char time[14], const PRInt64& inValue)
{
  nsInt64 value(inValue);
  if (value == nsInt64(0)) {
    strcpy(time, "0");
    return NS_OK;
  }
  nsCAutoString tempString;
  while (value != nsInt64(0)) {
    PRInt32 ones = PRInt32(value % nsInt64(10));
    value /= nsInt64(10);
    tempString.Insert(char('0' + ones), 0);
  }
  strcpy(time,(char *) tempString.get());
  return NS_OK;
}


nsresult writeEntry(OUTPUT_STREAM *file_handle, HistoryEntry *entry)
{
  nsresult rv = NS_OK;
  char sep = (char) defaultSeparator;
  char time[14];
  writePRInt64(time, GetLastVisitTime(entry));
  char *line = g_strdup_printf("%s%c%s%c%s%c\n", time, sep, GET_URL(entry), sep, GET_TITLE(entry), sep);
  BROKEN_STRING_BUILDER(line);
  guint64 size = file_handle_write(file_handle, (gpointer)line);
  if (size != strlen(line))
    rv = NS_ERROR_FAILURE;
  rv |= SetIsWritten(entry);
  g_free(line);
  return rv;
}

nsresult EmbedGlobalHistory::GetContentList(GtkMozHistoryItem **GtkHI, int *count)
{
  if (!mURLList) return NS_ERROR_FAILURE;

  unsigned int num_items = 0;
  *GtkHI = g_new0(GtkMozHistoryItem, g_list_length(mURLList));
  UNACCEPTABLE_CRASHY_GLIB_ALLOCATION(*GtkHI);
  GtkMozHistoryItem * item = (GtkMozHistoryItem *)*GtkHI;
  while (num_items < g_list_length(mURLList)) {
    HistoryEntry *entry = static_cast<HistoryEntry*>
                                     (g_list_nth_data(mURLList, num_items));
    
    if (entryHasExpired(entry)) {
      break;
    }
    glong accessed;
    PRInt64 temp, outValue;
    LL_MUL(outValue, GetLastVisitTime(entry), kOneThousand);
    LL_DIV(temp, outValue, PR_USEC_PER_SEC);
    LL_L2I(accessed, temp);
    
    item[num_items].title = GET_TITLE(entry);
    BROKEN_STRING_BUILDER(item[num_items].title);
    item[num_items].url = GET_URL(entry);
    item[num_items].accessed = accessed;
    num_items++;
  }
  *count = num_items;
  return NS_OK;
}

