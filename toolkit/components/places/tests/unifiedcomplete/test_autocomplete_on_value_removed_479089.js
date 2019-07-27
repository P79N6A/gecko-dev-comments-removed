













add_task(function* test_autocomplete_on_value_removed() {
  let listener = Cc["@mozilla.org/autocomplete/search;1?name=unifiedcomplete"].
                 getService(Components.interfaces.nsIAutoCompleteSimpleResultListener);

  let testUri = NetUtil.newURI("http://foo.mozilla.com/");
  yield PlacesTestUtils.addVisits({
    uri: testUri,
    referrer: uri("http://mozilla.com/")
  });

  let query = PlacesUtils.history.getNewQuery();
  let options = PlacesUtils.history.getNewQueryOptions();
  
  query.uri = testUri;

  let root = PlacesUtils.history.executeQuery(query, options).root;
  root.containerOpen = true;
  Assert.equal(root.childCount, 1);
  
  listener.onValueRemoved(null, testUri.spec, true);
  
  Assert.equal(root.childCount, 0);
  
  root.containerOpen = false;
});
