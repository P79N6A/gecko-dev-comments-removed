







































function test() {
  
  let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  
  const kTestSearchString = "privatebrowsing";
  let searchBar = BrowserSearch.searchBar;
  searchBar.value = kTestSearchString;

  
  pb.privateBrowsingEnabled = true;

  is(searchBar.value, kTestSearchString,
    "entering the private browsing mode should not clear the search bar");
  







  
  pb.privateBrowsingEnabled = false;

  is(searchBar.value, "",
    "leaving the private browsing mode should clear the search bar");
  







  
  prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
}
