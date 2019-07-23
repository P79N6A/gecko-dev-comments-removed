








































const Cc = Components.classes;
const Ci = Components.interfaces;

function test() {
  waitForExplicitFinish();

  
  ok(PlacesUtils, "PlacesUtils in context");
  ok(PlacesUIUtils, "PlacesUIUtils in context");

  
  var popup = document.getElementById("bookmarksMenuPopup");
  ok(popup, "Menu popup element exists");
  fakeOpenPopup(popup);

  
  var sidebar = document.getElementById("sidebar");
  sidebar.addEventListener("load", function() {
    sidebar.removeEventListener("load", arguments.callee, true);
    
    executeSoon(startTest);
  }, true);
  toggleSidebar("viewBookmarksSidebar", true);
}





function fakeOpenPopup(aPopup) {
  var popupEvent = document.createEvent("MouseEvent");
  popupEvent.initMouseEvent("popupshowing", true, true, window, 0,
                            0, 0, 0, 0, false, false, false, false,
                            0, null);
  aPopup.dispatchEvent(popupEvent);  
}




function startTest() {
  var bs = PlacesUtils.bookmarks;
  
  bs.addObserver(bookmarksObserver, false);
  var addedBookmarks = [];

  
  info("*** Acting on menu bookmarks");
  var id = bs.insertBookmark(bs.bookmarksMenuFolder,
                             PlacesUtils._uri("http://bm1.mozilla.org/"),
                             bs.DEFAULT_INDEX,
                             "bm1");
  addedBookmarks.push(id);
  
  bs.setItemTitle(id, "bm1_edited");
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

  
  info("*** Acting on toolbar bookmarks");
  id = bs.insertBookmark(bs.toolbarFolder,
                         PlacesUtils._uri("http://tb1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "tb1");
  addedBookmarks.push(id);
  
  bs.setItemTitle(id, "tb1_edited");
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

  
  info("*** Acting on unsorted bookmarks");
  id = bs.insertBookmark(bs.unfiledBookmarksFolder,
                         PlacesUtils._uri("http://ub1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "ub1");
  addedBookmarks.push(id);
  
  bs.setItemTitle(id, "ub1_edited");
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
                         "bubf1");
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
  
  toggleSidebar("viewBookmarksSidebar", false);

  finish();
}





var bookmarksObserver = {
  QueryInterface: function PSB_QueryInterface(aIID) {
    if (aIID.equals(Ci.nsINavBookmarkObserver) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_NOINTERFACE;
  },

  
  onItemAdded: function PSB_onItemAdded(aItemId, aFolderId, aIndex,
                                        aItemType) {
    var views = getViewsForFolder(aFolderId);
    ok(views.length > 0, "Found affected views (" + views.length + "): " + views);

    
    for (var i = 0; i < views.length; i++) {
      var [node, index] = searchItemInView(aItemId, views[i]);
      isnot(node, null, "Found new Places node in " + views[i]);
      is(index, aIndex, "Node is at index " + index);
    }
  },

  onItemRemoved: function PSB_onItemRemoved(aItemId, aFolder, aIndex,
                                            aItemType) {
    var views = getViewsForFolder(aFolderId);
    ok(views.length > 0, "Found affected views (" + views.length + "): " + views);
    
    for (var i = 0; i < views.length; i++) {
      var node = null;
      var index = null;
      [node, index] = searchItemInView(aItemId, views[i]);
      is(node, null, "Places node not found in " + views[i]);
    }
  },

  onItemMoved: function(aItemId,
                        aOldFolderId, aOldIndex,
                        aNewFolderId, aNewIndex,
                        aItemType) {
    var views = getViewsForFolder(aNewFolderId);
    ok(views.length > 0, "Found affected views: " + views);

    
    for (var i = 0; i < views.length; i++) {
      var node = null;
      var index = null;
      [node, index] = searchItemInView(aItemId, views[i]);
      isnot(node, null, "Found new Places node in " + views[i]);
      is(index, aNewIndex, "Node is at index " + index);
    }
  },

  onBeginUpdateBatch: function PSB_onBeginUpdateBatch() {},
  onEndUpdateBatch: function PSB_onEndUpdateBatch() {},
  onBeforeItemRemoved: function PSB_onBeforeItemRemoved(aItemId) {},
  onItemVisited: function() {},

  onItemChanged: function PSB_onItemChanged(aItemId, aProperty, aIsAnnotationProperty, aValue) {
    if (aProperty !== "title")
      return;

    var views = getViewsForFolder(PlacesUtils.bookmarks.getFolderIdForItem(aItemId));
    ok(views.length > 0, "Found affected views (" + views.length + "): " + views);

    
    for (var i = 0; i < views.length; i++) {
      var [node, index] = searchItemInView(aItemId, views[i]);
      isnot(node, null, "Found new Places node in " + views[i]);
      is(node.title, aValue, "Node has correct title: " + aValue);
    }
  }
};










function searchItemInView(aItemId, aView) {
  switch (aView) {
  case "toolbar":
    return getNodeForToolbarItem(aItemId);
  case "menu":
    return getNodeForMenuItem(aItemId);
  case "sidebar":
    return getNodeForSidebarItem(aItemId);
  }

  return [null, null];
}








function getNodeForToolbarItem(aItemId) {
  var toolbar = document.getElementById("bookmarksBarContent");

  function findNode(aContainer) {
    var children = aContainer.childNodes;
    for (var i = 0, staticNodes = 0; i < children.length; i++) {
      var child = children[i];

      
      if (!child.node) {
        staticNodes++;
        continue;
      }

      if (child.node.itemId == aItemId)
        return [child.node, i - staticNodes];

      
      
      if (PlacesUtils.nodeIsFolder(child.node)) {
        var popup = child.lastChild;
        popup.showPopup(popup);
        var foundNode = findNode(popup);
        popup.hidePopup();
        if (foundNode[0] != null)
          return foundNode;
      }
    }
    return [null, null];
  }

  return findNode(toolbar);
}








function getNodeForMenuItem(aItemId) {
  var menu = document.getElementById("bookmarksMenu");

  function findNode(aContainer) {
    var children = aContainer.childNodes;
    for (var i = 0, staticNodes = 0; i < children.length; i++) {
      var child = children[i];

      
      if (!child.node) {
        staticNodes++;
        continue;
      }

      if (child.node.itemId == aItemId)
        return [child.node, i - staticNodes];

      
      
      if (PlacesUtils.nodeIsFolder(child.node)) {
        var popup = child.lastChild;
        fakeOpenPopup(popup);
        var foundNode = findNode(popup);

        child.open = false;
        if (foundNode[0] != null)
          return foundNode;
      }
    }
    return [null, null];
  }

  return findNode(menu.lastChild);
}








function getNodeForSidebarItem(aItemId) {
  var sidebar = document.getElementById("sidebar");
  var tree = sidebar.contentDocument.getElementById("bookmarks-view");

  function findNode(aContainerIndex) {
    if (tree.view.isContainerEmpty(aContainerIndex))
      return [null, null];

    
    
    for (var i = aContainerIndex + 1; i < tree.view.rowCount; i++) {
      var node = tree.view.nodeForTreeIndex(i);

      if (node.itemId == aItemId) {
        
        return [node, i - tree.view.getParentIndex(i) - 1];
      }

      if (PlacesUtils.nodeIsFolder(node)) {
        
        tree.view.toggleOpenState(i);
        
        var foundNode = findNode(i);
        
        tree.view.toggleOpenState(i);
        
        if (foundNode[0] != null)
          return foundNode;
      }

      
      if (!tree.view.hasNextSibling(aContainerIndex + 1, i))
        break;
    }
    return [null, null]
  }

  
  for (var i = 0; i < tree.view.rowCount; i++) {
    
    tree.view.toggleOpenState(i);
    
    var foundNode = findNode(i);
    
    tree.view.toggleOpenState(i);
    
    if (foundNode[0] != null)
      return foundNode;
  }
  return [null, null];
}








function getViewsForFolder(aFolderId) {
  var rootId = aFolderId;
  while (!PlacesUtils.isRootItem(rootId))
    rootId = PlacesUtils.bookmarks.getFolderIdForItem(rootId);

  switch (rootId) {
    case PlacesUtils.toolbarFolderId:
      return ["toolbar", "sidebar"]
      break;
    case PlacesUtils.bookmarksMenuFolderId:
      return ["menu", "sidebar"]
      break;
    case PlacesUtils.unfiledBookmarksFolderId:
      return ["sidebar"]
      break;    
  }
  return new Array();
}
