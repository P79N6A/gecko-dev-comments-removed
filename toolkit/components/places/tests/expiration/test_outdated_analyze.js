




const TEST_URL = "http://www.mozilla.org/";

XPCOMUtils.defineLazyServiceGetter(this, "gHistory",
                                   "@mozilla.org/browser/history;1",
                                   "mozIAsyncHistory");










function VisitInfo(aTransitionType, aVisitTime) {
  this.transitionType =
    aTransitionType === undefined ? TRANSITION_LINK : aTransitionType;
  this.visitDate = aVisitTime || Date.now() * 1000;
}

function run_test() {
  do_test_pending();

  
  force_expiration_start();

  
  let places = [];
  for (let i = 0; i < 100; i++) {
    places.push({
      uri: NetUtil.newURI(TEST_URL + i),
      title: "Title" + i,
      visits: [new VisitInfo]
    });
  };
  gHistory.updatePlaces(places);

  
  setInterval(1); 

  Services.obs.addObserver(function observeExpiration(aSubject, aTopic, aData) {
    Services.obs.removeObserver(observeExpiration,
                                PlacesUtils.TOPIC_EXPIRATION_FINISHED);

    
    let stmt = DBConn().createAsyncStatement(
      "SELECT (SELECT COUNT(*) FROM moz_places) - "
      +        "(SELECT SUBSTR(stat,1,LENGTH(stat)-2) FROM sqlite_stat1 "
      +         "WHERE idx = 'moz_places_url_uniqueindex')"
    );
    stmt.executeAsync({
      handleResult: function(aResultSet) {
        let row = aResultSet.getNextRow();
        this._difference = row.getResultByIndex(0);
      },
      handleError: function(aError) {
        do_throw("Unexpected error (" + aError.result + "): " + aError.message);
      },
      handleCompletion: function(aReason) {
        do_check_true(this._difference === 0);
        do_test_finished();
      }
    });
    stmt.finalize();
  }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);
}
