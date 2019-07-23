




































#ifndef bookmarksservice___h___
#define bookmarksservice___h___

#include "nsIRDFDataSource.h"
#include "nsIRDFRemoteDataSource.h"
#include "nsIStreamListener.h"
#include "nsIRDFObserver.h"
#include "nsISupportsArray.h"
#include "nsCOMArray.h"
#include "nsIStringBundle.h"
#include "nsITimer.h"
#include "nsIRDFNode.h"
#include "nsIBookmarksService.h"
#include "nsStringAPI.h"
#include "nsIFile.h"
#include "nsIObserver.h"
#include "nsWeakReference.h"
#include "nsCOMArray.h"
#include "nsIIOService.h"
#include "nsICacheService.h"
#include "nsICacheSession.h"
#include "nsIPrefBranch.h"
#include "nsICharsetResolver.h"
#include "nsCycleCollectionParticipant.h"

class nsIOutputStream;

#ifdef DEBUG
#ifdef XP_MACOSX
#include <Timer.h>
#endif
#endif

class nsBookmarksService : public nsIBookmarksService,
                           public nsIRDFDataSource,
                           public nsIRDFRemoteDataSource,
                           public nsIStreamListener,
                           public nsICharsetResolver,
                           public nsIRDFObserver,
                           public nsIObserver,
                           public nsSupportsWeakReference
{
protected:
    nsCOMPtr<nsIRDFDataSource>      mInner;
    nsCOMPtr<nsIRDFResource>        busyResource;
    nsCOMArray<nsIRDFObserver>      mObservers;
    nsCOMPtr<nsIStringBundle>       mBundle;
    nsCOMPtr<nsITimer>              mTimer;
    nsCOMPtr<nsIIOService>          mNetService;
    nsCOMPtr<nsICacheService>       mCacheService;
    nsCOMPtr<nsICacheSession>       mCacheSession;

    PRUint32      htmlSize;
    PRInt32       mUpdateBatchNest;
    nsString      mPersonalToolbarName;
    PRBool        mBookmarksAvailable;
    PRBool        mDirty;
    PRBool        mBrowserIcons;
    PRBool        busySchedule;
    PRBool        mNeedBackupUpdate;

public:
    
    
    nsCOMPtr<nsIRDFResource>        mLivemarkLoadingBookmark;
    nsCOMPtr<nsIRDFResource>        mLivemarkLoadFailedBookmark;
    nsCOMPtr<nsIPrefBranch>         mBookmarksPrefs;

    
    static nsresult UpdateBookmarkForwardProxy(nsIRDFDataSource* aDS, nsIRDFResource* aBookmarkResource);

protected:
    static void FireTimer(nsITimer* aTimer, void* aClosure);

    nsresult ExamineBookmarkSchedule(nsIRDFResource *theBookmark, PRBool & examineFlag);

    nsresult GetBookmarkToPing(nsIRDFResource **theBookmark);

    nsresult WriteBookmarks(nsIFile* bookmarksFile, nsIRDFDataSource *ds,
                            nsIRDFResource *root);

    nsresult WriteBookmarksContainer(nsIRDFDataSource *ds,
                                     nsIOutputStream* strm,
                                     nsIRDFResource *container,
                                     PRInt32 level,
                                     nsCOMArray<nsIRDFResource>& parentArray);

    nsresult WriteBookmarkIdAndName(nsIRDFDataSource *aDs,
                                    nsIOutputStream* aStrm,
                                    nsIRDFResource *aNode);

    nsresult WriteBookmarkProperties(nsIRDFDataSource *aDs,
                                     nsIOutputStream* aStrm,
                                     nsIRDFResource *aNode,
                                     nsIRDFResource *aProperty,
                                     const char *aHtmlAttrib,
                                     PRBool aIsFirst);

    nsresult SerializeBookmarks(nsIURI* aURI);

    nsresult GetTextForNode(nsIRDFNode* aNode, nsString& aResult);

    nsresult GetSynthesizedType(nsIRDFResource *aNode, nsIRDFNode **aType);

    nsresult UpdateBookmarkLastModifiedDate(nsIRDFResource *aSource);

    PRBool   CanAccept(nsIRDFResource* aSource, nsIRDFResource* aProperty, nsIRDFNode* aTarget);

    nsresult getArgumentN(nsISupportsArray *arguments, nsIRDFResource *res,
                          PRInt32 offset, nsIRDFNode **argValue);

    nsresult setFolderHint(nsIRDFResource *src, nsIRDFResource *objType);

    nsresult getFolderViaHint(nsIRDFResource *src, PRBool fallbackFlag,
                              nsIRDFResource **folder);

    nsresult importBookmarks(nsISupportsArray *aArguments);

    nsresult exportBookmarks(nsISupportsArray *aArguments);

    void AnnotateBookmarkSchedule(nsIRDFResource* aSource,
                                  PRBool scheduleFlag);

    nsresult InsertResource(nsIRDFResource* aResource,
                            nsIRDFResource* aParentFolder, PRInt32 aIndex);

    nsresult getLocaleString(const char *key, nsString &str);

    nsresult GetURLFromResource(nsIRDFResource* aResource, nsAString& aURL);

    nsresult LoadBookmarks();
    nsresult ArchiveBookmarksFile(PRInt32 numberOfBackups, PRBool forceArchive);
    nsresult InitDataSource();

    nsresult GetLastModifiedFolders(nsISimpleEnumerator **aResult);

    PRBool LivemarkNeedsUpdate(nsIRDFResource* aSource);
    nsresult UpdateLivemarkChildren(nsIRDFResource* aSource);

    void SaveToBackup();

    
    NS_DECL_NSIREQUESTOBSERVER

    
    NS_DECL_NSISTREAMLISTENER

    
    NS_DECL_NSICHARSETRESOLVER

    
    NS_DECL_NSIOBSERVER

public:
    nsBookmarksService();
    virtual ~nsBookmarksService();
    nsresult Init();

    
    void RemoveBookmark(nsIRDFResource* aBookmark);
    nsresult ClearBookmarksContainer(nsIRDFResource* aContainer);

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsBookmarksService,
                                             nsIBookmarksService)

    
    NS_DECL_NSIBOOKMARKSSERVICE

    
    NS_IMETHOD GetURI(char* *uri);

    NS_IMETHOD GetSource(nsIRDFResource* property,
                         nsIRDFNode* target,
                         PRBool tv,
                         nsIRDFResource** source)
    {
        return mInner->GetSource(property, target, tv, source);
    }

    NS_IMETHOD GetSources(nsIRDFResource* property,
                          nsIRDFNode* target,
                          PRBool tv,
                          nsISimpleEnumerator** sources)
    {
        return mInner->GetSources(property, target, tv, sources);
    }

    NS_IMETHOD GetTarget(nsIRDFResource* source,
                         nsIRDFResource* property,
                         PRBool tv,
                         nsIRDFNode** target);

    NS_IMETHOD GetTargets(nsIRDFResource* source,
                          nsIRDFResource* property,
                          PRBool tv,
                          nsISimpleEnumerator** targets);

    NS_IMETHOD Assert(nsIRDFResource* aSource,
                      nsIRDFResource* aProperty,
                      nsIRDFNode* aTarget,
                      PRBool aTruthValue);

    NS_IMETHOD Unassert(nsIRDFResource* aSource,
                        nsIRDFResource* aProperty,
                        nsIRDFNode* aTarget);

    NS_IMETHOD Change(nsIRDFResource* aSource,
                      nsIRDFResource* aProperty,
                      nsIRDFNode* aOldTarget,
                      nsIRDFNode* aNewTarget);
    
    NS_IMETHOD Move(nsIRDFResource* aOldSource,
                    nsIRDFResource* aNewSource,
                    nsIRDFResource* aProperty,
                    nsIRDFNode* aTarget);
            
    NS_IMETHOD HasAssertion(nsIRDFResource* source,
                            nsIRDFResource* property,
                            nsIRDFNode* target,
                            PRBool tv,
                            PRBool* hasAssertion);

    NS_IMETHOD AddObserver(nsIRDFObserver* aObserver);
    NS_IMETHOD RemoveObserver(nsIRDFObserver* aObserver);

    NS_IMETHOD HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *_retval);
    NS_IMETHOD HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, PRBool *_retval);

    NS_IMETHOD ArcLabelsIn(nsIRDFNode* node,
                           nsISimpleEnumerator** labels)
    {
        return mInner->ArcLabelsIn(node, labels);
    }

    NS_IMETHOD ArcLabelsOut(nsIRDFResource* source,
                            nsISimpleEnumerator** labels);

    NS_IMETHOD GetAllResources(nsISimpleEnumerator** aResult);

    NS_IMETHOD GetAllCmds(nsIRDFResource* source,
                          nsISimpleEnumerator** commands);

    NS_IMETHOD IsCommandEnabled(nsISupportsArray* aSources,
                                nsIRDFResource*   aCommand,
                                nsISupportsArray* aArguments,
                                PRBool* aResult);

    NS_IMETHOD DoCommand(nsISupportsArray* aSources,
                         nsIRDFResource*   aCommand,
                         nsISupportsArray* aArguments);

    NS_IMETHOD BeginUpdateBatch() {
        return mInner->BeginUpdateBatch();
    }

    NS_IMETHOD EndUpdateBatch() {
        return mInner->EndUpdateBatch();
    }

    
    NS_DECL_NSIRDFREMOTEDATASOURCE

    
    NS_DECL_NSIRDFOBSERVER
};

#endif
