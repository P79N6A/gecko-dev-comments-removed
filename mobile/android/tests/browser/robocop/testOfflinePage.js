




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");

function promiseBrowserEvent(browser, eventType) {
  return new Promise((resolve) => {
    function handle(event) {
      
      if (event.target != browser.contentDocument || event.target.location.href == "about:blank") {
        do_print("Skipping spurious '" + eventType + "' event" + " for " + event.target.location.href);
        return;
      }
      do_print("Received event " + eventType + " from browser");
      browser.removeEventListener(eventType, handle, true);
      resolve(event);
    }

    browser.addEventListener(eventType, handle, true);
    do_print("Now waiting for " + eventType + " event from browser");
  });
}


function promiseOffline(isOffline) {
  return new Promise((resolve, reject) => {
    function observe(subject, topic, data) {
      do_print("Received topic: " + topic);
      Services.obs.removeObserver(observe, "network:offline-status-changed");
      resolve();
    }
    Services.obs.addObserver(observe, "network:offline-status-changed", false);
    Services.io.offline = isOffline;
  });
}


let chromeWin;


let browser;


let proxyPrefValue;

const kUniqueURI = Services.io.newURI("http://mochi.test:8888/tests/robocop/video_controls.html", null, null);

add_task(function* test_offline() {
  
  
  proxyPrefValue = Services.prefs.getIntPref("network.proxy.type");
  Services.prefs.setIntPref("network.proxy.type", 0);

  
  Cc["@mozilla.org/netwerk/cache-storage-service;1"].getService(Ci.nsICacheStorageService).clear();

  chromeWin = Services.wm.getMostRecentWindow("navigator:browser");
  let BrowserApp = chromeWin.BrowserApp;

  do_register_cleanup(function cleanup() {
    BrowserApp.closeTab(BrowserApp.getTabForBrowser(browser));
    Services.prefs.setIntPref("network.proxy.type", proxyPrefValue);
    Services.io.offline = false;
  });

  
  browser = BrowserApp.addTab("about:blank", { selected: true, parentId: BrowserApp.selectedTab.id }).browser;

  
  yield promiseOffline(true);

  
  browser.loadURI(kUniqueURI.spec, null, null)
  yield promiseBrowserEvent(browser, "DOMContentLoaded");

  
  is(browser.contentDocument.documentURI.substring(0, 27), "about:neterror?e=netOffline", "Document URI is the error page.");

  
  is(browser.contentWindow.location.href, kUniqueURI.spec, "Docshell URI is the original URI.");

  Services.prefs.setIntPref("network.proxy.type", proxyPrefValue);

  
  yield promiseOffline(false);

  ok(browser.contentDocument.getElementById("errorTryAgain"), "The error page has got a #errorTryAgain element");

  
  browser.contentDocument.getElementById("errorTryAgain").click();
  yield promiseBrowserEvent(browser, "DOMContentLoaded");

  
  is(browser.contentDocument.documentURI, kUniqueURI.spec, "Document URI is not the offline-error page, but the original URI.");
});

run_next_test();
