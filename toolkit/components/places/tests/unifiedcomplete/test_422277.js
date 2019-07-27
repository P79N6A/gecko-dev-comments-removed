








add_task(function* test_javascript_match() {
  do_log_info("Bad escaped uri stays escaped");
  let uri1 = NetUtil.newURI("http://site/%EAid");
  yield promiseAddVisits([ { uri: uri1, title: "title" } ]);
  yield check_autocomplete({
    search: "site",
    matches: [ { uri: uri1, title: "title" } ]
  });
  yield cleanup();
});
