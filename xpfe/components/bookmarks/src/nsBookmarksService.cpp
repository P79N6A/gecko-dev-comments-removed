















































#include "nsBookmarksService.h"
#include "nsIDOMWindow.h"
#include "nsIObserverService.h"
#include "nsIProfile.h"
#include "nsIRDFContainer.h"
#include "nsIRDFContainerUtils.h"
#include "nsIRDFService.h"
#include "nsIRDFXMLSerializer.h"
#include "nsIRDFXMLSource.h"
#include "nsRDFCID.h"
#include "nsISupportsPrimitives.h"
#include "rdf.h"
#include "nsCRT.h"
#include "nsEnumeratorUtils.h"
#include "nsEscape.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceDefs.h"
#include "nsUnicharUtils.h"

#include "nsISound.h"
#include "nsIPrompt.h"
#include "nsIWindowWatcher.h"
#include "nsWidgetsCID.h"

#include "nsIURL.h"
#include "nsIFileURL.h"
#include "nsIFile.h"
#include "nsIGenericFactory.h"
#include "nsIInputStream.h"
#include "nsILineInputStream.h"
#include "nsIOutputStream.h"
#include "nsNetUtil.h"
#include "nsICacheEntryDescriptor.h"

#include "nsICharsetConverterManager.h"
#include "nsICharsetAlias.h"
#include "nsIPlatformCharset.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"

#include "nsIWebNavigation.h"


#include "nsCollationCID.h"
#include "nsILocaleService.h"
#include "nsICollation.h"
#include "nsVoidArray.h"
#include "nsUnicharUtils.h"
#include "nsAutoBuffer.h"


#ifdef XP_WIN
#include <shlobj.h>
#include <intshcut.h>
#ifdef CompareString
#undef CompareString
#endif
#endif

nsIRDFResource      *kNC_IEFavoritesRoot;
nsIRDFResource      *kNC_SystemBookmarksStaticRoot;
nsIRDFResource      *kNC_Bookmark;
nsIRDFResource      *kNC_BookmarkSeparator;
nsIRDFResource      *kNC_BookmarkAddDate;
nsIRDFResource      *kNC_BookmarksTopRoot;
nsIRDFResource      *kNC_BookmarksRoot;
nsIRDFResource      *kNC_Description;
nsIRDFResource      *kNC_Folder;
nsIRDFResource      *kNC_FolderType;
nsIRDFResource      *kNC_FolderGroup;
nsIRDFResource      *kNC_IEFavorite;
nsIRDFResource      *kNC_IEFavoriteFolder;
nsIRDFResource      *kNC_Name;
nsIRDFResource      *kNC_Icon;
nsIRDFResource      *kNC_NewBookmarkFolder;
nsIRDFResource      *kNC_NewSearchFolder;
nsIRDFResource      *kNC_PersonalToolbarFolder;
nsIRDFResource      *kNC_ShortcutURL;
nsIRDFResource      *kNC_URL;
nsIRDFResource      *kRDF_type;
nsIRDFResource      *kRDF_nextVal;
nsIRDFResource      *kWEB_LastModifiedDate;
nsIRDFResource      *kWEB_LastVisitDate;
nsIRDFResource      *kWEB_Schedule;
nsIRDFResource      *kWEB_ScheduleActive;
nsIRDFResource      *kWEB_Status;
nsIRDFResource      *kWEB_LastPingDate;
nsIRDFResource      *kWEB_LastPingETag;
nsIRDFResource      *kWEB_LastPingModDate;
nsIRDFResource      *kWEB_LastCharset;
nsIRDFResource      *kWEB_LastPingContentLen;
nsIRDFLiteral       *kTrueLiteral;
nsIRDFLiteral       *kEmptyLiteral;
nsIRDFDate          *kEmptyDate;
nsIRDFResource      *kNC_Parent;
nsIRDFResource      *kNC_BookmarkCommand_NewBookmark;
nsIRDFResource      *kNC_BookmarkCommand_NewFolder;
nsIRDFResource      *kNC_BookmarkCommand_NewSeparator;
nsIRDFResource      *kNC_BookmarkCommand_DeleteBookmark;
nsIRDFResource      *kNC_BookmarkCommand_DeleteBookmarkFolder;
nsIRDFResource      *kNC_BookmarkCommand_DeleteBookmarkSeparator;
nsIRDFResource      *kNC_BookmarkCommand_SetNewBookmarkFolder = nsnull;
nsIRDFResource      *kNC_BookmarkCommand_SetPersonalToolbarFolder;
nsIRDFResource      *kNC_BookmarkCommand_SetNewSearchFolder;
nsIRDFResource      *kNC_BookmarkCommand_Import;
nsIRDFResource      *kNC_BookmarkCommand_Export;

#define BOOKMARK_TIMEOUT        15000       // fire every 15 seconds




static NS_DEFINE_CID(kRDFInMemoryDataSourceCID,   NS_RDFINMEMORYDATASOURCE_CID);
static NS_DEFINE_CID(kRDFServiceCID,              NS_RDFSERVICE_CID);
static NS_DEFINE_CID(kRDFContainerCID,            NS_RDFCONTAINER_CID);
static NS_DEFINE_CID(kRDFContainerUtilsCID,       NS_RDFCONTAINERUTILS_CID);

#define URINC_BOOKMARKS_TOPROOT_STRING            "NC:BookmarksTopRoot"
#define URINC_BOOKMARKS_ROOT_STRING               "NC:BookmarksRoot"

static const char kURINC_BookmarksTopRoot[]           = URINC_BOOKMARKS_TOPROOT_STRING; 
static const char kURINC_BookmarksRoot[]              = URINC_BOOKMARKS_ROOT_STRING; 
static const char kURINC_IEFavoritesRoot[]            = "NC:IEFavoritesRoot"; 
static const char kURINC_SystemBookmarksStaticRoot[]  = "NC:SystemBookmarksStaticRoot"; 
static const char kURINC_NewBookmarkFolder[]          = "NC:NewBookmarkFolder"; 
static const char kURINC_PersonalToolbarFolder[]      = "NC:PersonalToolbarFolder"; 
static const char kURINC_NewSearchFolder[]            = "NC:NewSearchFolder"; 
static const char kBookmarkCommand[]                  = "http://home.netscape.com/NC-rdf#command?";

#define bookmark_properties NS_LITERAL_CSTRING("chrome://communicator/locale/bookmarks/bookmarks.properties")



PRInt32               gRefCnt=0;
nsIRDFService        *gRDF;
nsIRDFContainerUtils *gRDFC;
nsICharsetAlias      *gCharsetAlias;
nsICollation         *gCollation;
PRBool                gImportedSystemBookmarks = PR_FALSE;

static nsresult
bm_AddRefGlobals()
{
    if (gRefCnt++ == 0)
    {
        nsresult rv;
        rv = CallGetService(kRDFServiceCID, &gRDF);
        if (NS_FAILED(rv)) {
            NS_ERROR("unable to get RDF service");
            return rv;
        }

        rv = CallGetService(kRDFContainerUtilsCID, &gRDFC);
        if (NS_FAILED(rv)) {
            NS_ERROR("unable to get RDF container utils");
            return rv;
        }

        rv = CallGetService(NS_CHARSETALIAS_CONTRACTID, &gCharsetAlias);
        if (NS_FAILED(rv)) {
            NS_ERROR("unable to get charset alias service");
            return rv;
        }

        nsCOMPtr<nsILocaleService> ls = do_GetService(NS_LOCALESERVICE_CONTRACTID);
        if (ls) {
            nsCOMPtr<nsILocale> locale;
            ls->GetApplicationLocale(getter_AddRefs(locale));
            if (locale) {
                nsCOMPtr<nsICollationFactory> factory = do_CreateInstance(NS_COLLATIONFACTORY_CONTRACTID);
                if (factory) {
                    factory->CreateCollation(locale, &gCollation);
                }
            }
        }

        gRDF->GetResource(NS_LITERAL_CSTRING(kURINC_BookmarksTopRoot),
                          &kNC_BookmarksTopRoot);
        gRDF->GetResource(NS_LITERAL_CSTRING(kURINC_BookmarksRoot),
                          &kNC_BookmarksRoot);
        gRDF->GetResource(NS_LITERAL_CSTRING(kURINC_IEFavoritesRoot),
                          &kNC_IEFavoritesRoot);
        gRDF->GetResource(NS_LITERAL_CSTRING(kURINC_SystemBookmarksStaticRoot),
                          &kNC_SystemBookmarksStaticRoot);
        gRDF->GetResource(NS_LITERAL_CSTRING(kURINC_NewBookmarkFolder),
                          &kNC_NewBookmarkFolder);
        gRDF->GetResource(NS_LITERAL_CSTRING(kURINC_PersonalToolbarFolder),
                          &kNC_PersonalToolbarFolder);
        gRDF->GetResource(NS_LITERAL_CSTRING(kURINC_NewSearchFolder),
                          &kNC_NewSearchFolder);

        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Bookmark"),
                          &kNC_Bookmark);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "BookmarkSeparator"),
                          &kNC_BookmarkSeparator);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "BookmarkAddDate"),
                          &kNC_BookmarkAddDate);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Description"),
                          &kNC_Description);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Folder"),
                          &kNC_Folder);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "FolderType"),
                          &kNC_FolderType);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "FolderGroup"),
                          &kNC_FolderGroup);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "IEFavorite"),
                          &kNC_IEFavorite);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "IEFavoriteFolder"),
                          &kNC_IEFavoriteFolder);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Name"),
                          &kNC_Name);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "Icon"),
                          &kNC_Icon);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "ShortcutURL"),
                          &kNC_ShortcutURL);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "URL"),
                          &kNC_URL);
        gRDF->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "type"),
                          &kRDF_type);
        gRDF->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "nextVal"),
                          &kRDF_nextVal);

        gRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastModifiedDate"),
                          &kWEB_LastModifiedDate);
        gRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastVisitDate"),
                          &kWEB_LastVisitDate);
        gRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastCharset"),
                          &kWEB_LastCharset);

        gRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "Schedule"),
                          &kWEB_Schedule);
        gRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "ScheduleFlag"),
                          &kWEB_ScheduleActive);
        gRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "status"),
                          &kWEB_Status);
        gRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastPingDate"),
                          &kWEB_LastPingDate);
        gRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastPingETag"),
                          &kWEB_LastPingETag);
        gRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastPingModDate"),
                          &kWEB_LastPingModDate);
        gRDF->GetResource(NS_LITERAL_CSTRING(WEB_NAMESPACE_URI "LastPingContentLen"),
                          &kWEB_LastPingContentLen);

        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "parent"), &kNC_Parent);

        gRDF->GetLiteral(NS_LITERAL_STRING("true").get(), &kTrueLiteral);

        gRDF->GetLiteral(EmptyString().get(), &kEmptyLiteral);

        gRDF->GetDateLiteral(0, &kEmptyDate);

        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=newbookmark"),
                          &kNC_BookmarkCommand_NewBookmark);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=newfolder"),
                          &kNC_BookmarkCommand_NewFolder);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=newseparator"),
                          &kNC_BookmarkCommand_NewSeparator);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=deletebookmark"),
                          &kNC_BookmarkCommand_DeleteBookmark);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=deletebookmarkfolder"),
                          &kNC_BookmarkCommand_DeleteBookmarkFolder);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=deletebookmarkseparator"),
                          &kNC_BookmarkCommand_DeleteBookmarkSeparator);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=setnewbookmarkfolder"),
                          &kNC_BookmarkCommand_SetNewBookmarkFolder);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=setpersonaltoolbarfolder"),
                          &kNC_BookmarkCommand_SetPersonalToolbarFolder);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=setnewsearchfolder"),
                          &kNC_BookmarkCommand_SetNewSearchFolder);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=import"),
                          &kNC_BookmarkCommand_Import);
        gRDF->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "command?cmd=export"),
                          &kNC_BookmarkCommand_Export);
    }
    return NS_OK;
}



static void
bm_ReleaseGlobals()
{
    if (--gRefCnt == 0)
    {
        NS_IF_RELEASE(gRDF);
        NS_IF_RELEASE(gRDFC);
        NS_IF_RELEASE(gCharsetAlias);

        NS_IF_RELEASE(gCollation);

        NS_IF_RELEASE(kNC_Bookmark);
        NS_IF_RELEASE(kNC_BookmarkSeparator);
        NS_IF_RELEASE(kNC_BookmarkAddDate);
        NS_IF_RELEASE(kNC_BookmarksTopRoot);
        NS_IF_RELEASE(kNC_BookmarksRoot);
        NS_IF_RELEASE(kNC_Description);
        NS_IF_RELEASE(kNC_Folder);
        NS_IF_RELEASE(kNC_FolderType);
        NS_IF_RELEASE(kNC_FolderGroup);
        NS_IF_RELEASE(kNC_IEFavorite);
        NS_IF_RELEASE(kNC_IEFavoriteFolder);
        NS_IF_RELEASE(kNC_IEFavoritesRoot);
        NS_IF_RELEASE(kNC_SystemBookmarksStaticRoot);
        NS_IF_RELEASE(kNC_Name);
        NS_IF_RELEASE(kNC_Icon);
        NS_IF_RELEASE(kNC_NewBookmarkFolder);
        NS_IF_RELEASE(kNC_NewSearchFolder);
        NS_IF_RELEASE(kNC_PersonalToolbarFolder);
        NS_IF_RELEASE(kNC_ShortcutURL);
        NS_IF_RELEASE(kNC_URL);
        NS_IF_RELEASE(kRDF_type);
        NS_IF_RELEASE(kRDF_nextVal);
        NS_IF_RELEASE(kWEB_LastModifiedDate);
        NS_IF_RELEASE(kWEB_LastVisitDate);
        NS_IF_RELEASE(kNC_Parent);
        NS_IF_RELEASE(kTrueLiteral);
        NS_IF_RELEASE(kEmptyLiteral);
        NS_IF_RELEASE(kEmptyDate);
        NS_IF_RELEASE(kWEB_Schedule);
        NS_IF_RELEASE(kWEB_ScheduleActive);
        NS_IF_RELEASE(kWEB_Status);
        NS_IF_RELEASE(kWEB_LastPingDate);
        NS_IF_RELEASE(kWEB_LastPingETag);
        NS_IF_RELEASE(kWEB_LastPingModDate);
        NS_IF_RELEASE(kWEB_LastPingContentLen);
        NS_IF_RELEASE(kWEB_LastCharset);
        NS_IF_RELEASE(kNC_BookmarkCommand_NewBookmark);
        NS_IF_RELEASE(kNC_BookmarkCommand_NewFolder);
        NS_IF_RELEASE(kNC_BookmarkCommand_NewSeparator);
        NS_IF_RELEASE(kNC_BookmarkCommand_DeleteBookmark);
        NS_IF_RELEASE(kNC_BookmarkCommand_DeleteBookmarkFolder);
        NS_IF_RELEASE(kNC_BookmarkCommand_DeleteBookmarkSeparator);
        NS_IF_RELEASE(kNC_BookmarkCommand_SetNewBookmarkFolder);
        NS_IF_RELEASE(kNC_BookmarkCommand_SetPersonalToolbarFolder);
        NS_IF_RELEASE(kNC_BookmarkCommand_SetNewSearchFolder);
        NS_IF_RELEASE(kNC_BookmarkCommand_Import);
        NS_IF_RELEASE(kNC_BookmarkCommand_Export);
    }
}







class BookmarkParser {
private:
    nsCOMPtr<nsIUnicodeDecoder> mUnicodeDecoder;
    nsIRDFDataSource*           mDataSource;
    nsCString                   mIEFavoritesRoot;
    PRBool                      mFoundIEFavoritesRoot;
    PRBool                      mFoundPersonalToolbarFolder;
    PRBool                      mIsImportOperation;
    char*                       mContents;
    PRUint32                    mContentsLen;
    PRInt32                     mStartOffset;
    nsCOMPtr<nsIInputStream>    mInputStream;

    friend  class nsBookmarksService;

protected:
    struct BookmarkField
    {
        const char      *mName;
        const char      *mPropertyName;
        nsIRDFResource  *mProperty;
        nsresult        (*mParse)(nsIRDFResource *arc, nsString& aValue, nsIRDFNode** aResult);
        nsIRDFNode      *mValue;
    };

    static BookmarkField gBookmarkFieldTable[];
    static BookmarkField gBookmarkHeaderFieldTable[];

    nsresult AssertTime(nsIRDFResource* aSource,
                        nsIRDFResource* aLabel,
                        PRInt32 aTime);

    nsresult setFolderHint(nsIRDFResource *newSource, nsIRDFResource *objType);

    nsresult Unescape(nsString &text);

    nsresult ParseMetaTag(const nsString &aLine, nsIUnicodeDecoder **decoder);

    nsresult ParseBookmarkInfo(BookmarkField *fields, PRBool isBookmarkFlag, const nsString &aLine,
                               const nsCOMPtr<nsIRDFContainer> &aContainer,
                               nsIRDFResource *nodeType, nsCOMPtr<nsIRDFResource> &bookmarkNode);

    nsresult ParseBookmarkSeparator(const nsString &aLine,
                                    const nsCOMPtr<nsIRDFContainer> &aContainer);

    nsresult ParseHeaderBegin(const nsString &aLine,
                              const nsCOMPtr<nsIRDFContainer> &aContainer);

    nsresult ParseHeaderEnd(const nsString &aLine);

    PRInt32 getEOL(const char *whole, PRInt32 startOffset, PRInt32 totalLength);

    nsresult updateAtom(nsIRDFDataSource *db, nsIRDFResource *src,
                        nsIRDFResource *prop, nsIRDFNode *newValue, PRBool *dirtyFlag);

    static    nsresult ParseResource(nsIRDFResource *arc, nsString& aValue, nsIRDFNode** aResult);
    static    nsresult ParseLiteral(nsIRDFResource *arc, nsString& aValue, nsIRDFNode** aResult);
    static    nsresult ParseDate(nsIRDFResource *arc, nsString& aValue, nsIRDFNode** aResult);

public:
    BookmarkParser();
    ~BookmarkParser();

    nsresult Init(nsIFile *aFile, nsIRDFDataSource *aDataSource, 
                  PRBool aIsImportOperation = PR_FALSE);
    nsresult DecodeBuffer(nsString &line, char *buf, PRUint32 aLength);
    nsresult ProcessLine(nsIRDFContainer *aContainer, nsIRDFResource *nodeType,
                         nsCOMPtr<nsIRDFResource> &bookmarkNode, const nsString &line,
                         nsString &description, PRBool &inDescription, PRBool &isActiveFlag);
    nsresult Parse(nsIRDFResource* aContainer, nsIRDFResource *nodeType);

    nsresult SetIEFavoritesRoot(const nsCString& IEFavoritesRootURL)
    {
        mIEFavoritesRoot = IEFavoritesRootURL;
        return NS_OK;
    }
    nsresult ParserFoundIEFavoritesRoot(PRBool *foundIEFavoritesRoot)
    {
        *foundIEFavoritesRoot = mFoundIEFavoritesRoot;
        return NS_OK;
    }
    nsresult ParserFoundPersonalToolbarFolder(PRBool *foundPersonalToolbarFolder)
    {
        *foundPersonalToolbarFolder = mFoundPersonalToolbarFolder;
        return NS_OK;
    }
};

BookmarkParser::BookmarkParser() :
    mContents(nsnull),
    mContentsLen(0L),
    mStartOffset(0L)
{
    bm_AddRefGlobals();
}

static const char kOpenAnchor[]    = "<A ";
static const char kCloseAnchor[]   = "</A>";

static const char kOpenHeading[]   = "<H";
static const char kCloseHeading[]  = "</H";

static const char kSeparator[]     = "<HR";

static const char kOpenUL[]        = "<UL>";
static const char kCloseUL[]       = "</UL>";

static const char kOpenMenu[]      = "<MENU>";
static const char kCloseMenu[]     = "</MENU>";

static const char kOpenDL[]        = "<DL>";
static const char kCloseDL[]       = "</DL>";

static const char kOpenDD[]        = "<DD>";

static const char kOpenMeta[]      = "<META ";

static const char kNewBookmarkFolderEquals[]      = "NEW_BOOKMARK_FOLDER=\"";
static const char kNewSearchFolderEquals[]        = "NEW_SEARCH_FOLDER=\"";
static const char kPersonalToolbarFolderEquals[]  = "PERSONAL_TOOLBAR_FOLDER=\"";

static const char kNameEquals[]            = "NAME=\"";
static const char kHREFEquals[]            = "HREF=\"";
static const char kTargetEquals[]          = "TARGET=\"";
static const char kAddDateEquals[]         = "ADD_DATE=\"";
static const char kFolderGroupEquals[]     = "FOLDER_GROUP=\"";
static const char kLastVisitEquals[]       = "LAST_VISIT=\"";
static const char kLastModifiedEquals[]    = "LAST_MODIFIED=\"";
static const char kLastCharsetEquals[]     = "LAST_CHARSET=\"";
static const char kShortcutURLEquals[]     = "SHORTCUTURL=\"";
static const char kIconEquals[]            = "ICON=\"";
static const char kScheduleEquals[]        = "SCHEDULE=\"";
static const char kLastPingEquals[]        = "LAST_PING=\"";
static const char kPingETagEquals[]        = "PING_ETAG=\"";
static const char kPingLastModEquals[]     = "PING_LAST_MODIFIED=\"";
static const char kPingContentLenEquals[]  = "PING_CONTENT_LEN=\"";
static const char kPingStatusEquals[]      = "PING_STATUS=\"";
static const char kIDEquals[]              = "ID=\"";
static const char kContentEquals[]         = "CONTENT=\"";
static const char kHTTPEquivEquals[]       = "HTTP-EQUIV=\"";
static const char kCharsetEquals[]         = "charset=";        

nsresult
BookmarkParser::Init(nsIFile *aFile, nsIRDFDataSource *aDataSource, 
                     PRBool aIsImportOperation)
{
    mDataSource = aDataSource;
    mFoundIEFavoritesRoot = PR_FALSE;
    mFoundPersonalToolbarFolder = PR_FALSE;
    mIsImportOperation = aIsImportOperation;

    nsresult rv;

    
    nsCOMPtr<nsIPlatformCharset> platformCharset = 
        do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv) && (platformCharset))
    {
        nsCAutoString    defaultCharset;
        if (NS_SUCCEEDED(rv = platformCharset->GetCharset(kPlatformCharsetSel_4xBookmarkFile, defaultCharset)))
        {
            
            nsCOMPtr<nsICharsetConverterManager> charsetConv = 
                do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
            if (NS_SUCCEEDED(rv) && (charsetConv))
            {
                rv = charsetConv->GetUnicodeDecoderRaw(defaultCharset.get(),
                                                       getter_AddRefs(mUnicodeDecoder));
            }
        }
    }

    nsCAutoString   str;
    BookmarkField   *field;
    for (field = gBookmarkFieldTable; field->mName; ++field)
    {
        str = field->mPropertyName;
        rv = gRDF->GetResource(str, &field->mProperty);
        if (NS_FAILED(rv))  return rv;
    }
    for (field = gBookmarkHeaderFieldTable; field->mName; ++field)
    {
        str = field->mPropertyName;
        rv = gRDF->GetResource(str, &field->mProperty);
        if (NS_FAILED(rv))  return rv;
    }

    if (aFile)
    {
        PRInt64 contentsLen;
        rv = aFile->GetFileSize(&contentsLen);
        NS_ENSURE_SUCCESS(rv, rv);

        if(LL_CMP(contentsLen, >, LL_INIT(0,0xFFFFFFFE)))
            return NS_ERROR_FILE_TOO_BIG;

        LL_L2UI(mContentsLen, contentsLen);

        if (mContentsLen > 0)
        {
            mContents = new char [mContentsLen + 1];
            if (mContents)
            {
                nsCOMPtr<nsIInputStream> inputStream;
                rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream),
                                                aFile,
                                                PR_RDONLY,
                                                -1, 0);

                if (NS_FAILED(rv))
                {
                    delete [] mContents;
                    mContents = nsnull;
                }
                else
                {
                    PRUint32 howMany;
                    rv = inputStream->Read(mContents, mContentsLen, &howMany);
                    if (NS_FAILED(rv))
                    {
                        delete [] mContents;
                        mContents = nsnull;
                        return NS_OK;
                    }

                    if (howMany == mContentsLen)
                    {
                        mContents[mContentsLen] = '\0';
                    }
                    else
                    {
                        delete [] mContents;
                        mContents = nsnull;
                    }
                }                    
            }
        }

        if (!mContents)
        {
            
            

            rv = NS_NewLocalFileInputStream(getter_AddRefs(mInputStream),
                                            aFile, PR_RDONLY, -1, 0);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    return NS_OK;
}

BookmarkParser::~BookmarkParser()
{
    if (mContents)
    {
        delete [] mContents;
        mContents = nsnull;
    }
    if (mInputStream)
    {
        mInputStream->Close();
    }
    BookmarkField   *field;
    for (field = gBookmarkFieldTable; field->mName; ++field)
    {
        NS_IF_RELEASE(field->mProperty);
    }
    for (field = gBookmarkHeaderFieldTable; field->mName; ++field)
    {
        NS_IF_RELEASE(field->mProperty);
    }
    bm_ReleaseGlobals();
}

PRInt32
BookmarkParser::getEOL(const char *whole, PRInt32 startOffset, PRInt32 totalLength)
{
    PRInt32 eolOffset = -1;

    while (startOffset < totalLength)
    {
        char    c;
        c = whole[startOffset];
        if ((c == '\n') || (c == '\r') || (c == '\0'))
        {
            eolOffset = startOffset;
            break;
        }
        ++startOffset;
    }
    return eolOffset;
}

nsresult
BookmarkParser::DecodeBuffer(nsString &line, char *buf, PRUint32 aLength)
{
    if (mUnicodeDecoder)
    {
        nsresult    rv;
        char        *aBuffer = buf;
        PRInt32     unicharBufLen = 0;
        mUnicodeDecoder->GetMaxLength(aBuffer, aLength, &unicharBufLen);
        
        nsAutoBuffer<PRUnichar, 256> stackBuffer;
        if (!stackBuffer.EnsureElemCapacity(unicharBufLen + 1))
          return NS_ERROR_OUT_OF_MEMORY;
        
        do
        {
            PRInt32     srcLength = aLength;
            PRInt32     unicharLength = unicharBufLen;
            PRUnichar *unichars = stackBuffer.get();
            
            rv = mUnicodeDecoder->Convert(aBuffer, &srcLength, stackBuffer.get(), &unicharLength);
            unichars[unicharLength]=0;  

            

            
            for(PRInt32 i=0;i<unicharLength-1; i++)
                if(0x0000 == unichars[i])   unichars[i] = 0x0020;
            

            line.Append(unichars, unicharLength);
            
            
            if(NS_FAILED(rv))
            {
                mUnicodeDecoder->Reset();
                line.Append( (PRUnichar)0xFFFD);
                if(((PRUint32) (srcLength + 1)) > (PRUint32)aLength)
                    srcLength = aLength;
                else 
                    srcLength++;
                aBuffer += srcLength;
                aLength -= srcLength;
            }
        } while (NS_FAILED(rv) && (aLength > 0));

    }
    else
    {
        line.AppendWithConversion(buf, aLength);
    }
    return NS_OK;
}



nsresult
BookmarkParser::ProcessLine(nsIRDFContainer *container, nsIRDFResource *nodeType,
            nsCOMPtr<nsIRDFResource> &bookmarkNode, const nsString &line,
            nsString &description, PRBool &inDescription, PRBool &isActiveFlag)
{
    nsresult    rv = NS_OK;
    PRInt32     offset;

    if (inDescription == PR_TRUE)
    {
        offset = line.FindChar('<');
        if (offset < 0)
        {
            if (!description.IsEmpty())
            {
                description.Append(PRUnichar('\n'));
            }
            description += line;
            return NS_OK;
        }

        Unescape(description);

        if (bookmarkNode)
        {
            nsCOMPtr<nsIRDFLiteral> descLiteral;
            if (NS_SUCCEEDED(rv = gRDF->GetLiteral(description.get(), getter_AddRefs(descLiteral))))
            {
                rv = mDataSource->Assert(bookmarkNode, kNC_Description, descLiteral, PR_TRUE);
            }
        }

        inDescription = PR_FALSE;
        description.Truncate();
    }

    if ((offset = line.Find(kHREFEquals, PR_TRUE)) >= 0)
    {
        rv = ParseBookmarkInfo(gBookmarkFieldTable, PR_TRUE, line, container, nodeType, bookmarkNode);
    }
    else if ((offset = line.Find(kOpenMeta, PR_TRUE)) >= 0)
    {
        rv = ParseMetaTag(line, getter_AddRefs(mUnicodeDecoder));
    }
    else if ((offset = line.Find(kOpenHeading, PR_TRUE)) >= 0 &&
         nsCRT::IsAsciiDigit(line.CharAt(offset + 2)))
    {
        
        if (line.CharAt(offset + 2) != PRUnichar('1'))
        {
            nsCOMPtr<nsIRDFResource>    dummy;
            rv = ParseBookmarkInfo(gBookmarkHeaderFieldTable, PR_FALSE, line, container, nodeType, dummy);
        }
    }
    else if ((offset = line.Find(kSeparator, PR_TRUE)) >= 0)
    {
        rv = ParseBookmarkSeparator(line, container);
    }
    else if ((offset = line.Find(kCloseUL, PR_TRUE)) >= 0 ||
         (offset = line.Find(kCloseMenu, PR_TRUE)) >= 0 ||
         (offset = line.Find(kCloseDL, PR_TRUE)) >= 0)
    {
        isActiveFlag = PR_FALSE;
        return ParseHeaderEnd(line);
    }
    else if ((offset = line.Find(kOpenUL, PR_TRUE)) >= 0 ||
         (offset = line.Find(kOpenMenu, PR_TRUE)) >= 0 ||
         (offset = line.Find(kOpenDL, PR_TRUE)) >= 0)
    {
        rv = ParseHeaderBegin(line, container);
    }
    else if ((offset = line.Find(kOpenDD, PR_TRUE)) >= 0)
    {
        inDescription = PR_TRUE;
        description = line;
        description.Cut(0, offset+sizeof(kOpenDD)-1);
    }
    else
    {
        
    }
    return rv;
}



nsresult
BookmarkParser::Parse(nsIRDFResource *aContainer, nsIRDFResource *nodeType)
{
    
    

    nsresult rv;
    nsCOMPtr<nsIRDFContainer> container =
            do_CreateInstance(kRDFContainerCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = container->Init(mDataSource, aContainer);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFResource>    bookmarkNode = aContainer;
    nsAutoString            description, line;
    PRBool              isActiveFlag = PR_TRUE, inDescriptionFlag = PR_FALSE;

    if ((mContents) && (mContentsLen > 0))
    {
        
        char                *linePtr;
        PRInt32             eol;

        while ((isActiveFlag == PR_TRUE) && (mStartOffset < (signed)mContentsLen))
        {
            linePtr = &mContents[mStartOffset];
            eol = getEOL(mContents, mStartOffset, mContentsLen);

            PRInt32 aLength;

            if ((eol >= mStartOffset) && (eol < (signed)mContentsLen))
            {
                
                aLength = eol - mStartOffset;
                mStartOffset = eol + 1;
            }
            else
            {
                aLength = mContentsLen - mStartOffset;
                mStartOffset = mContentsLen + 1;
                isActiveFlag = PR_FALSE;
            }
            if (aLength < 1)    continue;

            line.Truncate();
            DecodeBuffer(line, linePtr, aLength);

            rv = ProcessLine(container, nodeType, bookmarkNode,
                line, description, inDescriptionFlag, isActiveFlag);
            if (NS_FAILED(rv))  break;
        }
    }
    else
    {
        NS_ENSURE_TRUE(mInputStream, NS_ERROR_NULL_POINTER);

        
        
        nsCOMPtr<nsILineInputStream> lineInputStream =
            do_QueryInterface(mInputStream);
        NS_ENSURE_TRUE(lineInputStream, NS_NOINTERFACE);

        PRBool moreData = PR_TRUE;

        while(NS_SUCCEEDED(rv) && isActiveFlag && moreData)
        {
            nsCAutoString cLine;
            rv = lineInputStream->ReadLine(cLine, &moreData);

            if (NS_SUCCEEDED(rv))
            {
                CopyASCIItoUTF16(cLine, line);
                rv = ProcessLine(container, nodeType, bookmarkNode,
                    line, description, inDescriptionFlag, isActiveFlag);
            }
        }
    }
    return rv;
}

nsresult
BookmarkParser::Unescape(nsString &text)
{
    

    PRInt32     offset=0;

    while((offset = text.FindChar((PRUnichar('&')), offset)) >= 0)
    {
        if (Substring(text, offset, 4).LowerCaseEqualsLiteral("&lt;"))
        {
            text.Cut(offset, 4);
            text.Insert(PRUnichar('<'), offset);
        }
        else if (Substring(text, offset, 4).LowerCaseEqualsLiteral("&gt;"))
        {
            text.Cut(offset, 4);
            text.Insert(PRUnichar('>'), offset);
        }
        else if (Substring(text, offset, 5).LowerCaseEqualsLiteral("&amp;"))
        {
            text.Cut(offset, 5);
            text.Insert(PRUnichar('&'), offset);
        }
        else if (Substring(text, offset, 6).LowerCaseEqualsLiteral("&quot;"))
        {
            text.Cut(offset, 6);
            text.Insert(PRUnichar('\"'), offset);
        }
        else if (Substring(text, offset, 5).Equals(NS_LITERAL_STRING("&#39;")))
        {
            text.Cut(offset, 5);
            text.Insert(PRUnichar('\''), offset);
        }

        ++offset;
    }
    return NS_OK;
}

nsresult
BookmarkParser::ParseMetaTag(const nsString &aLine, nsIUnicodeDecoder **decoder)
{
    nsresult    rv = NS_OK;

    *decoder = nsnull;

    
    PRInt32 start = aLine.Find(kHTTPEquivEquals, PR_TRUE);
    NS_ASSERTION(start >= 0, "no 'HTTP-EQUIV=\"' string: how'd we get here?");
    if (start < 0)  return NS_ERROR_UNEXPECTED;
    
    start += (sizeof(kHTTPEquivEquals) - 1);
    
    PRInt32 end = aLine.FindChar(PRUnichar('"'), start);
    nsAutoString    httpEquiv;
    aLine.Mid(httpEquiv, start, end - start);

    
    if (!httpEquiv.LowerCaseEqualsLiteral("content-type"))
        return NS_OK;

    
    start = aLine.Find(kContentEquals, PR_TRUE);
    NS_ASSERTION(start >= 0, "no 'CONTENT=\"' string: how'd we get here?");
    if (start < 0)  return NS_ERROR_UNEXPECTED;
    
    start += (sizeof(kContentEquals) - 1);
    
    end = aLine.FindChar(PRUnichar('"'), start);
    nsAutoString    content;
    aLine.Mid(content, start, end - start);

    
    start = content.Find(kCharsetEquals, PR_TRUE);
    NS_ASSERTION(start >= 0, "no 'charset=' string: how'd we get here?");
    if (start < 0)  return NS_ERROR_UNEXPECTED;
    start += (sizeof(kCharsetEquals)-1);
    nsCAutoString    charset;
    charset.AssignWithConversion(Substring(content, start, content.Length() - start));
    if (charset.Length() < 1)   return NS_ERROR_UNEXPECTED;

    
    nsICharsetConverterManager *charsetConv;
    rv = CallGetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &charsetConv);
    if (NS_SUCCEEDED(rv))
    {
        rv = charsetConv->GetUnicodeDecoderRaw(charset.get(), decoder);
        NS_RELEASE(charsetConv);
    }
    return rv;
}

BookmarkParser::BookmarkField
BookmarkParser::gBookmarkFieldTable[] =
{
  
  { kIDEquals,              NC_NAMESPACE_URI  "ID",                nsnull,  BookmarkParser::ParseResource,  nsnull },
  { kHREFEquals,            NC_NAMESPACE_URI  "URL",               nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  { kAddDateEquals,         NC_NAMESPACE_URI  "BookmarkAddDate",   nsnull,  BookmarkParser::ParseDate,      nsnull },
  { kLastVisitEquals,       WEB_NAMESPACE_URI "LastVisitDate",     nsnull,  BookmarkParser::ParseDate,      nsnull },
  { kLastModifiedEquals,    WEB_NAMESPACE_URI "LastModifiedDate",  nsnull,  BookmarkParser::ParseDate,      nsnull },
  { kShortcutURLEquals,     NC_NAMESPACE_URI  "ShortcutURL",       nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  { kIconEquals,            NC_NAMESPACE_URI  "Icon",              nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  { kLastCharsetEquals,     WEB_NAMESPACE_URI "LastCharset",       nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  { kScheduleEquals,        WEB_NAMESPACE_URI "Schedule",          nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  { kLastPingEquals,        WEB_NAMESPACE_URI "LastPingDate",      nsnull,  BookmarkParser::ParseDate,      nsnull },
  { kPingETagEquals,        WEB_NAMESPACE_URI "LastPingETag",      nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  { kPingLastModEquals,     WEB_NAMESPACE_URI "LastPingModDate",   nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  { kPingContentLenEquals,  WEB_NAMESPACE_URI "LastPingContentLen",nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  { kPingStatusEquals,      WEB_NAMESPACE_URI "status",            nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  
  { nsnull,                 nsnull,                                nsnull,  nsnull,                         nsnull },
};

BookmarkParser::BookmarkField
BookmarkParser::gBookmarkHeaderFieldTable[] =
{
  
  { kIDEquals,                    NC_NAMESPACE_URI  "ID",                nsnull,  BookmarkParser::ParseResource,  nsnull },
  { kAddDateEquals,               NC_NAMESPACE_URI  "BookmarkAddDate",   nsnull,  BookmarkParser::ParseDate,      nsnull },
  { kLastModifiedEquals,          WEB_NAMESPACE_URI "LastModifiedDate",  nsnull,  BookmarkParser::ParseDate,      nsnull },
  { kFolderGroupEquals,           NC_NAMESPACE_URI  "FolderGroup",       nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  
  { kNewBookmarkFolderEquals,     kURINC_NewBookmarkFolder,              nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  { kNewSearchFolderEquals,       kURINC_NewSearchFolder,                nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  { kPersonalToolbarFolderEquals, kURINC_PersonalToolbarFolder,          nsnull,  BookmarkParser::ParseLiteral,   nsnull },
  
  { nsnull,                       nsnull,                                nsnull,  nsnull,                         nsnull },
};

nsresult
BookmarkParser::ParseBookmarkInfo(BookmarkField *fields, PRBool isBookmarkFlag,
                const nsString &aLine, const nsCOMPtr<nsIRDFContainer> &aContainer,
                nsIRDFResource *nodeType, nsCOMPtr<nsIRDFResource> &bookmarkNode)
{
    NS_PRECONDITION(aContainer != nsnull, "null ptr");
    if (! aContainer)
        return NS_ERROR_NULL_POINTER;

    bookmarkNode = nsnull;

    PRInt32     lineLen = aLine.Length();

    PRInt32     attrStart=0;
    if (isBookmarkFlag == PR_TRUE)
    {
        attrStart = aLine.Find(kOpenAnchor, PR_TRUE, attrStart);
        if (attrStart < 0)  return NS_ERROR_UNEXPECTED;
        attrStart += sizeof(kOpenAnchor)-1;
    }
    else
    {
        attrStart = aLine.Find(kOpenHeading, PR_TRUE, attrStart);
        if (attrStart < 0)  return NS_ERROR_UNEXPECTED;
        attrStart += sizeof(kOpenHeading)-1;
    }

    
    for (BookmarkField *preField = fields; preField->mName; ++preField)
    {
        NS_IF_RELEASE(preField->mValue);
    }

    
    while((attrStart < lineLen) && (aLine[attrStart] != '>'))
    {
        while(nsCRT::IsAsciiSpace(aLine[attrStart]))   ++attrStart;

        PRBool  fieldFound = PR_FALSE;

        nsAutoString id;
        id.AssignWithConversion(kIDEquals);
        for (BookmarkField *field = fields; field->mName; ++field)
        {
            nsAutoString name;
            name.AssignWithConversion(field->mName);
            if (mIsImportOperation && name.Equals(id)) 
                
                
                
                continue;
  
            if (aLine.Find(field->mName, PR_TRUE, attrStart, 1) == attrStart)
            {
                attrStart += strlen(field->mName);

                
                PRInt32 termQuote = aLine.FindChar(PRUnichar('\"'), attrStart);
                if (termQuote > attrStart)
                {
                    
                    nsAutoString    data;
                    aLine.Mid(data, attrStart, termQuote-attrStart);

                    attrStart = termQuote + 1;
                    fieldFound = PR_TRUE;

                    if (!data.IsEmpty())
                    {
                        
                        NS_ASSERTION(!field->mValue, "Field already has a value");
                        
                        NS_IF_RELEASE(field->mValue);

                        nsresult rv = (*field->mParse)(field->mProperty, data, &field->mValue);
                        if (NS_FAILED(rv)) break;
                    }
                }
                break;
            }
        }

        if (fieldFound == PR_FALSE)
        {
            
            while((attrStart < lineLen) && (aLine[attrStart] != '>') &&
                (!nsCRT::IsAsciiSpace(aLine[attrStart])))
            {
                ++attrStart;
            }
        }
    }

    nsresult    rv;

    
    nsCOMPtr<nsIRDFResource> bookmark = do_QueryInterface(fields[0].mValue);
    if (!bookmark)
    {
        
        rv = gRDF->GetAnonymousResource(getter_AddRefs(bookmark));
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create anonymous resource for folder");
    }
    else if (bookmark.get() == kNC_PersonalToolbarFolder)
    {
        mFoundPersonalToolbarFolder = PR_TRUE;
    }

    if (bookmark)
    {
        const char* bookmarkURI;
        bookmark->GetValueConst(&bookmarkURI);

        bookmarkNode = bookmark;

        
        PRBool isIEFavoriteRoot = PR_FALSE;
        if (!mIEFavoritesRoot.IsEmpty())
        {
            if (!nsCRT::strcmp(mIEFavoritesRoot.get(), bookmarkURI))
            {
                mFoundIEFavoritesRoot = PR_TRUE;
                isIEFavoriteRoot = PR_TRUE;
            }
        }
        if ((isIEFavoriteRoot == PR_TRUE) || ((nodeType == kNC_IEFavorite) && (isBookmarkFlag == PR_FALSE)))
        {
            rv = mDataSource->Assert(bookmark, kRDF_type, kNC_IEFavoriteFolder, PR_TRUE);
        }
        else if (nodeType == kNC_IEFavorite ||
                 nodeType == kNC_IEFavoriteFolder ||
                 nodeType == kNC_BookmarkSeparator)
        {
            rv = mDataSource->Assert(bookmark, kRDF_type, nodeType, PR_TRUE);
        }

        
        for (BookmarkField *field = fields; field->mName; ++field)
        {
            if (field->mValue)
            {
                
                if (field->mProperty == kNC_NewBookmarkFolder)
                {
                      rv = setFolderHint(bookmark, kNC_NewBookmarkFolder);
                }
                else if (field->mProperty == kNC_NewSearchFolder)
                {
                      rv = setFolderHint(bookmark, kNC_NewSearchFolder);
                }
                else if (field->mProperty == kNC_PersonalToolbarFolder)
                {
                      rv = setFolderHint(bookmark, kNC_PersonalToolbarFolder);
                      mFoundPersonalToolbarFolder = PR_TRUE;
                }
                else if (field->mProperty)
                {
                    updateAtom(mDataSource, bookmark, field->mProperty,
                               field->mValue, nsnull);
                }
            }
        }

        
        if (aLine[attrStart] == '>')
        {
            PRInt32 nameEnd;
            if (isBookmarkFlag == PR_TRUE)
                nameEnd = aLine.Find(kCloseAnchor, PR_TRUE, ++attrStart);
            else
                nameEnd = aLine.Find(kCloseHeading, PR_TRUE, ++attrStart);

            if (nameEnd > attrStart)
            {
                nsAutoString    name;
                aLine.Mid(name, attrStart, nameEnd-attrStart);
                if (!name.IsEmpty())
                {
                    Unescape(name);

                    nsCOMPtr<nsIRDFNode>    nameNode;
                    rv = ParseLiteral(kNC_Name, name, getter_AddRefs(nameNode));
                    if (NS_SUCCEEDED(rv) && nameNode)
                        updateAtom(mDataSource, bookmark, kNC_Name, nameNode, nsnull);
                }
            }
        }

        if (isBookmarkFlag == PR_FALSE)
        {
            rv = gRDFC->MakeSeq(mDataSource, bookmark, nsnull);
            NS_ASSERTION(NS_SUCCEEDED(rv), "unable to make new folder as sequence");
            if (NS_SUCCEEDED(rv))
            {
                
                rv = Parse(bookmark, nodeType);
                NS_ASSERTION(NS_SUCCEEDED(rv), "unable to parse bookmarks");
            }
        }

        
        PRInt32 aIndex;                                                             
        nsCOMPtr<nsIRDFResource> containerRes;
        aContainer->GetResource(getter_AddRefs(containerRes));
        if (containerRes && NS_SUCCEEDED(gRDFC->IndexOf(mDataSource, containerRes, bookmark, &aIndex)) &&                 
            (aIndex < 0))                                                           
        {                                                                           
          
          
          rv = aContainer->AppendElement(bookmark);                               
          NS_ASSERTION(NS_SUCCEEDED(rv), "unable to add bookmark to container");  
        }      
    }

    
    for (BookmarkField *postField = fields; postField->mName; ++postField)
    {
        NS_IF_RELEASE(postField->mValue);
    }

    return NS_OK;
}

nsresult
BookmarkParser::ParseResource(nsIRDFResource *arc, nsString& url, nsIRDFNode** aResult)
{
    *aResult = nsnull;

    if (arc == kNC_URL)
    {
        
        static const char kEscape22[] = "%22";
        PRInt32 offset;
        while ((offset = url.Find(kEscape22)) >= 0)
        {
            url.SetCharAt('\"',offset);
            url.Cut(offset + 1, sizeof(kEscape22) - 2);
        }

        
        
        
        

        
        
        if (url.FindChar(PRUnichar(':')) < 0)
        {
            url.Assign(NS_LITERAL_STRING("http://") + url);
        }
    }

    nsresult                    rv;
    nsCOMPtr<nsIRDFResource>    result;
    rv = gRDF->GetUnicodeResource(url, getter_AddRefs(result));
    if (NS_FAILED(rv)) return rv;
    return result->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) aResult);
}

nsresult
BookmarkParser::ParseLiteral(nsIRDFResource *arc, nsString& aValue, nsIRDFNode** aResult)
{
    *aResult = nsnull;

    if (arc == kNC_ShortcutURL)
    {
        
        ToLowerCase(aValue);
    }
    else if (arc == kWEB_LastCharset)
    {
        if (gCharsetAlias)
        {
            nsCAutoString charset; charset.AssignWithConversion(aValue);
            gCharsetAlias->GetPreferred(charset, charset);
            aValue.AssignWithConversion(charset.get());
        }
    }
    else if (arc == kWEB_LastPingETag)
    {
        
        PRInt32 offset;
        while ((offset = aValue.FindChar('\"')) >= 0)
        {
            aValue.Cut(offset, 1);
        }
    }

    nsresult                rv;
    nsCOMPtr<nsIRDFLiteral> result;
    rv = gRDF->GetLiteral(aValue.get(), getter_AddRefs(result));
    if (NS_FAILED(rv)) return rv;
    return result->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) aResult);
}

nsresult
BookmarkParser::ParseDate(nsIRDFResource *arc, nsString& aValue, nsIRDFNode** aResult)
{
    *aResult = nsnull;

    PRInt32 theDate = 0;
    if (!aValue.IsEmpty())
    {
        PRInt32 err;
        theDate = aValue.ToInteger(&err); 
    }
    if (theDate == 0)   return NS_RDF_NO_VALUE;

    
    PRInt64     dateVal, temp, million;
    LL_I2L(temp, theDate);
    LL_I2L(million, PR_USEC_PER_SEC);
    LL_MUL(dateVal, temp, million);

    nsresult                rv;
    nsCOMPtr<nsIRDFDate>    result;
    if (NS_FAILED(rv = gRDF->GetDateLiteral(dateVal, getter_AddRefs(result))))
    {
        NS_ERROR("unable to get date literal for time");
        return rv;
    }
    return result->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) aResult);
}

nsresult
BookmarkParser::updateAtom(nsIRDFDataSource *db, nsIRDFResource *src,
            nsIRDFResource *prop, nsIRDFNode *newValue, PRBool *dirtyFlag)
{
    nsresult        rv;
    nsCOMPtr<nsIRDFNode>    oldValue;

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

        if (prop == kWEB_Schedule)
        {
          
          updateAtom(db, src, kWEB_ScheduleActive, kTrueLiteral, dirtyFlag);
        }
    }
    return rv;
}

nsresult
BookmarkParser::ParseBookmarkSeparator(const nsString &aLine, const nsCOMPtr<nsIRDFContainer> &aContainer)
{
    nsCOMPtr<nsIRDFResource>  separator;
    nsresult rv = gRDF->GetAnonymousResource(getter_AddRefs(separator));
    if (NS_FAILED(rv))
        return rv;

    PRInt32 lineLen = aLine.Length();

    PRInt32 attrStart = 0;
    attrStart = aLine.Find(kSeparator, PR_TRUE, attrStart);
    if (attrStart < 0)
        return NS_ERROR_UNEXPECTED;
    attrStart += sizeof(kSeparator)-1;

    while((attrStart < lineLen) && (aLine[attrStart] != '>')) {
        while(nsCRT::IsAsciiSpace(aLine[attrStart]))
            ++attrStart;

        if (aLine.Find(kNameEquals, PR_TRUE, attrStart, 1) == attrStart) {
            attrStart += sizeof(kNameEquals) - 1;

            
            PRInt32 termQuote = aLine.FindChar(PRUnichar('\"'), attrStart);
            if (termQuote > attrStart) {
                nsAutoString name;
                aLine.Mid(name, attrStart, termQuote - attrStart);
                attrStart = termQuote + 1;
                if (!name.IsEmpty()) {
                    nsCOMPtr<nsIRDFLiteral> nameLiteral;
                    rv = gRDF->GetLiteral(name.get(), getter_AddRefs(nameLiteral));
                    if (NS_FAILED(rv))
                        return rv;
                    rv = mDataSource->Assert(separator, kNC_Name, nameLiteral, PR_TRUE);
                    if (NS_FAILED(rv))
                        return rv;
                }
            }
        }
    }

    rv = mDataSource->Assert(separator, kRDF_type, kNC_BookmarkSeparator, PR_TRUE);
    if (NS_FAILED(rv))
        return rv;

    rv = aContainer->AppendElement(separator);

    return rv;
}

nsresult
BookmarkParser::ParseHeaderBegin(const nsString &aLine, const nsCOMPtr<nsIRDFContainer> &aContainer)
{
    return NS_OK;
}

nsresult
BookmarkParser::ParseHeaderEnd(const nsString &aLine)
{
    return NS_OK;
}

nsresult
BookmarkParser::AssertTime(nsIRDFResource* aSource,
                           nsIRDFResource* aLabel,
                           PRInt32 aTime)
{
    nsresult    rv = NS_OK;

    if (aTime != 0)
    {
        
        PRInt64     dateVal, temp, million;

        LL_I2L(temp, aTime);
        LL_I2L(million, PR_USEC_PER_SEC);
        LL_MUL(dateVal, temp, million);     

        nsCOMPtr<nsIRDFDate>    dateLiteral;
        if (NS_FAILED(rv = gRDF->GetDateLiteral(dateVal, getter_AddRefs(dateLiteral))))
        {
            NS_ERROR("unable to get date literal for time");
            return rv;
        }
        updateAtom(mDataSource, aSource, aLabel, dateLiteral, nsnull);
    }
    return rv;
}

nsresult
BookmarkParser::setFolderHint(nsIRDFResource *newSource, nsIRDFResource *objType)
{
    nsresult            rv;
    nsCOMPtr<nsISimpleEnumerator>   srcList;
    if (NS_FAILED(rv = mDataSource->GetSources(kNC_FolderType, objType, PR_TRUE, getter_AddRefs(srcList))))
        return rv;

    PRBool  hasMoreSrcs = PR_TRUE;
    while(NS_SUCCEEDED(rv = srcList->HasMoreElements(&hasMoreSrcs))
        && (hasMoreSrcs == PR_TRUE))
    {
        nsCOMPtr<nsISupports>   aSrc;
        if (NS_FAILED(rv = srcList->GetNext(getter_AddRefs(aSrc))))
            break;
        nsCOMPtr<nsIRDFResource>    aSource = do_QueryInterface(aSrc);
        if (!aSource)   continue;

        if (NS_FAILED(rv = mDataSource->Unassert(aSource, kNC_FolderType, objType)))
            continue;
    }

    rv = mDataSource->Assert(newSource, kNC_FolderType, objType, PR_TRUE);

    return rv;
}


class SortInfo {
public:
    SortInfo(PRInt32 aDirection, PRBool aFoldersFirst)
        : mDirection(aDirection),
          mFoldersFirst(aFoldersFirst) {
    }

protected:
    PRInt32     mDirection;
    PRBool      mFoldersFirst;

    friend class nsBookmarksService;
};

class ElementInfo {
public:
    ElementInfo(nsIRDFResource* aElement, nsIRDFNode* aNode, PRBool aIsFolder)
      : mElement(aElement), mNode(aNode), mIsFolder(aIsFolder) {
    }

protected:
    nsCOMPtr<nsIRDFResource>    mElement;
    nsCOMPtr<nsIRDFNode>        mNode;
    PRBool                      mIsFolder;

    friend class nsBookmarksService;
};




class ElementArray : public nsAutoVoidArray
{
public:
  virtual ~ElementArray();

  ElementInfo* ElementAt(PRInt32 aIndex) const {
    return NS_STATIC_CAST(ElementInfo*, nsAutoVoidArray::ElementAt(aIndex));
  }

  ElementInfo* operator[](PRInt32 aIndex) const {
    return ElementAt(aIndex);
  }

  void   Clear();
};

ElementArray::~ElementArray()
{
    Clear();
}

void
ElementArray::Clear()
{
    PRInt32 index = Count();
    while (--index >= 0)
    {
        ElementInfo* elementInfo = ElementAt(index);
        delete elementInfo;
    }
    nsAutoVoidArray::Clear();
}





nsBookmarksService::nsBookmarksService() :
    mInner(nsnull),
    mUpdateBatchNest(0),
    mDirty(PR_FALSE)

#if defined(XP_MAC) || defined(XP_MACOSX)
    ,mIEFavoritesAvailable(PR_FALSE)
#endif
{ }

nsBookmarksService::~nsBookmarksService()
{
    if (mTimer)
    {
        
        
        mTimer->Cancel();
        mTimer = nsnull;
    }

    
    if (gRDF)
        gRDF->UnregisterDataSource(this);

    
    
    
    bm_ReleaseGlobals();
    NS_IF_RELEASE(mInner);
}

nsresult
nsBookmarksService::Init()
{
    nsresult rv;
    rv = bm_AddRefGlobals();
    if (NS_FAILED(rv))  return rv;

    mNetService = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv))  return rv;

    
    mCacheService = do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
    {
        rv = mCacheService->CreateSession("HTTP", nsICache::STORE_ANYWHERE,
            nsICache::STREAM_BASED, getter_AddRefs(mCacheSession));
    }

    mTransactionManager = do_CreateInstance(NS_TRANSACTIONMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsIURI> uri;
    mNetService->NewURI(bookmark_properties, nsnull, nsnull,
                        getter_AddRefs(uri));
    if (uri)
    {
        
        nsCOMPtr<nsIStringBundleService> stringService =
                do_GetService(NS_STRINGBUNDLE_CONTRACTID);
        if (stringService)
        {
            nsCAutoString spec;
            uri->GetSpec(spec);
            if (!spec.IsEmpty())
            {
                stringService->CreateBundle(spec.get(), getter_AddRefs(mBundle));
            }
        }
    }

    nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv))
    {
        
        PRInt32 toolbarIcons = 0;
        prefBranch->GetIntPref("browser.chrome.load_toolbar_icons", &toolbarIcons);
        if (toolbarIcons > 0) {
              prefBranch->GetBoolPref("browser.chrome.site_icons", &mBrowserIcons);
              mAlwaysLoadIcons = (toolbarIcons > 1);
        } else
              mAlwaysLoadIcons = mBrowserIcons = PR_FALSE;
        
        
        
        nsXPIDLCString prefValue;
        rv = prefBranch->GetCharPref("custtoolbar.personal_toolbar_folder", getter_Copies(prefValue));
        if (NS_SUCCEEDED(rv) && !prefValue.IsEmpty())
        {
            CopyUTF8toUTF16(prefValue, mPersonalToolbarName);
        }

        if (mPersonalToolbarName.IsEmpty())
        {
            
            rv = mBundle->GetStringFromName(NS_LITERAL_STRING("DefaultPersonalToolbarFolder").get(), 
                                            getter_Copies(mPersonalToolbarName));
            if (NS_FAILED(rv) || mPersonalToolbarName.IsEmpty()) {
              
              mPersonalToolbarName.AssignLiteral("Personal Toolbar Folder");
            }
        }
    }

    
    
    
    
    nsresult             useProfile;
    nsCOMPtr<nsIProfile> profileService(do_GetService(NS_PROFILE_CONTRACTID,&useProfile));
    if (NS_SUCCEEDED(useProfile))
    {
        nsXPIDLString        currentProfileName;
    
        useProfile = profileService->GetCurrentProfile(getter_Copies(currentProfileName));
        if (NS_SUCCEEDED(useProfile))
        {
            const PRUnichar *param[1] = {currentProfileName.get()};
            useProfile = mBundle->FormatStringFromName(NS_LITERAL_STRING("bookmarks_root").get(),
                                                    param, 1, getter_Copies(mBookmarksRootName));
            if (NS_SUCCEEDED(useProfile))
            {
                PRInt32 profileCount;
                useProfile = profileService->GetProfileCount(&profileCount);
                if (NS_SUCCEEDED(useProfile) && profileCount == 1)
                {
                    ToLowerCase(currentProfileName);
                    if (currentProfileName.EqualsLiteral("default"))
                        useProfile = NS_ERROR_FAILURE;
                }
            }
        }
    }

    if (NS_FAILED(useProfile))
    {
        rv = mBundle->GetStringFromName(NS_LITERAL_STRING("bookmarks_default_root").get(),
                                        getter_Copies(mBookmarksRootName));
        if (NS_FAILED(rv) || mBookmarksRootName.IsEmpty()) {
            mBookmarksRootName.AssignLiteral("Bookmarks");
        }
    }

    
    nsCOMPtr<nsIObserverService> observerService = 
             do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ASSERTION(observerService, "Could not get observer service.");
    if (observerService) {
        observerService->AddObserver(this, "profile-before-change", PR_TRUE);
        observerService->AddObserver(this, "profile-after-change", PR_TRUE);
    }

    rv = initDatasource();
    if (NS_FAILED(rv))
        return rv;

    
    busyResource = nsnull;

    if (!mTimer)
    {
        busySchedule = PR_FALSE;
        mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to create a timer");
        if (NS_FAILED(rv)) return rv;
        mTimer->InitWithFuncCallback(nsBookmarksService::FireTimer, this, BOOKMARK_TIMEOUT, 
                                     nsITimer::TYPE_REPEATING_SLACK);
        
    }

    
    
    
    
    rv = gRDF->RegisterDataSource(this, PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    return NS_OK;
}

nsresult
nsBookmarksService::getLocaleString(const char *key, nsString &str)
{
    PRUnichar   *keyUni = nsnull;
    nsAutoString    keyStr;
    keyStr.AssignWithConversion(key);

    nsresult    rv = NS_RDF_NO_VALUE;
    if (mBundle && (NS_SUCCEEDED(rv = mBundle->GetStringFromName(keyStr.get(), &keyUni)))
        && (keyUni))
    {
        str = keyUni;
        NS_Free(keyUni);
    }
    else
    {
        str.Truncate();
    }
    return rv;
}

nsresult
nsBookmarksService::ExamineBookmarkSchedule(nsIRDFResource *theBookmark, PRBool & examineFlag)
{
    examineFlag = PR_FALSE;
    
    nsresult    rv = NS_OK;

    nsCOMPtr<nsIRDFNode>    scheduleNode;
    if (NS_FAILED(rv = mInner->GetTarget(theBookmark, kWEB_Schedule, PR_TRUE,
        getter_AddRefs(scheduleNode))) || (rv == NS_RDF_NO_VALUE))
        return rv;
    
    nsCOMPtr<nsIRDFLiteral> scheduleLiteral = do_QueryInterface(scheduleNode);
    if (!scheduleLiteral)   return NS_ERROR_NO_INTERFACE;
    
    const PRUnichar     *scheduleUni = nsnull;
    if (NS_FAILED(rv = scheduleLiteral->GetValueConst(&scheduleUni)))
        return rv;
    if (!scheduleUni)   return NS_ERROR_NULL_POINTER;

    nsAutoString        schedule(scheduleUni);
    if (schedule.Length() < 1)  return NS_ERROR_UNEXPECTED;

    
    
    PRTime      now64 = PR_Now(), temp64, million;
    LL_I2L(million, PR_USEC_PER_SEC);
    LL_DIV(temp64, now64, million);
    PRInt32     now32;
    LL_L2I(now32, temp64);

    PRExplodedTime  nowInfo;
    PR_ExplodeTime(now64, PR_LocalTimeParameters, &nowInfo);
    
    
    PR_NormalizeTime(&nowInfo, PR_LocalTimeParameters);

    nsAutoString    dayNum;
    dayNum.AppendInt(nowInfo.tm_wday, 10);

    
    
    

    nsAutoString    notificationMethod;
    PRInt32     startHour = -1, endHour = -1, duration = -1;

    
    PRInt32     slashOffset;
    if ((slashOffset = schedule.FindChar(PRUnichar('|'))) >= 0)
    {
        nsAutoString    daySection;
        schedule.Left(daySection, slashOffset);
        schedule.Cut(0, slashOffset+1);
        if (daySection.Find(dayNum) >= 0)
        {
            
            if ((slashOffset = schedule.FindChar(PRUnichar('|'))) >= 0)
            {
                nsAutoString    hourRange;
                schedule.Left(hourRange, slashOffset);
                schedule.Cut(0, slashOffset+1);

                
                
                PRInt32     dashOffset;
                if ((dashOffset = hourRange.FindChar(PRUnichar('-'))) >= 1)
                {
                    nsAutoString    startStr, endStr;

                    hourRange.Right(endStr, hourRange.Length() - dashOffset - 1);
                    hourRange.Left(startStr, dashOffset);

                    PRInt32     errorCode2 = 0;
                    startHour = startStr.ToInteger(&errorCode2);
                    if (errorCode2) startHour = -1;
                    endHour = endStr.ToInteger(&errorCode2);
                    if (errorCode2) endHour = -1;
                    
                    if ((startHour >=0) && (endHour >=0))
                    {
                        if ((slashOffset = schedule.FindChar(PRUnichar('|'))) >= 0)
                        {
                            nsAutoString    durationStr;
                            schedule.Left(durationStr, slashOffset);
                            schedule.Cut(0, slashOffset+1);

                            
                            PRInt32     errorCode = 0;
                            duration = durationStr.ToInteger(&errorCode);
                            if (errorCode)  duration = -1;
                            
                            
                            notificationMethod = schedule;
                        }
                    }
                }
            }
        }
    }
        

#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
    char *methodStr = ToNewCString(notificationMethod);
    if (methodStr)
    {
        printf("Start Hour: %d    End Hour: %d    Duration: %d mins    Method: '%s'\n",
            startHour, endHour, duration, methodStr);
        delete [] methodStr;
        methodStr = nsnull;
    }
#endif

    if ((startHour <= nowInfo.tm_hour) && (endHour >= nowInfo.tm_hour) &&
        (duration >= 1) && (!notificationMethod.IsEmpty()))
    {
        
        

        examineFlag = PR_TRUE;

        nsCOMPtr<nsIRDFNode>    pingNode;
        if (NS_SUCCEEDED(rv = mInner->GetTarget(theBookmark, kWEB_LastPingDate,
            PR_TRUE, getter_AddRefs(pingNode))) && (rv != NS_RDF_NO_VALUE))
        {
            nsCOMPtr<nsIRDFDate>    pingLiteral = do_QueryInterface(pingNode);
            if (pingLiteral)
            {
                PRInt64     lastPing;
                if (NS_SUCCEEDED(rv = pingLiteral->GetValue(&lastPing)))
                {
                    PRInt64     diff64, sixty;
                    LL_SUB(diff64, now64, lastPing);
                    
                    
                    LL_DIV(diff64, diff64, million);
                    
                    LL_I2L(sixty, 60L);
                    LL_DIV(diff64, diff64, sixty);

                    PRInt32     diff32;
                    LL_L2I(diff32, diff64);
                    if (diff32 < duration)
                    {
                        examineFlag = PR_FALSE;

#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
                        printf("Skipping URL, its too soon.\n");
#endif
                    }
                }
            }
        }
    }
    return rv;
}

nsresult
nsBookmarksService::GetBookmarkToPing(nsIRDFResource **theBookmark)
{
    nsresult    rv = NS_OK;

    *theBookmark = nsnull;

    nsCOMPtr<nsISimpleEnumerator>   srcList;
    if (NS_FAILED(rv = GetSources(kWEB_ScheduleActive, kTrueLiteral, PR_TRUE, getter_AddRefs(srcList))))
        return rv;

    nsCOMPtr<nsISupportsArray>  bookmarkList;
    if (NS_FAILED(rv = NS_NewISupportsArray(getter_AddRefs(bookmarkList))))
        return rv;

    
    PRBool  hasMoreSrcs = PR_TRUE;
    while(NS_SUCCEEDED(rv = srcList->HasMoreElements(&hasMoreSrcs))
        && (hasMoreSrcs == PR_TRUE))
    {
        nsCOMPtr<nsISupports>   aSrc;
        if (NS_FAILED(rv = srcList->GetNext(getter_AddRefs(aSrc))))
            break;
        nsCOMPtr<nsIRDFResource>    aSource = do_QueryInterface(aSrc);
        if (!aSource)   continue;

        
        

        PRBool  examineFlag = PR_FALSE;
        if (NS_FAILED(rv = ExamineBookmarkSchedule(aSource, examineFlag))
            || (examineFlag == PR_FALSE))   continue;

        bookmarkList->AppendElement(aSource);
    }

    
    PRUint32    numBookmarks;
    if (NS_SUCCEEDED(rv = bookmarkList->Count(&numBookmarks)) && (numBookmarks > 0))
    {
        PRInt32     randomNum;
        LL_L2I(randomNum, PR_Now());
        PRUint32    randomBookmark = (numBookmarks-1) % randomNum;

        nsCOMPtr<nsISupports>   iSupports;
        if (NS_SUCCEEDED(rv = bookmarkList->GetElementAt(randomBookmark,
            getter_AddRefs(iSupports))))
        {
            nsCOMPtr<nsIRDFResource>    aBookmark = do_QueryInterface(iSupports);
            if (aBookmark)
            {
                *theBookmark = aBookmark;
                NS_ADDREF(*theBookmark);
            }
        }
    }
    return rv;
}

void
nsBookmarksService::FireTimer(nsITimer* aTimer, void* aClosure)
{
    nsBookmarksService *bmks = NS_STATIC_CAST(nsBookmarksService *, aClosure);
    if (!bmks)  return;
    nsresult            rv;

    if (bmks->mDirty)
    {
        bmks->Flush();
    }

    if (bmks->busySchedule == PR_FALSE)
    {
        nsCOMPtr<nsIRDFResource>    bookmark;
        if (NS_SUCCEEDED(rv = bmks->GetBookmarkToPing(getter_AddRefs(bookmark))) && (bookmark))
        {
            bmks->busyResource = bookmark;

            nsAutoString url;
            rv = bmks->GetURLFromResource(bookmark, url);
            if (NS_FAILED(rv))
                return;

#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
            printf("nsBookmarksService::FireTimer - Pinging '%s'\n",
                   NS_ConvertUTF16toUTF8(url).get());
#endif

            nsCOMPtr<nsIURI>    uri;
            if (NS_SUCCEEDED(rv = NS_NewURI(getter_AddRefs(uri), url)))
            {
                nsCOMPtr<nsIChannel>    channel;
                if (NS_SUCCEEDED(rv = NS_NewChannel(getter_AddRefs(channel), uri, nsnull)))
                {
                    channel->SetLoadFlags(nsIRequest::VALIDATE_ALWAYS);
                    nsCOMPtr<nsIHttpChannel>    httpChannel = do_QueryInterface(channel);
                    if (httpChannel)
                    {
                        bmks->htmlSize = 0;
                        httpChannel->SetRequestMethod(NS_LITERAL_CSTRING("HEAD"));
                        if (NS_SUCCEEDED(rv = channel->AsyncOpen(bmks, nsnull)))
                        {
                            bmks->busySchedule = PR_TRUE;
                        }
                    }
                }
            }
        }
    }
#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
    else
    {
        printf("nsBookmarksService::FireTimer - busy pinging.\n");
    }
#endif
}




NS_IMETHODIMP
nsBookmarksService::OnStartRequest(nsIRequest* request, nsISupports *ctxt)
{
    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::OnDataAvailable(nsIRequest* request, nsISupports *ctxt, nsIInputStream *aIStream,
                      PRUint32 sourceOffset, PRUint32 aLength)
{
    
    htmlSize += aLength;

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::OnStopRequest(nsIRequest* request, nsISupports *ctxt,
                    nsresult status)
{
    nsresult rv;

    nsAutoString url;
    if (NS_SUCCEEDED(rv = GetURLFromResource(busyResource, url)))
    {
#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
        printf("Finished polling '%s'\n", NS_ConvertUTF16toUTF8(url).get());
#endif
    }
    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
    nsCOMPtr<nsIHttpChannel>    httpChannel = do_QueryInterface(channel);
    if (httpChannel)
    {
        nsAutoString            eTagValue, lastModValue, contentLengthValue;

        nsCAutoString val;
        if (NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("ETag"), val)))
            CopyASCIItoUTF16(val, eTagValue);
        if (NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Last-Modified"), val)))
            CopyASCIItoUTF16(val, lastModValue);
        if (NS_SUCCEEDED(httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("Content-Length"), val)))
            CopyASCIItoUTF16(val, contentLengthValue);
        val.Truncate();

        PRBool      changedFlag = PR_FALSE;

        PRUint32    respStatus;
        if (NS_SUCCEEDED(rv = httpChannel->GetResponseStatus(&respStatus)))
        {
            if ((respStatus >= 200) && (respStatus <= 299))
            {
                if (!eTagValue.IsEmpty())
                {
#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
                    printf("eTag: '%s'\n", NS_ConvertUTF16toUTF8(eTagValue).get());
#endif
                    nsAutoString        eTagStr;
                    nsCOMPtr<nsIRDFNode>    currentETagNode;
                    if (NS_SUCCEEDED(rv = mInner->GetTarget(busyResource, kWEB_LastPingETag,
                        PR_TRUE, getter_AddRefs(currentETagNode))) && (rv != NS_RDF_NO_VALUE))
                    {
                        nsCOMPtr<nsIRDFLiteral> currentETagLit = do_QueryInterface(currentETagNode);
                        if (currentETagLit)
                        {
                            const PRUnichar* currentETagStr = nsnull;
                            currentETagLit->GetValueConst(&currentETagStr);
                            if ((currentETagStr) &&
                                !eTagValue.Equals(nsDependentString(currentETagStr),
                                                  nsCaseInsensitiveStringComparator()))
                            {
                                changedFlag = PR_TRUE;
                            }
                            eTagStr.Assign(eTagValue);
                            nsCOMPtr<nsIRDFLiteral> newETagLiteral;
                            if (NS_SUCCEEDED(rv = gRDF->GetLiteral(eTagStr.get(),
                                getter_AddRefs(newETagLiteral))))
                            {
                                rv = mInner->Change(busyResource, kWEB_LastPingETag,
                                    currentETagNode, newETagLiteral);
                            }
                        }
                    }
                    else
                    {
                        eTagStr.Assign(eTagValue);
                        nsCOMPtr<nsIRDFLiteral> newETagLiteral;
                        if (NS_SUCCEEDED(rv = gRDF->GetLiteral(eTagStr.get(),
                            getter_AddRefs(newETagLiteral))))
                        {
                            rv = mInner->Assert(busyResource, kWEB_LastPingETag,
                                newETagLiteral, PR_TRUE);
                        }
                    }
                }
            }
        }

        if ((changedFlag == PR_FALSE) && (!lastModValue.IsEmpty()))
        {
#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
            printf("Last-Modified: '%s'\n", lastModValue.get());
#endif
            nsAutoString        lastModStr;
            nsCOMPtr<nsIRDFNode>    currentLastModNode;
            if (NS_SUCCEEDED(rv = mInner->GetTarget(busyResource, kWEB_LastPingModDate,
                PR_TRUE, getter_AddRefs(currentLastModNode))) && (rv != NS_RDF_NO_VALUE))
            {
                nsCOMPtr<nsIRDFLiteral> currentLastModLit = do_QueryInterface(currentLastModNode);
                if (currentLastModLit)
                {
                    const PRUnichar* currentLastModStr = nsnull;
                    currentLastModLit->GetValueConst(&currentLastModStr);
                    if ((currentLastModStr) &&
                        !lastModValue.Equals(nsDependentString(currentLastModStr),
                                             nsCaseInsensitiveStringComparator()))
                    {
                        changedFlag = PR_TRUE;
                    }
                    lastModStr.Assign(lastModValue);
                    nsCOMPtr<nsIRDFLiteral> newLastModLiteral;
                    if (NS_SUCCEEDED(rv = gRDF->GetLiteral(lastModStr.get(),
                        getter_AddRefs(newLastModLiteral))))
                    {
                        rv = mInner->Change(busyResource, kWEB_LastPingModDate,
                            currentLastModNode, newLastModLiteral);
                    }
                }
            }
            else
            {
                lastModStr.Assign(lastModValue);
                nsCOMPtr<nsIRDFLiteral> newLastModLiteral;
                if (NS_SUCCEEDED(rv = gRDF->GetLiteral(lastModStr.get(),
                    getter_AddRefs(newLastModLiteral))))
                {
                    rv = mInner->Assert(busyResource, kWEB_LastPingModDate,
                        newLastModLiteral, PR_TRUE);
                }
            }
        }

        if ((changedFlag == PR_FALSE) && (!contentLengthValue.IsEmpty()))
        {
#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
            printf("Content-Length: '%s'\n", contentLengthValue.get());
#endif
            nsAutoString        contentLenStr;
            nsCOMPtr<nsIRDFNode>    currentContentLengthNode;
            if (NS_SUCCEEDED(rv = mInner->GetTarget(busyResource, kWEB_LastPingContentLen,
                PR_TRUE, getter_AddRefs(currentContentLengthNode))) && (rv != NS_RDF_NO_VALUE))
            {
                nsCOMPtr<nsIRDFLiteral> currentContentLengthLit = do_QueryInterface(currentContentLengthNode);
                if (currentContentLengthLit)
                {
                    const PRUnichar *currentContentLengthStr = nsnull;
                    currentContentLengthLit->GetValueConst(&currentContentLengthStr);
                    if ((currentContentLengthStr) &&
                        !contentLengthValue.Equals(nsDependentString(currentContentLengthStr),
                                                   nsCaseInsensitiveStringComparator()))
                    {
                        changedFlag = PR_TRUE;
                    }
                    contentLenStr.Assign(contentLengthValue);
                    nsCOMPtr<nsIRDFLiteral> newContentLengthLiteral;
                    if (NS_SUCCEEDED(rv = gRDF->GetLiteral(contentLenStr.get(),
                        getter_AddRefs(newContentLengthLiteral))))
                    {
                        rv = mInner->Change(busyResource, kWEB_LastPingContentLen,
                            currentContentLengthNode, newContentLengthLiteral);
                    }
                }
            }
            else
            {
                contentLenStr.Assign(contentLengthValue);
                nsCOMPtr<nsIRDFLiteral> newContentLengthLiteral;
                if (NS_SUCCEEDED(rv = gRDF->GetLiteral(contentLenStr.get(),
                    getter_AddRefs(newContentLengthLiteral))))
                {
                    rv = mInner->Assert(busyResource, kWEB_LastPingContentLen,
                        newContentLengthLiteral, PR_TRUE);
                }
            }
        }

        
        nsCOMPtr<nsIRDFDate>    dateLiteral;
        if (NS_SUCCEEDED(rv = gRDF->GetDateLiteral(PR_Now(), getter_AddRefs(dateLiteral))))
        {
            nsCOMPtr<nsIRDFNode>    lastPingNode;
            if (NS_SUCCEEDED(rv = mInner->GetTarget(busyResource, kWEB_LastPingDate, PR_TRUE,
                getter_AddRefs(lastPingNode))) && (rv != NS_RDF_NO_VALUE))
            {
                rv = mInner->Change(busyResource, kWEB_LastPingDate, lastPingNode, dateLiteral);
            }
            else
            {
                rv = mInner->Assert(busyResource, kWEB_LastPingDate, dateLiteral, PR_TRUE);
            }
            NS_ASSERTION(rv == NS_RDF_ASSERTION_ACCEPTED, "unable to assert new time");


        }
        else
        {
            NS_ERROR("unable to get date literal for now");
        }

        
        if (changedFlag == PR_TRUE)
        {
#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
            printf("URL has changed!\n\n");
#endif

            nsAutoString        schedule;

            nsCOMPtr<nsIRDFNode>    scheduleNode;
            if (NS_SUCCEEDED(rv = mInner->GetTarget(busyResource, kWEB_Schedule, PR_TRUE,
                getter_AddRefs(scheduleNode))) && (rv != NS_RDF_NO_VALUE))
            {
                nsCOMPtr<nsIRDFLiteral> scheduleLiteral = do_QueryInterface(scheduleNode);
                if (scheduleLiteral)
                {
                    const PRUnichar     *scheduleUni = nsnull;
                    if (NS_SUCCEEDED(rv = scheduleLiteral->GetValueConst(&scheduleUni))
                        && (scheduleUni))
                    {
                        schedule = scheduleUni;
                    }
                }
            }

            
            if (FindInReadable(NS_LITERAL_STRING("icon"),
                               schedule,
                               nsCaseInsensitiveStringComparator()))
            {
                nsCOMPtr<nsIRDFLiteral> statusLiteral;
                if (NS_SUCCEEDED(rv = gRDF->GetLiteral(NS_LITERAL_STRING("new").get(), getter_AddRefs(statusLiteral))))
                {
                    nsCOMPtr<nsIRDFNode>    currentStatusNode;
                    if (NS_SUCCEEDED(rv = mInner->GetTarget(busyResource, kWEB_Status, PR_TRUE,
                        getter_AddRefs(currentStatusNode))) && (rv != NS_RDF_NO_VALUE))
                    {
                        rv = mInner->Change(busyResource, kWEB_Status, currentStatusNode, statusLiteral);
                    }
                    else
                    {
                        rv = mInner->Assert(busyResource, kWEB_Status, statusLiteral, PR_TRUE);
                    }
                    NS_ASSERTION(rv == NS_RDF_ASSERTION_ACCEPTED, "unable to assert changed status");
                }
            }
            
            
            if (FindInReadable(NS_LITERAL_STRING("sound"),
                               schedule,
                               nsCaseInsensitiveStringComparator()))
            {
                nsCOMPtr<nsISound> soundInterface =
                        do_CreateInstance("@mozilla.org/sound;1", &rv);
                if (NS_SUCCEEDED(rv))
                {
                    
                    soundInterface->Beep();
                }
            }
            
            PRBool      openURLFlag = PR_FALSE;

            
            if (FindInReadable(NS_LITERAL_STRING("alert"),
                               schedule,
                               nsCaseInsensitiveStringComparator()))
            {
                nsCOMPtr<nsIPrompt> prompter;
                NS_QueryNotificationCallbacks(channel, prompter);
                if (!prompter)
                {
                    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
                    if (wwatch)
                        wwatch->GetNewPrompter(0, getter_AddRefs(prompter));
                }

                if (prompter)
                {
                    nsAutoString    promptStr;
                    getLocaleString("WebPageUpdated", promptStr);
                    if (!promptStr.IsEmpty())   promptStr.AppendLiteral("\n\n");

                    nsCOMPtr<nsIRDFNode>    nameNode;
                    if (NS_SUCCEEDED(mInner->GetTarget(busyResource, kNC_Name,
                        PR_TRUE, getter_AddRefs(nameNode))))
                    {
                        nsCOMPtr<nsIRDFLiteral> nameLiteral = do_QueryInterface(nameNode);
                        if (nameLiteral)
                        {
                            const PRUnichar *nameUni = nsnull;
                            if (NS_SUCCEEDED(rv = nameLiteral->GetValueConst(&nameUni))
                                && (nameUni))
                            {
                                nsAutoString    info;
                                getLocaleString("WebPageTitle", info);
                                promptStr += info;
                                promptStr.AppendLiteral(" ");
                                promptStr += nameUni;
                                promptStr.AppendLiteral("\n");
                                getLocaleString("WebPageURL", info);
                                promptStr += info;
                                promptStr.AppendLiteral(" ");
                            }
                        }
                    }
                    promptStr.Append(url);
                    
                    nsAutoString    temp;
                    getLocaleString("WebPageAskDisplay", temp);
                    if (!temp.IsEmpty())
                    {
                        promptStr.AppendLiteral("\n\n");
                        promptStr += temp;
                    }

                    nsAutoString    stopOption;
                    getLocaleString("WebPageAskStopOption", stopOption);
                    PRBool      stopCheckingFlag = PR_FALSE;
                    rv = prompter->ConfirmCheck(nsnull, promptStr.get(), stopOption.get(),
                                  &stopCheckingFlag, &openURLFlag);
                    if (NS_FAILED(rv))
                    {
                        openURLFlag = PR_FALSE;
                        stopCheckingFlag = PR_FALSE;
                    }
                    if (stopCheckingFlag == PR_TRUE)
                    {
#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
                        printf("\nStop checking this URL.\n");
#endif
                        rv = mInner->Unassert(busyResource, kWEB_Schedule, scheduleNode);
                        NS_ASSERTION(NS_SUCCEEDED(rv), "unable to unassert kWEB_Schedule");
                    }
                }
            }

            
            if ((openURLFlag == PR_TRUE) ||
                FindInReadable(NS_LITERAL_STRING("open"),
                               schedule,
                               nsCaseInsensitiveStringComparator()))
            {
                if (NS_SUCCEEDED(rv))
                {
                    nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
                    if (wwatch)
                    {
                        nsCOMPtr<nsIDOMWindow> newWindow;
                        nsCOMPtr<nsISupportsArray> suppArray;
                        rv = NS_NewISupportsArray(getter_AddRefs(suppArray));
                        if (NS_FAILED(rv)) return rv;

                        nsCOMPtr<nsISupportsString> suppString(do_CreateInstance("@mozilla.org/supports-string;1", &rv));
                        if (!suppString) return rv;

                        rv = suppString->SetData(url);
                        if (NS_FAILED(rv)) return rv;

                        suppArray->AppendElement(suppString);

                        nsXPIDLCString chromeUrl;
                        nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
                        if (NS_SUCCEEDED(rv))
                        {
                            prefs->GetCharPref("browser.chromeURL", getter_Copies(chromeUrl));
                        }
                        if (chromeUrl.IsEmpty())
                        {
                            chromeUrl.AssignLiteral("chrome://navigator/content/navigator.xul");
                        }
                        wwatch->OpenWindow(0, chromeUrl, "_blank",
                                           "chrome,dialog=no,all", suppArray,
                                           getter_AddRefs(newWindow));
                    }
                }
            }
        }
#ifdef  DEBUG_BOOKMARK_PING_OUTPUT
        else
        {
            printf("URL has not changed status.\n\n");
        }
#endif
    }

    busyResource = nsnull;
    busySchedule = PR_FALSE;

    return NS_OK;
}




NS_IMETHODIMP nsBookmarksService::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;

    if (!nsCRT::strcmp(aTopic, "profile-before-change"))
    {
        
        rv = Flush();
    
        if (!nsCRT::strcmp(someData, NS_LITERAL_STRING("shutdown-cleanse").get()))
        {
            if (mBookmarksFile)
            {
                mBookmarksFile->Remove(PR_FALSE);
            }
        }
    }    
    else if (mBookmarksFile && !nsCRT::strcmp(aTopic, "profile-after-change"))
    {
        
        rv = LoadBookmarks();
    }
    else if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID))
    {
        rv = Flush();
        if (NS_SUCCEEDED(rv))
            rv = LoadBookmarks();
    }

    return rv;
}





NS_IMPL_ADDREF(nsBookmarksService)

NS_IMETHODIMP_(nsrefcnt)
nsBookmarksService::Release()
{
    
    
    NS_PRECONDITION(PRInt32(mRefCnt) > 0, "duplicate release");
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "nsBookmarksService");

    if (mInner && mRefCnt == 1) {
        nsIRDFDataSource* tmp = mInner;
        mInner = nsnull;
        NS_IF_RELEASE(tmp);
        return 0;
    }
    else if (mRefCnt == 0) {
        delete this;
        return 0;
    }
    else {
        return mRefCnt;
    }
}

NS_IMPL_QUERY_INTERFACE10(nsBookmarksService,
             nsIBookmarksService,
             nsIRDFDataSource,
             nsIRDFRemoteDataSource,
             nsIRDFPropagatableDataSource,
             nsIRDFObserver,
             nsIStreamListener,
             nsIRequestObserver,
             nsICharsetResolver,
             nsIObserver,
             nsISupportsWeakReference)





nsresult
nsBookmarksService::InsertResource(nsIRDFResource* aResource,
                                   nsIRDFResource* aParentFolder, PRInt32 aIndex)
{
    nsresult rv = NS_OK;
    
    if (aParentFolder)
    {
        nsCOMPtr<nsIRDFContainer> container(do_CreateInstance("@mozilla.org/rdf/container;1", &rv));
        if (NS_FAILED(rv)) 
            return rv;
        rv = container->Init(mInner, aParentFolder);
        if (NS_FAILED(rv)) 
            return rv;
        
        if (aIndex > 0) 
            rv = container->InsertElementAt(aResource, aIndex, PR_TRUE);
        else
            rv = container->AppendElement(aResource);

        mDirty = PR_TRUE;
    }
    return rv;
}

NS_IMETHODIMP
nsBookmarksService::CreateFolder(const PRUnichar* aName, 
                                 nsIRDFResource** aResult)
{
    nsresult rv;

    
    nsCOMPtr<nsIRDFResource> folderResource;
    rv = gRDF->GetAnonymousResource(getter_AddRefs(folderResource));
    if (NS_FAILED(rv)) 
        return rv;

    rv = gRDFC->MakeSeq(mInner, folderResource, nsnull);
    if (NS_FAILED(rv)) 
        return rv;

    
    nsCOMPtr<nsIRDFLiteral> nameLiteral;
    nsAutoString folderName; 
    folderName.Assign(aName);
    if (folderName.IsEmpty()) {
        getLocaleString("NewFolder", folderName);

        rv = gRDF->GetLiteral(folderName.get(), getter_AddRefs(nameLiteral));
        if (NS_FAILED(rv)) 
            return rv;
    }
    else
    {
        rv = gRDF->GetLiteral(aName, getter_AddRefs(nameLiteral));
        if (NS_FAILED(rv)) 
            return rv;
    }

    rv = mInner->Assert(folderResource, kNC_Name, nameLiteral, PR_TRUE);
    if (NS_FAILED(rv)) 
        return rv;

    
    
    nsCOMPtr<nsIRDFDate> dateLiteral;
    rv = gRDF->GetDateLiteral(PR_Now(), getter_AddRefs(dateLiteral));
    if (NS_FAILED(rv)) 
        return rv;
    rv = mInner->Assert(folderResource, kNC_BookmarkAddDate, dateLiteral, PR_TRUE);
    if (NS_FAILED(rv)) 
        return rv;

    *aResult = folderResource;
    NS_ADDREF(*aResult);

    return rv;
}

NS_IMETHODIMP
nsBookmarksService::CreateFolderInContainer(const PRUnichar* aName, 
                                            nsIRDFResource* aParentFolder, PRInt32 aIndex,
                                            nsIRDFResource** aResult)
{
    nsresult rv = CreateFolder(aName, aResult);
    if (NS_SUCCEEDED(rv))
        rv = InsertResource(*aResult, aParentFolder, aIndex);
    return rv;
}

NS_IMETHODIMP
nsBookmarksService::CreateGroup(const PRUnichar* aName, 
                                nsIRDFResource** aResult)
{
    nsresult rv;
    rv = CreateFolderInContainer(aName, nsnull, nsnull, aResult);
    if (NS_SUCCEEDED(rv))
        rv = mInner->Assert(*aResult, kNC_FolderGroup, kTrueLiteral, PR_TRUE);
    return rv;
}

NS_IMETHODIMP
nsBookmarksService::CreateGroupInContainer(const PRUnichar* aName, 
                                           nsIRDFResource* aParentFolder, PRInt32 aIndex,
                                           nsIRDFResource** aResult)
{
    nsresult rv = CreateGroup(aName, aResult);
    if (NS_SUCCEEDED(rv))
        rv = InsertResource(*aResult, aParentFolder, aIndex);
    return rv;
}

int
nsBookmarksService::Compare(const void* aElement1, const void* aElement2, void* aData)
{
    const ElementInfo* elementInfo1 =
      NS_STATIC_CAST(ElementInfo*, NS_CONST_CAST(void*, aElement1));
    const ElementInfo* elementInfo2 =
      NS_STATIC_CAST(ElementInfo*, NS_CONST_CAST(void*, aElement2));
    SortInfo* sortInfo = (SortInfo*)aData;

    if (sortInfo->mFoldersFirst) {
        if (elementInfo1->mIsFolder) {
            if (!elementInfo2->mIsFolder) {
                return -1;
            }
        }
        else {
            if (elementInfo2->mIsFolder) {
                return 1;
            }
        }
    }
 
    PRInt32 result = 0;
 
    nsIRDFNode* node1 = elementInfo1->mNode;
    nsIRDFNode* node2 = elementInfo2->mNode;
 
    
    nsCOMPtr<nsIRDFLiteral> literal1 = do_QueryInterface(node1);
    if (literal1) {
        nsCOMPtr<nsIRDFLiteral> literal2 = do_QueryInterface(node2);
        if (literal2) {
            const PRUnichar* value1;
            literal1->GetValueConst(&value1);
            const PRUnichar* value2;
            literal2->GetValueConst(&value2);

            if (gCollation) {
                gCollation->CompareString(nsICollation::kCollationCaseInSensitive,
                                          nsDependentString(value1),
                                          nsDependentString(value2),
                                          &result);
            }
            else {
                result = ::Compare(nsDependentString(value1),
                                   nsDependentString(value2),
                                   nsCaseInsensitiveStringComparator());
            }

            return result * sortInfo->mDirection;
        }
    }
 
    
    nsCOMPtr<nsIRDFDate> date1 = do_QueryInterface(node1);
    if (date1) {
        nsCOMPtr<nsIRDFDate> date2 = do_QueryInterface(node2);
        if (date2) {
            PRTime value1;
            date1->GetValue(&value1);
            PRTime value2;
            date2->GetValue(&value2);

            PRInt64 delta;
            LL_SUB(delta, value1, value2);

            if (LL_IS_ZERO(delta))
                result = 0;
            else if (LL_GE_ZERO(delta))
                result = 1;
            else
                result = -1;

            return result * sortInfo->mDirection;
        }
    }

    
    return 0;
}
 
nsresult
nsBookmarksService::Sort(nsIRDFResource* aFolder, nsIRDFResource* aProperty,
                         PRInt32 aDirection, PRBool aFoldersFirst,
                         PRBool aRecurse)
{
    nsresult rv;
    nsCOMPtr<nsIRDFContainer> container =
        do_CreateInstance("@mozilla.org/rdf/container;1", &rv);
    if (NS_FAILED(rv))
        return rv;

    rv = container->Init(mInner, aFolder);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsISimpleEnumerator> elements;
    rv = container->GetElements(getter_AddRefs(elements));
    if (NS_FAILED(rv))
        return rv;

    ElementArray elementArray;

    PRBool hasMore = PR_FALSE;
    while (NS_SUCCEEDED(rv = elements->HasMoreElements(&hasMore)) &&
           hasMore) {
        nsCOMPtr<nsISupports> supports;
        rv = elements->GetNext(getter_AddRefs(supports));
        if (NS_FAILED(rv))
            return rv;

        nsCOMPtr<nsIRDFResource> element = do_QueryInterface(supports, &rv);
        if (NS_FAILED(rv))
            return rv;

        nsCOMPtr<nsIRDFNode> node;
        rv = mInner->GetTarget(element, aProperty, PR_TRUE, getter_AddRefs(node));
        if (NS_FAILED(rv))
            return rv;

        if (!node) {
            if (aProperty == kNC_BookmarkAddDate ||
                aProperty == kWEB_LastModifiedDate ||
                aProperty == kWEB_LastVisitDate) {
                node = do_QueryInterface(kEmptyDate);
            }
            else {
                node = do_QueryInterface(kEmptyLiteral);
            }
        }

        PRBool isContainer;
        rv = gRDFC->IsContainer(mInner, element, &isContainer);
        if (NS_FAILED(rv))
            return rv;

        PRBool isGroup;
        rv = mInner->HasAssertion(element, kNC_FolderGroup, kTrueLiteral,
                                  PR_TRUE, &isGroup);
        if (NS_FAILED(rv))
            return rv;

        ElementInfo* elementInfo = new ElementInfo(element, node,
                                                   isContainer && !isGroup);
        if (!elementInfo)
            return NS_ERROR_OUT_OF_MEMORY;

        elementArray.AppendElement(elementInfo);

        if (isContainer && aRecurse) {
            rv = Sort(element, aProperty, aDirection, aFoldersFirst, aRecurse);
            if (NS_FAILED(rv))
                return rv;
        }
    }

    SortInfo sortInfo(aDirection, aFoldersFirst);
    elementArray.Sort(Compare, &sortInfo);

    
    
    for (PRInt32 j = elementArray.Count() - 1; j >= 0; --j) {
        ElementInfo* elementInfo = elementArray[j];

        PRInt32 oldIndex;
        rv = gRDFC->IndexOf(mInner, aFolder, elementInfo->mElement, &oldIndex);
        if (NS_FAILED(rv))
            return rv;

        
        if (oldIndex != j + 1) {
            nsCOMPtr<nsIRDFResource> oldOrdinal;
            rv = gRDFC->IndexToOrdinalResource(oldIndex, getter_AddRefs(oldOrdinal));
            if (NS_FAILED(rv))
                return rv;

            nsCOMPtr<nsIRDFResource> newOrdinal;
            rv = gRDFC->IndexToOrdinalResource(j + 1, getter_AddRefs(newOrdinal));
            if (NS_FAILED(rv))
                return rv;

            
            
            nsCOMPtr<nsISimpleEnumerator> elements;
            rv = mInner->GetTargets(aFolder, oldOrdinal, PR_TRUE,
                                    getter_AddRefs(elements));

            PRBool hasMore = PR_FALSE;
            while (NS_SUCCEEDED(rv = elements->HasMoreElements(&hasMore)) &&
                   hasMore) {
                nsCOMPtr<nsISupports> supports;
                rv = elements->GetNext(getter_AddRefs(supports));
                if (NS_FAILED(rv))
                    return rv;

                nsCOMPtr<nsIRDFNode> element = do_QueryInterface(supports);
                if (element == elementInfo->mElement) {
                    rv = mInner->Unassert(aFolder, oldOrdinal, element);
                    if (NS_FAILED(rv))
                        return rv;

                    rv = mInner->Assert(aFolder, newOrdinal, element, PR_TRUE);
                    if (NS_FAILED(rv))
                        return rv;
                    break;
                }
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::SortFolder(nsIRDFResource* aFolder,
                               nsIRDFResource* aProperty,
                               PRInt32 aDirection,
                               PRBool aFoldersFirst,
                               PRBool aRecurse)
{
#ifdef DEBUG_varga
    PRIntervalTime startTime =  PR_IntervalNow();
#endif

    BeginUpdateBatch();
    SetPropagateChanges(PR_FALSE);
    nsresult rv = Sort(aFolder, aProperty, aDirection, aFoldersFirst,
                       aRecurse);
    SetPropagateChanges(PR_TRUE);
    EndUpdateBatch();

#ifdef DEBUG_varga
    PRIntervalTime endTime =  PR_IntervalNow();
    printf("Time spent in SortFolder(): %d msec\n", PR_IntervalToMilliseconds(endTime - startTime));
#endif

    return rv;
}

nsresult
nsBookmarksService::GetURLFromResource(nsIRDFResource* aResource,
                                       nsAString& aURL)
{
    NS_ENSURE_ARG(aResource);

    nsCOMPtr<nsIRDFNode> urlNode;
    nsresult rv = mInner->GetTarget(aResource, kNC_URL, PR_TRUE, getter_AddRefs(urlNode));
    if (NS_FAILED(rv))
        return rv;

    if (urlNode) {
        nsCOMPtr<nsIRDFLiteral> urlLiteral = do_QueryInterface(urlNode, &rv);
        if (NS_FAILED(rv))
            return rv;

        const PRUnichar* url = nsnull;
        rv = urlLiteral->GetValueConst(&url);
        if (NS_FAILED(rv))
            return rv;

        aURL.Assign(url);
    }

    return NS_OK;
}

nsresult
nsBookmarksService::CopyResource(nsIRDFResource* aOldResource,
                                 nsIRDFResource* aNewResource)
{
    
    
    nsCOMPtr<nsISimpleEnumerator> arcsOut;
    nsresult rv = mInner->ArcLabelsOut(aOldResource, getter_AddRefs(arcsOut));
    if (NS_FAILED(rv))
        return rv;

    while (1) {
        PRBool hasMoreArcsOut;
        rv = arcsOut->HasMoreElements(&hasMoreArcsOut);
        if (NS_FAILED(rv))
            return rv;

        if (!hasMoreArcsOut)
            break;

        nsCOMPtr<nsISupports> supports;
        rv = arcsOut->GetNext(getter_AddRefs(supports));
        if (NS_FAILED(rv))
            return rv;

        nsCOMPtr<nsIRDFResource> property = do_QueryInterface(supports);
        if (!property)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsIRDFNode> oldvalue;
        rv = mInner->GetTarget(aNewResource, property, PR_TRUE,
                               getter_AddRefs(oldvalue));
        if (NS_FAILED(rv))
            return rv;

        nsCOMPtr<nsIRDFNode> newvalue;
        rv = mInner->GetTarget(aOldResource, property, PR_TRUE,
                               getter_AddRefs(newvalue));
        if (NS_FAILED(rv))
            return rv;

        if (oldvalue) {
            if (newvalue) {
                 rv = mInner->Change(aNewResource, property, oldvalue, newvalue);
            }
            else {
                 rv = mInner->Unassert(aNewResource, property, oldvalue);
            }
        }
        else if (newvalue) {
            rv = mInner->Assert(aNewResource, property, newvalue, PR_TRUE);
        }

        if (NS_FAILED(rv))
            return rv;
    }

    
    nsCOMPtr<nsISimpleEnumerator> arcsIn;
    rv = mInner->ArcLabelsIn(aOldResource, getter_AddRefs(arcsIn));
    if (NS_FAILED(rv))
        return rv;
                                                                                
    while (1) {
        PRBool hasMoreArcsIn;
        rv = arcsIn->HasMoreElements(&hasMoreArcsIn);
        if (NS_FAILED(rv))
            return rv;

        if (!hasMoreArcsIn)
            break;

        nsCOMPtr<nsISupports> supports;
        rv = arcsIn->GetNext(getter_AddRefs(supports));
        if (NS_FAILED(rv))
            return rv;
                                                                                
        nsCOMPtr<nsIRDFResource> property = do_QueryInterface(supports);
        if (!property)
            return NS_ERROR_UNEXPECTED;

        nsCOMPtr<nsISimpleEnumerator> sources;
        rv = GetSources(property, aOldResource, PR_TRUE,
                        getter_AddRefs(sources));
        if (NS_FAILED(rv))
            return rv;

        while (1) {
            PRBool hasMoreSrcs;
            rv = sources->HasMoreElements(&hasMoreSrcs);
            if (NS_FAILED(rv))
                return rv;

            if (!hasMoreSrcs)
                break;

            nsCOMPtr<nsISupports> supports;
            rv = sources->GetNext(getter_AddRefs(supports));
            if (NS_FAILED(rv))
                return rv;

            nsCOMPtr<nsIRDFResource> source = do_QueryInterface(supports);
            if (!source)
                return NS_ERROR_UNEXPECTED;

            rv = mInner->Change(source, property, aOldResource, aNewResource);
            if (NS_FAILED(rv))
                return rv;
        }
    }

    return NS_OK;
}

nsresult
nsBookmarksService::SetNewPersonalToolbarFolder(nsIRDFResource* aFolder)
{
    nsCOMPtr<nsIRDFResource> tempResource;
    nsresult rv = gRDF->GetAnonymousResource(getter_AddRefs(tempResource));
    if (NS_FAILED(rv))
        return rv;

    rv = CopyResource(kNC_PersonalToolbarFolder, tempResource);
    if (NS_FAILED(rv))
        return rv;

    rv = CopyResource(aFolder, kNC_PersonalToolbarFolder);
    if (NS_FAILED(rv))
        return rv;

    return CopyResource(tempResource, aFolder);
}

NS_IMETHODIMP
nsBookmarksService::CreateBookmark(const PRUnichar* aName,
                                   const PRUnichar* aURL, 
                                   const PRUnichar* aShortcutURL,
                                   const PRUnichar* aDescription,
                                   const PRUnichar* aDocCharSet, 
                                   nsIRDFResource** aResult)
{
    
    nsCOMPtr<nsIRDFResource> bookmarkResource;
    nsresult rv = gRDF->GetAnonymousResource(getter_AddRefs(bookmarkResource));

    if (NS_FAILED(rv)) 
        return rv;

    
    nsCOMPtr<nsIRDFLiteral> nameLiteral;
    nsAutoString bookmarkName; 
    bookmarkName.Assign(aName);
    if (bookmarkName.IsEmpty()) {
        getLocaleString("NewBookmark", bookmarkName);

        rv = gRDF->GetLiteral(bookmarkName.get(), getter_AddRefs(nameLiteral));
        if (NS_FAILED(rv)) 
            return rv;
    }
    else
    {
        rv = gRDF->GetLiteral(aName, getter_AddRefs(nameLiteral));
        if (NS_FAILED(rv)) 
            return rv;
    }

    rv = mInner->Assert(bookmarkResource, kNC_Name, nameLiteral, PR_TRUE);
    if (NS_FAILED(rv)) 
        return rv;

    
    nsAutoString url;
    url.Assign(aURL);
    nsCOMPtr<nsIRDFLiteral> urlLiteral;
    rv = gRDF->GetLiteral(url.get(), getter_AddRefs(urlLiteral));
    if (NS_FAILED(rv)) 
        return rv;
    rv = mInner->Assert(bookmarkResource, kNC_URL, urlLiteral, PR_TRUE);
    if (NS_FAILED(rv)) 
        return rv;

    
    if (aShortcutURL && *aShortcutURL) {
        nsCOMPtr<nsIRDFLiteral> shortcutLiteral;
        rv = gRDF->GetLiteral(aShortcutURL, getter_AddRefs(shortcutLiteral));
        if (NS_FAILED(rv)) 
            return rv;
        rv = mInner->Assert(bookmarkResource, kNC_ShortcutURL, shortcutLiteral, PR_TRUE);
        if (NS_FAILED(rv)) 
            return rv;
    }

    
    if (aDescription && *aDescription) {
        nsCOMPtr<nsIRDFLiteral> descriptionLiteral;
        rv = gRDF->GetLiteral(aDescription, getter_AddRefs(descriptionLiteral));
        if (NS_FAILED(rv)) 
            return rv;
        rv = mInner->Assert(bookmarkResource, kNC_Description, descriptionLiteral, PR_TRUE);
        if (NS_FAILED(rv)) 
            return rv;
    }

    
    
    nsCOMPtr<nsIRDFDate> dateLiteral;
    rv = gRDF->GetDateLiteral(PR_Now(), getter_AddRefs(dateLiteral));
    if (NS_FAILED(rv)) 
        return rv;
    rv = mInner->Assert(bookmarkResource, kNC_BookmarkAddDate, dateLiteral, PR_TRUE);
    if (NS_FAILED(rv)) 
        return rv;

    
    nsAutoString charset; 
    charset.Assign(aDocCharSet);
    if (!charset.IsEmpty()) {
        nsCOMPtr<nsIRDFLiteral> charsetLiteral;
        rv = gRDF->GetLiteral(aDocCharSet, getter_AddRefs(charsetLiteral));
        if (NS_FAILED(rv)) 
            return rv;
        rv = mInner->Assert(bookmarkResource, kWEB_LastCharset, charsetLiteral, PR_TRUE);
        if (NS_FAILED(rv)) 
            return rv;
    }

    *aResult = bookmarkResource;
    NS_ADDREF(*aResult);

    return rv;
}

NS_IMETHODIMP
nsBookmarksService::CreateBookmarkInContainer(const PRUnichar* aName,
                                              const PRUnichar* aURL, 
                                              const PRUnichar* aShortcutURL, 
                                              const PRUnichar* aDescription, 
                                              const PRUnichar* aDocCharSet, 
                                              nsIRDFResource* aParentFolder,
                                              PRInt32 aIndex,
                                              nsIRDFResource** aResult)
{
    nsresult rv = CreateBookmark(aName, aURL, aShortcutURL, aDescription, aDocCharSet, aResult);
    if (NS_SUCCEEDED(rv))
        rv = InsertResource(*aResult, aParentFolder, aIndex);
    return rv;
}

NS_IMETHODIMP
nsBookmarksService::CreateSeparator(nsIRDFResource** aResult)
{
    nsresult rv;

    
    nsCOMPtr<nsIRDFResource> separatorResource;
    rv = gRDF->GetAnonymousResource(getter_AddRefs(separatorResource));
    if (NS_FAILED(rv)) 
        return rv;

    
    rv = mInner->Assert(separatorResource, kRDF_type, kNC_BookmarkSeparator, PR_TRUE);
    if (NS_FAILED(rv))
        return rv;

    *aResult = separatorResource;
    NS_ADDREF(*aResult);

    return rv;
}

NS_IMETHODIMP
nsBookmarksService::CloneResource(nsIRDFResource* aSource,
                                  nsIRDFResource** aResult)
{
    nsCOMPtr<nsIRDFResource> newResource;
    nsresult rv = gRDF->GetAnonymousResource(getter_AddRefs(newResource));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISimpleEnumerator> arcs;
    rv = mInner->ArcLabelsOut(aSource, getter_AddRefs(arcs));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore = PR_FALSE;
    while (NS_SUCCEEDED(arcs->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsISupports> supports;
        rv = arcs->GetNext(getter_AddRefs(supports));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIRDFResource> property = do_QueryInterface(supports, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
      
        
        
        PRBool isFolderType;
        rv = property->EqualsNode(kNC_FolderType, &isFolderType);
        NS_ENSURE_SUCCESS(rv, rv);
        if (isFolderType)
            continue;

        nsCOMPtr<nsIRDFNode> target;
        rv = mInner->GetTarget(aSource, property, PR_TRUE, getter_AddRefs(target));
        NS_ENSURE_SUCCESS(rv, rv);
 
        
        PRBool isOrdinal;
        rv = gRDFC->IsOrdinalProperty(property, &isOrdinal);
        NS_ENSURE_SUCCESS(rv, rv);

        if (isOrdinal) {
            nsCOMPtr<nsIRDFResource> oldChild = do_QueryInterface(target);
            nsCOMPtr<nsIRDFResource> newChild;
            rv = CloneResource(oldChild, getter_AddRefs(newChild));
            NS_ENSURE_SUCCESS(rv, rv);

            rv = mInner->Assert(newResource, property, newChild, PR_TRUE);
        }
        else {
            rv = mInner->Assert(newResource, property, target, PR_TRUE);
        }
        NS_ENSURE_SUCCESS(rv, rv);
    }

    NS_ADDREF(*aResult = newResource);

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::AddBookmarkImmediately(const PRUnichar *aURI,
                                           const PRUnichar *aTitle, 
                                           PRInt32 aBookmarkType, 
                                           const PRUnichar *aCharset)
{
    nsresult rv;

    
    nsCOMPtr<nsIRDFResource> bookmarkFolder = kNC_NewBookmarkFolder;

    switch(aBookmarkType)
    {
    case BOOKMARK_SEARCH_TYPE:
    case BOOKMARK_FIND_TYPE:
        bookmarkFolder = kNC_NewSearchFolder;
        break;
    }

    nsCOMPtr<nsIRDFResource> destinationFolder;
    rv = getFolderViaHint(bookmarkFolder, PR_TRUE, getter_AddRefs(destinationFolder));
    if (NS_FAILED(rv)) 
        return rv;

    nsCOMPtr<nsIRDFResource> bookmark;
    return CreateBookmarkInContainer(aTitle, aURI, nsnull, nsnull, aCharset, destinationFolder, -1, 
                                     getter_AddRefs(bookmark));
}

NS_IMETHODIMP
nsBookmarksService::IsBookmarkedResource(nsIRDFResource *bookmark, PRBool *isBookmarkedFlag)
{
    if (!bookmark)      return NS_ERROR_UNEXPECTED;
    if (!isBookmarkedFlag)  return NS_ERROR_UNEXPECTED;
    if (!mInner)        return NS_ERROR_UNEXPECTED;

    
    if (bookmark == kNC_BookmarksRoot)
    {
        *isBookmarkedFlag = PR_TRUE;
        return NS_OK;
    }

    *isBookmarkedFlag = PR_FALSE;

    
    nsresult rv;
    nsCOMPtr<nsISimpleEnumerator>   enumerator;
    if (NS_FAILED(rv = mInner->ArcLabelsIn(bookmark, getter_AddRefs(enumerator))))
        return rv;
        
    PRBool  more = PR_TRUE;
    while(NS_SUCCEEDED(rv = enumerator->HasMoreElements(&more))
        && (more == PR_TRUE))
    {
        nsCOMPtr<nsISupports>       isupports;
        if (NS_FAILED(rv = enumerator->GetNext(getter_AddRefs(isupports))))
            break;
        nsCOMPtr<nsIRDFResource>    property = do_QueryInterface(isupports);
        if (!property)  continue;

        PRBool  flag = PR_FALSE;
        if (NS_FAILED(rv = gRDFC->IsOrdinalProperty(property, &flag)))  continue;
        if (flag == PR_TRUE)
        {
            *isBookmarkedFlag = PR_TRUE;
            break;
        }
    }
    return rv;
}

NS_IMETHODIMP
nsBookmarksService::IsBookmarked(const char* aURL, PRBool* aIsBookmarked)
{
    NS_ENSURE_ARG(aURL);
    NS_ENSURE_ARG_POINTER(aIsBookmarked);

    if (!mInner)
        return NS_ERROR_UNEXPECTED;

    *aIsBookmarked = PR_FALSE;

    nsCOMPtr<nsIRDFLiteral> urlLiteral;
    nsresult rv = gRDF->GetLiteral(NS_ConvertUTF8toUTF16(aURL).get(),
                                   getter_AddRefs(urlLiteral));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIRDFResource> bookmark;
    rv = GetSource(kNC_URL, urlLiteral, PR_TRUE, getter_AddRefs(bookmark));
    if (NS_FAILED(rv))
        return rv;

    return IsBookmarkedResource(bookmark, aIsBookmarked);
}

NS_IMETHODIMP
nsBookmarksService::RequestCharset(nsIWebNavigation* aWebNavigation,
                                   nsIChannel* aChannel,
                                   PRBool* aWantCharset,
                                   nsISupports** aClosure,
                                   nsACString& aResult)
{
    if (!mInner)
        return NS_ERROR_UNEXPECTED;
    *aWantCharset = PR_FALSE;
    *aClosure = nsnull;

    nsresult rv;
    
    nsCOMPtr<nsIURI> uri;
    rv = aChannel->GetURI(getter_AddRefs(uri));

    nsCAutoString urlSpec;
    uri->GetSpec(urlSpec);
   
    nsCOMPtr<nsIRDFLiteral> urlLiteral;
    rv = gRDF->GetLiteral(NS_ConvertUTF8toUTF16(urlSpec).get(),
                                   getter_AddRefs(urlLiteral));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIRDFResource> bookmark;
    rv = GetSource(kNC_URL, urlLiteral, PR_TRUE, getter_AddRefs(bookmark));
    if (NS_FAILED(rv))
        return rv;

    if (bookmark) {
        
        

        nsCOMPtr<nsIRDFNode> nodeType;
        GetSynthesizedType(bookmark, getter_AddRefs(nodeType));
        if (nodeType == kNC_Bookmark) {
            nsCOMPtr<nsIRDFNode>  charsetNode;
            rv = mInner->GetTarget(bookmark, kWEB_LastCharset, PR_TRUE,
                                   getter_AddRefs(charsetNode));
            if (NS_FAILED(rv))
                return rv;

            if (charsetNode) {
                nsCOMPtr<nsIRDFLiteral> charsetLiteral = do_QueryInterface(charsetNode);
                if (charsetLiteral) {
                    const PRUnichar* charset;
                    charsetLiteral->GetValueConst(&charset);
                    LossyCopyUTF16toASCII(charset, aResult);
                    
                    return NS_OK;
                }
            }
        }
    }

    aResult.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::NotifyResolvedCharset(const nsACString& aCharset,
                                          nsISupports* aClosure)
{
    NS_ERROR("Unexpected call to NotifyResolvedCharset -- we never set aWantCharset to true!");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsBookmarksService::UpdateBookmarkIcon(const char *aURL, const PRUnichar *aIconURL)
{
    nsCOMPtr<nsIRDFLiteral> urlLiteral;
    nsresult rv = gRDF->GetLiteral(NS_ConvertUTF8toUTF16(aURL).get(),
                                   getter_AddRefs(urlLiteral));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsISimpleEnumerator> bookmarks;
    rv = GetSources(kNC_URL, urlLiteral, PR_TRUE, getter_AddRefs(bookmarks));
    if (NS_FAILED(rv))
        return rv;

    PRBool hasMoreBookmarks = PR_FALSE;
    while (NS_SUCCEEDED(rv = bookmarks->HasMoreElements(&hasMoreBookmarks)) &&
           hasMoreBookmarks) {
        nsCOMPtr<nsISupports> supports;
        rv = bookmarks->GetNext(getter_AddRefs(supports));
        if (NS_FAILED(rv)) 
            return rv;

        nsCOMPtr<nsIRDFResource> bookmark = do_QueryInterface(supports);
        if (bookmark) {
            nsCOMPtr<nsIRDFNode> iconNode;
            rv = ProcessCachedBookmarkIcon(bookmark, aIconURL,
                                           getter_AddRefs(iconNode));
            if (NS_FAILED(rv))
                return rv;

            if (iconNode) {
                
                (void)OnAssert(this, bookmark, kNC_Icon, iconNode);
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::RemoveBookmarkIcon(const char *aURL, const PRUnichar *aIconURL)
{
    nsCOMPtr<nsIRDFLiteral> urlLiteral;
    nsresult rv = gRDF->GetLiteral(NS_ConvertUTF8toUTF16(aURL).get(),
                                   getter_AddRefs(urlLiteral));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsISimpleEnumerator> bookmarks;
    rv = GetSources(kNC_URL, urlLiteral, PR_TRUE, getter_AddRefs(bookmarks));
    if (NS_FAILED(rv))
        return rv;

    PRBool hasMoreBookmarks = PR_FALSE;
    while (NS_SUCCEEDED(rv = bookmarks->HasMoreElements(&hasMoreBookmarks)) &&
           hasMoreBookmarks) {
        nsCOMPtr<nsISupports> supports;
        rv = bookmarks->GetNext(getter_AddRefs(supports));
        if (NS_FAILED(rv)) 
            return rv;

        nsCOMPtr<nsIRDFResource> bookmark = do_QueryInterface(supports);
        if (bookmark) {
            nsCOMPtr<nsIRDFLiteral> iconLiteral;
            rv = gRDF->GetLiteral(aIconURL, getter_AddRefs(iconLiteral));
            if (NS_FAILED(rv)) 
                return rv;

            PRBool hasThisIconURL = PR_FALSE;
            rv = mInner->HasAssertion(bookmark, kNC_Icon, iconLiteral, PR_TRUE,
                                      &hasThisIconURL);
            if (NS_FAILED(rv)) 
                return rv;

            if (hasThisIconURL) {
                (void)mInner->Unassert(bookmark, kNC_Icon, iconLiteral);

                mDirty = PR_TRUE;
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::UpdateLastVisitedDate(const char *aURL,
                                          const PRUnichar *aCharset)
{
    NS_PRECONDITION(aURL != nsnull, "null ptr");
    if (! aURL)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aCharset != nsnull, "null ptr");
    if (! aCharset)
        return NS_ERROR_NULL_POINTER;

    nsCOMPtr<nsIRDFLiteral> urlLiteral;
    nsresult rv = gRDF->GetLiteral(NS_ConvertUTF8toUTF16(aURL).get(),
                                   getter_AddRefs(urlLiteral));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsISimpleEnumerator> bookmarks;
    rv = GetSources(kNC_URL, urlLiteral, PR_TRUE, getter_AddRefs(bookmarks));
    if (NS_FAILED(rv))
        return rv;

    PRBool hasMoreBookmarks = PR_FALSE;
    while (NS_SUCCEEDED(rv = bookmarks->HasMoreElements(&hasMoreBookmarks)) &&
           hasMoreBookmarks) {
        nsCOMPtr<nsISupports> supports;
        rv = bookmarks->GetNext(getter_AddRefs(supports));
        if (NS_FAILED(rv)) 
            return rv;

        nsCOMPtr<nsIRDFResource> bookmark = do_QueryInterface(supports);
        if (bookmark) {
            
            

            nsCOMPtr<nsIRDFNode> nodeType;
            GetSynthesizedType(bookmark, getter_AddRefs(nodeType));
            if (nodeType == kNC_Bookmark) {
                nsCOMPtr<nsIRDFDate> now;
                rv = gRDF->GetDateLiteral(PR_Now(), getter_AddRefs(now));
                if (NS_FAILED(rv))
                    return rv;

                nsCOMPtr<nsIRDFNode> lastMod;
                rv = mInner->GetTarget(bookmark, kWEB_LastVisitDate, PR_TRUE,
                                       getter_AddRefs(lastMod));
                if (NS_FAILED(rv))
                    return rv;

                if (lastMod) {
                    rv = mInner->Change(bookmark, kWEB_LastVisitDate, lastMod, now);
                }
                else {
                    rv = mInner->Assert(bookmark, kWEB_LastVisitDate, now, PR_TRUE);
                }
                if (NS_FAILED(rv))
                    return rv;

                
                if (aCharset && *aCharset) {
                    nsCOMPtr<nsIRDFLiteral> charsetliteral;
                    rv = gRDF->GetLiteral(aCharset,
                                          getter_AddRefs(charsetliteral));
                    if (NS_FAILED(rv))
                        return rv;

                    nsCOMPtr<nsIRDFNode> charsetNode;
                    rv = mInner->GetTarget(bookmark, kWEB_LastCharset, PR_TRUE,
                                           getter_AddRefs(charsetNode));
                    if (NS_FAILED(rv))
                        return rv;

                    if (charsetNode) {
                        rv = mInner->Change(bookmark, kWEB_LastCharset,
                                            charsetNode, charsetliteral);
                    }
                    else {
                        rv = mInner->Assert(bookmark, kWEB_LastCharset,
                                            charsetliteral, PR_TRUE);
                    }
                    if (NS_FAILED(rv))
                        return rv;
                } 

                
                nsCOMPtr<nsIRDFNode> statusNode;
                rv = mInner->GetTarget(bookmark, kWEB_Status, PR_TRUE,
                                       getter_AddRefs(statusNode));
                if (NS_SUCCEEDED(rv) && statusNode) {
                    rv = mInner->Unassert(bookmark, kWEB_Status, statusNode);
                    NS_ASSERTION(rv == NS_RDF_ASSERTION_ACCEPTED, "unable to Unassert changed status");
                }

                mDirty = PR_TRUE;
            }
        }
    }

    return rv;
}

nsresult
nsBookmarksService::GetSynthesizedType(nsIRDFResource *aNode, nsIRDFNode **aType)
{
    *aType = nsnull;
    nsresult rv = mInner->GetTarget(aNode, kRDF_type, PR_TRUE, aType);
    if (NS_FAILED(rv) || (rv == NS_RDF_NO_VALUE))
    {
        
        
        
        PRBool isContainer = PR_FALSE;
        PRBool isBookmarkedFlag = PR_FALSE;
        (void)gRDFC->IsSeq(mInner, aNode, &isContainer);

        if (isContainer)
        {
            *aType =  kNC_Folder;
        }
        else if (NS_SUCCEEDED(rv = IsBookmarkedResource(aNode,
                                                        &isBookmarkedFlag)) && (isBookmarkedFlag == PR_TRUE))
        {
            *aType = kNC_Bookmark;
        }
#ifdef XP_BEOS
        else
        {
            
            *aType = kNC_URL;
        }
#endif
        NS_IF_ADDREF(*aType);
    }
    return NS_OK;
}

nsresult
nsBookmarksService::UpdateBookmarkLastModifiedDate(nsIRDFResource *aSource)
{
    nsCOMPtr<nsIRDFDate>    now;
    nsresult        rv;

    if (NS_SUCCEEDED(rv = gRDF->GetDateLiteral(PR_Now(), getter_AddRefs(now))))
    {
        nsCOMPtr<nsIRDFNode>    lastMod;

        
        

        if (NS_SUCCEEDED(rv = mInner->GetTarget(aSource, kWEB_LastModifiedDate, PR_TRUE,
            getter_AddRefs(lastMod))) && (rv != NS_RDF_NO_VALUE))
        {
            rv = mInner->Change(aSource, kWEB_LastModifiedDate, lastMod, now);
        }
        else
        {
            rv = mInner->Assert(aSource, kWEB_LastModifiedDate, now, PR_TRUE);
        }
    }
    return rv;
}

NS_IMETHODIMP
nsBookmarksService::GetLastCharset(const nsACString &aURL, nsAString &aCharset)
{
    aCharset.Truncate(); 

    nsCOMPtr<nsIRDFLiteral> urlLiteral;
    nsresult rv = gRDF->GetLiteral(NS_ConvertUTF8toUTF16(aURL).get(),
                                   getter_AddRefs(urlLiteral));

    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIRDFResource> bookmark;
    rv = GetSource(kNC_URL, urlLiteral, PR_TRUE, getter_AddRefs(bookmark));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIRDFNode> nodeType;
    GetSynthesizedType(bookmark, getter_AddRefs(nodeType));
    if (nodeType == kNC_Bookmark) {
        nsCOMPtr<nsIRDFNode> charsetNode;
        rv = GetTarget(bookmark, kWEB_LastCharset, PR_TRUE,
                       getter_AddRefs(charsetNode));
        if (NS_FAILED(rv))
            return rv;

        if (charsetNode) {
            nsCOMPtr<nsIRDFLiteral> charsetData(do_QueryInterface(charsetNode));
            if (charsetData) {
                const PRUnichar *charset;
                charsetData->GetValueConst(&charset);
                aCharset.Assign(charset);
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::ResolveKeyword(const PRUnichar *aUserInput, char **aShortcutURL)
{
    NS_PRECONDITION(aUserInput != nsnull, "null ptr");
    if (! aUserInput)
        return NS_ERROR_NULL_POINTER;

    NS_PRECONDITION(aShortcutURL != nsnull, "null ptr");
    if (! aShortcutURL)
        return NS_ERROR_NULL_POINTER;

    
    nsAutoString shortcut(aUserInput);
    ToLowerCase(shortcut);

    nsCOMPtr<nsIRDFLiteral> shortcutLiteral;
    nsresult rv = gRDF->GetLiteral(shortcut.get(),
                                   getter_AddRefs(shortcutLiteral));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIRDFResource> source;
    rv = GetSource(kNC_ShortcutURL, shortcutLiteral, PR_TRUE,
                   getter_AddRefs(source));
    if (NS_FAILED(rv))
        return rv;

    if (source) {
        nsAutoString url;
        rv = GetURLFromResource(source, url);
        if (NS_FAILED(rv))
           return rv;

        if (!url.IsEmpty()) {
            *aShortcutURL = ToNewUTF8String(url);
            return NS_OK;
        }
    }

    *aShortcutURL = nsnull;
    return NS_RDF_NO_VALUE;
}

#ifdef XP_WIN

static void ResolveShortcut(nsIFile* aFile, nsACString& aURI)
{
    nsCOMPtr<nsIFileProtocolHandler> fph;
    nsresult rv = NS_GetFileProtocolHandler(getter_AddRefs(fph));
    if (NS_FAILED(rv))
        return;

    nsCOMPtr<nsIURI> uri;
    rv = fph->ReadURLFile(aFile, getter_AddRefs(uri));
    if (NS_FAILED(rv))
        return;

    uri->GetSpec(aURI);
} 

nsresult
nsBookmarksService::ParseFavoritesFolder(nsIFile* aDirectory, nsIRDFResource* aParentResource)
{
    nsresult rv;

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = aDirectory->GetDirectoryEntries(getter_AddRefs(entries));
    if (NS_FAILED(rv)) 
        return rv;

    do
    {
        PRBool hasMore = PR_FALSE;
        rv = entries->HasMoreElements(&hasMore);
        if (NS_FAILED(rv) || !hasMore) 
            break;

        nsCOMPtr<nsISupports> supp;
        rv = entries->GetNext(getter_AddRefs(supp));
        if (NS_FAILED(rv)) 
            break;

        nsCOMPtr<nsIFile> currFile(do_QueryInterface(supp));
    
        nsCOMPtr<nsIURI> uri;
        rv = NS_NewFileURI(getter_AddRefs(uri), currFile);
        if (NS_FAILED(rv)) 
            break;

        nsAutoString bookmarkName;
        currFile->GetLeafName(bookmarkName);

        PRBool isSymlink = PR_FALSE;
        PRBool isDir = PR_FALSE;

        currFile->IsSymlink(&isSymlink);
        currFile->IsDirectory(&isDir);

        if (isSymlink)
        {
            
            
            

            
            nsCAutoString path;
            rv = currFile->GetNativeTarget(path);
            if (NS_FAILED(rv)) 
                continue;

            nsCOMPtr<nsILocalFile> localFile;
            rv = NS_NewNativeLocalFile(path, PR_TRUE, getter_AddRefs(localFile));
            if (NS_FAILED(rv)) 
                continue;

            
            
            rv = localFile->IsDirectory(&isDir);
            NS_ENSURE_SUCCESS(rv, rv);
            if (!isDir)
                continue;

            nsCAutoString spec;
            nsCOMPtr<nsIFile> filePath(localFile);
            
            rv = NS_GetURLSpecFromFile(filePath, spec);
            if (NS_FAILED(rv)) 
                continue;

            
            NS_NAMED_LITERAL_STRING(lnkExt, ".lnk");
            PRInt32 lnkExtStart = bookmarkName.Length() - lnkExt.Length();
            if (StringEndsWith(bookmarkName, lnkExt,
                  nsCaseInsensitiveStringComparator()))
                bookmarkName.Truncate(lnkExtStart);

            nsCOMPtr<nsIRDFResource> bookmark;
            
            
            
            
            CreateBookmarkInContainer(bookmarkName.get(),
                                      NS_ConvertUTF8toUTF16(spec).get(),
                                      nsnull, nsnull, nsnull, aParentResource,
                                      -1, getter_AddRefs(bookmark));
            if (NS_FAILED(rv)) 
                continue;
        }
        else if (isDir)
        {
            nsCOMPtr<nsIRDFResource> folder;
            rv = CreateFolderInContainer(bookmarkName.get(), aParentResource, -1, getter_AddRefs(folder));
            if (NS_FAILED(rv)) 
                continue;

            rv = ParseFavoritesFolder(currFile, folder);
            if (NS_FAILED(rv)) 
                continue;
        }
        else
        {
            nsCOMPtr<nsIURL> url(do_QueryInterface(uri));
            nsCAutoString extension;

            url->GetFileExtension(extension);
            if (!extension.LowerCaseEqualsLiteral("url"))
                continue;

            nsAutoString name(Substring(bookmarkName, 0, 
                                        bookmarkName.Length() - extension.Length() - 1));
     

            nsCAutoString resolvedURL;
            ResolveShortcut(currFile, resolvedURL);

            nsCOMPtr<nsIRDFResource> bookmark;
            
            rv = CreateBookmarkInContainer(name.get(),
                 NS_ConvertUTF8toUTF16(resolvedURL).get(),
                 nsnull, nsnull, nsnull, aParentResource, -1,
                 getter_AddRefs(bookmark));
            if (NS_FAILED(rv)) 
                continue;
        }
    }
    while (1);

    return rv;
}
#endif

NS_IMETHODIMP
nsBookmarksService::ImportSystemBookmarks(nsIRDFResource* aParentFolder)
{
    gImportedSystemBookmarks = PR_TRUE;

#if defined(XP_WIN)
    nsresult rv;

    nsCOMPtr<nsIProperties> fileLocator(do_GetService("@mozilla.org/file/directory_service;1", &rv));
    if (NS_FAILED(rv)) 
        return rv;

    nsCOMPtr<nsIFile> favoritesDirectory;
    fileLocator->Get("Favs", NS_GET_IID(nsIFile), getter_AddRefs(favoritesDirectory));

    
    
    
    
    if (favoritesDirectory) 
        return ParseFavoritesFolder(favoritesDirectory, aParentFolder);
#elif defined(XP_MAC) || defined(XP_MACOSX)
    nsCOMPtr<nsIFile> ieFavoritesFile;
    nsresult rv = NS_GetSpecialDirectory(NS_MAC_PREFS_DIR, getter_AddRefs(ieFavoritesFile));
    NS_ENSURE_SUCCESS(rv, rv);

    ieFavoritesFile->Append(NS_LITERAL_STRING("Explorer"));
    ieFavoritesFile->Append(NS_LITERAL_STRING("Favorites.html"));

    BookmarkParser parser;
    parser.Init(ieFavoritesFile, mInner);
    BeginUpdateBatch();
    parser.Parse(aParentFolder, kNC_Bookmark);
    EndUpdateBatch();
#endif

    return NS_OK;
}

#if defined(XP_WIN) || defined(XP_MAC) || defined(XP_MACOSX)
void
nsBookmarksService::HandleSystemBookmarks(nsIRDFNode* aNode) 
{
    if (!gImportedSystemBookmarks && aNode == kNC_SystemBookmarksStaticRoot)
    {
        PRBool isSeq = PR_TRUE;
        gRDFC->IsSeq(mInner, kNC_SystemBookmarksStaticRoot, &isSeq);
    
        if (!isSeq)
        {
            nsCOMPtr<nsIRDFContainer> ctr;
            gRDFC->MakeSeq(mInner, kNC_SystemBookmarksStaticRoot, getter_AddRefs(ctr));
      
            ImportSystemBookmarks(kNC_SystemBookmarksStaticRoot);
        }
    }
#if defined(XP_MAC) || defined(XP_MACOSX)
    
    
    else if ((aNode == kNC_IEFavoritesRoot) && (mIEFavoritesAvailable == PR_FALSE))
        ReadFavorites();
#endif
}
#endif

NS_IMETHODIMP
nsBookmarksService::GetTransactionManager(nsITransactionManager** aTransactionManager)
{
    NS_ENSURE_ARG_POINTER(aTransactionManager);

    NS_ADDREF(*aTransactionManager = mTransactionManager);

    return NS_OK;
}




NS_IMETHODIMP
nsBookmarksService::GetURI(char* *aURI)
{
    *aURI = nsCRT::strdup("rdf:bookmarks");
    if (! *aURI)
        return NS_ERROR_OUT_OF_MEMORY;

    return NS_OK;
}

static PRBool
isBookmarkCommand(nsIRDFResource *r)
{
    PRBool      isBookmarkCommandFlag = PR_FALSE;
    const char  *uri = nsnull;
    
    if (NS_SUCCEEDED(r->GetValueConst( &uri )) && (uri))
    {
        if (!strncmp(uri, kBookmarkCommand, sizeof(kBookmarkCommand) - 1))
        {
            isBookmarkCommandFlag = PR_TRUE;
        }
    }
    return isBookmarkCommandFlag;
}

NS_IMETHODIMP
nsBookmarksService::GetTarget(nsIRDFResource* aSource,
                              nsIRDFResource* aProperty,
                              PRBool aTruthValue,
                              nsIRDFNode** aTarget)
{
    *aTarget = nsnull;

    nsresult    rv;

    if (aTruthValue && (aProperty == kRDF_type))
    {
        rv = GetSynthesizedType(aSource, aTarget);
        return rv;
    }
    else if (aTruthValue && isBookmarkCommand(aSource) && (aProperty == kNC_Name))
    {
        nsAutoString    name;
        if (aSource == kNC_BookmarkCommand_NewBookmark)
            getLocaleString("NewBookmark", name);
        else if (aSource == kNC_BookmarkCommand_NewFolder)
            getLocaleString("NewFolder", name);
        else if (aSource == kNC_BookmarkCommand_NewSeparator)
            getLocaleString("NewSeparator", name);
        else if (aSource == kNC_BookmarkCommand_DeleteBookmark)
            getLocaleString("DeleteBookmark", name);
        else if (aSource == kNC_BookmarkCommand_DeleteBookmarkFolder)
            getLocaleString("DeleteFolder", name);
        else if (aSource == kNC_BookmarkCommand_DeleteBookmarkSeparator)
            getLocaleString("DeleteSeparator", name);
        else if (aSource == kNC_BookmarkCommand_SetNewBookmarkFolder)
            getLocaleString("SetNewBookmarkFolder", name);
        else if (aSource == kNC_BookmarkCommand_SetPersonalToolbarFolder)
            getLocaleString("SetPersonalToolbarFolder", name);
        else if (aSource == kNC_BookmarkCommand_SetNewSearchFolder)
            getLocaleString("SetNewSearchFolder", name);
        else if (aSource == kNC_BookmarkCommand_Import)
            getLocaleString("Import", name);
        else if (aSource == kNC_BookmarkCommand_Export)
            getLocaleString("Export", name);

        if (!name.IsEmpty())
        {
            *aTarget = nsnull;
            nsCOMPtr<nsIRDFLiteral> literal;
            if (NS_FAILED(rv = gRDF->GetLiteral(name.get(), getter_AddRefs(literal))))
                return rv;
            *aTarget = literal;
            NS_IF_ADDREF(*aTarget);
            return rv;
        }
    }
    else if (aProperty == kNC_Icon)
    {
        rv = ProcessCachedBookmarkIcon(aSource, nsnull, aTarget);
        return rv;
    }

    rv = mInner->GetTarget(aSource, aProperty, aTruthValue, aTarget);
    return rv;
}

nsresult
nsBookmarksService::ProcessCachedBookmarkIcon(nsIRDFResource* aSource,
                                              const PRUnichar *iconURL, nsIRDFNode** aTarget)
{
    *aTarget = nsnull;

    if (!mBrowserIcons)
    {
        return NS_RDF_NO_VALUE;
    }

    

    nsCOMPtr<nsIRDFNode> nodeType;
    GetSynthesizedType(aSource, getter_AddRefs(nodeType));
    if ((nodeType != kNC_Bookmark) && (nodeType != kNC_IEFavorite))
    {
        return NS_RDF_NO_VALUE;
    }

    nsresult rv;
    nsCAutoString path;
    nsCOMPtr<nsIRDFNode>    oldIconNode;

    
    if (iconURL)
    {
        path.AssignWithConversion(iconURL);

        nsCOMPtr<nsIRDFLiteral> iconLiteral;
        if (NS_FAILED(rv = gRDF->GetLiteral(iconURL, getter_AddRefs(iconLiteral))))
        {
            return rv;
        }

        rv = mInner->GetTarget(aSource, kNC_Icon, PR_TRUE, getter_AddRefs(oldIconNode));
        if (NS_SUCCEEDED(rv) && (rv != NS_RDF_NO_VALUE) && (oldIconNode))
        {
            (void)mInner->Unassert(aSource, kNC_Icon, oldIconNode);
        }
        (void)mInner->Assert(aSource, kNC_Icon, iconLiteral, PR_TRUE);

        mDirty = PR_TRUE;
    }
    else
    {
        
        rv = mInner->GetTarget(aSource, kNC_Icon, PR_TRUE, getter_AddRefs(oldIconNode));
    }
    
    if (oldIconNode)
    {
        nsCOMPtr<nsIRDFLiteral> tempLiteral = do_QueryInterface(oldIconNode);
        if (tempLiteral)
        {
            const PRUnichar *uni = nsnull;
            tempLiteral->GetValueConst(&uni);
            if (uni)    path.AssignWithConversion(uni);
        }
    }

    PRBool forceLoad = mAlwaysLoadIcons;

    
    if (path.IsEmpty())
    {
        const char  *uri;
        forceLoad = PR_FALSE;
        if (NS_FAILED(rv = aSource->GetValueConst( &uri )))
        {
            return rv;
        }

        nsCOMPtr<nsIURI>    nsURI;
        if (NS_FAILED(rv = mNetService->NewURI(nsDependentCString(uri), nsnull, nsnull, getter_AddRefs(nsURI))))
        {
            return rv;
        }
        
        
        PRBool  isHTTP = PR_FALSE;
        nsURI->SchemeIs("http", &isHTTP);
        if (!isHTTP)
        {
            nsURI->SchemeIs("https", &isHTTP);
        }
        if (!isHTTP)
        {
            return NS_RDF_NO_VALUE;
        }

        nsCAutoString prePath;
        if (NS_FAILED(rv = nsURI->GetPrePath(prePath)))
        {
            return rv;
        }
        path.Assign(prePath);
        path.Append("/favicon.ico");
    }

    if (!forceLoad) {
        
        
        if (!mCacheSession)
        {
            return NS_RDF_NO_VALUE;
        }
        nsCOMPtr<nsICacheEntryDescriptor> entry;
        rv = mCacheSession->OpenCacheEntry(path, nsICache::ACCESS_READ,
                                           nsICache::NON_BLOCKING, getter_AddRefs(entry));
        if (NS_FAILED(rv) || (!entry))
        {
            return NS_RDF_NO_VALUE;
        }
        if (entry) 
        {
            PRUint32 expTime;
            entry->GetExpirationTime(&expTime);
            if (expTime != PR_UINT32_MAX)
                entry->SetExpirationTime(PR_UINT32_MAX);
        }
        entry->Close();
    }

    
    nsAutoString litStr;
    litStr.AssignWithConversion(path.get());
    nsCOMPtr<nsIRDFLiteral> literal;
    if (NS_FAILED(rv = gRDF->GetLiteral(litStr.get(), getter_AddRefs(literal))))
    {
        return rv;
    }
    *aTarget = literal;
    NS_IF_ADDREF(*aTarget);
    return NS_OK;
}

void
nsBookmarksService::AnnotateBookmarkSchedule(nsIRDFResource* aSource, PRBool scheduleFlag)
{
    if (scheduleFlag)
    {
        PRBool exists = PR_FALSE;
        if (NS_SUCCEEDED(mInner->HasAssertion(aSource, kWEB_ScheduleActive,
                                              kTrueLiteral, PR_TRUE, &exists)) && (!exists))
        {
            (void)mInner->Assert(aSource, kWEB_ScheduleActive, kTrueLiteral, PR_TRUE);
        }
    }
    else
    {
        (void)mInner->Unassert(aSource, kWEB_ScheduleActive, kTrueLiteral);
    }
}

NS_IMETHODIMP
nsBookmarksService::Assert(nsIRDFResource* aSource,
                           nsIRDFResource* aProperty,
                           nsIRDFNode* aTarget,
                           PRBool aTruthValue)
{
    nsresult rv = NS_RDF_ASSERTION_REJECTED;

    if (CanAccept(aSource, aProperty, aTarget))
    {
        rv = mInner->Assert(aSource, aProperty, aTarget, aTruthValue);
        if (NS_FAILED(rv))
            return rv;

        UpdateBookmarkLastModifiedDate(aSource);
            
        if (aProperty == kWEB_Schedule) {
              AnnotateBookmarkSchedule(aSource, PR_TRUE);
        }
    }

    return rv;
}

NS_IMETHODIMP
nsBookmarksService::Unassert(nsIRDFResource* aSource,
                             nsIRDFResource* aProperty,
                             nsIRDFNode* aTarget)
{
    nsresult rv = NS_RDF_ASSERTION_REJECTED;

    if (CanAccept(aSource, aProperty, aTarget)) {
        rv = mInner->Unassert(aSource, aProperty, aTarget);
        if (NS_FAILED(rv))
            return rv;

        UpdateBookmarkLastModifiedDate(aSource);

        if (aProperty == kWEB_Schedule) {
            AnnotateBookmarkSchedule(aSource, PR_FALSE);
        }
    }

    return rv;
}

NS_IMETHODIMP
nsBookmarksService::Change(nsIRDFResource* aSource,
                           nsIRDFResource* aProperty,
                           nsIRDFNode* aOldTarget,
                           nsIRDFNode* aNewTarget)
{
    nsresult rv = NS_RDF_ASSERTION_REJECTED;

    if (CanAccept(aSource, aProperty, aNewTarget)) {
        rv = mInner->Change(aSource, aProperty, aOldTarget, aNewTarget);
        if (NS_FAILED(rv))
            return rv;

        UpdateBookmarkLastModifiedDate(aSource);

        if (aProperty == kWEB_Schedule) {
            AnnotateBookmarkSchedule(aSource, PR_TRUE);
        }
    }

    return rv;
}

NS_IMETHODIMP
nsBookmarksService::Move(nsIRDFResource* aOldSource,
                         nsIRDFResource* aNewSource,
                         nsIRDFResource* aProperty,
                         nsIRDFNode* aTarget)
{
    nsresult    rv = NS_RDF_ASSERTION_REJECTED;

    if (CanAccept(aNewSource, aProperty, aTarget))
    {
        rv = mInner->Move(aOldSource, aNewSource, aProperty, aTarget);
        if (NS_FAILED(rv))
            return rv;

        UpdateBookmarkLastModifiedDate(aOldSource);
        UpdateBookmarkLastModifiedDate(aNewSource);
    }
    return rv;
}

NS_IMETHODIMP
nsBookmarksService::HasAssertion(nsIRDFResource* source,
                nsIRDFResource* property,
                nsIRDFNode* target,
                PRBool tv,
                PRBool* hasAssertion)
{
#if defined(XP_WIN) || defined(XP_MAC) || defined(XP_MACOSX)
    HandleSystemBookmarks(source);
#endif

    return mInner->HasAssertion(source, property, target, tv, hasAssertion);
}

NS_IMETHODIMP
nsBookmarksService::AddObserver(nsIRDFObserver* aObserver)
{
    if (! aObserver)
        return NS_ERROR_NULL_POINTER;

    if (! mObservers.AppendObject(aObserver)) {
        return NS_ERROR_FAILURE;
    }
    
    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::RemoveObserver(nsIRDFObserver* aObserver)
{
    if (! aObserver)
        return NS_ERROR_NULL_POINTER;

    mObservers.RemoveObject(aObserver);

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *_retval)
{
#if defined(XP_WIN) || defined(XP_MAC) || defined(XP_MACOSX)
    HandleSystemBookmarks(aNode);
#endif

    return mInner->HasArcIn(aNode, aArc, _retval);
}

NS_IMETHODIMP
nsBookmarksService::HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, PRBool *_retval)
{
#if defined(XP_WIN) || defined(XP_MAC) || defined(XP_MACOSX)
    HandleSystemBookmarks(aSource);
#endif
  
    return mInner->HasArcOut(aSource, aArc, _retval);
}

NS_IMETHODIMP
nsBookmarksService::ArcLabelsOut(nsIRDFResource* source,
                nsISimpleEnumerator** labels)
{
#if defined(XP_WIN) || defined(XP_MAC) || defined(XP_MACOSX)
    HandleSystemBookmarks(source);
#endif

    return mInner->ArcLabelsOut(source, labels);
}

NS_IMETHODIMP
nsBookmarksService::GetAllResources(nsISimpleEnumerator** aResult)
{
#if defined(XP_WIN) || defined(XP_MAC) || defined(XP_MACOSX)
    HandleSystemBookmarks(kNC_SystemBookmarksStaticRoot);
#endif
  
    return mInner->GetAllResources(aResult);
}

NS_IMETHODIMP
nsBookmarksService::GetAllCmds(nsIRDFResource* source,
                   nsISimpleEnumerator** commands)
{
    nsCOMPtr<nsISupportsArray>  cmdArray;
    nsresult            rv;
    rv = NS_NewISupportsArray(getter_AddRefs(cmdArray));
    if (NS_FAILED(rv))  return rv;

    
    nsCOMPtr<nsIRDFNode> nodeType;
    GetSynthesizedType(source, getter_AddRefs(nodeType));

    PRBool  isBookmark, isBookmarkFolder, isBookmarkSeparator;
    isBookmark = (nodeType == kNC_Bookmark) ? PR_TRUE : PR_FALSE;
    isBookmarkFolder = (nodeType == kNC_Folder) ? PR_TRUE : PR_FALSE;
    isBookmarkSeparator = (nodeType == kNC_BookmarkSeparator) ? PR_TRUE : PR_FALSE;

    if (isBookmark || isBookmarkFolder || isBookmarkSeparator)
    {
        cmdArray->AppendElement(kNC_BookmarkCommand_NewBookmark);
        cmdArray->AppendElement(kNC_BookmarkCommand_NewFolder);
        cmdArray->AppendElement(kNC_BookmarkCommand_NewSeparator);
        cmdArray->AppendElement(kNC_BookmarkSeparator);
    }
    if (isBookmark)
    {
        cmdArray->AppendElement(kNC_BookmarkCommand_DeleteBookmark);
    }
    if (isBookmarkFolder && (source != kNC_BookmarksRoot) && (source != kNC_IEFavoritesRoot))
    {
        cmdArray->AppendElement(kNC_BookmarkCommand_DeleteBookmarkFolder);
    }
    if (isBookmarkSeparator)
    {
        cmdArray->AppendElement(kNC_BookmarkCommand_DeleteBookmarkSeparator);
    }
    if (isBookmarkFolder)
    {
        nsCOMPtr<nsIRDFResource>    newBookmarkFolder, personalToolbarFolder, newSearchFolder;
        getFolderViaHint(kNC_NewBookmarkFolder, PR_FALSE, getter_AddRefs(newBookmarkFolder));
        getFolderViaHint(kNC_PersonalToolbarFolder, PR_FALSE, getter_AddRefs(personalToolbarFolder));
        getFolderViaHint(kNC_NewSearchFolder, PR_FALSE, getter_AddRefs(newSearchFolder));

        cmdArray->AppendElement(kNC_BookmarkSeparator);
        if (source != newBookmarkFolder.get())      cmdArray->AppendElement(kNC_BookmarkCommand_SetNewBookmarkFolder);
        if (source != newSearchFolder.get())        cmdArray->AppendElement(kNC_BookmarkCommand_SetNewSearchFolder);
        if (source != personalToolbarFolder.get())  cmdArray->AppendElement(kNC_BookmarkCommand_SetPersonalToolbarFolder);
    }

    
    cmdArray->AppendElement(kNC_BookmarkSeparator);

    return NS_NewArrayEnumerator(commands, cmdArray);
}

NS_IMETHODIMP
nsBookmarksService::IsCommandEnabled(nsISupportsArray* aSources,
                                         nsIRDFResource*   aCommand,
                                         nsISupportsArray* aArguments,
                                         PRBool* aResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

nsresult
nsBookmarksService::getArgumentN(nsISupportsArray *arguments, nsIRDFResource *res,
                PRInt32 offset, nsIRDFNode **argValue)
{
    nsresult        rv;
    PRUint32        loop, numArguments;

    *argValue = nsnull;

    if (NS_FAILED(rv = arguments->Count(&numArguments)))    return rv;

    
    
    for (loop = 0; loop < numArguments; loop += 2)
    {
        nsCOMPtr<nsIRDFResource> src = do_QueryElementAt(arguments, loop, &rv);
        if (!src) return rv;
        
        if (src == res)
        {
            if (offset > 0)
            {
                --offset;
                continue;
            }

            nsCOMPtr<nsIRDFNode> val = do_QueryElementAt(arguments, loop + 1,
                                                         &rv);
            if (!val) return rv;

            *argValue = val;
            NS_ADDREF(*argValue);
            return NS_OK;
        }
    }
    return NS_ERROR_INVALID_ARG;
}





nsresult
nsBookmarksService::insertBookmarkItem(nsIRDFResource *aRelativeNode, 
                                       nsISupportsArray *aArguments, 
                                       nsIRDFResource *aItemType)
{
    nsresult rv;
    const PRInt32 kParentArgumentIndex = 0;
  
    nsCOMPtr<nsIRDFResource> rParent;

    if (aRelativeNode == kNC_BookmarksRoot)
        rParent = aRelativeNode;
    else
    {
        nsCOMPtr<nsIRDFNode> parentNode;
        rv = getArgumentN(aArguments, kNC_Parent, kParentArgumentIndex, getter_AddRefs(parentNode));
        if (NS_FAILED(rv)) return rv;
        rParent = do_QueryInterface(parentNode, &rv);
        if (NS_FAILED(rv)) return rv;
    }

    nsCOMPtr<nsIRDFContainer> container(do_CreateInstance("@mozilla.org/rdf/container;1", &rv));
    if (NS_FAILED(rv)) return rv;

    rv = container->Init(this, rParent);
    if (NS_FAILED(rv)) return rv;

    PRInt32 relNodeIdx = 0;
    if (aRelativeNode != kNC_BookmarksRoot) {
        
        
        rv = container->IndexOf(aRelativeNode, &relNodeIdx);
        if (NS_FAILED(rv)) return rv;

        
        if (relNodeIdx == -1) {
            rv = container->GetCount(&relNodeIdx);
            if (NS_FAILED(rv)) return rv;
        }
    }

    nsAutoString itemName;
  
    
    if (aItemType == kNC_Bookmark || aItemType == kNC_Folder) {
        nsCOMPtr<nsIRDFNode> nameNode;
        getArgumentN(aArguments, kNC_Name, kParentArgumentIndex, getter_AddRefs(nameNode));
        nsCOMPtr<nsIRDFLiteral> nameLiteral;
        nameLiteral = do_QueryInterface(nameNode);
        if (nameLiteral) {
            const PRUnichar* uName = nsnull;
            nameLiteral->GetValueConst(&uName);
            if (uName) 
                itemName = uName;
        }
    }

    if (itemName.IsEmpty())
    {
        
        if (aItemType == kNC_Bookmark) 
            getLocaleString("NewBookmark", itemName);
        else if (aItemType == kNC_Folder) 
            getLocaleString("NewFolder", itemName);
    }

    nsCOMPtr<nsIRDFResource> newResource;
  
    
    if (aItemType == kNC_Bookmark || aItemType == kNC_Folder)
    {
        nsCOMPtr<nsIRDFNode> urlNode;
        getArgumentN(aArguments, kNC_URL, kParentArgumentIndex, getter_AddRefs(urlNode));
        nsCOMPtr<nsIRDFLiteral> bookmarkURILiteral(do_QueryInterface(urlNode));
        if (bookmarkURILiteral)
        {
            const PRUnichar* uURL = nsnull;
            bookmarkURILiteral->GetValueConst(&uURL);
            if (uURL)
            {
                rv = gRDF->GetUnicodeResource(nsDependentString(uURL), getter_AddRefs(newResource));
                if (NS_FAILED(rv)) return rv;
            }
        }
    }

    if (!newResource)
    {
        
        rv = gRDF->GetAnonymousResource(getter_AddRefs(newResource));
        if (NS_FAILED(rv)) return rv;
    }

    if (aItemType == kNC_Folder)
    {
        
        rv = gRDFC->MakeSeq(mInner, newResource, nsnull);
        if (NS_FAILED(rv)) return rv;
    }

    
    if (!itemName.IsEmpty())
    {
        nsCOMPtr<nsIRDFLiteral> nameLiteral;
        rv = gRDF->GetLiteral(itemName.get(), getter_AddRefs(nameLiteral));
        if (NS_FAILED(rv)) return rv;
        rv = mInner->Assert(newResource, kNC_Name, nameLiteral, PR_TRUE);
        if (NS_FAILED(rv)) return rv;
    }

    
    rv = mInner->Assert(newResource, kRDF_type, aItemType, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    
    
  
    
    nsCOMPtr<nsIRDFDate> dateLiteral;
    rv = gRDF->GetDateLiteral(PR_Now(), getter_AddRefs(dateLiteral));
    if (NS_FAILED(rv)) return rv;
    rv = mInner->Assert(newResource, kNC_BookmarkAddDate, dateLiteral, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    
    rv = container->InsertElementAt(newResource, !relNodeIdx ? 1 : relNodeIdx, PR_TRUE);
  
    return rv;
}

nsresult
nsBookmarksService::deleteBookmarkItem(nsIRDFResource *src,
                                       nsISupportsArray *aArguments,
                                       PRInt32 parentArgIndex)
{
    nsresult            rv;

    nsCOMPtr<nsIRDFNode>        aNode;
    if (NS_FAILED(rv = getArgumentN(aArguments, kNC_Parent,
            parentArgIndex, getter_AddRefs(aNode))))
        return rv;
    nsCOMPtr<nsIRDFResource>    argParent = do_QueryInterface(aNode);
    if (!argParent) return NS_ERROR_NO_INTERFACE;

    nsCOMPtr<nsIRDFContainer> container =
            do_CreateInstance(kRDFContainerCID, &rv);
    if (NS_FAILED(rv))
        return rv;
    if (NS_FAILED(rv = container->Init(this, argParent)))
        return rv;

    if (NS_FAILED(rv = container->RemoveElement(src, PR_TRUE)))
        return rv;

    return rv;
}

nsresult
nsBookmarksService::setFolderHint(nsIRDFResource *newSource, nsIRDFResource *objType)
{
    nsresult            rv;
    nsCOMPtr<nsISimpleEnumerator>   srcList;
    if (NS_FAILED(rv = GetSources(kNC_FolderType, objType, PR_TRUE, getter_AddRefs(srcList))))
        return rv;

    PRBool  hasMoreSrcs = PR_TRUE;
    while(NS_SUCCEEDED(rv = srcList->HasMoreElements(&hasMoreSrcs))
        && (hasMoreSrcs == PR_TRUE))
    {
        nsCOMPtr<nsISupports>   aSrc;
        if (NS_FAILED(rv = srcList->GetNext(getter_AddRefs(aSrc))))
            break;
        nsCOMPtr<nsIRDFResource>    aSource = do_QueryInterface(aSrc);
        if (!aSource)   continue;

        
        if (aSource.get() == newSource)   return NS_OK;

        if (NS_FAILED(rv = mInner->Unassert(aSource, kNC_FolderType, objType)))
            continue;
    }

    
    
    if (objType != kNC_PersonalToolbarFolder) {
        rv = mInner->Assert(newSource, kNC_FolderType, objType, PR_TRUE);

        mDirty = PR_TRUE;
        return rv;
    }

    
    BeginUpdateBatch();
    rv = SetNewPersonalToolbarFolder(newSource);
    EndUpdateBatch();
    if (NS_FAILED(rv))
        return rv;

    rv = mInner->Assert(kNC_PersonalToolbarFolder, kNC_FolderType, objType, PR_TRUE);
    if (NS_FAILED(rv))
        return rv;

    mDirty = PR_TRUE;

    return NS_OK;
}

nsresult
nsBookmarksService::getFolderViaHint(nsIRDFResource *objType, PRBool fallbackFlag, nsIRDFResource **folder)
{
    if (!folder)    return NS_ERROR_UNEXPECTED;
    *folder = nsnull;
    if (!objType)   return NS_ERROR_UNEXPECTED;

    nsresult            rv;
    nsCOMPtr<nsIRDFResource>    oldSource;
    if (NS_FAILED(rv = mInner->GetSource(kNC_FolderType, objType, PR_TRUE, getter_AddRefs(oldSource))))
        return rv;

    if ((rv != NS_RDF_NO_VALUE) && (oldSource))
    {
        PRBool isBookmarkedFlag = PR_FALSE;
        if (NS_SUCCEEDED(rv = IsBookmarkedResource(oldSource, &isBookmarkedFlag)) &&
            isBookmarkedFlag) {
            *folder = oldSource;
        }
    }

    
    
    if ((!(*folder)) && (fallbackFlag == PR_TRUE) && (objType == kNC_NewSearchFolder))
    {
        rv = getFolderViaHint(kNC_NewBookmarkFolder, fallbackFlag, folder);
    }

    if (!(*folder))
    {
        
        if (objType == kNC_NewBookmarkFolder || objType == kNC_NewSearchFolder)
        {
            *folder = kNC_BookmarksRoot;
        }
        else if (objType == kNC_PersonalToolbarFolder)
        {
            *folder = kNC_PersonalToolbarFolder;
        }
    }

    NS_IF_ADDREF(*folder);

    return NS_OK;
}

nsresult
nsBookmarksService::importBookmarks(nsISupportsArray *aArguments)
{
    
    nsresult rv;
    nsCOMPtr<nsIRDFNode> aNode;
    rv = getArgumentN(aArguments, kNC_URL, 0, getter_AddRefs(aNode));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIRDFLiteral> pathLiteral = do_QueryInterface(aNode, &rv);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_NO_INTERFACE);
    const PRUnichar *pathUni = nsnull;
    pathLiteral->GetValueConst(&pathUni);
    NS_ENSURE_TRUE(pathUni, NS_ERROR_NULL_POINTER);

    nsCOMPtr<nsILocalFile> file;
    rv = NS_NewLocalFile(nsDependentString(pathUni), PR_TRUE, getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool isFile;
    rv = file->IsFile(&isFile);
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && isFile, NS_ERROR_UNEXPECTED);

    
    nsCOMPtr<nsIRDFResource> newBookmarkFolder;
    rv = getFolderViaHint(kNC_NewBookmarkFolder, PR_TRUE, 
                          getter_AddRefs(newBookmarkFolder));
    NS_ENSURE_SUCCESS(rv, rv);

    
    BookmarkParser parser;
    parser.Init(file, mInner, PR_TRUE);

    
    parser.Parse(newBookmarkFolder, kNC_Bookmark);

    return NS_OK;
}

nsresult
nsBookmarksService::exportBookmarks(nsISupportsArray *aArguments)
{
    
    nsCOMPtr<nsIRDFNode> node;
    nsresult rv = getArgumentN(aArguments, kNC_URL, 0, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIRDFLiteral> literal = do_QueryInterface(node, &rv);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_NO_INTERFACE);
    const PRUnichar* pathUni = nsnull;
    literal->GetValueConst(&pathUni);
    NS_ENSURE_TRUE(pathUni, NS_ERROR_NULL_POINTER);

    
    const PRUnichar* format = EmptyString().get();
    rv = getArgumentN(aArguments, kRDF_type, 0, getter_AddRefs(node));
    if (NS_SUCCEEDED(rv))
    {
        literal = do_QueryInterface(node, &rv);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_NO_INTERFACE);
        literal->GetValueConst(&format);
        NS_ENSURE_TRUE(format, NS_ERROR_NULL_POINTER);
    }

    nsCOMPtr<nsILocalFile> file;
    rv = NS_NewLocalFile(nsDependentString(pathUni), PR_TRUE, getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    if (NS_LITERAL_STRING("RDF").Equals(format, nsCaseInsensitiveStringComparator()))
    {
        nsCOMPtr<nsIURI> uri;
        nsresult rv = NS_NewFileURI(getter_AddRefs(uri), file);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = SerializeBookmarks(uri);
    }
    else
    {
        
        rv = WriteBookmarks(file, mInner, kNC_BookmarksRoot);
    }

    return rv;
}

NS_IMETHODIMP
nsBookmarksService::DoCommand(nsISupportsArray *aSources, nsIRDFResource *aCommand,
                nsISupportsArray *aArguments)
{
    nsresult        rv = NS_OK;
    PRInt32         loop;
    PRUint32        numSources;
    if (NS_FAILED(rv = aSources->Count(&numSources)))   return rv;
    if (numSources < 1)
    {
        return NS_ERROR_ILLEGAL_VALUE;
    }

    
    
    

    for (loop=((PRInt32)numSources)-1; loop>=0; loop--)
    {
        nsCOMPtr<nsIRDFResource> src = do_QueryElementAt(aSources, loop, &rv);
        if (!src) return rv;

        if (aCommand == kNC_BookmarkCommand_NewBookmark)
        {
            rv = insertBookmarkItem(src, aArguments, kNC_Bookmark);
            if (NS_FAILED(rv))  return rv;
            break;
        }
        else if (aCommand == kNC_BookmarkCommand_NewFolder)
        {
            rv = insertBookmarkItem(src, aArguments, kNC_Folder);
            if (NS_FAILED(rv))  return rv;
            break;
        }
        else if (aCommand == kNC_BookmarkCommand_NewSeparator)
        {
            rv = insertBookmarkItem(src, aArguments, kNC_BookmarkSeparator);
            if (NS_FAILED(rv))  return rv;
            break;
        }
        else if (aCommand == kNC_BookmarkCommand_DeleteBookmark ||
            aCommand == kNC_BookmarkCommand_DeleteBookmarkFolder ||
            aCommand == kNC_BookmarkCommand_DeleteBookmarkSeparator)
        {
            if (NS_FAILED(rv = deleteBookmarkItem(src, aArguments, loop)))
                return rv;
        }
        else if (aCommand == kNC_BookmarkCommand_SetNewBookmarkFolder)
        {
            rv = setFolderHint(src, kNC_NewBookmarkFolder);
            if (NS_FAILED(rv))  return rv;
            break;
        }
        else if (aCommand == kNC_BookmarkCommand_SetPersonalToolbarFolder)
        {
            rv = setFolderHint(src, kNC_PersonalToolbarFolder);
            if (NS_FAILED(rv))  return rv;
            break;
        }
        else if (aCommand == kNC_BookmarkCommand_SetNewSearchFolder)
        {
            rv = setFolderHint(src, kNC_NewSearchFolder);
            if (NS_FAILED(rv))  return rv;
            break;
        }
        else if (aCommand == kNC_BookmarkCommand_Import)
        {
            rv = importBookmarks(aArguments);
            if (NS_FAILED(rv))  return rv;
            break;
        }
        else if (aCommand == kNC_BookmarkCommand_Export)
        {
            rv = exportBookmarks(aArguments);
            if (NS_FAILED(rv))  return rv;
            break;
        }
    }

    mDirty = PR_TRUE;

    return NS_OK;
}





NS_IMETHODIMP
nsBookmarksService::GetLoaded(PRBool* _result)
{
    *_result = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::Init(const char* aURI)
{
    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::Refresh(PRBool aBlocking)
{
    
    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::Flush()
{
    nsresult    rv = NS_OK;

    
    
    
    if (mBookmarksFile)
    {
        rv = WriteBookmarks(mBookmarksFile, mInner, kNC_BookmarksRoot);
    }

    return rv;
}

NS_IMETHODIMP
nsBookmarksService::FlushTo(const char *aURI)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP
nsBookmarksService::GetPropagateChanges(PRBool* aPropagateChanges)
{
    nsCOMPtr<nsIRDFPropagatableDataSource> propagatable = do_QueryInterface(mInner);
    return propagatable->GetPropagateChanges(aPropagateChanges);
}

NS_IMETHODIMP
nsBookmarksService::SetPropagateChanges(PRBool aPropagateChanges)
{
    nsCOMPtr<nsIRDFPropagatableDataSource> propagatable = do_QueryInterface(mInner);
    return propagatable->SetPropagateChanges(aPropagateChanges);
}





nsresult
nsBookmarksService::EnsureBookmarksFile()
{
    nsresult rv;

    
    
    nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv))
    {
        nsCOMPtr<nsISupportsString> prefVal;
        rv = prefBranch->GetComplexValue("browser.bookmarks.file",
                                         NS_GET_IID(nsISupportsString),
                                         getter_AddRefs(prefVal));      
        if (NS_SUCCEEDED(rv))
        {
            nsAutoString bookmarksFile;
            prefVal->GetData(bookmarksFile); 
            rv = NS_NewLocalFile(bookmarksFile, PR_TRUE,
                                 getter_AddRefs(mBookmarksFile));

            if (NS_SUCCEEDED(rv))
            {
                return NS_OK;
            }
        }
    }


    
    
    rv = NS_GetSpecialDirectory(NS_APP_BOOKMARKS_50_FILE, (nsIFile **)(nsILocalFile **)getter_AddRefs(mBookmarksFile));
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}


#if defined(XP_MAC) || defined(XP_MACOSX)

nsresult
nsBookmarksService::ReadFavorites()
{
    mIEFavoritesAvailable = PR_TRUE;
    nsresult rv;
            
#ifdef DEBUG_varga
    PRTime      now;
#if defined(XP_MAC)
    Microseconds((UnsignedWide *)&now);
#else
    now = PR_Now();
#endif
    printf("Start reading in IE Favorites.html\n");
#endif

    
    nsAutoString    ieTitle;
    getLocaleString("ImportedIEFavorites", ieTitle);

    nsCOMPtr<nsIFile> ieFavoritesFile;
    rv = NS_GetSpecialDirectory(NS_MAC_PREFS_DIR, getter_AddRefs(ieFavoritesFile));
    NS_ENSURE_SUCCESS(rv, rv);

    ieFavoritesFile->Append(NS_LITERAL_STRING("Explorer"));
    ieFavoritesFile->Append(NS_LITERAL_STRING("Favorites.html"));

    if (NS_SUCCEEDED(rv = gRDFC->MakeSeq(mInner, kNC_IEFavoritesRoot, nsnull)))
    {
        BookmarkParser parser;
        parser.Init(ieFavoritesFile, mInner);
        BeginUpdateBatch();
        parser.Parse(kNC_IEFavoritesRoot, kNC_IEFavorite);
        EndUpdateBatch();
            
        nsCOMPtr<nsIRDFLiteral> ieTitleLiteral;
        rv = gRDF->GetLiteral(ieTitle.get(), getter_AddRefs(ieTitleLiteral));
        if (NS_SUCCEEDED(rv) && ieTitleLiteral)
        {
            rv = mInner->Assert(kNC_IEFavoritesRoot, kNC_Name, ieTitleLiteral, PR_TRUE);
        }
    }
#ifdef DEBUG_varga
    PRTime      now2;
#if defined(XP_MAC)
    Microseconds((UnsignedWide *)&now2);
#else
    now = PR_Now();
#endif
    PRUint64    loadTime64;
    LL_SUB(loadTime64, now2, now);
    PRUint32    loadTime32;
    LL_L2UI(loadTime32, loadTime64);
    printf("Finished reading in IE Favorites.html  (%u microseconds)\n", loadTime32);
#endif
    return rv;
}

#endif

NS_IMETHODIMP
nsBookmarksService::ReadBookmarks(PRBool *didLoadBookmarks)
{
    *didLoadBookmarks = PR_FALSE;
    if (!mBookmarksFile)
    {
        LoadBookmarks();
        if (mBookmarksFile) {
            *didLoadBookmarks = PR_TRUE;
            nsCOMPtr<nsIPrefBranch2> prefBranchInt(do_GetService(NS_PREFSERVICE_CONTRACTID));
            if (prefBranchInt)
                prefBranchInt->AddObserver("browser.bookmarks.file", this, true);
        }
    }
    return NS_OK;
}

nsresult
nsBookmarksService::initDatasource()
{
    
    
    NS_IF_RELEASE(mInner);

    
    nsresult rv = CallCreateInstance(kRDFInMemoryDataSourceCID, &mInner);
    if (NS_FAILED(rv)) return rv;

    rv = mInner->AddObserver(this);
    if (NS_FAILED(rv)) return rv;

    rv = gRDFC->MakeSeq(mInner, kNC_BookmarksTopRoot, nsnull);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to make NC:BookmarksTopRoot a sequence");
    if (NS_FAILED(rv)) return rv;

    rv = gRDFC->MakeSeq(mInner, kNC_BookmarksRoot, nsnull);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to make NC:BookmarksRoot a sequence");
    if (NS_FAILED(rv)) return rv;

    
    rv = mInner->Assert(kNC_BookmarksTopRoot, kRDF_type, kNC_Folder, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    rv = mInner->Assert(kNC_BookmarksRoot, kRDF_type, kNC_Folder, PR_TRUE);
    if (NS_FAILED(rv)) return rv;

    
    nsCOMPtr<nsIRDFContainer> container(do_CreateInstance(kRDFContainerCID, &rv));
    if (NS_FAILED(rv)) return rv;
    rv = container->Init(mInner, kNC_BookmarksTopRoot);
    if (NS_FAILED(rv)) return rv;
    rv = container->AppendElement(kNC_BookmarksRoot);

    return rv;
}

nsresult
nsBookmarksService::LoadBookmarks()
{
    nsresult    rv;

    rv = initDatasource();
    if (NS_FAILED(rv)) return NS_OK;

    rv = EnsureBookmarksFile();

    
    if (NS_FAILED(rv)) return NS_OK;

    PRBool foundIERoot = PR_FALSE;

    nsCOMPtr<nsIPrefService> prefSvc(do_GetService(NS_PREFSERVICE_CONTRACTID));
    nsCOMPtr<nsIPrefBranch> bookmarksPrefs;
    if (prefSvc)
        prefSvc->GetBranch("browser.bookmarks.", getter_AddRefs(bookmarksPrefs));

#ifdef DEBUG_varga
    PRTime now;
#if defined(XP_MAC)
    Microseconds((UnsignedWide *)&now);
#else
    now = PR_Now();
#endif
    printf("Start reading in bookmarks.html\n");
#endif
  
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    PRBool useDynamicSystemBookmarks;
#ifdef XP_BEOS
    
    useDynamicSystemBookmarks = PR_TRUE;
#else
    useDynamicSystemBookmarks = PR_FALSE;
    if (bookmarksPrefs)
        bookmarksPrefs->GetBoolPref("import_system_favorites", &useDynamicSystemBookmarks);
#endif

    nsCAutoString bookmarksURICString;

#if defined(XP_WIN) || defined(XP_BEOS)
    nsCOMPtr<nsIFile> systemBookmarksFolder;

#if defined(XP_WIN)
    rv = NS_GetSpecialDirectory(NS_WIN_FAVORITES_DIR, getter_AddRefs(systemBookmarksFolder));
#elif defined(XP_BEOS)
    rv = NS_GetSpecialDirectory(NS_BEOS_SETTINGS_DIR, getter_AddRefs(systemBookmarksFolder));

    if (NS_SUCCEEDED(rv))
        rv = systemBookmarksFolder->AppendNative(NS_LITERAL_CSTRING("NetPositive"));
   
    if (NS_SUCCEEDED(rv))
        rv = systemBookmarksFolder->AppendNative(NS_LITERAL_CSTRING("Bookmarks"));
#endif

    if (NS_SUCCEEDED(rv))
    {
        nsCOMPtr<nsIURI> bookmarksURI;
        rv = NS_NewFileURI(getter_AddRefs(bookmarksURI), systemBookmarksFolder);

        if (NS_SUCCEEDED(rv))
            rv = bookmarksURI->GetSpec(bookmarksURICString);
    }
#elif defined(XP_MAC) || defined(XP_MACOSX)
    bookmarksURICString.AssignLiteral(kURINC_IEFavoritesRoot);
#endif

    nsCOMPtr<nsIRDFResource> systemFolderResource;
    if (!bookmarksURICString.IsEmpty())
        gRDF->GetResource(bookmarksURICString,
                          getter_AddRefs(systemFolderResource));

    
    {
        BookmarkParser parser;
        parser.Init(mBookmarksFile, mInner);
        if (useDynamicSystemBookmarks && !bookmarksURICString.IsEmpty())
        {
            parser.SetIEFavoritesRoot(bookmarksURICString);
            parser.ParserFoundIEFavoritesRoot(&foundIERoot);
        }

        BeginUpdateBatch();
        parser.Parse(kNC_BookmarksRoot, kNC_Bookmark);
        EndUpdateBatch();
        
        PRBool foundPTFolder = PR_FALSE;
        parser.ParserFoundPersonalToolbarFolder(&foundPTFolder);
        
        if ((foundPTFolder == PR_FALSE) && (!mPersonalToolbarName.IsEmpty()))
        {
            nsCOMPtr<nsIRDFLiteral>   ptNameLiteral;
            rv = gRDF->GetLiteral(mPersonalToolbarName.get(), getter_AddRefs(ptNameLiteral));
            if (NS_SUCCEEDED(rv))
            {
                nsCOMPtr<nsIRDFResource>    ptSource;
                rv = mInner->GetSource(kNC_Name, ptNameLiteral, PR_TRUE, getter_AddRefs(ptSource));
                if (NS_FAILED(rv)) return rv;
        
                if ((rv != NS_RDF_NO_VALUE) && (ptSource))
                    setFolderHint(ptSource, kNC_PersonalToolbarFolder);
            }
        }

      
      nsCOMPtr<nsIRDFLiteral> brNameLiteral;
      rv = gRDF->GetLiteral(mBookmarksRootName.get(), getter_AddRefs(brNameLiteral));
      if (NS_SUCCEEDED(rv))
          mInner->Assert(kNC_BookmarksRoot, kNC_Name, brNameLiteral, PR_TRUE);

    } 

    
    
    
    
#if defined(XP_WIN) || defined(XP_MAC) || defined(XP_MACOSX)
    PRBool addedStaticRoot = PR_FALSE;
    if (bookmarksPrefs)
        bookmarksPrefs->GetBoolPref("added_static_root", 
                                    &addedStaticRoot);

    
    
    if (!addedStaticRoot && systemFolderResource)
    {
        nsCOMPtr<nsIRDFContainer> rootContainer(do_CreateInstance(kRDFContainerCID, &rv));
        if (NS_FAILED(rv)) return rv;

        rv = rootContainer->Init(this, kNC_BookmarksRoot);
        if (NS_FAILED(rv)) return rv;

        rv = mInner->Assert(kNC_SystemBookmarksStaticRoot, kRDF_type, kNC_Folder, PR_TRUE);
        if (NS_FAILED(rv)) return rv;

        nsAutoString importedStaticTitle;
        getLocaleString("ImportedIEStaticFavorites", importedStaticTitle);

        nsCOMPtr<nsIRDFLiteral> staticTitleLiteral;
        rv = gRDF->GetLiteral(importedStaticTitle.get(), getter_AddRefs(staticTitleLiteral));
        if (NS_FAILED(rv)) return rv;

        rv = mInner->Assert(kNC_SystemBookmarksStaticRoot, kNC_Name, staticTitleLiteral, PR_TRUE);
        if (NS_FAILED(rv)) return rv;

        rv = rootContainer->AppendElement(kNC_SystemBookmarksStaticRoot);
        if (NS_FAILED(rv)) return rv;

        
        
        
        if (!useDynamicSystemBookmarks)
        {
            nsCOMPtr<nsIRDFContainer> container(do_CreateInstance(kRDFContainerCID, &rv));
            if (NS_FAILED(rv)) return rv;

            rv = container->Init(this, kNC_BookmarksRoot);
            if (NS_FAILED(rv)) return rv;
      
            rv = container->RemoveElement(systemFolderResource, PR_TRUE);
            if (NS_FAILED(rv)) return rv;
        }

        bookmarksPrefs->SetBoolPref("added_static_root", PR_TRUE);
    }
#endif

    
    
    if (useDynamicSystemBookmarks)
    {
#if defined(XP_MAC) || defined(XP_MACOSX)
        
        if (!foundIERoot)
        {
            nsCOMPtr<nsIRDFContainer> bookmarksRoot(do_CreateInstance(kRDFContainerCID, &rv));
            if (NS_FAILED(rv)) return rv;

            rv = bookmarksRoot->Init(this, kNC_BookmarksRoot);
            if (NS_FAILED(rv)) return rv;

            rv = bookmarksRoot->AppendElement(kNC_IEFavoritesRoot);
            if (NS_FAILED(rv)) return rv;

            
            rv = mInner->Assert(kNC_IEFavoritesRoot, kRDF_type, kNC_IEFavoriteFolder, PR_TRUE);
            if (NS_FAILED(rv)) return rv;
        }
#elif defined(XP_WIN) || defined(XP_BEOS)
        if (systemFolderResource)
        {
            nsAutoString systemBookmarksFolderTitle;
#ifdef XP_BEOS
            getLocaleString("ImportedNetPositiveBookmarks", systemBookmarksFolderTitle);
#else
            getLocaleString("ImportedIEFavorites", systemBookmarksFolderTitle);
#endif

            nsCOMPtr<nsIRDFLiteral>   systemFolderTitleLiteral;
            rv = gRDF->GetLiteral(systemBookmarksFolderTitle.get(), 
                                  getter_AddRefs(systemFolderTitleLiteral));
            if (NS_SUCCEEDED(rv) && systemFolderTitleLiteral)
                rv = mInner->Assert(systemFolderResource, kNC_Name, 
                                    systemFolderTitleLiteral, PR_TRUE);
    
            
            if (!foundIERoot)
            {
                nsCOMPtr<nsIRDFContainer> container(do_CreateInstance(kRDFContainerCID, &rv));
                if (NS_FAILED(rv)) return rv;

                rv = container->Init(this, kNC_BookmarksRoot);
                if (NS_FAILED(rv)) return rv;

                rv = container->AppendElement(systemFolderResource);
                if (NS_FAILED(rv)) return rv;
            }
        }
#endif
    }

#ifdef DEBUG_varga
    PRTime      now2;
#if defined(XP_MAC)
    Microseconds((UnsignedWide *)&now2);
#else
    now2 = PR_Now();
#endif
    PRUint64    loadTime64;
    LL_SUB(loadTime64, now2, now);
    PRUint32    loadTime32;
    LL_L2UI(loadTime32, loadTime64);
    printf("Finished reading in bookmarks.html  (%u microseconds)\n", loadTime32);
#endif

    return NS_OK;
}

static char kFileIntro[] = 
    "<!DOCTYPE NETSCAPE-Bookmark-file-1>" NS_LINEBREAK
    "<!-- This is an automatically generated file." NS_LINEBREAK
    "     It will be read and overwritten." NS_LINEBREAK
    "     DO NOT EDIT! -->" NS_LINEBREAK
    
    "<META HTTP-EQUIV=\"Content-Type\" CONTENT=\"text/html; charset=UTF-8\">" NS_LINEBREAK
    "<TITLE>Bookmarks</TITLE>" NS_LINEBREAK
    "<H1>Bookmarks</H1>" NS_LINEBREAK NS_LINEBREAK;

nsresult
nsBookmarksService::WriteBookmarks(nsIFile* aBookmarksFile,
                                   nsIRDFDataSource* aDataSource,
                                   nsIRDFResource *aRoot)
{
    if (!aBookmarksFile || !aDataSource || !aRoot)
        return NS_ERROR_NULL_POINTER;

    
    
    nsCOMPtr<nsIOutputStream> out;
    nsresult rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(out),
                                                  aBookmarksFile,
                                                  -1,
                                                   0600);
    if (NS_FAILED(rv)) return rv;

    
    
    nsCOMPtr<nsIOutputStream> strm;
    rv = NS_NewBufferedOutputStream(getter_AddRefs(strm), out, 4096);
    if (NS_FAILED(rv)) return rv;

    PRUint32 dummy;
    strm->Write(kFileIntro, sizeof(kFileIntro)-1, &dummy);

    nsCOMArray<nsIRDFResource> parentArray;
    rv = WriteBookmarksContainer(aDataSource, strm, aRoot, 0, parentArray);

    
    
    nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(strm);
    NS_ASSERTION(safeStream, "expected a safe output stream!");
    if (NS_SUCCEEDED(rv) && safeStream)
        rv = safeStream->Finish();

    if (NS_FAILED(rv)) {
        NS_WARNING("failed to save bookmarks file! possible dataloss");
        return rv;
    }

    mDirty = PR_FALSE;
    return NS_OK;
}

static const char kBookmarkIntro[] = "<DL><p>" NS_LINEBREAK;
static const char kIndent[] = "    ";
static const char kContainerIntro[] = "<DT><H3";
static const char kSpaceStr[] = " ";
static const char kTrueEnd[] = "true\"";
static const char kQuoteStr[] = "\"";
static const char kCloseAngle[] = ">";
static const char kCloseH3[] = "</H3>" NS_LINEBREAK;
static const char kHROpen[] = "<HR";
static const char kAngleNL[] = ">" NS_LINEBREAK;
static const char kDTOpen[] = "<DT><A";
static const char kAClose[] = "</A>" NS_LINEBREAK;
static const char kBookmarkClose[] = "</DL><p>" NS_LINEBREAK;
static const char kNL[] = NS_LINEBREAK;

nsresult
nsBookmarksService::WriteBookmarksContainer(nsIRDFDataSource *ds,
                                            nsIOutputStream* strm,
                                            nsIRDFResource *parent, PRInt32 level,
                                            nsCOMArray<nsIRDFResource>& parentArray)
{
    
    nsresult rv;

    nsCOMPtr<nsIRDFContainer> container =
            do_CreateInstance(kRDFContainerCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCAutoString   indentation;

    for (PRInt32 loop=0; loop<level; loop++)
        indentation.Append(kIndent, sizeof(kIndent)-1);

    PRUint32 dummy;
    rv = strm->Write(indentation.get(), indentation.Length(), &dummy);
    rv |= strm->Write(kBookmarkIntro, sizeof(kBookmarkIntro)-1, &dummy);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

    rv = container->Init(ds, parent);
    if (NS_SUCCEEDED(rv) && (parentArray.IndexOfObject(parent) < 0))
    {
        
        
        parentArray.InsertObjectAt(parent, 0);

        nsCOMPtr<nsISimpleEnumerator>   children;
        if (NS_SUCCEEDED(rv = container->GetElements(getter_AddRefs(children))))
        {
            PRBool  more = PR_TRUE;
            while (more == PR_TRUE)
            {
                if (NS_FAILED(rv = children->HasMoreElements(&more)))   break;
                if (more != PR_TRUE)    break;

                nsCOMPtr<nsISupports>   iSupports;                  
                if (NS_FAILED(rv = children->GetNext(getter_AddRefs(iSupports))))   break;

                nsCOMPtr<nsIRDFResource>    child = do_QueryInterface(iSupports);
                if (!child) break;

                PRBool  isContainer = PR_FALSE;
                if (child.get() != kNC_IEFavoritesRoot)
                {
                    rv = gRDFC->IsContainer(ds, child, &isContainer);
                    if (NS_FAILED(rv)) break;
                }

                nsCOMPtr<nsIRDFNode>    nameNode;
                nsAutoString        nameString;
                nsCAutoString       name;
                rv = ds->GetTarget(child, kNC_Name, PR_TRUE, getter_AddRefs(nameNode));
                if (NS_SUCCEEDED(rv) && nameNode)
                {
                    nsCOMPtr<nsIRDFLiteral> nameLiteral = do_QueryInterface(nameNode);
                    if (nameLiteral)
                    {
                        const PRUnichar *title = nsnull;
                        if (NS_SUCCEEDED(rv = nameLiteral->GetValueConst(&title)))
                        {
                            nameString = title;
                            AppendUTF16toUTF8(nameString, name);
                        }
                    }
                }

                rv = strm->Write(indentation.get(), indentation.Length(), &dummy);
                rv |= strm->Write(kIndent, sizeof(kIndent)-1, &dummy);
                if (NS_FAILED(rv)) break;

                if (isContainer == PR_TRUE)
                {
                    rv = strm->Write(kContainerIntro, sizeof(kContainerIntro)-1, &dummy);
                    
                    rv |= WriteBookmarkProperties(ds, strm, child, kNC_BookmarkAddDate, kAddDateEquals, PR_FALSE);

                    
                    rv |= WriteBookmarkProperties(ds, strm, child, kWEB_LastModifiedDate, kLastModifiedEquals, PR_FALSE);
                    if (NS_FAILED(rv)) break;

                    
                    PRBool  hasType = PR_FALSE;
                    if (NS_SUCCEEDED(rv = mInner->HasAssertion(child, kNC_FolderType, kNC_NewBookmarkFolder,
                                                               PR_TRUE, &hasType)) && (hasType == PR_TRUE))
                    {
                        rv = strm->Write(kSpaceStr, sizeof(kSpaceStr)-1, &dummy);
                        rv |= strm->Write(kNewBookmarkFolderEquals, sizeof(kNewBookmarkFolderEquals)-1, &dummy);
                        rv |= strm->Write(kTrueEnd, sizeof(kTrueEnd)-1, &dummy);
                        if (NS_FAILED(rv)) break;
                    }
                    if (NS_SUCCEEDED(rv = mInner->HasAssertion(child, kNC_FolderType, kNC_NewSearchFolder,
                                                               PR_TRUE, &hasType)) && (hasType == PR_TRUE))
                    {
                        rv = strm->Write(kSpaceStr, sizeof(kSpaceStr)-1, &dummy);
                        rv |= strm->Write(kNewSearchFolderEquals, sizeof(kNewSearchFolderEquals)-1, &dummy);
                        rv |= strm->Write(kTrueEnd, sizeof(kTrueEnd)-1, &dummy);
                        if (NS_FAILED(rv)) break;
                    }
                    if (NS_SUCCEEDED(rv = mInner->HasAssertion(child, kNC_FolderType, kNC_PersonalToolbarFolder,
                                                               PR_TRUE, &hasType)) && (hasType == PR_TRUE))
                    {
                        rv = strm->Write(kSpaceStr, sizeof(kSpaceStr)-1, &dummy);
                        rv |= strm->Write(kPersonalToolbarFolderEquals, sizeof(kPersonalToolbarFolderEquals)-1, &dummy);
                        rv |= strm->Write(kTrueEnd, sizeof(kTrueEnd)-1, &dummy);
                        if (NS_FAILED(rv)) break;
                    }

                    if (NS_SUCCEEDED(rv = mInner->HasArcOut(child, kNC_FolderGroup, &hasType)) && 
                        (hasType == PR_TRUE))
                    {
                        rv = strm->Write(kSpaceStr, sizeof(kSpaceStr)-1, &dummy);
                        rv |= strm->Write(kFolderGroupEquals, sizeof(kFolderGroupEquals)-1, &dummy);
                        rv |= strm->Write(kTrueEnd, sizeof(kTrueEnd)-1, &dummy);
                        if (NS_FAILED(rv)) break;
                    }

                    
                    const char  *id = nsnull;
                    rv = child->GetValueConst(&id);
                    if (NS_SUCCEEDED(rv) && (id))
                    {
                        rv = strm->Write(kSpaceStr, sizeof(kSpaceStr)-1, &dummy);
                        rv |= strm->Write(kIDEquals, sizeof(kIDEquals)-1, &dummy);
                        rv |= strm->Write(id, strlen(id), &dummy);
                        rv |= strm->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
                        if (NS_FAILED(rv)) break;
                    }
          
                    rv = strm->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);
          
                    
                    if (!name.IsEmpty())
                    {
                        
                        char *escapedAttrib = nsEscapeHTML(name.get());
                        if (escapedAttrib)
                        {
                            rv |= strm->Write(escapedAttrib, strlen(escapedAttrib), &dummy);
                            NS_Free(escapedAttrib);
                        }
                    }
                    rv |= strm->Write(kCloseH3, sizeof(kCloseH3)-1, &dummy);

                    
                    rv |= WriteBookmarkProperties(ds, strm, child, kNC_Description, kOpenDD, PR_TRUE);

                    rv |= WriteBookmarksContainer(ds, strm, child, level+1, parentArray);
                }
                else
                {
                    const char  *url = nsnull;
                    if (NS_SUCCEEDED(rv = child->GetValueConst(&url)) && (url))
                    {
                        nsCAutoString   uri(url);

                        PRBool      isBookmarkSeparator = PR_FALSE;
                        if (NS_SUCCEEDED(mInner->HasAssertion(child, kRDF_type,
                                                              kNC_BookmarkSeparator, PR_TRUE, &isBookmarkSeparator)) &&
                            (isBookmarkSeparator == PR_TRUE) )
                        {
                            
                            rv = strm->Write(kHROpen, sizeof(kHROpen)-1, &dummy);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kNC_Name, kNameEquals, PR_FALSE);

                            rv |= strm->Write(kAngleNL, sizeof(kAngleNL)-1, &dummy);
                            if (NS_FAILED(rv)) break;
                        }
                        else
                        {
                            rv = strm->Write(kDTOpen, sizeof(kDTOpen)-1, &dummy);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kNC_URL, kHREFEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kNC_BookmarkAddDate, kAddDateEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kWEB_LastVisitDate, kLastVisitEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kWEB_LastModifiedDate, kLastModifiedEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kNC_ShortcutURL, kShortcutURLEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kNC_Icon, kIconEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kWEB_Schedule, kScheduleEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kWEB_LastPingDate, kLastPingEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kWEB_LastPingETag, kPingETagEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kWEB_LastPingModDate, kPingLastModEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kWEB_LastCharset, kLastCharsetEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kWEB_LastPingContentLen, kPingContentLenEquals, PR_FALSE);

                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kWEB_Status, kPingStatusEquals, PR_FALSE);
                            if (NS_FAILED(rv)) break;
                            
                            
                            const char  *id = nsnull;
                            rv = child->GetValueConst(&id);
                            if (NS_SUCCEEDED(rv) && (id))
                            {
                                rv = strm->Write(kSpaceStr, sizeof(kSpaceStr)-1, &dummy);
                                rv |= strm->Write(kIDEquals, sizeof(kIDEquals)-1, &dummy);
                                rv |= strm->Write(id, strlen(id), &dummy);
                                rv |= strm->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
                                if (NS_FAILED(rv)) break;
                            }

                            rv = strm->Write(kCloseAngle, sizeof(kCloseAngle)-1, &dummy);

                            
                            if (!name.IsEmpty())
                            {
                                
                                
                                char *escapedAttrib = nsEscapeHTML(name.get());
                                if (escapedAttrib)
                                {
                                    rv |= strm->Write(escapedAttrib,
                                                       strlen(escapedAttrib),
                                                       &dummy);
                                    NS_Free(escapedAttrib);
                                    escapedAttrib = nsnull;
                                }
                            }

                            rv |= strm->Write(kAClose, sizeof(kAClose)-1, &dummy);
                            
                            
                            rv |= WriteBookmarkProperties(ds, strm, child, kNC_Description, kOpenDD, PR_TRUE);
                        }
                    }
                }

                if (NS_FAILED(rv))  break;
            }
        }

        
        parentArray.RemoveObjectAt(0);
    }

    rv |= strm->Write(indentation.get(), indentation.Length(), &dummy);
    rv |= strm->Write(kBookmarkClose, sizeof(kBookmarkClose)-1, &dummy);

    NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

    return NS_OK;
}

nsresult
nsBookmarksService::SerializeBookmarks(nsIURI* aURI)
{
    NS_ASSERTION(aURI, "null ptr");

    nsresult rv;
    nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIFile> file;
    rv = fileURL->GetFile(getter_AddRefs(file));
    if (NS_FAILED(rv)) return rv;

    
    (void)file->Create(nsIFile::NORMAL_FILE_TYPE, 0666);

    nsCOMPtr<nsIOutputStream> out;
    rv = NS_NewLocalFileOutputStream(getter_AddRefs(out), file);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIOutputStream> bufferedOut;
    rv = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOut), out, 4096);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFXMLSerializer> serializer =
        do_CreateInstance("@mozilla.org/rdf/xml-serializer;1", &rv);
    if (NS_FAILED(rv)) return rv;

    rv = serializer->Init(this);
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIRDFXMLSource> source = do_QueryInterface(serializer);
    if (! source)
        return NS_ERROR_FAILURE;

    return source->Serialize(bufferedOut);
}





nsresult
nsBookmarksService::GetTextForNode(nsIRDFNode* aNode, nsString& aResult)
{
    nsresult        rv;
    nsIRDFResource  *resource;
    nsIRDFLiteral   *literal;
    nsIRDFDate      *dateLiteral;
    nsIRDFInt       *intLiteral;

    if (! aNode)
    {
        aResult.Truncate();
        rv = NS_OK;
    }
    else if (NS_SUCCEEDED(rv = aNode->QueryInterface(NS_GET_IID(nsIRDFResource), (void**) &resource)))
    {
        const char  *p = nsnull;
        if (NS_SUCCEEDED(rv = resource->GetValueConst( &p )) && (p))
        {
            aResult.AssignWithConversion(p);
        }
        NS_RELEASE(resource);
    }
    else if (NS_SUCCEEDED(rv = aNode->QueryInterface(NS_GET_IID(nsIRDFDate), (void**) &dateLiteral)))
    {
    PRInt64     theDate, million;
        if (NS_SUCCEEDED(rv = dateLiteral->GetValue( &theDate )))
        {
            LL_I2L(million, PR_USEC_PER_SEC);
            LL_DIV(theDate, theDate, million);          
            PRInt32     now32;
            LL_L2I(now32, theDate);
            aResult.Truncate();
            aResult.AppendInt(now32, 10);
        }
        NS_RELEASE(dateLiteral);
    }
    else if (NS_SUCCEEDED(rv = aNode->QueryInterface(NS_GET_IID(nsIRDFInt), (void**) &intLiteral)))
    {
        PRInt32     theInt;
        aResult.Truncate();
        if (NS_SUCCEEDED(rv = intLiteral->GetValue( &theInt )))
        {
            aResult.AppendInt(theInt, 10);
        }
        NS_RELEASE(intLiteral);
    }
    else if (NS_SUCCEEDED(rv = aNode->QueryInterface(NS_GET_IID(nsIRDFLiteral), (void**) &literal)))
    {
        const PRUnichar     *p = nsnull;
        if (NS_SUCCEEDED(rv = literal->GetValueConst( &p )) && (p))
        {
            aResult = p;
        }
        NS_RELEASE(literal);
    }
    else
    {
        NS_ERROR("not a resource or a literal");
        rv = NS_ERROR_UNEXPECTED;
    }

    return rv;
}

nsresult
nsBookmarksService::WriteBookmarkProperties(nsIRDFDataSource *ds,
    nsIOutputStream* strm, nsIRDFResource *child, nsIRDFResource *property,
    const char *htmlAttrib, PRBool isFirst)
{
    nsresult  rv;
    PRUint32  dummy;

    nsCOMPtr<nsIRDFNode>    node;
    if (NS_SUCCEEDED(rv = ds->GetTarget(child, property, PR_TRUE, getter_AddRefs(node)))
        && (rv != NS_RDF_NO_VALUE))
    {
        nsAutoString    literalString;
        if (NS_SUCCEEDED(rv = GetTextForNode(node, literalString)))
        {
            if (property == kNC_URL) {
                
                PRInt32 offset;
                while ((offset = literalString.FindChar('\"')) >= 0) {
                    literalString.Cut(offset, 1);
                    literalString.Insert(NS_LITERAL_STRING("%22"), offset);
                }
            }

            char        *attribute = ToNewUTF8String(literalString);
            if (nsnull != attribute)
            {
                if (isFirst == PR_FALSE)
                {
                    rv |= strm->Write(kSpaceStr, sizeof(kSpaceStr)-1, &dummy);
                }

                if (property == kNC_Description)
                {
                    if (!literalString.IsEmpty())
                    {
                        char *escapedAttrib = nsEscapeHTML(attribute);
                        if (escapedAttrib)
                        {
                            rv |= strm->Write(htmlAttrib, strlen(htmlAttrib), &dummy);
                            rv |= strm->Write(escapedAttrib, strlen(escapedAttrib), &dummy);
                            rv |= strm->Write(kNL, sizeof(kNL)-1, &dummy);

                            NS_Free(escapedAttrib);
                            escapedAttrib = nsnull;
                        }
                    }
                }
                else
                {
                    rv |= strm->Write(htmlAttrib, strlen(htmlAttrib), &dummy);
                    rv |= strm->Write(attribute, strlen(attribute), &dummy);
                    rv |= strm->Write(kQuoteStr, sizeof(kQuoteStr)-1, &dummy);
                }
                NS_Free(attribute);
                attribute = nsnull;
            }
        }
    }
    if (NS_FAILED(rv))
        return NS_ERROR_UNEXPECTED;
    
    return NS_OK;
}

PRBool
nsBookmarksService::CanAccept(nsIRDFResource* aSource,
                  nsIRDFResource* aProperty,
                  nsIRDFNode* aTarget)
{
    nsresult    rv;
    PRBool      isBookmarkedFlag = PR_FALSE, canAcceptFlag = PR_FALSE, isOrdinal;

    if (NS_SUCCEEDED(rv = IsBookmarkedResource(aSource, &isBookmarkedFlag)) &&
        (isBookmarkedFlag == PR_TRUE) &&
        (NS_SUCCEEDED(rv = gRDFC->IsOrdinalProperty(aProperty, &isOrdinal))))
    {
        if (isOrdinal == PR_TRUE)
        {
            canAcceptFlag = PR_TRUE;
        }
        else if ((aProperty == kNC_Description) ||
             (aProperty == kNC_Name) ||
             (aProperty == kNC_ShortcutURL) ||
             (aProperty == kNC_URL) ||
             (aProperty == kWEB_LastModifiedDate) ||
             (aProperty == kWEB_LastVisitDate) ||
             (aProperty == kNC_BookmarkAddDate) ||
             (aProperty == kRDF_nextVal) ||
             (aProperty == kRDF_type) ||
             (aProperty == kWEB_Schedule))
        {
            canAcceptFlag = PR_TRUE;
        }
    }
    return canAcceptFlag;
}







NS_IMETHODIMP
nsBookmarksService::OnAssert(nsIRDFDataSource* aDataSource,
                 nsIRDFResource* aSource,
                 nsIRDFResource* aProperty,
                 nsIRDFNode* aTarget)
{
    if (mUpdateBatchNest != 0)  return NS_OK;

    PRInt32 count = mObservers.Count();
    for (PRInt32 i = 0; i < count; ++i)
    {
        (void) mObservers[i]->OnAssert(this, aSource, aProperty, aTarget);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::OnUnassert(nsIRDFDataSource* aDataSource,
                   nsIRDFResource* aSource,
                   nsIRDFResource* aProperty,
                   nsIRDFNode* aTarget)
{
    if (mUpdateBatchNest != 0)  return NS_OK;

    PRInt32 count = mObservers.Count();
    for (PRInt32 i = 0; i < count; ++i)
    {
        (void) mObservers[i]->OnUnassert(this, aSource, aProperty, aTarget);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::OnChange(nsIRDFDataSource* aDataSource,
                 nsIRDFResource* aSource,
                 nsIRDFResource* aProperty,
                 nsIRDFNode* aOldTarget,
                 nsIRDFNode* aNewTarget)
{
    if (mUpdateBatchNest != 0)  return NS_OK;

    PRInt32 count = mObservers.Count();
    for (PRInt32 i = 0; i < count; ++i)
    {
        (void) mObservers[i]->OnChange(this, aSource, aProperty, aOldTarget, aNewTarget);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::OnMove(nsIRDFDataSource* aDataSource,
               nsIRDFResource* aOldSource,
               nsIRDFResource* aNewSource,
               nsIRDFResource* aProperty,
               nsIRDFNode* aTarget)
{
    if (mUpdateBatchNest != 0)  return NS_OK;

    PRInt32 count = mObservers.Count();
    for (PRInt32 i = 0; i < count; ++i)
    {
        (void) mObservers[i]->OnMove(this, aOldSource, aNewSource, aProperty, aTarget);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::OnBeginUpdateBatch(nsIRDFDataSource* aDataSource)
{
    if (mUpdateBatchNest++ == 0)
    {
        PRInt32 count = mObservers.Count();
        for (PRInt32 i = 0; i < count; ++i) {
            (void) mObservers[i]->OnBeginUpdateBatch(this);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsBookmarksService::OnEndUpdateBatch(nsIRDFDataSource* aDataSource)
{
    NS_ASSERTION(mUpdateBatchNest > 0, "badly nested update batch");

    if (--mUpdateBatchNest == 0)
    {
        PRInt32 count = mObservers.Count();
        for (PRInt32 i = 0; i < count; ++i) {
            (void) mObservers[i]->OnEndUpdateBatch(this);
        }
    }

    return NS_OK;
}

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsBookmarksService, Init)

static const nsModuleComponentInfo components[] = {
    { "Bookmarks", NS_BOOKMARKS_SERVICE_CID, NS_BOOKMARKS_SERVICE_CONTRACTID,
      nsBookmarksServiceConstructor },
    { "Bookmarks", NS_BOOKMARKS_SERVICE_CID,
      "@mozilla.org/embeddor.implemented/bookmark-charset-resolver;1",
      nsBookmarksServiceConstructor },
    { "Bookmarks", NS_BOOKMARKS_SERVICE_CID, NS_BOOKMARKS_DATASOURCE_CONTRACTID,
      nsBookmarksServiceConstructor }
};

NS_IMPL_NSGETMODULE(BookmarkModule, components)
