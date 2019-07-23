





































var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);
var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);


function run_test() {

  
  var testFolder = bmsvc.createFolder(bmsvc.placesRoot, "test Folder",
                                      bmsvc.DEFAULT_INDEX);

  
  var query = histsvc.getNewQuery();
  query.setFolders([testFolder], 1);
  var result = histsvc.executeQuery(query, histsvc.getNewQueryOptions());
  var rootNode = result.root;
  rootNode.containerOpen = true;

  
  bmsvc.runInBatchMode({
    runBatched: function(aUserData) {
      
      var bookmarkId = bmsvc.insertBookmark(testFolder, uri("http://google.com/"),
                                            bmsvc.DEFAULT_INDEX, "");

      
      do_check_eq(rootNode.childCount, 0);
    }
  }, null);

  
  
  do_check_eq(rootNode.childCount, 1);

  rootNode.containerOpen = false;
}
