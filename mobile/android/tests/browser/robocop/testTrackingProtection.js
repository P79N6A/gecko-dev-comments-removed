




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");

function promiseLoadEvent(browser, url, eventType="load", runBeforeLoad) {
  return new Promise((resolve, reject) => {
    do_print("Wait browser event: " + eventType);

    function handle(event) {
      if (event.target != browser.contentDocument || event.target.location.href == "about:blank" || (url && event.target.location.href != url)) {
        do_print("Skipping spurious '" + eventType + "' event" + " for " + event.target.location.href);
        return;
      }

      browser.removeEventListener(eventType, handle, true);
      do_print("Browser event received: " + eventType);
      resolve(event);
    }

    browser.addEventListener(eventType, handle, true);

    if (runBeforeLoad) {
      runBeforeLoad();
    }
    if (url) {
      browser.loadURI(url);
    }
  });
}







var TABLE = "urlclassifier.trackingTable";


function doUpdate() {
  
  var testData = "tracking.example.com/";
  var testUpdate =
    "n:1000\ni:test-track-simple\nad:1\n" +
    "a:524:32:" + testData.length + "\n" +
    testData;

  let dbService = Cc["@mozilla.org/url-classifier/dbservice;1"].getService(Ci.nsIUrlClassifierDBService);

  return new Promise((resolve, reject) => {
    let listener = {
      QueryInterface: function(iid) {
        if (iid.equals(Ci.nsISupports) || iid.equals(Ci.nsIUrlClassifierUpdateObserver))
          return this;

        throw Cr.NS_ERROR_NO_INTERFACE;
      },
      updateUrlRequested: function(url) { },
      streamFinished: function(status) { },
      updateError: function(errorCode) {
        ok(false, "Couldn't update classifier.");
        resolve();
      },
      updateSuccess: function(requestedTimeout) {
        resolve();
      }
    };

    dbService.beginUpdate(listener, "test-track-simple", "");
    dbService.beginStream("", "");
    dbService.updateStream(testUpdate);
    dbService.finishStream();
    dbService.finishUpdate();
  });
}



add_task(function* test_tracking_pb() {
  let BrowserApp = Services.wm.getMostRecentWindow("navigator:browser").BrowserApp;

  
  let browser = BrowserApp.addTab("about:blank", { selected: true, parentId: BrowserApp.selectedTab.id, isPrivate: true }).browser;
  yield new Promise((resolve, reject) => {
    browser.addEventListener("load", function startTests(event) {
      browser.removeEventListener("load", startTests, true);
      Services.tm.mainThread.dispatch(resolve, Ci.nsIThread.DISPATCH_NORMAL);
    }, true);
  });

  
  Services.prefs.setCharPref(TABLE, "test-track-simple");
  yield doUpdate();

  
  yield promiseLoadEvent(browser, "http://tracking.example.org/tests/robocop/tracking_good.html");
  Messaging.sendRequest({ type: "Test:Expected", expected: "unknown" });

  
  yield promiseLoadEvent(browser, "http://tracking.example.org/tests/robocop/tracking_bad.html");
  Messaging.sendRequest({ type: "Test:Expected", expected: "tracking_content_blocked" });

  
  
  yield promiseLoadEvent(browser, undefined, undefined, () => {
    Services.obs.notifyObservers(null, "Session:Reload", "{\"allowContent\":true,\"contentType\":\"tracking\"}");
  });
  Messaging.sendRequest({ type: "Test:Expected", expected: "tracking_content_loaded" });

  
  yield promiseLoadEvent(browser, undefined, undefined, () => {
    Services.obs.notifyObservers(null, "Session:Reload", "{\"allowContent\":false,\"contentType\":\"tracking\"}");
  });
  Messaging.sendRequest({ type: "Test:Expected", expected: "tracking_content_blocked" });

  
  Services.prefs.setBoolPref("privacy.trackingprotection.pbmode.enabled", false);

  
  yield promiseLoadEvent(browser, "http://tracking.example.org/tests/robocop/tracking_bad.html");
  Messaging.sendRequest({ type: "Test:Expected", expected: "unknown" });

  
  yield promiseLoadEvent(browser, "http://tracking.example.org/tests/robocop/tracking_good.html");
  Messaging.sendRequest({ type: "Test:Expected", expected: "unknown" });
});

run_next_test();
