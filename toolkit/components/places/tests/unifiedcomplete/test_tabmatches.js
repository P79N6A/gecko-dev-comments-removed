





let gTabRestrictChar = "%";

add_task(function* test_tab_matches() {
  Services.search.addEngineWithDetails("MozSearch", "", "", "", "GET",
                                       "http://s.example.com/search");
  let engine = Services.search.getEngineByName("MozSearch");
  Services.search.currentEngine = engine;

  let uri1 = NetUtil.newURI("http://abc.com/");
  let uri2 = NetUtil.newURI("http://xyz.net/");
  let uri3 = NetUtil.newURI("about:mozilla");
  let uri4 = NetUtil.newURI("data:text/html,test");
  yield promiseAddVisits([ { uri: uri1, title: "ABC rocks" },
                           { uri: uri2, title: "xyz.net - we're better than ABC" } ]);
  addOpenPages(uri1, 1);
  
  addOpenPages(uri3, 1);
  addOpenPages(uri4, 1);

  do_log_info("two results, normal result is a tab match");
  yield check_autocomplete({
    search: "abc.com",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("searchengine", {engineName: "MozSearch", input: "abc.com", searchQuery: "abc.com"}), title: "MozSearch" },
               { uri: makeActionURI("switchtab", {url: "http://abc.com/"}), title: "ABC rocks" } ]
  });

  do_log_info("three results, one tab match");
  yield check_autocomplete({
    search: "abc",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("searchengine", {engineName: "MozSearch", input: "abc", searchQuery: "abc"}), title: "MozSearch" },
               { uri: makeActionURI("switchtab", {url: "http://abc.com/"}), title: "ABC rocks" },
               { uri: uri2, title: "xyz.net - we're better than ABC" } ]
  });

  do_log_info("three results, both normal results are tab matches");
  addOpenPages(uri2, 1);
  yield check_autocomplete({
    search: "abc",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("searchengine", {engineName: "MozSearch", input: "abc", searchQuery: "abc"}), title: "MozSearch" },
               { uri: makeActionURI("switchtab", {url: "http://abc.com/"}), title: "ABC rocks" },
               { uri: makeActionURI("switchtab", {url: "http://xyz.net/"}), title: "xyz.net - we're better than ABC" } ]
  });

  do_log_info("three results, both normal results are tab matches, one has multiple tabs");
  addOpenPages(uri2, 5);
  yield check_autocomplete({
    search: "abc",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("searchengine", {engineName: "MozSearch", input: "abc", searchQuery: "abc"}), title: "MozSearch" },
               { uri: makeActionURI("switchtab", {url: "http://abc.com/"}), title: "ABC rocks" },
               { uri: makeActionURI("switchtab", {url: "http://xyz.net/"}), title: "xyz.net - we're better than ABC" } ]
  });

  do_log_info("three results, no tab matches");
  removeOpenPages(uri1, 1);
  removeOpenPages(uri2, 6);
  yield check_autocomplete({
    search: "abc",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("searchengine", {engineName: "MozSearch", input: "abc", searchQuery: "abc"}), title: "MozSearch" },
               { uri: uri1, title: "ABC rocks" },
               { uri: uri2, title: "xyz.net - we're better than ABC" } ]
  });

  do_log_info("tab match search with restriction character");
  addOpenPages(uri1, 1);
  yield check_autocomplete({
    search: gTabRestrictChar + " abc",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("searchengine", {engineName: "MozSearch", input: gTabRestrictChar + " abc", searchQuery: gTabRestrictChar + " abc"}), title: "MozSearch" },
               { uri: makeActionURI("switchtab", {url: "http://abc.com/"}), title: "ABC rocks" } ]
  });

  do_log_info("tab match with not-addable pages");
  yield check_autocomplete({
    search: "mozilla",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("searchengine", {engineName: "MozSearch", input: "mozilla", searchQuery: "mozilla"}), title: "MozSearch" },
               { uri: makeActionURI("switchtab", {url: "about:mozilla"}), title: "about:mozilla" } ]
  });

  do_log_info("tab match with not-addable pages and restriction character");
  yield check_autocomplete({
    search: gTabRestrictChar + " mozilla",
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("searchengine", {engineName: "MozSearch", input: gTabRestrictChar + " mozilla", searchQuery: gTabRestrictChar + " mozilla"}), title: "MozSearch" },
               { uri: makeActionURI("switchtab", {url: "about:mozilla"}), title: "about:mozilla" } ]
  });

  do_log_info("tab match with not-addable pages and only restriction character");
  yield check_autocomplete({
    search: gTabRestrictChar,
    searchParam: "enable-actions",
    matches: [ { uri: makeActionURI("searchengine", {engineName: "MozSearch", input: gTabRestrictChar, searchQuery: gTabRestrictChar}), title: "MozSearch" },
               { uri: makeActionURI("switchtab", {url: "http://abc.com/"}), title: "ABC rocks" },
               { uri: makeActionURI("switchtab", {url: "about:mozilla"}), title: "about:mozilla" },
               { uri: makeActionURI("switchtab", {url: "data:text/html,test"}), title: "data:text/html,test" } ]
  });

  yield cleanup();
});
