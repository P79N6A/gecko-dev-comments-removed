





































const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-sync/main.js");

let gProgressBar;
let gCounter = 0;

function onLoad(event) {
  Services.obs.addObserver(increaseProgressBar, "weave:engine:sync:finish", false);
  Services.obs.addObserver(increaseProgressBar, "weave:engine:sync:error", false);
  gProgressBar = document.getElementById('uploadProgressBar');

  if (Services.prefs.getPrefType("services.sync.firstSync") != Ci.nsIPrefBranch.PREF_INVALID) {
    gProgressBar.max = Weave.Engines.getEnabled().length;
    gProgressBar.style.display = "inline";
  }
  else {
    gProgressBar.style.display = "none";
  }
}

function onUnload(event) {
  Services.obs.removeObserver(increaseProgressBar, "weave:engine:sync:finish");
  Services.obs.removeObserver(increaseProgressBar, "weave:engine:sync:error");
}

function increaseProgressBar(){
  gCounter += 1;
  gProgressBar.setAttribute("value", gCounter);
}

function closeTab() {
  window.close();
}
