





























































































#include "nsPlacesImportExportService.h"
#include "nsNetUtil.h"
#include "nsParserCIID.h"
#include "nsUnicharUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIPrefService.h"
#include "nsToolkitCompsCID.h"
#include "nsIHTMLContentSink.h"
#include "nsIParser.h"
#include "prprf.h"
#include "nsIObserverService.h"
#include "nsISupportsPrimitives.h"
#include "nsPlacesMacros.h"
#include "mozilla/Util.h"

using namespace mozilla;

static NS_DEFINE_CID(kParserCID, NS_PARSER_CID);

#define KEY_TOOLBARFOLDER_LOWER "personal_toolbar_folder"
#define KEY_BOOKMARKSMENU_LOWER "bookmarks_menu"
#define KEY_UNFILEDFOLDER_LOWER "unfiled_bookmarks_folder"
#define KEY_PLACESROOT_LOWER "places_root"
#define KEY_HREF_LOWER "href"
#define KEY_FEEDURL_LOWER "feedurl"
#define KEY_WEB_PANEL_LOWER "web_panel"
#define KEY_LASTCHARSET_LOWER "last_charset"
#define KEY_ICON_LOWER "icon"
#define KEY_ICON_URI_LOWER "icon_uri"
#define KEY_SHORTCUTURL_LOWER "shortcuturl"
#define KEY_POST_DATA_LOWER "post_data"
#define KEY_NAME_LOWER "name"
#define KEY_MICSUM_GEN_URI_LOWER "micsum_gen_uri"
#define KEY_DATE_ADDED_LOWER "add_date"
#define KEY_LAST_MODIFIED_LOWER "last_modified"
#define KEY_GENERATED_TITLE_LOWER "generated_title"

#define LOAD_IN_SIDEBAR_ANNO NS_LITERAL_CSTRING("bookmarkProperties/loadInSidebar")
#define DESCRIPTION_ANNO NS_LITERAL_CSTRING("bookmarkProperties/description")
#define POST_DATA_ANNO NS_LITERAL_CSTRING("bookmarkProperties/POSTData")

#define BOOKMARKS_MENU_ICON_URI "chrome://browser/skin/places/bookmarksMenu.png"



#define RESTORE_BEGIN_NSIOBSERVER_TOPIC "bookmarks-restore-begin"
#define RESTORE_SUCCESS_NSIOBSERVER_TOPIC "bookmarks-restore-success"
#define RESTORE_FAILED_NSIOBSERVER_TOPIC "bookmarks-restore-failed"
#define RESTORE_NSIOBSERVER_DATA NS_LITERAL_STRING("html")
#define RESTORE_INITIAL_NSIOBSERVER_DATA NS_LITERAL_STRING("html-initial")


#define BROWSER_BOOKMARKS_MAX_BACKUPS_PREF  "browser.bookmarks.max_backups"





#if defined(XP_WIN) || defined(XP_OS2)
#define NS_LINEBREAK "\015\012"
#else
#define NS_LINEBREAK "\012"
#endif

class nsIOutputStream;
static const char kWhitespace[] = " \r\n\t\b";
static nsresult WriteEscapedUrl(const nsCString &aString, nsIOutputStream* aOutput);

class BookmarkImportFrame
{
public:
  BookmarkImportFrame(PRInt64 aID) :
      mContainerID(aID),
      mContainerNesting(0),
      mLastContainerType(Container_Normal),
      mInDescription(PR_FALSE),
      mPreviousId(0),
      mPreviousDateAdded(0),
      mPreviousLastModifiedDate(0)
  {
  }

  enum ContainerType { Container_Normal,
                       Container_Places,
                       Container_Menu,
                       Container_Toolbar,
                       Container_Unfiled};

  PRInt64 mContainerID;

  
  
  
  
  
  
  
  PRInt32 mContainerNesting;

  
  
  
  ContainerType mLastContainerType;

  
  
  
  nsString mPreviousText;

  
  
  
  
  
  
  
  
  
  
  
  
  
  PRBool mInDescription;

  
  
  
  
  nsCOMPtr<nsIURI> mPreviousLink;

  
  
  nsCOMPtr<nsIURI> mPreviousFeed;

  void ConsumeHeading(nsAString* aHeading, ContainerType* aContainerType)
  {
    *aHeading = mPreviousText;
    *aContainerType = mLastContainerType;
    mPreviousText.Truncate();
  }

  
  PRInt64 mPreviousId;

  
  
  PRTime mPreviousDateAdded;
  PRTime mPreviousLastModifiedDate;
};




char*
nsEscapeHTML(const char* string)
{
  
  char* escaped = nsnull;
  PRUint32 len = strlen(string);
  if (len >= (PR_UINT32_MAX / 6))
    return nsnull;

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
      *ptr = '\0';
    }
  }
  return escaped;
}

PLACES_FACTORY_SINGLETON_IMPLEMENTATION(nsPlacesImportExportService, gImportExportService)

NS_IMPL_ISUPPORTS2(nsPlacesImportExportService, nsIPlacesImportExportService,
                   nsINavHistoryBatchCallback)


nsPlacesImportExportService::nsPlacesImportExportService()
{
  NS_ASSERTION(!gImportExportService,
               "Attempting to create two instances of the service!");
  gImportExportService = this;
}

nsPlacesImportExportService::~nsPlacesImportExportService()
{
  NS_ASSERTION(gImportExportService == this,
               "Deleting a non-singleton instance of the service");
  if (gImportExportService == this)
    gImportExportService = nsnull;
}

nsresult
nsPlacesImportExportService::Init()
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




class BookmarkContentSink : public nsIHTMLContentSink
{
public:
  BookmarkContentSink();

  nsresult Init(PRBool aAllowRootChanges,
                PRInt64 aFolder,
                PRBool aIsImportDefaults);

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD WillParse() { return NS_OK; }
  NS_IMETHOD WillInterrupt() { return NS_OK; }
  NS_IMETHOD WillResume() { return NS_OK; }
  NS_IMETHOD SetParser(nsIParser* aParser) { return NS_OK; }
  virtual void FlushPendingNotifications(mozFlushType aType) { }
  NS_IMETHOD SetDocumentCharset(nsACString& aCharset) { return NS_OK; }
  virtual nsISupports *GetTarget() { return nsnull; }

  
  NS_IMETHOD OpenHead() { return NS_OK; }
  NS_IMETHOD BeginContext(PRInt32 aPosition) { return NS_OK; }
  NS_IMETHOD EndContext(PRInt32 aPosition) { return NS_OK; }
  NS_IMETHOD IsEnabled(PRInt32 aTag, PRBool* aReturn)
    { *aReturn = PR_TRUE; return NS_OK; }
  NS_IMETHOD DidProcessTokens() { return NS_OK; }
  NS_IMETHOD WillProcessAToken() { return NS_OK; }
  NS_IMETHOD DidProcessAToken() { return NS_OK; }
  NS_IMETHOD OpenContainer(const nsIParserNode& aNode);
  NS_IMETHOD CloseContainer(const nsHTMLTag aTag);
  NS_IMETHOD AddLeaf(const nsIParserNode& aNode);
  NS_IMETHOD AddComment(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD AddProcessingInstruction(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD AddDocTypeDecl(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD NotifyTagObservers(nsIParserNode* aNode) { return NS_OK; }
  NS_IMETHOD_(PRBool) IsFormOnStack() { return PR_FALSE; }

protected:
  nsCOMPtr<nsINavBookmarksService> mBookmarksService;
  nsCOMPtr<nsINavHistoryService> mHistoryService;
  nsCOMPtr<nsIAnnotationService> mAnnotationService;
  nsCOMPtr<nsILivemarkService> mLivemarkService;

  
  
  
  
  
  
  PRBool mAllowRootChanges;

  
  
  
  
  
  PRBool mIsImportDefaults;

  
  
  
  PRBool mFolderSpecified;

  void HandleContainerBegin(const nsIParserNode& node);
  void HandleContainerEnd();
  void HandleHead1Begin(const nsIParserNode& node);
  void HandleHeadBegin(const nsIParserNode& node);
  void HandleHeadEnd();
  void HandleLinkBegin(const nsIParserNode& node);
  void HandleLinkEnd();
  void HandleSeparator(const nsIParserNode& node);

  
  
  
  
  nsTArray<BookmarkImportFrame> mFrames;
  BookmarkImportFrame& CurFrame()
  {
    NS_ASSERTION(mFrames.Length() > 0, "Asking for frame when there are none!");
    return mFrames[mFrames.Length() - 1];
  }
  BookmarkImportFrame& PreviousFrame()
  {
    NS_ASSERTION(mFrames.Length() > 1, "Asking for frame when there are not enough!");
    return mFrames[mFrames.Length() - 2];
  }
  nsresult NewFrame();
  nsresult PopFrame();

  nsresult SetFaviconForURI(nsIURI* aPageURI, nsIURI* aFaviconURI,
                            const nsString& aData);

  PRTime ConvertImportedDateToInternalDate(const nsACString& aDate);

#ifdef DEBUG_IMPORT
  
  void PrintNesting()
  {
    for (PRUint32 i = 0; i < mFrames.Length(); i ++)
      printf("  ");
  }
#endif
};


BookmarkContentSink::BookmarkContentSink() : mFrames(16)
{
}


nsresult
BookmarkContentSink::Init(PRBool aAllowRootChanges,
                          PRInt64 aFolder,
                          PRBool aIsImportDefaults)
{
  mBookmarksService = do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(mBookmarksService, NS_ERROR_OUT_OF_MEMORY);
  mHistoryService = do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(mHistoryService, NS_ERROR_OUT_OF_MEMORY);
  mAnnotationService = do_GetService(NS_ANNOTATIONSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(mAnnotationService, NS_ERROR_OUT_OF_MEMORY);
  mLivemarkService = do_GetService(NS_LIVEMARKSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(mLivemarkService, NS_ERROR_OUT_OF_MEMORY);

  mAllowRootChanges = aAllowRootChanges;
  mIsImportDefaults = aIsImportDefaults;

  
  PRInt64 menuRoot;
  nsresult rv;
  if (aFolder == 0) {
    rv = mBookmarksService->GetBookmarksMenuFolder(&menuRoot);
    NS_ENSURE_SUCCESS(rv, rv);
    mFolderSpecified = false;
  }
  else {
    menuRoot = aFolder;
    mFolderSpecified = true;
  }
  if (!mFrames.AppendElement(BookmarkImportFrame(menuRoot)))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


NS_IMPL_ISUPPORTS2(BookmarkContentSink,
                   nsIContentSink,
                   nsIHTMLContentSink)


NS_IMETHODIMP
BookmarkContentSink::OpenContainer(const nsIParserNode& aNode)
{
  switch(aNode.GetNodeType()) {
    case eHTMLTag_h1:
      HandleHead1Begin(aNode);
      break;
    case eHTMLTag_h2:
    case eHTMLTag_h3:
    case eHTMLTag_h4:
    case eHTMLTag_h5:
    case eHTMLTag_h6:
      HandleHeadBegin(aNode);
      break;
    case eHTMLTag_a:
      HandleLinkBegin(aNode);
      break;
    case eHTMLTag_dl:
    case eHTMLTag_ul:
    case eHTMLTag_menu:
      HandleContainerBegin(aNode);
      break;
    case eHTMLTag_dd:
      CurFrame().mInDescription = PR_TRUE;
      break;
  }
  return NS_OK;
}


NS_IMETHODIMP
BookmarkContentSink::CloseContainer(const nsHTMLTag aTag)
{
  BookmarkImportFrame& frame = CurFrame();

  
  
  
  if (frame.mInDescription) {
    frame.mPreviousText.Trim(kWhitespace); 
    if (!frame.mPreviousText.IsEmpty()) {

      PRInt64 itemId = !frame.mPreviousLink ? frame.mContainerID
                                            : frame.mPreviousId;
                    
      PRBool hasDescription = PR_FALSE;
      nsresult rv = mAnnotationService->ItemHasAnnotation(itemId,
                                                          DESCRIPTION_ANNO,
                                                          &hasDescription);
      if (NS_SUCCEEDED(rv) && !hasDescription) {
        mAnnotationService->SetItemAnnotationString(itemId, DESCRIPTION_ANNO,
                                                    frame.mPreviousText, 0,
                                                    nsIAnnotationService::EXPIRE_NEVER);
      }
      frame.mPreviousText.Truncate();

      
      
      
      
      
      
      
      
      
      

      PRTime lastModified;
      if (!frame.mPreviousLink) {
        lastModified = PreviousFrame().mPreviousLastModifiedDate;
      } else {
        lastModified = frame.mPreviousLastModifiedDate;
      }

      if (itemId > 0 && lastModified > 0) {
        rv = mBookmarksService->SetItemLastModified(itemId, lastModified);
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetItemLastModified failed");
      }
    }
    frame.mInDescription = PR_FALSE;
  }

  switch (aTag) {
    case eHTMLTag_dl:
    case eHTMLTag_ul:
    case eHTMLTag_menu:
      HandleContainerEnd();
      break;
    case eHTMLTag_dt:
      break;
    case eHTMLTag_h1:
      
      break;
    case eHTMLTag_h2:
    case eHTMLTag_h3:
    case eHTMLTag_h4:
    case eHTMLTag_h5:
    case eHTMLTag_h6:
      HandleHeadEnd();
      break;
    case eHTMLTag_a:
      HandleLinkEnd();
      break;
    default:
      break;
  }
  return NS_OK;
}














NS_IMETHODIMP
BookmarkContentSink::AddLeaf(const nsIParserNode& aNode)
{
  switch (aNode.GetNodeType()) {
  case eHTMLTag_text:
    
    CurFrame().mPreviousText += aNode.GetText();
    break;
  case eHTMLTag_entity: {
    nsAutoString tmp;
    PRInt32 unicode = aNode.TranslateToUnicodeStr(tmp);
    if (unicode < 0) {
      
      CurFrame().mPreviousText += aNode.GetText();
    } else {
      CurFrame().mPreviousText.Append(unicode);
    }
    break;
  }
  case eHTMLTag_whitespace:
    CurFrame().mPreviousText.Append(PRUnichar(' '));
    break;
  case eHTMLTag_hr:
    HandleSeparator(aNode);
    break;
  }

  return NS_OK;
}


void
BookmarkContentSink::HandleContainerBegin(const nsIParserNode& node)
{
  CurFrame().mContainerNesting ++;
}








void
BookmarkContentSink::HandleContainerEnd()
{
  BookmarkImportFrame& frame = CurFrame();
  if (frame.mContainerNesting > 0)
    frame.mContainerNesting --;
  if (mFrames.Length() > 1 && frame.mContainerNesting == 0) {
    
    
    BookmarkImportFrame& prevFrame = PreviousFrame();
    if (prevFrame.mPreviousLastModifiedDate > 0) {
      (void)mBookmarksService->SetItemLastModified(frame.mContainerID,
                                                   prevFrame.mPreviousLastModifiedDate);
    }
    PopFrame();
  }
}








void
BookmarkContentSink::HandleHead1Begin(const nsIParserNode& node)
{
  PRInt32 attrCount = node.GetAttributeCount();
  for (PRInt32 i = 0; i < attrCount; i ++) {
    if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_PLACESROOT_LOWER)) {
      if (mFrames.Length() > 1) {
        NS_WARNING("Trying to set the places root from the middle of the hierarchy. "
                   "This can only be set at the beginning.");
        return;
      }

      PRInt64 placesRoot;
      DebugOnly<nsresult> rv = mBookmarksService->GetPlacesRoot(&placesRoot);
      NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "could not get placesRoot");
      CurFrame().mContainerID = placesRoot;
      break;
    }
  }
}










void
BookmarkContentSink::HandleHeadBegin(const nsIParserNode& node)
{
  BookmarkImportFrame& frame = CurFrame();

  
  
  frame.mPreviousLink = nsnull;
  frame.mLastContainerType = BookmarkImportFrame::Container_Normal;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (frame.mContainerNesting == 0)
    PopFrame();

  
  
  
  PRInt32 attrCount = node.GetAttributeCount();
  frame.mLastContainerType = BookmarkImportFrame::Container_Normal;
  if (!mFolderSpecified) {
    for (PRInt32 i = 0; i < attrCount; i ++) {
      if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_TOOLBARFOLDER_LOWER)) {
        if (mIsImportDefaults)
          frame.mLastContainerType = BookmarkImportFrame::Container_Toolbar;
        break;
      }
      else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_BOOKMARKSMENU_LOWER)) {
        if (mIsImportDefaults)
          frame.mLastContainerType = BookmarkImportFrame::Container_Menu;
        break;
      }
      else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_UNFILEDFOLDER_LOWER)) {
        if (mIsImportDefaults)
          frame.mLastContainerType = BookmarkImportFrame::Container_Unfiled;
        break;
      }
      else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_PLACESROOT_LOWER)) {
        if (mIsImportDefaults)
          frame.mLastContainerType = BookmarkImportFrame::Container_Places;
        break;
      }
      else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_DATE_ADDED_LOWER)) {
        frame.mPreviousDateAdded =
          ConvertImportedDateToInternalDate(NS_ConvertUTF16toUTF8(node.GetValueAt(i)));
      }
      else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_LAST_MODIFIED_LOWER)) {
        frame.mPreviousLastModifiedDate =
          ConvertImportedDateToInternalDate(NS_ConvertUTF16toUTF8(node.GetValueAt(i)));
      }
    }
  }
  CurFrame().mPreviousText.Truncate();
}








void
BookmarkContentSink::HandleHeadEnd()
{
  NewFrame();
}








void
BookmarkContentSink::HandleLinkBegin(const nsIParserNode& node)
{
  BookmarkImportFrame& frame = CurFrame();

  
  frame.mPreviousFeed = nsnull;
  
  frame.mPreviousId = 0;
  
  frame.mPreviousText.Truncate();

  
  nsAutoString href;
  nsAutoString feedUrl;
  nsAutoString icon;
  nsAutoString iconUri;
  nsAutoString lastCharset;
  nsAutoString keyword;
  nsAutoString postData;
  nsAutoString webPanel;
  nsAutoString micsumGenURI;
  nsAutoString generatedTitle;
  nsAutoString dateAdded;
  nsAutoString lastModified;

  PRInt32 attrCount = node.GetAttributeCount();
  for (PRInt32 i = 0; i < attrCount; i++) {
    const nsAString& key = node.GetKeyAt(i);
    nsAutoString value(node.GetValueAt(i));
    value.Trim(kWhitespace);

    if (key.LowerCaseEqualsLiteral(KEY_HREF_LOWER))
      href = value;
    else if (key.LowerCaseEqualsLiteral(KEY_FEEDURL_LOWER))
      feedUrl = value;
    else if (key.LowerCaseEqualsLiteral(KEY_ICON_LOWER))
      icon = value;
    else if (key.LowerCaseEqualsLiteral(KEY_ICON_URI_LOWER))
      iconUri = value;
    else if (key.LowerCaseEqualsLiteral(KEY_LASTCHARSET_LOWER))
      lastCharset = value;
    else if (key.LowerCaseEqualsLiteral(KEY_SHORTCUTURL_LOWER))
      keyword = value;
    else if (key.LowerCaseEqualsLiteral(KEY_POST_DATA_LOWER))
      postData = value;
    else if (key.LowerCaseEqualsLiteral(KEY_WEB_PANEL_LOWER))
      webPanel = value;
    else if (key.LowerCaseEqualsLiteral(KEY_MICSUM_GEN_URI_LOWER))
      micsumGenURI = value;
    else if (key.LowerCaseEqualsLiteral(KEY_GENERATED_TITLE_LOWER))
      generatedTitle = value;
    else if (key.LowerCaseEqualsLiteral(KEY_DATE_ADDED_LOWER))
      dateAdded = value;
    else if (key.LowerCaseEqualsLiteral(KEY_LAST_MODIFIED_LOWER))
      lastModified = value;
  }

  
  
  if (!feedUrl.IsEmpty()) {
    NS_NewURI(getter_AddRefs(frame.mPreviousFeed),
              NS_ConvertUTF16toUTF8(feedUrl), nsnull);
  }

  
  if (href.IsEmpty()) {
    frame.mPreviousLink = nsnull;
    
    
    if (!frame.mPreviousFeed)
      return;
  }
  else {
    
    
    nsresult rv = NS_NewURI(getter_AddRefs(frame.mPreviousLink), href, nsnull);
    if (NS_FAILED(rv) && !frame.mPreviousFeed) {
      frame.mPreviousLink = nsnull;
      return;
    }
  }

  
  if (!lastModified.IsEmpty()) {
    frame.mPreviousLastModifiedDate =
      ConvertImportedDateToInternalDate(NS_ConvertUTF16toUTF8(lastModified));
  }

  
  
  if (frame.mPreviousFeed)
    return;

  
  nsresult rv = mBookmarksService->InsertBookmark(frame.mContainerID,
                                                  frame.mPreviousLink,
                                                  mBookmarksService->DEFAULT_INDEX,
                                                  EmptyCString(),
                                                  &frame.mPreviousId);
  if (NS_FAILED(rv)) {
    
    NS_WARNING("InserBookmark failed");
    return;
  }
  
  
  if (!dateAdded.IsEmpty()) {
    PRTime convertedDateAdded =
      ConvertImportedDateToInternalDate(NS_ConvertUTF16toUTF8(dateAdded));
    if (convertedDateAdded) {
      rv = mBookmarksService->SetItemDateAdded(frame.mPreviousId, convertedDateAdded);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetItemDateAdded failed");
    }
  }

  
  if (!icon.IsEmpty() || !iconUri.IsEmpty()) {
    nsCOMPtr<nsIURI> iconUriObject;
    rv = NS_NewURI(getter_AddRefs(iconUriObject), iconUri);
    if (!icon.IsEmpty() || NS_SUCCEEDED(rv)) {
      rv = SetFaviconForURI(frame.mPreviousLink, iconUriObject, icon);
      if (NS_FAILED(rv)) {
        nsCAutoString warnMsg;
        warnMsg.Append("Bookmarks Import: unable to set favicon '");
        warnMsg.Append(NS_ConvertUTF16toUTF8(iconUri));
        warnMsg.Append("' for page '");
        nsCAutoString spec;
        rv = frame.mPreviousLink->GetSpec(spec);
        if (NS_SUCCEEDED(rv))
          warnMsg.Append(spec);
        warnMsg.Append("'");
        NS_WARNING(warnMsg.get());
      }
    }
  }

  
  if (!keyword.IsEmpty()) {
    rv = mBookmarksService->SetKeywordForBookmark(frame.mPreviousId, keyword);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetKeywordForBookmark failed");
    if (NS_SUCCEEDED(rv) && !postData.IsEmpty()) {
      rv = mAnnotationService->SetItemAnnotationString(frame.mPreviousId,
                                                       POST_DATA_ANNO,
                                                       postData, 0,
                                                       nsIAnnotationService::EXPIRE_NEVER);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetItemAnnotationString failed");
    }
  }

  
  if (webPanel.LowerCaseEqualsLiteral("true")) {

    rv = mAnnotationService->SetItemAnnotationInt32(frame.mPreviousId,
                                                    LOAD_IN_SIDEBAR_ANNO,
                                                    1, 0,
                                                    nsIAnnotationService::EXPIRE_NEVER);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetItemAnnotationInt32 failed");
  }

  
  if (!lastCharset.IsEmpty()) {
    rv = mHistoryService->SetCharsetForURI(frame.mPreviousLink,lastCharset);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "setCharsetForURI failed");
  }
}








void
BookmarkContentSink::HandleLinkEnd()
{
  nsresult rv;
  BookmarkImportFrame& frame = CurFrame();
  frame.mPreviousText.Trim(kWhitespace);

  if (frame.mPreviousFeed) {
    
    

    if (mIsImportDefaults) {
      
      rv = mLivemarkService->CreateLivemarkFolderOnly(frame.mContainerID,
                                                      frame.mPreviousText,
                                                      frame.mPreviousLink,
                                                      frame.mPreviousFeed,
                                                      -1,
                                                      &frame.mPreviousId);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "CreateLivemarkFolderOnly failed!");
    }
    else {
      rv = mLivemarkService->CreateLivemark(frame.mContainerID,
                                            frame.mPreviousText,
                                            frame.mPreviousLink,
                                            frame.mPreviousFeed,
                                            -1,
                                            &frame.mPreviousId);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "CreateLivemark failed!");
    }
#ifdef DEBUG_IMPORT
    PrintNesting();
    printf("Created livemark '%s' %lld\n",
           NS_ConvertUTF16toUTF8(frame.mPreviousText).get(), frame.mPreviousId);
#endif
  }
  else if (frame.mPreviousLink) {
    
#ifdef DEBUG_IMPORT
    PrintNesting();
    printf("Created bookmark '%s' %lld\n",
           NS_ConvertUTF16toUTF8(frame.mPreviousText).get(), frame.mPreviousId);
#endif
    rv = mBookmarksService->SetItemTitle(frame.mPreviousId,
                                         NS_ConvertUTF16toUTF8(frame.mPreviousText));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetItemTitle failed");
  }

  
  if (frame.mPreviousId > 0 && frame.mPreviousLastModifiedDate > 0) {
    rv = mBookmarksService->SetItemLastModified(frame.mPreviousId,
                                                frame.mPreviousLastModifiedDate);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetItemLastModified failed");
    
    
  }

  frame.mPreviousText.Truncate();
}





void
BookmarkContentSink::HandleSeparator(const nsIParserNode& aNode)
{
  BookmarkImportFrame& frame = CurFrame();

  

#ifdef DEBUG_IMPORT
  PrintNesting();
  printf("--------\n");
#endif

  nsresult rv = mBookmarksService->InsertSeparator(frame.mContainerID,
                                                   mBookmarksService->DEFAULT_INDEX,
                                                   &frame.mPreviousId);
  if (NS_FAILED(rv)) {
    NS_WARNING("InsertSeparator failed");
    return;
  }
  
  
  PRInt32 attrCount = aNode.GetAttributeCount();
  for (PRInt32 i = 0; i < attrCount; i ++) {
    const nsAString& key = aNode.GetKeyAt(i);

    if (key.LowerCaseEqualsLiteral(KEY_NAME_LOWER)) {
      nsAutoString name;
      name = aNode.GetValueAt(i);
      name.Trim(kWhitespace);
      if (!name.IsEmpty()) {
        rv = mBookmarksService->SetItemTitle(frame.mPreviousId,
                                             NS_ConvertUTF16toUTF8(name));
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetItemTitle failed");
      }
    }
  }

  
  
  
  
}







nsresult
BookmarkContentSink::NewFrame()
{
  nsresult rv;

  PRInt64 ourID = 0;
  nsString containerName;
  BookmarkImportFrame::ContainerType containerType;
  BookmarkImportFrame& frame = CurFrame();
  frame.ConsumeHeading(&containerName, &containerType);

  PRBool updateFolder = PR_FALSE;
  
  switch (containerType) {
    case BookmarkImportFrame::Container_Normal:
      
      rv = mBookmarksService->CreateFolder(CurFrame().mContainerID,
                                           NS_ConvertUTF16toUTF8(containerName),
                                           mBookmarksService->DEFAULT_INDEX, 
                                           &ourID);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    case BookmarkImportFrame::Container_Places:
      
      
      rv = mBookmarksService->GetPlacesRoot(&ourID);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    case BookmarkImportFrame::Container_Menu:
      
      rv = mBookmarksService->GetBookmarksMenuFolder(&ourID);
      NS_ENSURE_SUCCESS(rv, rv);
      if (mAllowRootChanges)
        updateFolder = PR_TRUE;
      break;
    case BookmarkImportFrame::Container_Unfiled:
      
      rv = mBookmarksService->GetUnfiledBookmarksFolder(&ourID);
      NS_ENSURE_SUCCESS(rv, rv);
      if (mAllowRootChanges)
        updateFolder = PR_TRUE;
      break;
    case BookmarkImportFrame::Container_Toolbar:
      
      rv = mBookmarksService->GetToolbarFolder(&ourID);
      NS_ENSURE_SUCCESS(rv, rv);
      
      break;
    default:
      NS_NOTREACHED("Unknown container type");
  }

#ifdef DEBUG_IMPORT
  PrintNesting();
  printf("Folder %lld \'%s\'", ourID, NS_ConvertUTF16toUTF8(containerName).get());
#endif

  if (updateFolder) {
    
    rv = mBookmarksService->MoveItem(ourID, CurFrame().mContainerID, -1);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mBookmarksService->SetItemTitle(ourID, NS_ConvertUTF16toUTF8(containerName));
    NS_ENSURE_SUCCESS(rv, rv);
#ifdef DEBUG_IMPORT
    printf(" [reparenting]");
#endif
  }

#ifdef DEBUG_IMPORT
  printf("\n");
#endif

  if (frame.mPreviousDateAdded > 0) {
    rv = mBookmarksService->SetItemDateAdded(ourID, frame.mPreviousDateAdded);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetItemDateAdded failed");
    frame.mPreviousDateAdded = 0;
  }
  if (frame.mPreviousLastModifiedDate > 0) {
    rv = mBookmarksService->SetItemLastModified(ourID, frame.mPreviousLastModifiedDate);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SetItemLastModified failed");
    
  }

  frame.mPreviousId = ourID;

  if (!mFrames.AppendElement(BookmarkImportFrame(ourID)))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}


nsresult
BookmarkContentSink::PopFrame()
{
  
  if (mFrames.Length() <= 1) {
    NS_NOTREACHED("Trying to complete more bookmark folders than you started");
    return NS_ERROR_FAILURE;
  }
  mFrames.RemoveElementAt(mFrames.Length() - 1);
  return NS_OK;
}














nsresult
BookmarkContentSink::SetFaviconForURI(nsIURI* aPageURI, nsIURI* aIconURI,
                                      const nsString& aData)
{
  nsresult rv;
  static PRUint32 serialNumber = 0; 

  nsCOMPtr<nsIFaviconService> faviconService =
    do_GetService(NS_FAVICONSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(faviconService, NS_ERROR_OUT_OF_MEMORY);

  
  
  if (aIconURI) {
    nsCString faviconScheme;
    aIconURI->GetScheme(faviconScheme);
    if (faviconScheme.EqualsLiteral("chrome")) {
      return faviconService->SetFaviconUrlForPage(aPageURI, aIconURI);
    }
  }

  
  
  if (aData.Length() <= 5)
    return NS_OK;

  nsCOMPtr<nsIURI> faviconURI;
  if (aIconURI) {
    faviconURI = aIconURI;
  }
  else {
    
    nsCAutoString faviconSpec;
    faviconSpec.AssignLiteral("http://www.mozilla.org/2005/made-up-favicon/");
    faviconSpec.AppendInt(serialNumber);
    faviconSpec.AppendLiteral("-");
    char buf[32];
    PR_snprintf(buf, sizeof(buf), "%lld", PR_Now());
    faviconSpec.Append(buf);
    rv = NS_NewURI(getter_AddRefs(faviconURI), faviconSpec);
    if (NS_FAILED(rv)) {
      nsCAutoString warnMsg;
      warnMsg.Append("Bookmarks Import: Unable to make up new favicon '");
      warnMsg.Append(faviconSpec);
      warnMsg.Append("' for page '");
      nsCAutoString spec;
      rv = aPageURI->GetSpec(spec);
      if (NS_SUCCEEDED(rv))
        warnMsg.Append(spec);
      warnMsg.Append("'");
      NS_WARNING(warnMsg.get());
      return NS_OK;
    }
    serialNumber++;
  }

  
  
  
  (void) faviconService->SetFaviconDataFromDataURL(faviconURI, aData, 0);

  rv = faviconService->SetFaviconUrlForPage(aPageURI, faviconURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK; 
}



PRTime
BookmarkContentSink::ConvertImportedDateToInternalDate(const nsACString& aDate) {
  PRTime convertedDate = 0;
  if (!aDate.IsEmpty()) {
    nsresult rv;
    convertedDate = PromiseFlatCString(aDate).ToInteger(&rv);
    if (NS_SUCCEEDED(rv)) {
      convertedDate *= 1000000; 
    }
    else {
      convertedDate = 0;
    }
  }
  return convertedDate;
}







static nsresult
SyncChannelStatus(nsIChannel* channel, nsresult status)
{
  nsresult channelStatus;
  channel->GetStatus(&channelStatus);
  if (NS_FAILED(channelStatus))
    return channelStatus;

  if (NS_SUCCEEDED(status))
    return NS_OK; 

  
  channel->Cancel(status);
  return status;
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
  PRUint32 dummy;
  nsresult rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kBookmarkIntro, sizeof(kBookmarkIntro)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}







static nsresult
WriteContainerEpilogue(const nsACString& aIndent, nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kBookmarkClose, sizeof(kBookmarkClose)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}







static nsresult
WriteFaviconAttribute(const nsACString& aURI, nsIOutputStream* aOutput)
{
  PRUint32 dummy;

  
  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURI);
  if (NS_FAILED(rv)) {
    nsCAutoString warnMsg;
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

  nsCAutoString faviconScheme;
  nsCAutoString faviconSpec;
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
WriteDateAttribute(const char aAttributeStart[], PRInt32 aLength, PRTime aAttributeValue, nsIOutputStream* aOutput)
{
  
  PRUint32 dummy;
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
nsPlacesImportExportService::WriteContainer(nsINavHistoryResultNode* aFolder,
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
nsPlacesImportExportService::WriteContainerHeader(nsINavHistoryResultNode* aFolder,
                                                  const nsACString& aIndent,
                                                  nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aOutput->Write(kContainerIntro, sizeof(kContainerIntro)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 folderId;
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

  PRInt64 placesRoot;
  rv = mBookmarksService->GetPlacesRoot(&placesRoot);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 bookmarksMenuFolder;
  rv = mBookmarksService->GetBookmarksMenuFolder(&bookmarksMenuFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 toolbarFolder;
  rv = mBookmarksService->GetToolbarFolder(&toolbarFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 unfiledBookmarksFolder;
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
nsPlacesImportExportService::WriteTitle(nsINavHistoryResultNode* aItem,
                                        nsIOutputStream* aOutput)
{
  
  PRUint32 type = 0;
  nsresult rv = aItem->GetType(&type);
  NS_ENSURE_SUCCESS(rv, rv);
  if (type == nsINavHistoryResultNode::RESULT_TYPE_SEPARATOR)
    return NS_ERROR_INVALID_ARG;

  nsCAutoString title;
  rv = aItem->GetTitle(title);
  NS_ENSURE_SUCCESS(rv, rv);

  char* escapedTitle = nsEscapeHTML(title.get());
  if (escapedTitle) {
    PRUint32 dummy;
    rv = aOutput->Write(escapedTitle, strlen(escapedTitle), &dummy);
    nsMemory::Free(escapedTitle);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}





nsresult
nsPlacesImportExportService::WriteDescription(PRInt64 aItemId, PRInt32 aType,
                                              nsIOutputStream* aOutput)
{
  PRBool hasDescription = PR_FALSE;
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
    PRUint32 dummy;
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
nsPlacesImportExportService::WriteItem(nsINavHistoryResultNode* aItem,
                                       const nsACString& aIndent,
                                       nsIOutputStream* aOutput)
{
  
  
  
  nsCAutoString uri;
  nsresult rv = aItem->GetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> pageURI;
  rv = NS_NewURI(getter_AddRefs(pageURI), uri, nsnull);
  if (NS_FAILED(rv)) {
    nsCAutoString warnMsg;
    warnMsg.Append("Bookmarks Export: Found invalid item uri '");
    warnMsg.Append(uri);
    warnMsg.Append("'");
    NS_WARNING(warnMsg.get());
    return NS_OK;
  }

  
  PRUint32 dummy;
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

  
  PRInt64 itemId;
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

  
  PRBool hasPostData;
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

  
  
  PRBool loadInSidebar = PR_FALSE;
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

  
  nsCAutoString title;
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
nsPlacesImportExportService::WriteLivemark(nsINavHistoryResultNode* aFolder, const nsACString& aIndent,
                              nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aOutput->Write(kItemOpen, sizeof(kItemOpen)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 folderId;
  rv = aFolder->GetItemId(&folderId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIURI> feedURI;
  rv = mLivemarkService->GetFeedURI(folderId, getter_AddRefs(feedURI));
  NS_ENSURE_SUCCESS(rv, rv);
  if (feedURI) {
    nsCString feedSpec;
    rv = feedURI->GetSpec(feedSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aOutput->Write(kFeedURIAttribute, sizeof(kFeedURIAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = WriteEscapedUrl(feedSpec, aOutput);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIURI> siteURI;
  rv = mLivemarkService->GetSiteURI(folderId, getter_AddRefs(siteURI));
  NS_ENSURE_SUCCESS(rv, rv);
  if (siteURI) {
    nsCString siteSpec;
    rv = siteURI->GetSpec(siteSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aOutput->Write(kHrefAttribute, sizeof(kHrefAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = WriteEscapedUrl(siteSpec, aOutput);
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
nsPlacesImportExportService::WriteSeparator(nsINavHistoryResultNode* aItem,
                                            const nsACString& aIndent,
                                            nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(PromiseFlatCString(aIndent).get(), aIndent.Length(),
                        &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aOutput->Write(kSeparator, sizeof(kSeparator)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 itemId;
  rv = aItem->GetItemId(&itemId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  nsCAutoString title;
  rv = mBookmarksService->GetItemTitle(itemId, title);
  if (NS_SUCCEEDED(rv) && !title.IsEmpty()) {
    rv = aOutput->Write(kNameAttribute, strlen(kNameAttribute), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);

    char* escapedTitle = nsEscapeHTML(title.get());
    if (escapedTitle) {
      PRUint32 dummy;
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
  nsCAutoString escaped(aString);
  PRInt32 offset;
  while ((offset = escaped.FindChar('\"')) >= 0) {
    escaped.Cut(offset, 1);
    escaped.Insert(NS_LITERAL_CSTRING("%22"), offset);
  }
  PRUint32 dummy;
  return aOutput->Write(escaped.get(), escaped.Length(), &dummy);
}






nsresult
nsPlacesImportExportService::WriteContainerContents(nsINavHistoryResultNode* aFolder,
                                                    const nsACString& aIndent,
                                                    nsIOutputStream* aOutput)
{
  nsCAutoString myIndent(aIndent);
  myIndent.Append(kIndent);

  PRInt64 folderId;
  nsresult rv = aFolder->GetItemId(&folderId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsINavHistoryContainerResultNode> folderNode = do_QueryInterface(aFolder, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = folderNode->SetContainerOpen(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 childCount = 0;
  folderNode->GetChildCount(&childCount);
  for (PRUint32 i = 0; i < childCount; ++i) {
    nsCOMPtr<nsINavHistoryResultNode> child;
    rv = folderNode->GetChild(i, getter_AddRefs(child));
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 type = 0;
    rv = child->GetType(&type);
    NS_ENSURE_SUCCESS(rv, rv);
    if (type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER) {
      
      PRInt64 childFolderId;
      rv = child->GetItemId(&childFolderId);
      NS_ENSURE_SUCCESS(rv, rv);

      
      PRBool isLivemark;
      rv = mLivemarkService->IsLivemark(childFolderId, &isLivemark);
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







static void
NotifyImportObservers(const char* aTopic,
                      PRInt64 aFolderId,
                      PRBool aIsInitialImport)
{
  nsCOMPtr<nsIObserverService> obs =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
  if (!obs)
    return;

  nsCOMPtr<nsISupports> folderIdSupp = nsnull;
  if (aFolderId > 0) {
    nsCOMPtr<nsISupportsPRInt64> folderIdInt =
      do_CreateInstance(NS_SUPPORTS_PRINT64_CONTRACTID);
    if (!folderIdInt)
      return;

    if (NS_FAILED(folderIdInt->SetData(aFolderId)))
      return;

    folderIdSupp = do_QueryInterface(folderIdInt);
  }

  obs->NotifyObservers(folderIdSupp,
                       aTopic,
                       (aIsInitialImport ? RESTORE_INITIAL_NSIOBSERVER_DATA
                                         : RESTORE_NSIOBSERVER_DATA).get());
}


NS_IMETHODIMP
nsPlacesImportExportService::ImportHTMLFromFile(nsILocalFile* aFile,
                                                PRBool aIsInitialImport)
{
  NotifyImportObservers(RESTORE_BEGIN_NSIOBSERVER_TOPIC, -1, aIsInitialImport);

  
  nsresult rv = ImportHTMLFromFileInternal(aFile,
                                           PR_FALSE,
                                           0,
                                           aIsInitialImport);

  if (NS_FAILED(rv)) {
    NotifyImportObservers(RESTORE_FAILED_NSIOBSERVER_TOPIC,
                          -1,
                          aIsInitialImport);
  }
  else {
    NotifyImportObservers(RESTORE_SUCCESS_NSIOBSERVER_TOPIC,
                          -1,
                          aIsInitialImport);
  }

  return rv;
}


NS_IMETHODIMP
nsPlacesImportExportService::ImportHTMLFromURI(nsIURI* aURI,
                                               PRBool aIsInitialImport)
{
  NotifyImportObservers(RESTORE_BEGIN_NSIOBSERVER_TOPIC, -1, aIsInitialImport);

  
  nsresult rv = ImportHTMLFromURIInternal(aURI,
                                          PR_FALSE,
                                          0,
                                          aIsInitialImport);

  if (NS_FAILED(rv)) {
    NotifyImportObservers(RESTORE_FAILED_NSIOBSERVER_TOPIC,
                          -1,
                          aIsInitialImport);
  }
  else {
    NotifyImportObservers(RESTORE_SUCCESS_NSIOBSERVER_TOPIC,
                          -1,
                          aIsInitialImport);
  }

  return rv;
}


NS_IMETHODIMP
nsPlacesImportExportService::ImportHTMLFromFileToFolder(nsILocalFile* aFile,
                                                        PRInt64 aFolderId,
                                                        PRBool aIsInitialImport)
{
  NotifyImportObservers(RESTORE_BEGIN_NSIOBSERVER_TOPIC,
                        aFolderId,
                        aIsInitialImport);

  
  nsresult rv = ImportHTMLFromFileInternal(aFile,
                                           PR_FALSE,
                                           aFolderId,
                                           aIsInitialImport);

  if (NS_FAILED(rv)) {
    NotifyImportObservers(RESTORE_FAILED_NSIOBSERVER_TOPIC,
                          aFolderId,
                          aIsInitialImport);
  }
  else {
    NotifyImportObservers(RESTORE_SUCCESS_NSIOBSERVER_TOPIC,
                          aFolderId,
                          aIsInitialImport);
  }

  return rv;
}


nsresult
nsPlacesImportExportService::ImportHTMLFromFileInternal(nsILocalFile* aFile,
                                                        PRBool aAllowRootChanges,
                                                        PRInt64 aFolder,
                                                        PRBool aIsImportDefaults)
{
  nsresult rv;

  nsCOMPtr<nsIFile> file = do_QueryInterface(aFile);
  NS_ENSURE_STATE(file);

#ifdef DEBUG_IMPORT
  nsAutoString path;
  file->GetPath(path);
  printf("\nImporting %s\n", NS_ConvertUTF16toUTF8(path).get());
#endif

  
  PRBool exists;
  rv = file->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIIOService> ioservice = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> fileURI;
  rv = ioservice->NewFileURI(file, getter_AddRefs(fileURI));
  NS_ENSURE_SUCCESS(rv, rv);

  return ImportHTMLFromURIInternal(fileURI, aAllowRootChanges, aFolder, aIsImportDefaults);
}

nsresult
nsPlacesImportExportService::ImportHTMLFromURIInternal(nsIURI* aURI,
                                                       PRBool aAllowRootChanges,
                                                       PRInt64 aFolder,
                                                       PRBool aIsImportDefaults)
{
  nsresult rv = EnsureServiceState();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIParser> parser = do_CreateInstance(kParserCID);
  NS_ENSURE_TRUE(parser, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<BookmarkContentSink> sink = new BookmarkContentSink();
  NS_ENSURE_TRUE(sink, NS_ERROR_OUT_OF_MEMORY);
  rv = sink->Init(aAllowRootChanges, aFolder, aIsImportDefaults);
  NS_ENSURE_SUCCESS(rv, rv);
  parser->SetContentSink(sink);

  
  
  nsCOMPtr<nsIIOService> ioservice = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ioservice->NewChannelFromURI(aURI, getter_AddRefs(mImportChannel));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mImportChannel->SetContentType(NS_LITERAL_CSTRING("text/html"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = parser->Parse(aURI, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  mIsImportDefaults = aIsImportDefaults;
  mBookmarksService->RunInBatchMode(this, parser);
  mImportChannel = nsnull;

  return NS_OK;
}


NS_IMETHODIMP
nsPlacesImportExportService::RunBatched(nsISupports* aUserData)
{
  nsresult rv;
  if (mIsImportDefaults) {
    PRInt64 bookmarksMenuFolder;
    rv = mBookmarksService->GetBookmarksMenuFolder(&bookmarksMenuFolder);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mBookmarksService->RemoveFolderChildren(bookmarksMenuFolder);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt64 toolbarFolder;
    rv = mBookmarksService->GetToolbarFolder(&toolbarFolder);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mBookmarksService->RemoveFolderChildren(toolbarFolder);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt64 unfiledBookmarksFolder;
    rv = mBookmarksService->GetUnfiledBookmarksFolder(&unfiledBookmarksFolder);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mBookmarksService->RemoveFolderChildren(unfiledBookmarksFolder);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIInputStream> stream;
  rv = mImportChannel->Open(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIInputStream> bufferedstream;
  rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedstream), stream, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsCOMPtr<nsIStreamListener> listener = do_QueryInterface(aUserData, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listener->OnStartRequest(mImportChannel, nsnull);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnStartRequest failed");
  rv = SyncChannelStatus(mImportChannel, rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "SyncChannelStatus failed");

  while (NS_SUCCEEDED(rv))
  {
    PRUint32 available;
    rv = bufferedstream->Available(&available);
    if (rv == NS_BASE_STREAM_CLOSED) {
      rv = NS_OK;
      available = 0;
    }
    if (NS_FAILED(rv)) {
      mImportChannel->Cancel(rv);
      break;
    }
    if (!available)
      break; 

    rv = listener->OnDataAvailable(mImportChannel, nsnull, bufferedstream, 0,
                                   available);
    if (NS_FAILED(rv))
      break;
    rv = SyncChannelStatus(mImportChannel, rv);
    if (NS_FAILED(rv))
      break;
  }

  rv = listener->OnStopRequest(mImportChannel, nsnull, rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsPlacesImportExportService::ExportHTMLToFile(nsILocalFile* aBookmarksFile)
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

  
  PRInt64 bookmarksMenuFolder;
  rv = mBookmarksService->GetBookmarksMenuFolder(&bookmarksMenuFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = query->SetFolders(&bookmarksMenuFolder, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mHistoryService->ExecuteQuery(query, options, getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsINavHistoryContainerResultNode> rootNode;
  rv = result->GetRoot(getter_AddRefs(rootNode));
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUint32 dummy;
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

  
  nsCAutoString indent;
  indent.Assign(kIndent);

  
  rv = WriteContainerContents(rootNode, EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  PRInt64 toolbarFolder;
  rv = mBookmarksService->GetToolbarFolder(&toolbarFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = query->SetFolders(&toolbarFolder, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mHistoryService->ExecuteQuery(query, options, getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = result->GetRoot(getter_AddRefs(rootNode));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = rootNode->SetContainerOpen(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  PRUint32 childCount = 0;
  rv = rootNode->GetChildCount(&childCount);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = rootNode->SetContainerOpen(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  if (childCount) {
    rv = WriteContainer(rootNode, nsDependentCString(kIndent), strm);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  PRInt64 unfiledBookmarksFolder;
  rv = mBookmarksService->GetUnfiledBookmarksFolder(&unfiledBookmarksFolder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = query->SetFolders(&unfiledBookmarksFolder, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mHistoryService->ExecuteQuery(query, options, getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = result->GetRoot(getter_AddRefs(rootNode));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = rootNode->SetContainerOpen(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  childCount = 0;
  rootNode->GetChildCount(&childCount);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = rootNode->SetContainerOpen(PR_FALSE);
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
nsPlacesImportExportService::BackupBookmarksFile()
{
  nsresult rv = EnsureServiceState();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFile> bookmarksFileDir;
  rv = NS_GetSpecialDirectory(NS_APP_BOOKMARKS_50_FILE,
                              getter_AddRefs(bookmarksFileDir));

  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsILocalFile> bookmarksFile = do_QueryInterface(bookmarksFileDir);
  NS_ENSURE_STATE(bookmarksFile);

  
  PRBool exists;
  rv = bookmarksFile->Exists(&exists);
  if (NS_FAILED(rv) || !exists) {
    rv = bookmarksFile->Create(nsIFile::NORMAL_FILE_TYPE, 0600);
    if (NS_FAILED(rv)) {
      NS_WARNING("Unable to create bookmarks.html!");
      return rv;
    }
  }

  
  rv = ExportHTMLToFile(bookmarksFile);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
