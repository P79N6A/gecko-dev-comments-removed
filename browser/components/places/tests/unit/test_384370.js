







































Cc["@mozilla.org/browser/livemark-service;2"].getService(Ci.nsILivemarkService);
Cc["@mozilla.org/feed-processor;1"].createInstance(Ci.nsIFeedProcessor);

const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const DESCRIPTION_ANNO = "bookmarkProperties/description";
const POST_DATA_ANNO = "bookmarkProperties/POSTData";

Components.utils.import("resource://gre/modules/utils.js");
do_check_eq(typeof PlacesUtils, "object");


function run_test() {
  











  
  var importer = Cc["@mozilla.org/browser/places/import-export-service;1"].getService(Ci.nsIPlacesImportExportService);

  
  Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch).
  setIntPref("browser.places.smartBookmarksVersion", -1);

  
  
  var bookmarksFileOld = do_get_file("bookmarks.preplaces.html");
  
  var jsonFile = dirSvc.get("ProfD", Ci.nsILocalFile);
  jsonFile.append("bookmarks.exported.json");

  
  if (jsonFile.exists())
    jsonFile.remove(false);
  jsonFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, 0600);
  if (!jsonFile.exists())
    do_throw("couldn't create file: bookmarks.exported.json");

  
  
  
  
  try {
    importer.importHTMLFromFile(bookmarksFileOld, true);
  } catch(ex) { do_throw("couldn't import legacy bookmarks file: " + ex); }
  populate();
  validate();

  
  
  
  
  
  try {
    PlacesUtils.backups.saveBookmarksToJSONFile(jsonFile);
  } catch(ex) { do_throw("couldn't export to file: " + ex); }
  LOG("exported json"); 
  try {
    PlacesUtils.backups.restoreBookmarksFromJSONFile(jsonFile);
  } catch(ex) { do_throw("couldn't import the exported file: " + ex); }
  LOG("imported json"); 
  validate();
  LOG("validated import"); 
}

var tagData = [
  { uri: uri("http://slint.us"), tags: ["indie", "kentucky", "music"] },
  { uri: uri("http://en.wikipedia.org/wiki/Diplodocus"), tags: ["dinosaur", "dj", "rad word"] }
];

var bookmarkData = [
  { uri: uri("http://slint.us"), title: "indie, kentucky, music" },
  { uri: uri("http://en.wikipedia.org/wiki/Diplodocus"), title: "dinosaur, dj, rad word" }
];





function populate() {
  
  for each(let {uri: u, tags: t} in tagData)
    PlacesUtils.tagging.tagURI(u, t);
  
  
  for each(let {uri: u, title: t} in bookmarkData) {
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarks.unfiledBookmarksFolder,
                                         u, PlacesUtils.bookmarks.DEFAULT_INDEX, t);
  }

  
  for each(let {uri: u, title: t} in bookmarkData) {
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarks.toolbarFolder,
                                         u, PlacesUtils.bookmarks.DEFAULT_INDEX, t);
  }
}

function validate() {
  testCanonicalBookmarks(PlacesUtils.bookmarks.bookmarksMenuFolder);
  testToolbarFolder();
  testUnfiledBookmarks();
  testTags();
}



function testCanonicalBookmarks() {
  
  var query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.bookmarks.bookmarksMenuFolder], 1);
  var result = PlacesUtils.history.executeQuery(query, PlacesUtils.history.getNewQueryOptions());
  var rootNode = result.root;
  rootNode.containerOpen = true;

  
  
  do_check_eq(rootNode.childCount, 4);

  
  var testFolder = rootNode.getChild(3);
  do_check_eq(testFolder.type, testFolder.RESULT_TYPE_FOLDER);
  do_check_eq(testFolder.title, "test");

  






  testFolder = testFolder.QueryInterface(Ci.nsINavHistoryQueryResultNode);
  do_check_eq(testFolder.hasChildren, true);
  
  do_check_true(PlacesUtils.annotations.itemHasAnnotation(testFolder.itemId,
                                                          DESCRIPTION_ANNO));
  do_check_eq("folder test comment",
              PlacesUtils.annotations.getItemAnnotation(testFolder.itemId, DESCRIPTION_ANNO));
  
  testFolder.containerOpen = true;
  var cc = testFolder.childCount;
  
  
  do_check_eq(cc, 1);

  
  var testBookmark1 = testFolder.getChild(0);
  
  do_check_eq("http://test/post", testBookmark1.uri);
  
  do_check_eq("test post keyword", testBookmark1.title);
  
  do_check_eq("test", PlacesUtils.bookmarks.getKeywordForBookmark(testBookmark1.itemId));
  
  do_check_true(PlacesUtils.annotations.itemHasAnnotation(testBookmark1.itemId,
                                                          LOAD_IN_SIDEBAR_ANNO));
  







  
  do_check_true(PlacesUtils.annotations.itemHasAnnotation(testBookmark1.itemId, POST_DATA_ANNO));
  do_check_eq("hidden1%3Dbar&text1%3D%25s",
              PlacesUtils.annotations.getItemAnnotation(testBookmark1.itemId, POST_DATA_ANNO));

  
  var testURI = PlacesUtils._uri(testBookmark1.uri);
  do_check_eq("ISO-8859-1", PlacesUtils.history.getCharsetForURI(testURI));

  
  do_check_true(PlacesUtils.annotations.itemHasAnnotation(testBookmark1.itemId,
                                                          DESCRIPTION_ANNO));
  do_check_eq("item description",
              PlacesUtils.annotations.getItemAnnotation(testBookmark1.itemId,
                                                        DESCRIPTION_ANNO));

  

















  
  testFolder.containerOpen = false;
  rootNode.containerOpen = false;
}

function testToolbarFolder() {
  var query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.bookmarks.toolbarFolder], 1);
  var result = PlacesUtils.history.executeQuery(query, PlacesUtils.history.getNewQueryOptions());

  var toolbar = result.root;
  toolbar.containerOpen = true;

  
  do_check_eq(toolbar.childCount, bookmarkData.length + 2);
  
  
  var livemark = toolbar.getChild(1);
  
  do_check_eq("Latest Headlines", livemark.title);
  
  do_check_true(PlacesUtils.livemarks.isLivemark(livemark.itemId));
  
  do_check_eq("http://en-us.fxfeeds.mozilla.com/en-US/firefox/livebookmarks/",
              PlacesUtils.livemarks.getSiteURI(livemark.itemId).spec);
  
  do_check_eq("http://en-us.fxfeeds.mozilla.com/en-US/firefox/headlines.xml",
              PlacesUtils.livemarks.getFeedURI(livemark.itemId).spec);

  
  var child = toolbar.getChild(2);
  do_check_eq(child.uri, bookmarkData[0].uri.spec);
  do_check_eq(child.title, bookmarkData[0].title);
  child = toolbar.getChild(3);
  do_check_eq(child.uri, bookmarkData[1].uri.spec);
  do_check_eq(child.title, bookmarkData[1].title);

  toolbar.containerOpen = false;
}

function testUnfiledBookmarks() {
  var query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.bookmarks.unfiledBookmarksFolder], 1);
  var result = PlacesUtils.history.executeQuery(query, PlacesUtils.history.getNewQueryOptions());
  var rootNode = result.root;
  rootNode.containerOpen = true;
  
  do_check_eq(rootNode.childCount, bookmarkData.length + 1);
  for (var i = 1; i < rootNode.childCount; i++) {
    var child = rootNode.getChild(i);
    dump(bookmarkData[i - 1].uri.spec + " == " + child.uri + "?\n");
    do_check_true(bookmarkData[i - 1].uri.equals(uri(child.uri)));
    do_check_eq(child.title, bookmarkData[i - 1].title);
    



  }
  rootNode.containerOpen = false;
}

function testTags() {
  for each(let {uri: u, tags: t} in tagData) {
    var i = 0;
    dump("test tags for " + u.spec + ": " + t + "\n");
    var tt = PlacesUtils.tagging.getTagsForURI(u, {});
    dump("true tags for " + u.spec + ": " + tt + "\n");
    do_check_true(t.every(function(el) {
      i++;
      return tt.indexOf(el) > -1;
    }));
    do_check_eq(i, t.length);
  }
}
