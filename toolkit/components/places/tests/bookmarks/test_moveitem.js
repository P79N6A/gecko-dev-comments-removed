








































try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
}


try {
  var annosvc= Cc["@mozilla.org/browser/annotation-service;1"].
               getService(Ci.nsIAnnotationService);
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
  onItemAdded: function(id, folder, index) {
    this._itemAddedId = id;
    this._itemAddedParent = folder;
    this._itemAddedIndex = index;
  },
  onItemRemoved: function(id, folder, index) {
    this._itemRemovedId = id;
    this._itemRemovedFolder = folder;
    this._itemRemovedIndex = index;
  },
  onItemChanged: function(id, property, isAnnotationProperty, value) {
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
  onItemMoved: function(id, oldParent, oldIndex, newParent, newIndex) {
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

var folder1;
var folder2;
var folder1_1;
var folder1_1_1;


function run_test() {
  
  folder1 = bmsvc.createFolder(root, "Folder1", bmsvc.DEFAULT_INDEX);
  folder2 = bmsvc.createFolder(root, "Folder2", bmsvc.DEFAULT_INDEX);
  var folders = [folder1, folder2];

  
  for (var f = 0; f < folders.length; f++) {
    for (var i = 0 ;i < 10; i++) {
      bmsvc.insertBookmark(folders[f],
                           uri("http://test." + f + "." + i + ".mozilla.org/"),
                           bmsvc.DEFAULT_INDEX,
                           "test." + f + "." + i);
    }
  }

  
  folder1_1 = bmsvc.createFolder(folder1, "Folder1_1", 5);
  do_check_eq(bmsvc.getItemIndex(folder1_1), 5);
  do_check_eq(bmsvc.getFolderIdForItem(folder1_1), folder1);

  
  for (var i = 0 ;i < 10; i++) {
    bmsvc.insertBookmark(folder1_1,
                         uri("http://test." + i + ".mozilla.org/"),
                         bmsvc.DEFAULT_INDEX,
                         "test." + i);
  }

  
  folder1_1_1 = bmsvc.createFolder(folder1_1, "Folder1_1_1", bmsvc.DEFAULT_INDEX);

  
  LOG("test 1");
  testMove(folder1_1, folder1, folder2, bmsvc.DEFAULT_INDEX);

  
  LOG("test 2");
  testMove(folder1_1, folder1, folder2, 3);

  
  LOG("test 3");
  testMove(folder1_1, folder1, folder1, bmsvc.DEFAULT_INDEX);

  
  LOG("test 4");
  testMove(folder1_1, folder1, folder1, bmsvc.getItemIndex(folder1_1));

  
  LOG("test 5");
  testMove(folder1_1, folder1, folder1, bmsvc.getItemIndex(folder1_1) - 1);

  
  LOG("test 6");
  testMove(folder1_1, folder1, folder1, bmsvc.getItemIndex(folder1_1) + 1);

  
  LOG("test 7");
  testMove(folder1_1, folder1, folder1, bmsvc.getItemIndex(folder1_1) - 3);

  
  LOG("test 8");
  testMove(folder1_1, folder1, folder1, bmsvc.getItemIndex(folder1_1) + 3);

  
  LOG("test 9");
  testMove(folder1_1, folder1, folder1, bmsvc.getItemIndex(folder1_1) - 100, true);

  
  LOG("test 10");
  testMove(folder1_1, folder1, folder1, bmsvc.getItemIndex(folder1_1) + 100, true);

  
  LOG("test 11");
  testMove(folder1_1, folder1, folder1_1, bmsvc.DEFAULT_INDEX, true);

  
  LOG("test 12");
  testMove(folder1, root, folder1_1, bmsvc.DEFAULT_INDEX, true);

  
  LOG("test 13");
  testMove(folder1, root, folder1_1_1, bmsvc.DEFAULT_INDEX, true);
}

function testMove(item, oldParent, newParent, index, expectingFailure) {
  var oldParentCount = getChildCount(oldParent);
  var oldIndex = bmsvc.getItemIndex(item);
  var newParentCount = getChildCount(newParent);
  var newIndex = index == bmsvc.DEFAULT_INDEX ? newParentCount: index;
  
  if (oldParent == newParent && newIndex > oldIndex) {
    newIndex--;
  }

  try {
    bmsvc.moveItem(item, newParent, index);
  }
  catch (e) {
    if (expectingFailure)
      return;
    else
      do_throw("moveItem Failed");
  }

  if (newParent != oldParent || newIndex != oldIndex) {
    
    
    do_check_eq(observer._itemMovedId, item);
    do_check_eq(observer._itemMovedOldParent, oldParent);
    do_check_eq(observer._itemMovedOldIndex, oldIndex);
    do_check_eq(observer._itemMovedNewParent, newParent);
    do_check_eq(observer._itemMovedNewIndex, newIndex);
  }

  
  if (oldParent != newParent) {
    do_check_eq(getChildCount(oldParent), oldParentCount - 1);
    do_check_eq(getChildCount(newParent), newParentCount + 1);
  }
  else
    do_check_eq(getChildCount(newParent), oldParentCount);

  
  do_check_eq(bmsvc.getItemIndex(item), newIndex);
  do_check_eq(bmsvc.getFolderIdForItem(item), newParent);

  if (oldParent != newParent) {
    
    
    do_check_neq(bmsvc.getIdForItemAt(oldParent, oldIndex), item);
  }
  
  do_check_eq(bmsvc.getIdForItemAt(newParent, newIndex), item);

  if (index == bmsvc.DEFAULT_INDEX) {
    
    do_check_eq(bmsvc.getIdForItemAt(newParent, -1), item);
  }

  
  
  if (oldParent == newParent && oldIndex > newIndex)
    bmsvc.moveItem(item, oldParent, oldIndex + 1);
  else
    bmsvc.moveItem(item, oldParent, oldIndex);

  
  do_check_eq(getChildCount(folder1), 11);
  do_check_eq(getChildCount(folder2), 10);
  do_check_eq(getChildCount(folder1_1), 11);
  do_check_eq(getChildCount(folder1_1_1), 0);
  do_check_eq(bmsvc.getItemIndex(folder1), 0);
  do_check_eq(bmsvc.getItemIndex(folder2), 1);
  do_check_eq(bmsvc.getItemIndex(folder1_1), 5);
  do_check_eq(bmsvc.getItemIndex(folder1_1_1), 10);
  do_check_eq(bmsvc.getItemIndex(item), oldIndex);
  do_check_eq(bmsvc.getFolderIdForItem(item), oldParent);
  do_check_eq(bmsvc.getIdForItemAt(oldParent, oldIndex), item);
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
