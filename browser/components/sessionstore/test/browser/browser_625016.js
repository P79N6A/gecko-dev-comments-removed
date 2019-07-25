


let newWin;
let newTab;

function test() {
  

  
  
  
  
  

  waitForExplicitFinish();

  
  
  Services.prefs.setIntPref("browser.sessionstore.interval", 2000);

  
  
  while(SS_SVC.getClosedWindowCount()) {
    SS_SVC.forgetClosedWindow(0);
  }
  is(SS_SVC.getClosedWindowCount(), 0, "starting with no closed windows");

  
  waitForSaveState(onSaveState);
  newWin = openDialog(location, "_blank", "chrome,all,dialog=no", "about:robots");
}

function onSaveState() {
  
  is(SS_SVC.getClosedWindowCount(), 0, "no closed windows on first save");

  Services.obs.addObserver(observe1, "sessionstore-state-write", false);

  
  newWin.close();
}

function observe1(aSubject, aTopic, aData) {
  info("observe1: " + aTopic);
  switch(aTopic) {
    case "sessionstore-state-write":
      aSubject.QueryInterface(Ci.nsISupportsString);
      let state = JSON.parse(aSubject.data);
      is(state.windows.length, 2,
         "observe1: 2 windows in data being writted to disk");
      is(state._closedWindows.length, 0,
         "observe1: no closed windows in data being writted to disk");

      
      is(SS_SVC.getClosedWindowCount(), 1,
         "observe1: 1 closed window according to API");
      Services.obs.removeObserver(observe1, "sessionstore-state-write", false);
      Services.obs.addObserver(observe1, "sessionstore-state-write-complete", false);
      break;
    case "sessionstore-state-write-complete":
      Services.obs.removeObserver(observe1, "sessionstore-state-write-complete", false);
      openTab();
      break;
  }
}

function observe2(aSubject, aTopic, aData) {
  info("observe2: " + aTopic);
  switch(aTopic) {
    case "sessionstore-state-write":
      aSubject.QueryInterface(Ci.nsISupportsString);
      let state = JSON.parse(aSubject.data);
      is(state.windows.length, 1,
         "observe2: 1 window in data being writted to disk");
      is(state._closedWindows.length, 1,
         "observe2: 1 closed window in data being writted to disk");

      
      is(SS_SVC.getClosedWindowCount(), 1,
         "observe2: 1 closed window according to API");
      Services.obs.removeObserver(observe2, "sessionstore-state-write", false);
      Services.obs.addObserver(observe2, "sessionstore-state-write-complete", false);
      break;
    case "sessionstore-state-write-complete":
      Services.obs.removeObserver(observe2, "sessionstore-state-write-complete", false);
      done();
      break;
  }
}



function openTab() {
  Services.obs.addObserver(observe2, "sessionstore-state-write", false);
  newTab = gBrowser.addTab("about:mozilla");
}

function done() {
  Services.prefs.clearUserPref("browser.sessionstore.interval");
  gBrowser.removeTab(newTab);
  
  
  is(SS_SVC.getClosedWindowCount(), 1, "1 closed window according to API");
  SS_SVC.forgetClosedWindow(0);
  executeSoon(finish);
}

