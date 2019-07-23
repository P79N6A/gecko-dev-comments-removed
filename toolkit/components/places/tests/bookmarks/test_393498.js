





































try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get services\n");
}


var observer = {
  onBeginUpdateBatch: function() {},
  onEndUpdateBatch: function() {},
  onItemAdded: function(id, folder, index) {
    this._itemAddedId = id;
    this._itemAddedParent = folder;
    this._itemAddedIndex = index;
  },
  onBeforeItemRemoved: function(id) {},
  onItemRemoved: function(id, folder, index) {},
  _itemChangedProperty: null,
  onItemChanged: function(id, property, isAnnotationProperty, value) {
    this._itemChangedId = id;
    this._itemChangedProperty = property;
    this._itemChanged_isAnnotationProperty = isAnnotationProperty;
    this._itemChangedValue = value;
  },
  onItemVisited: function(id, visitID, time) {},
  onItemMoved: function(id, oldParent, oldIndex, newParent, newIndex) {},
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsINavBookmarkObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};
bmsvc.addObserver(observer, false);


function run_test() {
  var testFolder = bmsvc.createFolder(bmsvc.placesRoot, "test Folder",
                                      bmsvc.DEFAULT_INDEX);
  var bookmarkId = bmsvc.insertBookmark(testFolder, uri("http://google.com/"),
                                   bmsvc.DEFAULT_INDEX, "");
  do_check_true(observer.itemChangedProperty == null);

  
  
  var newDate = Date.now() * 1000 - 1;
  bmsvc.setItemDateAdded(bookmarkId, newDate);
  
  do_check_eq(observer._itemChangedProperty, "dateAdded");
  do_check_eq(observer._itemChangedValue, newDate);
  
  var dateAdded = bmsvc.getItemDateAdded(bookmarkId);
  do_check_eq(dateAdded, newDate);

  
  var lastModified = bmsvc.getItemLastModified(bookmarkId);
  do_check_eq(lastModified, dateAdded);

  bmsvc.setItemLastModified(bookmarkId, newDate);
  
  do_check_eq(observer._itemChangedProperty, "lastModified");
  do_check_eq(observer._itemChangedValue, newDate);
  
  do_check_eq(bmsvc.getItemLastModified(bookmarkId), newDate);

  
  bmsvc.setItemTitle(bookmarkId, "Google");
  do_check_eq(observer._itemChangedId, bookmarkId);
  do_check_eq(observer._itemChangedProperty, "title");
  do_check_eq(observer._itemChangedValue, "Google");

  
  do_check_true(bmsvc.getItemLastModified(bookmarkId) > newDate);

  
  var query = histsvc.getNewQuery();
  query.setFolders([testFolder], 1);
  var result = histsvc.executeQuery(query, histsvc.getNewQueryOptions());
  var rootNode = result.root;
  rootNode.containerOpen = true;
  do_check_eq(rootNode.childCount, 1);
  var childNode = rootNode.getChild(0);

  
  do_check_eq(bmsvc.getItemDateAdded(bookmarkId), childNode.dateAdded);
  do_check_eq(bmsvc.getItemLastModified(bookmarkId), childNode.lastModified);

  
  
  
  var pastDate = Date.now() * 1000 - 1;
  bmsvc.setItemLastModified(bookmarkId, pastDate);
  
  var oldLastModified = bmsvc.getItemLastModified(bookmarkId);
  bmsvc.setItemTitle(bookmarkId, "Google");
  
  do_check_true(oldLastModified < childNode.lastModified);
  
  do_check_eq(bmsvc.getItemLastModified(bookmarkId), childNode.lastModified);

  
  newDate = Date.now() * 1000;
  bmsvc.setItemDateAdded(bookmarkId, newDate);
  do_check_eq(childNode.dateAdded, newDate);
  bmsvc.setItemLastModified(bookmarkId, newDate);
  do_check_eq(childNode.lastModified, newDate);

  rootNode.containerOpen = false;
}
