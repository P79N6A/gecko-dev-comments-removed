


"use strict";

let notificationObserver;
registerCleanupFunction(function() {
  Services.prefs.clearUserPref("browser.fixup.domainwhitelist.localhost");
  if (notificationObserver) {
    notificationObserver.disconnect();
  }
});

function promiseNotificationForTab(value, expected, tab=gBrowser.selectedTab) {
  let deferred = Promise.defer();
  let notificationBox = gBrowser.getNotificationBox(tab.linkedBrowser);
  if (expected) {
    let checkForNotification = function() {
      if (notificationBox.getNotificationWithValue(value)) {
        notificationObserver.disconnect();
        notificationObserver = null;
        deferred.resolve();
      }
    }
    if (notificationObserver) {
      notificationObserver.disconnect();
    }
    notificationObserver = new MutationObserver(checkForNotification);
    notificationObserver.observe(notificationBox, {childList: true});
  } else {
    setTimeout(() => {
      is(notificationBox.getNotificationWithValue(value), null, "We are expecting to not get a notification");
      deferred.resolve();
    }, 1000);
  }
  return deferred.promise;
}

function* runURLBarSearchTest(valueToOpen, expectSearch, expectNotification) {
  gURLBar.value = valueToOpen;
  let expectedURI;
  if (!expectSearch) {
    expectedURI = "http://" + valueToOpen + "/";
  } else {
    yield new Promise(resolve => {
      Services.search.init(resolve)
    });
    expectedURI = Services.search.defaultEngine.getSubmission(valueToOpen, null, "keyword").uri.spec;
  }
  gURLBar.focus();
  let docLoadPromise = waitForDocLoadAndStopIt(expectedURI);
  EventUtils.synthesizeKey("VK_RETURN", {});

  yield docLoadPromise;

  yield promiseNotificationForTab("keyword-uri-fixup", expectNotification);
}

add_task(function* test_navigate_full_domain() {
  let tab = gBrowser.selectedTab = gBrowser.addTab();
  yield* runURLBarSearchTest("www.mozilla.org", false, false);
  gBrowser.removeTab(tab);
});

function get_test_function_for_localhost_with_hostname(hostName) {
  return function* test_navigate_single_host() {
    const pref = "browser.fixup.domainwhitelist.localhost";
    Services.prefs.setBoolPref(pref, false);
    let tab = gBrowser.selectedTab = gBrowser.addTab();
    yield* runURLBarSearchTest(hostName, true, true);

    let notificationBox = gBrowser.getNotificationBox(tab.linkedBrowser);
    let notification = notificationBox.getNotificationWithValue("keyword-uri-fixup");
    let docLoadPromise = waitForDocLoadAndStopIt("http://" + hostName + "/");
    notification.querySelector(".notification-button-default").click();

    
    let prefValue = Services.prefs.getBoolPref(pref);
    ok(prefValue, "Pref should have been toggled");

    yield docLoadPromise;
    gBrowser.removeTab(tab);

    
    let tab = gBrowser.selectedTab = gBrowser.addTab();
    yield* runURLBarSearchTest(hostName, false, false);
    gBrowser.removeTab(tab);
  }
}

add_task(get_test_function_for_localhost_with_hostname("localhost"));
add_task(get_test_function_for_localhost_with_hostname("localhost."));

add_task(function* test_navigate_invalid_url() {
  let tab = gBrowser.selectedTab = gBrowser.addTab();
  yield* runURLBarSearchTest("mozilla is awesome", true, false);
  gBrowser.removeTab(tab);
});

