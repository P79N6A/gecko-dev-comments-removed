








































var gLibrary = null;

function test() {
  waitForExplicitFinish();
  
  
  
  requestLongerTimeout(2);

  
  ok(PlacesUtils, "PlacesUtils in context");
  ok(PlacesUIUtils, "PlacesUIUtils in context");

  
  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  function windowObserver(aSubject, aTopic, aData) {
    if (aTopic != "domwindowopened")
      return;
    ww.unregisterNotification(windowObserver);
    gLibrary = aSubject.QueryInterface(Ci.nsIDOMWindow);
    gLibrary.addEventListener("load", function onLoad(event) {
      gLibrary.removeEventListener("load", onLoad, false);
      executeSoon(startTest);
    }, false);
  }
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
  PlacesUtils.annotations.addObserver(bookmarksObserver, false);
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
  bs.setItemTitle(id, "bm2_edited");
  addedBookmarks.push(id);
  id = bs.insertSeparator(bs.bookmarksMenuFolder, bs.DEFAULT_INDEX);
  addedBookmarks.push(id);
  id = bs.createFolder(bs.bookmarksMenuFolder,
                       "bmf",
                       bs.DEFAULT_INDEX);
  bs.setItemTitle(id, "bmf_edited");
  addedBookmarks.push(id);
  id = bs.insertBookmark(id,
                         PlacesUtils._uri("http://bmf1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "bmf1");
  addedBookmarks.push(id);
  bs.moveItem(id, bs.bookmarksMenuFolder, 0);
  id = PlacesUtils.livemarks.createLivemarkFolderOnly(
    bs.bookmarksMenuFolder, "bml",
    PlacesUtils._uri("http://bml.siteuri.mozilla.org/"),
    PlacesUtils._uri("http://bml.feeduri.mozilla.org/"), bs.DEFAULT_INDEX);
  addedBookmarks.push(id);

  
  ok(true, "*** Acting on toolbar bookmarks");
  bs.insertBookmark(bs.toolbarFolder,
                    PlacesUtils._uri("http://tb1.mozilla.org/"),
                    bs.DEFAULT_INDEX,
                    "tb1");
  bs.setItemTitle(id, "tb1_edited");
  addedBookmarks.push(id);
  id = bs.insertBookmark(bs.toolbarFolder,
                         PlacesUtils._uri("place:"),
                         bs.DEFAULT_INDEX,
                         "tb2");
  bs.setItemTitle(id, "tb2_edited");
  addedBookmarks.push(id);
  id = bs.insertSeparator(bs.toolbarFolder, bs.DEFAULT_INDEX);
  addedBookmarks.push(id);
  id = bs.createFolder(bs.toolbarFolder,
                       "tbf",
                       bs.DEFAULT_INDEX);
  bs.setItemTitle(id, "tbf_edited");
  addedBookmarks.push(id);
  id = bs.insertBookmark(id,
                         PlacesUtils._uri("http://tbf1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "bmf1");
  addedBookmarks.push(id);
  bs.moveItem(id, bs.toolbarFolder, 0);
  id = PlacesUtils.livemarks.createLivemarkFolderOnly(
    bs.toolbarFolder, "tbl", PlacesUtils._uri("http://tbl.siteuri.mozilla.org/"),
    PlacesUtils._uri("http://tbl.feeduri.mozilla.org/"), bs.DEFAULT_INDEX);
  addedBookmarks.push(id);

  
  ok(true, "*** Acting on unsorted bookmarks");
  id = bs.insertBookmark(bs.unfiledBookmarksFolder,
                         PlacesUtils._uri("http://ub1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "ub1");
  bs.setItemTitle(id, "ub1_edited");
  addedBookmarks.push(id);
  id = bs.insertBookmark(bs.unfiledBookmarksFolder,
                         PlacesUtils._uri("place:"),
                         bs.DEFAULT_INDEX,
                         "ub2");
  bs.setItemTitle(id, "ub2_edited");
  addedBookmarks.push(id);
  id = bs.insertSeparator(bs.unfiledBookmarksFolder, bs.DEFAULT_INDEX);
  addedBookmarks.push(id);
  id = bs.createFolder(bs.unfiledBookmarksFolder,
                       "ubf",
                       bs.DEFAULT_INDEX);
  bs.setItemTitle(id, "ubf_edited");
  addedBookmarks.push(id);
  id = bs.insertBookmark(id,
                         PlacesUtils._uri("http://ubf1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "ubf1");
  addedBookmarks.push(id);
  bs.moveItem(id, bs.unfiledBookmarksFolder, 0);
  id = PlacesUtils.livemarks.createLivemarkFolderOnly(
    bs.unfiledBookmarksFolder, "bubl",
    PlacesUtils._uri("http://bubl.siteuri.mozilla.org/"),
    PlacesUtils._uri("http://bubl.feeduri.mozilla.org/"), bs.DEFAULT_INDEX);
  addedBookmarks.push(id);

  
  addedBookmarks.forEach(function (aItem) {
    
    
    try {
      bs.removeItem(aItem);
    } catch (ex) {}
  });

  
  bs.removeObserver(bookmarksObserver);
  PlacesUtils.annotations.removeObserver(bookmarksObserver);
  finishTest();
}




function finishTest() {
  
  gLibrary.close();
  finish();
}





var bookmarksObserver = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavBookmarkObserver
  , Ci.nsIAnnotationObserver
  ]),

  
  onItemAnnotationSet: function(aItemId, aAnnotationName) {
    if (aAnnotationName == PlacesUtils.LMANNO_FEEDURI) {
      
      let validator = function(aTreeRowIndex) {
        let tree = gLibrary.PlacesOrganizer._places;
        let livemarkAtom = Cc["@mozilla.org/atom-service;1"].
                           getService(Ci.nsIAtomService).
                           getAtom("livemark");
        let properties = Cc["@mozilla.org/supports-array;1"].
                         createInstance(Ci.nsISupportsArray);
        tree.view.getCellProperties(aTreeRowIndex,
                                    tree.columns.getColumnAt(0),
                                    properties);
        return properties.GetIndexOf(livemarkAtom) != -1;
      };

      var [node, index, valid] = getNodeForTreeItem(aItemId, gLibrary.PlacesOrganizer._places, validator);
      isnot(node, null, "Found new Places node in left pane at " + index);
      ok(valid, "Node is recognized as a livemark");
    }
  },
  onItemAnnotationRemoved: function() {},
  onPageAnnotationSet: function() {},
  onPageAnnotationRemoved: function() {},

  
  onItemAdded: function PSB_onItemAdded(aItemId, aFolderId, aIndex, aItemType,
                                        aURI) {
    var node = null;
    var index = null;
    [node, index] = getNodeForTreeItem(aItemId, gLibrary.PlacesOrganizer._places);
    
    switch (aItemType) {
      case PlacesUtils.bookmarks.TYPE_BOOKMARK:
        var uriString = aURI.spec;
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
                        aNewFolderId, aNewIndex, aItemType) {
    var node = null;
    var index = null;
    [node, index] = getNodeForTreeItem(aItemId, gLibrary.PlacesOrganizer._places);
    
    switch (aItemType) {
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

  onBeginUpdateBatch: function PSB_onBeginUpdateBatch() {},
  onEndUpdateBatch: function PSB_onEndUpdateBatch() {},
  onBeforeItemRemoved: function PSB_onBeforeItemRemoved(aItemId) {},
  onItemVisited: function() {},
  onItemChanged: function PSB_onItemChanged(aItemId, aProperty,
                                            aIsAnnotationProperty, aNewValue) {
    if (aProperty == "title") {
      let validator = function(aTreeRowIndex) {
        let tree = gLibrary.PlacesOrganizer._places;
        let cellText = tree.view.getCellText(aTreeRowIndex,
                                             tree.columns.getColumnAt(0));
        return cellText == aNewValue;
      }
      let [node, index, valid] = getNodeForTreeItem(aItemId, gLibrary.PlacesOrganizer._places, validator);
      if (node) 
        ok(valid, "Title cell value has been correctly updated");
    }
  }
};













function getNodeForTreeItem(aItemId, aTree, aValidator) {

  function findNode(aContainerIndex) {
    if (aTree.view.isContainerEmpty(aContainerIndex))
      return [null, null, false];

    
    
    for (var i = aContainerIndex + 1; i < aTree.view.rowCount; i++) {
      var node = aTree.view.nodeForTreeIndex(i);

      if (node.itemId == aItemId) {
        
        let valid = aValidator ? aValidator(i) : true;
        return [node, i - aTree.view.getParentIndex(i) - 1, valid];
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
    return [null, null, false]
  }

  
  for (var i = 0; i < aTree.view.rowCount; i++) {
    
    aTree.view.toggleOpenState(i);
    
    var foundNode = findNode(i);
    
    aTree.view.toggleOpenState(i);
    
    if (foundNode[0] != null)
      return foundNode;
  }
  return [null, null, false];
}
