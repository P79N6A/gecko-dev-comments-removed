



























































































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

static NS_DEFINE_CID(kParserCID, NS_PARSER_CID);

#define KEY_TOOLBARFOLDER_LOWER "personal_toolbar_folder"
#define KEY_BOOKMARKSSMENU_LOWER "bookmarks_menu"
#define KEY_PLACESROOT_LOWER "places_root"
#define KEY_HREF_LOWER "href"
#define KEY_FEEDURL_LOWER "feedurl"
#define KEY_WEB_PANEL_LOWER "web_panel"
#define KEY_LASTCHARSET_LOWER "last_charset"
#define KEY_ICON_LOWER "icon"
#define KEY_ICON_URI_LOWER "icon_uri"
#define KEY_SHORTCUTURL_LOWER "shortcuturl"
#define KEY_ID_LOWER "id"

#define LOAD_IN_SIDEBAR_ANNO NS_LITERAL_CSTRING("bookmarkProperties/loadInSidebar")

#define BOOKMARKSS_MENU_ICON_URI "chrome://browser/skin/places/bookmarksMenu.png"
#define BOOKMARKSS_TOOLBAR_ICON_URI "chrome://browser/skin/places/bookmarksToolbar.png"




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
      mInDescription(PR_FALSE)
  {
  }

  enum ContainerType { Container_Normal,
                       Container_Places,
                       Container_Menu,
                       Container_Toolbar };

  PRInt64 mContainerID;

  
  
  
  
  
  
  
  PRInt32 mContainerNesting;

  
  
  
  ContainerType mLastContainerType;

  
  PRInt64 mLastContainerId;

  
  
  
  nsString mPreviousText;

  
  
  
  
  
  
  
  
  
  
  
  
  
  PRBool mInDescription;

  
  
  
  
  nsCOMPtr<nsIURI> mPreviousLink;

  
  
  nsCOMPtr<nsIURI> mPreviousFeed;

  void ConsumeHeading(nsAString* aHeading, ContainerType* aContainerType, PRInt64* aContainerId)
  {
    *aHeading = mPreviousText;
    *aContainerType = mLastContainerType;
    *aContainerId = mLastContainerId;
    mPreviousText.Truncate();
  }

  
  PRInt64 mPreviousId;
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

NS_IMPL_ISUPPORTS1(nsPlacesImportExportService, nsIPlacesImportExportService)

nsPlacesImportExportService::nsPlacesImportExportService()
  : mPlacesRoot(0), mBookmarksRoot(0), mToolbarFolder(0)
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

  mBookmarksService->GetPlacesRoot(&mPlacesRoot);
  mBookmarksService->GetBookmarksRoot(&mBookmarksRoot);
  mBookmarksService->GetToolbarFolder(&mToolbarFolder);
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
  void HandleSeparator();

  
  
  
  
  nsTArray<BookmarkImportFrame> mFrames;
  BookmarkImportFrame& CurFrame()
  {
    NS_ASSERTION(mFrames.Length() > 0, "Asking for frame when there are none!");
    return mFrames[mFrames.Length() - 1];
  }
  nsresult NewFrame();
  nsresult PopFrame();

  nsresult SetFaviconForURI(nsIURI* aPageURI, nsIURI* aFaviconURI,
                            const nsCString& aData);
  nsresult SetFaviconForFolder(PRInt64 aFolder, const nsACString& aFavicon);

  PRInt64 ConvertImportedIdToInternalId(const nsCString& aId);

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
  
  
  
  BookmarkImportFrame& frame = CurFrame();
  if (frame.mInDescription) {
    frame.mPreviousText.Trim(kWhitespace); 
    if (!frame.mPreviousText.IsEmpty()) {
      
      
      
      frame.mPreviousText.Truncate();
    }
    frame.mInDescription = PR_FALSE;
  }

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
  }
  return NS_OK;
}

NS_IMETHODIMP
BookmarkContentSink::CloseContainer(const nsHTMLTag aTag)
{
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
    HandleSeparator();
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
      PRInt64 mPlacesRoot;
      mBookmarksService->GetPlacesRoot(&mPlacesRoot);
      CurFrame().mContainerID = mPlacesRoot;
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
  frame.mLastContainerId = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (frame.mContainerNesting == 0)
    PopFrame();

  
  
  
  PRInt32 attrCount = node.GetAttributeCount();
  frame.mLastContainerType = BookmarkImportFrame::Container_Normal;
  if (!mFolderSpecified) {
    for (PRInt32 i = 0; i < attrCount; i ++) {
      if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_TOOLBARFOLDER_LOWER)) {
        frame.mLastContainerType = BookmarkImportFrame::Container_Toolbar;
        break;
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_BOOKMARKSSMENU_LOWER)) {
        frame.mLastContainerType = BookmarkImportFrame::Container_Menu;
        break;
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_PLACESROOT_LOWER)) {
        frame.mLastContainerType = BookmarkImportFrame::Container_Places;
        break;
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_ID_LOWER)) {
        frame.mLastContainerId =
          ConvertImportedIdToInternalId(NS_ConvertUTF16toUTF8(node.GetValueAt(i)));
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
  nsAutoString webPanel;
  nsAutoString id;
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
    } else if (key.LowerCaseEqualsLiteral(KEY_WEB_PANEL_LOWER)) {
      webPanel = node.GetValueAt(i);
    } else if (key.LowerCaseEqualsLiteral(KEY_ID_LOWER)) {
      id = node.GetValueAt(i);
    }
  }
  href.Trim(kWhitespace);
  feedUrl.Trim(kWhitespace);
  icon.Trim(kWhitespace);
  iconUri.Trim(kWhitespace);
  lastCharset.Trim(kWhitespace);
  keyword.Trim(kWhitespace);
  webPanel.Trim(kWhitespace);
  id.Trim(kWhitespace);

  
  
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

  
  frame.mPreviousId = ConvertImportedIdToInternalId(NS_ConvertUTF16toUTF8(id));

  
  
  if (frame.mPreviousFeed)
    return;

  
  if (frame.mPreviousId > 0) {
    PRInt32 index;
    rv = mBookmarksService->GetItemIndex(frame.mPreviousId, &index);
    if (NS_FAILED(rv))
      frame.mPreviousId = 0;
  }

  
  if (frame.mPreviousId == 0) {
    
    rv = mBookmarksService->InsertItem(frame.mContainerID, frame.mPreviousLink,
                                       mBookmarksService->DEFAULT_INDEX, &frame.mPreviousId);
    NS_ASSERTION(NS_SUCCEEDED(rv), "InsertItem failed");
  }

  
  if (!icon.IsEmpty() || !iconUri.IsEmpty()) {
    nsCOMPtr<nsIURI> iconUriObject;
    NS_NewURI(getter_AddRefs(iconUriObject), iconUri);
    if (!icon.IsEmpty() || iconUriObject) {
      rv = SetFaviconForURI(frame.mPreviousLink, iconUriObject,
                            NS_ConvertUTF16toUTF8(icon));
    }
  }

  
  if (!keyword.IsEmpty())
    mBookmarksService->SetKeywordForBookmark(frame.mPreviousId, keyword);

  if (webPanel.LowerCaseEqualsLiteral("true")) {
    

    mAnnotationService->SetItemAnnotationInt32(frame.mPreviousId, LOAD_IN_SIDEBAR_ANNO,
                                               1, 0,
                                               nsIAnnotationService::EXPIRE_NEVER);
  }
  
}








void
BookmarkContentSink::HandleLinkEnd()
{
  nsresult rv;
  BookmarkImportFrame& frame = CurFrame();
  frame.mPreviousText.Trim(kWhitespace);
  if (frame.mPreviousFeed) {
    
    
    
    PRInt64 folderId;

    if (frame.mPreviousId > 0) {
      
      rv = mLivemarkService->SetSiteURI(frame.mPreviousId, frame.mPreviousLink);
      NS_ASSERTION(NS_SUCCEEDED(rv), "SetSiteURI failed!");
      rv = mLivemarkService->SetFeedURI(frame.mPreviousId, frame.mPreviousFeed);
      NS_ASSERTION(NS_SUCCEEDED(rv), "SetFeedURI failed!");
      rv = mBookmarksService->SetFolderTitle(frame.mPreviousId, frame.mPreviousText);
      NS_ASSERTION(NS_SUCCEEDED(rv), "SetFolderTitle failed!");
    } else {
      if (mIsImportDefaults) {
        rv = mLivemarkService->CreateLivemarkFolderOnly(mBookmarksService,
                                                   frame.mContainerID,
                                                   frame.mPreviousText,
                                                   frame.mPreviousLink,
                                                   frame.mPreviousFeed,
                                                   -1,
                                                   &folderId);
        NS_ASSERTION(NS_SUCCEEDED(rv), "CreateLivemarkFolderOnly failed!");
      } else {
        rv = mLivemarkService->CreateLivemark(frame.mContainerID,
                                         frame.mPreviousText,
                                         frame.mPreviousLink,
                                         frame.mPreviousFeed,
                                         -1,
                                         &folderId);
        NS_ASSERTION(NS_SUCCEEDED(rv), "CreateLivemark failed!");
      }
    }
#ifdef DEBUG_IMPORT
    PrintNesting();
    printf("Creating livemark '%s'\n",
           NS_ConvertUTF16toUTF8(frame.mPreviousText).get());
#endif
  }
  else if (frame.mPreviousLink) {
#ifdef DEBUG_IMPORT
    PrintNesting();
    printf("Creating bookmark '%s'\n",
           NS_ConvertUTF16toUTF8(frame.mPreviousText).get());
#endif
    mBookmarksService->SetItemTitle(frame.mPreviousId, frame.mPreviousText);
  }
  frame.mPreviousText.Truncate();
}





void
BookmarkContentSink::HandleSeparator()
{
  BookmarkImportFrame& frame = CurFrame();

  
  
  
  if (frame.mLastContainerType != BookmarkImportFrame::Container_Toolbar) {
    
#ifdef DEBUG_IMPORT
    PrintNesting();
    printf("--------\n");
#endif
    mBookmarksService->InsertSeparator(frame.mContainerID,
                                       mBookmarksService->DEFAULT_INDEX);
  }
}







nsresult
BookmarkContentSink::NewFrame()
{
  nsresult rv;

  PRInt64 ourID = 0;
  nsString containerName;
  BookmarkImportFrame::ContainerType containerType;
  CurFrame().ConsumeHeading(&containerName, &containerType, &ourID);

  PRBool updateFolder = PR_FALSE;
  if (ourID == 0) {
    switch (containerType) {
      case BookmarkImportFrame::Container_Normal:
        
        rv = mBookmarksService->GetChildFolder(CurFrame().mContainerID,
                                               containerName, &ourID);
        NS_ENSURE_SUCCESS(rv, rv);
        if (ourID == 0) {
          
          rv = mBookmarksService->CreateFolder(CurFrame().mContainerID,
                                              containerName,
                                              mBookmarksService->DEFAULT_INDEX, &ourID);
          NS_ENSURE_SUCCESS(rv, rv);
        }
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
          rv = SetFaviconForFolder(ourID, NS_LITERAL_CSTRING(BOOKMARKSS_MENU_ICON_URI));
          NS_ENSURE_SUCCESS(rv, rv);
        }
        break;
      case BookmarkImportFrame::Container_Toolbar:
        
        PRInt64 bookmarkToolbarFolder;
        rv = mBookmarksService->GetToolbarFolder(&bookmarkToolbarFolder);
        NS_ENSURE_SUCCESS(rv, rv);
        if (!bookmarkToolbarFolder) {
          
          rv = mBookmarksService->CreateFolder(CurFrame().mContainerID,
                                              containerName,
                                              mBookmarksService->DEFAULT_INDEX, &ourID);
          NS_ENSURE_SUCCESS(rv, rv);
          
          rv = mBookmarksService->SetToolbarFolder(ourID);
          NS_ENSURE_SUCCESS(rv, rv);
          
          rv = SetFaviconForFolder(ourID, NS_LITERAL_CSTRING(BOOKMARKSS_TOOLBAR_ICON_URI));
          NS_ENSURE_SUCCESS(rv, rv);
        }
        else {
          ourID = bookmarkToolbarFolder;
        }
        break;
      default:
        NS_NOTREACHED("Unknown container type");
    }
  }
#ifdef DEBUG_IMPORT
  PrintNesting();
  printf("Folder %lld \'%s\'", ourID, NS_ConvertUTF16toUTF8(containerName).get());
#endif

  if (updateFolder) {
    
    mBookmarksService->MoveFolder(ourID, CurFrame().mContainerID, -1);
    mBookmarksService->SetFolderTitle(ourID, containerName);
#ifdef DEBUG_IMPORT
    printf(" [reparenting]");
#endif
  }

#ifdef DEBUG_IMPORT
  printf("\n");
#endif

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
    faviconSpec.AppendInt(PR_Now());
    rv = NS_NewURI(getter_AddRefs(faviconURI), faviconSpec);
    NS_ENSURE_SUCCESS(rv, rv);
    serialNumber ++;
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

  
  PRUint8* buffer = NS_STATIC_CAST(PRUint8*,
                                   nsMemory::Alloc(sizeof(PRUint8) * available));
  if (!buffer)
    return NS_ERROR_OUT_OF_MEMORY;
  PRUint32 numRead;
  rv = stream->Read(NS_REINTERPRET_CAST(char*, buffer), available, &numRead);
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
  if (aId.IsEmpty() || Substring(aId, 0, 4).Equals(NS_LITERAL_CSTRING("rdf:"), CaseInsensitiveCompare))
    return intId;
  nsresult rv;
  intId = aId.ToInteger(&rv);
  if (NS_FAILED(rv))
    intId = 0;
  return intId;
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
static const char kSeparator[] = "<HR>" NS_LINEBREAK;
static const char kQuoteStr[] = "\"";
static const char kCloseAngle[] = ">";
static const char kIndent[] = "    ";

static const char kPlacesRootAttribute[] = " PLACES_ROOT=\"true\"";
static const char kBookmarksRootAttribute[] = " BOOKMARKSS_MENU=\"true\"";
static const char kToolbarFolderAttribute[] = " PERSONAL_TOOLBAR_FOLDER=\"true\"";
static const char kIconAttribute[] = " ICON=\"";
static const char kIconURIAttribute[] = " ICON_URI=\"";
static const char kHrefAttribute[] = " HREF=\"";
static const char kFeedURIAttribute[] = " FEEDURL=\"";
static const char kWebPanelAttribute[] = " WEB_PANEL=\"true\"";
static const char kKeywordAttribute[] = " SHORTCUTURL=\"";
static const char kIdAttribute[] = " ID=\"";







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
  char* encoded = PL_Base64Encode(NS_REINTERPRET_CAST(const char*, aData),
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






nsresult
nsPlacesImportExportService::WriteContainer(PRInt64 aFolder, const nsACString& aIndent,
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
nsPlacesImportExportService::WriteContainerHeader(PRInt64 aFolder, const nsACString& aIndent,
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

  
  if (aFolder == mPlacesRoot) {
    rv = aOutput->Write(kPlacesRootAttribute, sizeof(kPlacesRootAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (aFolder == mBookmarksRoot) {
    rv = aOutput->Write(kBookmarksRootAttribute, sizeof(kBookmarksRootAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (aFolder == mToolbarFolder) {
    rv = aOutput->Write(kToolbarFolderAttribute, sizeof(kToolbarFolderAttribute)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = aOutput->Write(kIdAttribute, sizeof(kIdAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString id;
  id.AppendInt(aFolder);
  rv = aOutput->Write(id.get(), id.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIURI> folderURI;
  rv = mBookmarksService->GetFolderURI(aFolder, getter_AddRefs(folderURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString folderSpec;
  rv = folderURI->GetSpec(folderSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteFaviconAttribute(folderSpec, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteContainerTitle(aFolder, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kContainerClose, sizeof(kContainerClose)-1, &dummy);
  return rv;
}






nsresult
nsPlacesImportExportService::WriteContainerTitle(PRInt64 aFolder, nsIOutputStream* aOutput)
{
  nsAutoString title;
  nsresult rv;
  
  rv = mBookmarksService->GetFolderTitle(aFolder, title);
  NS_ENSURE_SUCCESS(rv, rv);

  char* escapedTitle = nsEscapeHTML(NS_ConvertUTF16toUTF8(title).get());
  if (escapedTitle) {
    PRUint32 dummy;
    rv = aOutput->Write(escapedTitle, strlen(escapedTitle), &dummy);
    nsMemory::Free(escapedTitle);
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

  
  rv = WriteFaviconAttribute(uri, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 itemId;
  rv = aItem->GetItemId(&itemId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kIdAttribute, sizeof(kIdAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString id;
  id.AppendInt(itemId);
  rv = aOutput->Write(id.get(), id.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
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

  
  
  PRBool loadInSidebar = PR_FALSE;
  rv = mAnnotationService->ItemHasAnnotation(itemId, LOAD_IN_SIDEBAR_ANNO,
                                             &loadInSidebar);
  NS_ENSURE_SUCCESS(rv, rv);
  if (loadInSidebar)
    aOutput->Write(kWebPanelAttribute, sizeof(kWebPanelAttribute)-1, &dummy);

  

  
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

  

  return NS_OK;
}







nsresult
nsPlacesImportExportService::WriteLivemark(PRInt64 aFolderId, const nsACString& aIndent,
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

  
  nsCOMPtr<nsIURI> feedURI;
  rv = mLivemarkService->GetFeedURI(aFolderId, getter_AddRefs(feedURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCString feedSpec;
  rv = feedURI->GetSpec(feedSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kFeedURIAttribute, sizeof(kFeedURIAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteEscapedUrl(feedSpec, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIURI> siteURI;
  rv = mLivemarkService->GetSiteURI(aFolderId, getter_AddRefs(siteURI));
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

  
  rv = aOutput->Write(kIdAttribute, sizeof(kIdAttribute)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString id;
  id.AppendInt(aFolderId);
  rv = aOutput->Write(id.get(), id.Length(), &dummy);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteContainerTitle(aFolderId, aOutput);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kItemClose, sizeof(kItemClose)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}






nsresult
WriteSeparator(const nsCString& aIndent, nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  
  if (!aIndent.IsEmpty()) {
    rv = aOutput->Write(aIndent.get(), aIndent.Length(), &dummy);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aOutput->Write(kSeparator, sizeof(kSeparator)-1, &dummy);
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
nsPlacesImportExportService::WriteContainerContents(PRInt64 aFolder, const nsACString& aIndent,
                                       nsIOutputStream* aOutput)
{
  nsCAutoString myIndent(aIndent);
  myIndent.Append(kIndent);

  
  nsresult rv;
  nsCOMPtr<nsINavHistoryQueryOptions> optionsInterface;
  rv = mHistoryService->GetNewQueryOptions(getter_AddRefs(optionsInterface));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsINavHistoryQueryOptions> options = do_QueryInterface(optionsInterface);
  NS_ENSURE_TRUE(options, NS_ERROR_UNEXPECTED);

  
  nsCOMPtr<nsINavHistoryQuery> query;
  rv = mHistoryService->GetNewQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = query->SetFolders(&aFolder, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = query->SetOnlyBookmarked(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  const PRUint16 groupMode = nsINavHistoryQueryOptions::GROUP_BY_FOLDER;
  rv = options->SetGroupingMode(&groupMode, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsINavHistoryResult> result;
  rv = mHistoryService->ExecuteQuery(query, options, getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsINavHistoryQueryResultNode> rootNode;
  rv = result->GetRoot(getter_AddRefs(rootNode));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = rootNode->SetContainerOpen(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 childCount = 0;
  rootNode->GetChildCount(&childCount);
  for (PRUint32 i = 0; i < childCount; ++i) {
    nsCOMPtr<nsINavHistoryResultNode> child;
    rv = rootNode->GetChild(i, getter_AddRefs(child));
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 type = 0;
    rv = child->GetType(&type);
    NS_ENSURE_SUCCESS(rv, rv);
    if (type == nsINavHistoryResultNode::RESULT_TYPE_FOLDER) {
      
      nsCOMPtr<nsINavHistoryFolderResultNode> folderNode = do_QueryInterface(child);
      PRInt64 folderId;
      rv = folderNode->GetFolderId(&folderId);
      NS_ENSURE_SUCCESS(rv, rv);
      if (aFolder == mPlacesRoot && (folderId == mToolbarFolder ||
                               folderId == mBookmarksRoot)) {
        
        
        
        
        continue;
      }

      
      PRBool isLivemark;
      rv = mLivemarkService->IsLivemark(folderId, &isLivemark);
      NS_ENSURE_SUCCESS(rv, rv);

      if (isLivemark)
        rv = WriteLivemark(folderId, myIndent, aOutput);
      else
        rv = WriteContainer(folderId, myIndent, aOutput);
    } else if (type == nsINavHistoryResultNode::RESULT_TYPE_SEPARATOR) {
      rv = WriteSeparator(myIndent, aOutput);
    } else {
      rv = WriteItem(child, myIndent, aOutput);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}



NS_IMETHODIMP
nsPlacesImportExportService::ImportHTMLFromFile(nsILocalFile* aFile)
{
  
  return ImportHTMLFromFileInternal(aFile, PR_FALSE, 0, PR_FALSE);
}



NS_IMETHODIMP
nsPlacesImportExportService::ImportHTMLFromFileToFolder(nsILocalFile* aFile, PRInt64 aFolderId)
{
  
  return ImportHTMLFromFileInternal(aFile, PR_FALSE, aFolderId, PR_FALSE);
}

nsresult
nsPlacesImportExportService::ImportHTMLFromFileInternal(nsILocalFile* aFile,
                                       PRBool aAllowRootChanges,
                                       PRInt64 aFolder,
                                       PRBool aIsImportDefaults)
{
  nsCOMPtr<nsIFile> file(do_QueryInterface(aFile));
#ifdef DEBUG_IMPORT
  nsAutoString path;
  file->GetPath(path);
  printf("\nImporting %s\n", NS_ConvertUTF16toUTF8(path).get());
#endif

  
  nsresult rv;
  mBookmarksService->BeginUpdateBatch();

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
  nsCOMPtr<nsIChannel> channel;
  rv = ioservice->NewChannelFromURI(fileURI, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = channel->SetContentType(NS_LITERAL_CSTRING("text/html"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIInputStream> stream;
  rv = channel->Open(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIInputStream> bufferedstream;
  rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedstream), stream, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = parser->Parse(fileURI, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsCOMPtr<nsIStreamListener> listener = do_QueryInterface(parser, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = listener->OnStartRequest(channel, nsnull);
  rv = SyncChannelStatus(channel, rv);
  while(NS_SUCCEEDED(rv))
  {
    PRUint32 available;
    rv = bufferedstream->Available(&available);
    if (rv == NS_BASE_STREAM_CLOSED) {
      rv = NS_OK;
      available = 0;
    }
    if (NS_FAILED(rv)) {
      channel->Cancel(rv);
      break;
    }
    if (!available)
      break; 

    rv = listener->OnDataAvailable(channel, nsnull, bufferedstream, 0, available);
    rv = SyncChannelStatus(channel, rv);
    if (NS_FAILED(rv))
      break;
  }
  listener->OnStopRequest(channel, nsnull, rv);
  
  mBookmarksService->EndUpdateBatch();
  return NS_OK;
}



NS_IMETHODIMP
nsPlacesImportExportService::ExportHTMLToFile(nsILocalFile* aBookmarksFile)
{
  if (!aBookmarksFile)
    return NS_ERROR_NULL_POINTER;

  
  
  nsCOMPtr<nsIOutputStream> out;
  nsresult rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(out),
                                                aBookmarksFile,
                                                PR_WRONLY | PR_CREATE_FILE,
                                                 0600,
                                                0);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIOutputStream> strm;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(strm), out, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRUint32 dummy;
  rv = strm->Write(kFileIntro, sizeof(kFileIntro)-1, &dummy);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = strm->Write(kRootIntro, sizeof(kRootIntro)-1, &dummy); 
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIURI> folderURI;
  rv = mBookmarksService->GetFolderURI(mBookmarksRoot, getter_AddRefs(folderURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString folderSpec;
  rv = folderURI->GetSpec(folderSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteFaviconAttribute(folderSpec, strm);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = strm->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy); 
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteContainerTitle(mBookmarksRoot, strm);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = strm->Write(kCloseRootH1, sizeof(kCloseRootH1)-1, &dummy); 
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteContainerPrologue(EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString indent;
  indent.Assign(kIndent);

  
  rv = WriteContainer(mPlacesRoot, indent, strm);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteContainerContents(mBookmarksRoot, EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = WriteContainerEpilogue(EmptyCString(), strm);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(strm, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return safeStream->Finish();
}

NS_IMETHODIMP
nsPlacesImportExportService::BackupBookmarksFile()
{
  
  nsCOMPtr<nsIFile> bookmarksFileDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_BOOKMARKS_50_FILE,
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
