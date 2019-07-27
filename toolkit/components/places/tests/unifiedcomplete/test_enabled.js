








add_task(function* test_enabled() {
  let uri = NetUtil.newURI("http://url/0");
  yield PlacesTestUtils.addVisits([ { uri: uri, title: "title" } ]);

  do_print("plain search");
  yield check_autocomplete({
    search: "url",
    matches: [ { uri: uri, title: "title" } ]
  });

  do_print("search disabled");
  Services.prefs.setBoolPref("browser.urlbar.autocomplete.enabled", false);
  yield check_autocomplete({
    search: "url",
    matches: [ ]
  });

  do_print("resume normal search");
  Services.prefs.setBoolPref("browser.urlbar.autocomplete.enabled", true);
  yield check_autocomplete({
    search: "url",
    matches: [ { uri: uri, title: "title" } ]
  });

  yield cleanup();
});
