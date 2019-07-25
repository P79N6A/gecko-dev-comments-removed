








































function test() {
  waitForExplicitFinish();
  
  
  Services.prefs.setIntPref("browser.sessionstore.interval", 2000);

  
  waitForSaveState(testBug635418_1);

  
  gBrowser.addTab("about:mozilla");
}

function testBug635418_1() {
  ok(!gBrowser.tabs[0].hidden, "first tab should not be hidden");
  ok(!gBrowser.tabs[1].hidden, "second tab should not be hidden");

  waitForSaveState(testBug635418_2);

  
  gBrowser.hideTab(gBrowser.tabs[1]);
}

function testBug635418_2() {
  let state = JSON.parse(ss.getBrowserState());
  ok(!state.windows[0].tabs[0].hidden, "first tab should still not be hidden");
  ok(state.windows[0].tabs[1].hidden, "second tab should be hidden by now");

  waitForSaveState(testBug635418_3);
  gBrowser.showTab(gBrowser.tabs[1]);
}

function testBug635418_3() {
  let state = JSON.parse(ss.getBrowserState());
  ok(!state.windows[0].tabs[0].hidden, "first tab should still still not be hidden");
  ok(!state.windows[0].tabs[1].hidden, "second tab should not be hidden again");

  done();
}

function done() {
  gBrowser.removeTab(window.gBrowser.tabs[1]);

  Services.prefs.clearUserPref("browser.sessionstore.interval");

  executeSoon(finish);
}
