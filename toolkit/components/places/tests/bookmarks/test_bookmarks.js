





let bs = PlacesUtils.bookmarks;
let hs = PlacesUtils.history;
let anno = PlacesUtils.annotations;


let bookmarksObserver = {
  onBeginUpdateBatch: function() {
    this._beginUpdateBatch = true;
  },
  onEndUpdateBatch: function() {
    this._endUpdateBatch = true;
  },
  onItemAdded: function(id, folder, index, itemType, uri, title, dateAdded,
                        guid) {
    this._itemAddedId = id;
    this._itemAddedParent = folder;
    this._itemAddedIndex = index;
    this._itemAddedURI = uri;
    this._itemAddedTitle = title;

    
    let stmt = DBConn().createStatement(
      `SELECT guid
       FROM moz_bookmarks
       WHERE id = :item_id`
    );
    stmt.params.item_id = id;
    do_check_true(stmt.executeStep());
    do_check_false(stmt.getIsNull(0));
    do_check_valid_places_guid(stmt.row.guid);
    do_check_eq(stmt.row.guid, guid);
    stmt.finalize();
  },
  onItemRemoved: function(id, folder, index, itemType) {
    this._itemRemovedId = id;
    this._itemRemovedFolder = folder;
    this._itemRemovedIndex = index;
  },
  onItemChanged: function(id, property, isAnnotationProperty, value,
                          lastModified, itemType) {
    this._itemChangedId = id;
    this._itemChangedProperty = property;
    this._itemChanged_isAnnotationProperty = isAnnotationProperty;
    this._itemChangedValue = value;
  },
  onItemVisited: function(id, visitID, time) {
    this._itemVisitedId = id;
    this._itemVisitedVistId = visitID;
    this._itemVisitedTime = time;
  },
  onItemMoved: function(id, oldParent, oldIndex, newParent, newIndex,
                        itemType) {
    this._itemMovedId = id
    this._itemMovedOldParent = oldParent;
    this._itemMovedOldIndex = oldIndex;
    this._itemMovedNewParent = newParent;
    this._itemMovedNewIndex = newIndex;
  },
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavBookmarkObserver,
  ])
};



let root = bs.bookmarksMenuFolder;

let bmStartIndex = 0;


function run_test() {
  run_next_test();
}

add_task(function test_bookmarks() {
  bs.addObserver(bookmarksObserver, false);

  
  do_check_true(bs.placesRoot > 0);
  do_check_true(bs.bookmarksMenuFolder > 0);
  do_check_true(bs.tagsFolder > 0);
  do_check_true(bs.toolbarFolder > 0);
  do_check_true(bs.unfiledBookmarksFolder > 0);

  
  try {
    let id = bs.getFolderIdForItem(0);
    do_throw("getFolderIdForItem accepted bad input");
  } catch(ex) {}

  
  try {
    let id = bs.getFolderIdForItem(-1);
    do_throw("getFolderIdForItem accepted bad input");
  } catch(ex) {}

  
  do_check_eq(bs.getFolderIdForItem(bs.bookmarksMenuFolder), bs.placesRoot);
  do_check_eq(bs.getFolderIdForItem(bs.tagsFolder), bs.placesRoot);
  do_check_eq(bs.getFolderIdForItem(bs.toolbarFolder), bs.placesRoot);
  do_check_eq(bs.getFolderIdForItem(bs.unfiledBookmarksFolder), bs.placesRoot);

  
  
  let testRoot = bs.createFolder(root, "places bookmarks xpcshell tests",
                                 bs.DEFAULT_INDEX);
  do_check_eq(bookmarksObserver._itemAddedId, testRoot);
  do_check_eq(bookmarksObserver._itemAddedParent, root);
  do_check_eq(bookmarksObserver._itemAddedIndex, bmStartIndex);
  do_check_eq(bookmarksObserver._itemAddedURI, null);
  let testStartIndex = 0;

  
  do_check_eq(bs.getItemIndex(testRoot), bmStartIndex);

  
  do_check_eq(bs.getItemType(testRoot), bs.TYPE_FOLDER);

  
  
  let beforeInsert = Date.now() * 1000;
  do_check_true(beforeInsert > 0);

  let newId = bs.insertBookmark(testRoot, uri("http://google.com/"),
                                bs.DEFAULT_INDEX, "");
  do_check_eq(bookmarksObserver._itemAddedId, newId);
  do_check_eq(bookmarksObserver._itemAddedParent, testRoot);
  do_check_eq(bookmarksObserver._itemAddedIndex, testStartIndex);
  do_check_true(bookmarksObserver._itemAddedURI.equals(uri("http://google.com/")));
  do_check_eq(bs.getBookmarkURI(newId).spec, "http://google.com/");

  let dateAdded = bs.getItemDateAdded(newId);
  
  do_check_true(is_time_ordered(beforeInsert, dateAdded));

  
  let lastModified = bs.getItemLastModified(newId);
  do_check_eq(lastModified, dateAdded);

  
  let beforeSetTitle = Date.now() * 1000;
  do_check_true(beforeSetTitle >= beforeInsert);

  
  
  lastModified -= 1000;
  bs.setItemLastModified(newId, lastModified);
  dateAdded -= 1000;
  bs.setItemDateAdded(newId, dateAdded);

  
  bs.setItemTitle(newId, "Google");
  do_check_eq(bookmarksObserver._itemChangedId, newId);
  do_check_eq(bookmarksObserver._itemChangedProperty, "title");
  do_check_eq(bookmarksObserver._itemChangedValue, "Google");

  
  let dateAdded2 = bs.getItemDateAdded(newId);
  do_check_eq(dateAdded2, dateAdded);

  
  let lastModified2 = bs.getItemLastModified(newId);
  LOG("test setItemTitle");
  LOG("dateAdded = " + dateAdded);
  LOG("beforeSetTitle = " + beforeSetTitle);
  LOG("lastModified = " + lastModified);
  LOG("lastModified2 = " + lastModified2);
  do_check_true(is_time_ordered(lastModified, lastModified2));
  do_check_true(is_time_ordered(dateAdded, lastModified2));

  
  let title = bs.getItemTitle(newId);
  do_check_eq(title, "Google");

  
  do_check_eq(bs.getItemType(newId), bs.TYPE_BOOKMARK);

  
  try {
    let title = bs.getItemTitle(-3);
    do_throw("getItemTitle accepted bad input");
  } catch(ex) {}

  
  let folderId = bs.getFolderIdForItem(newId);
  do_check_eq(folderId, testRoot);

  
  do_check_eq(bs.getItemIndex(newId), testStartIndex);

  
  let workFolder = bs.createFolder(testRoot, "Work", 0);
  do_check_eq(bookmarksObserver._itemAddedId, workFolder);
  do_check_eq(bookmarksObserver._itemAddedParent, testRoot);
  do_check_eq(bookmarksObserver._itemAddedIndex, 0);
  do_check_eq(bookmarksObserver._itemAddedURI, null);

  do_check_eq(bs.getItemTitle(workFolder), "Work");
  bs.setItemTitle(workFolder, "Work #");
  do_check_eq(bs.getItemTitle(workFolder), "Work #");

  
  let newId2 = bs.insertBookmark(workFolder,
                                 uri("http://developer.mozilla.org/"),
                                 0, "");
  do_check_eq(bookmarksObserver._itemAddedId, newId2);
  do_check_eq(bookmarksObserver._itemAddedParent, workFolder);
  do_check_eq(bookmarksObserver._itemAddedIndex, 0);

  
  bs.setItemTitle(newId2, "DevMo");
  do_check_eq(bookmarksObserver._itemChangedProperty, "title");

  
  let newId3 = bs.insertBookmark(workFolder,
                                 uri("http://msdn.microsoft.com/"),
                                 bs.DEFAULT_INDEX, "");
  do_check_eq(bookmarksObserver._itemAddedId, newId3);
  do_check_eq(bookmarksObserver._itemAddedParent, workFolder);
  do_check_eq(bookmarksObserver._itemAddedIndex, 1);

  
  bs.setItemTitle(newId3, "MSDN");
  do_check_eq(bookmarksObserver._itemChangedProperty, "title");

  
  bs.removeItem(newId2);
  do_check_eq(bookmarksObserver._itemRemovedId, newId2);
  do_check_eq(bookmarksObserver._itemRemovedFolder, workFolder);
  do_check_eq(bookmarksObserver._itemRemovedIndex, 0);

  
  let newId4 = bs.insertBookmark(workFolder,
                                 uri("http://developer.mozilla.org/"),
                                 bs.DEFAULT_INDEX, "");
  do_check_eq(bookmarksObserver._itemAddedId, newId4);
  do_check_eq(bookmarksObserver._itemAddedParent, workFolder);
  do_check_eq(bookmarksObserver._itemAddedIndex, 1);
  
  
  let homeFolder = bs.createFolder(testRoot, "Home", bs.DEFAULT_INDEX);
  do_check_eq(bookmarksObserver._itemAddedId, homeFolder);
  do_check_eq(bookmarksObserver._itemAddedParent, testRoot);
  do_check_eq(bookmarksObserver._itemAddedIndex, 2);

  
  let newId5 = bs.insertBookmark(homeFolder, uri("http://espn.com/"),
                                 bs.DEFAULT_INDEX, "");
  do_check_eq(bookmarksObserver._itemAddedId, newId5);
  do_check_eq(bookmarksObserver._itemAddedParent, homeFolder);
  do_check_eq(bookmarksObserver._itemAddedIndex, 0);

  
  bs.setItemTitle(newId5, "ESPN");
  do_check_eq(bookmarksObserver._itemChangedId, newId5);
  do_check_eq(bookmarksObserver._itemChangedProperty, "title");

  
  let uri6 = uri("place:domain=google.com&type="+
                 Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY);
  let newId6 = bs.insertBookmark(testRoot, uri6, bs.DEFAULT_INDEX, "");
  do_check_eq(bookmarksObserver._itemAddedParent, testRoot);
  do_check_eq(bookmarksObserver._itemAddedIndex, 3);

  
  bs.setItemTitle(newId6, "Google Sites");
  do_check_eq(bookmarksObserver._itemChangedProperty, "title");

  
  do_check_eq(bs.getIdForItemAt(testRoot, 0), workFolder);
  
  do_check_eq(bs.getIdForItemAt(1337, 0), -1);
  
  do_check_eq(bs.getIdForItemAt(testRoot, 1337), -1);
  
  do_check_eq(bs.getIdForItemAt(1337, 1337), -1);

  
  let oldParentCC = getChildCount(testRoot);
  bs.moveItem(workFolder, homeFolder, bs.DEFAULT_INDEX);
  do_check_eq(bookmarksObserver._itemMovedId, workFolder);
  do_check_eq(bookmarksObserver._itemMovedOldParent, testRoot);
  do_check_eq(bookmarksObserver._itemMovedOldIndex, 0);
  do_check_eq(bookmarksObserver._itemMovedNewParent, homeFolder);
  do_check_eq(bookmarksObserver._itemMovedNewIndex, 1);

  
  do_check_eq(bs.getItemIndex(workFolder), 1);
  do_check_eq(bs.getFolderIdForItem(workFolder), homeFolder);

  
  
  do_check_neq(bs.getIdForItemAt(testRoot, 0), workFolder);
  
  do_check_neq(bs.getIdForItemAt(testRoot, -1), workFolder);
  
  do_check_eq(bs.getIdForItemAt(homeFolder, 1), workFolder);
  
  do_check_eq(bs.getIdForItemAt(homeFolder, -1), workFolder);
  
  do_check_eq(getChildCount(testRoot), oldParentCC-1);

  
  bs.moveItem(newId5, testRoot, bs.DEFAULT_INDEX);
  do_check_eq(bookmarksObserver._itemMovedId, newId5);
  do_check_eq(bookmarksObserver._itemMovedOldParent, homeFolder);
  do_check_eq(bookmarksObserver._itemMovedOldIndex, 0);
  do_check_eq(bookmarksObserver._itemMovedNewParent, testRoot);
  do_check_eq(bookmarksObserver._itemMovedNewIndex, 3);

  
  let tmpFolder = bs.createFolder(testRoot, "tmp", 2);
  do_check_eq(bs.getItemIndex(tmpFolder), 2);

  
  let kwTestItemId = bs.insertBookmark(testRoot, uri("http://keywordtest.com"),
                                       bs.DEFAULT_INDEX, "");
  bs.setKeywordForBookmark(kwTestItemId, "bar");

  
  let k = bs.getKeywordForBookmark(kwTestItemId);
  do_check_eq("bar", k);

  
  let u = bs.getURIForKeyword("bar");
  do_check_eq("http://keywordtest.com/", u.spec);

  
  
  tmpFolder = bs.createFolder(testRoot, "removeFolderChildren",
                              bs.DEFAULT_INDEX);
  bs.insertBookmark(tmpFolder, uri("http://foo9.com/"), bs.DEFAULT_INDEX, "");
  bs.createFolder(tmpFolder, "subfolder", bs.DEFAULT_INDEX);
  bs.insertSeparator(tmpFolder, bs.DEFAULT_INDEX);
  
  let options = hs.getNewQueryOptions();
  let query = hs.getNewQuery();
  query.setFolders([tmpFolder], 1);
  try {
    let result = hs.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;
    do_check_eq(rootNode.childCount, 3);
    rootNode.containerOpen = false;
  } catch(ex) {
    do_throw("test removeFolderChildren() - querying for children failed: " + ex);
  }
  
  bs.removeFolderChildren(tmpFolder);
  
  try {
    result = hs.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;
    do_check_eq(rootNode.childCount, 0);
    rootNode.containerOpen = false;
  } catch(ex) {
    do_throw("removeFolderChildren(): " + ex);
  }

  

  
  try {
    let options = hs.getNewQueryOptions();
    let query = hs.getNewQuery();
    query.setFolders([testRoot], 1);
    let result = hs.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;
    let cc = rootNode.childCount;
    LOG("bookmark itemId test: CC = " + cc);
    do_check_true(cc > 0);
    for (let i=0; i < cc; ++i) {
      let node = rootNode.getChild(i);
      if (node.type == node.RESULT_TYPE_FOLDER ||
          node.type == node.RESULT_TYPE_URI ||
          node.type == node.RESULT_TYPE_SEPARATOR ||
          node.type == node.RESULT_TYPE_QUERY) {
        do_check_true(node.itemId > 0);
      }
      else {
        do_check_eq(node.itemId, -1);
      }
    }
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("bookmarks query: " + ex);
  }

  
  
  try {
    
    let mURI = uri("http://multiple.uris.in.query");

    let testFolder = bs.createFolder(testRoot, "test Folder", bs.DEFAULT_INDEX);
    
    bs.insertBookmark(testFolder, mURI, bs.DEFAULT_INDEX, "title 1");
    bs.insertBookmark(testFolder, mURI, bs.DEFAULT_INDEX, "title 2");

    
    let options = hs.getNewQueryOptions();
    let query = hs.getNewQuery();
    query.setFolders([testFolder], 1);
    let result = hs.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;
    let cc = rootNode.childCount;
    do_check_eq(cc, 2);
    do_check_eq(rootNode.getChild(0).title, "title 1");
    do_check_eq(rootNode.getChild(1).title, "title 2");
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("bookmarks query: " + ex);
  }

  
  let newId10 = bs.insertBookmark(testRoot, uri("http://foo10.com/"),
                                  bs.DEFAULT_INDEX, "");
  dateAdded = bs.getItemDateAdded(newId10);
  
  lastModified = bs.getItemLastModified(newId10);
  do_check_eq(lastModified, dateAdded);

  
  
  lastModified -= 1000;
  bs.setItemLastModified(newId10, lastModified);
  dateAdded -= 1000;
  bs.setItemDateAdded(newId10, dateAdded);

  bs.changeBookmarkURI(newId10, uri("http://foo11.com/"));

  
  lastModified2 = bs.getItemLastModified(newId10);
  LOG("test changeBookmarkURI");
  LOG("dateAdded = " + dateAdded);
  LOG("lastModified = " + lastModified);
  LOG("lastModified2 = " + lastModified2);
  do_check_true(is_time_ordered(lastModified, lastModified2));
  do_check_true(is_time_ordered(dateAdded, lastModified2));

  do_check_eq(bookmarksObserver._itemChangedId, newId10);
  do_check_eq(bookmarksObserver._itemChangedProperty, "uri");
  do_check_eq(bookmarksObserver._itemChangedValue, "http://foo11.com/");

  
  let newId11 = bs.insertBookmark(testRoot, uri("http://foo11.com/"),
                                  bs.DEFAULT_INDEX, "");
  let bmURI = bs.getBookmarkURI(newId11);
  do_check_eq("http://foo11.com/", bmURI.spec);

  
  try {
    bs.getBookmarkURI(testRoot);
    do_throw("getBookmarkURI() should throw for non-bookmark items!");
  } catch(ex) {}

  
  let newId12 = bs.insertBookmark(testRoot, uri("http://foo11.com/"), 1, "");
  let bmIndex = bs.getItemIndex(newId12);
  do_check_eq(1, bmIndex);

  
  
  
  
  let newId13 = bs.insertBookmark(testRoot, uri("http://foobarcheese.com/"),
                                  bs.DEFAULT_INDEX, "");
  do_check_eq(bookmarksObserver._itemAddedId, newId13);
  do_check_eq(bookmarksObserver._itemAddedParent, testRoot);
  do_check_eq(bookmarksObserver._itemAddedIndex, 11);

  
  bs.setItemTitle(newId13, "ZZZXXXYYY");
  do_check_eq(bookmarksObserver._itemChangedId, newId13);
  do_check_eq(bookmarksObserver._itemChangedProperty, "title");
  do_check_eq(bookmarksObserver._itemChangedValue, "ZZZXXXYYY");

  
  bookmarksObserver._itemChangedId = -1;
  anno.setItemAnnotation(newId3, "test-annotation", "foo", 0, 0);
  do_check_eq(bookmarksObserver._itemChangedId, newId3);
  do_check_eq(bookmarksObserver._itemChangedProperty, "test-annotation");
  do_check_true(bookmarksObserver._itemChanged_isAnnotationProperty);
  do_check_eq(bookmarksObserver._itemChangedValue, "");

  
  try {
    let options = hs.getNewQueryOptions();
    options.excludeQueries = 1;
    options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
    let query = hs.getNewQuery();
    query.searchTerms = "ZZZXXXYYY";
    let result = hs.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;
    let cc = rootNode.childCount;
    do_check_eq(cc, 1);
    let node = rootNode.getChild(0);
    do_check_eq(node.title, "ZZZXXXYYY");
    do_check_true(node.itemId > 0);
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("bookmarks query: " + ex);
  }

  
  
  try {
    let options = hs.getNewQueryOptions();
    options.excludeQueries = 1;
    options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
    let query = hs.getNewQuery();
    query.searchTerms = "ZZZXXXYYY";
    let result = hs.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;
    let cc = rootNode.childCount;
    do_check_eq(cc, 1);
    let node = rootNode.getChild(0);

    do_check_eq(typeof node.dateAdded, "number");
    do_check_true(node.dateAdded > 0);
    
    do_check_eq(typeof node.lastModified, "number");
    do_check_true(node.lastModified > 0);

    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("bookmarks query: " + ex);
  }

  
  
  try {
    let options = hs.getNewQueryOptions();
    let query = hs.getNewQuery();
    query.setFolders([testRoot], 1);
    let result = hs.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;
    let cc = rootNode.childCount;
    do_check_true(cc > 0);
    for (let i = 0; i < cc; i++) {
      let node = rootNode.getChild(i);

      if (node.type == node.RESULT_TYPE_URI) {
        do_check_eq(typeof node.dateAdded, "number");
        do_check_true(node.dateAdded > 0);

        do_check_eq(typeof node.lastModified, "number");
        do_check_true(node.lastModified > 0);
        break;
      }
    }
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("bookmarks query: " + ex);
  }

  
  let newId14 = bs.insertBookmark(testRoot, uri("http://bar.tld/"),
                                  bs.DEFAULT_INDEX, "");
  dateAdded = bs.getItemDateAdded(newId14);
  lastModified = bs.getItemLastModified(newId14);
  do_check_eq(lastModified, dateAdded);
  bs.setItemLastModified(newId14, 1234000000000000);
  let fakeLastModified = bs.getItemLastModified(newId14);
  do_check_eq(fakeLastModified, 1234000000000000);
  bs.setItemDateAdded(newId14, 4321000000000000);
  let fakeDateAdded = bs.getItemDateAdded(newId14);
  do_check_eq(fakeDateAdded, 4321000000000000);
  
  
  do_check_true(anno.itemHasAnnotation(newId3, "test-annotation"));
  bs.removeItem(newId3);
  do_check_false(anno.itemHasAnnotation(newId3, "test-annotation"));

  
  let uri1 = uri("http://foo.tld/a");
  bs.insertBookmark(testRoot, uri1, bs.DEFAULT_INDEX, "");
  yield PlacesTestUtils.addVisits(uri1);

  
  let title15 = Array(TITLE_LENGTH_MAX + 5).join("X");
  let title15expected = title15.substring(0, TITLE_LENGTH_MAX);
  let newId15 = bs.insertBookmark(testRoot, uri("http://evil.com/"),
                                  bs.DEFAULT_INDEX, title15);

  do_check_eq(bs.getItemTitle(newId15).length,
              title15expected.length);
  do_check_eq(bookmarksObserver._itemAddedTitle, title15expected);
  
  bs.setItemTitle(newId15, title15 + " updated");
  do_check_eq(bs.getItemTitle(newId15).length,
              title15expected.length);
  do_check_eq(bookmarksObserver._itemChangedId, newId15);
  do_check_eq(bookmarksObserver._itemChangedProperty, "title");
  do_check_eq(bookmarksObserver._itemChangedValue, title15expected);

  testSimpleFolderResult();
});

function testSimpleFolderResult() {
  
  
  let beforeCreate = Date.now() * 1000 - 1;
  do_check_true(beforeCreate > 0);

  
  let parent = bs.createFolder(root, "test", bs.DEFAULT_INDEX);

  let dateCreated = bs.getItemDateAdded(parent);
  LOG("check that the folder was created with a valid dateAdded");
  LOG("beforeCreate = " + beforeCreate);
  LOG("dateCreated = " + dateCreated);
  do_check_true(is_time_ordered(beforeCreate, dateCreated));

  
  
  let beforeInsert = Date.now() * 1000 - 1;
  do_check_true(beforeInsert > 0);

  
  let sep = bs.insertSeparator(parent, bs.DEFAULT_INDEX);

  let dateAdded = bs.getItemDateAdded(sep);
  LOG("check that the separator was created with a valid dateAdded");
  LOG("beforeInsert = " + beforeInsert);
  LOG("dateAdded = " + dateAdded);
  do_check_true(is_time_ordered(beforeInsert, dateAdded));

  
  let item = bs.insertBookmark(parent, uri("about:blank"),
                               bs.DEFAULT_INDEX, "");
  bs.setItemTitle(item, "test bookmark");

  
  let folder = bs.createFolder(parent, "test folder", bs.DEFAULT_INDEX);
  bs.setItemTitle(folder, "test folder");

  let longName = Array(TITLE_LENGTH_MAX + 5).join("A");
  let folderLongName = bs.createFolder(parent, longName, bs.DEFAULT_INDEX);
  do_check_eq(bookmarksObserver._itemAddedTitle, longName.substring(0, TITLE_LENGTH_MAX));

  let options = hs.getNewQueryOptions();
  let query = hs.getNewQuery();
  query.setFolders([parent], 1);
  let result = hs.executeQuery(query, options);
  let rootNode = result.root;
  rootNode.containerOpen = true;
  do_check_eq(rootNode.childCount, 4);

  let node = rootNode.getChild(0);
  do_check_true(node.dateAdded > 0);
  do_check_eq(node.lastModified, node.dateAdded);
  do_check_eq(node.itemId, sep);
  do_check_eq(node.title, "");
  node = rootNode.getChild(1);
  do_check_eq(node.itemId, item);
  do_check_true(node.dateAdded > 0);
  do_check_true(node.lastModified > 0);
  do_check_eq(node.title, "test bookmark");
  node = rootNode.getChild(2);
  do_check_eq(node.itemId, folder);
  do_check_eq(node.title, "test folder");
  do_check_true(node.dateAdded > 0);
  do_check_true(node.lastModified > 0);
  node = rootNode.getChild(3);
  do_check_eq(node.itemId, folderLongName);
  do_check_eq(node.title, longName.substring(0, TITLE_LENGTH_MAX));
  do_check_true(node.dateAdded > 0);
  do_check_true(node.lastModified > 0);

  
  bs.setItemTitle(folderLongName, longName + " updated");
  do_check_eq(bookmarksObserver._itemChangedId, folderLongName);
  do_check_eq(bookmarksObserver._itemChangedProperty, "title");
  do_check_eq(bookmarksObserver._itemChangedValue, longName.substring(0, TITLE_LENGTH_MAX));

  node = rootNode.getChild(3);
  do_check_eq(node.title, longName.substring(0, TITLE_LENGTH_MAX));

  rootNode.containerOpen = false;
}

function getChildCount(aFolderId) {
  let cc = -1;
  try {
    let options = hs.getNewQueryOptions();
    let query = hs.getNewQuery();
    query.setFolders([aFolderId], 1);
    let result = hs.executeQuery(query, options);
    let rootNode = result.root;
    rootNode.containerOpen = true;
    cc = rootNode.childCount;
    rootNode.containerOpen = false;
  } catch(ex) {
    do_throw("getChildCount failed: " + ex);
  }
  return cc;
}
