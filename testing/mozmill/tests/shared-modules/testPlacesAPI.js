











































var MODULE_NAME = 'PlacesAPI';






var bookmarksService = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                       getService(Ci.nsINavBookmarksService);






var historyService = Cc["@mozilla.org/browser/nav-history-service;1"].
                     getService(Ci.nsINavHistoryService);






var livemarkService = Cc["@mozilla.org/browser/livemark-service;2"].
                      getService(Ci.nsILivemarkService);











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
