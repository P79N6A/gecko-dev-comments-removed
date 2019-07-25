





































const DEFAULT_INDEX = PlacesUtils.bookmarks.DEFAULT_INDEX;

function run_test() {
  








  var folderA =
    PlacesUtils.bookmarks.createFolder(PlacesUtils.toolbarFolderId,
                                       "test folder", DEFAULT_INDEX);
  var bookmarkURI = uri("http://test");
  PlacesUtils.bookmarks.insertBookmark(folderA, bookmarkURI,
                                       DEFAULT_INDEX, "");

  
  var queryURI = uri("place:folder=" + folderA);
  var queryTitle = "test query";
  var queryId =
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.toolbarFolderId,
                                         queryURI, DEFAULT_INDEX, queryTitle);
  LOG("queryId: " + queryId);

  
  var queryURI2 = uri("place:");
  var queryTitle2 = "non-folder test query";
  var queryId2 =
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.toolbarFolderId,
                                         queryURI2, DEFAULT_INDEX, queryTitle2);

  
  var query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.toolbarFolderId], 1);
  var options = PlacesUtils.history.getNewQueryOptions();
  options.expandQueries = true;
  var result = PlacesUtils.history.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  
  var queryNode = root.getChild(root.childCount-2);
  do_check_eq(queryNode.type, queryNode.RESULT_TYPE_FOLDER_SHORTCUT);
  do_check_eq(queryNode.title, queryTitle);
  do_check_true(queryURI.equals(uri(queryNode.uri)));
  queryNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  queryNode.containerOpen = true;
  do_check_eq(queryNode.childCount, 1);
  var bookmark = queryNode.getChild(0);
  do_check_true(bookmarkURI.equals(uri(bookmark.uri)));
  queryNode.containerOpen = false;

  
  var queryNode2 = root.getChild(root.childCount-1);
  do_check_eq(queryNode2.type, queryNode2.RESULT_TYPE_QUERY);
  do_check_eq(queryNode2.title, queryTitle2);
  do_check_true(queryURI2.equals(uri(queryNode2.uri)));
  queryNode2.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  queryNode2.containerOpen = true;
  do_check_eq(queryNode2.childCount, 0);
  queryNode2.containerOpen = false;

  
  root.containerOpen = false;

  
  var stream = {
    _str: "",
    write: function(aData, aLen) {
      this._str += aData;
    }
  };
  PlacesUtils.serializeNodeAsJSONToOutputStream(queryNode, stream, false, true);

  LOG("SERIALIZED: " + stream._str);

  PlacesUtils.bookmarks.removeItem(queryId);

  
  PlacesUtils.importJSONNode(stream._str, PlacesUtils.toolbarFolderId, -1);

  
  var query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.toolbarFolderId], 1);
  var options = PlacesUtils.history.getNewQueryOptions();
  var result = PlacesUtils.history.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;

  
  var queryNode = root.getChild(root.childCount-2);
  do_check_eq(queryNode.type, queryNode.RESULT_TYPE_FOLDER);
  queryNode.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  queryNode.containerOpen = true;
  do_check_eq(queryNode.childCount, 1);
  var child = queryNode.getChild(0);
  do_check_true(bookmarkURI.equals(uri(child.uri)));
  queryNode.containerOpen = false;

  var queryNode2 = root.getChild(root.childCount-1);
  do_check_eq(queryNode2.type, queryNode2.RESULT_TYPE_QUERY);
  queryNode2.QueryInterface(Ci.nsINavHistoryContainerResultNode);
  queryNode2.containerOpen = true;
  do_check_eq(queryNode2.childCount, 0);
  queryNode.containerOpen = false;

  root.containerOpen = false;
}
