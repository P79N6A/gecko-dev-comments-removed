



function test() {
  let uriString = "http://example.com/";
  let cookieBehavior = "network.cookie.cookieBehavior";
  let uriObj = Services.io.newURI(uriString, null, null)
  let cp = Components.classes["@mozilla.org/cookie/permission;1"]
                     .getService(Components.interfaces.nsICookiePermission);
  
  Services.prefs.setIntPref(cookieBehavior, 2);

  cp.setAccess(uriObj, cp.ACCESS_ALLOW);
  gBrowser.selectedTab = gBrowser.addTab(uriString);
  waitForExplicitFinish();
  gBrowser.selectedBrowser.addEventListener("load", onTabLoaded, true);
  
  function onTabLoaded() {
    is(gBrowser.selectedBrowser.contentWindow.navigator.cookieEnabled, true,
       "navigator.cookieEnabled should be true");
    
    gBrowser.selectedBrowser.removeEventListener("load", onTabLoaded, true);
    gBrowser.removeTab(gBrowser.selectedTab);
    Services.prefs.setIntPref(cookieBehavior, 0);
    cp.setAccess(uriObj, cp.ACCESS_DEFAULT);
    finish();
  }
}
