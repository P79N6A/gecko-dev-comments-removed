






var histsvc = PlacesUtils.history;
var bhist = PlacesUtils.bhistory;
var bmsvc = PlacesUtils.bookmarks;

var resultObserver = {
  insertedNode: null,
  nodeInserted: function(parent, node, newIndex) {
    this.insertedNode = node;
  },
  removedNode: null,
  nodeRemoved: function(parent, node, oldIndex) {
    this.removedNode = node;
  },

  nodeAnnotationChanged: function() {},

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

  movedNode: null,
  nodeMoved: function(node, oldParent, oldIndex, newParent, newIndex) {
    this.movedNode = node;
  },
  openedContainer: null,
  closedContainer: null,
  containerStateChanged: function (aNode, aOldState, aNewState) {
    if (aNewState == Ci.nsINavHistoryContainerResultNode.STATE_OPENED) {
      this.openedContainer = aNode;
    }
    else if (aNewState == Ci.nsINavHistoryContainerResultNode.STATE_CLOSED) {
      this.closedContainer = aNode;
    }
  },
  invalidatedContainer: null,
  invalidateContainer: function(node) {
    this.invalidatedContainer = node;
  },
  sortingMode: null,
  sortingChanged: function(sortingMode) {
    this.sortingMode = sortingMode;
  },
  inBatchMode: false,
  batching: function(aToggleMode) {
    this.inBatchMode = aToggleMode;
  },
  result: null,
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

function promiseBatch(aResult) {
  let query = PlacesUtils.asQuery(aResult.root).getQueries()[0];
  return Task.spawn(function() {
    let deferred = Promise.defer();
    aResult.addObserver({
      batching: function (aStatus) {
        if (!aStatus) {
          deferred.resolve();
          aResult.removeObserver(this);
        }
      }
    }, false);
    for (let i = 0; i < 10; i++) {
      if (query.onlyBookmarked)
        bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, testURI, bmsvc.DEFAULT_INDEX, "foo");
      else
        yield promiseAddVisits(testURI);
    }
    yield deferred.promise;
  });
}

var testURI = uri("http://mozilla.com");

function run_test() {
  run_next_test();
}

add_task(function check_history_query() {
  var options = histsvc.getNewQueryOptions();
  options.sortingMode = options.SORT_BY_DATE_DESCENDING;
  options.resultType = options.RESULTS_AS_VISIT;
  var query = histsvc.getNewQuery();
  var result = histsvc.executeQuery(query, options);
  result.addObserver(resultObserver, false);
  var root = result.root;
  root.containerOpen = true;

  
  do_check_neq(resultObserver.openedContainer, null);

  
  yield promiseAddVisits(testURI);
  do_check_eq(testURI.spec, resultObserver.insertedNode.uri);

  
  
  do_check_eq(root.uri, resultObserver.nodeChangedByHistoryDetails.uri);

  
  yield promiseAddVisits({ uri: testURI, title: "baz" });

  do_check_eq(resultObserver.nodeChangedByTitle.title, "baz");

  
  var removedURI = uri("http://google.com");
  yield promiseAddVisits(removedURI);
  bhist.removePage(removedURI);
  do_check_eq(removedURI.spec, resultObserver.removedNode.uri);

  
  resultObserver.invalidatedContainer = null;
  result.sortingMode = options.SORT_BY_TITLE_ASCENDING;
  do_check_eq(resultObserver.sortingMode, options.SORT_BY_TITLE_ASCENDING);
  do_check_eq(resultObserver.invalidatedContainer, result.root);

  
  bhist.removeAllPages();
  do_check_eq(root.uri, resultObserver.invalidatedContainer.uri);

  
  do_check_false(resultObserver.inBatchMode);
  yield promiseBatch(result);

  
  do_check_eq(root.uri, resultObserver.invalidatedContainer.uri);

  
  root.containerOpen = false;
  do_check_eq(resultObserver.closedContainer, resultObserver.openedContainer);
  result.removeObserver(resultObserver);
  resultObserver.reset();

  yield promiseAsyncUpdates();
});

add_task(function check_bookmarks_query() {
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.setFolders([bmsvc.bookmarksMenuFolder], 1);
  var result = histsvc.executeQuery(query, options);
  result.addObserver(resultObserver, false);
  var root = result.root;
  root.containerOpen = true;

  
  do_check_neq(resultObserver.openedContainer, null);

  
  
  var testBookmark = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, testURI, bmsvc.DEFAULT_INDEX, "foo");
  do_check_eq("foo", resultObserver.insertedNode.title);
  do_check_eq(testURI.spec, resultObserver.insertedNode.uri);

  
  
  do_check_eq(root.uri, resultObserver.nodeChangedByHistoryDetails.uri);

  
  bmsvc.setItemTitle(testBookmark, "baz");
  do_check_eq(resultObserver.nodeChangedByTitle.title, "baz");
  do_check_eq(resultObserver.newTitle, "baz");

  var testBookmark2 = bmsvc.insertBookmark(bmsvc.bookmarksMenuFolder, uri("http://google.com"), bmsvc.DEFAULT_INDEX, "foo");
  bmsvc.moveItem(testBookmark2, bmsvc.bookmarksMenuFolder, 0);
  do_check_eq(resultObserver.movedNode.itemId, testBookmark2);

  
  bmsvc.removeItem(testBookmark2);
  do_check_eq(testBookmark2, resultObserver.removedNode.itemId);

  
  resultObserver.invalidatedContainer = null;
  result.sortingMode = options.SORT_BY_TITLE_ASCENDING;
  do_check_eq(resultObserver.sortingMode, options.SORT_BY_TITLE_ASCENDING);
  do_check_eq(resultObserver.invalidatedContainer, result.root);

  
  do_check_false(resultObserver.inBatchMode);
  yield promiseBatch(result);

  
  do_check_eq(root.uri, resultObserver.invalidatedContainer.uri);

  
  root.containerOpen = false;
  do_check_eq(resultObserver.closedContainer, resultObserver.openedContainer);
  result.removeObserver(resultObserver);
  resultObserver.reset();

  yield promiseAsyncUpdates();
});

add_task(function check_mixed_query() {
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.onlyBookmarked = true;
  var result = histsvc.executeQuery(query, options);
  result.addObserver(resultObserver, false);
  var root = result.root;
  root.containerOpen = true;

  
  do_check_neq(resultObserver.openedContainer, null);

  
  do_check_false(resultObserver.inBatchMode);
  yield promiseBatch(result);

  
  do_check_eq(root.uri, resultObserver.invalidatedContainer.uri);

  
  root.containerOpen = false;
  do_check_eq(resultObserver.closedContainer, resultObserver.openedContainer);
  result.removeObserver(resultObserver);
  resultObserver.reset();

  yield promiseAsyncUpdates();
});
