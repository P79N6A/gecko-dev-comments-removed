













add_task(function* test_keyword_search() {
  let uri1 = NetUtil.newURI("http://abc/?search=%s");
  let uri2 = NetUtil.newURI("http://abc/?search=ThisPageIsInHistory");
  yield PlacesTestUtils.addVisits([
    { uri: uri1, title: "Generic page title" },
    { uri: uri2, title: "Generic page title" }
  ]);
  yield addBookmark({ uri: uri1, title: "Bookmark title", keyword: "key"});

  do_print("Plain keyword query");
  yield check_autocomplete({
    search: "key term",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=term", input: "key term"}), title: "Generic page title", style: [ "action", "keyword" ] } ]
  });

  do_print("Multi-word keyword query");
  yield check_autocomplete({
    search: "key multi word",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=multi+word", input: "key multi word"}), title: "Generic page title", style: [ "action", "keyword" ] } ]
  });

  do_print("Keyword query with +");
  yield check_autocomplete({
    search: "key blocking+",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=blocking%2B", input: "key blocking+"}), title: "Generic page title", style: [ "action", "keyword" ] } ]
  });

  do_print("Unescaped term in query");
  yield check_autocomplete({
    search: "key ユニコード",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=ユニコード", input: "key ユニコード"}), title: "Generic page title", style: [ "action", "keyword" ] } ]
  });

  do_print("Keyword that happens to match a page");
  yield check_autocomplete({
    search: "key ThisPageIsInHistory",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=ThisPageIsInHistory", input: "key ThisPageIsInHistory"}), title: "Generic page title", style: [ "action", "keyword" ] } ]
  });

  do_print("Keyword without query (without space)");
  yield check_autocomplete({
    search: "key",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=", input: "key"}), title: "Generic page title", style: [ "action", "keyword" ] } ]
  });

  do_print("Keyword without query (with space)");
  yield check_autocomplete({
    search: "key ",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("keyword", {url: "http://abc/?search=", input: "key "}), title: "Generic page title", style: [ "action", "keyword" ] } ]
  });

  yield cleanup();
});
