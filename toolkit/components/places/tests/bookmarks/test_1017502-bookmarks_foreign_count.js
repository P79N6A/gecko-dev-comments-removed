










const T_URI = NetUtil.newURI("https://www.mozilla.org/firefox/nightly/firstrun/");

function* getForeignCountForURL(conn, url){
  let url = url instanceof Ci.nsIURI ? url.spec : url;
  let rows = yield conn.executeCached(
      "SELECT foreign_count FROM moz_places WHERE url = :t_url ", { t_url: url });
  return rows[0].getResultByName("foreign_count");
}

function run_test() {
  run_next_test();
}

add_task(function* add_remove_change_bookmark_test() {
  let conn = yield PlacesUtils.promiseDBConnection();

  
  yield promiseAddVisits(T_URI);
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 0);

  
  let id1 = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                    T_URI, PlacesUtils.bookmarks.DEFAULT_INDEX, "First Run");
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 1);

  
  let id2 = PlacesUtils.bookmarks.insertBookmark(PlacesUtils.bookmarksMenuFolderId,
                      T_URI, PlacesUtils.bookmarks.DEFAULT_INDEX, "First Run");
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 2);

  
  PlacesUtils.bookmarks.removeItem(id2);
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 1);

  
  const URI2 = NetUtil.newURI("http://www.mozilla.org");
  PlacesUtils.bookmarks.changeBookmarkURI(id1, URI2);
  
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 0);
  
  Assert.equal((yield getForeignCountForURL(conn, URI2)), 1);

  
  let id = PlacesUtils.bookmarks.getBookmarkIdsForURI(URI2);
  PlacesUtils.bookmarks.removeItem(id);
  Assert.equal((yield getForeignCountForURL(conn, URI2)), 0);

});

add_task(function* maintenance_foreign_count_test() {
  let conn = yield PlacesUtils.promiseDBConnection();

  
  yield promiseAddVisits(T_URI);

  
  let deferred = Promise.defer();
  let stmt = DBConn().createAsyncStatement(
    "UPDATE moz_places SET foreign_count = 10 WHERE url = :t_url ");
  stmt.params.t_url = T_URI.spec;
  stmt.executeAsync({
    handleCompletion: function(){
      deferred.resolve();
    }
  });
  stmt.finalize();
  yield deferred.promise;
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 10);

  
  Components.utils.import("resource://gre/modules/PlacesDBUtils.jsm");
  let promiseMaintenanceFinished =
    promiseTopicObserved("places-maintenance-finished");
  PlacesDBUtils.maintenanceOnIdle();
  yield promiseMaintenanceFinished;

  
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 0);
});

add_task(function* add_remove_tags_test(){
  let conn = yield PlacesUtils.promiseDBConnection();

  yield promiseAddVisits(T_URI);
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 0);

  
  PlacesUtils.tagging.tagURI(T_URI, ["test tag"]);
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 1);

  
  PlacesUtils.tagging.tagURI(T_URI, ["one", "two"]);
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 3);

  
  PlacesUtils.tagging.untagURI(T_URI, ["test tag", "one", "two"]);
  Assert.equal((yield getForeignCountForURL(conn, T_URI)), 0);
});