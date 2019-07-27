












add_task(function* test_swap_protocol() {
  let uri1 = NetUtil.newURI("http://www.site/");
  let uri2 = NetUtil.newURI("http://site/");
  let uri3 = NetUtil.newURI("ftp://ftp.site/");
  let uri4 = NetUtil.newURI("ftp://site/");
  let uri5 = NetUtil.newURI("https://www.site/");
  let uri6 = NetUtil.newURI("https://site/");
  let uri7 = NetUtil.newURI("http://woohoo/");
  let uri8 = NetUtil.newURI("http://wwwwwwacko/");
  yield promiseAddVisits([ { uri: uri1, title: "title" },
                           { uri: uri2, title: "title" },
                           { uri: uri3, title: "title" },
                           { uri: uri4, title: "title" },
                           { uri: uri5, title: "title" },
                           { uri: uri6, title: "title" },
                           { uri: uri7, title: "title" },
                           { uri: uri8, title: "title" } ]);

  let allMatches = [
    { uri: uri1, title: "title" },
    { uri: uri2, title: "title" },
    { uri: uri3, title: "title" },
    { uri: uri4, title: "title" },
    { uri: uri5, title: "title" },
    { uri: uri6, title: "title" }
  ];

  Services.prefs.setBoolPref("browser.urlbar.autoFill", "false");

  do_log_info("http://www.site matches all site");
  yield check_autocomplete({
    search: "http://www.site",
    matches: allMatches
  });




































































































  yield cleanup();
});
