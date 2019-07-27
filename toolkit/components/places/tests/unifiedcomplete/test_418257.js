









add_task(function* test_javascript_match() {
  let uri1 = NetUtil.newURI("http://page1");
  let uri2 = NetUtil.newURI("http://page2");
  let uri3 = NetUtil.newURI("http://page3");
  let uri4 = NetUtil.newURI("http://page4");
  yield promiseAddVisits([ { uri: uri1, title: "tagged" },
                           { uri: uri2, title: "tagged" },
                           { uri: uri3, title: "tagged" },
                           { uri: uri4, title: "tagged" } ]);
  addBookmark({ uri: uri1,
                title: "tagged",
                tags: [ "tag1" ] });
  addBookmark({ uri: uri2,
                title: "tagged",
                tags: [ "tag1", "tag2" ] });
  addBookmark({ uri: uri3,
                title: "tagged",
                tags: [ "tag1", "tag3" ] });
  addBookmark({ uri: uri4,
                title: "tagged",
                tags: [ "tag1", "tag2", "tag3" ] });

  do_log_info("Make sure tags come back in the title when matching tags");
  yield check_autocomplete({
    search: "page1 tag",
    matches: [ { uri: uri1, title: "tagged", tags: [ "tag1" ], style: [ "tag" ] } ]
  });

  do_log_info("Check tags in title for page2");
  yield check_autocomplete({
    search: "page2 tag",
    matches: [ { uri: uri2, title: "tagged", tags: [ "tag1", "tag2" ], style: [ "tag" ] } ]
  });

  do_log_info("Make sure tags appear even when not matching the tag");
  yield check_autocomplete({
    search: "page3",
    matches: [ { uri: uri3, title: "tagged", tags: [ "tag1", "tag3" ], style: [ "tag" ] } ]
  });

  do_log_info("Multiple tags come in commas for page4");
  yield check_autocomplete({
    search: "page4",
    matches: [ { uri: uri4, title: "tagged", tags: [ "tag1", "tag2", "tag3" ], style: [ "tag" ] } ]
  });

  do_log_info("Extra test just to make sure we match the title");
  yield check_autocomplete({
    search: "tag2",
    matches: [ { uri: uri2, title: "tagged", tags: [ "tag1", "tag2" ], style: [ "tag" ] },
               { uri: uri4, title: "tagged", tags: [ "tag1", "tag2", "tag3" ], style: [ "tag" ] } ]
  });

  yield cleanup();
});
