



function test() {
  

  waitForExplicitFinish();

  let uniqueValue = Math.random() + "\u2028Second line\u2029Second paragraph\u2027";

  let tab = gBrowser.addTab();
  promiseBrowserLoaded(tab.linkedBrowser).then(() => {
    ss.setTabValue(tab, "bug485563", uniqueValue);
    let tabState = JSON.parse(ss.getTabState(tab));
    is(tabState.extData["bug485563"], uniqueValue,
       "unicode line separator wasn't over-encoded");
    ss.deleteTabValue(tab, "bug485563");
    ss.setTabState(tab, JSON.stringify(tabState));
    is(ss.getTabValue(tab, "bug485563"), uniqueValue,
       "unicode line separator was correctly preserved");

    gBrowser.removeTab(tab);
    finish();
  });
}
