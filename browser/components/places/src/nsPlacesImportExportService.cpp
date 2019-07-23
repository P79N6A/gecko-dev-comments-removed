



























































































#include "nsPlacesImportExportService.h"
#include "nsNetUtil.h"
#include "nsParserCIID.h"
#include "nsStringAPI.h"
#include "nsUnicharUtils.h"
#include "plbase64.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIPrefService.h"
#include "nsToolkitCompsCID.h"
#include "nsIHTMLContentSink.h"
#include "nsIParser.h"
#include "prprf.h"

static NS_DEFINE_CID(kParserCID, NS_PARSER_CID);

#define KEY_TOOLBARFOLDER_LOWER "personal_toolbar_folder"
#define KEY_BOOKMARKSMENU_LOWER "bookmarks_menu"
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
#define KEY_GENERATED_TITLE_LOWER "generated_title"
#define KEY_DATE_ADDED_LOWER "add_date"
#define KEY_LAST_MODIFIED_LOWER "last_modified"

#define LOAD_IN_SIDEBAR_ANNO NS_LITERAL_CSTRING("bookmarkProperties/loadInSidebar")
#define DESCRIPTION_ANNO NS_LITERAL_CSTRING("bookmarkProperties/description")
#define POST_DATA_ANNO NS_LITERAL_CSTRING("URIProperties/POSTData")
#define GENERATED_TITLE_ANNO NS_LITERAL_CSTRING("bookmarks/generatedTitle")

#define BOOKMARKS_MENU_ICON_URI "chrome://browser/skin/places/bookmarksMenu.png"





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
                       Container_Toolbar };

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




char *
nsEscapeHTML(const char * string)
{
	
	char *rv = (char *) nsMemory::Alloc(strlen(string) * 6 + 1);
	char *ptr = rv;

	if(rv)
	  {
		for(; *string != '\0'; string++)
		  {
			if(*string == '<')
			  {
				*ptr++ = '&';
				*ptr++ = 'l';
				*ptr++ = 't';
				*ptr++ = ';';
			  }
			else if(*string == '>')
			  {
				*ptr++ = '&';
				*ptr++ = 'g';
				*ptr++ = 't';
				*ptr++ = ';';
			  }
			else if(*string == '&')
			  {
				*ptr++ = '&';
				*ptr++ = 'a';
				*ptr++ = 'm';
				*ptr++ = 'p';
				*ptr++ = ';';
			  }
			else if (*string == '"')
			  {
				*ptr++ = '&';
				*ptr++ = 'q';
				*ptr++ = 'u';
				*ptr++ = 'o';
				*ptr++ = 't';
				*ptr++ = ';';
			  }			
			else if (*string == '\'')
			  {
				*ptr++ = '&';
				*ptr++ = '#';
				*ptr++ = '3';
				*ptr++ = '9';
				*ptr++ = ';';
			  }
			else
			  {
				*ptr++ = *string;
			  }
		  }
		*ptr = '\0';
	  }

	return(rv);
}

NS_IMPL_ISUPPORTS2(nsPlacesImportExportService, nsIPlacesImportExportService,
                   nsINavHistoryBatchCallback)

nsPlacesImportExportService::nsPlacesImportExportService()
{
  nsresult rv;
  mHistoryService = do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get history service");
  mFaviconService = do_GetService(NS_FAVICONSERVICE_CONTRACTID, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get favicon service");
  mAnnotationService = do_GetService(NS_ANNOTATIONSERVICE_CONTRACTID, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get annotation service");
  mBookmarksService = do_GetService(NS_NAVBOOKMARKSSERVICE_CONTRACTID, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get bookmarks service");
  mLivemarkService = do_GetService(NS_LIVEMARKSERVICE_CONTRACTID, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get livemark service");
  mMicrosummaryService = do_GetService("@mozilla.org/microsummary/service;1", &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get microsummary service");
}

nsPlacesImportExportService::~nsPlacesImportExportService()
{
}




class BookmarkContentSink : public nsIHTMLContentSink
{
public:
  nsresult Init(PRBool aAllowRootChanges,
                nsINavBookmarksService* bookmarkService,
                PRInt64 aFolder,
                PRBool aIsImportDefaults);

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD WillTokenize() { return NS_OK; }
  NS_IMETHOD WillBuildModel() { return NS_OK; }
  NS_IMETHOD DidBuildModel() { return NS_OK; }
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
  NS_IMETHOD WillProcessTokens() { return NS_OK; }
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
  nsCOMPtr<nsIMicrosummaryService> mMicrosummaryService;

  
  
  
  
  
  
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
                            const nsCString& aData);
  nsresult SetFaviconForFolder(PRInt64 aFolder, const nsACString& aFavicon);

  PRInt64 ConvertImportedIdToInternalId(const nsCString& aId);
  PRTime ConvertImportedDateToInternalDate(const nsACString& aDate);

#ifdef DEBUG_IMPORT
  
  void PrintNesting()
  {
    for (PRUint32 i = 0; i < mFrames.Length(); i ++)
      printf("  ");
  }
#endif
};







nsresult
BookmarkContentSink::Init(PRBool aAllowRootChanges,
                          nsINavBookmarksService* bookmarkService,
                          PRInt64 aFolder,
                          PRBool aIsImportDefaults)
{
  nsresult rv;
  mBookmarksService = bookmarkService;
  mHistoryService = do_GetService(NS_NAVHISTORYSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mAnnotationService = do_GetService(NS_ANNOTATIONSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mLivemarkService = do_GetService(NS_LIVEMARKSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mMicrosummaryService = do_GetService("@mozilla.org/microsummary/service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mAllowRootChanges = aAllowRootChanges;
  mIsImportDefaults = aIsImportDefaults;

  
  PRInt64 menuRoot;
  if (aFolder == 0) {
    rv = mBookmarksService->GetBookmarksRoot(&menuRoot);
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

      PRInt64 itemId = !frame.mPreviousLink ?
                       frame.mContainerID : frame.mPreviousId;
                    
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
        NS_ASSERTION(NS_SUCCEEDED(rv), "SetItemLastModified failed");
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
  if (mFrames.Length() > 1 && frame.mContainerNesting == 0)
    PopFrame();
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
      nsresult rv = mBookmarksService->GetPlacesRoot(&placesRoot);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "could not get placesRoot");
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
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_BOOKMARKSMENU_LOWER)) {
        if (mIsImportDefaults)
          frame.mLastContainerType = BookmarkImportFrame::Container_Menu;
        break;
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_PLACESROOT_LOWER)) {
        if (mIsImportDefaults)
          frame.mLastContainerType = BookmarkImportFrame::Container_Places;
        break;
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_DATE_ADDED_LOWER)) {
        frame.mPreviousDateAdded =
          ConvertImportedDateToInternalDate(NS_ConvertUTF16toUTF8(node.GetValueAt(i)));
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_LAST_MODIFIED_LOWER)) {
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
  nsresult rv;

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
  nsAutoString itemId;
  nsAutoString micsumGenURI;
  nsAutoString generatedTitle;
  nsAutoString dateAdded;
  nsAutoString lastModified;

  PRInt32 attrCount = node.GetAttributeCount();
  for (PRInt32 i = 0; i < attrCount; i ++) {
    const nsAString& key = node.GetKeyAt(i);
    if (key.LowerCaseEqualsLiteral(KEY_HREF_LOWER)) {
      href = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_FEEDURL_LOWER)) {
      feedUrl = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_ICON_LOWER)) {
      icon = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_ICON_URI_LOWER)) {
      iconUri = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_LASTCHARSET_LOWER)) {
      lastCharset = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_SHORTCUTURL_LOWER)) {
      keyword = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_POST_DATA_LOWER)) {
      postData = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_WEB_PANEL_LOWER)) {
      webPanel = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_MICSUM_GEN_URI_LOWER)) {
      micsumGenURI = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_GENERATED_TITLE_LOWER)) {
      generatedTitle = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_DATE_ADDED_LOWER)) {
      dateAdded = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_LAST_MODIFIED_LOWER)) {
      lastModified = node.GetValueAt(i);
    }
  }
  href.Trim(kWhitespace);
  feedUrl.Trim(kWhitespace);
  icon.Trim(kWhitespace);
  iconUri.Trim(kWhitespace);
  lastCharset.Trim(kWhitespace);
  keyword.Trim(kWhitespace);
  postData.Trim(kWhitespace);
  webPanel.Trim(kWhitespace);
  itemId.Trim(kWhitespace);
  micsumGenURI.Trim(kWhitespace);
  generatedTitle.Trim(kWhitespace);
  dateAdded.Trim(kWhitespace);
  lastModified.Trim(kWhitespace);

  
  
  if (!feedUrl.IsEmpty()) {
    NS_NewURI(getter_AddRefs(frame.mPreviousFeed),
              NS_ConvertUTF16toUTF8(feedUrl), nsnull);
  }

  
  if (href.IsEmpty()) {
    frame.mPreviousLink = nsnull;

    
    
    if (!frame.mPreviousFeed)
      return;
  } else {
    
    
    
    nsresult rv = NS_NewURI(getter_AddRefs(frame.mPreviousLink),
                   href, nsnull);
    if (NS_FAILED(rv) && !frame.mPreviousFeed) {
      frame.mPreviousLink = nsnull;
      return; 
    }
  }

  
  frame.mPreviousId = ConvertImportedIdToInternalId(NS_ConvertUTF16toUTF8(itemId));

  
  if (!lastModified.IsEmpty()) {
    frame.mPreviousLastModifiedDate = ConvertImportedDateToInternalDate(NS_ConvertUTF16toUTF8(lastModified));
  }

  
  
  if (frame.mPreviousFeed)
    return;

  
  PRInt64 parent;
  if (frame.mPreviousId > 0) {
    rv = mBookmarksService->GetFolderIdForItem(frame.mPreviousId, &parent);
    if (NS_FAILED(rv) || frame.mContainerID != parent)
      frame.mPreviousId = 0;
  }

  
  if (frame.mPreviousId == 0) {
    
    rv = mBookmarksService->InsertBookmark(frame.mContainerID,
                                           frame.mPreviousLink,
                                           mBookmarksService->DEFAULT_INDEX,
                                           EmptyString(),
                                           &frame.mPreviousId);
    NS_ASSERTION(NS_SUCCEEDED(rv), "InsertBookmark failed");

    
    
    
    if (!dateAdded.IsEmpty()) {
      PRTime convertedDateAdded = ConvertImportedDateToInternalDate(NS_ConvertUTF16toUTF8(dateAdded));
      if (convertedDateAdded) {
        rv = mBookmarksService->SetItemDateAdded(frame.mPreviousId, convertedDateAdded);
        NS_ASSERTION(NS_SUCCEEDED(rv), "SetItemDateAdded failed");
      }
    }
  }

  
  if (!icon.IsEmpty() || !iconUri.IsEmpty()) {
    nsCOMPtr<nsIURI> iconUriObject;
    NS_NewURI(getter_AddRefs(iconUriObject), iconUri);
    if (!icon.IsEmpty() || iconUriObject) {
      rv = SetFaviconForURI(frame.mPreviousLink, iconUriObject,
                            NS_ConvertUTF16toUTF8(icon));
    }
  }

  
  if (!keyword.IsEmpty()) {
    mBookmarksService->SetKeywordForBookmark(frame.mPreviousId, keyword);

    
    if (!postData.IsEmpty()) {
      mAnnotationService->SetPageAnnotationString(frame.mPreviousLink, POST_DATA_ANNO,
                                                  postData, 0,
                                                  nsIAnnotationService::EXPIRE_NEVER);
    }
  }

  if (webPanel.LowerCaseEqualsLiteral("true")) {
    
    mAnnotationService->SetItemAnnotationInt32(frame.mPreviousId, LOAD_IN_SIDEBAR_ANNO,
                                               1, 0,
                                               nsIAnnotationService::EXPIRE_NEVER);
  }

  
  
  
  if (!micsumGenURI.IsEmpty()) {
    nsCOMPtr<nsIURI> micsumGenURIObject;
    nsCOMPtr<nsIURI> hrefObject;
    if (NS_SUCCEEDED(NS_NewURI(getter_AddRefs(micsumGenURIObject), micsumGenURI)) &&
        NS_SUCCEEDED(NS_NewURI(getter_AddRefs(hrefObject), href))) {
      nsCOMPtr<nsIMicrosummary> microsummary;
      mMicrosummaryService->CreateMicrosummary(hrefObject, micsumGenURIObject,
                                               getter_AddRefs(microsummary));
      mMicrosummaryService->SetMicrosummary(frame.mPreviousId, microsummary);

      
      rv = mAnnotationService->SetItemAnnotationString(frame.mPreviousId, GENERATED_TITLE_ANNO,
                                                       generatedTitle, 0,
                                                       nsIAnnotationService::EXPIRE_NEVER);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Creating microsummary generated title failed");
    }
  }

  
}








void
BookmarkContentSink::HandleLinkEnd()
{
  nsresult rv;
  BookmarkImportFrame& frame = CurFrame();
  frame.mPreviousText.Trim(kWhitespace);
  if (frame.mPreviousFeed) {
    
    
    

    
    if (frame.mPreviousId > 0) {
      PRInt64 parent;
      nsresult rv = mBookmarksService->GetFolderIdForItem(frame.mPreviousId, &parent);
      if (NS_FAILED(rv) || parent != frame.mContainerID) {
        frame.mPreviousId = 0;
      }
    }

    PRBool isLivemark = PR_FALSE;
    if (frame.mPreviousId > 0) {
      mLivemarkService->IsLivemark(frame.mPreviousId, &isLivemark);
      if (isLivemark) {
        
#ifdef DEBUG_IMPORT
        PrintNesting();
        printf("Updating livemark '%s' %lld\n",
               NS_ConvertUTF16toUTF8(frame.mPreviousText).get(), frame.mPreviousId);
#endif
        rv = mLivemarkService->SetSiteURI(frame.mPreviousId, frame.mPreviousLink);
        NS_ASSERTION(NS_SUCCEEDED(rv), "SetSiteURI failed!");
        rv = mLivemarkService->SetFeedURI(frame.mPreviousId, frame.mPreviousFeed);
        NS_ASSERTION(NS_SUCCEEDED(rv), "SetFeedURI failed!");
        rv = mBookmarksService->SetItemTitle(frame.mPreviousId, frame.mPreviousText);
        NS_ASSERTION(NS_SUCCEEDED(rv), "SetItemTitle failed!");
      }
    }

    if (!isLivemark) {
      if (mIsImportDefaults) {
        rv = mLivemarkService->CreateLivemarkFolderOnly(mBookmarksService,
                                                   frame.mContainerID,
                                                   frame.mPreviousText,
                                                   frame.mPreviousLink,
                                                   frame.mPreviousFeed,
                                                   -1,
                                                   &frame.mPreviousId);
        NS_ASSERTION(NS_SUCCEEDED(rv), "CreateLivemarkFolderOnly failed!");
      } else {
        rv = mLivemarkService->CreateLivemark(frame.mContainerID,
                                         frame.mPreviousText,
                                         frame.mPreviousLink,
                                         frame.mPreviousFeed,
                                         -1,
                                         &frame.mPreviousId);
        NS_ASSERTION(NS_SUCCEEDED(rv), "CreateLivemark failed!");
      }
#ifdef DEBUG_IMPORT
      PrintNesting();
      printf("Creating livemark '%s' %lld\n",
             NS_ConvertUTF16toUTF8(frame.mPreviousText).get(), frame.mPreviousId);
#endif
    }
  }
  else if (frame.mPreviousLink) {
#ifdef DEBUG_IMPORT
    PrintNesting();
    printf("Creating bookmark '%s' %lld\n",
           NS_ConvertUTF16toUTF8(frame.mPreviousText).get(), frame.mPreviousId);
#endif
    mBookmarksService->SetItemTitle(frame.mPreviousId, frame.mPreviousText);
  }

  
  
  
  if (frame.mPreviousId > 0 && frame.mPreviousLastModifiedDate > 0) {
    rv = mBookmarksService->SetItemLastModified(frame.mPreviousId, frame.mPreviousLastModifiedDate);
    NS_ASSERTION(NS_SUCCEEDED(rv), "SetItemLastModified failed");
    
    
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

  PRInt64 itemId;
  mBookmarksService->InsertSeparator(frame.mContainerID,
                                     mBookmarksService->DEFAULT_INDEX,
                                     &itemId);
  
  nsAutoString name;
  PRInt32 attrCount = aNode.GetAttributeCount();
  for (PRInt32 i = 0; i < attrCount; i ++) {
    const nsAString& key = aNode.GetKeyAt(i);
    if (key.LowerCaseEqualsLiteral(KEY_NAME_LOWER))
      name = aNode.GetValueAt(i);
  }
  name.Trim(kWhitespace);

  if (!name.IsEmpty())
    mBookmarksService->SetItemTitle(itemId, name);

  
  
  
  
}







nsresult
BookmarkContentSink::NewFrame()
{
  nsresult rv;

  PRInt64 ourID = 0;
  nsString containerName;
  BookmarkImportFrame::ContainerType containerType;
  CurFrame().ConsumeHeading(&containerName, &containerType);

  PRBool updateFolder = PR_FALSE;
  
  switch (containerType) {
    case BookmarkImportFrame::Container_Normal:
      
      rv = mBookmarksService->CreateFolder(CurFrame().mContainerID,
                                           containerName,
                                           mBookmarksService->DEFAULT_INDEX, 
                                           &ourID);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    case BookmarkImportFrame::Container_Places:
      
      
      rv = mBookmarksService->GetPlacesRoot(&ourID);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    case BookmarkImportFrame::Container_Menu:
      
      rv = mBookmarksService->GetBookmarksRoot(&ourID);
      NS_ENSURE_SUCCESS(rv, rv);
      if (mAllowRootChanges) {
        updateFolder = PR_TRUE;
        rv = SetFaviconForFolder(ourID, NS_LITERAL_CSTRING(BOOKMARKS_MENU_ICON_URI));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      break;
    case BookmarkImportFrame::Container_Toolbar:
      
      PRInt64 toolbarFolder;
      rv = mBookmarksService->GetToolbarFolder(&toolbarFolder);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!toolbarFolder) {
        
        rv = mBookmarksService->CreateFolder(CurFrame().mContainerID,
                                             containerName,
                                             mBookmarksService->DEFAULT_INDEX, &ourID);
        NS_ENSURE_SUCCESS(rv, rv);
        
        rv = mBookmarksService->SetToolbarFolder(ourID);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        ourID = toolbarFolder;
      }
      break;
    default:
      NS_NOTREACHED("Unknown container type");
  }

#ifdef DEBUG_IMPORT
  PrintNesting();
  printf("Folder %lld \'%s\'", ourID, NS_ConvertUTF16toUTF8(containerName).get());
#endif

  if (updateFolder) {
    
    mBookmarksService->MoveItem(ourID, CurFrame().mContainerID, -1);
    mBookmarksService->SetItemTitle(ourID, containerName);
#ifdef DEBUG_IMPORT
    printf(" [reparenting]");
#endif
  }

#ifdef DEBUG_IMPORT
  printf("\n");
#endif

  BookmarkImportFrame& frame = CurFrame();
  if (frame.mPreviousDateAdded > 0) {
    nsresult rv = mBookmarksService->SetItemDateAdded(ourID, frame.mPreviousDateAdded);
    NS_ASSERTION(NS_SUCCEEDED(rv), "SetItemDateAdded failed");
    frame.mPreviousDateAdded = 0;
  }
  if (frame.mPreviousLastModifiedDate > 0) {
    nsresult rv = mBookmarksService->SetItemLastModified(ourID, frame.mPreviousLastModifiedDate);
    NS_ASSERTION(NS_SUCCEEDED(rv), "SetItemLastModified failed");
    
  }

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
                                      const nsCString& aData)
{
  nsresult rv;
  static PRUint32 serialNumber = 0; 

  nsCOMPtr<nsIFaviconService> faviconService(do_GetService(NS_FAVICONSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
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
  } else {
    
    nsCAutoString faviconSpec;
    faviconSpec.AssignLiteral("http://www.mozilla.org/2005/made-up-favicon/");
    faviconSpec.AppendInt(serialNumber);
    faviconSpec.AppendLiteral("-");
    char buf[32];
    PR_snprintf(buf, sizeof(buf), "%lld", PR_Now());
    faviconSpec.Append(buf);
    rv = NS_NewURI(getter_AddRefs(faviconURI), faviconSpec);
    NS_ENSURE_SUCCESS(rv, rv);
    serialNumber++;
  }

  nsCOMPtr<nsIURI> dataURI;
  rv = NS_NewURI(getter_AddRefs(dataURI), aData);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIProtocolHandler> protocolHandler;
  rv = ioService->GetProtocolHandler("data", getter_AddRefs(protocolHandler));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIChannel> channel;
  rv = protocolHandler->NewChannel(dataURI, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIInputStream> stream;
  rv = channel->Open(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 available;
  rv = stream->Available(&available);
  NS_ENSURE_SUCCESS(rv, rv);
  if (available == 0)
    return NS_ERROR_FAILURE;

  
  PRUint8* buffer = static_cast<PRUint8*>
                               (nsMemory::Alloc(sizeof(PRUint8) * available));
  if (!buffer)
    return NS_ERROR_OUT_OF_MEMORY;
  PRUint32 numRead;
  rv = stream->Read(reinterpret_cast<char*>(buffer), available, &numRead);
  if (NS_FAILED(rv) || numRead != available) {
    nsMemory::Free(buffer);
    return rv;
  }

  nsCAutoString mimeType;
  rv = channel->GetContentType(mimeType);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = faviconService->SetFaviconData(faviconURI, buffer, available, mimeType, 0);
  nsMemory::Free(buffer);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = faviconService->SetFaviconUrlForPage(aPageURI, faviconURI);
  return NS_OK; 
}









nsresult
BookmarkContentSink::SetFaviconForFolder(PRInt64 aFolder,
                                         const nsACString& aFavicon)
{
  nsresult rv;
  nsCOMPtr<nsIFaviconService> faviconService(do_GetService(NS_FAVICONSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> folderURI;
  rv = mBookmarksService->GetFolderURI(aFolder,
                                                getter_AddRefs(folderURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> faviconURI;
  rv = NS_NewURI(getter_AddRefs(faviconURI), aFavicon);
  NS_ENSURE_SUCCESS(rv, rv);

  return faviconService->SetFaviconUrlForPage(folderURI, faviconURI);
}


PRInt64
BookmarkContentSink::ConvertImportedIdToInternalId(const nsCString& aId) {
  PRInt64 intId = 0;
  if (aId.IsEmpty())
    return intId;
  nsresult rv;
  intId = aId.ToInteger(&rv);
  if (NS_FAILED(rv))
    intId = 0;
  return intId;
}


PRTime
BookmarkContentSink::ConvertImportedDateToInternalDate(const nsACString& aDate) {
  PRTime convertedDate = 0;
  if (!aDate.IsEmpty()) {
    nsresult rv;
    convertedDate = aDate.ToInteger(&rv);
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
static const char kIconAttribute[] = " ICON=\"";
static const char kIconURIAttribute[] = " ICON_URI=\"";
static const char kHrefAttribute[] = " HREF=\"";
static const char kFeedURIAttribute[] = " FEEDURL=\"";
static const char kWebPanelAttribute[] = " WEB_PANEL=\"true\"";
static const char kKeywordAttribute[] = " SHORTCUTURL=\"";
static const char kPostDataAttribute[] = " POST_DATA=\"";
static const char kItemIdAttribute[] = " ITEM_ID=\"";
static const char kNameAttribute[] = " NAME=\"";
static const char kMicsumGenURIAttribute[]    = " MICSUM_GEN_URI=\"";
static const char kDateAddedAttribute[] = " ADD_DATE=\"";
static const char kLastModifiedAttribute[] = " LAST_MODIFIED=\"";







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
DataToDataURI(PRUint8* aData, PRUint32 aDataLen, const nsACString& aMimeType,
              nsACString& aDataURI)
{
  char* encoded = PL_Base64Encode(reinterpret_cast<const char*>(aData),
                                  aDataLen, nsnull);
  if (!encoded)
    return NS_ERROR_OUT_OF_MEMORY;

  aDataURI.AssignLiteral("data:");
  aDataURI.Append(aMimeType);
  aDataURI.AppendLiteral(";base64,");
  aDataURI.Append(encoded);

  nsMemory::Free(encoded);
  return NS_OK;
}








static nsresult
WriteFaviconAttribute(const nsACString& aURI, nsIOutputStream* aOutput)
{
  nsresult rv;
  PRUint32 dummy;

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  
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
    

    
    nsCAutoString mimeType;
    PRUint32 dataLen;
    PRUint8* data;
    rv = faviconService->GetFaviconData(faviconURI, mimeType, &dataLen, &data);
    NS_ENSURE_SUCCESS(rv, rv);
    if (dataLen > 0) {
      
      nsCString faviconContents;
      rv = DataToDataURI(data, dataLen, mimeType, faviconContents);
      nsMemory::Free(data);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aOutput->Write(kIconAttribute, sizeof(kIconAttribute)-1, &dummy);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aOutput->Write(faviconContents.get(), faviconContents.Length(), &dummy);
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
  return aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
}






nsresult
nsPlacesImportExportService::WriteContainer(nsINavHistoryResultNode* aFolder, const nsACString& aIndent,
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
nsPlacesImportExportService::WriteContainerHeader(nsINavHistoryResultNode* aFolder, const nsACString& aIndent,
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
  NS_ENSURE_SUCCESS(rv,rv);

  PRInt64 bookmarksRoot;
  rv = mBookmarksService->GetBookmarksRoot(&bookmarksRoot);
  NS_ENSURE_SUCCESS(rv,rv);

  PRInt64 toolbarFolder;
  rv = mBookmarksService->GetToolbarFolder(&toolbarFolder);
  NS_ENSURE_SUCCESS(rv,rv);

  
  if (folderId == placesRoot) {
    rv = aOutput->Write(kPlacesRootAttribute, sizeof(kPlacesRootAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (folderId == bookmarksRoot) {
    rv = aOutput->Write(kBookmarksRootAttribute, sizeof(kBookmarksRootAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (folderId == toolbarFolder) {
    rv = aOutput->Write(kToolbarFolderAttribute, sizeof(kToolbarFolderAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }
 
  
  nsCOMPtr<nsIURI> folderURI;
  rv = mBookmarksService->GetFolderURI(folderId, getter_AddRefs(folderURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString folderSpec;
  rv = folderURI->GetSpec(folderSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteFaviconAttribute(folderSpec, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  
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
nsPlacesImportExportService::WriteTitle(nsINavHistoryResultNode* aItem, nsIOutputStream* aOutput)
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
    aOutput->Write(kDescriptionClose, sizeof(kDescriptionClose)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}





nsresult
nsPlacesImportExportService::WriteItem(nsINavHistoryResultNode* aItem,
                          const nsACString& aIndent,
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

  
  
  rv = aOutput->Write(kHrefAttribute, sizeof(kHrefAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString uri;
  rv = aItem->GetUri(uri);
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

  
  nsCOMPtr<nsIURI> pageURI;
  rv = NS_NewURI(getter_AddRefs(pageURI), uri, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRBool hasPostData;
  rv = mAnnotationService->PageHasAnnotation(pageURI, POST_DATA_ANNO,
                                             &hasPostData);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasPostData) {
    nsAutoString postData;
    rv = mAnnotationService->GetPageAnnotationString(pageURI, POST_DATA_ANNO,
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

  
  nsCOMPtr<nsIMicrosummary> microsummary;
  rv = mMicrosummaryService->GetMicrosummary(itemId, getter_AddRefs(microsummary));
  NS_ENSURE_SUCCESS(rv, rv);
  if (microsummary) {
    nsCOMPtr<nsIMicrosummaryGenerator> generator;
    rv = microsummary->GetGenerator(getter_AddRefs(generator));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIURI> genURI;
    rv = generator->GetUri(getter_AddRefs(genURI));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCAutoString spec;
    rv = genURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aOutput->Write(kMicsumGenURIAttribute, sizeof(kMicsumGenURIAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = WriteEscapedUrl(spec, aOutput);
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

  
  PRInt64 itemId;
  rv = aItem->GetItemId(&itemId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  nsAutoString title;
  rv = mBookmarksService->GetItemTitle(itemId, title);
  if (NS_SUCCEEDED(rv) && !title.IsEmpty()) {
    rv = aOutput->Write(kNameAttribute, strlen(kNameAttribute), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);

    char* escapedTitle = nsEscapeHTML(NS_ConvertUTF16toUTF8(title).get());
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
nsPlacesImportExportService::WriteContainerContents(nsINavHistoryResultNode* aFolder, const nsACString& aIndent,
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

  PRInt64 placesRoot;
  rv = mBookmarksService->GetPlacesRoot(&placesRoot);
  NS_ENSURE_SUCCESS(rv,rv);

  PRInt64 bookmarksRoot;
  rv = mBookmarksService->GetBookmarksRoot(&bookmarksRoot);
  NS_ENSURE_SUCCESS(rv,rv);

  PRInt64 toolbarFolder;
  rv = mBookmarksService->GetToolbarFolder(&toolbarFolder);
  NS_ENSURE_SUCCESS(rv,rv);

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
      if (folderId == placesRoot && (childFolderId == toolbarFolder ||
                                    childFolderId == bookmarksRoot)) {
        
        
        
        
        continue;
      }

      
      PRBool isLivemark;
      rv = mLivemarkService->IsLivemark(childFolderId, &isLivemark);
      NS_ENSURE_SUCCESS(rv, rv);

      if (isLivemark)
        rv = WriteLivemark(child, myIndent, aOutput);
      else
        rv = WriteContainer(child, myIndent, aOutput);
    } else if (type == nsINavHistoryResultNode::RESULT_TYPE_SEPARATOR) {
      rv = WriteSeparator(child, myIndent, aOutput);
    } else {
      rv = WriteItem(child, myIndent, aOutput);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}




NS_IMETHODIMP
nsPlacesImportExportService::ImportHTMLFromFile(nsILocalFile* aFile, PRBool aIsInitialImport)
{
  
  return ImportHTMLFromFileInternal(aFile, PR_FALSE, 0, aIsInitialImport);
}



NS_IMETHODIMP
nsPlacesImportExportService::ImportHTMLFromFileToFolder(nsILocalFile* aFile, PRInt64 aFolderId, PRBool aIsInitialImport)
{
  
  return ImportHTMLFromFileInternal(aFile, PR_FALSE, aFolderId, aIsInitialImport);
}

nsresult
nsPlacesImportExportService::ImportHTMLFromFileInternal(nsILocalFile* aFile,
                                       PRBool aAllowRootChanges,
                                       PRInt64 aFolder,
                                       PRBool aIsImportDefaults)
{
  nsresult rv = EnsureServiceState();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> file(do_QueryInterface(aFile));
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

  nsCOMPtr<nsIParser> parser = do_CreateInstance(kParserCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<BookmarkContentSink> sink = new BookmarkContentSink;
  NS_ENSURE_TRUE(sink, NS_ERROR_OUT_OF_MEMORY);
  rv = sink->Init(aAllowRootChanges, mBookmarksService, aFolder, aIsImportDefaults);
  NS_ENSURE_SUCCESS(rv, rv);
  parser->SetContentSink(sink);

  
  
  nsCOMPtr<nsIIOService> ioservice = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> fileURI;
  rv = ioservice->NewFileURI(file, getter_AddRefs(fileURI));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ioservice->NewChannelFromURI(fileURI, getter_AddRefs(mImportChannel));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mImportChannel->SetContentType(NS_LITERAL_CSTRING("text/html"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = parser->Parse(fileURI, nsnull);
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
    PRInt64 bookmarksRoot;
    rv = mBookmarksService->GetBookmarksRoot(&bookmarksRoot);
    NS_ENSURE_SUCCESS(rv,rv);

    rv = mBookmarksService->RemoveFolderChildren(bookmarksRoot);
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
  rv = SyncChannelStatus(mImportChannel, rv);
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
    rv = SyncChannelStatus(mImportChannel, rv);
    if (NS_FAILED(rv))
      break;
  }
  listener->OnStopRequest(mImportChannel, nsnull, rv);
  return NS_OK;
}



NS_IMETHODIMP
nsPlacesImportExportService::ExportHTMLToFile(nsILocalFile* aBookmarksFile)
{
  if (!aBookmarksFile)
    return NS_ERROR_NULL_POINTER;

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
                                        0600,
                                       0);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIOutputStream> strm;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(strm), out, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 bookmarksRoot;
  rv = mBookmarksService->GetBookmarksRoot(&bookmarksRoot);
  NS_ENSURE_SUCCESS(rv,rv);

  
  PRUint32 dummy;
  rv = strm->Write(kFileIntro, sizeof(kFileIntro)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsINavHistoryQueryOptions> options;
  rv = mHistoryService->GetNewQueryOptions(getter_AddRefs(options));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsINavHistoryQuery> query;
  rv = mHistoryService->GetNewQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = query->SetFolders(&bookmarksRoot, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsINavHistoryResult> result;
  rv = mHistoryService->ExecuteQuery(query, options, getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsINavHistoryQueryResultNode> rootNode;
  rv = result->GetRoot(getter_AddRefs(rootNode));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = strm->Write(kRootIntro, sizeof(kRootIntro)-1, &dummy); 
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIURI> folderURI;
  rv = mBookmarksService->GetFolderURI(bookmarksRoot, getter_AddRefs(folderURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString folderSpec;
  rv = folderURI->GetSpec(folderSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteFaviconAttribute(folderSpec, strm);
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

  
  rv = WriteContainerEpilogue(EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(strm, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = safeStream->Finish();
#ifdef DEBUG_EXPORT
  printf("\nTotal time in seconds: %lld\n", (PR_Now() - startTime)/1000000);
#endif
  return rv;
}

NS_IMETHODIMP
nsPlacesImportExportService::BackupBookmarksFile()
{
  nsresult rv = EnsureServiceState();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIFile> bookmarksFileDir;
  rv = NS_GetSpecialDirectory(NS_APP_BOOKMARKS_50_FILE,
                              getter_AddRefs(bookmarksFileDir));

  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsILocalFile> bookmarksFile(do_QueryInterface(bookmarksFileDir));

  
  PRBool exists;
  rv = bookmarksFile->Exists(&exists);
  if (NS_FAILED(rv)) {
    rv = bookmarksFile->Create(nsIFile::NORMAL_FILE_TYPE, 0600);
    NS_ASSERTION(rv, "Unable to create bookmarks.html!");
    return rv;
  }

  
  rv = ExportHTMLToFile(bookmarksFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPrefService> prefServ(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrefBranch> bookmarksPrefs;
  rv = prefServ->GetBranch("browser.bookmarks.", getter_AddRefs(bookmarksPrefs));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 numberOfBackups;
  rv = bookmarksPrefs->GetIntPref("max_backups", &numberOfBackups);
  if (NS_FAILED(rv))
    numberOfBackups = 5;

  if (numberOfBackups > 0) {
    rv = ArchiveBookmarksFile(numberOfBackups, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}











nsresult
nsPlacesImportExportService::ArchiveBookmarksFile(PRInt32 numberOfBackups,
                                         PRBool forceArchive)
{
  nsCOMPtr<nsIFile> bookmarksBackupDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(bookmarksBackupDir));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsDependentCString dirName("bookmarkbackups");
  rv = bookmarksBackupDir->AppendNative(dirName);
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRBool exists;
  rv = bookmarksBackupDir->Exists(&exists);
  if (NS_FAILED(rv) || !exists) {
    rv = bookmarksBackupDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
    
    
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRTime          now64 = PR_Now();
  PRExplodedTime  nowInfo;
  PR_ExplodeTime(now64, PR_LocalTimeParameters, &nowInfo);
  PR_NormalizeTime(&nowInfo, PR_LocalTimeParameters);

  char timeString[128];
  
  PR_FormatTime(timeString, 128, "bookmarks-%Y-%m-%d.html", &nowInfo);

  
  
  nsAutoString backupFilenameString = NS_ConvertUTF8toUTF16((timeString));

  nsCOMPtr<nsIFile> backupFile;
  if (forceArchive) {
    
    nsCOMPtr<nsIFile> currentBackup;
    rv = bookmarksBackupDir->Clone(getter_AddRefs(currentBackup));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = currentBackup->Append(backupFilenameString);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = currentBackup->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists) {
      rv = currentBackup->Remove(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    nsCOMPtr<nsISimpleEnumerator> existingBackups;
    rv = bookmarksBackupDir->GetDirectoryEntries(getter_AddRefs(existingBackups));
    NS_ENSURE_SUCCESS(rv, rv);

    nsStringArray backupFileNames;

    PRBool hasMoreElements = PR_FALSE;
    PRBool hasCurrentBackup = PR_FALSE;
    
    while (NS_SUCCEEDED(existingBackups->HasMoreElements(&hasMoreElements)) &&
           hasMoreElements)
    {
      rv = existingBackups->GetNext(getter_AddRefs(backupFile));
      NS_ENSURE_SUCCESS(rv, rv);
      nsAutoString backupName;
      rv = backupFile->GetLeafName(backupName);
      NS_ENSURE_SUCCESS(rv, rv);
      
      
      if (backupName == backupFilenameString) {
        hasCurrentBackup = PR_TRUE;
        continue;
      }

      
      if (Substring(backupName, 0, 10) == NS_LITERAL_STRING("bookmarks-"))
        backupFileNames.AppendString(backupName);
    }

    if (numberOfBackups > 0 && backupFileNames.Count() >= numberOfBackups) {
      PRInt32 numberOfBackupsToDelete = backupFileNames.Count() - numberOfBackups + 1;
      backupFileNames.Sort();

      while (numberOfBackupsToDelete--) {
        (void)bookmarksBackupDir->Clone(getter_AddRefs(backupFile));
        (void)backupFile->Append(*backupFileNames[0]);
        (void)backupFile->Remove(PR_FALSE);
        backupFileNames.RemoveStringAt(0);
      }
    }

    if (hasCurrentBackup)
      return NS_OK;
  }

  nsCOMPtr<nsIFile> bookmarksFile;
  rv = NS_GetSpecialDirectory(NS_APP_BOOKMARKS_50_FILE,
                              getter_AddRefs(bookmarksFile));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = bookmarksFile->CopyTo(bookmarksBackupDir, backupFilenameString);
  
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}
