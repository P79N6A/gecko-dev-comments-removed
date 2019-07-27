



function test() {
  

  waitForExplicitFinish();

  let uniqueName = "bug 465215";
  let uniqueValue1 = "as good as unique: " + Date.now();
  let uniqueValue2 = "as good as unique: " + Math.random();

  
  let tab1 = gBrowser.addTab();
  promiseBrowserLoaded(tab1.linkedBrowser).then(() => {
    ss.setTabValue(tab1, uniqueName, uniqueValue1);

    
    let tab2 = ss.duplicateTab(window, tab1);
    is(ss.getTabValue(tab2, uniqueName), uniqueValue1, "tab value was duplicated");

    ss.setTabValue(tab2, uniqueName, uniqueValue2);
    isnot(ss.getTabValue(tab1, uniqueName), uniqueValue2, "tab values aren't sync'd");

    
    promiseTabState(tab1, {entries: []}).then(() => {
      is(ss.getTabValue(tab1, uniqueName), "", "tab value was cleared");

      
      gBrowser.removeTab(tab2);
      gBrowser.removeTab(tab1);
      finish();
    });
  });
}
