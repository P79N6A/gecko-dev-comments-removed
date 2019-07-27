













add_task(function* test_keyword_searc() {
  let uri1 = NetUtil.newURI("http://abc/?search=%s");
  let uri2 = NetUtil.newURI("http://abc/?search=ThisPageIsInHistory");
  yield promiseAddVisits([ { uri: uri1, title: "Generic page title" },
                           { uri: uri2, title: "Generic page title" } ]);
  addBookmark({ uri: uri1, title: "Keyword title", keyword: "key"});

  do_log_info("Plain keyword query");
  yield check_autocomplete({
    search: "key term",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=term"), title: "Keyword title", style: ["keyword"] } ]
  });

  do_log_info("Multi-word keyword query");
  yield check_autocomplete({
    search: "key multi word",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=multi+word"), title: "Keyword title", style: ["keyword"] } ]
  });

  do_log_info("Keyword query with +");
  yield check_autocomplete({
    search: "key blocking+",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=blocking%2B"), title: "Keyword title", style: ["keyword"] } ]
  });

  do_log_info("Unescaped term in query");
  yield check_autocomplete({
    search: "key ユニコード",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=ユニコード"), title: "Keyword title", style: ["keyword"] } ]
  });

  do_log_info("Keyword that happens to match a page");
  yield check_autocomplete({
    search: "key ThisPageIsInHistory",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=ThisPageIsInHistory"), title: "Generic page title", style: ["bookmark"] } ]
  });

  do_log_info("Keyword without query (without space)");
  yield check_autocomplete({
    search: "key",
    matches: [ { uri: NetUtil.newURI("http://abc/?search="), title: "Keyword title", style: ["keyword"] } ]
  });

  do_log_info("Keyword without query (with space)");
  yield check_autocomplete({
    search: "key ",
    matches: [ { uri: NetUtil.newURI("http://abc/?search="), title: "Keyword title", style: ["keyword"] } ]
  });

  
  let uri3 = NetUtil.newURI("http://xyz/?foo=%s");
  yield promiseAddVisits([ { uri: uri3, title: "Generic page title" } ]);
  addBookmark({ uri: uri3, title: "Keyword title", keyword: "key", style: ["keyword"] });

  do_log_info("Two keywords matched");
  yield check_autocomplete({
    search: "key twoKey",
    matches: [ { uri: NetUtil.newURI("http://abc/?search=twoKey"), title: "Keyword title", style: ["keyword"] },
               { uri: NetUtil.newURI("http://xyz/?foo=twoKey"), title: "Keyword title", style: ["keyword"] } ]
  });

  yield cleanup();
});
