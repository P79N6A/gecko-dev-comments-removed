







































try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


try {
  var annosvc= Cc["@mozilla.org/browser/annotation-service;1"].getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotation service\n");
} 


var observer = {
  onBeginUpdateBatch: function() {
    this._beginUpdateBatch = true;
  },
  onEndUpdateBatch: function() {
    this._endUpdateBatch = true;
  },
  onItemAdded: function(id, folder, index, itemType) {
    this._itemAddedId = id;
    this._itemAddedParent = folder;
    this._itemAddedIndex = index;
  },
  onBeforeItemRemoved: function(){},
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
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsINavBookmarkObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  },
};
bmsvc.addObserver(observer, false);


var root = bmsvc.bookmarksMenuFolder;


var bmStartIndex = 0;


function run_test() {
  
  do_check_true(bmsvc.placesRoot > 0);
  do_check_true(bmsvc.bookmarksMenuFolder > 0);
  do_check_true(bmsvc.tagsFolder > 0);
  do_check_true(bmsvc.toolbarFolder > 0);
  do_check_true(bmsvc.unfiledBookmarksFolder > 0);

  
  try {
    var title = bmsvc.getFolderIdForItem(0);
    do_throw("getFolderIdForItem accepted bad input");
  } catch(ex) {}

  
  try {
    var title = bmsvc.getFolderIdForItem(-1);
    do_throw("getFolderIdForItem accepted bad input");
  } catch(ex) {}

  
  do_check_eq(bmsvc.getFolderIdForItem(bmsvc.bookmarksMenuFolder), bmsvc.placesRoot);
  do_check_eq(bmsvc.getFolderIdForItem(bmsvc.tagsFolder), bmsvc.placesRoot);
  do_check_eq(bmsvc.getFolderIdForItem(bmsvc.toolbarFolder), bmsvc.placesRoot);
  do_check_eq(bmsvc.getFolderIdForItem(bmsvc.unfiledBookmarksFolder), bmsvc.placesRoot);

  
  
  var testRoot = bmsvc.createFolder(root, "places bookmarks xpcshell tests", bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._itemAddedId, testRoot);
  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedIndex, bmStartIndex);
  var testStartIndex = 0;

  
  do_check_eq(bmsvc.getItemIndex(testRoot), bmStartIndex);

  
  do_check_eq(bmsvc.getItemType(testRoot), bmsvc.TYPE_FOLDER);

  
  
  var beforeInsert = Date.now() * 1000;
  do_check_true(beforeInsert > 0);

  var newId = bmsvc.insertBookmark(testRoot, uri("http://google.com/"),
                                   bmsvc.DEFAULT_INDEX, "");
  do_check_eq(observer._itemAddedId, newId);
  do_check_eq(observer._itemAddedParent, testRoot);
  do_check_eq(observer._itemAddedIndex, testStartIndex);
  do_check_eq(bmsvc.getBookmarkURI(newId).spec, "http://google.com/");

  var dateAdded = bmsvc.getItemDateAdded(newId);
  
  do_check_true(dateAdded >= beforeInsert);

  
  var lastModified = bmsvc.getItemLastModified(newId);
  do_check_eq(lastModified, dateAdded);

  
  var beforeSetTitle = Date.now() * 1000;
  do_check_true(beforeSetTitle >= beforeInsert);

  
  
  bmsvc.setItemLastModified(newId, --lastModified);
  bmsvc.setItemDateAdded(newId, --dateAdded);

  
  bmsvc.setItemTitle(newId, "Google");
  do_check_eq(observer._itemChangedId, newId);
  do_check_eq(observer._itemChangedProperty, "title");
  do_check_eq(observer._itemChangedValue, "Google");

  
  var dateAdded2 = bmsvc.getItemDateAdded(newId);
  do_check_eq(dateAdded2, dateAdded);

  
  var lastModified2 = bmsvc.getItemLastModified(newId);
  LOG("test setItemTitle");
  LOG("dateAdded = " + dateAdded);
  LOG("beforeSetTitle = " + beforeSetTitle);
  LOG("lastModified = " + lastModified);
  LOG("lastModified2 = " + lastModified2);
  do_check_true(lastModified2 > lastModified);
  do_check_true(lastModified2 > dateAdded);

  
  var title = bmsvc.getItemTitle(newId);
  do_check_eq(title, "Google");

  
  do_check_eq(bmsvc.getItemType(newId), bmsvc.TYPE_BOOKMARK);

  
  try {
    var title = bmsvc.getItemTitle(-3);
    do_throw("getItemTitle accepted bad input");
  } catch(ex) {}

  
  var folderId = bmsvc.getFolderIdForItem(newId);
  do_check_eq(folderId, testRoot);

  
  do_check_eq(bmsvc.getItemIndex(newId), testStartIndex);

  
  var workFolder = bmsvc.createFolder(testRoot, "Work", 0);
  do_check_eq(observer._itemAddedId, workFolder);
  do_check_eq(observer._itemAddedParent, testRoot);
  do_check_eq(observer._itemAddedIndex, 0);
  
  

  do_check_eq(bmsvc.getItemTitle(workFolder), "Work");
  bmsvc.setItemTitle(workFolder, "Work #");
  do_check_eq(bmsvc.getItemTitle(workFolder), "Work #");

  
  var newId2 = bmsvc.insertBookmark(workFolder, uri("http://developer.mozilla.org/"),
                                    0, "");
  do_check_eq(observer._itemAddedId, newId2);
  do_check_eq(observer._itemAddedParent, workFolder);
  do_check_eq(observer._itemAddedIndex, 0);

  
  bmsvc.setItemTitle(newId2, "DevMo");
  do_check_eq(observer._itemChangedProperty, "title");

  
  var newId3 = bmsvc.insertBookmark(workFolder, uri("http://msdn.microsoft.com/"),
                                    bmsvc.DEFAULT_INDEX, "");
  do_check_eq(observer._itemAddedId, newId3);
  do_check_eq(observer._itemAddedParent, workFolder);
  do_check_eq(observer._itemAddedIndex, 1);

  
  bmsvc.setItemTitle(newId3, "MSDN");
  do_check_eq(observer._itemChangedProperty, "title");

  
  bmsvc.removeItem(newId2);
  do_check_eq(observer._itemRemovedId, newId2);
  do_check_eq(observer._itemRemovedFolder, workFolder);
  do_check_eq(observer._itemRemovedIndex, 0);

  
  var newId4 = bmsvc.insertBookmark(workFolder, uri("http://developer.mozilla.org/"),
                                    bmsvc.DEFAULT_INDEX, "");
  do_check_eq(observer._itemAddedId, newId4);
  do_check_eq(observer._itemAddedParent, workFolder);
  do_check_eq(observer._itemAddedIndex, 1);
  
  
  var homeFolder = bmsvc.createFolder(testRoot, "Home", bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._itemAddedId, homeFolder);
  do_check_eq(observer._itemAddedParent, testRoot);
  do_check_eq(observer._itemAddedIndex, 2);

  
  var newId5 = bmsvc.insertBookmark(homeFolder, uri("http://espn.com/"),
                                    bmsvc.DEFAULT_INDEX, "");
  do_check_eq(observer._itemAddedId, newId5);
  do_check_eq(observer._itemAddedParent, homeFolder);
  do_check_eq(observer._itemAddedIndex, 0);

  
  bmsvc.setItemTitle(newId5, "ESPN");
  do_check_eq(observer._itemChangedId, newId5);
  do_check_eq(observer._itemChangedProperty, "title");

  
  var uri6 = uri("place:domain=google.com&type="+
                 Ci.nsINavHistoryQueryOptions.RESULTS_AS_SITE_QUERY);
  var newId6 = bmsvc.insertBookmark(testRoot, uri6, bmsvc.DEFAULT_INDEX, "");
  do_check_eq(observer._itemAddedParent, testRoot);
  do_check_eq(observer._itemAddedIndex, 3);

  
  bmsvc.setItemTitle(newId6, "Google Sites");
  do_check_eq(observer._itemChangedProperty, "title");

  
  do_check_eq(bmsvc.getIdForItemAt(testRoot, 0), workFolder);
  
  do_check_eq(bmsvc.getIdForItemAt(1337, 0), -1);
  
  do_check_eq(bmsvc.getIdForItemAt(testRoot, 1337), -1);
  
  do_check_eq(bmsvc.getIdForItemAt(1337, 1337), -1);

  
  var oldParentCC = getChildCount(testRoot);
  bmsvc.moveItem(workFolder, homeFolder, bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._itemMovedId, workFolder);
  do_check_eq(observer._itemMovedOldParent, testRoot);
  do_check_eq(observer._itemMovedOldIndex, 0);
  do_check_eq(observer._itemMovedNewParent, homeFolder);
  do_check_eq(observer._itemMovedNewIndex, 1);

  
  do_check_eq(bmsvc.getItemIndex(workFolder), 1);
  do_check_eq(bmsvc.getFolderIdForItem(workFolder), homeFolder);

  
  
  do_check_neq(bmsvc.getIdForItemAt(testRoot, 0), workFolder);
  
  do_check_neq(bmsvc.getIdForItemAt(testRoot, -1), workFolder);
  
  do_check_eq(bmsvc.getIdForItemAt(homeFolder, 1), workFolder);
  
  do_check_eq(bmsvc.getIdForItemAt(homeFolder, -1), workFolder);
  
  do_check_eq(getChildCount(testRoot), oldParentCC-1);

  
  bmsvc.moveItem(newId5, testRoot, bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._itemMovedId, newId5);
  do_check_eq(observer._itemMovedOldParent, homeFolder);
  do_check_eq(observer._itemMovedOldIndex, 0);
  do_check_eq(observer._itemMovedNewParent, testRoot);
  do_check_eq(observer._itemMovedNewIndex, 3);

  
  var tmpFolder = bmsvc.createFolder(testRoot, "tmp", 2);
  do_check_eq(bmsvc.getItemIndex(tmpFolder), 2);

  
  var kwTestItemId = bmsvc.insertBookmark(testRoot, uri("http://keywordtest.com"),
                                          bmsvc.DEFAULT_INDEX, "");
  try {
    var dateAdded = bmsvc.getItemDateAdded(kwTestItemId);
    
    var lastModified = bmsvc.getItemLastModified(kwTestItemId);
    do_check_eq(lastModified, dateAdded);

    
    
    bmsvc.setItemLastModified(kwTestItemId, --lastModified);
    bmsvc.setItemDateAdded(kwTestItemId, --dateAdded);

    bmsvc.setKeywordForBookmark(kwTestItemId, "bar");

    var lastModified2 = bmsvc.getItemLastModified(kwTestItemId);
    LOG("test setKeywordForBookmark");
    LOG("dateAdded = " + dateAdded);
    LOG("lastModified = " + lastModified);
    LOG("lastModified2 = " + lastModified2);
    do_check_true(lastModified2 > lastModified);
    do_check_true(lastModified2 > dateAdded);
  } catch(ex) {
    do_throw("setKeywordForBookmark: " + ex);
  }

  var lastModified3 = bmsvc.getItemLastModified(kwTestItemId);
  
  var k = bmsvc.getKeywordForBookmark(kwTestItemId);
  do_check_eq("bar", k);

  
  var k = bmsvc.getKeywordForURI(uri("http://keywordtest.com/"));
  do_check_eq("bar", k);

  
  var u = bmsvc.getURIForKeyword("bar");
  do_check_eq("http://keywordtest.com/", u.spec);

  
  
  var tmpFolder = bmsvc.createFolder(testRoot, "removeFolderChildren", bmsvc.DEFAULT_INDEX);
  bmsvc.insertBookmark(tmpFolder, uri("http://foo9.com/"), bmsvc.DEFAULT_INDEX,
                       "");
  bmsvc.createFolder(tmpFolder, "subfolder", bmsvc.DEFAULT_INDEX);
  bmsvc.insertSeparator(tmpFolder, bmsvc.DEFAULT_INDEX);
  
  try {
    var options = histsvc.getNewQueryOptions();
    var query = histsvc.getNewQuery();
    query.setFolders([tmpFolder], 1);
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
  } catch(ex) { do_throw("test removeFolderChildren() - querying for children failed: " + ex); }
  do_check_eq(rootNode.childCount, 3);
  rootNode.containerOpen = false;
  
  bmsvc.removeFolderChildren(tmpFolder);
  
  try {
    result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    do_check_eq(rootNode.childCount, 0);
    rootNode.containerOpen = false;
  } catch(ex) { do_throw("removeFolderChildren(): " + ex); }

  

  
  try {
    var options = histsvc.getNewQueryOptions();
    var query = histsvc.getNewQuery();
    query.setFolders([testRoot], 1);
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    LOG("bookmark itemId test: CC = " + cc);
    do_check_true(cc > 0);
    for (var i=0; i < cc; ++i) {
      var node = rootNode.getChild(i);
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
    
    var mURI = uri("http://multiple.uris.in.query");

    var testFolder = bmsvc.createFolder(testRoot, "test Folder", bmsvc.DEFAULT_INDEX);
    
    bmsvc.insertBookmark(testFolder, mURI, bmsvc.DEFAULT_INDEX, "title 1");
    bmsvc.insertBookmark(testFolder, mURI, bmsvc.DEFAULT_INDEX, "title 2");

    
    var options = histsvc.getNewQueryOptions();
    var query = histsvc.getNewQuery();
    query.setFolders([testFolder], 1);
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    do_check_eq(cc, 2);
    do_check_eq(rootNode.getChild(0).title, "title 1");
    do_check_eq(rootNode.getChild(1).title, "title 2");
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("bookmarks query: " + ex);
  }

  
  var newId10 = bmsvc.insertBookmark(testRoot, uri("http://foo10.com/"),
                                     bmsvc.DEFAULT_INDEX, "");
  var dateAdded = bmsvc.getItemDateAdded(newId10);
  
  var lastModified = bmsvc.getItemLastModified(newId10);
  do_check_eq(lastModified, dateAdded);

  
  
  bmsvc.setItemLastModified(newId10, --lastModified);
  bmsvc.setItemDateAdded(newId10, --dateAdded);

  bmsvc.changeBookmarkURI(newId10, uri("http://foo11.com/"));

  
  var lastModified2 = bmsvc.getItemLastModified(newId10);
  LOG("test changeBookmarkURI");
  LOG("dateAdded = " + dateAdded);
  LOG("lastModified = " + lastModified);
  LOG("lastModified2 = " + lastModified2);
  do_check_true(lastModified2 > lastModified);
  do_check_true(lastModified2 > dateAdded);

  do_check_eq(observer._itemChangedId, newId10);
  do_check_eq(observer._itemChangedProperty, "uri");
  do_check_eq(observer._itemChangedValue, "http://foo11.com/");

  
  var newId11 = bmsvc.insertBookmark(testRoot, uri("http://foo11.com/"),
                                     bmsvc.DEFAULT_INDEX, "");
  var bmURI = bmsvc.getBookmarkURI(newId11);
  do_check_eq("http://foo11.com/", bmURI.spec);

  
  try {
    bmsvc.getBookmarkURI(testRoot);
    do_throw("getBookmarkURI() should throw for non-bookmark items!");
  } catch(ex) {}

  
  var newId12 = bmsvc.insertBookmark(testRoot, uri("http://foo11.com/"), 1, "");
  var bmIndex = bmsvc.getItemIndex(newId12);
  do_check_eq(1, bmIndex);

  
  
  
  
  var newId13 = bmsvc.insertBookmark(testRoot, uri("http://foobarcheese.com/"),
                                     bmsvc.DEFAULT_INDEX, "");
  do_check_eq(observer._itemAddedId, newId13);
  do_check_eq(observer._itemAddedParent, testRoot);
  do_check_eq(observer._itemAddedIndex, 11);

  
  bmsvc.setItemTitle(newId13, "ZZZXXXYYY");
  do_check_eq(observer._itemChangedId, newId13);
  do_check_eq(observer._itemChangedProperty, "title");
  do_check_eq(observer._itemChangedValue, "ZZZXXXYYY");

  
  observer._itemChangedId = -1;
  annosvc.setItemAnnotation(newId3, "test-annotation", "foo", 0, 0);
  do_check_eq(observer._itemChangedId, newId3);
  do_check_eq(observer._itemChangedProperty, "test-annotation");
  do_check_true(observer._itemChanged_isAnnotationProperty);
  do_check_eq(observer._itemChangedValue, "");

  
  try {
    var options = histsvc.getNewQueryOptions();
    options.excludeQueries = 1;
    options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
    var query = histsvc.getNewQuery();
    query.searchTerms = "ZZZXXXYYY";
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    do_check_eq(cc, 1);
    var node = rootNode.getChild(0);
    do_check_eq(node.title, "ZZZXXXYYY");
    do_check_true(node.itemId > 0);
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("bookmarks query: " + ex);
  }

  
  
  try {
    var options = histsvc.getNewQueryOptions();
    options.excludeQueries = 1;
    options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
    var query = histsvc.getNewQuery();
    query.searchTerms = "ZZZXXXYYY";
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    do_check_eq(cc, 1);
    var node = rootNode.getChild(0);

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
    var options = histsvc.getNewQueryOptions();
    var query = histsvc.getNewQuery();
    query.setFolders([testRoot], 1);
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    do_check_true(cc > 0);
    for (var i = 0; i < cc; i++) {
      var node = rootNode.getChild(i);

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

  
  var newId14 = bmsvc.insertBookmark(testRoot, uri("http://bar.tld/"),
                                     bmsvc.DEFAULT_INDEX, "");
  var dateAdded = bmsvc.getItemDateAdded(newId14);
  var lastModified = bmsvc.getItemLastModified(newId14);
  do_check_eq(lastModified, dateAdded);
  bmsvc.setItemLastModified(newId14, 1234);
  var fakeLastModified = bmsvc.getItemLastModified(newId14);
  do_check_eq(fakeLastModified, 1234);
  bmsvc.setItemDateAdded(newId14, 4321);
  var fakeDateAdded = bmsvc.getItemDateAdded(newId14);
  do_check_eq(fakeDateAdded, 4321);
  
  
  do_check_true(annosvc.itemHasAnnotation(newId3, "test-annotation"));
  bmsvc.removeItem(newId3);
  do_check_false(annosvc.itemHasAnnotation(newId3, "test-annotation"));

  
  var uri1 = uri("http://foo.tld/a");
  bmsvc.insertBookmark(testRoot, uri1, bmsvc.DEFAULT_INDEX, "");
  histsvc.addVisit(uri1, Date.now() * 1000, null, histsvc.TRANSITION_TYPED, false, 0);

  testSimpleFolderResult();
}

function testSimpleFolderResult() {
  
  
  var beforeCreate = Date.now() * 1000 - 1;
  do_check_true(beforeCreate > 0);

  
  var parent = bmsvc.createFolder(root, "test", bmsvc.DEFAULT_INDEX);

  var dateCreated = bmsvc.getItemDateAdded(parent);
  LOG("check that the folder was created with a valid dateAdded");
  LOG("beforeCreate = " + beforeCreate);
  LOG("dateCreated = " + dateCreated);
  do_check_true(dateCreated > beforeCreate);

  
  
  var beforeInsert = Date.now() * 1000 - 1;
  do_check_true(beforeInsert > 0);

  
  var sep = bmsvc.insertSeparator(parent, bmsvc.DEFAULT_INDEX);

  var dateAdded = bmsvc.getItemDateAdded(sep);
  LOG("check that the separator was created with a valid dateAdded");
  LOG("beforeInsert = " + beforeInsert);
  LOG("dateAdded = " + dateAdded);
  do_check_true(dateAdded > beforeInsert);

  
  var item = bmsvc.insertBookmark(parent, uri("about:blank"),
                                  bmsvc.DEFAULT_INDEX, "");
  bmsvc.setItemTitle(item, "test bookmark");

  
  var folder = bmsvc.createFolder(parent, "test folder", bmsvc.DEFAULT_INDEX);
  bmsvc.setItemTitle(folder, "test folder");

  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.setFolders([parent], 1);
  var result = histsvc.executeQuery(query, options);
  var rootNode = result.root;
  rootNode.containerOpen = true;
  do_check_eq(rootNode.childCount, 3);

  var node = rootNode.getChild(0);
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

  rootNode.containerOpen = false;
}

function getChildCount(aFolderId) {
  var cc = -1;
  try {
    var options = histsvc.getNewQueryOptions();
    var query = histsvc.getNewQuery();
    query.setFolders([aFolderId], 1);
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    cc = rootNode.childCount;
    rootNode.containerOpen = false;
  } catch(ex) {
    do_throw("getChildCount failed: " + ex);
  }
  return cc;
}
