








































let ss = Cc["@mozilla.org/browser/sessionstore;1"].
         getService(Ci.nsISessionStore);


let tab;






let state = -1;
const URI_TO_LOAD = "about:home";

function test() {
  waitForExplicitFinish();

  
  
  Services.prefs.setIntPref("browser.sessionstore.interval", 2000);

  gBrowser.addTabsProgressListener(tabsListener);

  waitForSaveState(test_bug625257_1);
  tab = gBrowser.addTab();

  tab.linkedBrowser.addEventListener("load", onLoad, true);

  gBrowser.tabContainer.addEventListener("TabClose", onTabClose, true);
}


function test_bug625257_1() {
  is(gBrowser.tabs[1], tab, "newly created tab should exist by now");
  ok(tab.linkedBrowser.__SS_data, "newly created tab should be in save state");

  tab.linkedBrowser.loadURI(URI_TO_LOAD);
  state = 0;
}

let tabsListener = {
  onLocationChange: function onLocationChange(aBrowser) {
    gBrowser.removeTabsProgressListener(tabsListener);
    is(state, 0, "should be the first listener event");
    state++;

    
    
    executeSoon(function() {
      tab.linkedBrowser.removeEventListener("load", onLoad, true);
      gBrowser.removeTab(tab);
    });
  }
};

function onTabClose(aEvent) {
  let uri = aEvent.target.location;

  is(state, 1, "should only remove tab at this point");
  state++;
  gBrowser.tabContainer.removeEventListener("TabClose", onTabClose, true);

  executeSoon(function() {
    tab = ss.undoCloseTab(window, 0);
    tab.linkedBrowser.addEventListener("load", onLoad, true);
  });
}

function onLoad(aEvent) {
  let uri = aEvent.target.location;

  if (state == 2) {
    is(uri, URI_TO_LOAD, "should load page from undoCloseTab");
    done();
  }
  else {
    isnot(uri, URI_TO_LOAD, "should not fully load page");
  }
}

function done() {
  tab.linkedBrowser.removeEventListener("load", onLoad, true);
  gBrowser.removeTab(gBrowser.tabs[1]);
  Services.prefs.clearUserPref("browser.sessionstore.interval");

  executeSoon(finish);
}
