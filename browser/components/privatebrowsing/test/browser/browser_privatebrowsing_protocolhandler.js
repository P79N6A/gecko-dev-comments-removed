






function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const testPageURL = "http://example.com/browser/" +
    "browser/components/privatebrowsing/test/browser/browser_privatebrowsing_protocolhandler_page.html";
  waitForExplicitFinish();

  const notificationValue = "Protocol Registration: testprotocol";

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function () {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    setTimeout(function() {
      
      let notificationBox = gBrowser.getNotificationBox();
      let notification = notificationBox.getNotificationWithValue(notificationValue);
      ok(notification, "Notification box should be displaying outside of private browsing mode");
      gBrowser.removeCurrentTab();

      
      pb.privateBrowsingEnabled = true;

      gBrowser.selectedTab = gBrowser.addTab();
      gBrowser.selectedBrowser.addEventListener("load", function () {
        gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

        setTimeout(function () {
          
          let notificationBox = gBrowser.getNotificationBox();
          let notification = notificationBox.getNotificationWithValue(notificationValue);
          ok(!notification, "Notification box should not be displayed inside of private browsing mode");

          gBrowser.removeCurrentTab();

          
          pb.privateBrowsingEnabled = false;
          finish();
        }, 100); 
      }, true);
      content.location = testPageURL;
    }, 100); 
  }, true);
  content.location = testPageURL;
}
