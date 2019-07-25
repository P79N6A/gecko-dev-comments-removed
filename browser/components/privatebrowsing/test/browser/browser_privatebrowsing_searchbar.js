







































function test() {
  
  waitForExplicitFinish();
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  
  const kTestSearchString = "privatebrowsing";
  let searchBar = BrowserSearch.searchBar;
  searchBar.value = kTestSearchString + "foo";
  searchBar.value = kTestSearchString;

  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);

  registerCleanupFunction(function () {
    searchBar.textbox.reset();

    gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
  });

  
  pb.privateBrowsingEnabled = true;

  is(searchBar.value, kTestSearchString,
    "entering the private browsing mode should not clear the search bar");
  ok(searchBar.textbox.editor.transactionManager.numberOfUndoItems > 0,
    "entering the private browsing mode should not reset the undo list of the searchbar control");

  
  searchBar.value = "something else";

  
  pb.privateBrowsingEnabled = false;

  is(searchBar.value, kTestSearchString,
    "leaving the private browsing mode should restore the search bar contents");
  is(searchBar.textbox.editor.transactionManager.numberOfUndoItems, 1,
    "leaving the private browsing mode should only leave 1 item in the undo list of the searchbar control");

  
  pb.privateBrowsingEnabled = true;

  const TEST_URL =
    "data:text/html,<head><link rel=search type='application/opensearchdescription+xml' href='http://foo.bar' title=dummy></head>";
  gBrowser.selectedTab = gBrowser.addTab(TEST_URL);
  gBrowser.selectedBrowser.addEventListener("load", function(e) {
    e.currentTarget.removeEventListener("load", arguments.callee, true);

    var browser = gBrowser.selectedBrowser;
    is(typeof browser.engines, "undefined",
       "An engine should not be discovered in private browsing mode");

    gBrowser.removeTab(gBrowser.selectedTab);
    pb.privateBrowsingEnabled = false;

    finish();
  }, true);
}
