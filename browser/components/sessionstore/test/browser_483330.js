function test() {
  

  waitForExplicitFinish();

  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;

  let browser = tab.linkedBrowser;
  browser.addEventListener("load", function loadListener(e) {
    browser.removeEventListener("load", arguments.callee, true);

    
    browser.contentWindow.scrollTo(1100, 1200);
    is(browser.contentWindow.scrollX, 1100, "scrolled horizontally");
    is(browser.contentWindow.scrollY, 1200, "scrolled vertically");

    gBrowser.removeTab(tab);

    let newTab = ss.undoCloseTab(window, 0);
    newTab.addEventListener("SSTabRestored", function tabRestored(e) {
      newTab.removeEventListener("SSTabRestored", arguments.callee, true);

      let newBrowser = newTab.linkedBrowser;

      
      is(newBrowser.contentWindow.scrollX, 1100, "still scrolled horizontally");
      is(newBrowser.contentWindow.scrollY, 1200, "still scrolled vertically");

      gBrowser.removeTab(newTab);
      
      
      e.stopPropagation();

      finish();
    }, true);
  }, true);

  browser.loadURI("data:text/html;charset=utf-8,<body style='width: 100000px; height: 100000px;'><p>top</p></body>");
}
