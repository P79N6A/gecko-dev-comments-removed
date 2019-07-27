"use strict";

let stateBackup = ss.getBrowserState();

add_task(function* () {
  
  ignoreAllUncaughtExceptions();

  
  
  Services.prefs.setBoolPref("browser.sessionstore.restore_on_demand", true);

  let state = { windows: [{ tabs: [
    { entries: [{ url: "http://example.org#1" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org#2" }], extData: { "uniq": r() } }, 
    { entries: [{ url: "http://example.org#3" }], extData: { "uniq": r() } }, 
    { entries: [{ url: "http://example.org#4" }], extData: { "uniq": r() } }, 
    { entries: [{ url: "http://example.org#5" }], extData: { "uniq": r() } }, 
    { entries: [{ url: "http://example.org#6" }] } 
  ], selected: 1 }] };

  function* progressCallback() {
    let curState = JSON.parse(ss.getBrowserState());
    for (let i = 0; i < curState.windows[0].tabs.length; i++) {
      let tabState = state.windows[0].tabs[i];
      let tabCurState = curState.windows[0].tabs[i];
      if (tabState.extData) {
        is(tabCurState.extData["uniq"], tabState.extData["uniq"],
           "sanity check that tab has correct extData");
      }
      else {
        
        
        
        ok(!("extData" in tabCurState) || !("uniq" in tabCurState.extData),
           "sanity check that tab doesn't have extData or extData doesn't have 'uniq'");
      }
    }

    
    let newUniq = r();
    ss.setTabValue(gBrowser.tabs[1], "uniq", newUniq);
    yield promiseRemoveTab(gBrowser.tabs[1]);
    let closedTabData = (JSON.parse(ss.getClosedTabData(window)))[0];
    is(closedTabData.state.extData.uniq, newUniq,
       "(overwriting) new data is stored in extData");

    
    gBrowser.hideTab(gBrowser.tabs[1]);
    yield promiseRemoveTab(gBrowser.tabs[1]);
    closedTabData = (JSON.parse(ss.getClosedTabData(window)))[0];
    ok(closedTabData.state.hidden, "(hiding) tab data has hidden == true");

    
    let stillUniq = r();
    ss.setTabValue(gBrowser.tabs[1], "stillUniq", stillUniq);
    yield promiseRemoveTab(gBrowser.tabs[1]);
    closedTabData = (JSON.parse(ss.getClosedTabData(window)))[0];
    is(closedTabData.state.extData.stillUniq, stillUniq,
       "(adding) new data is stored in extData");

    
    ss.deleteTabValue(gBrowser.tabs[1], "uniq");
    yield promiseRemoveTab(gBrowser.tabs[1]);
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
    yield promiseRemoveTab(gBrowser.tabs[1]);
    closedTabData = (JSON.parse(ss.getClosedTabData(window)))[0];
    is(closedTabData.state.extData.uniq, newUniq2,
       "(creating) new data is stored in extData where there was none");
  }

  
  ss.setBrowserState(JSON.stringify(state));

  
  yield Promise.all(Array.map(gBrowser.tabs, tab => {
    return (tab == gBrowser.selectedTab) ?
      promiseTabRestored(tab) : promiseTabRestoring(tab)
  }));

  
  yield progressCallback();

  
  yield promiseBrowserState(stateBackup);
});
