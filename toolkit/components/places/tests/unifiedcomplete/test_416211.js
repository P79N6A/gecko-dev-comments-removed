








add_task(function* test_tag_match_has_bookmark_title() {
  do_print("Make sure the tag match gives the bookmark title");
  let uri = NetUtil.newURI("http://theuri/");
  yield PlacesTestUtils.addVisits({ uri: uri, title: "Page title" });
  addBookmark({ uri: uri,
                title: "Bookmark title",
                tags: [ "superTag" ]});
  yield check_autocomplete({
    search: "superTag",
    matches: [ { uri: uri, title: "Bookmark title", tags: [ "superTag" ], style: [ "bookmark-tag" ] } ]
  });
  yield cleanup();
});
