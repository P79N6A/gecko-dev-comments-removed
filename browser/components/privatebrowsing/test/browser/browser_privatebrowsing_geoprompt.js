






function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const testPageURL = "http://mochi.test:8888/browser/" +
    "browser/components/privatebrowsing/test/browser/browser_privatebrowsing_geoprompt_page.html";
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function () {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    let notification = PopupNotifications.getNotification("geolocation");
    ok(notification, "Notification should exist");
    ok(notification.secondaryActions.length > 1, "Secondary actions should exist (always/never remember)");
    notification.remove();

    gBrowser.removeCurrentTab();

    
    pb.privateBrowsingEnabled = true;

    gBrowser.selectedTab = gBrowser.addTab();
    gBrowser.selectedBrowser.addEventListener("load", function () {
      gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

      
      let notification = PopupNotifications.getNotification("geolocation");
      ok(notification, "Notification should exist");
      is(notification.secondaryActions.length, 0, "Secondary actions shouldn't exist (always/never remember)");
      notification.remove();

      gBrowser.removeCurrentTab();

      
      pb.privateBrowsingEnabled = false;
      finish();
    }, true);
    content.location = testPageURL;
  }, true);
  content.location = testPageURL;
}
