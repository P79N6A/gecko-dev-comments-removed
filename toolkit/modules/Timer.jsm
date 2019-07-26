



"use strict";





this.EXPORTED_SYMBOLS = ["setTimeout", "clearTimeout"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");


let gNextTimeoutId = 1; 

let gTimeoutTable = new Map(); 

this.setTimeout = function setTimeout(aCallback, aMilliseconds) {
  let id = gNextTimeoutId++;
  let args = Array.slice(arguments, 2);
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(function setTimeout_timer() {
    gTimeoutTable.delete(id);
    aCallback.apply(null, args);
  }, aMilliseconds, timer.TYPE_ONE_SHOT);

  gTimeoutTable.set(id, timer);
  return id;
}

this.clearTimeout = function clearTimeout(aId) {
  if (gTimeoutTable.has(aId)) {
    gTimeoutTable.get(aId).cancel();
    gTimeoutTable.delete(aId);
  }
}
