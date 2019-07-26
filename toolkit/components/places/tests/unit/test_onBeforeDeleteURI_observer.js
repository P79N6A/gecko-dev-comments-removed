













let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);




function Observer()
{
}
Observer.prototype =
{
  checked: false,
  onBeginUpdateBatch: function () {},
  onEndUpdateBatch: function () {},
  onVisit: function () {},
  onTitleChanged: function () {},
  onBeforeDeleteURI: function (aURI, aGUID)
  {
    this.removedURI = aURI;
    this.removedGUID = aGUID;
    do_check_guid_for_uri(aURI, aGUID);
  },
  onDeleteURI: function (aURI, aGUID)
  {
    do_check_false(this.checked);
    do_check_true(this.removedURI.equals(aURI));
    do_check_eq(this.removedGUID, aGUID);
    this.checked = true;
  },
  onPageChanged: function () {},
  onDeleteVisits: function () {},
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsINavHistoryObserver
  ])
};




function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  
  let testURI = uri("http://mozilla.org");
  yield promiseAddVisits(testURI);

  
  let observer = new Observer();
  hs.addObserver(observer, false);
  hs.removePage(testURI);

  
  do_check_true(observer.checked);
  hs.removeObserver(observer);
});
