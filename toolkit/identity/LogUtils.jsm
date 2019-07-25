





"use strict";

const EXPORTED_SYMBOLS = ["Logger"];
const PREF_DEBUG = "toolkit.identity.debug";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function IdentityLogger() {
  Services.prefs.addObserver(PREF_DEBUG, this, false);
  this._debug = Services.prefs.getBoolPref(PREF_DEBUG);
  return this;
}

IdentityLogger.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports, Ci.nsIObserver]),

  observe: function observe(aSubject, aTopic, aData) {
    switch(aTopic) {
      case "nsPref:changed":
        this._debug = Services.prefs.getBoolPref(PREF_DEBUG);
        break;

      case "quit-application-granted":
        Services.prefs.removeObserver(PREF_DEBUG, this);
        break;

      default:
        this.log("Logger observer", "Unknown topic:", aTopic);
        break;
    }
  },

  _generateLogMessage: function _generateLogMessage(aPrefix, ...args) {
    
    let strings = [];

    

    args.forEach(function(arg) {
      if (typeof arg === 'string') {
        strings.push(arg);
      } else if (typeof arg === 'undefined') {
        strings.push('undefined');
      } else if (arg === null) {
        strings.push('null');
      } else {
        try {
          strings.push(JSON.stringify(arg, null, 2));
        } catch(err) {
          strings.push("<<something>>");
        }
      }
    });
    return 'Identity ' + aPrefix + ': ' + strings.join(' : ');
  },

  





  log: function log(aPrefix, ...args) {
    if (!this._debug) {
      return;
    }
    if (typeof this === 'undefined') {
      for (var frame=Components.stack; frame; frame = frame.caller) {
        dump (frame + "\n");
      }
    }
    let output = this._generateLogMessage(aPrefix, args);
    dump(output + "\n");

    
    Services.console.logStringMessage(output);
  },

  



  reportError: function reportError(aPrefix, ...aArgs) {
    let prefix = aPrefix + ' ERROR';

    
    let output = this._generateLogMessage(aPrefix, aArgs);
    Cu.reportError("Identity: " + output);
    dump(output + "\n");
  }

};

let Logger = new IdentityLogger();
