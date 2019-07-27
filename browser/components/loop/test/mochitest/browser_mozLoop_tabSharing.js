








const {injectLoopAPI} = Cu.import("resource:///modules/loop/MozLoopAPI.jsm");
gMozLoopAPI = injectLoopAPI({});

let promiseTabWindowId = function() {
  return new Promise(resolve => {
    gMozLoopAPI.getActiveTabWindowId((err, windowId) => {
      Assert.equal(null, err, "No error should've occurred.");
      Assert.equal(typeof windowId, "number", "We should have a window ID");
      resolve(windowId);
    });
  });
};

add_task(function* test_windowIdFetch_simple() {
  Assert.ok(gMozLoopAPI, "mozLoop should exist");

  yield promiseTabWindowId();
});

add_task(function* test_windowIdFetch_multipleTabs() {
  let previousTab = gBrowser.selectedTab;
  let previousTabId = yield promiseTabWindowId();

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  yield promiseTabLoadEvent(tab, "about:mozilla");
  let tabId = yield promiseTabWindowId();
  Assert.ok(tabId !== previousTabId, "Tab contentWindow IDs shouldn't be the same");
  gBrowser.removeTab(tab);

  tabId = yield promiseTabWindowId();
  Assert.equal(previousTabId, tabId, "Window IDs should be back to what they were");
});
