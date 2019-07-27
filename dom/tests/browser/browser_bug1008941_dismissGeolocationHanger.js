



"use strict";

const TEST_URI = "http://example.com/" +
                 "browser/dom/tests/browser/position.html";

add_task(function testDismissHanger() {
  info("Check that location is not shared when dismissing the geolocation hanger");

  let promisePanelShown = waitForPanelShow();

  gBrowser.selectedTab = gBrowser.addTab(TEST_URI);
  yield waitForPageLoad(gBrowser.selectedTab);
  info("Page was loaded");

  yield promisePanelShown;
  info("Panel is shown");

  
  window.document.getElementById("nav-bar").click();
  info("Clicked outside the Geolocation panel to dismiss it");

  let result = gBrowser.getBrowserForTab(gBrowser.selectedTab)
                       .contentDocument.body.innerHTML;
  ok(result.contains("location..."), "Location is not shared");
});

add_task(function asyncCleanup() {
  
  gBrowser.removeTab(gBrowser.selectedTab);
  info("Cleanup: Closed the tab");
});

function waitForPageLoad(aTab) {
  let deferred = Promise.defer();

  function onTabLoad(event) {
    aTab.linkedBrowser.removeEventListener("load", onTabLoad, true);
    info("Load tab event received");
    deferred.resolve();
  }

  aTab.linkedBrowser.addEventListener("load", onTabLoad, true, true);
  return deferred.promise;
}

function waitForPanelShow(aPanel) {
  let deferred = Promise.defer();

  function onPopupShown(event) {
    PopupNotifications.panel.removeEventListener("popupshown", onPopupShown, true);
    info("Popup shown event received");
    deferred.resolve();
  }

  PopupNotifications.panel.addEventListener("popupshown", onPopupShown, true, true);
  return deferred.promise;
}
