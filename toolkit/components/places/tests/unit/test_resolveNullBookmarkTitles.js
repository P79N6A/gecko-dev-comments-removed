





function run_test() {
  run_next_test();
}

add_test(function test_resolveNullBookmarkTitles() {
  let uri1 = uri("http://foo.tld/");
  let uri2 = uri("https://bar.tld/");

  PlacesTestUtils.addVisits([
    { uri: uri1, title: "foo title" },
    { uri: uri2, title: "bar title" }
  ]).then(function () {
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarksMenuFolderId,
                                         uri1,
                                         PlacesUtils.bookmarks.DEFAULT_INDEX,
                                         null);
    PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarksMenuFolderId,
                                         uri2,
                                         PlacesUtils.bookmarks.DEFAULT_INDEX,
                                         null);

    PlacesUtils.tagging.tagURI(uri1, ["tag 1"]);
    PlacesUtils.tagging.tagURI(uri2, ["tag 2"]);

    let options = PlacesUtils.history.getNewQueryOptions();
    options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
    options.resultType = options.RESULTS_AS_TAG_CONTENTS;

    let query = PlacesUtils.history.getNewQuery();
    
    
    let root = PlacesUtils.history.executeQuery(query, options).root;
    root.containerOpen = true;
    do_check_eq(root.childCount, 2);
    
    
    do_check_eq(root.getChild(0).title, "bar title");
    do_check_eq(root.getChild(1).title, "foo title");
    root.containerOpen = false;

    run_next_test();
  });
});
