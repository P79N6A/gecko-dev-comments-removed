














































let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);




function Observer()
{
}
Observer.prototype =
{
  checked: false,
  onBeginUpdateBatch: function() {
  },
  onEndUpdateBatch: function() {
  },
  onVisit: function(aURI, aVisitID, aTime, aSessionId, aReferringId,
                    aTransitionType, _added)
  {
  },
  onTitleChanged: function(aURI, aPageTable)
  {
  },
  onBeforeDeleteURI: function(aURI, aGUID)
  {
    this.removedURI = aURI;
    this.removedGUID = aGUID;
    do_check_guid_for_uri(aURI, aGUID);
  },
  onDeleteURI: function(aURI, aGUID)
  {
    do_check_false(this.checked);
    do_check_true(this.removedURI.equals(aURI));
    do_check_eq(this.removedGUID, aGUID);
    this.checked = true;
  },
  onPageChanged: function(aURI, aWhat, aValue)
  {
  },
  onDeleteVisits: function()
  {
  },
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsINavHistoryObserver) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};




function test_removePage()
{
  
  let testURI = uri("http://mozilla.org");
  hs.addVisit(testURI, Date.now() * 1000, null,
              Ci.nsINavHistoryService.TRANSITION_LINK, false, 0);

  
  let observer = new Observer();
  hs.addObserver(observer, false);
  hs.removePage(testURI);

  
  do_check_true(observer.checked);
  hs.removeObserver(observer);
}




let tests = [
  test_removePage,
];
function run_test()
{
  tests.forEach(function(test) test());
}
