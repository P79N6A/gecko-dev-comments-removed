







































try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
} catch(ex) {
  do_throw("Could not get history service\n");
}


try {
  var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
              getService(Ci.nsINavBookmarksService);
}
catch(ex) {
  do_throw("Could not get the nav-bookmarks-service\n");
}


try {
  var tagssvc = Cc["@mozilla.org/browser/tagging-service;1"].
                getService(Ci.nsITaggingService);
} catch(ex) {
  do_throw("Could not get tagging service\n");
}


function run_test() {
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();

  query.setFolders([bmsvc.tagRoot], 1);
  var result = histsvc.executeQuery(query, options);
  var tagRoot = result.root;
  tagRoot.containerOpen = true;

  do_check_eq(tagRoot.childCount, 0);

  var uri1 = uri("http://foo.tld/");
  var uri2 = uri("https://bar.tld/");

  
  tagssvc.tagURI(uri1, ["tag 1"], 1);
  tagssvc.tagURI(uri2, ["tag 1"], 1);
  do_check_eq(tagRoot.childCount, 1);

  var tag1node = tagRoot.getChild(0)
                        .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  do_check_eq(tag1node.title, "tag 1");
  tag1node.containerOpen = true;
  do_check_eq(tag1node.childCount, 2);

  
  tagssvc.tagURI(uri1, ["tag 1"], 1);
  do_check_eq(tag1node.childCount, 2);

  
  do_check_eq(tagRoot.childCount, 1);
  tagssvc.tagURI(uri1, ["tag 1", "tag 2"], 2);
  do_check_eq(tagRoot.childCount, 2);

  
  var uri1tags = tagssvc.getTagsForURI(uri1, {});
  do_check_eq(uri1tags.length, 2);
  do_check_eq(uri1tags[0], "tag 2");
  do_check_eq(uri1tags[1], "tag 1");
  var uri2tags = tagssvc.getTagsForURI(uri2, {});
  do_check_eq(uri2tags.length, 1);
  do_check_eq(uri2tags[0], "tag 1");

  
  var tag1uris = tagssvc.getURIsForTag("tag 1");
  do_check_eq(tag1uris.length, 2);
  do_check_true(tag1uris[0].equals(uri1));
  do_check_true(tag1uris[1].equals(uri2));

  tagssvc.untagURI(uri1, ["tag 1"], 1);
  do_check_eq(tag1node.childCount, 1);

  
  tagssvc.untagURI(uri2, ["tag 1"], 1);
  do_check_eq(tagRoot.childCount, 1);
}
