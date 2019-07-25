



































function test() {
  let tab1 = gBrowser.addTab("about:robots");
  let tab2 = gBrowser.addTab("about:mozilla");
  tab1.linkedBrowser.addEventListener("load", mainPart, true);
  waitForExplicitFinish();

  function mainPart() {
    tab1.linkedBrowser.removeEventListener("load", mainPart, true);

    
    let newTabState = '{"entries":[{"url":"about:robots"}],"pinned":true,"userTypedValue":"Hello World!"}';
    ss.setTabState(tab1, newTabState);

    
    gBrowser.unpinTab(tab1);

    is(tab1.linkedBrowser.__SS_data._tabStillLoading, true, 
       "_tabStillLoading should be true.");

    
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
