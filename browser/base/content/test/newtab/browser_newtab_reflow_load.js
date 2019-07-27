


"use strict";

const FRAME_SCRIPT = getRootDirectory(gTestPath) + "content-reflows.js";
const ADDITIONAL_WAIT_MS = 2000;




function runTests() {
  gBrowser.selectedTab = gBrowser.addTab("about:blank", {animate: false});
  let browser = gBrowser.selectedBrowser;
  yield whenBrowserLoaded(browser);

  let mm = browser.messageManager;
  mm.loadFrameScript(FRAME_SCRIPT, true);
  mm.addMessageListener("newtab-reflow", ({data: stack}) => {
    ok(false, `unexpected uninterruptible reflow ${stack}`);
  });

  browser.loadURI("about:newtab");
  yield whenBrowserLoaded(browser);

  
  yield setTimeout(TestRunner.next, ADDITIONAL_WAIT_MS);

  
  gBrowser.removeCurrentTab({animate: false});
}

function whenBrowserLoaded(browser) {
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    executeSoon(TestRunner.next);
  }, true);
}
