






































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
  var livemarksvc = Cc["@mozilla.org/browser/livemark-service;2"].getService(Ci.nsILivemarkService);
} catch(ex) {
  do_throw("Could not get livemark service\n");
} 


try {
  var mssvc = Cc["@mozilla.org/microsummary/service;1"].getService(Ci.nsIMicrosummaryService);
} catch(ex) {
  do_throw("Could not get microsummary service\n");
}


try {
  var iosvc = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
} catch (ex) {
  do_throw("Could not get io service\n");
}

const DESCRIPTION_ANNO = "bookmarkProperties/description";
const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const POST_DATA_ANNO = "bookmarkProperties/POSTData";


function run_test() {
  
  var importer = Cc["@mozilla.org/browser/places/import-export-service;1"].getService(Ci.nsIPlacesImportExportService);

  
  Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch).
  setBoolPref("browser.places.createdSmartBookmarks", true);

  
  var bookmarksFileOld = do_get_file("browser/components/places/tests/unit/bookmarks.preplaces.html");
  
  var bookmarksFileNew = dirSvc.get("ProfD", Ci.nsILocalFile);
  bookmarksFileNew.append("bookmarks.exported.html");

  
  if (bookmarksFileNew.exists())
    bookmarksFileNew.remove(false);
  bookmarksFileNew.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
  if (!bookmarksFileNew.exists())
    do_throw("couldn't create file: bookmarks.exported.html");

  
  
  
  
  try {
    importer.importHTMLFromFile(bookmarksFileOld, true);
  } catch(ex) { do_throw("couldn't import legacy bookmarks file: " + ex); }
  testCanonicalBookmarks(bmsvc.bookmarksMenuFolder);

  
  
  
  
  
  try {
    importer.exportHTMLToFile(bookmarksFileNew);
  } catch(ex) { do_throw("couldn't export to file: " + ex); }
  bmsvc.removeFolderChildren(bmsvc.bookmarksMenuFolder);
  bmsvc.removeFolderChildren(bmsvc.toolbarFolder);
  try {
    importer.importHTMLFromFile(bookmarksFileNew, true);
  } catch(ex) { do_throw("couldn't import the exported file: " + ex); }
  testCanonicalBookmarks(bmsvc.bookmarksMenuFolder);

  












































  








}



function testCanonicalBookmarks(aFolder) {
  
  var query = histsvc.getNewQuery();
  query.setFolders([aFolder], 1);
  var result = histsvc.executeQuery(query, histsvc.getNewQueryOptions());
  var rootNode = result.root;
  rootNode.containerOpen = true;

  
  
  do_check_eq(rootNode.childCount, 4);

  
  var testFolder = rootNode.getChild(3);
  do_check_eq(testFolder.type, testFolder.RESULT_TYPE_FOLDER);
  do_check_eq(testFolder.title, "test");

  
  do_check_eq(bmsvc.getItemDateAdded(testFolder.itemId)/1000000, 1177541020);
  
  do_check_eq(bmsvc.getItemLastModified(testFolder.itemId)/1000000, 1177541050);

  testFolder = testFolder.QueryInterface(Ci.nsINavHistoryQueryResultNode);
  do_check_eq(testFolder.hasChildren, true);
  
  do_check_true(annosvc.itemHasAnnotation(testFolder.itemId,
                                          DESCRIPTION_ANNO));
  do_check_eq("folder test comment",
              annosvc.getItemAnnotation(testFolder.itemId, DESCRIPTION_ANNO));
  
  testFolder.containerOpen = true;
  var cc = testFolder.childCount;
  
  
  do_check_eq(cc, 1);

  
  var testBookmark1 = testFolder.getChild(0);
  
  do_check_eq("http://test/post", testBookmark1.uri);
  
  do_check_eq("test post keyword", testBookmark1.title);
  
  do_check_eq("test", bmsvc.getKeywordForBookmark(testBookmark1.itemId));
  
  do_check_true(annosvc.itemHasAnnotation(testBookmark1.itemId,
                                          LOAD_IN_SIDEBAR_ANNO));
  
  do_check_eq(testBookmark1.dateAdded/1000000, 1177375336);

  
  do_check_eq(testBookmark1.lastModified/1000000, 1177375423);

  
  do_check_true(annosvc.itemHasAnnotation(testBookmark1.itemId,
                                          POST_DATA_ANNO));
  do_check_eq("hidden1%3Dbar&text1%3D%25s",
              annosvc.getItemAnnotation(testBookmark1.itemId, POST_DATA_ANNO));

  
  var testURI = uri(testBookmark1.uri);
  do_check_eq("ISO-8859-1", histsvc.getCharsetForURI(testURI));

  
  do_check_true(annosvc.itemHasAnnotation(testBookmark1.itemId,
                                          DESCRIPTION_ANNO));
  do_check_eq("item description",
              annosvc.getItemAnnotation(testBookmark1.itemId,
                                        DESCRIPTION_ANNO));

  
















  
  testFolder.containerOpen = false;
  rootNode.containerOpen = false;

  query.setFolders([bmsvc.toolbarFolder], 1);
  result = histsvc.executeQuery(query, histsvc.getNewQueryOptions());
  
  var toolbar = result.root;
  toolbar.containerOpen = true;
  do_check_eq(toolbar.childCount, 2);
  
  
  var livemark = toolbar.getChild(1);
  
  do_check_eq("Latest Headlines", livemark.title);
  
  do_check_true(livemarksvc.isLivemark(livemark.itemId));
  
  do_check_eq("http://en-us.fxfeeds.mozilla.com/en-US/firefox/livebookmarks/",
              livemarksvc.getSiteURI(livemark.itemId).spec);
  
  do_check_eq("http://en-us.fxfeeds.mozilla.com/en-US/firefox/headlines.xml",
              livemarksvc.getFeedURI(livemark.itemId).spec);

  toolbar.containerOpen = false;
  
  
  query.setFolders([bmsvc.unfiledBookmarksFolder], 1);
  result = histsvc.executeQuery(query, histsvc.getNewQueryOptions());
  var unfiledBookmarks = result.root;
  unfiledBookmarks.containerOpen = true;
  do_check_eq(unfiledBookmarks.childCount, 1);
  unfiledBookmarks.containerOpen = false;
}
