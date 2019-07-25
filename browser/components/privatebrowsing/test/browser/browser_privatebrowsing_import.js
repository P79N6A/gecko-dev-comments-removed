







































function test() {
  
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  let importMenuItem = document.getElementById("menu_import");

  
  ok(!importMenuItem.hasAttribute("disabled"),
    "File->Import menu item should not be disabled outside of the private browsing mode");

  
  pb.privateBrowsingEnabled = true;

  ok(importMenuItem.hasAttribute("disabled"),
    "File->Import menu item should be disabled inside of the private browsing mode");

  
  pb.privateBrowsingEnabled = false;

  ok(!importMenuItem.hasAttribute("disabled"),
    "File->Import menu item should not be disabled after leaving the private browsing mode");

  
  gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
}
