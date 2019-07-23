













































var tests = [
  {
    desc: "Removing a tagged bookmark should cause the tag to be removed.",
    run:   function () {
      print("  Make a bookmark.");
      var bmId = bmsvc.insertBookmark(bmsvc.unfiledBookmarksFolder,
                                      BOOKMARK_URI,
                                      bmsvc.DEFAULT_INDEX,
                                      "test bookmark");
      do_check_true(bmId > 0);

      print("  Tag it up.");
      var tags = ["foo", "bar"];
      tagssvc.tagURI(BOOKMARK_URI, tags);
      ensureTagsExist(tags);

      print("  Remove the bookmark.  The tags should no longer exist.");
      bmsvc.removeItem(bmId);
      ensureTagsExist([]);
    }
  },

  {
    desc: "Removing a folder containing a tagged bookmark should cause the " +
          "tag to be removed.",
    run:   function () {
      print("  Make a folder.");
      var folderId = bmsvc.createFolder(bmsvc.unfiledBookmarksFolder,
                                        "test folder",
                                        bmsvc.DEFAULT_INDEX);
      do_check_true(folderId > 0);

      print("  Stick a bookmark in the folder.");
      var bmId = bmsvc.insertBookmark(folderId,
                                      BOOKMARK_URI,
                                      bmsvc.DEFAULT_INDEX,
                                      "test bookmark");
      do_check_true(bmId > 0);

      print("  Tag the bookmark.");
      var tags = ["foo", "bar"];
      tagssvc.tagURI(BOOKMARK_URI, tags);
      ensureTagsExist(tags);

      print("  Remove the folder.  The tags should no longer exist.");
      bmsvc.removeItem(folderId);
      ensureTagsExist([]);
    }
  }
];

var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
            getService(Ci.nsINavBookmarksService);

var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);

var tagssvc = Cc["@mozilla.org/browser/tagging-service;1"].
              getService(Ci.nsITaggingService);

const BOOKMARK_URI = uri("http://example.com/");









function ensureTagsExist(aTags) {
  var query = histsvc.getNewQuery();
  var opts = histsvc.getNewQueryOptions();
  opts.resultType = opts.RESULTS_AS_TAG_QUERY;
  var resultRoot = histsvc.executeQuery(query, opts).root;

  
  var tags = aTags.slice(0);

  resultRoot.containerOpen = true;

  
  
  do_check_eq(resultRoot.childCount, tags.length);

  
  
  for (let i = 0; i < resultRoot.childCount; i++) {
    var tag = resultRoot.getChild(i).title;
    var indexOfTag = tags.indexOf(tag);
    do_check_true(indexOfTag >= 0);
    tags.splice(indexOfTag, 1);
  }

  resultRoot.containerOpen = false;
}

function run_test()
{
  tests.forEach(function (test) {
    print("Running test: " + test.desc);
    test.run();
  });
}
