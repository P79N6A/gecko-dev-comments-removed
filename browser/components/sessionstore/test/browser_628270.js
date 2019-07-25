


function test() {
  let assertNumberOfTabs = function (num, msg) {
    is(gBrowser.tabs.length, num, msg);
  }

  let assertNumberOfVisibleTabs = function (num, msg) {
    is(gBrowser.visibleTabs.length, num, msg);
  }

  let assertNumberOfPinnedTabs = function (num, msg) {
    is(gBrowser._numPinnedTabs, num, msg);
  }

  waitForExplicitFinish();

  
  assertNumberOfTabs(1, "we start off with one tab");

  
  let tab = gBrowser.addTab("about:robots");

  whenTabIsLoaded(tab, function () {
    
    assertNumberOfVisibleTabs(2, "there are two visible tabs");
    gBrowser.showOnlyTheseTabs([gBrowser.tabs[0]]);
    assertNumberOfVisibleTabs(1, "there is one visible tab");
    ok(tab.hidden, "newly created tab is now hidden");

    
    gBrowser.removeTab(tab);
    tab = ss.undoCloseTab(window, 0);

    
    whenTabIsLoaded(tab, function () {
      is(tab.linkedBrowser.currentURI.spec, "about:robots", "restored tab has correct url");

      gBrowser.removeTab(tab);
      finish();
    });
  });
}

function whenTabIsLoaded(tab, callback) {
  tab.linkedBrowser.addEventListener("load", function onLoad() {
    tab.linkedBrowser.removeEventListener("load", onLoad, true);
    callback();
  }, true);
}
