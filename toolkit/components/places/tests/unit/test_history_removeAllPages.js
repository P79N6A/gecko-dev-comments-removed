





let mDBConn = DBConn();

let historyObserver = {
  onBeginUpdateBatch: function() {},
  onEndUpdateBatch: function() {},
  onVisit: function() {},
  onTitleChanged: function() {},
  onBeforeDeleteURI: function() {},
  onDeleteURI: function(aURI) {},
  onPageChanged: function() {},
  onDeleteVisits: function() {},

  onClearHistory: function() {
    PlacesUtils.history.removeObserver(this, false);

    
    do_check_eq(0, PlacesUtils.history.hasHistoryEntries);

    Services.obs.addObserver(function observeExpiration(aSubject, aTopic, aData)
    {
      Services.obs.removeObserver(observeExpiration, aTopic, false);

      promiseAsyncUpdates().then(function () {
        
        
        
        stmt = mDBConn.createStatement(
          "SELECT h.id FROM moz_places h WHERE h.frecency > 0 ");
        do_check_false(stmt.executeStep());
        stmt.finalize();

        stmt = mDBConn.createStatement(
          "SELECT h.id FROM moz_places h WHERE h.frecency < 0 " +
            "AND EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) LIMIT 1");
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
          "SELECT h.id FROM moz_places h WHERE SUBSTR(h.url, 1, 6) <> 'place:' "+
            "AND NOT EXISTS (SELECT id FROM moz_bookmarks WHERE fk = h.id) LIMIT 1");
        do_check_false(stmt.executeStep());
        stmt.finalize();

        
        stmt = mDBConn.createStatement(
          "SELECT f.id FROM moz_favicons f WHERE NOT EXISTS " +
            "(SELECT id FROM moz_places WHERE favicon_id = f.id) LIMIT 1");
        do_check_false(stmt.executeStep());
        stmt.finalize();

        
        stmt = mDBConn.createStatement(
          "SELECT a.id FROM moz_annos a WHERE NOT EXISTS " +
            "(SELECT id FROM moz_places WHERE id = a.place_id) LIMIT 1");
        do_check_false(stmt.executeStep());
        stmt.finalize();

        
        stmt = mDBConn.createStatement(
          "SELECT i.place_id FROM moz_inputhistory i WHERE NOT EXISTS " +
            "(SELECT id FROM moz_places WHERE id = i.place_id) LIMIT 1");
        do_check_false(stmt.executeStep());
        stmt.finalize();

        
        stmt = mDBConn.createStatement(
          "SELECT h.id FROM moz_places h " +
          "WHERE SUBSTR(h.url, 1, 6) = 'place:' AND h.frecency <> 0 LIMIT 1");
        do_check_false(stmt.executeStep());
        stmt.finalize();

        do_test_finished();
      });
    }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);
  },

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavHistoryObserver,
  ]),
}
PlacesUtils.history.addObserver(historyObserver, false);

function run_test() {
  
  
  
  Services.obs.addObserver(function(aSubject, aTopic, aData) {
    Services.obs.removeObserver(arguments.callee, aTopic, false);
    do_execute_soon(continue_test);
  }, PlacesUtils.TOPIC_INIT_COMPLETE, false);

  do_test_pending();
}

function continue_test() {
  PlacesUtils.history.addVisit(uri("http://typed.mozilla.org/"), Date.now(),
                               null, Ci.nsINavHistoryService.TRANSITION_TYPED,
                               false, 0);
  PlacesUtils.history.addVisit(uri("http://link.mozilla.org/"), Date.now(),
                               null, Ci.nsINavHistoryService.TRANSITION_LINK,
                               false, 0);
  PlacesUtils.history.addVisit(uri("http://download.mozilla.org/"), Date.now(),
                               null, Ci.nsINavHistoryService.TRANSITION_DOWNLOAD,
                               false, 0);
  PlacesUtils.history.addVisit(uri("http://invalid.mozilla.org/"), Date.now(),
                               null, 0, false, 0); 
  PlacesUtils.history.addVisit(uri("http://redir_temp.mozilla.org/"), Date.now(),
                               uri("http://link.mozilla.org/"),
                               Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY,
                               true, 0);
  PlacesUtils.history.addVisit(uri("http://redir_perm.mozilla.org/"), Date.now(),
                               uri("http://link.mozilla.org/"),
                               Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT,
                               true, 0);

  
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

  PlacesUtils.history.addVisit(uri("http://typed.mozilla.org/"), Date.now(),
                               null, PlacesUtils.history.TRANSITION_BOOKMARK,
                               false, 0);

  
  PlacesUtils.history.addVisit(uri("http://frecency.mozilla.org/"), Date.now(),
                               null, Ci.nsINavHistoryService.TRANSITION_LINK,
                               false, 0);
  promiseAsyncUpdates().then(function () PlacesUtils.bhistory.removeAllPages());
}
