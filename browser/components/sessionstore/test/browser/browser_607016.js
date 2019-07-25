




































const TAB_STATE_NEEDS_RESTORE = 1;
const TAB_STATE_RESTORING = 2;

let stateBackup = ss.getBrowserState();

function cleanup() {
  
  try {
    Services.prefs.clearUserPref("browser.sessionstore.max_concurrent_tabs");
  } catch (e) {}
  ss.setBrowserState(stateBackup);
  executeSoon(finish);
}

function test() {
  
  waitForExplicitFinish();

  
  
  Services.prefs.setIntPref("browser.sessionstore.max_concurrent_tabs", 0);

  
  let progressListener = {
    onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
      if (aBrowser.__SS_restoreState == TAB_STATE_RESTORING &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)
        progressCallback(aBrowser);
    }
  }

  let state = { windows: [{ tabs: [
    { entries: [{ url: "http://example.org#1" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org#2" }], extData: { "uniq": r() } }, 
    { entries: [{ url: "http://example.org#3" }], extData: { "uniq": r() } }, 
    { entries: [{ url: "http://example.org#4" }], extData: { "uniq": r() } }, 
    { entries: [{ url: "http://example.org#5" }], extData: { "uniq": r() } }, 
    { entries: [{ url: "http://example.org#6" }] } 
  ], selected: 1 }] };

  function progressCallback(aBrowser) {
    
    
    window.gBrowser.removeTabsProgressListener(progressListener);

    let curState = JSON.parse(ss.getBrowserState());
    for (let i = 0; i < curState.windows[0].tabs.length; i++) {
      if (state.windows[0].tabs[i].extData) {
        is(curState.windows[0].tabs[i].extData["uniq"],
           state.windows[0].tabs[i].extData["uniq"],
           "sanity check that tab has correct extData");
      }
      else
        ok(!("extData" in curState.windows[0].tabs[i]),
           "sanity check that tab doesn't have extData");
    }

    
    let newUniq = r();
    ss.setTabValue(gBrowser.tabs[1], "uniq", newUniq);
    gBrowser.removeTab(gBrowser.tabs[1]);
    let closedTabData = (JSON.parse(ss.getClosedTabData(window)))[0];
    is(closedTabData.state.extData.uniq, newUniq,
       "(overwriting) new data is stored in extData");

    
    gBrowser.hideTab(gBrowser.tabs[1]);
    gBrowser.removeTab(gBrowser.tabs[1]);
    closedTabData = (JSON.parse(ss.getClosedTabData(window)))[0];
    ok(closedTabData.state.hidden, "(hiding) tab data has hidden == true");

    
    let stillUniq = r();
    ss.setTabValue(gBrowser.tabs[1], "stillUniq", stillUniq);
    gBrowser.removeTab(gBrowser.tabs[1]);
    closedTabData = (JSON.parse(ss.getClosedTabData(window)))[0];
    is(closedTabData.state.extData.stillUniq, stillUniq,
       "(adding) new data is stored in extData");

    
    ss.deleteTabValue(gBrowser.tabs[1], "uniq");
    gBrowser.removeTab(gBrowser.tabs[1]);
    closedTabData = (JSON.parse(ss.getClosedTabData(window)))[0];
    
    
    if ("extData" in closedTabData.state) {
      ok(!("uniq" in closedTabData.state.extData),
         "(deleting) uniq not in existing extData");
    }
    else {
      ok(true, "(deleting) no data is stored in extData");
    }

    
    let newUniq2 = r();
    ss.setTabValue(gBrowser.tabs[1], "uniq", newUniq2);
    gBrowser.removeTab(gBrowser.tabs[1]);
    closedTabData = (JSON.parse(ss.getClosedTabData(window)))[0];
    is(closedTabData.state.extData.uniq, newUniq2,
       "(creating) new data is stored in extData where there was none");

    cleanup();
  }

  window.gBrowser.addTabsProgressListener(progressListener);
  ss.setBrowserState(JSON.stringify(state));
}

