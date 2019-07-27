


let url = "data:text/html;charset=utf-8,<input%20id='foo'>";
let tabState = {
  entries: [{ url }], formdata: { id: { "foo": "bar" }, url }
};

function test() {
  waitForExplicitFinish();
  Services.prefs.setBoolPref("browser.sessionstore.restore_on_demand", true);

  registerCleanupFunction(function () {
    if (gBrowser.tabs.length > 1)
      gBrowser.removeTab(gBrowser.tabs[1]);
    Services.prefs.clearUserPref("browser.sessionstore.restore_on_demand");
  });

  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;

  promiseBrowserLoaded(browser).then(() => {
    isnot(gBrowser.selectedTab, tab, "newly created tab is not selected");

    ss.setTabState(tab, JSON.stringify(tabState));
    is(browser.__SS_restoreState, TAB_STATE_NEEDS_RESTORE, "tab needs restoring");

    let {formdata} = JSON.parse(ss.getTabState(tab));
    is(formdata && formdata.id["foo"], "bar", "tab state's formdata is valid");

    promiseTabRestored(tab).then(() => {
      let input = browser.contentDocument.getElementById("foo");
      is(input.value, "bar", "formdata has been restored correctly");
      finish();
    });

    
    gBrowser.selectedTab = tab;
  });
}
