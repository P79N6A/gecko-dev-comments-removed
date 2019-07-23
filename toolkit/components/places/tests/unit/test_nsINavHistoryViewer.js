







































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


try {
  var bhist = Cc["@mozilla.org/browser/global-history;2"].getService(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get history service\n");
} 


function add_visit(aURI, aDate) {
  var date = aDate || Date.now() * 1000;
  var placeID = histsvc.addVisit(aURI,
                                 date,
                                 null, 
                                 histsvc.TRANSITION_TYPED, 
                                 false, 
                                 0);
  do_check_true(placeID > 0);
  return placeID;
}

var viewer = {
  insertedItem: null,
  itemInserted: function(parent, item, newIndex) {
    this.insertedItem = item;
  },
  removedItem: null,
  itemRemoved: function(parent, item, oldIndex) {
    this.removedItem = item;
  },
  changedItem: null,
  itemChanged: function(item) {
    this.changedItem = item;
  },
  replacedItem: null,
  itemReplaced: function(parent, oldItem, newItem, index) {
    dump("itemReplaced: " + newItem.uri + "\n");
    this.replacedItem = item;
  },
  movedItem: null,
  itemMoved: function(item, oldParent, oldIndex, newParent, newIndex) {
    this.movedItem = item;
  },
  openedContainer: null,
  containerOpened: function(item) {
    this.openedContainer = item;
  },
  closedContainer: null,
  containerClosed: function(item) {
    this.closedContainer = item;
  },
  invalidatedContainer: null,
  invalidateContainer: function(item) {
    dump("invalidateContainer()\n");
    this.invalidatedContainer = item;
  },
  allInvalidated: null,
  invalidateAll: function() {
    this.allInvalidated = true;
  },
  sortingMode: null,
  sortingChanged: function(sortingMode) {
    this.sortingMode = sortingMode;
  },
  result: null,
  ignoreInvalidateContainer: false,
  addViewObserver: function(observer, ownsWeak) {},
  removeViewObserver: function(observer) {},
  reset: function() {
    this.insertedItem = null;
    this.removedItem = null;
    this.changedItem = null;
    this.replacedItem = null;
    this.movedItem = null;
    this.openedContainer = null;
    this.closedContainer = null;
    this.invalidatedContainer = null;
    this.allInvalidated = null;
    this.sortingMode = null;
  }
};


function run_test() {

  
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  result.viewer = viewer;
  var root = result.root;
  root.containerOpen = true;

  
  do_check_neq(viewer.openedContainer, null);

  
  
  var testURI = uri("http://mozilla.com");
  add_visit(testURI);
  do_check_eq(testURI.spec, viewer.insertedItem.uri);

  
  
  do_check_eq(root.uri, viewer.changedItem.uri);

  
  bhist.addPageWithDetails(testURI, "baz", Date.now() * 1000);
  do_check_eq(viewer.changedItem.title, "baz");

  
  var removedURI = uri("http://google.com");
  add_visit(removedURI);
  bhist.removePage(removedURI);
  do_check_eq(removedURI.spec, viewer.removedItem.uri);

  
  

  
  bhist.removePagesFromHost("mozilla.com", false);
  do_check_eq(root.uri, viewer.invalidatedContainer.uri);

  
  
  result.sortingMode = options.SORT_BY_TITLE_ASCENDING;
  do_check_true(viewer.allInvalidated);
  do_check_eq(viewer.sortingMode, options.SORT_BY_TITLE_ASCENDING);

  
  root.containerOpen = false;
  do_check_eq(viewer.closedContainer, viewer.openedContainer);
  result.viewer = null;

  
  
  
  viewer.reset();

  try {
    var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].getService(Ci.nsINavBookmarksService);
  } catch(ex) {
    do_throw("Could not get nav-bookmarks-service\n");
  }

  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.setFolders([bmsvc.bookmarksMenuFolder], 1);
  var result = histsvc.executeQuery(query, options);
  result.viewer = viewer;
  var root = result.root;
  root.containerOpen = true;

  
  do_check_neq(viewer.openedContainer, null);

  
  
  var testBookmark = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, testURI, bmsvc.DEFAULT_INDEX, "foo");
  do_check_eq("foo", viewer.insertedItem.title);
  do_check_eq(testURI.spec, viewer.insertedItem.uri);

  
  
  do_check_eq(root.uri, viewer.changedItem.uri);

  
  bmsvc.setItemTitle(testBookmark, "baz");
  do_check_eq(viewer.changedItem.title, "baz");

  var testBookmark2 = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, uri("http://google.com"), bmsvc.DEFAULT_INDEX, "foo");
  bmsvc.moveItem(testBookmark2, bmsvc.bookmarksMenuFolder, 0);
  do_check_eq(viewer.movedItem.itemId, testBookmark2);

  
  bmsvc.removeItem(testBookmark2);
  do_check_eq(testBookmark2, viewer.removedItem.itemId);

  
  

  

  
  
  result.sortingMode = options.SORT_BY_TITLE_ASCENDING;
  do_check_true(viewer.allInvalidated);
  do_check_eq(viewer.sortingMode, options.SORT_BY_TITLE_ASCENDING);

  
  root.containerOpen = false;
  do_check_eq(viewer.closedContainer, viewer.openedContainer);
  result.viewer = null;
}
