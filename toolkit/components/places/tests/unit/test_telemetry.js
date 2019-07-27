




Components.utils.import("resource://gre/modules/PlacesDBUtils.jsm");

let histograms = {
  PLACES_PAGES_COUNT: function (val) do_check_eq(val, 1),
  PLACES_BOOKMARKS_COUNT: function (val) do_check_eq(val, 1),
  PLACES_TAGS_COUNT: function (val) do_check_eq(val, 1),
  PLACES_KEYWORDS_COUNT: function (val) do_check_eq(val, 1),
  PLACES_SORTED_BOOKMARKS_PERC: function (val) do_check_eq(val, 100),
  PLACES_TAGGED_BOOKMARKS_PERC: function (val) do_check_eq(val, 100),
  PLACES_DATABASE_FILESIZE_MB: function (val) do_check_true(val > 0),
  PLACES_DATABASE_PAGESIZE_B: function (val) do_check_eq(val, 32768),
  PLACES_DATABASE_SIZE_PER_PAGE_B: function (val) do_check_true(val > 0),
  PLACES_EXPIRATION_STEPS_TO_CLEAN2: function (val) do_check_true(val > 1),
  
  PLACES_IDLE_FRECENCY_DECAY_TIME_MS: function (val) do_check_true(val > 0),
  PLACES_IDLE_MAINTENANCE_TIME_MS: function (val) do_check_true(val > 0),
  PLACES_ANNOS_BOOKMARKS_COUNT: function (val) do_check_eq(val, 1),
  PLACES_ANNOS_PAGES_COUNT: function (val) do_check_eq(val, 1),
  PLACES_MAINTENANCE_DAYSFROMLAST: function (val) do_check_true(val >= 0),
}

function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  
  let uri = NetUtil.newURI("http://moz.org/");

  let folderId = PlacesUtils.bookmarks.createFolder(PlacesUtils.unfiledBookmarksFolderId,
                                                    "moz test",
                                                    PlacesUtils.bookmarks.DEFAULT_INDEX);
  let itemId = PlacesUtils.bookmarks.insertBookmark(folderId,
                                                    uri,
                                                    PlacesUtils.bookmarks.DEFAULT_INDEX,
                                                    "moz test");
  PlacesUtils.tagging.tagURI(uri, ["tag"]);
  yield PlacesUtils.keywords.insert({ url: uri.spec, keyword: "keyword"});

  
  let content = "";
  while (content.length < 1024) {
    content += "0";
  }
  PlacesUtils.annotations.setItemAnnotation(itemId, "test-anno", content, 0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);
  PlacesUtils.annotations.setPageAnnotation(uri, "test-anno", content, 0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);

  
  Cc["@mozilla.org/places/categoriesStarter;1"]
    .getService(Ci.nsIObserver)
    .observe(null, "gather-telemetry", null);

  yield PlacesTestUtils.promiseAsyncUpdates();

  
  for (let i = 0; i < 2; i++) {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://" +  i + ".moz.org/"),
      visitDate: Date.now() 
    });
  }
  Services.prefs.setIntPref("places.history.expiration.max_pages", 0);
  let expire = Cc["@mozilla.org/places/expiration;1"].getService(Ci.nsIObserver);
  expire.observe(null, "places-debug-start-expiration", 1);
  expire.observe(null, "places-debug-start-expiration", -1);

  
  


































  
  PlacesUtils.history.QueryInterface(Ci.nsIObserver)
                     .observe(null, "idle-daily", null);
  PlacesDBUtils.maintenanceOnIdle();

  yield promiseTopicObserved("places-maintenance-finished");

  for (let histogramId in histograms) {
    do_print("checking histogram " + histogramId);
    let validate = histograms[histogramId];
    let snapshot = Services.telemetry.getHistogramById(histogramId).snapshot();
    validate(snapshot.sum);
    do_check_true(snapshot.counts.reduce(function(a, b) a + b) > 0);
  }
});

add_test(function test_healthreport_callback() {
  Services.prefs.clearUserPref("places.database.lastMaintenance");
  PlacesDBUtils.telemetry(null, function onResult(data) {
    do_check_neq(data, null);

    do_check_eq(Object.keys(data).length, 2);
    do_check_eq(data.PLACES_PAGES_COUNT, 1);
    do_check_eq(data.PLACES_BOOKMARKS_COUNT, 1);

    do_check_true(!Services.prefs.prefHasUserValue("places.database.lastMaintenance"));
    run_next_test();
  });
});

