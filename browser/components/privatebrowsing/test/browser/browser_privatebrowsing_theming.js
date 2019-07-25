







































function test() {
  
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let docRoot = document.documentElement;

  ok(!docRoot.hasAttribute("privatebrowsingmode"),
    "privatebrowsingmode should not be present in normal mode");

  
  pb.privateBrowsingEnabled = true;

  is(docRoot.getAttribute("privatebrowsingmode"), "temporary",
    "privatebrowsingmode should be \"temporary\" inside the private browsing mode");

  
  pb.privateBrowsingEnabled = false;

  ok(!docRoot.hasAttribute("privatebrowsingmode"),
    "privatebrowsingmode should not be present in normal mode");

  
  gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
}
