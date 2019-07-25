






function test() {
  waitForExplicitFinish();

  let testURL = "http://mochi.test:8888/browser/" +
    "browser/components/sessionstore/test/browser_739531_sample.html";

  let loadCount = 0;
  let tab = gBrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function onLoad(aEvent) {
    
    if (++loadCount < 2)
      return;
    tab.linkedBrowser.removeEventListener("load", onLoad, true);

    
    executeSoon(function() {

      let tab2;
      let caughtError = false;
      try {
        tab2 = ss.duplicateTab(window, tab);
      }
      catch (e) {
        caughtError = true;
        info(e);
      }

      is(gBrowser.tabs.length, 3, "there should be 3 tabs")

      ok(!caughtError, "duplicateTab didn't throw");

      
      if (tab2)
        gBrowser.removeTab(tab2);
      gBrowser.removeTab(tab);

      finish();
    });
  }, true);
}
