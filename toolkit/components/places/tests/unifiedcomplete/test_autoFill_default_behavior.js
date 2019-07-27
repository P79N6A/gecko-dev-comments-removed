







add_task(function* test_default_behavior_host() {
  let uri1 = NetUtil.newURI("http://typed/");
  let uri2 = NetUtil.newURI("http://visited/");
  let uri3 = NetUtil.newURI("http://bookmarked/");
  let uri4 = NetUtil.newURI("http://tpbk/");

  yield promiseAddVisits([
    { uri: uri1, title: "typed", transition: TRANSITION_TYPED },
    { uri: uri2, title: "visited" },
    { uri: uri4, title: "tpbk", transition: TRANSITION_TYPED },
  ]);
  addBookmark( { uri: uri3, title: "bookmarked" } );
  addBookmark( { uri: uri4, title: "tpbk" } );

  
  Services.prefs.setIntPref("browser.urlbar.default.behavior", 1);

  do_log_info("Restrict history, common visit, should not autoFill");
  yield check_autocomplete({
    search: "vi",
    matches: [ { uri: uri2, title: "visited" } ],
    autofilled: "vi",
    completed: "vi"
  });

  do_log_info("Restrict history, typed visit, should autoFill");
  yield check_autocomplete({
    search: "ty",
    matches: [ { uri: uri1, title: "typed" } ],
    autofilled: "typed/",
    completed: "typed/"
  });

  
  do_log_info("Restrict history, bookmark, should not autoFill");
  yield check_autocomplete({
    search: "bo",
    matches: [ ],
    autofilled: "bo",
    completed: "bo"
  });

  
  do_log_info("Restrict history, typed bookmark, should autoFill");
  yield check_autocomplete({
    search: "tp",
    matches: [ { uri: uri4, title: "tpbk" } ],
    autofilled: "tpbk/",
    completed: "tpbk/"
  });

  Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", false);

  
  
  
  do_log_info("Restrict history, bookmark, autoFill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "bo",
    matches: [ { uri: uri3, title: "bookmarked" } ],
    autofilled: "bookmarked/",
    completed: "bookmarked/"
  });

  do_log_info("Restrict history, common visit, autoFill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "vi",
    matches: [ { uri: uri2, title: "visited" } ],
    autofilled: "visited/",
    completed: "visited/"
  });

  
  
  Services.prefs.setIntPref("browser.urlbar.default.behavior", 32);

  
  do_log_info("Restrict typed, common visit, autoFill.typed = false, should not autoFill");
  yield check_autocomplete({
    search: "vi",
    matches: [ ],
    autofilled: "vi",
    completed: "vi"
  });

  do_log_info("Restrict typed, typed visit, autofill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "ty",
    matches: [ { uri: uri1, title: "typed" } ],
    autofilled: "typed/",
    completed: "typed/"
  });

  do_log_info("Restrict typed, bookmark, autofill.typed = false, should not autoFill");
  yield check_autocomplete({
    search: "bo",
    matches: [ ],
    autofilled: "bo",
    completed: "bo"
  });

  do_log_info("Restrict typed, typed bookmark, autofill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "tp",
    matches: [ { uri: uri4, title: "tpbk" } ],
    autofilled: "tpbk/",
    completed: "tpbk/"
  });

  
  Services.prefs.setIntPref("browser.urlbar.default.behavior", 2);
  Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", true);

  do_log_info("Restrict bookmarks, common visit, should not autoFill");
  yield check_autocomplete({
    search: "vi",
    matches: [ ],
    autofilled: "vi",
    completed: "vi"
  });

  do_log_info("Restrict bookmarks, typed visit, should not autoFill");
  yield check_autocomplete({
    search: "ty",
    matches: [ ],
    autofilled: "ty",
    completed: "ty"
  });

  
  do_log_info("Restrict bookmarks, bookmark, should not autoFill");
  yield check_autocomplete({
    search: "bo",
    matches: [ { uri: uri3, title: "bookmarked" } ],
    autofilled: "bo",
    completed: "bo"
  });

  
  do_log_info("Restrict bookmarks, typed bookmark, should autoFill");
  yield check_autocomplete({
    search: "tp",
    matches: [ { uri: uri4, title: "tpbk" } ],
    autofilled: "tpbk/",
    completed: "tpbk/"
  });

  Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", false);

  do_log_info("Restrict bookmarks, bookmark, autofill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "bo",
    matches: [ { uri: uri3, title: "bookmarked" } ],
    autofilled: "bookmarked/",
    completed: "bookmarked/"
  });

  yield cleanup();
});

add_task(function* test_default_behavior_url() {
  let uri1 = NetUtil.newURI("http://typed/ty/");
  let uri2 = NetUtil.newURI("http://visited/vi/");
  let uri3 = NetUtil.newURI("http://bookmarked/bo/");
  let uri4 = NetUtil.newURI("http://tpbk/tp/");

  yield promiseAddVisits([
    { uri: uri1, title: "typed", transition: TRANSITION_TYPED },
    { uri: uri2, title: "visited" },
    { uri: uri4, title: "tpbk", transition: TRANSITION_TYPED },
  ]);
  addBookmark( { uri: uri3, title: "bookmarked" } );
  addBookmark( { uri: uri4, title: "tpbk" } );

  
  Services.prefs.setIntPref("browser.urlbar.default.behavior", 1);
  Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", true);

  do_log_info("URL: Restrict history, common visit, should not autoFill");
  yield check_autocomplete({
    search: "visited/v",
    matches: [ { uri: uri2, title: "visited" } ],
    autofilled: "visited/v",
    completed: "visited/v"
  });

  do_log_info("URL: Restrict history, typed visit, should autoFill");
  yield check_autocomplete({
    search: "typed/t",
    matches: [ { uri: uri1, title: "typed/ty/" } ],
    autofilled: "typed/ty/",
    completed: "http://typed/ty/"
  });

  
  do_log_info("URL: Restrict history, bookmark, should not autoFill");
  yield check_autocomplete({
    search: "bookmarked/b",
    matches: [ ],
    autofilled: "bookmarked/b",
    completed: "bookmarked/b"
  });

  
  do_log_info("URL: Restrict history, typed bookmark, should autoFill");
  yield check_autocomplete({
    search: "tpbk/t",
    matches: [ { uri: uri4, title: "tpbk/tp/" } ],
    autofilled: "tpbk/tp/",
    completed: "http://tpbk/tp/"
  });

  
  Services.prefs.setIntPref("browser.urlbar.default.behavior", 2);

  do_log_info("URL: Restrict bookmarks, common visit, should not autoFill");
  yield check_autocomplete({
    search: "visited/v",
    matches: [ ],
    autofilled: "visited/v",
    completed: "visited/v"
  });

  do_log_info("URL: Restrict bookmarks, typed visit, should not autoFill");
  yield check_autocomplete({
    search: "typed/t",
    matches: [ ],
    autofilled: "typed/t",
    completed: "typed/t"
  });

  
  do_log_info("URL: Restrict bookmarks, bookmark, should not autoFill");
  yield check_autocomplete({
    search: "bookmarked/b",
    matches: [ { uri: uri3, title: "bookmarked" } ],
    autofilled: "bookmarked/b",
    completed: "bookmarked/b"
  });

  
  do_log_info("URL: Restrict bookmarks, typed bookmark, should autoFill");
  yield check_autocomplete({
    search: "tpbk/t",
    matches: [ { uri: uri4, title: "tpbk/tp/" } ],
    autofilled: "tpbk/tp/",
    completed: "http://tpbk/tp/"
  });

  Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", false);

  do_log_info("URL: Restrict bookmarks, bookmark, autofill.typed = false, should autoFill");
  yield check_autocomplete({
    search: "bookmarked/b",
    matches: [ { uri: uri3, title: "bookmarked/bo/" } ],
    autofilled: "bookmarked/bo/",
    completed: "http://bookmarked/bo/"
  });

  yield cleanup();
});
