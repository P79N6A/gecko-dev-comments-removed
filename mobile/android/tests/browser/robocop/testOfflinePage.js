




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");

function ok(passed, text) {
  do_report_result(passed, text, Components.stack.caller, false);
}

function is(lhs, rhs, text) {
  do_report_result(lhs === rhs, text, Components.stack.caller, false);
}


let chromeWin;


let browser;


let proxyPrefValue;

const kUniqueURI = Services.io.newURI("http://mochi.test:8888/tests/robocop/video_controls.html", null, null);

add_test(function setup_browser() {
  
  
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

  do_test_pending();

  
  browser = BrowserApp.addTab("about:blank", { selected: true, parentId: BrowserApp.selectedTab.id }).browser;

  
  Services.io.offline = true;

  
  browser.addEventListener("DOMContentLoaded", errorListener, true);
  browser.loadURI(kUniqueURI.spec, null, null)
});



function errorListener() {
  if (browser.contentWindow.location == "about:blank") {
    do_print("got about:blank, which is expected once, so return");
    return;
  }

  browser.removeEventListener("DOMContentLoaded", errorListener, true);
  ok(Services.io.offline, "Services.io.offline is true.");

  
  is(browser.contentDocument.documentURI.substring(0, 27), "about:neterror?e=netOffline", "Document URI is the error page.");

  
  is(browser.contentWindow.location.href, kUniqueURI.spec, "Docshell URI is the original URI.");

  Services.prefs.setIntPref("network.proxy.type", proxyPrefValue);

  
  Services.io.offline = false;

  browser.addEventListener("DOMContentLoaded", reloadListener, true);

  ok(browser.contentDocument.getElementById("errorTryAgain"), "The error page has got a #errorTryAgain element");
  browser.contentDocument.getElementById("errorTryAgain").click();
}




function reloadListener() {
  browser.removeEventListener("DOMContentLoaded", reloadListener, true);

  ok(!Services.io.offline, "Services.io.offline is false.");

  
  is(browser.contentDocument.documentURI, kUniqueURI.spec, "Document URI is not the offline-error page, but the original URI.");

  do_test_finished();

  run_next_test();
}

run_next_test();
