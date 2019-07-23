


























































































#include "nsToolkitCompsCID.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsEscape.h"
#include "nsFaviconService.h"
#include "nsIAnnotationService.h"
#include "nsIFile.h"
#include "nsIHTMLContentSink.h"
#include "nsILivemarkService.h"
#include "nsIParser.h"
#include "nsIServiceManager.h"
#include "nsNavBookmarks.h"
#include "nsNavHistory.h"
#include "nsAnnotationService.h"
#include "nsNavHistoryResult.h"
#include "nsNetUtil.h"
#include "nsParserCIID.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsUnicharUtils.h"
#include "mozStorageHelper.h"
#include "plbase64.h"
#include "prtime.h"

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
#define KEY_ID_LOWER "id"

#define LOAD_IN_SIDEBAR_ANNO NS_LITERAL_CSTRING("bookmarkProperties/loadInSidebar")

#define BOOKMARKS_MENU_ICON_URI "chrome://browser/skin/places/bookmarksMenu.png"
#define BOOKMARKS_TOOLBAR_ICON_URI "chrome://browser/skin/places/bookmarksToolbar.png"




static const char kWhitespace[] = " \r\n\t\b";

static nsresult WriteEscapedUrl(const nsCString& aString,
                                nsIOutputStream* aOutput);

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
    mPreviousText.Truncate(0);
  }

  
  PRInt64 mPreviousId;
};





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

  
#ifdef MOZILLA_1_8_BRANCH
  NS_IMETHOD SetTitle(const nsString& aValue) { return NS_OK; }
  NS_IMETHOD OpenHTML(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD CloseHTML() { return NS_OK; }
  NS_IMETHOD OpenHead(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD CloseHead() { return NS_OK; }
  NS_IMETHOD OpenBody(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD CloseBody() { return NS_OK; }
  NS_IMETHOD OpenForm(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD CloseForm() { return NS_OK; }
  NS_IMETHOD OpenMap(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD CloseMap() { return NS_OK; }
  NS_IMETHOD OpenFrameset(const nsIParserNode& aNode) { return NS_OK; }
  NS_IMETHOD CloseFrameset() { return NS_OK; }
  NS_IMETHOD AddHeadContent(const nsIParserNode& aNode) { return NS_OK; }
#else
  NS_IMETHOD OpenHead() { return NS_OK; }
#endif
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
  if (! mFrames.AppendElement(BookmarkImportFrame(menuRoot)))
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
    if (! frame.mPreviousText.IsEmpty()) {
      
      
      
      frame.mPreviousText.Truncate(0);
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
      } else if (node.GetKeyAt(i).LowerCaseEqualsLiteral(KEY_BOOKMARKSMENU_LOWER)) {
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
  CurFrame().mPreviousText.Truncate(0);
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

  
  
  if (! feedUrl.IsEmpty()) {
    NS_NewURI(getter_AddRefs(frame.mPreviousFeed),
              NS_ConvertUTF16toUTF8(feedUrl), nsnull);
  }

  
  if (href.IsEmpty()) {
    frame.mPreviousLink = nsnull;

    
    
    if (! frame.mPreviousFeed)
      return;
  } else {
    
    
    
    nsresult rv = NS_NewURI(getter_AddRefs(frame.mPreviousLink),
                   href, nsnull);
    if (NS_FAILED(rv) && ! frame.mPreviousFeed) {
      frame.mPreviousLink = nsnull;
      return; 
    }
  }

  
  frame.mPreviousId = ConvertImportedIdToInternalId(NS_ConvertUTF16toUTF8(id));

  
  
  if (frame.mPreviousFeed)
    return;

  
  if (frame.mPreviousId == 0) {
    
    rv = mBookmarksService->InsertItem(frame.mContainerID, frame.mPreviousLink,
                                       mBookmarksService->DEFAULT_INDEX, &frame.mPreviousId);
    NS_ASSERTION(NS_SUCCEEDED(rv), "InsertItem failed");
  }

  
  if (! icon.IsEmpty() || ! iconUri.IsEmpty()) {
    nsCOMPtr<nsIURI> iconUriObject;
    NS_NewURI(getter_AddRefs(iconUriObject), iconUri);
    if (! icon.IsEmpty() || iconUriObject)
      SetFaviconForURI(frame.mPreviousLink, iconUriObject,
                       NS_ConvertUTF16toUTF8(icon));
  }

  
  if (! keyword.IsEmpty())
    mBookmarksService->SetKeywordForBookmark(frame.mPreviousId, keyword);

  if (webPanel.LowerCaseEqualsLiteral("true")) {
    

    nsCOMPtr<nsIURI> placeURI;
    rv = mBookmarksService->GetItemURI(frame.mPreviousId,
                                       getter_AddRefs(placeURI));
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "failed to get a place: uri for a new bookmark");
    if (NS_SUCCEEDED(rv)) {
      mAnnotationService->SetAnnotationInt32(placeURI, LOAD_IN_SIDEBAR_ANNO,
                                             1, 0,
                                             nsIAnnotationService::EXPIRE_NEVER);
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
  frame.mPreviousText.Truncate(0);
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
          rv = SetFaviconForFolder(ourID, NS_LITERAL_CSTRING(BOOKMARKS_MENU_ICON_URI));
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
          
          rv = SetFaviconForFolder(ourID, NS_LITERAL_CSTRING(BOOKMARKS_TOOLBAR_ICON_URI));
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

  if (! mFrames.AppendElement(BookmarkImportFrame(ourID)))
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

  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  if (! faviconService)
    return NS_ERROR_NO_INTERFACE;

  
  
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
  if (! buffer)
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
  return faviconService->SetFaviconUrlForPage(aPageURI, faviconURI);
}









nsresult
BookmarkContentSink::SetFaviconForFolder(PRInt64 aFolder,
                                         const nsACString& aFavicon)
{
  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIURI> folderURI;
  nsresult rv = mBookmarksService->GetFolderURI(aFolder,
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
  if (aId.IsEmpty() || nsCRT::strncasecmp("rdf:", aId.get(), 4) == 0)
    return intId;
  PRInt32 rv;
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




NS_IMETHODIMP
nsNavBookmarks::ImportBookmarksHTML(nsIURI* aURL)
{
  
  return ImportBookmarksHTMLInternal(aURL, PR_FALSE, 0, PR_FALSE);
}

NS_IMETHODIMP
nsNavBookmarks::ImportBookmarksHTMLToFolder(nsIURI* aURL, PRInt64 aFolder)
{
  
  return ImportBookmarksHTMLInternal(aURL, PR_FALSE, aFolder, PR_FALSE);
}

nsresult
nsNavBookmarks::ImportBookmarksHTMLInternal(nsIURI* aURL,
                                            PRBool aAllowRootChanges,
                                            PRInt64 aFolder,
                                            PRBool aIsImportDefaults)
{
  
  mozStorageTransaction transaction(DBConn(), PR_FALSE);

#ifdef DEBUG_IMPORT
  nsCAutoString spec;
  aURL->GetSpec(spec);
  printf("\nImporting %s\n", spec.get());
#endif

  nsresult rv;
  nsCOMPtr<nsIParser> parser = do_CreateInstance(kParserCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<BookmarkContentSink> sink = new BookmarkContentSink;
  NS_ENSURE_TRUE(sink, NS_ERROR_OUT_OF_MEMORY);
  rv = sink->Init(aAllowRootChanges, this, aFolder, aIsImportDefaults);
  NS_ENSURE_SUCCESS(rv, rv);
  parser->SetContentSink(sink);

  
  
  nsCOMPtr<nsIIOService> ioservice = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIChannel> channel;
  rv = ioservice->NewChannelFromURI(aURL, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = channel->SetContentType(NS_LITERAL_CSTRING("text/html"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIInputStream> stream;
  rv = channel->Open(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIInputStream> bufferedstream;
  rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedstream), stream, 4096);
  NS_ENSURE_SUCCESS(rv, rv);

  
#ifdef MOZILLA_1_8_BRANCH
  rv = parser->Parse(aURL, nsnull, PR_FALSE);
#else
  rv = parser->Parse(aURL, nsnull);
#endif
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
    if (! available)
      break; 

    rv = listener->OnDataAvailable(channel, nsnull, bufferedstream, 0, available);
    rv = SyncChannelStatus(channel, rv);
    if (NS_FAILED(rv))
      break;
  }
  listener->OnStopRequest(channel, nsnull, rv);
  transaction.Commit();
  return NS_OK;
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
static const char kBookmarksRootAttribute[] = " BOOKMARKS_MENU=\"true\"";
static const char kToolbarFolderAttribute[] = " PERSONAL_TOOLBAR_FOLDER=\"true\"";
static const char kIconAttribute[] = " ICON=\"";
static const char kIconURIAttribute[] = " ICON_URI=\"";
static const char kHrefAttribute[] = " HREF=\"";
static const char kFeedURIAttribute[] = " FEEDURL=\"";
static const char kWebPanelAttribute[] = " WEB_PANEL=\"true\"";
static const char kKeywordAttribute[] = " SHORTCUTURL=\"";
static const char kIdAttribute[] = " ID=\"";







static nsresult
WriteContainerPrologue(const nsCString& aIndent, nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv = aOutput->Write(aIndent.get(), aIndent.Length(), &dummy);
  if (NS_FAILED(rv)) return rv;
  rv = aOutput->Write(kBookmarkIntro, sizeof(kBookmarkIntro)-1, &dummy);
  if (NS_FAILED(rv)) return rv;
  return NS_OK;
}








static nsresult
WriteContainerEpilogue(const nsCString& aIndent, nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv = aOutput->Write(aIndent.get(), aIndent.Length(), &dummy);
  if (NS_FAILED(rv)) return rv;
  rv = aOutput->Write(kBookmarkClose, sizeof(kBookmarkClose)-1, &dummy);
  if (NS_FAILED(rv)) return rv;
  return NS_OK;
}




static nsresult
DataToDataURI(PRUint8* aData, PRUint32 aDataLen, const nsACString& aMimeType,
              nsACString& aDataURI)
{
  char* encoded = PL_Base64Encode(NS_REINTERPRET_CAST(const char*, aData),
                                  aDataLen, nsnull);
  if (! encoded)
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

  nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(faviconService, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aURI);
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
  if (NS_FAILED(rv)) return rv;
  rv = WriteEscapedUrl(faviconSpec, aOutput);
  if (NS_FAILED(rv)) return rv;
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  if (! faviconScheme.EqualsLiteral("chrome")) {
    

    
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
      if (NS_FAILED(rv)) return rv;
      rv = aOutput->Write(faviconContents.get(), faviconContents.Length(), &dummy);
      if (NS_FAILED(rv)) return rv;
      rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
      if (NS_FAILED(rv)) return rv;
    }
  }
  return NS_OK;
}






nsresult
nsNavBookmarks::WriteContainer(PRInt64 aFolder, const nsCString& aIndent,
                               nsIOutputStream* aOutput)
{
  nsresult rv = WriteContainerHeader(aFolder, aIndent, aOutput);
  if (NS_FAILED(rv)) return rv;

  

  rv = WriteContainerPrologue(aIndent, aOutput);
  if (NS_FAILED(rv)) return rv;
  rv = WriteContainerContents(aFolder, aIndent, aOutput);
  if (NS_FAILED(rv)) return rv;
  rv = WriteContainerEpilogue(aIndent, aOutput);
  if (NS_FAILED(rv)) return rv;
  return NS_OK;
}







nsresult
nsNavBookmarks::WriteContainerHeader(PRInt64 aFolder, const nsCString& aIndent,
                                     nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  
  if (! aIndent.IsEmpty()) {
    rv = aOutput->Write(aIndent.get(), aIndent.Length(), &dummy);
    if (NS_FAILED(rv)) return rv;
  }

  
  rv = aOutput->Write(kContainerIntro, sizeof(kContainerIntro)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  if (aFolder == mRoot) {
    rv = aOutput->Write(kPlacesRootAttribute, sizeof(kPlacesRootAttribute)-1, &dummy);
    if (NS_FAILED(rv)) return rv;
  } else if (aFolder == mBookmarksRoot) {
    rv = aOutput->Write(kBookmarksRootAttribute, sizeof(kBookmarksRootAttribute)-1, &dummy);
    if (NS_FAILED(rv)) return rv;
  } else if (aFolder == mToolbarFolder) {
    rv = aOutput->Write(kToolbarFolderAttribute, sizeof(kToolbarFolderAttribute)-1, &dummy);
    if (NS_FAILED(rv)) return rv;
  }

  
  rv = aOutput->Write(kIdAttribute, sizeof(kIdAttribute)-1, &dummy);
  if (NS_FAILED(rv)) return rv;
  nsCAutoString id;
  id.AppendInt(aFolder);
  rv = aOutput->Write(id.get(), id.Length(), &dummy);
  if (NS_FAILED(rv)) return rv;
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIURI> folderURI;
  rv = GetFolderURI(aFolder, getter_AddRefs(folderURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString folderSpec;
  rv = folderURI->GetSpec(folderSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteFaviconAttribute(folderSpec, aOutput);
  if (NS_FAILED(rv)) return rv;

  
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  rv = WriteContainerTitle(aFolder, aOutput);
  if (NS_FAILED(rv)) return rv;

  
  rv = aOutput->Write(kContainerClose, sizeof(kContainerClose)-1, &dummy);
  return rv;
}






nsresult
nsNavBookmarks::WriteContainerTitle(PRInt64 aFolder, nsIOutputStream* aOutput)
{
  nsAutoString title;
  nsresult rv = GetFolderTitle(aFolder, title);
  NS_ENSURE_SUCCESS(rv, rv);

  char* escapedTitle = nsEscapeHTML(NS_ConvertUTF16toUTF8(title).get());
  if (escapedTitle) {
    PRUint32 dummy;
    rv = aOutput->Write(escapedTitle, strlen(escapedTitle), &dummy);
    nsMemory::Free(escapedTitle);
    if (NS_FAILED(rv)) return rv;
  }
  return NS_OK;
}






nsresult
nsNavBookmarks::WriteItem(nsNavHistoryResultNode* aItem,
                          const nsCString& aIndent,
                          nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  
  if (! aIndent.IsEmpty()) {
    rv = aOutput->Write(aIndent.get(), aIndent.Length(), &dummy);
    if (NS_FAILED(rv)) return rv;
  }

  
  rv = aOutput->Write(kItemOpen, sizeof(kItemOpen)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  
  rv = aOutput->Write(kHrefAttribute, sizeof(kHrefAttribute)-1, &dummy);
  if (NS_FAILED(rv)) return rv;
  nsCAutoString uri;
  rv = aItem->GetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteEscapedUrl(uri, aOutput);
  if (NS_FAILED(rv)) return rv;
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  rv = WriteFaviconAttribute(uri, aOutput);
  if (NS_FAILED(rv)) return rv;

  
  PRInt64 bookmarkId;
  rv = aItem->GetBookmarkId(&bookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kIdAttribute, sizeof(kIdAttribute)-1, &dummy);
  if (NS_FAILED(rv)) return rv;
  nsCAutoString id;
  id.AppendInt(bookmarkId);
  rv = aOutput->Write(id.get(), id.Length(), &dummy);
  if (NS_FAILED(rv)) return rv;
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  nsAutoString keyword;
  rv = GetKeywordForBookmark(bookmarkId, keyword);
  if (NS_FAILED(rv)) return rv;
  if (!keyword.IsEmpty()) {
    rv = aOutput->Write(kKeywordAttribute, sizeof(kKeywordAttribute)-1, &dummy);
    if (NS_FAILED(rv)) return rv;
    char* escapedKeyword = nsEscapeHTML(NS_ConvertUTF16toUTF8(keyword).get());
    rv = aOutput->Write(escapedKeyword, strlen(escapedKeyword), &dummy);
    nsMemory::Free(escapedKeyword);
    if (NS_FAILED(rv)) return rv;
    rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
    if (NS_FAILED(rv)) return rv;
  }

  
  nsCOMPtr<nsIURI> placeURI;
  rv = GetItemURI(bookmarkId, getter_AddRefs(placeURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);

  
  
  PRBool loadInSidebar = PR_FALSE;
  rv = annosvc->HasAnnotation(placeURI, LOAD_IN_SIDEBAR_ANNO, &loadInSidebar);
  NS_ENSURE_SUCCESS(rv, rv);
  if (loadInSidebar)
    aOutput->Write(kWebPanelAttribute, sizeof(kWebPanelAttribute)-1, &dummy);

  

  
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  char* escapedTitle = nsEscapeHTML(aItem->mTitle.get());
  if (escapedTitle) {
    rv = aOutput->Write(escapedTitle, strlen(escapedTitle), &dummy);
    nsMemory::Free(escapedTitle);
    if (NS_FAILED(rv)) return rv;
  }

  
  rv = aOutput->Write(kItemClose, sizeof(kItemClose)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  

  return NS_OK;
}







nsresult
nsNavBookmarks::WriteLivemark(PRInt64 aFolderId, const nsCString& aIndent,
                              nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  nsCOMPtr<nsILivemarkService> livemarkService(do_GetService(NS_LIVEMARKSERVICE_CONTRACTID));
  NS_ENSURE_TRUE(livemarkService, NS_ERROR_UNEXPECTED);

  
  if (! aIndent.IsEmpty()) {
    rv = aOutput->Write(aIndent.get(), aIndent.Length(), &dummy);
    if (NS_FAILED(rv)) return rv;
  }

  
  rv = aOutput->Write(kItemOpen, sizeof(kItemOpen)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIURI> feedURI;
  rv = livemarkService->GetFeedURI(aFolderId, getter_AddRefs(feedURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCString feedSpec;
  rv = feedURI->GetSpec(feedSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = aOutput->Write(kFeedURIAttribute, sizeof(kFeedURIAttribute)-1, &dummy);
  if (NS_FAILED(rv)) return rv;
  rv = WriteEscapedUrl(feedSpec, aOutput);
  if (NS_FAILED(rv)) return rv;
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIURI> siteURI;
  rv = livemarkService->GetSiteURI(aFolderId, getter_AddRefs(siteURI));
  NS_ENSURE_SUCCESS(rv, rv);
  if (siteURI) {
    nsCString siteSpec;
    rv = siteURI->GetSpec(siteSpec);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aOutput->Write(kHrefAttribute, sizeof(kHrefAttribute)-1, &dummy);
    if (NS_FAILED(rv)) return rv;
    rv = WriteEscapedUrl(siteSpec, aOutput);
    if (NS_FAILED(rv)) return rv;
    rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
    if (NS_FAILED(rv)) return rv;
  }

  
  rv = aOutput->Write(kIdAttribute, sizeof(kIdAttribute)-1, &dummy);
  if (NS_FAILED(rv)) return rv;
  nsCAutoString id;
  id.AppendInt(aFolderId);
  rv = aOutput->Write(id.get(), id.Length(), &dummy);
  if (NS_FAILED(rv)) return rv;
  rv = aOutput->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  rv = aOutput->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  rv = WriteContainerTitle(aFolderId, aOutput);
  if (NS_FAILED(rv)) return rv;

  
  rv = aOutput->Write(kItemClose, sizeof(kItemClose)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  return NS_OK;
}






nsresult
WriteSeparator(const nsCString& aIndent, nsIOutputStream* aOutput)
{
  PRUint32 dummy;
  nsresult rv;

  
  if (! aIndent.IsEmpty()) {
    rv = aOutput->Write(aIndent.get(), aIndent.Length(), &dummy);
    if (NS_FAILED(rv)) return rv;
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
nsNavBookmarks::WriteContainerContents(PRInt64 aFolder, const nsCString& aIndent,
                                       nsIOutputStream* aOutput)
{
  nsCString myIndent = aIndent;
  myIndent.Append(kIndent);

  nsNavHistory* historyService = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(historyService, NS_ERROR_UNEXPECTED);

  
  nsCOMPtr<nsINavHistoryQueryOptions> optionsInterface;
  nsresult rv = historyService->GetNewQueryOptions(getter_AddRefs(optionsInterface));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsNavHistoryQueryOptions> options = do_QueryInterface(optionsInterface);
  NS_ENSURE_TRUE(options, NS_ERROR_UNEXPECTED);

  
  nsCOMArray<nsNavHistoryResultNode> items;
  rv = QueryFolderChildren(aFolder, options, &items);
  NS_ENSURE_SUCCESS(rv, rv);

  
  for (PRInt32 i = 0; i < items.Count(); i ++) {
    if (items[i]->IsFolder()) {
      
      PRInt64 folderId = items[i]->GetAsFolder()->mFolderId;
      if (aFolder == mRoot && (folderId == mToolbarFolder ||
                               folderId == mBookmarksRoot)) {
        
        
        
        
        continue;
      }

      
      nsCOMPtr<nsILivemarkService> livemarkService(do_GetService(NS_LIVEMARKSERVICE_CONTRACTID));
      NS_ENSURE_TRUE(livemarkService, NS_ERROR_UNEXPECTED);
      PRBool isLivemark;
      rv = livemarkService->IsLivemark(folderId, &isLivemark);
      NS_ENSURE_SUCCESS(rv, rv);

      if (isLivemark)
        rv = WriteLivemark(folderId, myIndent, aOutput);
      else
        rv = WriteContainer(folderId, myIndent, aOutput);
    } else if (items[i]->IsSeparator()) {
      rv = WriteSeparator(myIndent, aOutput);
    } else {
      rv = WriteItem(items[i], myIndent, aOutput);
    }
    if (NS_FAILED(rv)) return rv;
  }
  return NS_OK;
}





NS_IMETHODIMP
nsNavBookmarks::ExportBookmarksHTML(nsIFile* aBookmarksFile)
{
  if (! aBookmarksFile)
    return NS_ERROR_NULL_POINTER;

  
  
  nsCOMPtr<nsIOutputStream> out;
  nsresult rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(out),
                                                aBookmarksFile,
                                                PR_WRONLY | PR_CREATE_FILE,
                                                 0600,
                                                0);
  if (NS_FAILED(rv)) return rv;

  
  
  nsCOMPtr<nsIOutputStream> strm;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(strm), out, 4096);
  if (NS_FAILED(rv)) return rv;

  
  PRUint32 dummy;
  rv = strm->Write(kFileIntro, sizeof(kFileIntro)-1, &dummy);
  if (NS_FAILED(rv)) return rv;

  
  rv = strm->Write(kRootIntro, sizeof(kRootIntro)-1, &dummy); 
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsIURI> folderURI;
  rv = GetFolderURI(mBookmarksRoot, getter_AddRefs(folderURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString folderSpec;
  rv = folderURI->GetSpec(folderSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = WriteFaviconAttribute(folderSpec, strm);
  if (NS_FAILED(rv)) return rv;

  
  rv = strm->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy); 
  if (NS_FAILED(rv)) return rv;
  rv = WriteContainerTitle(mBookmarksRoot, strm);
  if (NS_FAILED(rv)) return rv;
  rv = strm->Write(kCloseRootH1, sizeof(kCloseRootH1)-1, &dummy); 
  if (NS_FAILED(rv)) return rv;

  
  rv = WriteContainerPrologue(EmptyCString(), strm);
  if (NS_FAILED(rv)) return rv;

  
  nsCAutoString indent;
  indent.Assign(kIndent);

  
  rv = WriteContainer(mRoot, indent, strm);
  if (NS_FAILED(rv)) return rv;

  
  rv = WriteContainerContents(mBookmarksRoot, EmptyCString(), strm);
  if (NS_FAILED(rv)) return rv;

  
  rv = WriteContainerEpilogue(EmptyCString(), strm);
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(strm, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return safeStream->Finish();
}
