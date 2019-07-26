



const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-sync/main.js");

let gProgressBar;
let gCounter = 0;

function onLoad(event) {
  Services.obs.addObserver(onEngineSync, "weave:engine:sync:finish", false);
  Services.obs.addObserver(onEngineSync, "weave:engine:sync:error", false);
  Services.obs.addObserver(onServiceSync, "weave:service:sync:finish", false);
  Services.obs.addObserver(onServiceSync, "weave:service:sync:error", false);

  gProgressBar = document.getElementById('uploadProgressBar');

  if (Services.prefs.getPrefType("services.sync.firstSync") != Ci.nsIPrefBranch.PREF_INVALID) {
    gProgressBar.hidden = false;
  }
  else {
    gProgressBar.hidden = true;
  }
}

function onUnload(event) {
  cleanUpObservers();
}

function cleanUpObservers() {
  try {
    Services.obs.removeObserver(onEngineSync, "weave:engine:sync:finish");
    Services.obs.removeObserver(onEngineSync, "weave:engine:sync:error");
    Services.obs.removeObserver(onServiceSync, "weave:service:sync:finish");
    Services.obs.removeObserver(onServiceSync, "weave:service:sync:error");
  }
  catch (e) {
    
  }
}

function onEngineSync(subject, topic, data) {
  
  
  
  
  if (data == "clients") {
    return;
  }

  if (!gCounter &&
      Services.prefs.getPrefType("services.sync.firstSync") != Ci.nsIPrefBranch.PREF_INVALID) {
    gProgressBar.max = Weave.Service.engineManager.getEnabled().length;
  }

  gCounter += 1;
  gProgressBar.setAttribute("value", gCounter);
}

function onServiceSync(subject, topic, data) {
  
  
  gProgressBar.setAttribute("value", gProgressBar.max);
  cleanUpObservers();
}

function closeTab() {
  window.close();
}
