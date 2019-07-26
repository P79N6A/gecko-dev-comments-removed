


function run_test() {
  run_next_test();
}





add_task(function test_InsertVisitedURIs_UpdateFrecency_and_History_InsertPlace() {
  
  
  
  let uri = NetUtil.newURI("http://example.com/a");
  Cc["@mozilla.org/browser/download-history;1"].
    getService(Ci.nsIDownloadHistory).
    addDownload(uri);
  yield Promise.all([onFrecencyChanged(uri), onFrecencyChanged(uri)]);
});


add_task(function test_nsNavHistory_UpdateFrecency() {
  let bm = PlacesUtils.bookmarks;
  let uri = NetUtil.newURI("http://example.com/b");
  bm.insertBookmark(bm.unfiledBookmarksFolder, uri,
                    Ci.nsINavBookmarksService.DEFAULT_INDEX, "test");
  yield onFrecencyChanged(uri);
});


add_task(function test_nsNavHistory_invalidateFrecencies_somePages() {
  let uri = NetUtil.newURI("http://test-nsNavHistory-invalidateFrecencies-somePages.com/");
  
  
  
  let bm = PlacesUtils.bookmarks;
  bm.insertBookmark(bm.unfiledBookmarksFolder, uri,
                    Ci.nsINavBookmarksService.DEFAULT_INDEX, "test");
  PlacesUtils.history.removePagesFromHost(uri.host, false);
  yield onFrecencyChanged(uri);
});


add_task(function test_nsNavHistory_invalidateFrecencies_allPages() {
  PlacesUtils.history.removeAllPages();
  yield onManyFrecenciesChanged();
});


add_task(function test_nsNavHistory_DecayFrecency_and_nsNavHistory_FixInvalidFrecencies() {
  
  
  
  PlacesUtils.history.QueryInterface(Ci.nsIObserver).
    observe(null, "idle-daily", "");
  yield Promise.all([onManyFrecenciesChanged(), onManyFrecenciesChanged()]);
});

function onFrecencyChanged(expectedURI) {
  let deferred = Promise.defer();
  let obs = new NavHistoryObserver();
  obs.onFrecencyChanged =
    (uri, newFrecency, guid, hidden, visitDate) => {
      PlacesUtils.history.removeObserver(obs);
      do_check_true(!!uri);
      do_check_true(uri.equals(expectedURI));
      deferred.resolve();
    };
  PlacesUtils.history.addObserver(obs, false);
  return deferred.promise;
}

function onManyFrecenciesChanged() {
  let deferred = Promise.defer();
  let obs = new NavHistoryObserver();
  obs.onManyFrecenciesChanged = () => {
    PlacesUtils.history.removeObserver(obs);
    do_check_true(true);
    deferred.resolve();
  };
  PlacesUtils.history.addObserver(obs, false);
  return deferred.promise;
}
