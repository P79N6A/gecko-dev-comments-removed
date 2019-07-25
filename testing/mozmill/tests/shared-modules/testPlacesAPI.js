












































var MODULE_NAME = 'PlacesAPI';

const gTimeout = 5000;






var bookmarksService = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                       getService(Ci.nsINavBookmarksService);






var historyService = Cc["@mozilla.org/browser/nav-history-service;1"].
                     getService(Ci.nsINavHistoryService);






var livemarkService = Cc["@mozilla.org/browser/livemark-service;2"].
                      getService(Ci.nsILivemarkService);







var browserHistory = Cc["@mozilla.org/browser/nav-history-service;1"].
                     getService(Ci.nsIBrowserHistory);











function isBookmarkInFolder(uri, folderId)
{
  var ids = bookmarksService.getBookmarkIdsForURI(uri, {});
  for (let i = 0; i < ids.length; i++) {
    if (bookmarksService.getFolderIdForItem(ids[i]) == folderId)
      return true;
  }

  return false;
}




function restoreDefaultBookmarks() {
  
  let dirService = Cc["@mozilla.org/file/directory_service;1"].
                   getService(Ci.nsIProperties);

  bookmarksFile = dirService.get("profDef", Ci.nsILocalFile);
  bookmarksFile.append("bookmarks.html");

  
  let importer = Cc["@mozilla.org/browser/places/import-export-service;1"].
                 getService(Ci.nsIPlacesImportExportService);
  importer.importHTMLFromFile(bookmarksFile, true);
}





function removeAllHistory() {
  const TOPIC_EXPIRATION_FINISHED = "places-expiration-finished";

  
  var finishedFlag = {
    state: false
  }

  
  var observerService = Cc["@mozilla.org/observer-service;1"].
                        getService(Ci.nsIObserverService);
  let observer = {
    observe: function(aSubject, aTopic, aData) {
      observerService.removeObserver(this, TOPIC_EXPIRATION_FINISHED);    
      finishedFlag.state = true;
    }
  }
  observerService.addObserver(observer, TOPIC_EXPIRATION_FINISHED, false);

  
  browserHistory.removeAllPages();
  mozmill.controller.waitForEval("subject.state == true", gTimeout, 100, finishedFlag);
}
