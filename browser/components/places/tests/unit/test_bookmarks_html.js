






































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
  var iconsvc = Cc["@mozilla.org/browser/favicon-service;1"].getService(Ci.nsIFaviconService);
} catch(ex) {
  do_throw("Could not get favicon service\n");
} 


try {
  var iosvc = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
} catch (ex) {
  do_throw("Could not get io service\n");
}

const DESCRIPTION_ANNO = "bookmarkProperties/description";
const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const POST_DATA_ANNO = "bookmarkProperties/POSTData";

const TEST_FAVICON_PAGE_URL = "http://en-US.www.mozilla.com/en-US/firefox/central/";
const TEST_FAVICON_DATA_URL = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABGdBTUEAAK/INwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAHWSURBVHjaYvz//z8DJQAggJiQOe/fv2fv7Oz8rays/N+VkfG/iYnJfyD/1+rVq7ffu3dPFpsBAAHEAHIBCJ85c8bN2Nj4vwsDw/8zQLwKiO8CcRoQu0DxqlWrdsHUwzBAAIGJmTNnPgYa9j8UqhFElwPxf2MIDeIrKSn9FwSJoRkAEEAM0DD4DzMAyPi/G+QKY4hh5WAXGf8PDQ0FGwJ22d27CjADAAIIrLmjo+MXA9R2kAHvGBA2wwx6B8W7od6CeQcggKCmCEL8bgwxYCbUIGTDVkHDBia+CuotgACCueD3TDQN75D4xmAvCoK9ARMHBzAw0AECiBHkAlC0Mdy7x9ABNA3obAZXIAa6iKEcGlMVQHwWyjYuL2d4v2cPg8vZswx7gHyAAAK7AOif7SAbOqCmn4Ha3AHFsIDtgPq/vLz8P4MSkJ2W9h8ggBjevXvHDo4FQUQg/kdypqCg4H8lUIACnQ/SOBMYI8bAsAJFPcj1AAEEjwVQqLpAbXmH5BJjqI0gi9DTAAgDBBCcAVLkgmQ7yKCZxpCQxqUZhAECCJ4XgMl493ug21ZD+aDAXH0WLM4A9MZPXJkJIIAwTAR5pQMalaCABQUULttBGCCAGCnNzgABBgAMJ5THwGvJLAAAAABJRU5ErkJggg==";


function run_test() {
  
  var importer = Cc["@mozilla.org/browser/places/import-export-service;1"].getService(Ci.nsIPlacesImportExportService);

  
  Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch).
  setIntPref("browser.places.smartBookmarksVersion", -1);

  
  var bookmarksFileOld = do_get_file("bookmarks.preplaces.html");
  
  var bookmarksFileNew = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
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

  
  
  do_check_eq(rootNode.childCount, DEFAULT_BOOKMARKS_ON_MENU + 1);

  
  var testFolder = rootNode.getChild(DEFAULT_BOOKMARKS_ON_MENU);
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

  
  var faviconURI = iconsvc.getFaviconForPage(uri(TEST_FAVICON_PAGE_URL));
  var dataURL = iconsvc.getFaviconDataAsDataURL(faviconURI);
  do_check_eq(TEST_FAVICON_DATA_URL, dataURL);
}
