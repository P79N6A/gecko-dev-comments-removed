







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const testPageURL = "http://localhost:8888/browser/" +
    "browser/components/privatebrowsing/test/browser/browser_privatebrowsing_geoprompt_page.html";
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function () {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

    setTimeout(function() {
      
      let notificationBox = gBrowser.getNotificationBox();
      let notification = notificationBox.getNotificationWithValue("geolocation");
      ok(notification, "Notification box should be displaying outside of private browsing mode");
      is(notification.getElementsByClassName("rememberChoice").length, 1,
         "The remember control must be displayed outside of private browsing mode");
      notificationBox.currentNotification.close();

      gBrowser.removeCurrentTab();

      
      pb.privateBrowsingEnabled = true;

      gBrowser.selectedTab = gBrowser.addTab();
      gBrowser.selectedBrowser.addEventListener("load", function () {
        gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

        setTimeout(function () {
          
          let notificationBox = gBrowser.getNotificationBox();
          let notification = notificationBox.getNotificationWithValue("geolocation");
          ok(notification, "Notification box should be displaying outside of private browsing mode");
          is(notification.getElementsByClassName("rememberChoice").length, 0,
             "The remember control must not be displayed inside of private browsing mode");
          notificationBox.currentNotification.close();

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
