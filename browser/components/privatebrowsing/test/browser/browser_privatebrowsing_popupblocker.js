







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  let oldPopupPolicy = gPrefService.getBoolPref("dom.disable_open_during_load");
  gPrefService.setBoolPref("dom.disable_open_during_load", true);

  const TEST_URI = "http://mochi.test:8888/browser/browser/components/privatebrowsing/test/browser/popup.html";

  waitForExplicitFinish();

  function testPopupBlockerMenuItem(expectedDisabled, callback) {
    gBrowser.addEventListener("DOMUpdatePageReport", function() {
      gBrowser.removeEventListener("DOMUpdatePageReport", arguments.callee, false);
      executeSoon(function() {
        let pageReportButton = document.getElementById("page-report-button");
        let notification = gBrowser.getNotificationBox().getNotificationWithValue("popup-blocked");

        ok(!pageReportButton.hidden, "The page report button should not be hidden");
        ok(notification, "The notification box should be displayed");

        function checkMenuItem(callback) {
          document.addEventListener("popupshown", function(event) {
            document.removeEventListener("popupshown", arguments.callee, false);

            if (expectedDisabled)
              is(document.getElementById("blockedPopupAllowSite").getAttribute("disabled"), "true",
                 "The allow popups menu item should be disabled");

            event.originalTarget.hidePopup();
            callback();
          }, false);
        }

        checkMenuItem(function() {
          checkMenuItem(function() {
            gBrowser.removeTab(tab);
            callback();
          });
          notification.querySelector("button").doCommand();
        });
        EventUtils.synthesizeMouse(document.getElementById("page-report-button"), 1, 1, {});
      });
    }, false);

    let tab = gBrowser.addTab(TEST_URI);
    gBrowser.selectedTab = tab;
  }

  testPopupBlockerMenuItem(false, function() {
    pb.privateBrowsingEnabled = true;
    testPopupBlockerMenuItem(true, function() {
      pb.privateBrowsingEnabled = false;
      testPopupBlockerMenuItem(false, function() {
        gPrefService.setBoolPref("dom.disable_open_during_load", oldPopupPolicy);
        finish();
      });
    });
  });
}
