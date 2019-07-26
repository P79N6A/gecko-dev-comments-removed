



function test() {
  let tab1 = gBrowser.addTab("about:rights");
  let tab2 = gBrowser.addTab("about:mozilla");
  whenBrowserLoaded(tab1.linkedBrowser, mainPart);
  waitForExplicitFinish();

  function mainPart() {
    
    let newTabState = '{"entries":[{"url":"about:rights"}],"pinned":true,"userTypedValue":"Hello World!"}';
    ss.setTabState(tab1, newTabState);

    
    gBrowser.unpinTab(tab1);

    
    gBrowser.removeTab(tab1);
    let savedState = JSON.parse(ss.getClosedTabData(window))[0].state;
    isnot(savedState.pinned, true, "Pinned should not be true");
    tab1 = ss.undoCloseTab(window, 0);

    isnot(tab1.pinned, true, "Should not be pinned");
    gBrowser.removeTab(tab1);
    gBrowser.removeTab(tab2);
    finish();
  }
}
