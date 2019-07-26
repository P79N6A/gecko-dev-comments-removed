let wrapper = {};
Cu.import("resource:///modules/sessionstore/TabStateCache.jsm", wrapper);
let {TabStateCache} = wrapper;

let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);















function test() {
  waitForExplicitFinish();

  
  let runs = [1, 3, 5, 7];

  function runOneTest() {
    if (runs.length == 0) {
      finish();
      return;
    }

    let pauses = runs.shift();
    testPauses(pauses, runOneTest);
  }

  runOneTest();
}

function testPauses(numPauses, done) {
  let tab = gBrowser.addTab("about:robots");

  
  forceSaveState().then(function() {
    TabStateCache.clear();

    
    const PREF = "browser.sessionstore.interval";
    Services.prefs.setIntPref(PREF, 1000);
    Services.prefs.setIntPref(PREF, 0);

    let tabState = {
      entries: [{url: "http://example.com"}]
    };

    
    function go(n) {
      if (n < numPauses) {
        executeSoon(() => go(n+1));
        return;
      }

      
      ss.setTabState(tab, JSON.stringify(tabState));

      
      waitForTopic("sessionstore-state-write", 1000, function () {
        let state = ss.getTabState(tab);
        ok(state.indexOf("example.com") != -1,
           "getTabState returns correct URL after " + numPauses + " pauses");

        gBrowser.removeTab(tab);
        done();
      });
    }

    go(0);
  }, Cu.reportError);
}
