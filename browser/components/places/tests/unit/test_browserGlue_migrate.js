








const PREF_SMART_BOOKMARKS_VERSION = "browser.places.smartBookmarksVersion";

function run_test() {
  
  create_bookmarks_html("bookmarks.glue.html");

  
  clearDB();

  run_next_test();
}

do_register_cleanup(remove_bookmarks_html);

add_task(function* test_migrate_bookmarks() {
  
  
  Assert.equal(PlacesUtils.history.databaseStatus,
               PlacesUtils.history.DATABASE_STATUS_CREATE);

  
  
  let bg = Cc["@mozilla.org/browser/browserglue;1"].getService(Ci.nsIObserver);
  bg.observe(null, "initial-migration-will-import-default-bookmarks", null);

  yield PlacesUtils.bookmarks.insert({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: PlacesUtils.bookmarks.DEFAULT_INDEX,
    type: PlacesUtils.bookmarks.TYPE_BOOKMARK,
    url: "http://mozilla.org/",
    title: "migrated"
  });

  let promise = promiseEndUpdateBatch();
  bg.observe(null, "initial-migration-did-import-default-bookmarks", null);
  yield promise;

  let bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: 0
  });
  yield checkItemHasAnnotation(bm.guid, SMART_BOOKMARKS_ANNO);

  
  bm = yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: SMART_BOOKMARKS_ON_MENU
  });
  Assert.equal(bm.title, "migrated");

  
  Assert.ok(!(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.menuGuid,
    index: SMART_BOOKMARKS_ON_MENU + 1
  })));

  Assert.ok(!(yield PlacesUtils.bookmarks.fetch({
    parentGuid: PlacesUtils.bookmarks.toolbarGuid,
    index: SMART_BOOKMARKS_ON_MENU
  })));
});
