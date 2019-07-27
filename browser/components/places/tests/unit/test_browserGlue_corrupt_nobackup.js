










function run_test() {
  
  create_bookmarks_html("bookmarks.glue.html");

  
  remove_all_JSON_backups();

  run_next_test();
}

do_register_cleanup(remove_bookmarks_html);

add_task(function* () {
  
  yield createCorruptDB();

  
  Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIBrowserGlue);

  
  
  Assert.equal(PlacesUtils.history.databaseStatus,
               PlacesUtils.history.DATABASE_STATUS_CORRUPT);

  
  
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
  Assert.equal(bm.title, "example");
});
