









const URIS = [
  "http://a.example1.com/"
, "http://b.example1.com/"
, "http://b.example2.com/"
, "http://c.example3.com/"
];

const TOPIC_CONNECTION_CLOSED = "places-connection-closed";

let EXPECTED_NOTIFICATIONS = [
  "places-shutdown"
, "places-will-close-connection"
, "places-expiration-finished"
, "places-connection-closed"
];

const UNEXPECTED_NOTIFICATIONS = [
  "xpcom-shutdown"
];

const FTP_URL = "ftp://localhost/clearHistoryOnShutdown/";



var formHistoryStartup = Cc["@mozilla.org/satchel/form-history-startup;1"].
                         getService(Ci.nsIObserver);
formHistoryStartup.observe(null, "profile-after-change", null);

let timeInMicroseconds = Date.now() * 1000;

function run_test() {
  run_next_test();
}

add_task(function* test_execute() {
  do_print("Initialize browserglue before Places");

  
  let glue = Cc["@mozilla.org/browser/browserglue;1"].
             getService(Ci.nsIObserver);
  glue.observe(null, "initial-migration-will-import-default-bookmarks", null);

  Services.prefs.setBoolPref("privacy.clearOnShutdown.cache", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.cookies", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.offlineApps", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.history", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.downloads", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.cookies", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.formData", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.sessions", true);
  Services.prefs.setBoolPref("privacy.clearOnShutdown.siteSettings", true);

  Services.prefs.setBoolPref("privacy.sanitize.sanitizeOnShutdown", true);

  do_print("Add visits.");
  for (let aUrl of URIS) {
    yield PlacesTestUtils.addVisits({
      uri: uri(aUrl), visitDate: timeInMicroseconds++,
      transition: PlacesUtils.history.TRANSITION_TYPED
    });
  }
  do_print("Add cache.");
  yield storeCache(FTP_URL, "testData");
});

add_task(function* run_test_continue() {
  do_print("Simulate and wait shutdown.");
  yield shutdownPlaces();

  let stmt = DBConn().createStatement(
    "SELECT id FROM moz_places WHERE url = :page_url "
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

  do_print("Check cache");
  
  let promiseCacheChecked = checkCache(FTP_URL);

  do_print("Shutdown the download manager");
  
  Services.obs.notifyObservers(null, "quit-application", null);

  yield promiseCacheChecked;
});

function storeCache(aURL, aContent) {
  let cache = Services.cache2;
  let storage = cache.diskCacheStorage(LoadContextInfo.default, false);

  return new Promise(resolve => {
    let storeCacheListener = {
      onCacheEntryCheck: function (entry, appcache) {
        return Ci.nsICacheEntryOpenCallback.ENTRY_WANTED;
      },

      onCacheEntryAvailable: function (entry, isnew, appcache, status) {
        do_check_eq(status, Cr.NS_OK);

        entry.setMetaDataElement("servertype", "0");
        var os = entry.openOutputStream(0);

        var written = os.write(aContent, aContent.length);
        if (written != aContent.length) {
          do_throw("os.write has not written all data!\n" +
                   "  Expected: " + written  + "\n" +
                   "  Actual: " + aContent.length + "\n");
        }
        os.close();
        entry.close();
        resolve();
      }
    };

    storage.asyncOpenURI(Services.io.newURI(aURL, null, null), "",
                         Ci.nsICacheStorage.OPEN_NORMALLY,
                         storeCacheListener);
  });
}


function checkCache(aURL) {
  let cache = Services.cache2;
  let storage = cache.diskCacheStorage(LoadContextInfo.default, false);

  return new Promise(resolve => {
    let checkCacheListener = {
      onCacheEntryAvailable: function (entry, isnew, appcache, status) {
        do_check_eq(status, Cr.NS_ERROR_CACHE_KEY_NOT_FOUND);
        resolve();
      }
    };

    storage.asyncOpenURI(Services.io.newURI(aURL, null, null), "",
                        Ci.nsICacheStorage.OPEN_READONLY,
                        checkCacheListener);
  });
}
