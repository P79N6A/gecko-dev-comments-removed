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

add_task(function* test_sync_enabled() {
  
  Cc["@mozilla.org/autocomplete/search;1?name=unifiedcomplete"]
    .getService(Ci.mozIPlacesAutoComplete);

  let types = [ "history", "bookmark", "openpage", "searches" ];

  
  
  for (let type of types) {
    Services.prefs.setBoolPref("browser.urlbar.suggest." + type, true);
  }
  Assert.equal(Services.prefs.getBoolPref("browser.urlbar.autocomplete.enabled"), true);

  
  Services.prefs.setBoolPref("browser.urlbar.autocomplete.enabled", false);
  for (let type of types) {
    Assert.equal(Services.prefs.getBoolPref("browser.urlbar.suggest." + type), false);
  }

  
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", true);
  for (let type of types.filter(t => t != "history")) {
    Assert.equal(Services.prefs.getBoolPref("browser.urlbar.suggest." + type), false);
  }
  Assert.equal(Services.prefs.getBoolPref("browser.urlbar.autocomplete.enabled"), true);

  
  
  Services.prefs.setBoolPref("browser.urlbar.autocomplete.enabled", false);
  Services.prefs.setBoolPref("browser.urlbar.autocomplete.enabled", true);
  for (let type of types.filter(t => t != "history")) {
    Assert.equal(Services.prefs.getBoolPref("browser.urlbar.suggest." + type), true);
  }
});
