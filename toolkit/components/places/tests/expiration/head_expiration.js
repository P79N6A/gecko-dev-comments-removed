






































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


let (commonFile = do_get_file("../head_common.js", false)) {
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}





function shutdownExpiration()
{
  let expire = Cc["@mozilla.org/places/expiration;1"].getService(Ci.nsIObserver);
  expire.observe(null, "places-will-close-connection", null);
}






function force_expiration_start() {
  Cc["@mozilla.org/places/expiration;1"].getService(Ci.nsISupports);
}









function force_expiration_step(aLimit) {
  const TOPIC_DEBUG_START_EXPIRATION = "places-debug-start-expiration";
  let expire = Cc["@mozilla.org/places/expiration;1"].getService(Ci.nsIObserver);
  expire.observe(null, TOPIC_DEBUG_START_EXPIRATION, aLimit);
}






function setInterval(aNewInterval) {
  Services.prefs.setIntPref("places.history.expiration.interval_seconds", aNewInterval);
}
function getInterval() {
  return Services.prefs.getIntPref("places.history.expiration.interval_seconds");
}
function clearInterval() {
  try {
    Services.prefs.clearUserPref("places.history.expiration.interval_seconds");
  }
  catch(ex) {}
}


function setMaxPages(aNewMaxPages) {
  Services.prefs.setIntPref("places.history.expiration.max_pages", aNewMaxPages);
}
function getMaxPages() {
  return Services.prefs.getIntPref("places.history.expiration.max_pages");
}
function clearMaxPages() {
  try {
    Services.prefs.clearUserPref("places.history.expiration.max_pages");
  }
  catch(ex) {}
}


function setHistoryEnabled(aHistoryEnabled) {
  Services.prefs.setBoolPref("places.history.enabled", aHistoryEnabled);
}
function getHistoryEnabled() {
  return Services.prefs.getBoolPref("places.history.enabled");
}
function clearHistoryEnabled() {
  try {
    Services.prefs.clearUserPref("places.history.enabled");
  }
  catch(ex) {}
}
