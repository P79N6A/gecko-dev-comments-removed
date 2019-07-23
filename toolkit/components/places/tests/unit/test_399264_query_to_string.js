













































function query_string(aFolderID)
{
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);

  var query = hs.getNewQuery();
  query.setFolders([aFolderID], 1);
  var options = hs.getNewQueryOptions();
  return hs.queriesToQueryString([query], 1, options);
}

function run_test()
{
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);

  const QUERIES = [
      "folder=PLACES_ROOT"
    , "folder=BOOKMARKS_MENU"
    , "folder=TAGS"
    , "folder=UNFILED_BOOKMARKS"
    , "folder=TOOLBAR"
  ];
  const FOLDER_IDS = [
      bs.placesRoot
    , bs.bookmarksMenuFolder
    , bs.tagsFolder
    , bs.unfiledBookmarksFolder
    , bs.toolbarFolder
  ];


  for (var i = 0; i < QUERIES.length; i++) {
    var result = query_string(FOLDER_IDS[i]);
    dump("Looking for '" + QUERIES[i] + "' in '" + result + "'\n");
    do_check_neq(-1, result.indexOf(QUERIES[i]));
  }
}
