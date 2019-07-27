const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
const DESCRIPTION_ANNO = "bookmarkProperties/description";

let tagData = [
  { uri: uri("http://slint.us"), tags: ["indie", "kentucky", "music"] },
  { uri: uri("http://en.wikipedia.org/wiki/Diplodocus"), tags: ["dinosaur", "dj", "rad word"] }
];

let bookmarkData = [
  { uri: uri("http://slint.us"), title: "indie, kentucky, music" },
  { uri: uri("http://en.wikipedia.org/wiki/Diplodocus"), title: "dinosaur, dj, rad word" }
];

function run_test() {
  run_next_test();
}












add_task(function* () {
  
  let jsonFile = OS.Path.join(OS.Constants.Path.profileDir, "bookmarks.exported.json");
  if ((yield OS.File.exists(jsonFile)))
    yield OS.File.remove(jsonFile);

  
  
  let htmlFile = OS.Path.join(do_get_cwd().path, "bookmarks.preplaces.html");
  yield BookmarkHTMLUtils.importFromFile(htmlFile, true);

  
  for (let { uri, tags } of tagData) {
    PlacesUtils.tagging.tagURI(uri, tags);
  }
  for (let { uri, title } of bookmarkData) {
    yield PlacesUtils.bookmarks.insert({ parentGuid: PlacesUtils.bookmarks.unfiledGuid,
                                         url: uri,
                                         title });
  }
  for (let { uri, title } of bookmarkData) {
    yield PlacesUtils.bookmarks.insert({ parentGuid: PlacesUtils.bookmarks.toolbarGuid,
                                         url: uri,
                                         title });
  }

  yield validate();

  
  
  yield BookmarkJSONUtils.exportToFile(jsonFile);
  do_print("exported json");

  
  
  yield BookmarkJSONUtils.importFromFile(jsonFile, true);
  do_print("imported json");

  
  yield validate();
  do_print("validated import");
});

function* validate() {
  yield testMenuBookmarks();
  yield testToolbarBookmarks();
  testUnfiledBookmarks();
  testTags();
  yield PlacesTestUtils.promiseAsyncUpdates();
}



function* testMenuBookmarks() {
  let root = PlacesUtils.getFolderContents(PlacesUtils.bookmarksMenuFolderId).root;
  Assert.equal(root.childCount, 3);

  let separatorNode = root.getChild(1);
  Assert.equal(separatorNode.type, separatorNode.RESULT_TYPE_SEPARATOR);

  let folderNode = root.getChild(2);
  Assert.equal(folderNode.type, folderNode.RESULT_TYPE_FOLDER);
  Assert.equal(folderNode.title, "test");
  let folder = yield PlacesUtils.bookmarks.fetch(folderNode.bookmarkGuid);
  Assert.equal(folder.dateAdded.getTime(), 1177541020000);

  Assert.equal(PlacesUtils.asQuery(folderNode).hasChildren, true);

  Assert.equal("folder test comment",
              PlacesUtils.annotations.getItemAnnotation(folderNode.itemId,
                                                        DESCRIPTION_ANNO));

  
  folderNode.containerOpen = true;
  Assert.equal(folderNode.childCount, 1);

  let bookmarkNode = folderNode.getChild(0);
  Assert.equal("http://test/post", bookmarkNode.uri);
  Assert.equal("test post keyword", bookmarkNode.title);
  Assert.ok(PlacesUtils.annotations.itemHasAnnotation(bookmarkNode.itemId,
                                                      LOAD_IN_SIDEBAR_ANNO));
  Assert.equal(bookmarkNode.dateAdded, 1177375336000000);

  let entry = yield PlacesUtils.keywords.fetch({ url: bookmarkNode.uri });
  Assert.equal("test", entry.keyword);
  Assert.equal("hidden1%3Dbar&text1%3D%25s", entry.postData);

  Assert.equal("ISO-8859-1",
               (yield PlacesUtils.getCharsetForURI(NetUtil.newURI(bookmarkNode.uri))));
  Assert.equal("item description",
              PlacesUtils.annotations.getItemAnnotation(bookmarkNode.itemId,
                                                        DESCRIPTION_ANNO));

  folderNode.containerOpen = false;
  root.containerOpen = false;
}

function* testToolbarBookmarks() {
  let root = PlacesUtils.getFolderContents(PlacesUtils.toolbarFolderId).root;

  
  Assert.equal(root.childCount, bookmarkData.length + 2);
  
  let livemarkNode = root.getChild(1);
  Assert.equal("Latest Headlines", livemarkNode.title);

  let livemark = yield PlacesUtils.livemarks.getLivemark({ id: livemarkNode.itemId });
  Assert.equal("http://en-us.fxfeeds.mozilla.com/en-US/firefox/livebookmarks/",
               livemark.siteURI.spec);
  Assert.equal("http://en-us.fxfeeds.mozilla.com/en-US/firefox/headlines.xml",
               livemark.feedURI.spec);

  
  let bookmarkNode = root.getChild(2);
  Assert.equal(bookmarkNode.uri, bookmarkData[0].uri.spec);
  Assert.equal(bookmarkNode.title, bookmarkData[0].title);
  bookmarkNode = root.getChild(3);
  Assert.equal(bookmarkNode.uri, bookmarkData[1].uri.spec);
  Assert.equal(bookmarkNode.title, bookmarkData[1].title);

  root.containerOpen = false;
}

function testUnfiledBookmarks() {
  let root = PlacesUtils.getFolderContents(PlacesUtils.unfiledBookmarksFolderId).root;
  
  Assert.equal(root.childCount, bookmarkData.length + 1);
  for (let i = 1; i < root.childCount; ++i) {
    let child = root.getChild(i);
    Assert.equal(child.uri, bookmarkData[i - 1].uri.spec);
    Assert.equal(child.title, bookmarkData[i - 1].title);
    if (child.tags)
      Assert.equal(child.tags, bookmarkData[i - 1].title);
  }
  root.containerOpen = false;
}

function testTags() {
  for (let { uri, tags } of tagData) {
    do_print("Test tags for " + uri.spec + ": " + tags + "\n");
    let foundTags = PlacesUtils.tagging.getTagsForURI(uri);
    Assert.equal(foundTags.length, tags.length);
    Assert.ok(tags.every(tag => foundTags.indexOf(tag) != -1));
  }
}
