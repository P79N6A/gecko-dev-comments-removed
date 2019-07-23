







































var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);
var tagssvc = Cc["@mozilla.org/browser/tagging-service;1"].
              getService(Ci.nsITaggingService);

function run_test() {
  
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();
  query.setFolders([bmsvc.toolbarFolder], 1);
  var result = histsvc.executeQuery(query, options);
  var toolbarNode = result.root;
  toolbarNode.containerOpen = true;

  
  var bookmarkURI = uri("http://foo.com");
  var bookmarkId = bmsvc.insertBookmark(bmsvc.toolbarFolder, bookmarkURI,
                                        bmsvc.DEFAULT_INDEX, "");

  
  var node = toolbarNode.getChild(toolbarNode.childCount-1);
  do_check_eq(node.itemId, bookmarkId);

  
  do_check_eq(node.tags, null); 

  
  tagssvc.tagURI(bookmarkURI, ["foo"]);
  do_check_eq(node.tags, "foo");

  
  tagssvc.tagURI(bookmarkURI, ["bar"]);
  do_check_eq(node.tags, "bar, foo");

  
  tagssvc.untagURI(bookmarkURI, null);
  do_check_eq(node.tags, null); 

  toolbarNode.containerOpen = false;
}
