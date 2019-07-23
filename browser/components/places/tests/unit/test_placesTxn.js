







































try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
} catch(ex) {
  do_throw("Could not get nav-bookmarks-service\n");
}


try {
  var lmsvc = Cc["@mozilla.org/browser/livemark-service;2"].getService(Ci.nsILivemarkService);
} catch(ex) {
  do_throw("Could not get livemark-service\n");
} 


try {
  var mss = Cc["@mozilla.org/microsummary/service;1"].getService(Ci.nsIMicrosummaryService);
} catch(ex) {
  do_throw("Could not get microsummary-service\n");
} 


try {
    var ptSvc =
      Cc["@mozilla.org/browser/placesTransactionsService;1"].
        getService(Ci.nsIPlacesTransactionsService);
} catch(ex) {
  do_throw("Could not get Places Transactions Service\n");
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
  }
};
bmsvc.addObserver(observer, false);


var root = bmsvc.bookmarksRoot;


var bmStartIndex = 1;


function run_test() {
  const DESCRIPTION_ANNO = "bookmarkProperties/description";
  var testDescription = "this is my test description";
  var annotationService = Cc["@mozilla.org/browser/annotation-service;1"].
                          getService(Ci.nsIAnnotationService);

  
  var annos = [{ name: DESCRIPTION_ANNO,
                 type: Ci.nsIAnnotationService.TYPE_STRING,
                flags: 0,
                value: testDescription,
              expires: Ci.nsIAnnotationService.EXPIRE_NEVER }];
  var txn1 = ptSvc.createFolder("Testing folder", root, bmStartIndex, annos);
  txn1.doTransaction();
  var folderId = bmsvc.getChildFolder(root, "Testing folder");
  do_check_eq(testDescription, 
              annotationService.getItemAnnotation(folderId, DESCRIPTION_ANNO));
  do_check_eq(observer._itemAddedIndex, bmStartIndex);
  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedId, folderId);
  txn1.undoTransaction();
  do_check_eq(observer._itemRemovedId, folderId);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, bmStartIndex);

  
  
  var txn2 = ptSvc.createItem(uri("http://www.example.com"), root, bmStartIndex, "Testing1");
  ptSvc.commitTransaction(txn2); 
  var b = (bmsvc.getBookmarkIdsForURI(uri("http://www.example.com"), {}))[0];
  do_check_eq(observer._itemAddedId, b);
  do_check_eq(observer._itemAddedIndex, bmStartIndex);
  do_check_true(bmsvc.isBookmarked(uri("http://www.example.com")));
  txn2.undoTransaction();
  do_check_eq(observer._itemRemovedId, b);
  do_check_eq(observer._itemRemovedIndex, bmStartIndex);

  
  var txn2a = ptSvc.createFolder("Folder", root, bmStartIndex);
  var fldrId = bmsvc.getChildFolder(root, "Folder");
  var txn2b = ptSvc.createItem(uri("http://www.example2.com"), fldrId, bmStartIndex, "Testing1b");
  ptSvc.commitTransaction(txn2b);
  var b2 = (bmsvc.getBookmarkIdsForURI(uri("http://www.example2.com"), {}))[0];
  do_check_eq(observer._itemAddedId, b2);
  do_check_eq(observer._itemAddedIndex, bmStartIndex);
  do_check_true(bmsvc.isBookmarked(uri("http://www.example2.com")));
  txn2.undoTransaction();
  do_check_eq(observer._itemRemovedId, b2);
  do_check_eq(observer._itemRemovedIndex, bmStartIndex);

  
  ptSvc.commitTransaction(ptSvc.createItem(uri("http://www.example3.com"), root, -1, "Testing2"));
  ptSvc.commitTransaction(ptSvc.createItem(uri("http://www.example3.com"), root, -1, "Testing3"));   
  ptSvc.commitTransaction(ptSvc.createItem(uri("http://www.example3.com"), fldrId, -1, "Testing4"));
  var bkmkIds = bmsvc.getBookmarkIdsForURI(uri("http://www.example3.com"), {});
  bkmkIds.sort();
  var bkmk1Id = bkmkIds[0];
  var bkmk2Id = bkmkIds[1];
  var bkmk3Id = bkmkIds[2];
  var txn3 = ptSvc.moveItem(bkmk1Id, root, -1);
  txn3.doTransaction();

  
  do_check_eq(observer._itemMovedId, bkmk1Id);
  do_check_eq(observer._itemMovedOldParent, root);
  do_check_eq(observer._itemMovedOldIndex, 1);
  do_check_eq(observer._itemMovedNewParent, root);
  do_check_eq(observer._itemMovedNewIndex, 2);
  txn3.undoTransaction();
  do_check_eq(observer._itemMovedId, bkmk1Id);
  do_check_eq(observer._itemMovedOldParent, root);
  do_check_eq(observer._itemMovedOldIndex, 2);
  do_check_eq(observer._itemMovedNewParent, root);
  do_check_eq(observer._itemMovedNewIndex, 1);

  
  var txn3b = ptSvc.moveItem(bkmk1Id, fldrId, -1);
  txn3b.doTransaction();
  do_check_eq(observer._itemMovedId, bkmk1Id);
  do_check_eq(observer._itemMovedOldParent, root);
  do_check_eq(observer._itemMovedOldIndex, 1);
  do_check_eq(observer._itemMovedNewParent, fldrId);
  do_check_eq(observer._itemMovedNewIndex, 2);
  txn3.undoTransaction();
  do_check_eq(observer._itemMovedId, bkmk1Id);
  do_check_eq(observer._itemMovedOldParent, fldrId);
  do_check_eq(observer._itemMovedOldIndex, 2);
  do_check_eq(observer._itemMovedNewParent, root);
  do_check_eq(observer._itemMovedNewIndex, 1);

  
  ptSvc.commitTransaction(ptSvc.createFolder("Folder2", root, -1));
  var fldrId2 = bmsvc.getChildFolder(root, "Folder2");
  var txn4 = ptSvc.removeItem(fldrId2);
  txn4.doTransaction();
  do_check_eq(observer._itemRemovedId, fldrId2);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, 3);
  txn4.undoTransaction();
  do_check_eq(observer._itemAddedId, fldrId2);
  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedIndex, 3);

  
  var txn5 = ptSvc.removeItem(bkmk2Id);
  txn5.doTransaction();
  do_check_eq(observer._itemRemovedId, bkmk2Id);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, 1);
  txn5.undoTransaction();

  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedIndex, 1);

  
  var txn6 = ptSvc.createSeparator(root, 1);
  txn6.doTransaction();
  var sepId = observer._itemAddedId;
  do_check_eq(observer._itemAddedIndex, 1);
  do_check_eq(observer._itemAddedParent, root);
  txn6.undoTransaction();
  do_check_eq(observer._itemRemovedId, sepId);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, 1);

  
  ptSvc.commitTransaction(ptSvc.createSeparator(root, 1));
  var sepId2 = observer._itemAddedId;
  var txn7 = ptSvc.removeItem(sepId2);
  txn7.doTransaction();
  do_check_eq(observer._itemRemovedId, sepId2);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, 1);
  txn7.undoTransaction();
  do_check_eq(observer._itemAddedId, sepId2); 
  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedIndex, 1);

  
  var txn8 = ptSvc.editItemTitle(bkmk1Id, "Testing2_mod");
  txn8.doTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id); 
  do_check_eq(observer._itemChangedProperty, "title");
  do_check_eq(observer._itemChangedValue, "Testing2_mod");
  txn8.undoTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id); 
  do_check_eq(observer._itemChangedProperty, "title");
  do_check_eq(observer._itemChangedValue, "Testing2");

  
  var txn9 = ptSvc.editBookmarkURI(bkmk1Id, uri("http://newuri.com"));
  txn9.doTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, "uri");
  do_check_eq(observer._itemChangedValue, "http://newuri.com/");
  txn9.undoTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, "uri");
  do_check_eq(observer._itemChangedValue, "http://www.example3.com/");
  
  
  var txn10 = ptSvc.editItemDescription(bkmk1Id, "Description1");
  txn10.doTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, "bookmarkProperties/description");

  
  var txn11 = ptSvc.editBookmarkKeyword(bkmk1Id, "kw1");
  txn11.doTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, "keyword");
  do_check_eq(observer._itemChangedValue, "kw1"); 
  txn11.undoTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, "keyword");
  do_check_eq(observer._itemChangedValue, ""); 

  var txn12 = ptSvc.createLivemark(uri("http://feeduri.com"), uri("http://siteuri.com"), "Livemark1", root);
  txn12.doTransaction();

  
  
  
  do_check_true(lmsvc.isLivemark(observer._itemAddedId-1));
  do_check_eq(lmsvc.getSiteURI(observer._itemAddedId-1).spec, "http://siteuri.com/");
  do_check_eq(lmsvc.getFeedURI(observer._itemAddedId-1).spec, "http://feeduri.com/");
  var lvmkId = observer._itemAddedId-1;

  
  var txn13 = ptSvc.editLivemarkSiteURI(lvmkId, uri("http://NEWsiteuri.com/"));
  txn13.doTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/siteURI");

  txn13.undoTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/siteURI");
  do_check_eq(observer._itemChangedValue, "");

  
  var txn14 = ptSvc.editLivemarkFeedURI(lvmkId, uri("http://NEWfeeduri.com/"));
  txn14.doTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/feedURI");

  txn14.undoTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/feedURI");
  do_check_eq(observer._itemChangedValue, "");

  
  ptSvc.commitTransaction(ptSvc.createFolder("Testing toolbar folder", root, bmStartIndex));
  var tmpFolderId = bmsvc.getChildFolder(root, "Testing toolbar folder");
  var txn15 = ptSvc.setBookmarksToolbar(tmpFolderId);
  txn15.doTransaction();
  do_check_eq(observer._itemChangedId, tmpFolderId);
  do_check_eq(observer._itemChangedProperty, "became_toolbar_folder");
  txn15.undoTransaction();

  
  var txn16 = ptSvc.setLoadInSidebar(bkmk1Id, true);
  txn16.doTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, "bookmarkProperties/loadInSidebar");
  do_check_eq(observer._itemChanged_isAnnotationProperty, true);
  txn16.undoTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, "bookmarkProperties/loadInSidebar");
  do_check_eq(observer._itemChanged_isAnnotationProperty, true);

  
  ptSvc.commitTransaction(ptSvc.createFolder("Sorting folder", root, bmStartIndex, [], null));
  var srtFldId = bmsvc.getChildFolder(root, "Sorting folder");
  ptSvc.commitTransaction(ptSvc.createItem(uri("http://www.sortingtest.com"), srtFldId, -1, "c"));
  ptSvc.commitTransaction(ptSvc.createItem(uri("http://www.sortingtest.com"), srtFldId, -1, "b"));   
  ptSvc.commitTransaction(ptSvc.createItem(uri("http://www.sortingtest.com"), srtFldId, -1, "a"));
  var b = bmsvc.getBookmarkIdsForURI(uri("http://www.sortingtest.com"), {});
  b.sort();
  var b1 = b[0];
  var b2 = b[1];
  var b3 = b[2];
  do_check_eq(0, bmsvc.getItemIndex(b1));
  do_check_eq(1, bmsvc.getItemIndex(b2));
  do_check_eq(2, bmsvc.getItemIndex(b3));
  var txn17 = ptSvc.sortFolderByName(srtFldId, 1);
  txn17.doTransaction();
  do_check_eq(2, bmsvc.getItemIndex(b1));
  do_check_eq(1, bmsvc.getItemIndex(b2));
  do_check_eq(0, bmsvc.getItemIndex(b3));
  txn17.undoTransaction();
  do_check_eq(0, bmsvc.getItemIndex(b1));
  do_check_eq(1, bmsvc.getItemIndex(b2));
  do_check_eq(2, bmsvc.getItemIndex(b3));

  
  var tmpMs = mss.createMicrosummary(uri("http://testmicro.com"), 
                                     uri("http://dietrich.ganx4.com/mozilla/test-microsummary.xml"));
  ptSvc.commitTransaction(
  ptSvc.createItem(uri("http://dietrich.ganx4.com/mozilla/test-microsummary-content.php"),
                   root, -1, "micro test", null, null, null));
  var bId = (bmsvc.getBookmarkIdsForURI(uri("http://dietrich.ganx4.com/mozilla/test-microsummary-content.php"),{}))[0];
  do_check_true(!mss.hasMicrosummary(bId));
  var txn18 = ptSvc.editBookmarkMicrosummary(bId, tmpMs);
  txn18.doTransaction();
  do_check_eq(observer._itemChangedId, bId);
  do_check_true(mss.hasMicrosummary(bId));
  txn18.undoTransaction();
  do_check_eq(observer._itemChangedId, bId);
  do_check_true(!mss.hasMicrosummary(bId));

  
  
}
