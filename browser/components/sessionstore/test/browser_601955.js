







































function test() {
  waitForExplicitFinish();
  
  
  Services.prefs.setIntPref("browser.sessionstore.interval", 2000);

  
  waitForSaveState(testBug601955_1);

  
  gBrowser.addTab("about:mozilla");
}

function testBug601955_1() {
  
  
  ok(!gBrowser.tabs[0].pinned, "first tab should not be pinned yet");
  ok(!gBrowser.tabs[1].pinned, "second tab should not be pinned yet");

  waitForSaveState(testBug601955_2);
  gBrowser.pinTab(gBrowser.tabs[0]);
}

function testBug601955_2() {
  let state = JSON.parse(ss.getBrowserState());
  ok(state.windows[0].tabs[0].pinned, "first tab should be pinned by now");
  ok(!state.windows[0].tabs[1].pinned, "second tab should still not be pinned");

  waitForSaveState(testBug601955_3);
  gBrowser.unpinTab(window.gBrowser.tabs[0]);
}

function testBug601955_3() {
  let state = JSON.parse(ss.getBrowserState());
  ok(!state.windows[0].tabs[0].pinned, "first tab should not be pinned");
  ok(!state.windows[0].tabs[1].pinned, "second tab should not be pinned");

  done();
}

function done() {
  gBrowser.removeTab(window.gBrowser.tabs[0]);

  Services.prefs.clearUserPref("browser.sessionstore.interval");

  executeSoon(finish);
}
