







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const testPageURL = "http://localhost:8888/browser/" +
    "browser/components/privatebrowsing/test/browser/browser_privatebrowsing_geoprompt_page.html";
  waitForExplicitFinish();

  let pageTab = gBrowser.addTab();
  gBrowser.selectedTab = pageTab;
  let pageBrowser = gBrowser.getBrowserForTab(pageTab);
  pageBrowser.addEventListener("load", function () {
    pageBrowser.removeEventListener("load", arguments.callee, true);

    setTimeout(function() {
      
      let notificationBox = gBrowser.getNotificationBox(pageBrowser);
      let notification = notificationBox.getNotificationWithValue("geolocation");
      ok(notification, "Notification box should be displaying outside of private browsing mode");
      is(notification.getElementsByClassName("rememberChoice").length, 1,
         "The remember control must be displayed outside of private browsing mode");
      notificationBox.currentNotification.close();

      gBrowser.removeTab(pageTab);

      
      pb.privateBrowsingEnabled = true;

      pageTab = gBrowser.addTab();
      gBrowser.selectedTab = pageTab;
      pageBrowser = gBrowser.getBrowserForTab(pageTab);
      pageBrowser.addEventListener("load", function () {
        pageBrowser.removeEventListener("load", arguments.callee, true);

        setTimeout(function() {
          
          let notificationBox = gBrowser.getNotificationBox(pageBrowser);
          let notification = notificationBox.getNotificationWithValue("geolocation");
          ok(notification, "Notification box should be displaying outside of private browsing mode");
          is(notification.getElementsByClassName("rememberChoice").length, 0,
             "The remember control must not be displayed inside of private browsing mode");
          notificationBox.currentNotification.close();

          gBrowser.removeTab(pageTab);

          
          pb.privateBrowsingEnabled = false;
          finish();
        }, 100); 
      }, true);
      pageBrowser.contentWindow.location = testPageURL;
    }, 100); 
  }, true);
  pageBrowser.contentWindow.location = testPageURL;
}
