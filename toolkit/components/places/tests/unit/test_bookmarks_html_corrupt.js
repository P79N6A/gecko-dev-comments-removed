




const DESCRIPTION_ANNO = "bookmarkProperties/description";
const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";

const TEST_FAVICON_PAGE_URL = "http://en-US.www.mozilla.com/en-US/firefox/central/";
const TEST_FAVICON_DATA_SIZE = 580;

function run_test() {
  run_next_test();
}

add_task(function* test_corrupt_file() {
  
  Services.prefs.setIntPref("browser.places.smartBookmarksVersion", -1);

  
  let corruptHtml = OS.Path.join(do_get_cwd().path, "bookmarks.corrupt.html");
  yield BookmarkHTMLUtils.importFromFile(corruptHtml, true);

  
  yield PlacesTestUtils.promiseAsyncUpdates();
  yield database_check();
});

add_task(function* test_corrupt_database() {
  
  let corruptBookmark = yield PlacesUtils.bookmarks.insert({ parentGuid: PlacesUtils.bookmarks.toolbarGuid,
                                                             url: "http://test.mozilla.org",
                                                             title: "We love belugas" });
  let db = yield PlacesUtils.promiseWrappedConnection();
  yield db.execute("UPDATE moz_bookmarks SET fk = NULL WHERE guid = :guid",
                   { guid: corruptBookmark.guid });

  let bookmarksFile = OS.Path.join(OS.Constants.Path.profileDir, "bookmarks.exported.html");
  if ((yield OS.File.exists(bookmarksFile)))
    yield OS.File.remove(bookmarksFile);
  yield BookmarkHTMLUtils.exportToFile(bookmarksFile);

  
  remove_all_bookmarks();
  yield BookmarkHTMLUtils.importFromFile(bookmarksFile, true);
  yield PlacesTestUtils.promiseAsyncUpdates();
  yield database_check();
});








let database_check = Task.async(function* () {
  
  let root = PlacesUtils.getFolderContents(PlacesUtils.bookmarksMenuFolderId).root;
  Assert.equal(root.childCount, 2);

  let folderNode = root.getChild(1);
  Assert.equal(folderNode.type, folderNode.RESULT_TYPE_FOLDER);
  Assert.equal(folderNode.title, "test");
  Assert.equal(PlacesUtils.bookmarks.getItemDateAdded(folderNode.itemId), 1177541020000000);
  Assert.equal(PlacesUtils.bookmarks.getItemLastModified(folderNode.itemId), 1177541050000000);
  Assert.equal("folder test comment",
               PlacesUtils.annotations.getItemAnnotation(folderNode.itemId,
                                                         DESCRIPTION_ANNO));
  
  PlacesUtils.asQuery(folderNode);
  Assert.equal(folderNode.hasChildren, true);
  folderNode.containerOpen = true;
  Assert.equal(folderNode.childCount, 1);

  let bookmarkNode = folderNode.getChild(0);
  Assert.equal("http://test/post", bookmarkNode.uri);
  Assert.equal("test post keyword", bookmarkNode.title);

  let entry = yield PlacesUtils.keywords.fetch({ url: bookmarkNode.uri });
  Assert.equal("test", entry.keyword);
  Assert.equal("hidden1%3Dbar&text1%3D%25s", entry.postData);

  Assert.ok(PlacesUtils.annotations.itemHasAnnotation(bookmarkNode.itemId,
                                                      LOAD_IN_SIDEBAR_ANNO));
  Assert.equal(bookmarkNode.dateAdded, 1177375336000000);
  Assert.equal(bookmarkNode.lastModified, 1177375423000000);

  Assert.equal((yield PlacesUtils.getCharsetForURI(NetUtil.newURI(bookmarkNode.uri))),
               "ISO-8859-1");

  Assert.equal("item description",
               PlacesUtils.annotations.getItemAnnotation(bookmarkNode.itemId,
                                                         DESCRIPTION_ANNO));

  
  folderNode.containerOpen = false;
  root.containerOpen = false;

  
  root = PlacesUtils.getFolderContents(PlacesUtils.toolbarFolderId).root;
  Assert.equal(root.childCount, 3);

  let livemarkNode = root.getChild(1);
  Assert.equal("Latest Headlines", livemarkNode.title);

  let livemark = yield PlacesUtils.livemarks.getLivemark({ id: livemarkNode.itemId });
  Assert.equal("http://en-us.fxfeeds.mozilla.com/en-US/firefox/livebookmarks/",
               livemark.siteURI.spec);
  Assert.equal("http://en-us.fxfeeds.mozilla.com/en-US/firefox/headlines.xml",
               livemark.feedURI.spec);

  
  root.containerOpen = false;

  
  root = PlacesUtils.getFolderContents(PlacesUtils.unfiledBookmarksFolderId).root;
  Assert.equal(root.childCount, 1);
  root.containerOpen = false;

  
  yield new Promise(resolve => {
    PlacesUtils.favicons.getFaviconDataForPage(uri(TEST_FAVICON_PAGE_URL),
      (aURI, aDataLen, aData, aMimeType) => {
        
        Assert.notEqual(aURI, null);
        
        
        
        Assert.equal(TEST_FAVICON_DATA_SIZE, aDataLen);
        resolve();
      });
  });
});
