







add_task(function* test_default_behavior_host() {
  let uri1 = NetUtil.newURI("http://typed/");
  let uri2 = NetUtil.newURI("http://visited/");
  let uri3 = NetUtil.newURI("http://bookmarked/");
  let uri4 = NetUtil.newURI("http://tpbk/");
  let uri5 = NetUtil.newURI("http://tagged/");

  yield PlacesTestUtils.addVisits([
    { uri: uri1, title: "typed", transition: TRANSITION_TYPED },
    { uri: uri2, title: "visited" },
    { uri: uri4, title: "tpbk", transition: TRANSITION_TYPED },
  ]);
  yield addBookmark( { uri: uri3, title: "bookmarked" } );
  yield addBookmark( { uri: uri4, title: "tpbk" } );
  yield addBookmark( { uri: uri5, title: "title", tags: ["foo"] } );

  yield setFaviconForHref(uri1.spec, "chrome://global/skin/icons/information-16.png");
  yield setFaviconForHref(uri3.spec, "chrome://global/skin/icons/error-16.png");

  
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", true);
  Services.prefs.setBoolPref("browser.urlbar.suggest.history.onlyTyped", false);
  Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", false);

  do_print("Restrict history, common visit, should not autoFill");
  yield check_autocomplete({
    search: "vi",
    matches: [ { uri: uri2, title: "visited" } ],
    autofilled: "vi",
    completed: "vi"
  });

  do_print("Restrict history, typed visit, should autoFill");
  yield check_autocomplete({
    search: "ty",
    matches: [ { uri: uri1, title: "typed", style: [ "autofill" ],
                 icon: "chrome://global/skin/icons/information-16.png" } ],
    autofilled: "typed/",
    completed: "typed/"
  });

  
  do_print("Restrict history, bookmark, should not autoFill");
  yield check_autocomplete({
    search: "bo",
    matches: [ ],
    autofilled: "bo",
    completed: "bo"
  });

  
  do_print("Restrict history, typed bookmark, should autoFill");
  yield check_autocomplete({
    search: "tp",
    matches: [ { uri: uri4, title: "tpbk", style: [ "autofill" ] } ],
    autofilled: "tpbk/",
    completed: "tpbk/"
  });

  Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", false);

  
  
  
  do_print("Restrict history, bookmark, autoFill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "bo",
    matches: [ { uri: uri3, title: "bookmarked", style: [ "bookmark" ], style: [ "autofill" ],
                 icon: "chrome://global/skin/icons/error-16.png" } ],
    autofilled: "bookmarked/",
    completed: "bookmarked/"
  });

  do_print("Restrict history, common visit, autoFill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "vi",
    matches: [ { uri: uri2, title: "visited", style: [ "autofill" ] } ],
    autofilled: "visited/",
    completed: "visited/"
  });

  
  
  Services.prefs.setBoolPref("browser.urlbar.suggest.history.onlyTyped", true);

  
  do_print("Restrict typed, common visit, autoFill.typed = false, should not autoFill");
  yield check_autocomplete({
    search: "vi",
    matches: [ ],
    autofilled: "vi",
    completed: "vi"
  });

  do_print("Restrict typed, typed visit, autofill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "ty",
    matches: [ { uri: uri1, title: "typed", style: [ "autofill" ],
                 icon: "chrome://global/skin/icons/information-16.png"} ],
    autofilled: "typed/",
    completed: "typed/"
  });

  do_print("Restrict typed, bookmark, autofill.typed = false, should not autoFill");
  yield check_autocomplete({
    search: "bo",
    matches: [ ],
    autofilled: "bo",
    completed: "bo"
  });

  do_print("Restrict typed, typed bookmark, autofill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "tp",
    matches: [ { uri: uri4, title: "tpbk", style: [ "autofill" ] } ],
    autofilled: "tpbk/",
    completed: "tpbk/"
  });

  
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", false);
  Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", true);
  Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", true);

  do_print("Restrict bookmarks, common visit, should not autoFill");
  yield check_autocomplete({
    search: "vi",
    matches: [ ],
    autofilled: "vi",
    completed: "vi"
  });

  do_print("Restrict bookmarks, typed visit, should not autoFill");
  yield check_autocomplete({
    search: "ty",
    matches: [ ],
    autofilled: "ty",
    completed: "ty"
  });

  
  do_print("Restrict bookmarks, bookmark, should not autoFill");
  yield check_autocomplete({
    search: "bo",
    matches: [ { uri: uri3, title: "bookmarked", style: [ "bookmark" ],
                 icon: "chrome://global/skin/icons/error-16.png"} ],
    autofilled: "bo",
    completed: "bo"
  });

  
  do_print("Restrict bookmarks, typed bookmark, should autoFill");
  yield check_autocomplete({
    search: "tp",
    matches: [ { uri: uri4, title: "tpbk", style: [ "autofill" ] } ],
    autofilled: "tpbk/",
    completed: "tpbk/"
  });

  Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", false);

  do_print("Restrict bookmarks, bookmark, autofill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "bo",
    matches: [ { uri: uri3, title: "bookmarked", style: [ "autofill" ],
                 icon: "chrome://global/skin/icons/error-16.png" } ],
    autofilled: "bookmarked/",
    completed: "bookmarked/"
  });

  
  do_print("Restrict bookmarks, title, autofill.typed = false, should not autoFill");
  yield check_autocomplete({
    search: "# ta",
    matches: [ ],
    autofilled: "# ta",
    completed: "# ta"
  });

  
  do_print("Restrict bookmarks, tag, autofill.typed = false, should not autoFill");
  yield check_autocomplete({
    search: "+ ta",
    matches: [ { uri: uri5, title: "title", tags: [ "foo" ], style: [ "tag" ] } ],
    autofilled: "+ ta",
    completed: "+ ta"
  });

  yield cleanup();
});

add_task(function* test_default_behavior_url() {
  let uri1 = NetUtil.newURI("http://typed/ty/");
  let uri2 = NetUtil.newURI("http://visited/vi/");
  let uri3 = NetUtil.newURI("http://bookmarked/bo/");
  let uri4 = NetUtil.newURI("http://tpbk/tp/");

  yield PlacesTestUtils.addVisits([
    { uri: uri1, title: "typed", transition: TRANSITION_TYPED },
    { uri: uri2, title: "visited" },
    { uri: uri4, title: "tpbk", transition: TRANSITION_TYPED },
  ]);
  yield addBookmark( { uri: uri3, title: "bookmarked" } );
  yield addBookmark( { uri: uri4, title: "tpbk" } );

  yield setFaviconForHref(uri1.spec, "chrome://global/skin/icons/information-16.png");
  yield setFaviconForHref(uri3.spec, "chrome://global/skin/icons/error-16.png");

  
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", true);
  Services.prefs.setBoolPref("browser.urlbar.suggest.history.onlyTyped", false);
  Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", false);
  Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", true);
  Services.prefs.setBoolPref("browser.urlbar.autoFill.searchEngines", false);

  do_print("URL: Restrict history, common visit, should not autoFill");
  yield check_autocomplete({
    search: "visited/v",
    matches: [ { uri: uri2, title: "visited" } ],
    autofilled: "visited/v",
    completed: "visited/v"
  });

  do_print("URL: Restrict history, typed visit, should autoFill");
  yield check_autocomplete({
    search: "typed/t",
    matches: [ { uri: uri1, title: "typed/ty/", style: [ "autofill" ],
                 icon: "chrome://global/skin/icons/information-16.png"} ],
    autofilled: "typed/ty/",
    completed: "http://typed/ty/"
  });

  
  do_print("URL: Restrict history, bookmark, should not autoFill");
  yield check_autocomplete({
    search: "bookmarked/b",
    matches: [ ],
    autofilled: "bookmarked/b",
    completed: "bookmarked/b"
  });

  
  do_print("URL: Restrict history, typed bookmark, should autoFill");
  yield check_autocomplete({
    search: "tpbk/t",
    matches: [ { uri: uri4, title: "tpbk/tp/", style: [ "autofill" ] } ],
    autofilled: "tpbk/tp/",
    completed: "http://tpbk/tp/"
  });

  
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", false);
  Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", true);

  do_print("URL: Restrict bookmarks, common visit, should not autoFill");
  yield check_autocomplete({
    search: "visited/v",
    matches: [ ],
    autofilled: "visited/v",
    completed: "visited/v"
  });

  do_print("URL: Restrict bookmarks, typed visit, should not autoFill");
  yield check_autocomplete({
    search: "typed/t",
    matches: [ ],
    autofilled: "typed/t",
    completed: "typed/t"
  });

  
  do_print("URL: Restrict bookmarks, bookmark, should not autoFill");
  yield check_autocomplete({
    search: "bookmarked/b",
    matches: [ { uri: uri3, title: "bookmarked", style: [ "bookmark" ],
                 icon: "chrome://global/skin/icons/error-16.png" } ],
    autofilled: "bookmarked/b",
    completed: "bookmarked/b"
  });

  
  do_print("URL: Restrict bookmarks, typed bookmark, should autoFill");
  yield check_autocomplete({
    search: "tpbk/t",
    matches: [ { uri: uri4, title: "tpbk/tp/", style: [ "autofill" ] } ],
    autofilled: "tpbk/tp/",
    completed: "http://tpbk/tp/"
  });

  Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", false);

  do_print("URL: Restrict bookmarks, bookmark, autofill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "bookmarked/b",
    matches: [ { uri: uri3, title: "bookmarked/bo/", style: [ "autofill" ],
                 icon: "chrome://global/skin/icons/error-16.png" } ],
    autofilled: "bookmarked/bo/",
    completed: "http://bookmarked/bo/"
  });

  yield cleanup();
});
