







const PREF_IMPORT_BOOKMARKS_HTML = "browser.places.importBookmarksHTML";
const PREF_RESTORE_DEFAULT_BOOKMARKS = "browser.bookmarks.restore_default_bookmarks";
const PREF_SMART_BOOKMARKS_VERSION = "browser.places.smartBookmarksVersion";
const PREF_AUTO_EXPORT_HTML = "browser.bookmarks.autoExportHTML";

const TOPIC_BROWSERGLUE_TEST = "browser-glue-test";
const TOPICDATA_FORCE_PLACES_INIT = "force-places-init";

let bg = Cc["@mozilla.org/browser/browserglue;1"].
         getService(Ci.nsIObserver);

function run_test() {
  
  create_bookmarks_html("bookmarks.glue.html");

  remove_all_JSON_backups();

  
  create_JSON_backup("bookmarks.glue.json");

  run_next_test();
}

do_register_cleanup(function () {
  remove_bookmarks_html();
  remove_all_JSON_backups();

  return PlacesUtils.bookmarks.eraseEverything();
});

function simulatePlacesInit() {
  do_print("Simulate Places init");
  let promise = waitForImportAndSmartBookmarks();

  
  bg.observe(null, TOPIC_BROWSERGLUE_TEST, TOPICDATA_FORCE_PLACES_INIT);
  return promise;
}

add_task(function* test_checkPreferences() {
  
  
  Assert.equal(PlacesUtils.history.databaseStatus,
               PlacesUtils.history.DATABASE_STATUS_CREATE);

  
  yield promiseTopicObserved("places-browser-init-complete");

  
  Assert.ok(!Services.prefs.getBoolPref(PREF_AUTO_EXPORT_HTML));

  Assert.throws(() => Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
  Assert.throws(() => Services.prefs.getBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS));
});

add_task(function* test_import() {
  do_print("Import from bookmarks.html if importBookmarksHTML is true.");

  yield PlacesUtils.bookmarks.eraseEverything();

  
  Assert.ok(!(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  })));

  
  Services.prefs.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);

  yield simulatePlacesInit();

  
  
  let bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: SMART_BOOKMARKS_ON_TOOLBAR
  });
  Assert.equal(bm.title, "example");

  
  Assert.ok(!Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
});

add_task(function* test_import_noSmartBookmarks() {
  do_print("import from bookmarks.html, but don't create smart bookmarks " +
              "if they are disabled");

  yield PlacesUtils.bookmarks.eraseEverything();

  
  Assert.ok(!(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  })));

  
  Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, -1);
  Services.prefs.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);

  yield simulatePlacesInit();

  
  
  let bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  });
  Assert.equal(bm.title, "example");

  
  Assert.ok(!Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
});

add_task(function* test_import_autoExport_updatedSmartBookmarks() {
  do_print("Import from bookmarks.html, but don't create smart bookmarks " +
              "if autoExportHTML is true and they are at latest version");

  yield PlacesUtils.bookmarks.eraseEverything();

  
  Assert.ok(!(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  })));

  
  Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 999);
  Services.prefs.setBoolPref(PREF_AUTO_EXPORT_HTML, true);
  Services.prefs.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);

  yield simulatePlacesInit();

  
  
  let bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  });
  Assert.equal(bm.title, "example");

  
  Assert.ok(!Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));

  Services.prefs.setBoolPref(PREF_AUTO_EXPORT_HTML, false);
});

add_task(function* test_import_autoExport_oldSmartBookmarks() {
  do_print("Import from bookmarks.html, and create smart bookmarks if " +
              "autoExportHTML is true and they are not at latest version.");

  yield PlacesUtils.bookmarks.eraseEverything();

  
  Assert.ok(!(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  })));

  
  Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 0);
  Services.prefs.setBoolPref(PREF_AUTO_EXPORT_HTML, true);
  Services.prefs.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);

  yield simulatePlacesInit();

  
  
  let bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: SMART_BOOKMARKS_ON_TOOLBAR
  });
  Assert.equal(bm.title, "example");

  
  Assert.ok(!Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));

  Services.prefs.setBoolPref(PREF_AUTO_EXPORT_HTML, false);
});

add_task(function* test_restore() {
  do_print("restore from default bookmarks.html if " +
              "restore_default_bookmarks is true.");

  yield PlacesUtils.bookmarks.eraseEverything();

  
  Assert.ok(!(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  })));

  
  Services.prefs.setBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS, true);

  yield simulatePlacesInit();

  
  Assert.ok(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: SMART_BOOKMARKS_ON_TOOLBAR
  }));

  
  Assert.ok(!Services.prefs.getBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS));
});

add_task(function* test_restore_import() {
  do_print("setting both importBookmarksHTML and " +
              "restore_default_bookmarks should restore defaults.");

  yield PlacesUtils.bookmarks.eraseEverything();

  
  Assert.ok(!(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  })));

  
  Services.prefs.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);
  Services.prefs.setBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS, true);

  yield simulatePlacesInit();

  
  Assert.ok(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: SMART_BOOKMARKS_ON_TOOLBAR
  }));

  
  Assert.ok(!Services.prefs.getBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS));
  Assert.ok(!Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
});
