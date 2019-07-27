


"use strict";

this.EXPORTED_SYMBOLS = ["Startup"];

const { utils: Cu, interfaces: Ci, classes: Cc } = Components;
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
const { XulApp } = Cu.import("resource://gre/modules/sdk/system/XulApp.js", {});
const { defer } = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;

const appStartupSrv = Cc["@mozilla.org/toolkit/app-startup;1"]
                       .getService(Ci.nsIAppStartup);

const NAME2TOPIC = {
  'Firefox': 'sessionstore-windows-restored',
  'Fennec': 'sessionstore-windows-restored',
  'SeaMonkey': 'sessionstore-windows-restored',
  'Thunderbird': 'mail-startup-done'
};

var exports = {
  initialized: !appStartupSrv.startingUp
};
this.Startup = exports;

let gOnceInitializedDeferred = defer();
exports.onceInitialized = gOnceInitializedDeferred.promise;


let appStartup = 'final-ui-startup';

if (Startup.initialized) {
  gOnceInitializedDeferred.resolve()
}
else {
  
  
  for (let name of Object.keys(NAME2TOPIC)) {
    if (XulApp.is(name)) {
      appStartup = NAME2TOPIC[name];
      break;
    }
  }

  let listener = function (subject, topic) {
    Services.obs.removeObserver(this, topic);
    Startup.initialized = true;
    Services.tm.currentThread.dispatch(() => gOnceInitializedDeferred.resolve(),
                                       Ci.nsIThread.DISPATCH_NORMAL);
  }

  Services.obs.addObserver(listener, appStartup, false);
}
