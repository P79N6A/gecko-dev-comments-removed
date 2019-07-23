






































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get nav-history-service\n");
}


try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


function run_test() {
  
  var importer = Cc["@mozilla.org/browser/places/import-export-service;1"].getService(Ci.nsIPlacesImportExportService);

  
  var bookmarksFileOld = do_get_file("browser/components/places/tests/unit/bookmarks.preplaces.html");
  var bookmarksFileNew = dirSvc.get("ProfD", Ci.nsILocalFile);
  bookmarksFileNew.append("bookmarks.exported.html");

  
  if (!bookmarksFileOld.exists())
    bookmarksFileOld.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
  if (!bookmarksFileOld.exists())
    do_throw("couldn't create file: bookmarks.exported.html");

  
  
  
  bmsvc.removeFolderChildren(bmsvc.bookmarksRoot);
  importer.importHTMLFromFile(bookmarksFileOld);
  testCanonicalBookmarks(bmsvc.bookmarksRoot); 

  
  
  
  
  






  
  
  
  
  





  









  







}



function testCanonicalBookmarks(aFolder) {
  
  try {
    var query = histsvc.getNewQuery();
    query.setFolders([bmsvc.bookmarksRoot], 1);
    var result = histsvc.executeQuery(query, histsvc.getNewQueryOptions());
    var rootNode = result.root;
    rootNode.containerOpen = true;
    
    var idx = 3; 
    var testFolder = rootNode.getChild(idx);
    do_check_eq(testFolder.type, testFolder.RESULT_TYPE_FOLDER);
    do_check_eq(testFolder.title, "test");
    testFolder = testFolder.QueryInterface(Ci.nsINavHistoryFolderResultNode);
    do_check_eq(testFolder.hasChildren, true);
    
    testFolder.containerOpen = true;
    var cc = testFolder.childCount;
    do_check_eq(cc, 2);
    
    var testBookmark1 = testFolder.getChild(0);
    
    do_check_eq("http://test/post", testBookmark1.uri);
    
    do_check_eq("test post keyword", testBookmark1.title);
    
    do_check_eq("test", bmsvc.getKeywordForBookmark(testBookmark1.itemId));
    
    
    
    
    
    

    
    var testBookmark2 = testFolder.getChild(1);
    
    do_check_eq("http://test/micsum", testBookmark2.uri);
    
    do_check_eq("test microsummary", testBookmark2.title);
    
    
    

    
    testFolder.containerOpen = false;
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("bookmarks query tests failed: " + ex);
  }
}
