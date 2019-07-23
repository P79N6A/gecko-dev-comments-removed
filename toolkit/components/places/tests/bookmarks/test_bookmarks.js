







































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


var observer = {
  onBeginUpdateBatch: function() {
    this._beginUpdateBatch = true;
  },
  onEndUpdateBatch: function() {
    this._endUpdateBatch = true;
  },
  onItemAdded: function(id, uri, folder, index) {
    this._itemAdded = uri;
    this._itemAddedId = id;
    this._itemAddedFolder = folder;
    this._itemAddedIndex = index;
  },
  onItemRemoved: function(id, uri, folder, index) {
    this._itemRemoved = uri;
    this._itemRemovedId = id;
    this._itemRemovedFolder = folder;
    this._itemRemovedIndex = index;
  },
  onItemChanged: function(id, uri, property, value) {
    this._itemChangedId = id;
    this._itemChanged = uri;
    this._itemChangedProperty = property;
    this._itemChangedValue = value;
  },
  onItemVisited: function(uri, visitID, time) {
    this._itemVisited = uri;
    this._itemVisitedID = visitID;
    this._itemVisitedTime = time;
  },
  onFolderAdded: function(folder, parent, index) {
    this._folderAdded = folder;
    this._folderAddedParent = parent;
    this._folderAddedIndex = index;
  },
  onFolderRemoved: function(folder, parent, index) {
    this._folderRemoved = folder;
    this._folderRemovedParent = parent;
    this._folderRemovedIndex = index;
  },
  onFolderMoved: function(folder, oldParent, oldIndex, newParent, newIndex) {
    this._folderMoved = folder;
    this._folderMovedOldParent = oldParent;
    this._folderMovedOldIndex = oldIndex;
    this._folderMovedNewParent = newParent;
    this._folderMovedNewIndex = newIndex;
  },
  onFolderChanged: function(folder, property) {
    this._folderChanged = folder;
    this._folderChangedProperty = property;
  },
  onSeparatorAdded: function(folder, index) {
    this._separatorAdded = folder;
    this._separatorAddedIndex = index;
  },
  onSeparatorRemoved: function(folder, index) {
    this._separatorRemoved = folder;
    this._separatorRemovedIndex = index;
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


var root = bmsvc.bookmarksRoot;


var bmStartIndex = 4;


function run_test() {
  
  do_check_true(bmsvc.placesRoot > 0);
  do_check_true(bmsvc.bookmarksRoot > 0);
  do_check_true(bmsvc.toolbarFolder > 0);

  
  
  var testRoot = bmsvc.createFolder(root, "places bookmarks xpcshell tests", bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._folderAdded, testRoot);
  do_check_eq(observer._folderAddedParent, root);
  do_check_eq(observer._folderAddedIndex, bmStartIndex);
  var testStartIndex = 0;

  
  var newId = bmsvc.insertItem(testRoot, uri("http://google.com/"), bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._itemAddedId, newId);
  do_check_eq(observer._itemAdded.spec, "http://google.com/");
  do_check_eq(observer._itemAddedFolder, testRoot);
  do_check_eq(observer._itemAddedIndex, testStartIndex);

  
  bmsvc.setItemTitle(newId, "Google");
  do_check_eq(observer._itemChangedId, newId);
  do_check_eq(observer._itemChanged.spec, "http://google.com/");
  do_check_eq(observer._itemChangedProperty, "title");
  do_check_eq(observer._itemChangedValue, "Google");

  
  var title = bmsvc.getItemTitle(newId);
  do_check_eq(title, "Google");

  
  try {
    var title = bmsvc.getItemTitle(-3);
    do_throw("getItemTitle accepted bad input");
  } catch(ex) {}

  
  var folderId = bmsvc.getFolderIdForItem(newId);
  do_check_eq(folderId, testRoot);

  
  var workFolder = bmsvc.createFolder(testRoot, "Work", 0);
  do_check_eq(observer._folderAdded, workFolder);
  do_check_eq(observer._folderAddedParent, testRoot);
  do_check_eq(observer._folderAddedIndex, 0);
  
  

  
  

  
  var newId2 = bmsvc.insertItem(workFolder, uri("http://developer.mozilla.org/"), 0);
  do_check_eq(observer._itemAddedId, newId2);
  do_check_eq(observer._itemAdded.spec, "http://developer.mozilla.org/");
  do_check_eq(observer._itemAddedFolder, workFolder);
  do_check_eq(observer._itemAddedIndex, 0);

  
  bmsvc.setItemTitle(newId2, "DevMo");
  do_check_eq(observer._itemChanged.spec, "http://developer.mozilla.org/");
  do_check_eq(observer._itemChangedProperty, "title");

  
  var newId3 = bmsvc.insertItem(workFolder, uri("http://msdn.microsoft.com/"), bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._itemAddedId, newId3);
  do_check_eq(observer._itemAdded.spec, "http://msdn.microsoft.com/");
  do_check_eq(observer._itemAddedFolder, workFolder);
  do_check_eq(observer._itemAddedIndex, 1);

  
  bmsvc.setItemTitle(newId3, "MSDN");
  do_check_eq(observer._itemChanged.spec, "http://msdn.microsoft.com/");
  do_check_eq(observer._itemChangedProperty, "title");

  
  bmsvc.removeItem(newId2);
  do_check_eq(observer._itemRemovedId, newId2);
  do_check_eq(observer._itemRemoved.spec, "http://developer.mozilla.org/");
  do_check_eq(observer._itemRemovedFolder, workFolder);
  do_check_eq(observer._itemRemovedIndex, 0);

  
  var newId4 = bmsvc.insertItem(workFolder, uri("http://developer.mozilla.org/"), bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._itemAddedId, newId4);
  do_check_eq(observer._itemAdded.spec, "http://developer.mozilla.org/");
  do_check_eq(observer._itemAddedFolder, workFolder);
  do_check_eq(observer._itemAddedIndex, 1);
  
  
  var homeFolder = bmsvc.createFolder(testRoot, "Home", bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._folderAdded, homeFolder);
  do_check_eq(observer._folderAddedParent, testRoot);
  do_check_eq(observer._folderAddedIndex, 2);

  
  var newId5 = bmsvc.insertItem(homeFolder, uri("http://espn.com/"), bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._itemAddedId, newId5);
  do_check_eq(observer._itemAdded.spec, "http://espn.com/");
  do_check_eq(observer._itemAddedFolder, homeFolder);
  do_check_eq(observer._itemAddedIndex, 0);

  
  bmsvc.setItemTitle(newId5, "ESPN");
  do_check_eq(observer._itemChanged.spec, "http://espn.com/");
  do_check_eq(observer._itemChangedProperty, "title");

  
  var newId6 = bmsvc.insertItem(testRoot, uri("place:domain=google.com&group=1"), bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._itemAdded.spec, "place:domain=google.com&group=1");
  do_check_eq(observer._itemAddedFolder, testRoot);
  do_check_eq(observer._itemAddedIndex, 3);

  
  bmsvc.setItemTitle(newId6, "Google Sites");
  do_check_eq(observer._itemChanged.spec, "place:domain=google.com&group=1");
  do_check_eq(observer._itemChangedProperty, "title");

  
  bmsvc.moveFolder(workFolder, homeFolder, bmsvc.DEFAULT_INDEX);
  do_check_eq(observer._folderMoved, workFolder);
  do_check_eq(observer._folderMovedOldParent, testRoot);
  do_check_eq(observer._folderMovedOldIndex, 0);
  do_check_eq(observer._folderMovedNewParent, homeFolder);
  do_check_eq(observer._folderMovedNewIndex, 1);
  
  do_check_eq(bmsvc.indexOfFolder(testRoot, workFolder), -1);

  
  try {
    bmsvc.moveFolder(workFolder, workFolder, bmsvc.DEFAULT_INDEX);
    do_throw("moveFolder() allowed moving a folder to be it's own parent.");
  } catch (e) {}
  do_check_eq(bmsvc.indexOfFolder(homeFolder, workFolder), 1);

  
  
  
  try {
    bmsvc.insertSeparator(testRoot, 1);
    bmsvc.removeChildAt(testRoot, 1);
  } catch(ex) {
    do_throw("insertSeparator: " + ex);
  }

  
  var tmpFolder = bmsvc.createFolder(testRoot, "tmp", 2);
  do_check_eq(bmsvc.indexOfFolder(testRoot, tmpFolder), 2);

  
  var kwTestItemId = bmsvc.insertItem(testRoot, uri("http://keywordtest.com"), bmsvc.DEFAULT_INDEX);
  try {
    bmsvc.setKeywordForBookmark(kwTestItemId, "bar");
  } catch(ex) {
    do_throw("setKeywordForBookmark: " + ex);
  }

  
  var k = bmsvc.getKeywordForBookmark(kwTestItemId);
  do_check_eq("bar", k);

  
  var k = bmsvc.getKeywordForURI(uri("http://keywordtest.com/"));
  do_check_eq("bar", k);

  
  var u = bmsvc.getURIForKeyword("bar");
  do_check_eq("http://keywordtest.com/", u.spec);

  
  var newId8 = bmsvc.insertItem(testRoot, uri("http://foo8.com/"), bmsvc.DEFAULT_INDEX);
  var b = bmsvc.getBookmarkIdsForURI(uri("http://foo8.com/"), {});
  do_check_eq(b[0], newId8);

  
  
  var tmpFolder = bmsvc.createFolder(testRoot, "removeFolderChildren", bmsvc.DEFAULT_INDEX);
  bmsvc.insertItem(tmpFolder, uri("http://foo9.com/"), bmsvc.DEFAULT_INDEX);
  bmsvc.createFolder(tmpFolder, "subfolder", bmsvc.DEFAULT_INDEX);
  bmsvc.insertSeparator(tmpFolder, bmsvc.DEFAULT_INDEX);
  
  try {
    var options = histsvc.getNewQueryOptions();
    options.maxResults = 1;
    options.setGroupingMode([Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER], 1);
    var query = histsvc.getNewQuery();
    query.setFolders([tmpFolder], 1);
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    do_check_eq(rootNode.childCount, 3);
    rootNode.containerOpen = false;
  } catch(ex) { do_throw("removeFolderChildren(): " + ex); }
  
  bmsvc.removeFolderChildren(tmpFolder);
  
  try {
    result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    do_check_eq(rootNode.childCount, 0);
    rootNode.containerOpen = false;
  } catch(ex) { do_throw("removeFolderChildren(): " + ex); }

  
  var newId9 = bmsvc.insertItem(testRoot, uri("http://foo9.com/"), bmsvc.DEFAULT_INDEX);
  var placeURI = bmsvc.getItemURI(newId9);
  do_check_eq(placeURI.spec, "place:moz_bookmarks.id=" + newId9 + "&group=3");

  

  
  try {
    var options = histsvc.getNewQueryOptions();
    options.maxResults = 1;
    options.setGroupingMode([Ci.nsINavHistoryQueryOptions.GROUP_BY_FOLDER], 1);
    var query = histsvc.getNewQuery();
    query.setFolders([testRoot], 1);
    var result = histsvc.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    for (var i=0; i < cc; ++i) {
      var node = rootNode.getChild(i);
      do_check_true(node.bookmarkId > 0);
    }
    testRoot.containerOpen = false;
  }
  catch(ex) {
    do_throw("bookmarks query: " + ex);
  }

  



























  
  var newId10 = bmsvc.insertItem(testRoot, uri("http://foo10.com/"), bmsvc.DEFAULT_INDEX);
  bmsvc.changeBookmarkURI(newId10, uri("http://foo11.com/"));
  do_check_eq(observer._itemChangedId, newId10);
  do_check_eq(observer._itemChanged.spec, "http://foo11.com/");
  do_check_eq(observer._itemChangedProperty, "uri");
  do_check_eq(observer._itemChangedValue, "");

  
  var newId11 = bmsvc.insertItem(testRoot, uri("http://foo11.com/"), bmsvc.DEFAULT_INDEX);
  var bmURI = bmsvc.getBookmarkURI(newId11);
  do_check_eq("http://foo11.com/", bmURI.spec);

  
  var newId12 = bmsvc.insertItem(testRoot, uri("http://foo11.com/"), 1);
  var bmIndex = bmsvc.getItemIndex(newId12);
  do_check_eq(1, bmIndex);

  
  var oldToolbarFolder = bmsvc.toolbarFolder;
  var newToolbarFolderId = bmsvc.createFolder(testRoot, "new toolbar folder", -1);
  bmsvc.toolbarFolder = newToolbarFolderId;
  do_check_eq(bmsvc.toolbarFolder, newToolbarFolderId);
  do_check_eq(observer._itemChangedId, newToolbarFolderId);
  do_check_eq(observer._itemChanged.spec, bmsvc.getFolderURI(newToolbarFolderId).spec);
  do_check_eq(observer._itemChangedProperty, "became_toolbar_folder");
  do_check_eq(observer._itemChangedValue, "");
}






























































