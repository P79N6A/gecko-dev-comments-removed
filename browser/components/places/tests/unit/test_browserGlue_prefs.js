











































let bg = Cc["@mozilla.org/browser/browserglue;1"].
         getService(Ci.nsIBrowserGlue);


let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);


let ps = Cc["@mozilla.org/preferences-service;1"].
         getService(Ci.nsIPrefBranch);
let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

const PREF_IMPORT_BOOKMARKS_HTML = "browser.places.importBookmarksHTML";
const PREF_RESTORE_DEFAULT_BOOKMARKS = "browser.bookmarks.restore_default_bookmarks";
const PREF_SMART_BOOKMARKS_VERSION = "browser.places.smartBookmarksVersion";
const PREF_AUTO_EXPORT_HTML = "browser.bookmarks.autoExportHTML";

const TOPIC_PLACES_INIT_COMPLETE = "places-init-complete";

let tests = [];



tests.push({
  description: "Import from bookmarks.html if importBookmarksHTML is true.",
  exec: function() {
    
    do_check_eq(bs.getIdForItemAt(bs.toolbarFolder, 0), -1);
    
    ps.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);
    
    os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

    
    
    let itemId = bs.getIdForItemAt(bs.toolbarFolder,
                                   SMART_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(bs.getItemTitle(itemId), "example");
    
    do_check_false(ps.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));

    next_test();
  }
});



tests.push({
  description: "import from bookmarks.html, but don't create smart bookmarks if they are disabled",
  exec: function() {
    
    do_check_eq(bs.getIdForItemAt(bs.toolbarFolder, 0), -1);
    
    ps.setIntPref(PREF_SMART_BOOKMARKS_VERSION, -1);
    ps.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);
    
    os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

    
    
    let itemId = bs.getIdForItemAt(bs.toolbarFolder, 0);
    do_check_eq(bs.getItemTitle(itemId), "example");
    
    do_check_false(ps.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));

    next_test();
  }
});



tests.push({
  description: "Import from bookmarks.html, but don't create smart bookmarks if autoExportHTML is true and they are at latest version",
  exec: function() {
    
    do_check_eq(bs.getIdForItemAt(bs.toolbarFolder, 0), -1);
    
    ps.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 999);
    ps.setBoolPref(PREF_AUTO_EXPORT_HTML, true);
    ps.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);
    
    os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

    
    
    let itemId = bs.getIdForItemAt(bs.toolbarFolder, 0);
    do_check_eq(bs.getItemTitle(itemId), "example");
    do_check_false(ps.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
    
    ps.setBoolPref(PREF_AUTO_EXPORT_HTML, false);

    next_test();
  }
});



tests.push({
  description: "Import from bookmarks.html, and create smart bookmarks if autoExportHTML is true and they are not at latest version.",
  exec: function() {
    
    do_check_eq(bs.getIdForItemAt(bs.toolbarFolder, 0), -1);
    
    ps.setIntPref(PREF_SMART_BOOKMARKS_VERSION, 0);
    ps.setBoolPref(PREF_AUTO_EXPORT_HTML, true);
    ps.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);
    
    os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

    
    
    let itemId = bs.getIdForItemAt(bs.toolbarFolder, SMART_BOOKMARKS_ON_TOOLBAR);
    do_check_eq(bs.getItemTitle(itemId), "example");
    do_check_false(ps.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));
    
    ps.setBoolPref(PREF_AUTO_EXPORT_HTML, false);

    next_test();
  }
});


tests.push({
  description: "restore from default bookmarks.html if restore_default_bookmarks is true.",
  exec: function() {
    
    do_check_eq(bs.getIdForItemAt(bs.toolbarFolder, 0), -1);
    
    ps.setBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS, true);
    
    os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

    
    let itemId = bs.getIdForItemAt(bs.toolbarFolder, SMART_BOOKMARKS_ON_TOOLBAR + 1);
    do_check_true(itemId > 0);
    
    do_check_false(ps.getBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS));

    next_test();
  }
});



tests.push({
  description: "setting both importBookmarksHTML and restore_default_bookmarks should restore defaults.",
  exec: function() {
    
    do_check_eq(bs.getIdForItemAt(bs.toolbarFolder, 0), -1);
    
    ps.setBoolPref(PREF_IMPORT_BOOKMARKS_HTML, true);
    ps.setBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS, true);
    
    os.notifyObservers(null, TOPIC_PLACES_INIT_COMPLETE, null);

    
    let itemId = bs.getIdForItemAt(bs.toolbarFolder, SMART_BOOKMARKS_ON_TOOLBAR + 1);
    do_check_true(itemId > 0);
    
    do_check_false(ps.getBoolPref(PREF_RESTORE_DEFAULT_BOOKMARKS));
    do_check_false(ps.getBoolPref(PREF_IMPORT_BOOKMARKS_HTML));

    do_test_finished();
  }
});



function finish_test() {
  
  remove_all_bookmarks();

  do_test_finished();
}

var testIndex = 0;
function next_test() {
  
  remove_all_bookmarks();

  
  
  os.addObserver(bg, TOPIC_PLACES_INIT_COMPLETE, false);

  
  let test = tests.shift();
  print("\nTEST " + (++testIndex) + ": " + test.description);
  test.exec();
}

function run_test() {
  
  create_bookmarks_html("bookmarks.glue.html");

  
  create_JSON_backup("bookmarks.glue.json");

  
  do_test_pending();
  next_test();
}
