










function run_test() {
  
  create_bookmarks_html("bookmarks.glue.html");

  remove_all_JSON_backups();

  
  create_JSON_backup("bookmarks.glue.json");

  
  clearDB();

  run_next_test();
}

do_register_cleanup(function () {
  remove_bookmarks_html();
  remove_all_JSON_backups();
  return PlacesUtils.bookmarks.eraseEverything();
});

add_task(function* test_main() {
  
  Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIBrowserGlue);

  
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);

  
  
  Assert.equal(hs.databaseStatus, hs.DATABASE_STATUS_CREATE);

  
  
  yield promiseEndUpdateBatch();

  let bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  });
  yield checkItemHasAnnotation(bm.guid, SMART_BOOKMARKS_ANNO);

  
  
  bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: SMART_BOOKMARKS_ON_TOOLBAR
  });
  Assert.equal(bm.title, "examplejson");
});
