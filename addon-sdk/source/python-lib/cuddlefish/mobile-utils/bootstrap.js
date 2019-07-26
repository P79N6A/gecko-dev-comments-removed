



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/Services.jsm");

const DEBUG = false;

let log = DEBUG ? dump : function (){};


function startup(data, reason) {
  
  try {
    Components.utils.import("resource://gre/modules/ctypes.jsm");
    let libdvm = ctypes.open("libdvm.so");
    let dvmStdioConverterStartup;
    
    
    try {
      dvmStdioConverterStartup = libdvm.declare("_Z24dvmStdioConverterStartupv", ctypes.default_abi, ctypes.bool);
    }
    catch(e) {
      
      dvmStdioConverterStartup = libdvm.declare("dvmStdioConverterStartup", ctypes.default_abi, ctypes.void_t);
    }
    dvmStdioConverterStartup();
    log("MU: console redirected to adb logcat.\n");
  } catch(e) {
    Cu.reportError("MU: unable to execute jsctype hack: "+e);
  }

  try {
    let QuitObserver = {
      observe: function (aSubject, aTopic, aData) {
        Services.obs.removeObserver(QuitObserver, "quit-application", false);
        dump("MU: APPLICATION-QUIT\n");
      }
    };
    Services.obs.addObserver(QuitObserver, "quit-application", false);
    log("MU: ready to watch firefox exit.\n");
  } catch(e) {
    log("MU: unable to register quit-application observer: " + e + "\n");
  }
}

function install() {}
function shutdown() {}
