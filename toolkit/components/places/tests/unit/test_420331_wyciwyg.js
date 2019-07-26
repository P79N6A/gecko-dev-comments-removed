





function run_test()
{
  run_next_test();
}

add_task(function test_execute()
{
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var testURI = uri("wyciwyg://nodontjudgeabookbyitscover");

  try
  {
    yield promiseAddVisits(testURI);
    do_throw("Should have generated an exception.");
  } catch (ex if ex && ex.result == Cr.NS_ERROR_ILLEGAL_VALUE) {
    
  }

  
  histsvc.QueryInterface(Ci.nsIGlobalHistory2);
  placeID = histsvc.addURI(testURI, false, false, null);
  do_check_false(placeID > 0);
});
