








































const Cc = Components.classes;
const Ci = Components.interfaces;

var gLibrary = null;

function test() {
  waitForExplicitFinish();

  
  ok(PlacesUtils, "PlacesUtils in context");
  ok(PlacesUIUtils, "PlacesUIUtils in context");

  
  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  var windowObserver = {
    observe: function(aSubject, aTopic, aData) {
      if (aTopic === "domwindowopened") {
        ww.unregisterNotification(this);
        gLibrary = aSubject.QueryInterface(Ci.nsIDOMWindow);
        gLibrary.addEventListener("load", function onLoad(event) {
          gLibrary.removeEventListener("load", onLoad, false);
          executeSoon(startTest);
        }, false);
      }
    }
  };
  ww.registerNotification(windowObserver);
  ww.openWindow(null,
                "chrome://browser/content/places/places.xul",
                "",
                "chrome,toolbar=yes,dialog=no,resizable",
                null);
}




function startTest() {
  var bs = PlacesUtils.bookmarks;
  
  bs.addObserver(bookmarksObserver, false);
  var addedBookmarks = [];

  
  ok(true, "*** Acting on menu bookmarks");
  var id = bs.insertBookmark(bs.bookmarksMenuFolder,
                             PlacesUtils._uri("http://bm1.mozilla.org/"),
                             bs.DEFAULT_INDEX,
                             "bm1");
  addedBookmarks.push(id);
  id = bs.insertBookmark(bs.bookmarksMenuFolder,
                         PlacesUtils._uri("place:"),
                         bs.DEFAULT_INDEX,
                         "bm2");
  addedBookmarks.push(id);
  id = bs.insertSeparator(bs.bookmarksMenuFolder, bs.DEFAULT_INDEX);
  addedBookmarks.push(id);
  id = bs.createFolder(bs.bookmarksMenuFolder,
                       "bmf",
                       bs.DEFAULT_INDEX);
  addedBookmarks.push(id);
  id = bs.insertBookmark(id,
                         PlacesUtils._uri("http://bmf1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "bmf1");
  addedBookmarks.push(id);
  bs.moveItem(id, bs.bookmarksMenuFolder, 0);

  
  ok(true, "*** Acting on toolbar bookmarks");
  bs.insertBookmark(bs.toolbarFolder,
                    PlacesUtils._uri("http://tb1.mozilla.org/"),
                    bs.DEFAULT_INDEX,
                    "tb1");
  addedBookmarks.push(id);
  id = bs.insertBookmark(bs.toolbarFolder,
                         PlacesUtils._uri("place:"),
                         bs.DEFAULT_INDEX,
                         "tb2");
  addedBookmarks.push(id);
  id = bs.insertSeparator(bs.toolbarFolder, bs.DEFAULT_INDEX);
  addedBookmarks.push(id);
  id = bs.createFolder(bs.toolbarFolder,
                       "tbf",
                       bs.DEFAULT_INDEX);
  addedBookmarks.push(id);
  id = bs.insertBookmark(id,
                         PlacesUtils._uri("http://tbf1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "bmf1");
  addedBookmarks.push(id);
  bs.moveItem(id, bs.toolbarFolder, 0);

  
  ok(true, "*** Acting on unsorted bookmarks");
  id = bs.insertBookmark(bs.unfiledBookmarksFolder,
                         PlacesUtils._uri("http://ub1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "ub1");
  addedBookmarks.push(id);
  id = bs.insertBookmark(bs.unfiledBookmarksFolder,
                         PlacesUtils._uri("place:"),
                         bs.DEFAULT_INDEX,
                         "ub2");
  addedBookmarks.push(id);
  id = bs.insertSeparator(bs.unfiledBookmarksFolder, bs.DEFAULT_INDEX);
  addedBookmarks.push(id);
  id = bs.createFolder(bs.unfiledBookmarksFolder,
                       "ubf",
                       bs.DEFAULT_INDEX);
  addedBookmarks.push(id);
  id = bs.insertBookmark(id,
                         PlacesUtils._uri("http://ubf1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "ubf1");
  addedBookmarks.push(id);
  bs.moveItem(id, bs.unfiledBookmarksFolder, 0);

  
  addedBookmarks.forEach(function (aItem) {
    
    
    try {
      bs.removeItem(aItem);
    } catch (ex) {}
  });

  
  bs.removeObserver(bookmarksObserver);
  finishTest();
}




function finishTest() {
  
  gLibrary.close();
  finish();
}





var bookmarksObserver = {
  QueryInterface: function PSB_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsINavBookmarkObserver) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_NOINTERFACE;
  },

  
  onItemAdded: function PSB_onItemAdded(aItemId, aFolderId, aIndex) {
    var node = null;
    var index = null;
    [node, index] = getNodeForTreeItem(aItemId, gLibrary.PlacesOrganizer._places);
    
    var type = PlacesUtils.bookmarks.getItemType(aItemId);
    switch (type) {
      case PlacesUtils.bookmarks.TYPE_BOOKMARK:
        var uriString = PlacesUtils.bookmarks.getBookmarkURI(aItemId).spec;
        var isQuery = uriString.substr(0, 6) == "place:";
        if (isQuery) {
          isnot(node, null, "Found new Places node in left pane");
          ok(index >= 0, "Node is at index " + index);
          break;
        }
        
      case PlacesUtils.bookmarks.TYPE_SEPARATOR:
        is(node, null, "New Places node not added in left pane");
        break;
      default:
        isnot(node, null, "Found new Places node in left pane");
        ok(index >= 0, "Node is at index " + index);
    }
  },

  onItemRemoved: function PSB_onItemRemoved(aItemId, aFolder, aIndex) {
    var node = null;
    var index = null;
    [node, index] = getNodeForTreeItem(aItemId, gLibrary.PlacesOrganizer._places);
    is(node, null, "Places node not found in left pane");
  },

  onItemMoved: function(aItemId,
                        aOldFolderId, aOldIndex,
                        aNewFolderId, aNewIndex) {
    var node = null;
    var index = null;
    [node, index] = getNodeForTreeItem(aItemId, gLibrary.PlacesOrganizer._places);
    
    var type = PlacesUtils.bookmarks.getItemType(aItemId);
    switch (type) {
      case PlacesUtils.bookmarks.TYPE_BOOKMARK:
        var uriString = PlacesUtils.bookmarks.getBookmarkURI(aItemId).spec;
        var isQuery = uriString.substr(0, 6) == "place:";
        if (isQuery) {
          isnot(node, null, "Found new Places node in left pane");
          ok(index >= 0, "Node is at index " + index);
          break;
        }
        
      case type == PlacesUtils.bookmarks.TYPE_SEPARATOR:
        is(node, null, "New Places node not added in left pane");
        break;
      default:
        isnot(node, null, "Found new Places node in left pane");
        ok(index >= 0, "Node is at index " + index);
    }
  },

  onBeginUpdateBatch: function PSB_onBeginUpdateBatch() {},
  onEndUpdateBatch: function PSB_onEndUpdateBatch() {},
  onBeforeItemRemoved: function PSB_onBeforeItemRemoved(aItemId) {},
  onItemVisited: function() {},
  onItemChanged: function PSB_onItemChanged(aItemId, aProperty,
                                            aIsAnnotationProperty, aValue) {}
};









function getNodeForTreeItem(aItemId, aTree) {

  function findNode(aContainerIndex) {
    if (aTree.view.isContainerEmpty(aContainerIndex))
      return [null, null];

    
    
    for (var i = aContainerIndex + 1; i < aTree.view.rowCount; i++) {
      var node = aTree.view.nodeForTreeIndex(i);

      if (node.itemId == aItemId) {
        
        return [node, i - aTree.view.getParentIndex(i) - 1];
      }

      if (PlacesUtils.nodeIsFolder(node)) {
        
        aTree.view.toggleOpenState(i);
        
        var foundNode = findNode(i);
        
        aTree.view.toggleOpenState(i);
        
        if (foundNode[0] != null)
          return foundNode;
      }

      
      if (!aTree.view.hasNextSibling(aContainerIndex + 1, i))
        break;
    }
    return [null, null]
  }

  
  for (var i = 0; i < aTree.view.rowCount; i++) {
    
    aTree.view.toggleOpenState(i);
    
    var foundNode = findNode(i);
    
    aTree.view.toggleOpenState(i);
    
    if (foundNode[0] != null)
      return foundNode;
  }
  return [null, null];
}
