









































const URIS = [
  "http://a.example1.com/"
, "http://b.example1.com/"
, "http://b.example2.com/"
, "http://c.example3.com/"
];

let expirationObserver = {
  observe: function observe(aSubject, aTopic, aData) {
    print("Finished expiration.");
    Services.obs.removeObserver(expirationObserver,
                                PlacesUtils.TOPIC_EXPIRATION_FINISHED);
  
    let db = PlacesUtils.history
                        .QueryInterface(Ci.nsPIPlacesDatabase)
                        .DBConnection;

    let stmt = db.createStatement(
      "SELECT id FROM moz_places_temp WHERE url = :page_url "
    + "UNION ALL "
    + "SELECT id FROM moz_places WHERE url = :page_url "
    );

    try {
      URIS.forEach(function(aUrl) {
        stmt.params.page_url = aUrl;
        do_check_false(stmt.executeStep());
        stmt.reset();
      });
    } finally {
      stmt.finalize();
    }

    do_test_finished();
  }
}

let timeInMicroseconds = Date.now() * 1000;

function run_test() {
  do_test_pending();

  print("Initialize browserglue before Places");
  Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIBrowserGlue);

  Services.prefs.setBoolPref("privacy.clearOnShutdown.cache", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.cookies", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.offlineApps", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.history", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.downloads", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.cookies", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.formData", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.passwords", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.sessions", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.siteSettings", true);

  Services.prefs.setBoolPref("privacy.sanitize.sanitizeOnShutdown", true);

  print("Add visits.");
  URIS.forEach(function(aUrl) {
    PlacesUtils.history.addVisit(uri(aUrl), timeInMicroseconds++, null,
                                 PlacesUtils.history.TRANSITION_TYPED,
                                 false, 0);
  });

  print("Wait expiration.");
  Services.obs.addObserver(expirationObserver,
                           PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);
  print("Simulate shutdown.");
  PlacesUtils.history.QueryInterface(Ci.nsIObserver)
                     .observe(null, TOPIC_GLOBAL_SHUTDOWN, null);
}
