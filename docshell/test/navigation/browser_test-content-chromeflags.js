const TEST_PAGE = `data:text/html,<html><body><a href="about:blank" target="_blank">Test</a></body></html>`;
const CHROME_ALL = Ci.nsIWebBrowserChrome.CHROME_ALL;
const CHROME_REMOTE_WINDOW = Ci.nsIWebBrowserChrome.CHROME_REMOTE_WINDOW;





add_task(function* () {
  
  
  yield new Promise(resolve => {
    SpecialPowers.pushPrefEnv({
      "set": [
        ["browser.link.open_newwindow", 2],
      ]
    }, resolve);
  });

  yield BrowserTestUtils.withNewTab({
    gBrowser,
    url: TEST_PAGE
  }, function*(browser) {
    let openedPromise = BrowserTestUtils.waitForNewWindow();
    BrowserTestUtils.synthesizeMouse("a", 0, 0, {}, browser);
    let win = yield openedPromise;

    let chromeFlags = win.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIWebNavigation)
                         .QueryInterface(Ci.nsIDocShellTreeItem)
                         .treeOwner
                         .QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIXULWindow)
                         .chromeFlags;

    
    
    const EXPECTED = gMultiProcessBrowser ? CHROME_ALL | CHROME_REMOTE_WINDOW
                                          : CHROME_ALL;

    is(chromeFlags, EXPECTED, "Window should have opened with all chrome");

    BrowserTestUtils.closeWindow(win);
  });
});
