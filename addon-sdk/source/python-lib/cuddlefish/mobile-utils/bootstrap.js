


"use strict";

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

let { log } = console;

function startup(data, reason) {
  
  try {
    Cu.import("resource://gre/modules/ctypes.jsm");
    let libdvm = ctypes.open("libdvm.so");
    let dvmStdioConverterStartup;
    
    
    try {
      dvmStdioConverterStartup = libdvm.declare("_Z24dvmStdioConverterStartupv", ctypes.default_abi, ctypes.bool);
    }
    catch(e) {
      
      dvmStdioConverterStartup = libdvm.declare("dvmStdioConverterStartup", ctypes.default_abi, ctypes.void_t);
    }
    dvmStdioConverterStartup();
    log("MU: console redirected to adb logcat.");
  } catch(e) {
    Cu.reportError("MU: unable to execute jsctype hack: "+e);
  }

  try {
    let QuitObserver = {
      observe: function (aSubject, aTopic, aData) {
        Services.obs.removeObserver(QuitObserver, "quit-application");
        dump("MU: APPLICATION-QUIT\n");
      }
    };
    Services.obs.addObserver(QuitObserver, "quit-application", false);
    log("MU: ready to watch firefox exit.");
  } catch(e) {
    log("MU: unable to register quit-application observer: " + e);
  }
}

function install() {}
function shutdown() {}
