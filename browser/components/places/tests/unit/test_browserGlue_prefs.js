







const PREF_IMPORT_BOOKMARKS_HTML = "browser.places.importBookmarksHTML";
const PREF_RESTORE_DEFAULT_BOOKMARKS = "browser.bookmarks.restore_default_bookmarks";
const PREF_SMART_BOOKMARKS_VERSION = "browser.places.smartBookmarksVersion";
const PREF_AUTO_EXPORT_HTML = "browser.bookmarks.autoExportHTML";

const TOPIC_BROWSERGLUE_TEST = "browser-glue-test";
const TOPICDATA_FORCE_PLACES_INIT = "force-places-init";

let bg = Cc["@mozilla.org/browser/browserglue;1"].
         getService(Ci.nsIBrowserGlue);

let gTests = [

  
  function test_checkPreferences() {
    
    
    do_check_eq(PlacesUtils.history.databaseStatus,
                PlacesUtils.history.DATABASE_STATUS_CREATE);

    
    Services.obs.addObserver(function(aSubject, aTopic, aData) {
      Services.obs.removeObserver(arguments.callee,
                                  PlacesUtils.TOPIC_INIT_COMPLETE);
      do_execute_soon(function () {
        
        do_check_false(Services.prefs.getBoolPref(PREF_AUTO_EXPORT_HTML));

        try {
          do_check_false(Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
          do_throw("importBookmarksHTML pref should not exist");
        }
        catch(ex) {}

        try {
          do_check_false(Services.prefs.getBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS));
          do_throw("importBookmarksHTML pref should not exist");
        }
        catch(ex) {}

        run_next_test();
      });
    }, PlacesUtils.TOPIC_INIT_COMPLETE, false);
  },

  function test_import()
  {
    do_log_info("Import from bookmarks.html if importBookmarksHTML is true.");

    remove_all_bookmarks();
    
    let itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
    do_check_eq(itemId, -1);

    
    Services.prefs.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);

    
    print("Simulate Places init");
    bg.QueryInterface(Ci.nsIObserver).observe(null,
                                              TOPIC_BROWSERGLUE_TEST,
                                              TOPICDATA_FORCE_PLACES_INIT);

    
    
    itemId = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId,
                                                  SMART_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(PlacesUtils.bookmarks.getItemTitle(itemId), "example");
    
    do_check_false(Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));

    run_next_test();
  },

  function test_import_noSmartBookmarks()
  {
    do_log_info("import from bookmarks.html, but don't create smart bookmarks \
                 if they are disabled");

    remove_all_bookmarks();
    
    let itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
    do_check_eq(itemId, -1);

    
    Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, -1);
    Services.prefs.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);

    
    print("Simulate Places init");
    bg.QueryInterface(Ci.nsIObserver).observe(null,
                                              TOPIC_BROWSERGLUE_TEST,
                                              TOPICDATA_FORCE_PLACES_INIT);

    
    
    itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
    do_check_eq(PlacesUtils.bookmarks.getItemTitle(itemId), "example");
    
    do_check_false(Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));

    run_next_test();
  },

  function test_import_autoExport_updatedSmartBookmarks()
  {
    do_log_info("Import from bookmarks.html, but don't create smart bookmarks \
                 if autoExportHTML is true and they are at latest version");

    remove_all_bookmarks();
    
    let itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
    do_check_eq(itemId, -1);

    
    Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 999);
    Services.prefs.setBoolPref(PREF_AUTO_EXPORT_HTML, true);
    Services.prefs.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);

    
    print("Simulate Places init");
    bg.QueryInterface(Ci.nsIObserver).observe(null,
                                              TOPIC_BROWSERGLUE_TEST,
                                              TOPICDATA_FORCE_PLACES_INIT);

    
    
    itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
    do_check_eq(PlacesUtils.bookmarks.getItemTitle(itemId), "example");
    do_check_false(Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
    
    Services.prefs.setBoolPref(PREF_AUTO_EXPORT_HTML, false);

    run_next_test();
  },

  function test_import_autoExport_oldSmartBookmarks()
  {
    do_log_info("Import from bookmarks.html, and create smart bookmarks if \
                 autoExportHTML is true and they are not at latest version.");

    remove_all_bookmarks();
    
    let itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
    do_check_eq(itemId, -1);

    
    Services.prefs.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 0);
    Services.prefs.setBoolPref(PREF_AUTO_EXPORT_HTML, true);
    Services.prefs.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);

    
    print("Simulate Places init");
    bg.QueryInterface(Ci.nsIObserver).observe(null,
                                              TOPIC_BROWSERGLUE_TEST,
                                              TOPICDATA_FORCE_PLACES_INIT);

    
    
    itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId,
                                           SMART_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(PlacesUtils.bookmarks.getItemTitle(itemId), "example");
    do_check_false(Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
    
    Services.prefs.setBoolPref(PREF_AUTO_EXPORT_HTML, false);

    run_next_test();
  },

  function test_restore()
  {
    do_log_info("restore from default bookmarks.html if \
                 restore_default_bookmarks is true.");

    remove_all_bookmarks();
    
    let itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
    do_check_eq(itemId, -1);

    
    Services.prefs.setBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS, true);

    
    print("Simulate Places init");
    bg.QueryInterface(Ci.nsIObserver).observe(null,
                                              TOPIC_BROWSERGLUE_TEST,
                                              TOPICDATA_FORCE_PLACES_INIT);

    
    itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId,
                                           SMART_BOOKMARKS_ON_TOOLBAR + 1);
    do_check_true(itemId > 0);
    
    do_check_false(Services.prefs.getBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS));

    run_next_test();
  },

  function test_restore_import()
  {
    do_log_info("setting both importBookmarksHTML and \
                 restore_default_bookmarks should restore defaults.");

    remove_all_bookmarks();
    
    let itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId, 0);
    do_check_eq(itemId, -1);

    
    Services.prefs.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);
    Services.prefs.setBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS, true);

    
    print("Simulate Places init");
    bg.QueryInterface(Ci.nsIObserver).observe(null,
                                              TOPIC_BROWSERGLUE_TEST,
                                              TOPICDATA_FORCE_PLACES_INIT);

    
    itemId =
      PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.toolbarFolderId,
                                           SMART_BOOKMARKS_ON_TOOLBAR + 1);
    do_check_true(itemId > 0);
    
    do_check_false(Services.prefs.getBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS));
    do_check_false(Services.prefs.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));

    run_next_test();
  }

];

do_register_cleanup(function () {
  remove_all_bookmarks();
  remove_bookmarks_html();
  remove_all_JSON_backups();
});

function run_test()
{
  
  create_bookmarks_html("bookmarks.glue.html");
  
  create_JSON_backup("bookmarks.glue.json");

  run_next_test();
}
