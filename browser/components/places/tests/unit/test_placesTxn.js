







































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
  var ptSvc = Cc["@mozilla.org/browser/placesTransactionsService;1"].
              getService(Ci.nsIPlacesTransactionsService);
} catch(ex) {
  do_throw("Could not get Places Transactions Service\n");
}


try {
  var tagssvc = Cc["@mozilla.org/browser/tagging-service;1"].
                getService(Ci.nsITaggingService);
} catch(ex) {
  do_throw("Could not get tagging service\n");
}


try {
  var annosvc = Cc["@mozilla.org/browser/annotation-service;1"].
                getService(Ci.nsIAnnotationService);
} catch(ex) {
  do_throw("Could not get annotations service\n");
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
  onBeforeItemRemoved: function(id) {
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


var bmStartIndex = 0;


function run_test() {
  
  var root = bmsvc.bookmarksMenuFolder;

  
  const DESCRIPTION_ANNO = "bookmarkProperties/description";
  const TEST_DESCRIPTION = "this is my test description";
  var annos = [{ name: DESCRIPTION_ANNO,
                 type: annosvc.TYPE_STRING,
                flags: 0,
                value: TEST_DESCRIPTION,
              expires: annosvc.EXPIRE_NEVER }];
  var txn1 = ptSvc.createFolder("Testing folder", root, bmStartIndex, annos);
  txn1.doTransaction();
  var folderId = bmsvc.getChildFolder(root, "Testing folder");
  do_check_eq(TEST_DESCRIPTION, 
              annosvc.getItemAnnotation(folderId, DESCRIPTION_ANNO));
  do_check_eq(observer._itemAddedIndex, bmStartIndex);
  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedId, folderId);
  txn1.undoTransaction();
  do_check_eq(observer._itemRemovedId, folderId);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, bmStartIndex);
  txn1.redoTransaction();
  do_check_eq(observer._itemAddedIndex, bmStartIndex);
  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedId, folderId);
  txn1.undoTransaction();
  do_check_eq(observer._itemRemovedId, folderId);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, bmStartIndex);

  
  
  var txn2 = ptSvc.createItem(uri("http://www.example.com"), root, bmStartIndex, "Testing1");
  ptSvc.doTransaction(txn2); 
  var b = (bmsvc.getBookmarkIdsForURI(uri("http://www.example.com"), {}))[0];
  do_check_eq(observer._itemAddedId, b);
  do_check_eq(observer._itemAddedIndex, bmStartIndex);
  do_check_true(bmsvc.isBookmarked(uri("http://www.example.com")));
  txn2.undoTransaction();
  do_check_eq(observer._itemRemovedId, b);
  do_check_eq(observer._itemRemovedIndex, bmStartIndex);
  do_check_false(bmsvc.isBookmarked(uri("http://www.example.com")));
  txn2.redoTransaction();
  do_check_true(bmsvc.isBookmarked(uri("http://www.example.com")));
  var newId = (bmsvc.getBookmarkIdsForURI(uri("http://www.example.com"), {}))[0];
  do_check_eq(observer._itemAddedIndex, bmStartIndex);
  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedId, newId);
  txn2.undoTransaction();
  do_check_eq(observer._itemRemovedId, newId);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, bmStartIndex);

  
  var txn2a = ptSvc.createFolder("Folder", root, bmStartIndex);
  ptSvc.doTransaction(txn2a);
  var fldrId = bmsvc.getChildFolder(root, "Folder");
  var txn2b = ptSvc.createItem(uri("http://www.example2.com"), fldrId, bmStartIndex, "Testing1b");
  ptSvc.doTransaction(txn2b);
  var b2 = (bmsvc.getBookmarkIdsForURI(uri("http://www.example2.com"), {}))[0];
  do_check_eq(observer._itemAddedId, b2);
  do_check_eq(observer._itemAddedIndex, bmStartIndex);
  do_check_true(bmsvc.isBookmarked(uri("http://www.example2.com")));
  txn2b.undoTransaction();
  do_check_eq(observer._itemRemovedId, b2);
  do_check_eq(observer._itemRemovedIndex, bmStartIndex);
  txn2b.redoTransaction();
  newId = (bmsvc.getBookmarkIdsForURI(uri("http://www.example2.com"), {}))[0];
  do_check_eq(observer._itemAddedIndex, bmStartIndex);
  do_check_eq(observer._itemAddedParent, fldrId);
  do_check_eq(observer._itemAddedId, newId);
  txn2b.undoTransaction();
  do_check_eq(observer._itemRemovedId, newId);
  do_check_eq(observer._itemRemovedFolder, fldrId);
  do_check_eq(observer._itemRemovedIndex, bmStartIndex);

  
  ptSvc.doTransaction(ptSvc.createItem(uri("http://www.example3.com"), root, -1, "Testing2"));
  ptSvc.doTransaction(ptSvc.createItem(uri("http://www.example3.com"), root, -1, "Testing3"));
  ptSvc.doTransaction(ptSvc.createItem(uri("http://www.example3.com"), fldrId, -1, "Testing4"));
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
  txn3.redoTransaction();
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
  do_check_eq(observer._itemMovedNewIndex, 1);
  txn3.undoTransaction();
  do_check_eq(observer._itemMovedId, bkmk1Id);
  do_check_eq(observer._itemMovedOldParent, fldrId);
  do_check_eq(observer._itemMovedOldIndex, 1);
  do_check_eq(observer._itemMovedNewParent, root);
  do_check_eq(observer._itemMovedNewIndex, 1);
  txn3b.redoTransaction();
  do_check_eq(observer._itemMovedId, bkmk1Id);
  do_check_eq(observer._itemMovedOldParent, root);
  do_check_eq(observer._itemMovedOldIndex, 1);
  do_check_eq(observer._itemMovedNewParent, fldrId);
  do_check_eq(observer._itemMovedNewIndex, 1);
  txn3.undoTransaction();
  do_check_eq(observer._itemMovedId, bkmk1Id);
  do_check_eq(observer._itemMovedOldParent, fldrId);
  do_check_eq(observer._itemMovedOldIndex, 1);
  do_check_eq(observer._itemMovedNewParent, root);
  do_check_eq(observer._itemMovedNewIndex, 1);

  
  ptSvc.doTransaction(ptSvc.createFolder("Folder2", root, -1));
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
  txn4.redoTransaction();
  do_check_eq(observer._itemRemovedId, fldrId2);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, 3);
  txn4.undoTransaction();
  do_check_eq(observer._itemAddedId, fldrId2);
  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedIndex, 3);

  
  bmsvc.setKeywordForBookmark(bkmk2Id, "test_keyword");
  var txn5 = ptSvc.removeItem(bkmk2Id);
  txn5.doTransaction();
  do_check_eq(observer._itemRemovedId, bkmk2Id);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, 2);
  do_check_eq(bmsvc.getKeywordForBookmark(bkmk2Id), null);
  txn5.undoTransaction();
  var newbkmk2Id = observer._itemAddedId;
  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedIndex, 2);
  do_check_eq(bmsvc.getKeywordForBookmark(newbkmk2Id), "test_keyword");
  txn5.redoTransaction();
  do_check_eq(observer._itemRemovedId, newbkmk2Id);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, 2);
  do_check_eq(bmsvc.getKeywordForBookmark(newbkmk2Id), null);
  txn5.undoTransaction();
  do_check_eq(observer._itemAddedParent, root);
  do_check_eq(observer._itemAddedIndex, 2);

  
  var txn6 = ptSvc.createSeparator(root, 1);
  txn6.doTransaction();
  var sepId = observer._itemAddedId;
  do_check_eq(observer._itemAddedIndex, 1);
  do_check_eq(observer._itemAddedParent, root);
  txn6.undoTransaction();
  do_check_eq(observer._itemRemovedId, sepId);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, 1);
  txn6.redoTransaction();
  var newSepId = observer._itemAddedId;
  do_check_eq(observer._itemAddedIndex, 1);
  do_check_eq(observer._itemAddedParent, root);
  txn6.undoTransaction();
  do_check_eq(observer._itemRemovedId, newSepId);
  do_check_eq(observer._itemRemovedFolder, root);
  do_check_eq(observer._itemRemovedIndex, 1);

  
  ptSvc.doTransaction(ptSvc.createSeparator(root, 1));
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
  txn7.redoTransaction();
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
  txn8.redoTransaction();
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
  txn9.redoTransaction();
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

  
  var txn12 = ptSvc.createLivemark(uri("http://feeduri.com"),
                                   uri("http://siteuri.com"),
                                   "Livemark1", root);
  txn12.doTransaction();
  var lvmkId = observer._itemAddedId;
  do_check_true(lmsvc.isLivemark(lvmkId));
  do_check_eq(lmsvc.getSiteURI(lvmkId).spec, "http://siteuri.com/");
  do_check_eq(lmsvc.getFeedURI(lvmkId).spec, "http://feeduri.com/");
  txn12.undoTransaction();
  do_check_false(lmsvc.isLivemark(lvmkId));
  txn12.redoTransaction();
  lvmkId = observer._itemAddedId;
  do_check_true(lmsvc.isLivemark(lvmkId));
  do_check_eq(lmsvc.getSiteURI(lvmkId).spec, "http://siteuri.com/");
  do_check_eq(lmsvc.getFeedURI(lvmkId).spec, "http://feeduri.com/");

  
  var txn13 = ptSvc.editLivemarkSiteURI(lvmkId, uri("http://new-siteuri.com/"));
  txn13.doTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/siteURI");
  do_check_eq(lmsvc.getSiteURI(lvmkId).spec, "http://new-siteuri.com/");
  txn13.undoTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/siteURI");
  do_check_eq(observer._itemChangedValue, "");
  do_check_eq(lmsvc.getSiteURI(lvmkId).spec, "http://siteuri.com/");
  txn13.redoTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/siteURI");
  do_check_eq(lmsvc.getSiteURI(lvmkId).spec, "http://new-siteuri.com/");
  txn13.undoTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/siteURI");
  do_check_eq(observer._itemChangedValue, "");
  do_check_eq(lmsvc.getSiteURI(lvmkId).spec, "http://siteuri.com/");

  
  var txn14 = ptSvc.editLivemarkFeedURI(lvmkId, uri("http://new-feeduri.com/"));
  txn14.doTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/feedURI");
  do_check_eq(lmsvc.getFeedURI(lvmkId).spec, "http://new-feeduri.com/");
  txn14.undoTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/feedURI");
  do_check_eq(observer._itemChangedValue, "");
  do_check_eq(lmsvc.getFeedURI(lvmkId).spec, "http://feeduri.com/");
  txn14.redoTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/feedURI");
  do_check_eq(observer._itemChangedValue, "");
  do_check_eq(lmsvc.getFeedURI(lvmkId).spec, "http://new-feeduri.com/");
  txn14.undoTransaction();
  do_check_eq(observer._itemChangedId, lvmkId);
  do_check_eq(observer._itemChangedProperty, "livemark/feedURI");
  do_check_eq(observer._itemChangedValue, "");
  do_check_eq(lmsvc.getFeedURI(lvmkId).spec, "http://feeduri.com/");

  
  
  annosvc.setItemAnnotation(lvmkId, "livemark/testAnno", "testAnno",
                            0, annosvc.EXPIRE_NEVER);
  var txn15 = ptSvc.removeItem(lvmkId);
  txn15.doTransaction();
  do_check_false(lmsvc.isLivemark(lvmkId));
  do_check_eq(observer._itemRemovedId, lvmkId);
  txn15.undoTransaction();
  lvmkId = observer._itemAddedId;
  do_check_true(lmsvc.isLivemark(lvmkId));
  do_check_eq(lmsvc.getSiteURI(lvmkId).spec, "http://siteuri.com/");
  do_check_eq(lmsvc.getFeedURI(lvmkId).spec, "http://feeduri.com/");
  do_check_eq(annosvc.getItemAnnotation(lvmkId, "livemark/testAnno"), "testAnno");
  txn15.redoTransaction();
  do_check_false(lmsvc.isLivemark(lvmkId));
  do_check_eq(observer._itemRemovedId, lvmkId);

  
  const LOAD_IN_SIDEBAR_ANNO = "bookmarkProperties/loadInSidebar";
  var txn16 = ptSvc.setLoadInSidebar(bkmk1Id, true);
  txn16.doTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, LOAD_IN_SIDEBAR_ANNO);
  do_check_eq(observer._itemChanged_isAnnotationProperty, true);
  txn16.undoTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, LOAD_IN_SIDEBAR_ANNO);
  do_check_eq(observer._itemChanged_isAnnotationProperty, true);

  
  var itemAnnoObj = { name: "testAnno/testInt",
                      type: Ci.nsIAnnotationService.TYPE_INT32,
                      flags: 0,
                      value: 123,
                      expires: Ci.nsIAnnotationService.EXPIRE_NEVER };
  var genItemAnnoTxn = ptSvc.setItemAnnotation(bkmk1Id, itemAnnoObj);
  genItemAnnoTxn.doTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, "testAnno/testInt");
  do_check_eq(observer._itemChanged_isAnnotationProperty, true);
  genItemAnnoTxn.undoTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, "testAnno/testInt");
  do_check_eq(observer._itemChanged_isAnnotationProperty, true);
  genItemAnnoTxn.redoTransaction();
  do_check_eq(observer._itemChangedId, bkmk1Id);
  do_check_eq(observer._itemChangedProperty, "testAnno/testInt");
  do_check_eq(observer._itemChanged_isAnnotationProperty, true);

  
  var pageAnnoObj = { name: "testAnno/testInt",
                      type: Ci.nsIAnnotationService.TYPE_INT32,
                      flags: 0,
                      value: 123,
                      expires: Ci.nsIAnnotationService.EXPIRE_NEVER };
  var genPageAnnoTxn = ptSvc.setPageAnnotation(uri("http://www.mozilla.org/"), pageAnnoObj);
  genPageAnnoTxn.doTransaction();
  do_check_true(annosvc.pageHasAnnotation(uri("http://www.mozilla.org/"), "testAnno/testInt"));
  genPageAnnoTxn.undoTransaction();
  do_check_false(annosvc.pageHasAnnotation(uri("http://www.mozilla.org/"), "testAnno/testInt"));
  genPageAnnoTxn.redoTransaction();
  do_check_true(annosvc.pageHasAnnotation(uri("http://www.mozilla.org/"), "testAnno/testInt"));

  
  ptSvc.doTransaction(ptSvc.createFolder("Sorting folder", root, bmStartIndex, [], null));
  var srtFldId = bmsvc.getChildFolder(root, "Sorting folder");
  ptSvc.doTransaction(ptSvc.createItem(uri("http://www.sortingtest.com"), srtFldId, -1, "c"));
  ptSvc.doTransaction(ptSvc.createItem(uri("http://www.sortingtest.com"), srtFldId, -1, "b"));   
  ptSvc.doTransaction(ptSvc.createItem(uri("http://www.sortingtest.com"), srtFldId, -1, "a"));
  var b = bmsvc.getBookmarkIdsForURI(uri("http://www.sortingtest.com"), {});
  b.sort();
  var b1 = b[0];
  var b2 = b[1];
  var b3 = b[2];
  do_check_eq(0, bmsvc.getItemIndex(b1));
  do_check_eq(1, bmsvc.getItemIndex(b2));
  do_check_eq(2, bmsvc.getItemIndex(b3));
  var txn17 = ptSvc.sortFolderByName(srtFldId);
  txn17.doTransaction();
  do_check_eq(2, bmsvc.getItemIndex(b1));
  do_check_eq(1, bmsvc.getItemIndex(b2));
  do_check_eq(0, bmsvc.getItemIndex(b3));
  txn17.undoTransaction();
  do_check_eq(0, bmsvc.getItemIndex(b1));
  do_check_eq(1, bmsvc.getItemIndex(b2));
  do_check_eq(2, bmsvc.getItemIndex(b3));
  txn17.redoTransaction();
  do_check_eq(2, bmsvc.getItemIndex(b1));
  do_check_eq(1, bmsvc.getItemIndex(b2));
  do_check_eq(0, bmsvc.getItemIndex(b3));
  txn17.undoTransaction();
  do_check_eq(0, bmsvc.getItemIndex(b1));
  do_check_eq(1, bmsvc.getItemIndex(b2));
  do_check_eq(2, bmsvc.getItemIndex(b3));

  
  var tmpMs = mss.createMicrosummary(uri("http://testmicro.com"), 
                                     uri("http://dietrich.ganx4.com/mozilla/test-microsummary.xml"));
  ptSvc.doTransaction(
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

  
  const POST_DATA_ANNO = "bookmarkProperties/POSTData";
  var postData = "foo";
  var postDataURI = uri("http://foo.com");
  ptSvc.doTransaction(
    ptSvc.createItem(postDataURI, root, -1, "postdata test", null, null, null));
  var postDataId = (bmsvc.getBookmarkIdsForURI(postDataURI,{}))[0];
  var postDataTxn = ptSvc.editBookmarkPostData(postDataId, postData);
  postDataTxn.doTransaction();
  do_check_true(annosvc.itemHasAnnotation(postDataId, POST_DATA_ANNO))
  do_check_eq(annosvc.getItemAnnotation(postDataId, POST_DATA_ANNO), postData);
  postDataTxn.undoTransaction();
  do_check_false(annosvc.itemHasAnnotation(postDataId, POST_DATA_ANNO))

  
  var oldAdded = bmsvc.getItemDateAdded(bkmk1Id);
  var newAdded = Date.now();
  var eidaTxn = ptSvc.editItemDateAdded(bkmk1Id, newAdded);
  eidaTxn.doTransaction();
  do_check_eq(newAdded, bmsvc.getItemDateAdded(bkmk1Id));
  eidaTxn.undoTransaction();
  do_check_eq(oldAdded, bmsvc.getItemDateAdded(bkmk1Id));

  
  var oldModified = bmsvc.getItemLastModified(bkmk1Id);
  var newModified = Date.now();
  var eilmTxn = ptSvc.editItemLastModified(bkmk1Id, newModified);
  eilmTxn.doTransaction();
  do_check_eq(newModified, bmsvc.getItemLastModified(bkmk1Id));
  eilmTxn.undoTransaction();
  do_check_eq(oldModified, bmsvc.getItemLastModified(bkmk1Id));

  
  var tagURI = uri("http://foo.tld");
  var tagTxn = ptSvc.tagURI(tagURI, ["foo", "bar"]);
  tagTxn.doTransaction();
  do_check_eq(uneval(tagssvc.getTagsForURI(tagURI, { })), uneval(["bar","foo"]));
  tagTxn.undoTransaction();
  do_check_true(tagssvc.getTagsForURI(tagURI, { }).length == 0);
  tagTxn.redoTransaction();
  do_check_eq(uneval(tagssvc.getTagsForURI(tagURI, { })), uneval(["bar","foo"]));
  var untagTxn = ptSvc.untagURI(tagURI, ["bar"]);
  untagTxn.doTransaction();
  do_check_eq(uneval(tagssvc.getTagsForURI(tagURI, { })), uneval(["foo"]));
  untagTxn.undoTransaction();
  do_check_eq(uneval(tagssvc.getTagsForURI(tagURI, { })), uneval(["bar","foo"]));
  untagTxn.redoTransaction();
  do_check_eq(uneval(tagssvc.getTagsForURI(tagURI, { })), uneval(["foo"]));

  
  var bkmk1Id = bmsvc.insertBookmark(root, uri("http://www.mozilla.org/"), 0, "Mozilla");
  var bkmk2Id = bmsvc.insertSeparator(root, 1);
  var bkmk3Id = bmsvc.createFolder(root, "folder", 2);
  var bkmk3_1Id = bmsvc.insertBookmark(bkmk3Id, uri("http://www.mozilla.org/"), 0, "Mozilla");
  var bkmk3_2Id = bmsvc.insertSeparator(bkmk3Id, 1);
  var bkmk3_3Id = bmsvc.createFolder(bkmk3Id, "folder", 2);

  var transactions = [];
  transactions.push(ptSvc.removeItem(bkmk1Id));
  transactions.push(ptSvc.removeItem(bkmk2Id));
  transactions.push(ptSvc.removeItem(bkmk3Id));
  var txn = ptSvc.aggregateTransactions("RemoveItems", transactions);

  txn.doTransaction();
  do_check_eq(bmsvc.getItemIndex(bkmk1Id), -1);
  do_check_eq(bmsvc.getItemIndex(bkmk2Id), -1);
  do_check_eq(bmsvc.getItemIndex(bkmk3Id), -1);
  do_check_eq(bmsvc.getItemIndex(bkmk3_1Id), -1);
  do_check_eq(bmsvc.getItemIndex(bkmk3_2Id), -1);
  do_check_eq(bmsvc.getItemIndex(bkmk3_3Id), -1);

  txn.undoTransaction();
  var newBkmk1Id = bmsvc.getIdForItemAt(root, 0);
  var newBkmk2Id = bmsvc.getIdForItemAt(root, 1);
  var newBkmk3Id = bmsvc.getIdForItemAt(root, 2);
  var newBkmk3_1Id = bmsvc.getIdForItemAt(newBkmk3Id, 0);
  var newBkmk3_2Id = bmsvc.getIdForItemAt(newBkmk3Id, 1);
  var newBkmk3_3Id = bmsvc.getIdForItemAt(newBkmk3Id, 2);
  do_check_eq(bmsvc.getItemType(newBkmk1Id), bmsvc.TYPE_BOOKMARK);
  do_check_eq(bmsvc.getBookmarkURI(newBkmk1Id).spec, "http://www.mozilla.org/");
  do_check_eq(bmsvc.getItemType(newBkmk2Id), bmsvc.TYPE_SEPARATOR);
  do_check_eq(bmsvc.getItemType(newBkmk3Id), bmsvc.TYPE_FOLDER);
  do_check_eq(bmsvc.getItemTitle(newBkmk3Id), "folder");
  do_check_eq(bmsvc.getFolderIdForItem(newBkmk3_1Id), newBkmk3Id);
  do_check_eq(bmsvc.getItemType(newBkmk3_1Id), bmsvc.TYPE_BOOKMARK);
  do_check_eq(bmsvc.getBookmarkURI(newBkmk3_1Id).spec, "http://www.mozilla.org/");
  do_check_eq(bmsvc.getFolderIdForItem(newBkmk3_2Id), newBkmk3Id);
  do_check_eq(bmsvc.getItemType(newBkmk3_2Id), bmsvc.TYPE_SEPARATOR);
  do_check_eq(bmsvc.getFolderIdForItem(newBkmk3_3Id), newBkmk3Id);
  do_check_eq(bmsvc.getItemType(newBkmk3_3Id), bmsvc.TYPE_FOLDER);
  do_check_eq(bmsvc.getItemTitle(newBkmk3_3Id), "folder");

  txn.redoTransaction();
  do_check_eq(bmsvc.getItemIndex(newBkmk1Id), -1);
  do_check_eq(bmsvc.getItemIndex(newBkmk2Id), -1);
  do_check_eq(bmsvc.getItemIndex(newBkmk3Id), -1);
  do_check_eq(bmsvc.getItemIndex(newBkmk3_1Id), -1);
  do_check_eq(bmsvc.getItemIndex(newBkmk3_2Id), -1);
  do_check_eq(bmsvc.getItemIndex(newBkmk3_3Id), -1);

  txn.undoTransaction();
  newBkmk1Id = bmsvc.getIdForItemAt(root, 0);
  newBkmk2Id = bmsvc.getIdForItemAt(root, 1);
  newBkmk3Id = bmsvc.getIdForItemAt(root, 2);
  newBkmk3_1Id = bmsvc.getIdForItemAt(newBkmk3Id, 0);
  newBkmk3_2Id = bmsvc.getIdForItemAt(newBkmk3Id, 1);
  newBkmk3_3Id = bmsvc.getIdForItemAt(newBkmk3Id, 2);
  do_check_eq(bmsvc.getItemType(newBkmk1Id), bmsvc.TYPE_BOOKMARK);
  do_check_eq(bmsvc.getBookmarkURI(newBkmk1Id).spec, "http://www.mozilla.org/");
  do_check_eq(bmsvc.getItemType(newBkmk2Id), bmsvc.TYPE_SEPARATOR);
  do_check_eq(bmsvc.getItemType(newBkmk3Id), bmsvc.TYPE_FOLDER);
  do_check_eq(bmsvc.getItemTitle(newBkmk3Id), "folder");
  do_check_eq(bmsvc.getFolderIdForItem(newBkmk3_1Id), newBkmk3Id);
  do_check_eq(bmsvc.getItemType(newBkmk3_1Id), bmsvc.TYPE_BOOKMARK);
  do_check_eq(bmsvc.getBookmarkURI(newBkmk3_1Id).spec, "http://www.mozilla.org/");
  do_check_eq(bmsvc.getFolderIdForItem(newBkmk3_2Id), newBkmk3Id);
  do_check_eq(bmsvc.getItemType(newBkmk3_2Id), bmsvc.TYPE_SEPARATOR);
  do_check_eq(bmsvc.getFolderIdForItem(newBkmk3_3Id), newBkmk3Id);
  do_check_eq(bmsvc.getItemType(newBkmk3_3Id), bmsvc.TYPE_FOLDER);
  do_check_eq(bmsvc.getItemTitle(newBkmk3_3Id), "folder");

  
  var newDateAdded = Date.now() - 20000;
  var childTxn = ptSvc.editItemDateAdded(null, newDateAdded);
  var itemWChildTxn = ptSvc.createItem(uri("http://www.example.com"), root, bmStartIndex, "Testing1", null, null, [childTxn]);
  try {
    ptSvc.doTransaction(itemWChildTxn); 
    var itemId = (bmsvc.getBookmarkIdsForURI(uri("http://www.example.com"), {}))[0];
    do_check_eq(observer._itemAddedId, itemId);
    do_check_eq(newDateAdded, bmsvc.getItemDateAdded(itemId));
    itemWChildTxn.undoTransaction();
    do_check_eq(observer._itemRemovedId, itemId);
    itemWChildTxn.redoTransaction();
    do_check_true(bmsvc.isBookmarked(uri("http://www.example.com")));
    var newId = (bmsvc.getBookmarkIdsForURI(uri("http://www.example.com"), {}))[0];
    do_check_eq(newDateAdded, bmsvc.getItemDateAdded(newId));
    do_check_eq(observer._itemAddedId, newId);
    itemWChildTxn.undoTransaction();
    do_check_eq(observer._itemRemovedId, newId);
  } catch (ex) {
    do_throw("Setting a child transaction in a createItem transaction did throw: " + ex);
  }

  
  var childItemTxn = ptSvc.createItem(uri("http://www.childItem.com"), root, bmStartIndex, "childItem");
  var folderWChildItemTxn = ptSvc.createFolder("Folder", root, bmStartIndex, null, [childItemTxn]);
  try {
    ptSvc.doTransaction(folderWChildItemTxn);
    var childItemId = (bmsvc.getBookmarkIdsForURI(uri("http://www.childItem.com"), {}))[0];
    do_check_eq(observer._itemAddedId, childItemId);
    do_check_eq(observer._itemAddedIndex, 0);
    do_check_true(bmsvc.isBookmarked(uri("http://www.childItem.com")));
    folderWChildItemTxn.undoTransaction();
    do_check_false(bmsvc.isBookmarked(uri("http://www.childItem.com")));
    folderWChildItemTxn.redoTransaction();
    newchildItemId = (bmsvc.getBookmarkIdsForURI(uri("http://www.childItem.com"), {}))[0];
    do_check_eq(observer._itemAddedIndex, 0);
    do_check_eq(observer._itemAddedId, newchildItemId);
    do_check_true(bmsvc.isBookmarked(uri("http://www.childItem.com")));
    folderWChildItemTxn.undoTransaction();
    do_check_false(bmsvc.isBookmarked(uri("http://www.childItem.com")));
  } catch (ex) {
    do_throw("Setting a child item transaction in a createFolder transaction did throw: " + ex);
  }

  bmsvc.removeObserver(observer, false);
}
