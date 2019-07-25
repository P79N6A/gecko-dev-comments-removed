








































let toolbar = document.getElementById("PersonalToolbar");
let wasCollapsed = toolbar.collapsed;

function test() {
  
  if (wasCollapsed)
    setToolbarVisibility(toolbar, true);

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
  PlacesUtils.annotations.addObserver(bookmarksObserver, false);
  var addedBookmarks = [];

  
  info("*** Acting on menu bookmarks");
  var id = bs.insertBookmark(bs.bookmarksMenuFolder,
                             PlacesUtils._uri("http://bm1.mozilla.org/"),
                             bs.DEFAULT_INDEX,
                             "bm1");
  bs.setItemTitle(id, "bm1_edited");
  addedBookmarks.push(id);
  id = bs.insertBookmark(bs.bookmarksMenuFolder,
                         PlacesUtils._uri("place:"),
                         bs.DEFAULT_INDEX,
                         "bm2");
  bs.setItemTitle(id, "");
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
  bs.setItemTitle(id, "bmf1_edited");
  addedBookmarks.push(id);
  bs.moveItem(id, bs.bookmarksMenuFolder, 0);
  id = PlacesUtils.livemarks.createLivemarkFolderOnly(
    bs.bookmarksMenuFolder, "bml",
    PlacesUtils._uri("http://bml.siteuri.mozilla.org/"),
    PlacesUtils._uri("http://bml.feeduri.mozilla.org/"), bs.DEFAULT_INDEX);
  addedBookmarks.push(id);

  
  info("*** Acting on toolbar bookmarks");
  id = bs.insertBookmark(bs.toolbarFolder,
                         PlacesUtils._uri("http://tb1.mozilla.org/"),
                         bs.DEFAULT_INDEX,
                         "tb1");
  bs.setItemTitle(id, "tb1_edited");
  addedBookmarks.push(id);
  
  bs.setItemTitle(id, "tb1_edited");
  id = bs.insertBookmark(bs.toolbarFolder,
                         PlacesUtils._uri("place:"),
                         bs.DEFAULT_INDEX,
                         "tb2");
  bs.setItemTitle(id, "");
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
                         "tbf1");
  bs.setItemTitle(id, "tbf1_edited");
  addedBookmarks.push(id);
  bs.moveItem(id, bs.toolbarFolder, 0);
  id = PlacesUtils.livemarks.createLivemarkFolderOnly(
    bs.toolbarFolder, "tbl", PlacesUtils._uri("http://tbl.siteuri.mozilla.org/"),
    PlacesUtils._uri("http://tbl.feeduri.mozilla.org/"), bs.DEFAULT_INDEX);
  addedBookmarks.push(id);

  
  info("*** Acting on unsorted bookmarks");
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
                         "bubf1");
  bs.setItemTitle(id, "bubf1_edited");
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
  
  toggleSidebar("viewBookmarksSidebar", false);

  
  if (wasCollapsed)
    setToolbarVisibility(toolbar, false);

  finish();
}





var bookmarksObserver = {
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavBookmarkObserver
  , Ci.nsIAnnotationObserver
  ]),

  
  onItemAnnotationSet: function(aItemId, aAnnotationName) {
    if (aAnnotationName == PlacesUtils.LMANNO_FEEDURI) {
      var views = getViewsForFolder(PlacesUtils.bookmarks.getFolderIdForItem(aItemId));
      ok(views.length > 0, "Found affected views (" + views.length + "): " + views);

      
      let validator = function(aElementOrTreeIndex) {
        if (typeof(aElementOrTreeIndex) == "number") {
          var sidebar = document.getElementById("sidebar");
          var tree = sidebar.contentDocument.getElementById("bookmarks-view");
          let livemarkAtom = Cc["@mozilla.org/atom-service;1"].
                             getService(Ci.nsIAtomService).
                             getAtom("livemark");
          let properties = Cc["@mozilla.org/supports-array;1"].
                           createInstance(Ci.nsISupportsArray);
          tree.view.getCellProperties(aElementOrTreeIndex,
                                      tree.columns.getColumnAt(0),
                                      properties);
          return properties.GetIndexOf(livemarkAtom) != -1;
        }
        else {
          return aElementOrTreeIndex.hasAttribute("livemark");
        }
      };

      for (var i = 0; i < views.length; i++) {
        var [node, index, valid] = searchItemInView(aItemId, views[i], validator);
        isnot(node, null, "Found new Places node in " + views[i] + " at " + index);
        ok(valid, "Node is recognized as a livemark");
      }
    }
  },
  onItemAnnotationRemoved: function() {},
  onPageAnnotationSet: function() {},
  onPageAnnotationRemoved: function() {},

  
  onItemAdded: function PSB_onItemAdded(aItemId, aFolderId, aIndex,
                                        aItemType, aURI) {
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

  onItemChanged: function PSB_onItemChanged(aItemId, aProperty,
                                            aIsAnnotationProperty, aNewValue,
                                            aLastModified, aItemType,
                                            aParentId) {
    if (aProperty !== "title")
      return;

    var views = getViewsForFolder(aParentId);
    ok(views.length > 0, "Found affected views (" + views.length + "): " + views);

    
    let validator = function(aElementOrTreeIndex) {
      if (typeof(aElementOrTreeIndex) == "number") {
        var sidebar = document.getElementById("sidebar");
        var tree = sidebar.contentDocument.getElementById("bookmarks-view");
        let cellText = tree.view.getCellText(aElementOrTreeIndex,
                                             tree.columns.getColumnAt(0));
        if (!aNewValue)
          return cellText == PlacesUIUtils.getBestTitle(tree.view.nodeForTreeIndex(aElementOrTreeIndex));
        return cellText == aNewValue;
      }
      else {
        if (!aNewValue && aElementOrTreeIndex.localName != "toolbarbutton")
          return aElementOrTreeIndex.getAttribute("label") == PlacesUIUtils.getBestTitle(aElementOrTreeIndex._placesNode);
        return aElementOrTreeIndex.getAttribute("label") == aNewValue;
      }
    };

    for (var i = 0; i < views.length; i++) {
      var [node, index, valid] = searchItemInView(aItemId, views[i], validator);
      isnot(node, null, "Found changed Places node in " + views[i]);
      is(node.title, aNewValue, "Node has correct title: " + aNewValue);
      ok(valid, "Node element has correct label: " + aNewValue);
    }
  }
};












function searchItemInView(aItemId, aView, aValidator) {
  switch (aView) {
  case "toolbar":
    return getNodeForToolbarItem(aItemId, aValidator);
  case "menu":
    return getNodeForMenuItem(aItemId, aValidator);
  case "sidebar":
    return getNodeForSidebarItem(aItemId, aValidator);
  }

  return [null, null, false];
}








function getNodeForToolbarItem(aItemId, aValidator) {
  var toolbar = document.getElementById("PlacesToolbarItems");

  function findNode(aContainer) {
    var children = aContainer.childNodes;
    for (var i = 0, staticNodes = 0; i < children.length; i++) {
      var child = children[i];

      
      if (!child._placesNode) {
        staticNodes++;
        continue;
      }

      if (child._placesNode.itemId == aItemId) {
        let valid = aValidator ? aValidator(child) : true;
        return [child._placesNode, i - staticNodes, valid];
      }

      
      
      if (PlacesUtils.nodeIsFolder(child._placesNode)) {
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








function getNodeForMenuItem(aItemId, aValidator) {
  var menu = document.getElementById("bookmarksMenu");

  function findNode(aContainer) {
    var children = aContainer.childNodes;
    for (var i = 0, staticNodes = 0; i < children.length; i++) {
      var child = children[i];

      
      if (!child._placesNode) {
        staticNodes++;
        continue;
      }

      if (child._placesNode.itemId == aItemId) {
        let valid = aValidator ? aValidator(child) : true;
        return [child._placesNode, i - staticNodes, valid];
      }

      
      
      if (PlacesUtils.nodeIsFolder(child._placesNode)) {
        var popup = child.lastChild;
        fakeOpenPopup(popup);
        var foundNode = findNode(popup);

        child.open = false;
        if (foundNode[0] != null)
          return foundNode;
      }
    }
    return [null, null, false];
  }

  return findNode(menu.lastChild);
}








function getNodeForSidebarItem(aItemId, aValidator) {
  var sidebar = document.getElementById("sidebar");
  var tree = sidebar.contentDocument.getElementById("bookmarks-view");

  function findNode(aContainerIndex) {
    if (tree.view.isContainerEmpty(aContainerIndex))
      return [null, null, false];

    
    
    for (var i = aContainerIndex + 1; i < tree.view.rowCount; i++) {
      var node = tree.view.nodeForTreeIndex(i);

      if (node.itemId == aItemId) {
        
        let valid = aValidator ? aValidator(i) : true;
        return [node, i - tree.view.getParentIndex(i) - 1, valid];
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
    return [null, null, false]
  }

  
  for (var i = 0; i < tree.view.rowCount; i++) {
    
    tree.view.toggleOpenState(i);
    
    var foundNode = findNode(i);
    
    tree.view.toggleOpenState(i);
    
    if (foundNode[0] != null)
      return foundNode;
  }
  return [null, null, false];
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
