




































const NUM_TABS = 12;

Cu.import("resource://gre/modules/Services.jsm");
let ss = Cc["@mozilla.org/browser/sessionstore;1"].
         getService(Ci.nsISessionStore);

let stateBackup = ss.getBrowserState();


function test() {
  
  waitForExplicitFinish();

  let startedTest = false;

  
  
  let wasLoaded = { };
  let restoringTabsCount = 0;
  let uniq2 = { };
  let uniq2Count = 0;
  let state = { windows: [{ tabs: [] }] };
  
  for (let i = 0; i < NUM_TABS; i++) {
    let uniq = r();
    let tabData = {
      entries: [{ url: "http://example.com/#" + i }],
      extData: { "uniq": uniq }
    };
    state.windows[0].tabs.push(tabData);
    wasLoaded[uniq] = false;
  }


  function onSSTabRestoring(aEvent) {
    restoringTabsCount++;
    let uniq = ss.getTabValue(aEvent.originalTarget, "uniq");
    wasLoaded[uniq] = true;

    
    
    if (restoringTabsCount == 1)
      onFirstSSTabRestoring();
    else if (restoringTabsCount == NUM_TABS)
      onLastSSTabRestoring();
  }

  
  
  function onFirstSSTabRestoring() {
    info("onFirstSSTabRestoring...");
    for (let i = gBrowser.tabs.length - 1; i >= 0; i--) {
      let tab = gBrowser.tabs[i];
      let actualUniq = ss.getTabValue(tab, "uniq");
      let expectedUniq = state.windows[0].tabs[i].extData["uniq"];

      if (wasLoaded[actualUniq]) {
        info("tab " + i + ": already restored");
        continue;
      }
      is(actualUniq, expectedUniq, "tab " + i + ": extData was correct");

      
      
      uniq2[actualUniq] = r();
      ss.setTabValue(tab, "uniq2", uniq2[actualUniq]);
      
      
      uniq2Count++;
    }
  }

  function onLastSSTabRestoring() {
    let checked = 0;
    for (let i = 0; i < gBrowser.tabs.length; i++) {
      let tab = gBrowser.tabs[i];
      let uniq = ss.getTabValue(tab, "uniq");

      
      if (uniq in uniq2) {
        is(ss.getTabValue(tab, "uniq2"), uniq2[uniq], "tab " + i + " has correct uniq2 value");
        checked++;
      }
    }
    is(checked, uniq2Count, "checked the same number of uniq2 as we set");
    cleanup();
  }

  function cleanup() {
    
    gBrowser.tabContainer.removeEventListener("SSTabRestoring", onSSTabRestoring, false);
    
    
    
    executeSoon(function() {
      ss.setBrowserState(stateBackup);
      executeSoon(finish);
    });
  }

  
  gBrowser.tabContainer.addEventListener("SSTabRestoring", onSSTabRestoring, false);
  
  ss.setBrowserState(JSON.stringify(state));
}


function r() {
  return "" + Date.now() + Math.random();
}

