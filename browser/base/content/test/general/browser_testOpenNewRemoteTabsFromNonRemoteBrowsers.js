



const OPEN_LOCATION_PREF = "browser.link.open_newwindow";
const NON_REMOTE_PAGE = "about:welcomeback";

Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

function frame_script() {
  content.document.body.innerHTML = `
    <a href="about:home" target="_blank" id="testAnchor">Open a window</a>
  `;

  let element = content.document.getElementById("testAnchor");
  element.click();
}







function prepareNonRemoteBrowser(aWindow, browser) {
  browser.loadURI(NON_REMOTE_PAGE);
  return waitForDocLoadComplete(browser);
}

registerCleanupFunction(() => {
  Services.prefs.clearUserPref(OPEN_LOCATION_PREF);
});





add_task(function* test_new_tab() {
  let normalWindow = yield promiseOpenAndLoadWindow({
    remote: true
  }, true);
  let privateWindow = yield promiseOpenAndLoadWindow({
    remote: true,
    private: true,
  }, true);

  for (let testWindow of [normalWindow, privateWindow]) {
    yield promiseWaitForFocus(testWindow);
    let testBrowser = testWindow.gBrowser.selectedBrowser;
    info("Preparing non-remote browser");
    yield prepareNonRemoteBrowser(testWindow, testBrowser);
    info("Non-remote browser prepared - sending frame script");

    
    let mm = testBrowser.messageManager;
    mm.loadFrameScript("data:,(" + frame_script.toString() + ")();", true);

    let tabOpenEvent = yield waitForNewTabEvent(testWindow.gBrowser);
    let newTab = tabOpenEvent.target;

    yield promiseTabLoadEvent(newTab);

    
    
    ok(newTab.linkedBrowser.isRemoteBrowser,
       "The opened browser never became remote.");

    testWindow.gBrowser.removeTab(newTab);
  }

  normalWindow.close();
  privateWindow.close();
});






add_task(function* test_new_window() {
  let normalWindow = yield promiseOpenAndLoadWindow({
    remote: true
  }, true);
  let privateWindow = yield promiseOpenAndLoadWindow({
    remote: true,
    private: true,
  }, true);

  
  
  Services.prefs.setIntPref(OPEN_LOCATION_PREF,
                            Ci.nsIBrowserDOMWindow.OPEN_NEWWINDOW);

  for (let testWindow of [normalWindow, privateWindow]) {
    yield promiseWaitForFocus(testWindow);
    let testBrowser = testWindow.gBrowser.selectedBrowser;
    yield prepareNonRemoteBrowser(testWindow, testBrowser);

    
    let mm = testBrowser.messageManager;
    mm.loadFrameScript("data:,(" + frame_script.toString() + ")();", true);

    
    let {subject: newWindow} =
      yield promiseTopicObserved("browser-delayed-startup-finished");

    is(PrivateBrowsingUtils.isWindowPrivate(testWindow),
       PrivateBrowsingUtils.isWindowPrivate(newWindow),
       "Private browsing state of new window does not match the original!");

    let newTab = newWindow.gBrowser.selectedTab;

    yield promiseTabLoadEvent(newTab);

    
    
    ok(newTab.linkedBrowser.isRemoteBrowser,
       "The opened browser never became remote.");
    newWindow.close();
  }

  normalWindow.close();
  privateWindow.close();
});
