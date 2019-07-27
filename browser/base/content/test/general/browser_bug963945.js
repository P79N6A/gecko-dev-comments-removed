








add_task(function* test() {
  let win = yield BrowserTestUtils.openNewBrowserWindow({private: true});

  let tab = win.gBrowser.selectedTab = win.gBrowser.addTab("about:addons");
  yield BrowserTestUtils.browserLoaded(tab.linkedBrowser);
  yield promiseWaitForFocus(win);

  EventUtils.synthesizeKey("a", { ctrlKey: true, shiftKey: true }, win);

  is(win.gBrowser.tabs.length, 2, "about:addons tab was re-focused.");
  is(win.gBrowser.currentURI.spec, "about:addons", "Addons tab was opened.");

  yield BrowserTestUtils.closeWindow(win);
});
