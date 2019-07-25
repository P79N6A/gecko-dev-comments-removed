










const FAVICON_URI = NetUtil.newURI(do_get_file("favicon-normal16.png"));
const LAST_PAGE_URI = NetUtil.newURI("http://example.com/verification");
const LAST_FAVICON_URI = NetUtil.newURI(do_get_file("favicon-normal32.png"));




function run_test()
{
  
  
  waitForFaviconChanged(LAST_PAGE_URI, LAST_FAVICON_URI,
                        function final_callback() {
    
    let resultCount = 0;
    let stmt = DBConn().createAsyncStatement("SELECT url FROM moz_favicons");
    stmt.executeAsync({
      handleResult: function final_handleResult(aResultSet) {
        for (let row; (row = aResultSet.getNextRow()); ) {
          do_check_eq(LAST_FAVICON_URI.spec, row.getResultByIndex(0));
          resultCount++;
        }
      },
      handleError: function final_handleError(aError) {
        do_throw("Unexpected error (" + aError.result + "): " + aError.message);
      },
      handleCompletion: function final_handleCompletion(aReason) {
        do_check_eq(Ci.mozIStorageStatementCallback.REASON_FINISHED, aReason);
        do_check_eq(1, resultCount);
        run_next_test();
      }
    });
    stmt.finalize();
  });

  run_next_test();
}

add_test(function test_null_pageURI()
{
  try {
    PlacesUtils.favicons.setAndFetchFaviconForPage(
                         null,
                         FAVICON_URI, true);
    do_throw("Exception expected because aPageURI is null.");
  } catch (ex) {
    
  }

  run_next_test();
});

add_test(function test_null_faviconURI()
{
  try {
    PlacesUtils.favicons.setAndFetchFaviconForPage(
                         NetUtil.newURI("http://example.com/null_faviconURI"),
                         null, true);
    do_throw("Exception expected because aFaviconURI is null.");
  } catch (ex) {
    
  }

  run_next_test();
});

add_test(function test_aboutURI()
{
  PlacesUtils.favicons.setAndFetchFaviconForPage(
                       NetUtil.newURI("about:testAboutURI"),
                       FAVICON_URI, true);

  run_next_test();
});

add_test(function test_privateBrowsing_nonBookmarkedURI()
{
  if (!("@mozilla.org/privatebrowsing;1" in Cc)) {
    run_next_test();
    return;
  }

  let pb = Cc["@mozilla.org/privatebrowsing;1"]
           .getService(Ci.nsIPrivateBrowsingService);
  Services.prefs.setBoolPref("browser.privatebrowsing.keep_current_session",
                             true);
  pb.privateBrowsingEnabled = true;

  PlacesUtils.favicons.setAndFetchFaviconForPage(
                       NetUtil.newURI("http://example.com/privateBrowsing"),
                       FAVICON_URI, true);

  
  
  pb.privateBrowsingEnabled = false;
  Services.prefs.clearUserPref("browser.privatebrowsing.keep_current_session");
  run_next_test();
});

add_test(function test_disabledHistory()
{
  Services.prefs.setBoolPref("places.history.enabled", false);

  PlacesUtils.favicons.setAndFetchFaviconForPage(
                       NetUtil.newURI("http://example.com/disabledHistory"),
                       FAVICON_URI, true);

  
  
  
  Services.prefs.setBoolPref("places.history.enabled", true);
  run_next_test();
});

add_test(function test_errorIcon()
{
  PlacesUtils.favicons.setAndFetchFaviconForPage(
                       NetUtil.newURI("http://example.com/errorIcon"),
                       FAVICON_ERRORPAGE_URI, true);

  run_next_test();
});

add_test(function test_finalVerification()
{
  
  
  
  PlacesUtils.favicons.setAndFetchFaviconForPage(
                       LAST_PAGE_URI,
                       LAST_FAVICON_URI, true);
});
