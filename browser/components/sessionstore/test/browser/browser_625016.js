


let newWin;

function test() {
  

  
  
  

  waitForExplicitFinish();

  registerCleanupFunction(function() {
  });

  
  
  Services.prefs.setIntPref("browser.sessionstore.interval", 2000);

  
  while(SS_SVC.getClosedWindowCount()) {
    SS_SVC.forgetClosedWindow();
  }

  
  waitForSaveState(onSaveState);
  newWin = openDialog(location, "_blank", "chrome,all,dialog=no", "about:robots");
}

function onSaveState() {
  
  is(SS_SVC.countClosedWindows(), 0, "no closed windows on first save");
  Services.obs.addObserver(observe, "sessionstore-state-write", true);

  
  newWin.close();
}


function observe(aSubject, aTopic, aData) {
  dump(aSubject);
  dump(aData);
  let state = JSON.parse(aData);
  done();
}


function done() {
  Service.prefs.clearUserPref("browser.sessionstore.interval");
  finish();
}
