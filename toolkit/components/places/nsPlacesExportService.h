



#ifndef nsPlacesExportService_h_
#define nsPlacesExportService_h_

#include "nsIPlacesImportExportService.h"

#include "nsCOMPtr.h"
#include "nsIFile.h"
#include "nsIOutputStream.h"
#include "nsIFaviconService.h"
#include "nsIAnnotationService.h"
#include "mozIAsyncLivemarks.h"
#include "nsINavHistoryService.h"
#include "nsINavBookmarksService.h"
#include "nsIChannel.h"

class nsPlacesExportService : public nsIPlacesImportExportService
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPLACESIMPORTEXPORTSERVICE
    nsPlacesExportService();

  


  static nsPlacesExportService* GetSingleton();

  


  nsresult Init();

  private:
    static nsPlacesExportService* gExportService;
    virtual ~nsPlacesExportService();

  protected:
    nsCOMPtr<nsIFaviconService> mFaviconService;
    nsCOMPtr<nsIAnnotationService> mAnnotationService;
    nsCOMPtr<nsINavBookmarksService> mBookmarksService;
    nsCOMPtr<nsINavHistoryService> mHistoryService;
    nsCOMPtr<mozIAsyncLivemarks> mLivemarkService;

    nsresult WriteContainer(nsINavHistoryResultNode* aFolder, const nsACString& aIndent, nsIOutputStream* aOutput);
    nsresult WriteContainerHeader(nsINavHistoryResultNode* aFolder, const nsACString& aIndent, nsIOutputStream* aOutput);
    nsresult WriteTitle(nsINavHistoryResultNode* aItem, nsIOutputStream* aOutput);
    nsresult WriteItem(nsINavHistoryResultNode* aItem, const nsACString& aIndent, nsIOutputStream* aOutput);
    nsresult WriteLivemark(nsINavHistoryResultNode* aFolder, const nsACString& aIndent, nsIOutputStream* aOutput);
    nsresult WriteContainerContents(nsINavHistoryResultNode* aFolder, const nsACString& aIndent, nsIOutputStream* aOutput);
    nsresult WriteSeparator(nsINavHistoryResultNode* aItem, const nsACString& aIndent, nsIOutputStream* aOutput);
    nsresult WriteDescription(int64_t aId, int32_t aType, nsIOutputStream* aOutput);

    inline nsresult EnsureServiceState() {
      NS_ENSURE_STATE(mHistoryService);
      NS_ENSURE_STATE(mFaviconService);
      NS_ENSURE_STATE(mAnnotationService);
      NS_ENSURE_STATE(mBookmarksService);
      NS_ENSURE_STATE(mLivemarkService);
      return NS_OK;
    }
};

#endif 
