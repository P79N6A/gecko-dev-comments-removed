











































let bg = Cc["@mozilla.org/browser/browserglue;1"].
         getService(Ci.nsIBrowserGlue);


let bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);


let ps = Cc["@mozilla.org/preferences-service;1"].
         getService(Ci.nsIPrefBranch);
let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);

const PREF_AUTO_EXPORT_HTML = "browser.bookmarks.autoExportHTML";

const TOPIC_QUIT_APPLICATION_GRANTED = "quit-application-granted";

let tests = [];



tests.push({
  description: "Export to bookmarks.html if autoExportHTML is true.",
  exec: function() {
    
    do_check_true(bs.getIdForItemAt(bs.toolbarFolder, 0) > 0);
    
    ps.setBoolPref(PREF_AUTO_EXPORT_HTML, true);
    
    os.notifyObservers(null, TOPIC_QUIT_APPLICATION_GRANTED, null);

    
    check_bookmarks_html();
    
    check_JSON_backup();

    
    do_check_true(ps.getBoolPref(PREF_AUTO_EXPORT_HTML));
    
    ps.setBoolPref(PREF_AUTO_EXPORT_HTML, false);

    next_test();
  }
});



tests.push({
  description: "Export to bookmarks.html if autoExportHTML is true and a bookmarks.html exists.",
  exec: function() {
    
    do_check_true(bs.getIdForItemAt(bs.toolbarFolder, 0) > 0);
    
    ps.setBoolPref(PREF_AUTO_EXPORT_HTML, true);
    
    let profileBookmarksHTMLFile = create_bookmarks_html("bookmarks.glue.html");
    
    let lastMod = profileBookmarksHTMLFile.lastModifiedTime;
    let fileSize = profileBookmarksHTMLFile.fileSize;
    
    os.notifyObservers(null, TOPIC_QUIT_APPLICATION_GRANTED, null);

    
    let profileBookmarksHTMLFile = check_bookmarks_html();
    
    
    do_check_neq(profileBookmarksHTMLFile.fileSize, fileSize);

    
    do_check_true(ps.getBoolPref(PREF_AUTO_EXPORT_HTML));
    
    ps.setBoolPref(PREF_AUTO_EXPORT_HTML, false);

    next_test();
  }
});



tests.push({
  description: "Backup to JSON should be a no-op if a backup for today already exists.",
  exec: function() {
    
    do_check_true(bs.getIdForItemAt(bs.toolbarFolder, 0) > 0);
    
    let profileBookmarksJSONFile = create_JSON_backup("bookmarks.glue.json");
    
    let lastMod = profileBookmarksJSONFile.lastModifiedTime;
    let fileSize = profileBookmarksJSONFile.fileSize;
    
    os.notifyObservers(null, TOPIC_QUIT_APPLICATION_GRANTED, null);

    
    do_check_true(profileBookmarksJSONFile.exists());
    do_check_eq(profileBookmarksJSONFile.lastModifiedTime, lastMod);
    do_check_eq(profileBookmarksJSONFile.fileSize, fileSize);

    do_test_finished();
  }
});



function finish_test() {
  do_test_finished();
}

var testIndex = 0;
function next_test() {
  
  remove_bookmarks_html();
  
  remove_all_JSON_backups();

  
  let test = tests.shift();
  dump("\nTEST " + (++testIndex) + ": " + test.description);
  test.exec();
}

function run_test() {
  
  remove_all_bookmarks();

  
  bs.insertBookmark(bs.bookmarksMenuFolder, uri("http://mozilla.org/"),
                    bs.DEFAULT_INDEX, "bookmark-on-menu");
  bs.insertBookmark(bs.toolbarFolder, uri("http://mozilla.org/"),
                    bs.DEFAULT_INDEX, "bookmark-on-toolbar");

  
  do_test_pending();
  next_test();
}
