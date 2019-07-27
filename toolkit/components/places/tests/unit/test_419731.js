





function run_test() {
  let uri1 = NetUtil.newURI("http://foo.bar/");

  
  let bookmark1id = PlacesUtils.bookmarks
                               .insertBookmark(PlacesUtils.bookmarksMenuFolderId,
                                               uri1,
                                               PlacesUtils.bookmarks.DEFAULT_INDEX,
                                               "title 1");
  let bookmark2id = PlacesUtils.bookmarks
                               .insertBookmark(PlacesUtils.toolbarFolderId,
                                               uri1,
                                               PlacesUtils.bookmarks.DEFAULT_INDEX,
                                               "title 2");
  
  PlacesUtils.tagging.tagURI(uri1, ["foo"]);

  
  let options = PlacesUtils.history.getNewQueryOptions();
  let query = PlacesUtils.history.getNewQuery();
  query.setFolders([PlacesUtils.tagsFolderId], 1);
  let result = PlacesUtils.history.executeQuery(query, options);
  let tagRoot = result.root;
  tagRoot.containerOpen = true;
  let tagNode = tagRoot.getChild(0)
                       .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  let tagItemId = tagNode.itemId;
  tagRoot.containerOpen = false;

  
  PlacesUtils.bookmarks.setItemTitle(bookmark1id, "new title 1");

  
  let bookmark2LastMod = PlacesUtils.bookmarks.getItemLastModified(bookmark2id);
  PlacesUtils.bookmarks.setItemLastModified(bookmark1id, bookmark2LastMod + 1000);

  
  options = PlacesUtils.history.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.resultType = options.RESULTS_AS_TAG_QUERY;

  query = PlacesUtils.history.getNewQuery();
  result = PlacesUtils.history.executeQuery(query, options);
  let root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 1);

  let theTag = root.getChild(0)
                   .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  
  do_check_eq(theTag.title, "foo")
  PlacesUtils.bookmarks.setItemTitle(tagItemId, "bar");

  
  do_check_neq(theTag, root.getChild(0));
  theTag = root.getChild(0)
                   .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(theTag.title, "bar");

  
  theTag.containerOpen = true;
  do_check_eq(theTag.childCount, 1);
  let node = theTag.getChild(0);
  do_check_eq(node.title, "new title 1");
  theTag.containerOpen = false;
  root.containerOpen = false;

  
  PlacesUtils.bookmarks.setItemTitle(bookmark2id, "new title 2");

  
  let bookmark1LastMod = PlacesUtils.bookmarks.getItemLastModified(bookmark1id);
  PlacesUtils.bookmarks.setItemLastModified(bookmark2id, bookmark1LastMod + 1000);

  
  options = PlacesUtils.history.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  options.resultType = options.RESULTS_AS_TAG_CONTENTS;

  query = PlacesUtils.history.getNewQuery();
  query.setFolders([tagItemId], 1);
  result = PlacesUtils.history.executeQuery(query, options);
  root = result.root;

  root.containerOpen = true;
  do_check_eq(root.childCount, 1);
  node = root.getChild(0);
  do_check_eq(node.title, "new title 2");
  root.containerOpen = false;
}
