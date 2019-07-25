











































var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var dbConn = hs.QueryInterface(Ci.nsPIPlacesDatabase).DBConnection;
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);
var as = Cc["@mozilla.org/browser/annotation-service;1"].
         getService(Ci.nsIAnnotationService);
var icos = Cc["@mozilla.org/browser/favicon-service;1"].
           getService(Ci.nsIFaviconService);
var ps = Cc["@mozilla.org/preferences-service;1"].
         getService(Ci.nsIPrefBranch);
var ies = Cc["@mozilla.org/browser/places/import-export-service;1"].
          getService(Ci.nsIPlacesImportExportService);
Cu.import("resource://gre/modules/BookmarkHTMLUtils.jsm");

const DESCRIPTION_ANNO = "bookmarkProperties/description";
const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const POST_DATA_ANNO = "bookmarkProperties/POSTData";

const TEST_FAVICON_PAGE_URL = "http://en-US.www.mozilla.com/en-US/firefox/central/";
const TEST_FAVICON_DATA_URL = "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAABGdBTUEAAK/INwWK6QAAABl0RVh0U29mdHdhcmUAQWRvYmUgSW1hZ2VSZWFkeXHJZTwAAAHWSURBVHjaYvz//z8DJQAggJiQOe/fv2fv7Oz8rays/N+VkfG/iYnJfyD/1+rVq7ffu3dPFpsBAAHEAHIBCJ85c8bN2Nj4vwsDw/8zQLwKiO8CcRoQu0DxqlWrdsHUwzBAAIGJmTNnPgYa9j8UqhFElwPxf2MIDeIrKSn9FwSJoRkAEEAM0DD4DzMAyPi/G+QKY4hh5WAXGf8PDQ0FGwJ22d27CjADAAIIrLmjo+MXA9R2kAHvGBA2wwx6B8W7od6CeQcggKCmCEL8bgwxYCbUIGTDVkHDBia+CuotgACCueD3TDQN75D4xmAvCoK9ARMHBzAw0AECiBHkAlC0Mdy7x9ABNA3obAZXIAa6iKEcGlMVQHwWyjYuL2d4v2cPg8vZswx7gHyAAAK7AOif7SAbOqCmn4Ha3AHFsIDtgPq/vLz8P4MSkJ2W9h8ggBjevXvHDo4FQUQg/kdypqCg4H8lUIACnQ/SOBMYI8bAsAJFPcj1AAEEjwVQqLpAbXmH5BJjqI0gi9DTAAgDBBCcAVLkgmQ7yKCZxpCQxqUZhAECCJ4XgMl493ug21ZD+aDAXH0WLM4A9MZPXJkJIIAwTAR5pQMalaCABQUULttBGCCAGCnNzgABBgAMJ5THwGvJLAAAAABJRU5ErkJggg==";

function run_test() {
  do_test_pending();

  
  ps.setIntPref("browser.places.smartBookmarksVersion", -1);

  
  var corruptBookmarksFile = do_get_file("bookmarks.corrupt.html");
  try {
    BookmarkHTMLUtils.importFromFile(corruptBookmarksFile, true, after_import);
  } catch(ex) { do_throw("couldn't import corrupt bookmarks file: " + ex); }
}

function after_import(success) {
  if (!success) {
    do_throw("Couldn't import corrupt bookmarks file.");
  }

  
  
  database_check();
  waitForAsyncUpdates(function() {
    
    var corruptItemId = bs.insertBookmark(bs.toolbarFolder,
                                          uri("http://test.mozilla.org"),
                                          bs.DEFAULT_INDEX, "We love belugas");
    var stmt = dbConn.createStatement("UPDATE moz_bookmarks SET fk = NULL WHERE id = :itemId");
    stmt.params.itemId = corruptItemId;
    stmt.execute();
    stmt.finalize();

    
    var bookmarksFile = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
    bookmarksFile.append("bookmarks.exported.html");
    if (bookmarksFile.exists())
      bookmarksFile.remove(false);
    bookmarksFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
    if (!bookmarksFile.exists())
      do_throw("couldn't create file: bookmarks.exported.html");
    try {
      ies.exportHTMLToFile(bookmarksFile);
    } catch(ex) { do_throw("couldn't export to bookmarks.exported.html: " + ex); }

    
    remove_all_bookmarks();

    
    try {
      BookmarkHTMLUtils.importFromFile(bookmarksFile, true, before_database_check);
    } catch(ex) { do_throw("couldn't import the exported file: " + ex); }
  });
}

function before_database_check(success) {
    
    database_check();

    waitForAsyncUpdates(do_test_finished);
}




function database_check() {
  
  var query = hs.getNewQuery();
  query.setFolders([bs.bookmarksMenuFolder], 1);
  var options = hs.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  var result = hs.executeQuery(query, options);
  var rootNode = result.root;
  rootNode.containerOpen = true;
  do_check_eq(rootNode.childCount, DEFAULT_BOOKMARKS_ON_MENU + 1);

  
  var testFolder = rootNode.getChild(DEFAULT_BOOKMARKS_ON_MENU);
  do_check_eq(testFolder.type, testFolder.RESULT_TYPE_FOLDER);
  do_check_eq(testFolder.title, "test");
  
  do_check_eq(bs.getItemDateAdded(testFolder.itemId)/1000000, 1177541020);
  
  do_check_eq(bs.getItemLastModified(testFolder.itemId)/1000000, 1177541050);
  testFolder = testFolder.QueryInterface(Ci.nsINavHistoryQueryResultNode);
  do_check_eq(testFolder.hasChildren, true);
  
  do_check_true(as.itemHasAnnotation(testFolder.itemId,
                                          DESCRIPTION_ANNO));
  do_check_eq("folder test comment",
              as.getItemAnnotation(testFolder.itemId, DESCRIPTION_ANNO));
  
  testFolder.containerOpen = true;
  var cc = testFolder.childCount;
  do_check_eq(cc, 1);

  
  var testBookmark1 = testFolder.getChild(0);
  
  do_check_eq("http://test/post", testBookmark1.uri);
  
  do_check_eq("test post keyword", testBookmark1.title);
  
  do_check_eq("test", bs.getKeywordForBookmark(testBookmark1.itemId));
  
  do_check_true(as.itemHasAnnotation(testBookmark1.itemId,
                                          LOAD_IN_SIDEBAR_ANNO));
  
  do_check_eq(testBookmark1.dateAdded/1000000, 1177375336);
  
  do_check_eq(testBookmark1.lastModified/1000000, 1177375423);
  
  do_check_true(as.itemHasAnnotation(testBookmark1.itemId,
                                          POST_DATA_ANNO));
  do_check_eq("hidden1%3Dbar&text1%3D%25s",
              as.getItemAnnotation(testBookmark1.itemId, POST_DATA_ANNO));
  
  var testURI = uri(testBookmark1.uri);
  do_check_eq("ISO-8859-1", hs.getCharsetForURI(testURI));
  
  do_check_true(as.itemHasAnnotation(testBookmark1.itemId,
                                          DESCRIPTION_ANNO));
  do_check_eq("item description",
              as.getItemAnnotation(testBookmark1.itemId,
                                        DESCRIPTION_ANNO));

  
  testFolder.containerOpen = false;
  rootNode.containerOpen = false;

  
  query.setFolders([bs.toolbarFolder], 1);
  result = hs.executeQuery(query, hs.getNewQueryOptions());
  var toolbar = result.root;
  toolbar.containerOpen = true;
  do_check_eq(toolbar.childCount, 3);
  
  
  var livemark = toolbar.getChild(1);
  
  do_check_eq("Latest Headlines", livemark.title);
  PlacesUtils.livemarks.getLivemark(
    { id: livemark.itemId },
    function (aStatus, aLivemark) {
      do_check_true(Components.isSuccessCode(aStatus));
      do_check_eq("http://en-us.fxfeeds.mozilla.com/en-US/firefox/livebookmarks/",
                  aLivemark.siteURI.spec);
      do_check_eq("http://en-us.fxfeeds.mozilla.com/en-US/firefox/headlines.xml",
                  aLivemark.feedURI.spec);
    }
  );

  
  toolbar.containerOpen = false;

  
  query.setFolders([bs.unfiledBookmarksFolder], 1);
  result = hs.executeQuery(query, hs.getNewQueryOptions());
  var unfiledBookmarks = result.root;
  unfiledBookmarks.containerOpen = true;
  do_check_eq(unfiledBookmarks.childCount, 1);
  unfiledBookmarks.containerOpen = false;

  
  var faviconURI = icos.getFaviconForPage(uri(TEST_FAVICON_PAGE_URL));
  var dataURL = icos.getFaviconDataAsDataURL(faviconURI);
  do_check_eq(TEST_FAVICON_DATA_URL, dataURL);
}
