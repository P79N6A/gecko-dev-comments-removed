





add_task(function* testNoSessionRestoreButton() {
  
  (yield BrowserTestUtils.openNewBrowserWindow({private: true})).close();

  let win = yield BrowserTestUtils.openNewBrowserWindow({private: true});
  let tab = win.gBrowser.addTab("about:sessionrestore");
  let browser = tab.linkedBrowser;

  yield BrowserTestUtils.browserLoaded(browser);

  let disabled = yield ContentTask.spawn(browser, {}, function* (){
    return content.document.getElementById("errorTryAgain").disabled;
  });

  ok(disabled, "The Restore about:sessionrestore button should be disabled");

  win.close();
});
