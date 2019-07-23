







































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
                createInstance().QueryInterface(Ci.nsITaggingService);
} catch(ex) {
  do_throw("Could not get tagging service\n");
}


function run_test() {
  var options = histsvc.getNewQueryOptions();
  var query = histsvc.getNewQuery();

  query.setFolders([bmsvc.tagsFolder], 1);
  var result = histsvc.executeQuery(query, options);
  var tagRoot = result.root;
  tagRoot.containerOpen = true;

  do_check_eq(tagRoot.childCount, 0);

  var uri1 = uri("http://foo.tld/");
  var uri2 = uri("https://bar.tld/");

  
  tagssvc.tagURI(uri1, ["tag 1"]);
  tagssvc.tagURI(uri2, ["tag 1"]);
  do_check_eq(tagRoot.childCount, 1);

  var tag1node = tagRoot.getChild(0)
                        .QueryInterface(Ci.nsINavHistoryContainerResultNode);
  var tag1itemId = tag1node.itemId;

  do_check_eq(tag1node.title, "tag 1");
  tag1node.containerOpen = true;
  do_check_eq(tag1node.childCount, 2);

  
  
  tagssvc.tagURI(uri1, ["tag 1"]);
  do_check_eq(tag1node.childCount, 2);
  tagssvc.tagURI(uri1, [tag1itemId]);
  do_check_eq(tag1node.childCount, 2);
  do_check_eq(tagRoot.childCount, 1);

  
  tagssvc.tagURI(uri1, [tag1itemId, "tag 1", "tag 2", "Tag 1", "Tag 2"]);
  do_check_eq(tagRoot.childCount, 2);
  do_check_eq(tag1node.childCount, 2);

  
  var uri1tags = tagssvc.getTagsForURI(uri1);
  do_check_eq(uri1tags.length, 2);
  do_check_eq(uri1tags[0], "Tag 1");
  do_check_eq(uri1tags[1], "Tag 2");
  var uri2tags = tagssvc.getTagsForURI(uri2);
  do_check_eq(uri2tags.length, 1);
  do_check_eq(uri2tags[0], "Tag 1");

  
  var tag1uris = tagssvc.getURIsForTag("tag 1");
  do_check_eq(tag1uris.length, 2);
  do_check_true(tag1uris[0].equals(uri1));
  do_check_true(tag1uris[1].equals(uri2));

  
  var allTags = tagssvc.allTags;
  do_check_eq(allTags.length, 2);
  do_check_eq(allTags[0], "Tag 1");
  do_check_eq(allTags[1], "Tag 2");

  
  tagssvc.untagURI(uri1, ["tag 1"]);
  do_check_eq(tag1node.childCount, 1);

  
  tagssvc.untagURI(uri2, ["tag 1"]);
  do_check_eq(tagRoot.childCount, 1);

  
  tag1node.containerOpen = false;

  
  
  var tagFolders = [];
  var child = tagRoot.getChild(0);
  var tagId = child.itemId;
  var tagTitle = child.title;

  
  
  var uri3 = uri("http://testuri/3");
  tagssvc.tagURI(uri3, [tagId, "tag 3", "456"]);
  var tags = tagssvc.getTagsForURI(uri3);
  do_check_true(tags.indexOf(tagTitle) != -1);
  do_check_true(tags.indexOf("tag 3") != -1);
  do_check_true(tags.indexOf("456") != -1);

  
  tagssvc.untagURI(uri3, [tagId, "tag 3", "456"]);
  tags = tagssvc.getTagsForURI(uri3);
  do_check_eq(tags.length, 0);

  
  
  
  var uri4 = uri("http://testuri/4");
  tagssvc.tagURI(uri4, [tagId, "tag 3", "456"]);
  tagssvc = null;
  tagssvc = Cc["@mozilla.org/browser/tagging-service;1"].
            getService(Ci.nsITaggingService);
  var uri4Tags = tagssvc.getTagsForURI(uri4);
  do_check_eq(uri4Tags.length, 3);
  do_check_true(uri4Tags.indexOf(tagTitle) != -1);
  do_check_true(uri4Tags.indexOf("tag 3") != -1);
  do_check_true(uri4Tags.indexOf("456") != -1);

  
  tagRoot.containerOpen = false;
}
