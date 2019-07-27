













add_task(function* test_keyword_searc() {
  let uri1 = NetUtil.newURI("http://abc/?search=%s");
  let uri2 = NetUtil.newURI("http://abc/?search=ThisPageIsInHistory");
  yield PlacesTestUtils.addVisits([
    { uri: uri1, title: "Generic page title" },
    { uri: uri2, title: "Generic page title" }
  ]);
  addBookmark({ uri: uri1, title: "Keyword title", keyword: "key"});

  do_print("Plain keyword query");
  yield check_autocomplete({
    search: "key term",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=term"), title: "Keyword title", style: ["keyword"] } ]
  });

  do_print("Multi-word keyword query");
  yield check_autocomplete({
    search: "key multi word",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=multi+word"), title: "Keyword title", style: ["keyword"] } ]
  });

  do_print("Keyword query with +");
  yield check_autocomplete({
    search: "key blocking+",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=blocking%2B"), title: "Keyword title", style: ["keyword"] } ]
  });

  do_print("Unescaped term in query");
  yield check_autocomplete({
    search: "key ユニコード",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=ユニコード"), title: "Keyword title", style: ["keyword"] } ]
  });

  do_print("Keyword that happens to match a page");
  yield check_autocomplete({
    search: "key ThisPageIsInHistory",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=ThisPageIsInHistory"), title: "Generic page title", style: ["bookmark"] } ]
  });

  do_print("Keyword without query (without space)");
  yield check_autocomplete({
    search: "key",
    matches: [ { uri: NetUtil.newURI("http://abc/?search="), title: "Keyword title", style: ["keyword"] } ]
  });

  do_print("Keyword without query (with space)");
  yield check_autocomplete({
    search: "key ",
    matches: [ { uri: NetUtil.newURI("http://abc/?search="), title: "Keyword title", style: ["keyword"] } ]
  });

  
  let uri3 = NetUtil.newURI("http://xyz/?foo=%s");
  yield PlacesTestUtils.addVisits([ { uri: uri3, title: "Generic page title" } ]);
  addBookmark({ uri: uri3, title: "Keyword title", keyword: "key", style: ["keyword"] });

  do_print("Two keywords matched");
  yield check_autocomplete({
    search: "key twoKey",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=twoKey"), title: "Keyword title", style: ["keyword"] },
               { uri: NetUtil.newURI("http://xyz/?foo=twoKey"), title: "Keyword title", style: ["keyword"] } ]
  });

  yield cleanup();
});
