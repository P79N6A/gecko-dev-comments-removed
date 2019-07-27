





 




var PU = PlacesUtils;
var hs = PU.history;
var bs = PU.bookmarks;

var tests = [

function() {
  dump("\n\n*** TEST: folder\n");
  
  var folderId = bs.createFolder(bs.toolbarFolder, "folder", bs.DEFAULT_INDEX);

  
  
  bs.createFolder(folderId, "inside folder", bs.DEFAULT_INDEX);
  bs.insertBookmark(folderId, uri("place:sort=1"),
                    bs.DEFAULT_INDEX, "inside query");

  var query = hs.getNewQuery();
  query.setFolders([bs.toolbarFolder], 1);
  var options = hs.getNewQueryOptions();

  dump("Check folder without uri nodes\n");
  check_uri_nodes(query, options, 0);

  dump("Check folder with uri nodes\n");
  
  bs.insertBookmark(folderId, uri("http://www.mozilla.org/"),
                    bs.DEFAULT_INDEX, "bookmark");
  check_uri_nodes(query, options, 1);
},

function() {
  dump("\n\n*** TEST: folder in an excludeItems root\n");
  
  var folderId = bs.createFolder(bs.toolbarFolder, "folder", bs.DEFAULT_INDEX);

  
  
  bs.createFolder(folderId, "inside folder", bs.DEFAULT_INDEX);
  bs.insertBookmark(folderId, uri("place:sort=1"), bs.DEFAULT_INDEX, "inside query");

  var query = hs.getNewQuery();
  query.setFolders([bs.toolbarFolder], 1);
  var options = hs.getNewQueryOptions();
  options.excludeItems = true;

  dump("Check folder without uri nodes\n");
  check_uri_nodes(query, options, 0);

  dump("Check folder with uri nodes\n");
  
  bs.insertBookmark(folderId, uri("http://www.mozilla.org/"),
                    bs.DEFAULT_INDEX, "bookmark");
  check_uri_nodes(query, options, 1);
},

function() {
  dump("\n\n*** TEST: query\n");
  
  bs.insertBookmark(bs.toolbarFolder, uri("place:folder=BOOKMARKS_MENU&sort=1"),
                    bs.DEFAULT_INDEX, "inside query");

  
  
  bs.createFolder(bs.bookmarksMenuFolder, "inside folder", bs.DEFAULT_INDEX);
  bs.insertBookmark(bs.bookmarksMenuFolder, uri("place:sort=1"),
                    bs.DEFAULT_INDEX, "inside query");

  var query = hs.getNewQuery();
  query.setFolders([bs.toolbarFolder], 1);
  var options = hs.getNewQueryOptions();

  dump("Check query without uri nodes\n");
  check_uri_nodes(query, options, 0);

  dump("Check query with uri nodes\n");
  
  bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://www.mozilla.org/"),
                    bs.DEFAULT_INDEX, "bookmark");
  check_uri_nodes(query, options, 1);
},

function() {
  dump("\n\n*** TEST: excludeItems Query\n");
  
  bs.insertBookmark(bs.toolbarFolder, uri("place:folder=BOOKMARKS_MENU&sort=8"),
                    bs.DEFAULT_INDEX, "inside query");

  
  
  bs.createFolder(bs.bookmarksMenuFolder, "inside folder", bs.DEFAULT_INDEX);
  bs.insertBookmark(bs.bookmarksMenuFolder, uri("place:sort=1"),
                    bs.DEFAULT_INDEX, "inside query");

  var query = hs.getNewQuery();
  query.setFolders([bs.toolbarFolder], 1);
  var options = hs.getNewQueryOptions();
  options.excludeItems = true;

  dump("Check folder without uri nodes\n");
  check_uri_nodes(query, options, 0);

  dump("Check folder with uri nodes\n");
  
  bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://www.mozilla.org/"),
                    bs.DEFAULT_INDEX, "bookmark");
  check_uri_nodes(query, options, 1);
},

function() {
  dump("\n\n*** TEST: !expandQueries Query\n");
  
  bs.insertBookmark(bs.toolbarFolder, uri("place:folder=BOOKMARKS_MENU&sort=8"),
                    bs.DEFAULT_INDEX, "inside query");

  
  
  bs.createFolder(bs.bookmarksMenuFolder, "inside folder", bs.DEFAULT_INDEX);
  bs.insertBookmark(bs.bookmarksMenuFolder, uri("place:sort=1"),
                    bs.DEFAULT_INDEX, "inside query");

  var query = hs.getNewQuery();
  query.setFolders([bs.toolbarFolder], 1);
  var options = hs.getNewQueryOptions();
  options.expandQueries = false;

  dump("Check folder without uri nodes\n");
  check_uri_nodes(query, options, 0);

  dump("Check folder with uri nodes\n");
  
  bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://www.mozilla.org/"),
                    bs.DEFAULT_INDEX, "bookmark");
  check_uri_nodes(query, options, 1);
}

];













function check_uri_nodes(aQuery, aOptions, aExpectedURINodes) {
  var result = hs.executeQuery(aQuery, aOptions);
  var root = result.root;
  root.containerOpen = true;
  var node = root.getChild(0);
  do_check_eq(PU.hasChildURIs(node), aExpectedURINodes > 0);
  do_check_eq(PU.getURLsForContainerNode(node).length, aExpectedURINodes);
  root.containerOpen = false;
}

add_task(function* () {
  for (let test of tests) {
    yield PlacesUtils.bookmarks.eraseEverything();
    test();
  }

  
  yield PlacesUtils.bookmarks.eraseEverything();
});
