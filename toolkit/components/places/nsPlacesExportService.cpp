

























































#include "nsPlacesExportService.h"
#include "nsNetUtil.h"
#include "nsParserCIID.h"
#include "nsUnicharUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsToolkitCompsCID.h"
#include "nsIParser.h"
#include "prprf.h"
#include "nsIObserverService.h"
#include "nsISupportsPrimitives.h"
#include "nsPlacesMacros.h"
#include "mozilla/Util.h"

using namespace mozilla;

#define LOAD_IN_SIDEBAR_ANNO NS_LITERAL_CSTRING("bookmarkProperties/loadInSidebar")
#define DESCRIPTION_ANNO NS_LITERAL_CSTRING("bookmarkProperties/description")
#define POST_DATA_ANNO NS_LITERAL_CSTRING("bookmarkProperties/POSTData")

#define LMANNO_FEEDURI "livemark/feedURI"
#define LMANNO_SITEURI "livemark/siteURI"




#if defined(XP_WIN) || defined(XP_OS2)
#define NS_LINEBREAK "\015\012"
#else
#define NS_LINEBREAK "\012"
#endif

class nsIOutputStream;
static const char kWhitespace[] = " \r\n\t\b";
static nsresult WriteEscapedUrl(const nsCString &aString, nsIOutputStream* aOutput);




char*
nsEscapeHTML(const char* string)
{
  
  char* escaped = nullptr;
  uint32_t len = strlen(string);
  if (len >= (PR_UINT32_MAX / 6))
    return nullptr;

  escaped = (char*)NS_Alloc((len * 6) + 1);
  if (escaped) {
    char* ptr = escaped;
    for (; *string != '\0'; string++) {
      switch(*string) {
        case '<':
          *ptr++ = '&';
          *ptr++ = 'l';
          *ptr++ = 't';
          *ptr++ = ';';
          break;
        case '>':
          *ptr++ = '&';
          *ptr++ = 'g';
          *ptr++ = 't';
          *ptr++ = ';';
          break;
        case '&':
          *ptr++ = '&';
          *ptr++ = 'a';
          *ptr++ = 'm';
          *ptr++ = 'p';
          *ptr++ = ';';
          break;
        case '"':
          *ptr++ = '&';
          *ptr++ = 'q';
          *ptr++ = 'u';
          *ptr++ = 'o';
          *ptr++ = 't';
          *ptr++ = ';';
          break;
        case '\'':
          *ptr++ = '&';
          *ptr++ = '#';
          *ptr++ = '3';
          *ptr++ = '9';
          *ptr++ = ';';
          break;
        default:
          *ptr++ = *string;
      }
    }
    *ptr = '\0';
  }
  return escaped;
}

PLACES_FACTORY_SINGLETON_IMPLEMENTATION(nsPlacesExportService, gExportService)

NS_IMPL_ISUPPORTS1(nsPlacesExportService, nsIPlacesImportExportService)

nsPlacesExportService::nsPlacesExportService()
{
  NS_ASSERTION(!gExportService,
               "Attempting to create two instances of the service!");
  gExportService = this;
}

nsPlacesExportService::~nsPlacesExportService()
{
  NS_ASSERTION(gExportService == this,
               "Deleting a non-singleton instance of the service");
  if (gExportService == this)
    gExportService = nullptr;
}

nsresult
nsPlacesExportService::Init()
{
  
  mHistoryService = do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(mHistoryService, NS_ERROR_OUT_OF_MEMORY);
  mFaviconService = do_GetService(NS_FAVICONSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(mFaviconService, NS_ERROR_OUT_OF_MEMORY);
  mAnnotationService = do_GetService(NS_ANNOTATIONSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(mAnnotationService, NS_ERROR_OUT_OF_MEMORY);
  mBookmarksService = do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(mBookmarksService, NS_ERROR_OUT_OF_MEMORY);
  mLivemarkService = do_GetService(NS_LIVEMARKSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(mLivemarkService, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

static char kFileIntro[] =
    "<!DOCTYPE NETSCAPE-Bookmark-file-1>" NS_LINEBREAK
    
    "<!-- This is an automatically generated file." NS_LINEBREAK
    "     It will be read and overwritten." NS_LINEBREAK
    "     DO NOT EDIT! -->" NS_LINEBREAK
    "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">" NS_LINEBREAK
    "<TITLE>Bookmarks</TITLE>" NS_LINEBREAK;
static const char kRootIntro[] = "<H1";
static const char kCloseRootH1[] = "</H1>" NS_LINEBREAK NS_LINEBREAK;

static const char kBookmarkIntro[] = "<DL><p>" NS_LINEBREAK;
static const char kBookmarkClose[] = "</DL><p>" NS_LINEBREAK;
static const char kContainerIntro[] = "<DT><H3";
static const char kContainerClose[] = "</H3>" NS_LINEBREAK;
static const char kItemOpen[] = "<DT><A";
static const char kItemClose[] = "</A>" NS_LINEBREAK;
static const char kSeparator[] = "<HR";
static const char kQuoteStr[] = "\"";
static const char kCloseAngle[] = ">";
static const char kIndent[] = "    ";
static const char kDescriptionIntro[] = "<DD>";
static const char kDescriptionClose[] = NS_LINEBREAK;

static const char kPlacesRootAttribute[] = " PLACES_ROOT=\"true\"";
static const char kBookmarksRootAttribute[] = " BOOKMARKS_MENU=\"true\"";
static const char kToolbarFolderAttribute[] = " PERSONAL_TOOLBAR_FOLDER=\"true\"";
static const char kUnfiledBookmarksFolderAttribute[] = " UNFILED_BOOKMARKS_FOLDER=\"true\"";
static const char kIconAttribute[] = " ICON=\"";
static const char kIconURIAttribute[] = " ICON_URI=\"";
static const char kHrefAttribute[] = " HREF=\"";
static const char kFeedURIAttribute[] = " FEEDURL=\"";
static const char kWebPanelAttribute[] = " WEB_PANEL=\"true\"";
static const char kKeywordAttribute[] = " SHORTCUTURL=\"";
static const char kPostDataAttribute[] = " POST_DATA=\"";
static const char kNameAttribute[] = " NAME=\"";
static const char kMicsumGenURIAttribute[]    = " MICSUM_GEN_URI=\"";
static const char kDateAddedAttribute[] = " ADD_DATE=\"";
static const char kLastModifiedAttribute[] = " LAST_MODIFIED=\"";
static const char kLastCharsetAttribute[] = " LAST_CHARSET=\"";







static nsresult
WriteContainerPrologue(const nsACString& aIndent, nsIOutputStream* aOutput)
{
  uint32_t dummy;
  nsresult rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kBookmarkIntro, sizeof(kBookmarkIntro)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}







static nsresult
WriteContainerEpilogue(const nsACString& aIndent, nsIOutputStream* aOutput)
{
  uint32_t dummy;
  nsresult rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kBookmarkClose, sizeof(kBookmarkClose)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}







static nsresult
WriteFaviconAttribute(const nsACString& aURI, nsIOutputStream* aOutput)
{
  uint32_t dummy;

  
  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURI);
  if (NS_FAILED(rv)) {
    nsAutoCString warnMsg;
    warnMsg.Append("Bookmarks Export: Found invalid favicon '");
    warnMsg.Append(aURI);
    warnMsg.Append("'");
    NS_WARNING(warnMsg.get());
    return NS_OK;
  }

  
  nsCOMPtr<nsIFaviconService> faviconService = do_GetService(NS_FAVICONSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> faviconURI;
  rv = faviconService->GetFaviconForPage(uri, getter_AddRefs(faviconURI));
  if (rv == NS_ERROR_NOT_AVAILABLE)
    return NS_OK; 
  NS_ENSURE_SUCCESS(rv, rv); 

  nsAutoCString faviconScheme;
  nsAutoCString faviconSpec;
  rv = faviconURI->GetSpec(faviconSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = faviconURI->GetScheme(faviconScheme);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kIconURIAttribute, sizeof(kIconURIAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteEscapedUrl(faviconSpec, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!faviconScheme.EqualsLiteral("chrome")) {
    

    nsAutoString faviconContents;
    rv = faviconService->GetFaviconDataAsDataURL(faviconURI, faviconContents);
    NS_ENSURE_SUCCESS(rv, rv);
    if (faviconContents.Length() > 0) {
      rv = aOutput->Write(kIconAttribute, sizeof(kIconAttribute)-1, &dummy);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ConvertUTF16toUTF8 utf8Favicon(faviconContents);
      rv = aOutput->Write(utf8Favicon.get(), utf8Favicon.Length(), &dummy);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}






static nsresult
WriteDateAttribute(const char aAttributeStart[], int32_t aLength, PRTime aAttributeValue, nsIOutputStream* aOutput)
{
  
  uint32_t dummy;
  nsresult rv = aOutput->Write(aAttributeStart, aLength, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  aAttributeValue /= 1000000; 

  
  char dateInSeconds[32];
  PR_snprintf(dateInSeconds, sizeof(dateInSeconds), "%lld", aAttributeValue);
  rv = aOutput->Write(dateInSeconds, strlen(dateInSeconds), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





nsresult
nsPlacesExportService::WriteContainer(nsINavHistoryResultNode* aFolder,
                                            const nsACString& aIndent,
                                            nsIOutputStream* aOutput)
{
  nsresult rv = WriteContainerHeader(aFolder, aIndent, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteContainerPrologue(aIndent, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteContainerContents(aFolder, aIndent, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteContainerEpilogue(aIndent, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}






nsresult
nsPlacesExportService::WriteContainerHeader(nsINavHistoryResultNode* aFolder,
                                                  const nsACString& aIndent,
                                                  nsIOutputStream* aOutput)
{
  uint32_t dummy;
  nsresult rv;

  
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aOutput->Write(kContainerIntro, sizeof(kContainerIntro)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int64_t folderId;
  rv = aFolder->GetItemId(&folderId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRTime dateAdded = 0;
  rv = aFolder->GetDateAdded(&dateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  if (dateAdded) {
    rv = WriteDateAttribute(kDateAddedAttribute, sizeof(kDateAddedAttribute)-1, dateAdded, aOutput);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRTime lastModified = 0;
  rv = aFolder->GetLastModified(&lastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  if (lastModified) {
    rv = WriteDateAttribute(kLastModifiedAttribute, sizeof(kLastModifiedAttribute)-1, lastModified, aOutput);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  int64_t placesRoot;
  rv = mBookmarksService->GetPlacesRoot(&placesRoot);
  NS_ENSURE_SUCCESS(rv, rv);

  int64_t bookmarksMenuFolder;
  rv = mBookmarksService->GetBookmarksMenuFolder(&bookmarksMenuFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  int64_t toolbarFolder;
  rv = mBookmarksService->GetToolbarFolder(&toolbarFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  int64_t unfiledBookmarksFolder;
  rv = mBookmarksService->GetUnfiledBookmarksFolder(&unfiledBookmarksFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (folderId == placesRoot) {
    rv = aOutput->Write(kPlacesRootAttribute, sizeof(kPlacesRootAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (folderId == bookmarksMenuFolder) {
    rv = aOutput->Write(kBookmarksRootAttribute, sizeof(kBookmarksRootAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (folderId == unfiledBookmarksFolder) {
    rv = aOutput->Write(kUnfiledBookmarksFolderAttribute, sizeof(kUnfiledBookmarksFolderAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (folderId == toolbarFolder) {
    rv = aOutput->Write(kToolbarFolderAttribute, sizeof(kToolbarFolderAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteTitle(aFolder, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kContainerClose, sizeof(kContainerClose)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteDescription(folderId, nsINavBookmarksService::TYPE_FOLDER, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}





nsresult
nsPlacesExportService::WriteTitle(nsINavHistoryResultNode* aItem,
                                        nsIOutputStream* aOutput)
{
  
  uint32_t type = 0;
  nsresult rv = aItem->GetType(&type);
  NS_ENSURE_SUCCESS(rv, rv);
  if (type == nsINavHistoryResultNode::RESULT_TYPE_SEPARATOR)
    return NS_ERROR_INVALID_ARG;

  nsAutoCString title;
  rv = aItem->GetTitle(title);
  NS_ENSURE_SUCCESS(rv, rv);

  char* escapedTitle = nsEscapeHTML(title.get());
  if (escapedTitle) {
    uint32_t dummy;
    rv = aOutput->Write(escapedTitle, strlen(escapedTitle), &dummy);
    nsMemory::Free(escapedTitle);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}





nsresult
nsPlacesExportService::WriteDescription(int64_t aItemId, int32_t aType,
                                              nsIOutputStream* aOutput)
{
  bool hasDescription = false;
  nsresult rv = mAnnotationService->ItemHasAnnotation(aItemId,
                                                      DESCRIPTION_ANNO,
                                                      &hasDescription);
  if (NS_FAILED(rv) || !hasDescription)
    return rv;

  nsAutoString description;
  rv = mAnnotationService->GetItemAnnotationString(aItemId, DESCRIPTION_ANNO,
                                                   description);
  NS_ENSURE_SUCCESS(rv, rv);

  char* escapedDesc = nsEscapeHTML(NS_ConvertUTF16toUTF8(description).get());
  if (escapedDesc) {
    uint32_t dummy;
    rv = aOutput->Write(kDescriptionIntro, sizeof(kDescriptionIntro)-1, &dummy);
    if (NS_FAILED(rv)) {
      nsMemory::Free(escapedDesc);
      return rv;
    }
    rv = aOutput->Write(escapedDesc, strlen(escapedDesc), &dummy);
    nsMemory::Free(escapedDesc);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aOutput->Write(kDescriptionClose, sizeof(kDescriptionClose)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}




nsresult
nsPlacesExportService::WriteItem(nsINavHistoryResultNode* aItem,
                                       const nsACString& aIndent,
                                       nsIOutputStream* aOutput)
{
  
  
  
  nsAutoCString uri;
  nsresult rv = aItem->GetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> pageURI;
  rv = NS_NewURI(getter_AddRefs(pageURI), uri, nullptr);
  if (NS_FAILED(rv)) {
    nsAutoCString warnMsg;
    warnMsg.Append("Bookmarks Export: Found invalid item uri '");
    warnMsg.Append(uri);
    warnMsg.Append("'");
    NS_WARNING(warnMsg.get());
    return NS_OK;
  }

  
  uint32_t dummy;
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aOutput->Write(kItemOpen, sizeof(kItemOpen)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = aOutput->Write(kHrefAttribute, sizeof(kHrefAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteEscapedUrl(uri, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRTime dateAdded = 0;
  rv = aItem->GetDateAdded(&dateAdded);
  NS_ENSURE_SUCCESS(rv, rv);

  if (dateAdded) {
    rv = WriteDateAttribute(kDateAddedAttribute, sizeof(kDateAddedAttribute)-1, dateAdded, aOutput);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRTime lastModified = 0;
  rv = aItem->GetLastModified(&lastModified);
  NS_ENSURE_SUCCESS(rv, rv);

  if (lastModified) {
    rv = WriteDateAttribute(kLastModifiedAttribute, sizeof(kLastModifiedAttribute)-1, lastModified, aOutput);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = WriteFaviconAttribute(uri, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int64_t itemId;
  rv = aItem->GetItemId(&itemId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoString keyword;
  rv = mBookmarksService->GetKeywordForBookmark(itemId, keyword);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!keyword.IsEmpty()) {
    rv = aOutput->Write(kKeywordAttribute, sizeof(kKeywordAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
    char* escapedKeyword = nsEscapeHTML(NS_ConvertUTF16toUTF8(keyword).get());
    rv = aOutput->Write(escapedKeyword, strlen(escapedKeyword), &dummy);
    nsMemory::Free(escapedKeyword);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  bool hasPostData;
  rv = mAnnotationService->ItemHasAnnotation(itemId, POST_DATA_ANNO,
                                             &hasPostData);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasPostData) {
    nsAutoString postData;
    rv = mAnnotationService->GetItemAnnotationString(itemId, POST_DATA_ANNO,
                                                     postData);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aOutput->Write(kPostDataAttribute, sizeof(kPostDataAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
    char* escapedPostData = nsEscapeHTML(NS_ConvertUTF16toUTF8(postData).get());
    rv = aOutput->Write(escapedPostData, strlen(escapedPostData), &dummy);
    nsMemory::Free(escapedPostData);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  bool loadInSidebar = false;
  rv = mAnnotationService->ItemHasAnnotation(itemId, LOAD_IN_SIDEBAR_ANNO,
                                             &loadInSidebar);
  NS_ENSURE_SUCCESS(rv, rv);
  if (loadInSidebar)
    aOutput->Write(kWebPanelAttribute, sizeof(kWebPanelAttribute)-1, &dummy);

  
  nsAutoString lastCharset;
  if (NS_SUCCEEDED(mHistoryService->GetCharsetForURI(pageURI, lastCharset)) &&
      !lastCharset.IsEmpty()) {
    rv = aOutput->Write(kLastCharsetAttribute, sizeof(kLastCharsetAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
    char* escapedLastCharset = nsEscapeHTML(NS_ConvertUTF16toUTF8(lastCharset).get());
    rv = aOutput->Write(escapedLastCharset, strlen(escapedLastCharset), &dummy);
    nsMemory::Free(escapedLastCharset);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoCString title;
  rv = aItem->GetTitle(title);
  NS_ENSURE_SUCCESS(rv, rv);
  char* escapedTitle = nsEscapeHTML(title.get());
  if (escapedTitle) {
    rv = aOutput->Write(escapedTitle, strlen(escapedTitle), &dummy);
    nsMemory::Free(escapedTitle);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aOutput->Write(kItemClose, sizeof(kItemClose)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteDescription(itemId, nsINavBookmarksService::TYPE_BOOKMARK, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}






nsresult
nsPlacesExportService::WriteLivemark(nsINavHistoryResultNode* aFolder, const nsACString& aIndent,
                              nsIOutputStream* aOutput)
{
  uint32_t dummy;
  nsresult rv;

  
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aOutput->Write(kItemOpen, sizeof(kItemOpen)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int64_t folderId;
  rv = aFolder->GetItemId(&folderId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsString feedSpec;
  rv = mAnnotationService->GetItemAnnotationString(folderId,
                                                   NS_LITERAL_CSTRING(LMANNO_FEEDURI),
                                                   feedSpec);
  
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kFeedURIAttribute, sizeof(kFeedURIAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteEscapedUrl(NS_ConvertUTF16toUTF8(feedSpec), aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsString siteSpec;
  rv = mAnnotationService->GetItemAnnotationString(folderId,
                                                   NS_LITERAL_CSTRING(LMANNO_SITEURI),
                                                   siteSpec);
  if (NS_SUCCEEDED(rv) && !siteSpec.IsEmpty()) {
    
    rv = aOutput->Write(kHrefAttribute, sizeof(kHrefAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = WriteEscapedUrl(NS_ConvertUTF16toUTF8(siteSpec), aOutput);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteTitle(aFolder, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kItemClose, sizeof(kItemClose)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteDescription(folderId, nsINavBookmarksService::TYPE_BOOKMARK, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





nsresult
nsPlacesExportService::WriteSeparator(nsINavHistoryResultNode* aItem,
                                            const nsACString& aIndent,
                                            nsIOutputStream* aOutput)
{
  uint32_t dummy;
  nsresult rv;

  
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(),
                        &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aOutput->Write(kSeparator, sizeof(kSeparator)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  int64_t itemId;
  rv = aItem->GetItemId(&itemId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  nsAutoCString title;
  rv = mBookmarksService->GetItemTitle(itemId, title);
  if (NS_SUCCEEDED(rv) && !title.IsEmpty()) {
    rv = aOutput->Write(kNameAttribute, strlen(kNameAttribute), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);

    char* escapedTitle = nsEscapeHTML(title.get());
    if (escapedTitle) {
      uint32_t dummy;
      rv = aOutput->Write(escapedTitle, strlen(escapedTitle), &dummy);
      nsMemory::Free(escapedTitle);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(NS_LINEBREAK, sizeof(NS_LINEBREAK)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}









nsresult
WriteEscapedUrl(const nsCString& aString, nsIOutputStream* aOutput)
{
  nsAutoCString escaped(aString);
  int32_t offset;
  while ((offset = escaped.FindChar('\"')) >= 0) {
    escaped.Cut(offset, 1);
    escaped.Insert(NS_LITERAL_CSTRING("%22"), offset);
  }
  uint32_t dummy;
  return aOutput->Write(escaped.get(), escaped.Length(), &dummy);
}






nsresult
nsPlacesExportService::WriteContainerContents(nsINavHistoryResultNode* aFolder,
                                                    const nsACString& aIndent,
                                                    nsIOutputStream* aOutput)
{
  nsAutoCString myIndent(aIndent);
  myIndent.Append(kIndent);

  int64_t folderId;
  nsresult rv = aFolder->GetItemId(&folderId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsINavHistoryContainerResultNode> folderNode = do_QueryInterface(aFolder, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = folderNode->SetContainerOpen(true);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t childCount = 0;
  folderNode->GetChildCount(&childCount);
  for (uint32_t i = 0; i < childCount; ++i) {
    nsCOMPtr<nsINavHistoryResultNode> child;
    rv = folderNode->GetChild(i, getter_AddRefs(child));
    NS_ENSURE_SUCCESS(rv, rv);
    uint32_t type = 0;
    rv = child->GetType(&type);
    NS_ENSURE_SUCCESS(rv, rv);
    if (type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER) {
      
      int64_t childFolderId;
      rv = child->GetItemId(&childFolderId);
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      bool isLivemark;
      nsresult rv = mAnnotationService->ItemHasAnnotation(childFolderId,
                                                          NS_LITERAL_CSTRING(LMANNO_FEEDURI),
                                                          &isLivemark);
      NS_ENSURE_SUCCESS(rv, rv);

      if (isLivemark)
        rv = WriteLivemark(child, myIndent, aOutput);
      else
        rv = WriteContainer(child, myIndent, aOutput);
    }
    else if (type == nsINavHistoryResultNode::RESULT_TYPE_SEPARATOR) {
      rv = WriteSeparator(child, myIndent, aOutput);
    }
    else {
      rv = WriteItem(child, myIndent, aOutput);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}



NS_IMETHODIMP
nsPlacesExportService::ExportHTMLToFile(nsIFile* aBookmarksFile)
{
  NS_ENSURE_ARG(aBookmarksFile);

#ifdef DEBUG_EXPORT
  nsAutoString path;
  aBookmarksFile->GetPath(path);
  printf("\nExporting %s\n", NS_ConvertUTF16toUTF8(path).get());

  PRTime startTime = PR_Now();
  printf("\nStart time: %lld\n", startTime);
#endif

  nsresult rv = EnsureServiceState();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIOutputStream> out;
  rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(out),
                                       aBookmarksFile,
                                       PR_WRONLY | PR_CREATE_FILE,
                                       0600, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIOutputStream> strm;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(strm), out, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsINavHistoryQuery> query;
  rv = mHistoryService->GetNewQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsINavHistoryQueryOptions> options;
  rv = mHistoryService->GetNewQueryOptions(getter_AddRefs(options));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsINavHistoryResult> result;

  
  int64_t bookmarksMenuFolder;
  rv = mBookmarksService->GetBookmarksMenuFolder(&bookmarksMenuFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = query->SetFolders(&bookmarksMenuFolder, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mHistoryService->ExecuteQuery(query, options, getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsINavHistoryContainerResultNode> rootNode;
  rv = result->GetRoot(getter_AddRefs(rootNode));
  NS_ENSURE_SUCCESS(rv, rv);

  
  uint32_t dummy;
  rv = strm->Write(kFileIntro, sizeof(kFileIntro)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = strm->Write(kRootIntro, sizeof(kRootIntro)-1, &dummy); 
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = strm->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy); 
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteTitle(rootNode, strm);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = strm->Write(kCloseRootH1, sizeof(kCloseRootH1)-1, &dummy); 
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteContainerPrologue(EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoCString indent;
  indent.Assign(kIndent);

  
  rv = WriteContainerContents(rootNode, EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  int64_t toolbarFolder;
  rv = mBookmarksService->GetToolbarFolder(&toolbarFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = query->SetFolders(&toolbarFolder, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mHistoryService->ExecuteQuery(query, options, getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = result->GetRoot(getter_AddRefs(rootNode));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = rootNode->SetContainerOpen(true);
  NS_ENSURE_SUCCESS(rv, rv);
  uint32_t childCount = 0;
  rv = rootNode->GetChildCount(&childCount);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = rootNode->SetContainerOpen(false);
  NS_ENSURE_SUCCESS(rv, rv);
  if (childCount) {
    rv = WriteContainer(rootNode, nsDependentCString(kIndent), strm);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  int64_t unfiledBookmarksFolder;
  rv = mBookmarksService->GetUnfiledBookmarksFolder(&unfiledBookmarksFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = query->SetFolders(&unfiledBookmarksFolder, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mHistoryService->ExecuteQuery(query, options, getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = result->GetRoot(getter_AddRefs(rootNode));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = rootNode->SetContainerOpen(true);
  NS_ENSURE_SUCCESS(rv, rv);
  childCount = 0;
  rootNode->GetChildCount(&childCount);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = rootNode->SetContainerOpen(false);
  NS_ENSURE_SUCCESS(rv, rv);
  if (childCount) {
    rv = WriteContainer(rootNode, nsDependentCString(kIndent), strm);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = WriteContainerEpilogue(EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(strm, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = safeStream->Finish();
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG_EXPORT
  printf("\nTotal time in seconds: %lld\n", (PR_Now() - startTime)/1000000);
#endif

  return NS_OK;
}


NS_IMETHODIMP
nsPlacesExportService::BackupBookmarksFile()
{
  nsresult rv = EnsureServiceState();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFile> bookmarksFileDir;
  rv = NS_GetSpecialDirectory(NS_APP_BOOKMARKS_50_FILE,
                              getter_AddRefs(bookmarksFileDir));

  NS_ENSURE_SUCCESS(rv, rv);

  
  bool exists;
  rv = bookmarksFileDir->Exists(&exists);
  if (NS_FAILED(rv) || !exists) {
    rv = bookmarksFileDir->Create(nsIFile::NORMAL_FILE_TYPE, 0600);
    if (NS_FAILED(rv)) {
      NS_WARNING("Unable to create bookmarks.html!");
      return rv;
    }
  }

  
  rv = ExportHTMLToFile(bookmarksFileDir);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
