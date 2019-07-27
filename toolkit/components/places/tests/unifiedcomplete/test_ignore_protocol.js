







add_task(function* test_escape() {
  let uri1 = NetUtil.newURI("http://site/");
  let uri2 = NetUtil.newURI("http://happytimes/");
  yield promiseAddVisits([ { uri: uri1, title: "title" },
                           { uri: uri2, title: "title" } ]);

  do_log_info("Searching for h matches site and not http://");
  yield check_autocomplete({
    search: "h",
    matches: [ { uri: uri2, title: "title" } ]
  });

  yield cleanup();
});
