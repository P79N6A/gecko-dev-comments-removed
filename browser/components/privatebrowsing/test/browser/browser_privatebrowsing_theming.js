







































function test() {
  
  let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let docRoot = document.documentElement;

  is(docRoot.getAttribute("browsingmode"), "normal",
    "browsingmode should be \"normal\" initially");

  
  pb.privateBrowsingEnabled = true;

  is(docRoot.getAttribute("browsingmode"), "private",
    "browsingmode should be \"private\" inside the private browsing mode");

  
  pb.privateBrowsingEnabled = false;

  is(docRoot.getAttribute("browsingmode"), "normal",
    "browsingmode should be \"normal\" outside the private browsing mode");

  
  prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
}
