










function run_test() {
  run_next_test();
}

add_task(function* () {
  remove_bookmarks_html();

  Services.prefs.setBoolPref("browser.bookmarks.autoExportHTML", true);
  do_register_cleanup(() => Services.prefs.clearUserPref("browser.bookmarks.autoExportHTML"));

  
  Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsISupports);

  
  Cc["@mozilla.org/browser/nav-history-service;1"]
    .getService(Ci.nsINavHistoryService);

  Services.obs.addObserver(function observer() {
    Services.obs.removeObserver(observer, "profile-before-change");
    check_bookmarks_html();
  }, "profile-before-change", false);
});
