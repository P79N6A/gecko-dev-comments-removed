



"use strict";





this.EXPORTED_SYMBOLS = ["setTimeout", "clearTimeout", "setInterval", "clearInterval"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");


let gNextId = 1; 

let gTimerTable = new Map(); 

this.setTimeout = function setTimeout(aCallback, aMilliseconds) {
  let id = gNextId++;
  let args = Array.slice(arguments, 2);
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(function setTimeout_timer() {
    gTimerTable.delete(id);
    aCallback.apply(null, args);
  }, aMilliseconds, timer.TYPE_ONE_SHOT);

  gTimerTable.set(id, timer);
  return id;
}

this.setInterval = function setInterval(aCallback, aMilliseconds) {
  let id = gNextId++;
  let args = Array.slice(arguments, 2);
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(function setInterval_timer() {
    aCallback.apply(null, args);
  }, aMilliseconds, timer.TYPE_REPEATING_SLACK);

  gTimerTable.set(id, timer);
  return id;
}

this.clearInterval = this.clearTimeout = function clearTimeout(aId) {
  if (gTimerTable.has(aId)) {
    gTimerTable.get(aId).cancel();
    gTimerTable.delete(aId);
  }
}
