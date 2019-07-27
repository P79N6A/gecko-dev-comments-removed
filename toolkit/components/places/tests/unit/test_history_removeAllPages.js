





let mDBConn = DBConn();

function promiseOnClearHistoryObserved() {
  let deferred = Promise.defer();

  let historyObserver = {
    onBeginUpdateBatch: function() {},
    onEndUpdateBatch: function() {},
    onVisit: function() {},
    onTitleChanged: function() {},
    onDeleteURI: function(aURI) {},
    onPageChanged: function() {},
    onDeleteVisits: function() {},

    onClearHistory: function() {
      PlacesUtils.history.removeObserver(this, false);
      deferred.resolve();
    },

    QueryInterface: XPCOMUtils.generateQI([
      Ci.nsINavHistoryObserver,
    ])
  }
  PlacesUtils.history.addObserver(historyObserver, false);
  return deferred.promise;
}





let promiseInit;

function run_test() {
  
  
  
  promiseInit = promiseTopicObserved(PlacesUtils.TOPIC_INIT_COMPLETE);

  run_next_test();
}

add_task(function test_history_removeAllPages()
{
  yield promiseInit;

  yield promiseAddVisits([
    { uri: uri("http://typed.mozilla.org/"),
      transition: TRANSITION_TYPED },
    { uri: uri("http://link.mozilla.org/"),
      transition: TRANSITION_LINK },
    { uri: uri("http://download.mozilla.org/"),
      transition: TRANSITION_DOWNLOAD },
    { uri: uri("http://redir_temp.mozilla.org/"),
      transition: TRANSITION_REDIRECT_TEMPORARY,
      referrer: "http://link.mozilla.org/"},
    { uri: uri("http://redir_perm.mozilla.org/"),
      transition: TRANSITION_REDIRECT_PERMANENT,
      referrer: "http://link.mozilla.org/"},
  ]);

  
  PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                       uri("place:folder=4"),
                                       PlacesUtils.bookmarks.DEFAULT_INDEX,
                                       "shortcut");

  
  
  
  PlacesUtils.annotations.setPageAnnotation(uri("http://download.mozilla.org/"),
                                            "never", "never", 0,
                                            PlacesUtils.annotations.EXPIRE_NEVER);

  
  
  PlacesUtils.bookmarks.insertBookmark(PlacesUtils.unfiledBookmarksFolderId,
                                       uri("http://typed.mozilla.org/"),
                                       PlacesUtils.bookmarks.DEFAULT_INDEX,
                                       "bookmark");

  yield promiseAddVisits([
    { uri: uri("http://typed.mozilla.org/"),
      transition: TRANSITION_BOOKMARK },
    { uri: uri("http://frecency.mozilla.org/"),
      transition: TRANSITION_LINK },
  ]);
  yield promiseAsyncUpdates();

  
  let promiseWaitClearHistory = promiseOnClearHistoryObserved();
  PlacesUtils.bhistory.removeAllPages();
  yield promiseWaitClearHistory;

  
  do_check_eq(0, PlacesUtils.history.hasHistoryEntries);

  yield promiseTopicObserved(PlacesUtils.TOPIC_EXPIRATION_FINISHED);
  yield promiseAsyncUpdates();

  
  
  
  stmt = mDBConn.createStatement(
    "SELECT h.id FROM moz_places h WHERE h.frecency > 0 ");
  do_check_false(stmt.executeStep());
  stmt.finalize();

  stmt = mDBConn.createStatement(
    `SELECT h.id FROM moz_places h WHERE h.frecency < 0
       AND EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) LIMIT 1`);
  do_check_true(stmt.executeStep());
  stmt.finalize();

  
  stmt = mDBConn.createStatement(
    "SELECT id FROM moz_places WHERE visit_count <> 0 LIMIT 1");
  do_check_false(stmt.executeStep());
  stmt.finalize();

  
  stmt = mDBConn.createStatement(
    "SELECT * FROM (SELECT id FROM moz_historyvisits LIMIT 1)");
  do_check_false(stmt.executeStep());
  stmt.finalize();

  
  stmt = mDBConn.createStatement(
    `SELECT h.id FROM moz_places h WHERE SUBSTR(h.url, 1, 6) <> 'place:'
       AND NOT EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) LIMIT 1`);
  do_check_false(stmt.executeStep());
  stmt.finalize();

  
  stmt = mDBConn.createStatement(
    `SELECT f.id FROM moz_favicons f WHERE NOT EXISTS
       (SELECT id FROM moz_places WHERE favicon_id = f.id) LIMIT 1`);
  do_check_false(stmt.executeStep());
  stmt.finalize();

  
  stmt = mDBConn.createStatement(
    `SELECT a.id FROM moz_annos a WHERE NOT EXISTS
       (SELECT id FROM moz_places WHERE id = a.place_id) LIMIT 1`);
  do_check_false(stmt.executeStep());
  stmt.finalize();

  
  stmt = mDBConn.createStatement(
    `SELECT i.place_id FROM moz_inputhistory i WHERE NOT EXISTS
       (SELECT id FROM moz_places WHERE id = i.place_id) LIMIT 1`);
  do_check_false(stmt.executeStep());
  stmt.finalize();

  
  stmt = mDBConn.createStatement(
    `SELECT h.id FROM moz_places h
     WHERE SUBSTR(h.url, 1, 6) = 'place:' AND h.frecency <> 0 LIMIT 1`);
  do_check_false(stmt.executeStep());
  stmt.finalize();
});
