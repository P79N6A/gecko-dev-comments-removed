




"use strict";

var gSessionStore = Cc["@mozilla.org/browser/sessionstore;1"]
                    .getService(Ci.nsISessionStore);

function test() {
  runTests();
}

function getState() {
  return JSON.parse(gSessionStore.getBrowserState());
}

function getTabData() {
  return getState().windows[0].tabs;
}

function isValidTabData(aData) {
  return aData && aData.entries && aData.entries.length &&
           typeof aData.index == "number";
}

gTests.push({
  desc: "getBrowserState tests",
  run: function() {
    
    yield waitForCondition(() => window.__SSID);
    info(window.__SSID);
    let tabData1 = getTabData();
    ok(tabData1.every(isValidTabData), "Tab data starts out valid");

    
    let tab = Browser.addTab("about:mozilla");
    let tabData2 = getTabData();
    is(tabData2.length, tabData1.length, "New tab not added yet.");

    
    yield waitForMessage("Content:SessionHistory", tab.browser.messageManager);
    yield waitForMs(0);
    let tabData3 = getTabData();
    is(tabData3.length, tabData1.length + 1, "New tab added.");
    ok(tabData3.every(isValidTabData), "Tab data still valid");

    
    Browser.closeTab(tab, { forceClose: true } );
    let tabData4 = getTabData();
    is(tabData4.length, tabData1.length, "Closed tab removed.");
    ok(tabData4.every(isValidTabData), "Tab data valid again");
  }
});
