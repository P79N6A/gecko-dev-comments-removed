















add_task(function* test_tag_match_url() {
  do_print("Make sure tag matches return the right url as well as '+' remain escaped");
  let uri1 = NetUtil.newURI("http://escaped/ユニコード");
  let uri2 = NetUtil.newURI("http://asciiescaped/blocking-firefox3%2B");
  yield PlacesTestUtils.addVisits([
    { uri: uri1, title: "title" },
    { uri: uri2, title: "title" }
  ]);
  addBookmark({ uri: uri1,
                title: "title",
                tags: [ "superTag" ],
                style: [ "bookmark-tag" ] });
  addBookmark({ uri: uri2,
                title: "title",
                tags: [ "superTag" ],
                style: [ "bookmark-tag" ] });
  yield check_autocomplete({
    search: "superTag",
    matches: [ { uri: uri1, title: "title", tags: [ "superTag" ], style: [ "bookmark-tag" ] },
     		   { uri: uri2, title: "title", tags: [ "superTag" ], style: [ "bookmark-tag" ] } ]
  });
  yield cleanup();
});
