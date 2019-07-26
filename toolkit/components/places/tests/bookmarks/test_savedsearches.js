






var root = PlacesUtils.bookmarksMenuFolderId;


const searchTerm = "about";

var testRoot;


function run_test() {
  
  
  
  testRoot = PlacesUtils.bookmarks.createFolder(
    root, searchTerm, PlacesUtils.bookmarks.DEFAULT_INDEX);

  run_next_test();
}

add_test(function test_savedsearches_bookmarks() {
  
  var bookmarkId = PlacesUtils.bookmarks.insertBookmark(
    root, uri("http://foo.com"), PlacesUtils.bookmarks.DEFAULT_INDEX,
    searchTerm);

  
  var searchId = PlacesUtils.bookmarks.insertBookmark(
    testRoot, uri("place:terms=" + searchTerm + "&excludeQueries=1&expandQueries=1&queryType=1"),
    PlacesUtils.bookmarks.DEFAULT_INDEX, searchTerm);

  
  
  try {
    var options = PlacesUtils.history.getNewQueryOptions();
    options.expandQueries = 0;
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([testRoot], 1);
    var result = PlacesUtils.history.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    do_check_eq(cc, 1);
    for (var i = 0; i < cc; i++) {
      var node = rootNode.getChild(i);
      
      do_check_true(node.itemId > 0);
      
      node.QueryInterface(Ci.nsINavHistoryContainerResultNode);
      do_check_eq(node.containerOpen, false);
    }
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("expandQueries=0 query error: " + ex);
  }

  
  
  
  try {
    var options = PlacesUtils.history.getNewQueryOptions();
    options.expandQueries = 1;
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([testRoot], 1);
    var result = PlacesUtils.history.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    do_check_eq(cc, 1);
    for (var i = 0; i < cc; i++) {
      var node = rootNode.getChild(i);
      
      do_check_eq(node.type, node.RESULT_TYPE_QUERY);
      
      do_check_true(node.itemId > 0);
      node.QueryInterface(Ci.nsINavHistoryContainerResultNode);
      node.containerOpen = true;

      
      
      
      do_check_eq(node.childCount, 1);

      
      var item = node.getChild(0);
      do_check_eq(item.itemId, bookmarkId);

      
      
      
      
      

      
      
      

      
      PlacesUtils.bookmarks.createFolder(
        root, searchTerm + "zaa", PlacesUtils.bookmarks.DEFAULT_INDEX);
      do_check_eq(node.childCount, 1);
      
      PlacesUtils.bookmarks.insertBookmark(
        root, uri("place:terms=foo&excludeQueries=1&expandQueries=1&queryType=1"),
        PlacesUtils.bookmarks.DEFAULT_INDEX, searchTerm + "blah");
      do_check_eq(node.childCount, 1);
    }
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("expandQueries=1 bookmarks query: " + ex);
  }

  
  PlacesUtils.bookmarks.removeItem(searchId);

  run_next_test();
});

add_task(function test_savedsearches_history() {
  
  var testURI = uri("http://" + searchTerm + ".com");
  yield promiseAddVisits({ uri: testURI, title: searchTerm });

  
  var searchId = PlacesUtils.bookmarks.insertBookmark(testRoot,
    uri("place:terms=" + searchTerm + "&excludeQueries=1&expandQueries=1&queryType=0"),
    PlacesUtils.bookmarks.DEFAULT_INDEX, searchTerm);

  
  
  try {
    var options = PlacesUtils.history.getNewQueryOptions();
    options.expandQueries = 1;
    var query = PlacesUtils.history.getNewQuery();
    query.setFolders([testRoot], 1);
    var result = PlacesUtils.history.executeQuery(query, options);
    var rootNode = result.root;
    rootNode.containerOpen = true;
    var cc = rootNode.childCount;
    do_check_eq(cc, 1);
    for (var i = 0; i < cc; i++) {
      var node = rootNode.getChild(i);
      
      do_check_eq(node.type, node.RESULT_TYPE_QUERY);
      
      do_check_eq(node.itemId, searchId);
      node.QueryInterface(Ci.nsINavHistoryContainerResultNode);
      node.containerOpen = true;

      
      
      
      do_check_eq(node.childCount, 1);

      
      var item = node.getChild(0);
      do_check_eq(item.type, item.RESULT_TYPE_URI);
      do_check_eq(item.itemId, -1); 
      do_check_eq(item.uri, testURI.spec); 

      
      yield promiseAddVisits({
        uri: uri("http://foo.com"),
        title: searchTerm + "blah"
      });
      do_check_eq(node.childCount, 2);

      
      PlacesUtils.history.removePage(uri("http://foo.com"));
      do_check_eq(node.childCount, 1);
      node.containerOpen = false;
    }

    
    var tmpFolderId = PlacesUtils.bookmarks.createFolder(
      testRoot, "foo", PlacesUtils.bookmarks.DEFAULT_INDEX);
    PlacesUtils.bookmarks.moveItem(
      searchId, tmpFolderId, PlacesUtils.bookmarks.DEFAULT_INDEX);
    var tmpFolderNode = rootNode.getChild(0);
    do_check_eq(tmpFolderNode.itemId, tmpFolderId);
    tmpFolderNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
    tmpFolderNode.containerOpen = true;
    do_check_eq(tmpFolderNode.childCount, 1);

    
    PlacesUtils.bookmarks.setItemTitle(searchId, "foo");
    do_check_eq(tmpFolderNode.title, "foo");

    
    PlacesUtils.bookmarks.removeItem(searchId);
    try {
      var tmpFolderNode = root.getChild(1);
      do_throw("query was not removed");
    } catch(ex) {}

    tmpFolderNode.containerOpen = false;
    rootNode.containerOpen = false;
  }
  catch(ex) {
    do_throw("expandQueries=1 bookmarks query: " + ex);
  }
});
