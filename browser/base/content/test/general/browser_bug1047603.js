



const OPEN_LOCATION_PREF = "browser.link.open_newwindow";
const NON_REMOTE_PAGE = "about:crashes";

const SIMPLE_PAGE_HTML = `
<a href="about:home" target="_blank" id="testAnchor">Open a window</a>
`;

function frame_script() {
  addMessageListener("test:click", (message) => {
    let element = content.document.getElementById("testAnchor");
    element.click();
  });
  sendAsyncMessage("test:ready");
}





function waitForFrameScriptReady(mm) {
  return new Promise((resolve, reject) => {
    mm.addMessageListener("test:ready", function onTestReady() {
      mm.removeMessageListener("test:ready", onTestReady);
      resolve();
    });
  });
}







function prepareNonRemoteBrowser(aWindow, browser) {
  aWindow.gBrowser.updateBrowserRemoteness(browser, false);
  browser.loadURI(NON_REMOTE_PAGE);
  return new Promise((resolve, reject) => {
    waitForCondition(() => !browser.isRemoteBrowser, () => {
      resolve();
    }, "Waiting for browser to become non-remote");
  })
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
    let testBrowser = testWindow.gBrowser.selectedBrowser;
    yield prepareNonRemoteBrowser(testWindow, testBrowser);

    
    let mm = testBrowser.messageManager;
    mm.loadFrameScript("data:,(" + frame_script.toString() + ")();", true);
    let readyPromise = waitForFrameScriptReady(mm);
    yield readyPromise;

    
    testBrowser.contentDocument.body.innerHTML = SIMPLE_PAGE_HTML;

    
    mm.sendAsyncMessage("test:click");
    let tabOpenEvent = yield waitForNewTab(testWindow.gBrowser);
    let newTab = tabOpenEvent.target;
    ok(!newTab.linkedBrowser.isRemoteBrowser,
       "The opened browser should not be remote.");

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
    let testBrowser = testWindow.gBrowser.selectedBrowser;
    yield prepareNonRemoteBrowser(testWindow, testBrowser);

    
    let mm = testBrowser.messageManager;
    mm.loadFrameScript("data:,(" + frame_script.toString() + ")();", true);
    let readyPromise = waitForFrameScriptReady(mm);
    yield readyPromise;

    
    testBrowser.contentDocument.body.innerHTML = SIMPLE_PAGE_HTML;

    
    let windowOpenPromise = promiseTopicObserved("browser-delayed-startup-finished");
    mm.sendAsyncMessage("test:click");
    let [newWindow] = yield windowOpenPromise;
    ok(!newWindow.gMultiProcessBrowser,
       "The opened window should not be an e10s window.");
    newWindow.close();
  }

  normalWindow.close();
  privateWindow.close();

  Services.prefs.clearUserPref(OPEN_LOCATION_PREF);
});
