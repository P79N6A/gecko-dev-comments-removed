







































function test() {
  
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  
  const kTestSearchString = "privatebrowsing";
  let searchBar = BrowserSearch.searchBar;
  searchBar.value = kTestSearchString;

  
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

  
  gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
}
