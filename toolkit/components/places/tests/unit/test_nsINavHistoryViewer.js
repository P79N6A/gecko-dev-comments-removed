







































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
  insertedNode: null,
  nodeInserted: function(parent, node, newIndex) {
    this.insertedNode = node;
  },
  removedNode: null,
  nodeRemoved: function(parent, node, oldIndex) {
    this.removedNode = node;
  },

  newTitle: "",
  nodeChangedByTitle: null,
  nodeTitleChanged: function(node, newTitle) {
    this.nodeChangedByTitle = node;
    this.newTitle = newTitle;
  },

  newAccessCount: 0,
  newTime: 0,
  nodeChangedByHistoryDetails: null,
  nodeHistoryDetailsChanged: function(node,
                                         updatedVisitDate,
                                         updatedVisitCount) {
    this.nodeChangedByHistoryDetails = node
    this.newTime = updatedVisitDate;
    this.newAccessCount = updatedVisitCount;
  },

  replacedNode: null,
  nodeReplaced: function(parent, oldNode, newNode, index) {
    this.replacedNode = node;
  },
  movedNode: null,
  nodeMoved: function(node, oldParent, oldIndex, newParent, newIndex) {
    this.movedNode = node;
  },
  openedContainer: null,
  containerOpened: function(node) {
    this.openedContainer = node;
  },
  closedContainer: null,
  containerClosed: function(node) {
    this.closedContainer = node;
  },
  invalidatedContainer: null,
  invalidateContainer: function(node) {    
    this.invalidatedContainer = node;
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
    this.insertedNode = null;
    this.removedNode = null;
    this.nodeChangedByTitle = null;
    this.nodeChangedByHistoryDetails = null;
    this.replacedNode = null;
    this.movedNode = null;
    this.openedContainer = null;
    this.closedContainer = null;
    this.invalidatedContainer = null;
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
  do_check_eq(testURI.spec, viewer.insertedNode.uri);

  
  
  do_check_eq(root.uri, viewer.nodeChangedByHistoryDetails.uri);

  
  bhist.addPageWithDetails(testURI, "baz", Date.now() * 1000);
  do_check_eq(viewer.nodeChangedByTitle.title, "baz");

  
  var removedURI = uri("http://google.com");
  add_visit(removedURI);
  bhist.removePage(removedURI);
  do_check_eq(removedURI.spec, viewer.removedNode.uri);

  
  

  
  bhist.removePagesFromHost("mozilla.com", false);
  do_check_eq(root.uri, viewer.invalidatedContainer.uri);

  
  viewer.invalidatedContainer = null;
  result.sortingMode = options.SORT_BY_TITLE_ASCENDING;
  do_check_eq(viewer.sortingMode, options.SORT_BY_TITLE_ASCENDING);
  do_check_eq(viewer.invalidatedContainer, result.root);

  
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
  do_check_eq("foo", viewer.insertedNode.title);
  do_check_eq(testURI.spec, viewer.insertedNode.uri);

  
  
  do_check_eq(root.uri, viewer.nodeChangedByHistoryDetails.uri);

  
  bmsvc.setItemTitle(testBookmark, "baz");
  do_check_eq(viewer.nodeChangedByTitle.title, "baz");
  do_check_eq(viewer.newTitle, "baz");

  var testBookmark2 = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, uri("http://google.com"), bmsvc.DEFAULT_INDEX, "foo");
  bmsvc.moveItem(testBookmark2, bmsvc.bookmarksMenuFolder, 0);
  do_check_eq(viewer.movedNode.itemId, testBookmark2);

  
  bmsvc.removeItem(testBookmark2);
  do_check_eq(testBookmark2, viewer.removedNode.itemId);

  
  

  

  
  viewer.invalidatedContainer = null;
  result.sortingMode = options.SORT_BY_TITLE_ASCENDING;
  do_check_eq(viewer.sortingMode, options.SORT_BY_TITLE_ASCENDING);
  do_check_eq(viewer.invalidatedContainer, result.root);

  
  root.containerOpen = false;
  do_check_eq(viewer.closedContainer, viewer.openedContainer);
  result.viewer = null;
}
