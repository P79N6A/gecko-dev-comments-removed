













add_task(function* test_keyword_search() {
  let uri1 = NetUtil.newURI("http://abc/?search=%s");
  let uri2 = NetUtil.newURI("http://abc/?search=ThisPageIsInHistory");
  yield promiseAddVisits([ { uri: uri1, title: "Generic page title" },
                           { uri: uri2, title: "Generic page title" } ]);
  addBookmark({ uri: uri1, title: "Keyword title", keyword: "key"});

  do_log_info("Plain keyword query");
  yield check_autocomplete({
    search: "key term",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=term", input: "key term"}), title: "Keyword title" } ]
  });

  do_log_info("Multi-word keyword query");
  yield check_autocomplete({
    search: "key multi word",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=multi+word", input: "key multi word"}), title: "Keyword title" } ]
  });

  do_log_info("Keyword query with +");
  yield check_autocomplete({
    search: "key blocking+",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=blocking%2B", input: "key blocking+"}), title: "Keyword title" } ]
  });

  do_log_info("Unescaped term in query");
  yield check_autocomplete({
    search: "key ユニコード",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=ユニコード", input: "key ユニコード"}), title: "Keyword title" } ]
  });

  do_log_info("Keyword that happens to match a page");
  yield check_autocomplete({
    search: "key ThisPageIsInHistory",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=ThisPageIsInHistory", input: "key ThisPageIsInHistory"}), title: "Keyword title" } ]
  });

  do_log_info("Keyword without query (without space)");
  yield check_autocomplete({
    search: "key",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=", input: "key"}), title: "Keyword title" } ]
  });

  do_log_info("Keyword without query (with space)");
  yield check_autocomplete({
    search: "key ",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=", input: "key "}), title: "Keyword title" } ]
  });

  
  let uri3 = NetUtil.newURI("http://xyz/?foo=%s");
  yield promiseAddVisits([ { uri: uri3, title: "Generic page title" } ]);
  addBookmark({ uri: uri3, title: "Keyword title", keyword: "key"});

  do_log_info("Two keywords matched");
  yield check_autocomplete({
    search: "key twoKey",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=twoKey", input: "key twoKey"}), title: "Keyword title" },
               { uri: makeActionURI("keyword", {url: "http://xyz/?foo=twoKey", input: "key twoKey"}), title: "Keyword title" } ]
  });

  yield cleanup();
});
