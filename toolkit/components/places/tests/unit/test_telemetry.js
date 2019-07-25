




Components.utils.import("resource://gre/modules/PlacesDBUtils.jsm");

let histograms = {
  PLACES_PAGES_COUNT: function (val) do_check_eq(val, 1),
  PLACES_BOOKMARKS_COUNT: function (val) do_check_eq(val, 1),
  PLACES_TAGS_COUNT: function (val) do_check_eq(val, 1),
  PLACES_FOLDERS_COUNT: function (val) do_check_eq(val, 1),
  PLACES_KEYWORDS_COUNT: function (val) do_check_eq(val, 1),
  PLACES_SORTED_BOOKMARKS_PERC: function (val) do_check_eq(val, 100),
  PLACES_TAGGED_BOOKMARKS_PERC: function (val) do_check_eq(val, 100),
  PLACES_DATABASE_FILESIZE_MB: function (val) do_check_true(val > 0),
  
  PLACES_DATABASE_JOURNALSIZE_MB: function (val) do_check_true(val >= 0),
  PLACES_DATABASE_PAGESIZE_B: function (val) do_check_eq(val, 32768),
  PLACES_DATABASE_SIZE_PER_PAGE_B: function (val) do_check_true(val > 0),
  PLACES_EXPIRATION_STEPS_TO_CLEAN: function (val) do_check_true(val > 1),
  
  PLACES_IDLE_FRECENCY_DECAY_TIME_MS: function (val) do_check_true(val > 0),
}

function run_test() {
  do_test_pending();

  
  const URI = NetUtil.newURI("http://moz.org/");

  let folderId = PlacesUtils.bookmarks.createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                                    "moz test",
                                                    PlacesUtils.bookmarks.DEFAULT_INDEX);
  let itemId = PlacesUtils.bookmarks.insertBookmark(folderId,
                                                    uri,
                                                    PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                    "moz test");
  PlacesUtils.tagging.tagURI(uri, ["tag"]);
  PlacesUtils.bookmarks.setKeywordForBookmark(itemId, "keyword");

  
  Cc["@mozilla.org/places/categoriesStarter;1"]
    .getService(Ci.nsIObserver)
    .observe(null, "gather-telemetry", null);

  waitForAsyncUpdates(continue_test);
}

function continue_test() {
  
  for (let i = 0; i < 2; i++) {
    PlacesUtils.history.addVisit(NetUtil.newURI("http://" +  i + ".moz.org/"),
                                 Date.now(), null,
                                 PlacesUtils.history.TRANSITION_TYPED, false, 0);
  }
  Services.prefs.setIntPref("places.history.expiration.max_pages", 0);
  let expire = Cc["@mozilla.org/places/expiration;1"].getService(Ci.nsIObserver);
  expire.observe(null, "places-debug-start-expiration", 1);
  expire.observe(null, "places-debug-start-expiration", -1);

  
  


































  
  PlacesUtils.history.QueryInterface(Ci.nsIObserver)
                     .observe(null, "idle-daily", null);

  waitForAsyncUpdates(check_telemetry);
}

function check_telemetry() {
  for (let histogramId in histograms) {
    do_log_info("checking histogram " + histogramId);
    let validate = histograms[histogramId];
    validate(Services.telemetry.getHistogramById(histogramId).snapshot().sum);
  }
  do_test_finished();
}
