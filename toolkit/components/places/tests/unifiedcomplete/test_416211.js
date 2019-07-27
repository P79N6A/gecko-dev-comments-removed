








add_task(function* test_tag_match_has_bookmark_title() {
  do_log_info("Make sure the tag match gives the bookmark title");
  let uri = NetUtil.newURI("http://theuri/");
  yield promiseAddVisits({ uri: uri, title: "Page title" });
  addBookmark({ uri: uri,
                title: "Bookmark title",
                tags: [ "superTag" ]});
  yield check_autocomplete({
    search: "superTag",
    matches: [ { uri: uri, title: "Bookmark title", tags: [ "superTag" ], style: [ "tag" ] } ]
  });
  yield cleanup();
});
