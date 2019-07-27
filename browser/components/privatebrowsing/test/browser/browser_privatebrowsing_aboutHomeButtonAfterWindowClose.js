





add_task(function* test_no_sessionrestore_button() {
  
  (yield BrowserTestUtils.openNewBrowserWindow({private: true})).close();

  let win = yield BrowserTestUtils.openNewBrowserWindow({private: true});
  let tab = win.gBrowser.addTab("about:home");
  let browser = tab.linkedBrowser;

  yield BrowserTestUtils.browserLoaded(browser);

  let display = yield ContentTask.spawn(browser, {}, function* (){
    let button = content.document.getElementById("restorePreviousSession");
    return content.getComputedStyle(button).display;
  });

  is(display, "none",
    "The Session Restore about:home button should be disabled");

  win.close();
});
