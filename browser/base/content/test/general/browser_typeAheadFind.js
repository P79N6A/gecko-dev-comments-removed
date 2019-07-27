



add_task(function *() {
  let testWindow = yield BrowserTestUtils.openNewBrowserWindow();

  testWindow.gBrowser.loadURI("data:text/html,<h1>A Page</h1>");
  yield BrowserTestUtils.browserLoaded(testWindow.gBrowser.selectedBrowser);

  yield SimpleTest.promiseFocus(testWindow.gBrowser.selectedBrowser);

  ok(!testWindow.gFindBarInitialized, "find bar is not initialized");

  let findBarOpenPromise = promiseWaitForEvent(testWindow.gBrowser, "findbaropen");
  EventUtils.synthesizeKey("/", {}, testWindow);
  yield findBarOpenPromise;

  ok(testWindow.gFindBarInitialized, "find bar is now initialized");

  yield BrowserTestUtils.closeWindow(testWindow);
});
