








function test() {
  waitForExplicitFinish();
	
  var win = OpenBrowserWindow({private: true});

  whenDelayedStartupFinished(win, function() {
    win.gBrowser.loadURI("about:addons");

    waitForFocus(function() {
      EventUtils.synthesizeKey("a", { ctrlKey: true, shiftKey: true }, win);

      is(win.gBrowser.tabs.length, 1, "about:addons tab was re-focused.");
      is(win.gBrowser.currentURI.spec, "about:addons", "Addons tab was opened.");

      win.close();
      finish();    
    });
  });
}


