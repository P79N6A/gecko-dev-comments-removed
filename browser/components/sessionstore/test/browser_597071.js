






add_task(function test_close_last_nonpopup_window() {
  
  while (ss.getClosedWindowCount()) {
    ss.forgetClosedWindow(0);
  }

  let oldState = ss.getWindowState(window);

  let popupState = {windows: [
    {tabs: [{entries: []}], isPopup: true, hidden: "toolbar"}
  ]};

  
  ss.setWindowState(window, JSON.stringify(popupState), true);

  
  let win = yield promiseNewWindowLoaded({private: false});
  let tab = win.gBrowser.addTab("http://example.com/");
  yield promiseBrowserLoaded(tab.linkedBrowser);

  
  let state = JSON.parse(ss.getBrowserState());
  is(state.windows.length, 2, "sessionstore knows about this window");

  
  yield promiseWindowClosed(win);
  is(ss.getClosedWindowCount(), 1, "correct closed window count");

  
  ss.setWindowState(window, oldState, true);
});
