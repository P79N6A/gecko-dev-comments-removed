






































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


try {
  var annosvc = Cc["@mozilla.org/browser/annotation-service;1"].getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotation service\n");
} 


try {
  var mssvc = Cc["@mozilla.org/microsummary/service;1"].getService(Ci.nsIMicrosummaryService);
} catch(ex) {
  do_throw("Could not get microsummary service\n");
}

const DESCRIPTION_ANNO = "bookmarkProperties/description";


function run_test() {
  
  var importer = Cc["@mozilla.org/browser/places/import-export-service;1"].getService(Ci.nsIPlacesImportExportService);

  
  var bookmarksFileOld = do_get_file("browser/components/places/tests/unit/bookmarks.preplaces.html");
  var bookmarksFileNew = dirSvc.get("ProfD", Ci.nsILocalFile);
  bookmarksFileNew.append("bookmarks.exported.html");

  
  if (!bookmarksFileNew.exists())
    bookmarksFileNew.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
  if (!bookmarksFileNew.exists())
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
    
    var testFolder = rootNode.getChild(3);
    do_check_eq(testFolder.type, testFolder.RESULT_TYPE_FOLDER);
    do_check_eq(testFolder.title, "test");
    testFolder = testFolder.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    do_check_eq(testFolder.hasChildren, true);
    
    do_check_true(annosvc.itemHasAnnotation(testFolder.itemId,
                                            DESCRIPTION_ANNO));
    do_check_eq("folder test comment",
                annosvc.getItemAnnotationString(testFolder.itemId,
                                                DESCRIPTION_ANNO));
    
    testFolder.containerOpen = true;
    var cc = testFolder.childCount;
    
    
    do_check_eq(cc, 1);

    
    var testBookmark1 = testFolder.getChild(0);
    
    do_check_eq("http://test/post", testBookmark1.uri);
    
    do_check_eq("test post keyword", testBookmark1.title);
    
    do_check_eq("test", bmsvc.getKeywordForBookmark(testBookmark1.itemId));
    
    
    
    
    
    
    do_check_true(annosvc.itemHasAnnotation(testBookmark1.itemId,
                                            DESCRIPTION_ANNO));
    do_check_eq("item description",
                annosvc.getItemAnnotationString(testBookmark1.itemId,
                                                DESCRIPTION_ANNO));
    

















    
    testFolder.containerOpen = false;
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("bookmarks query tests failed: " + ex);
  }
}
