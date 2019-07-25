



































function observeOneRestore(callback) {
  let topic = "sessionstore-browser-state-restored";
  Services.obs.addObserver(function() {
    Services.obs.removeObserver(arguments.callee, topic, false);
    callback();
  }, topic, false);
};

function test() {
  waitForExplicitFinish();

  
  let [origTab] = gBrowser.visibleTabs;
  let hiddenTab = gBrowser.addTab();

  is(gBrowser.visibleTabs.length, 2, "should have 2 tabs before hiding");
  gBrowser.showOnlyTheseTabs([origTab]);
  is(gBrowser.visibleTabs.length, 1, "only 1 after hiding");
  ok(hiddenTab.hidden, "sanity check that it's hidden");

  let extraTab = gBrowser.addTab();
  let state = ss.getBrowserState();
  let stateObj = JSON.parse(state);
  let tabs = stateObj.windows[0].tabs;
  is(tabs.length, 3, "just checking that browser state is correct");
  ok(!tabs[0].hidden, "first tab is visible");
  ok(tabs[1].hidden, "second is hidden");
  ok(!tabs[2].hidden, "third is visible");

  
  tabs[2].hidden = true;

  observeOneRestore(function() {
    let testWindow = Services.wm.getEnumerator("navigator:browser").getNext();
    is(testWindow.gBrowser.visibleTabs.length, 1, "only restored 1 visible tab");
    let tabs = testWindow.gBrowser.tabs;
    ok(!tabs[0].hidden, "first is still visible");
    ok(tabs[1].hidden, "second tab is still hidden");
    ok(tabs[2].hidden, "third tab is now hidden");

    
    gBrowser.removeTab(hiddenTab);
    gBrowser.removeTab(extraTab);
    finish();
  });
  ss.setBrowserState(JSON.stringify(stateObj));
}
