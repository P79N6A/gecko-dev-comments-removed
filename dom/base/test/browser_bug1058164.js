



"use strict";

const kTimeout = 5000; 






function frame_script() {
  addEventListener("pageshow", (event) => {
    if (event.persisted) {
      sendAsyncMessage("test:pageshow");
    }
  });
  addEventListener("pagehide", (event) => {
    if (event.persisted) {
      sendAsyncMessage("test:pagehide");
    }
  });
}







function prepareForPageEvent(browser, eventType) {
  return new Promise((resolve, reject) => {
    let mm = browser.messageManager;

    let timeoutId = setTimeout(() => {
      ok(false, "Timed out waiting for " + eventType)
      reject();
    }, kTimeout);

    mm.addMessageListener("test:" + eventType, function onSawEvent(message) {
      mm.removeMessageListener("test:" + eventType, onSawEvent);
      ok(true, "Saw " + eventType + " event in frame script.");
      clearTimeout(timeoutId);
      resolve();
    });
  });
}





function prepareForPageHideAndShow(browser) {
  return Promise.all([prepareForPageEvent(browser, "pagehide"),
                      prepareForPageEvent(browser, "pageshow")]);
}





function waitForLoad(aWindow) {
  return new Promise((resolve, reject) => {
    aWindow.addEventListener("load", function onLoad(aEvent) {
      aWindow.removeEventListener("load", onLoad, true);
      resolve();
    }, true);
  });
}






add_task(function* test_swap_frameloader_pagevisibility_events() {
  
  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;
  let mm = window.getGroupMessageManager("browsers");
  mm.loadFrameScript("data:,(" + frame_script.toString() + ")();", true);
  let browser = gBrowser.selectedBrowser;

  
  let newWindow = gBrowser.replaceTabWithWindow(tab);
  
  
  yield waitForLoad(newWindow);
  let pageHideAndShowPromise = prepareForPageHideAndShow(newWindow.gBrowser.selectedBrowser);
  
  yield pageHideAndShowPromise;

  
  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  browser = newWindow.gBrowser.selectedBrowser;
  pageHideAndShowPromise = prepareForPageHideAndShow(gBrowser.selectedBrowser);
  gBrowser.swapBrowsersAndCloseOther(newTab, newWindow.gBrowser.selectedTab);

  
  yield pageHideAndShowPromise;

  gBrowser.removeTab(gBrowser.selectedTab);
});
