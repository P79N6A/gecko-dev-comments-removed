





"use strict";

this.EXPORTED_SYMBOLS = ["Logger", "getLogger"];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function Logger(aIdentifier, aEnablingPref) {
  this._identifier = aIdentifier;
  this._enablingPref = aEnablingPref;

  
  this._enabled = !this._enablingPref;

  this.init();
}

Logger.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports, Ci.nsIObserver]),

  init: function Logger_init() {
    if (this._enablingPref) {
      Services.prefs.addObserver(this._enablingPref, this, false);
      this._enabled = Services.prefs.getBoolPref(this._enablingPref);
    }
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "nsPref:changed":
        this._enabled = Services.prefs.getBoolPref(this._enablingPref);
        dump("LogUtils " +
             (this._enabled ? "enabled" : "disabled") +
             " for " + this._identifier + "\n");
        break;

      case "quit-application-granted":
        Services.prefs.removeObserver(this._enablingPref, this);
        break;

      default:
        this.log("Logger observer", "Unknown topic:", aTopic);
        break;
    }
  },

  _generatePrefix: function _generatePrefix() {
    let caller = Components.stack.caller.caller;
    let parts = ['[' + this._identifier + ']'];

    
    if (caller.filename) {
      let path = caller.filename.split('/');
      if (path[path.length - 1].match(/\./)) {
        parts.push(path[path.length - 1])
      } else {
        parts.push(caller.filename);
      }
    }

    
    if (caller.name) {
      parts.push(caller.name + '()');
    }

    parts.push('line ' + caller.lineNumber + ': ');

    return parts.join(' ');
  },

  _generateLogMessage: function _generateLogMessage(severity, argList) {
    let strings = [];
    argList.forEach(function(arg) {
      if (arg === null) {
        strings.push('null');
      } else {
        switch (typeof arg) {
          case 'string':
            strings.push(arg);
            break;
          case 'undefined':
            strings.push('undefined');
            break;
          case 'function':
            strings.push('<<function>>');
            break;
          case 'object':
            try {
              strings.push(JSON.stringify(arg, null, 2));
            } catch (err) {
              strings.push('<<object>>');
            }
            break;
          default:
            try {
              strings.push(arg.toString());
            } catch (err) {
              strings.push('<<something>>');
            }
            break;
        }
      }
    });
    return strings.join(' ');
  },

  




  log: function log(...argList) {
    if (!this._enabled) {
      return;
    }
    let output = this._generatePrefix() + this._generateLogMessage('info', argList);

    
    dump(output + "\n");
    Services.console.logStringMessage(output);
  },

  warning: function Logger_warning(...argList) {
    if (!this._enabled) {
      return;
    }

    let output = this._generatePrefix() + this._generateLogMessage('warning', argList);
  },

  



  error: function Logger_error(...argList) {
    if (!this._enabled) {
      return;
    }

    
    let output = this._generatePrefix() + this._generateLogMessage('error', argList);
    Cu.reportError(output);

    
    dump("ERROR: " + output + "\n");
    dump("   traceback follows:\n");
    for (let frame = Components.stack.caller; frame; frame = frame.caller) {
      dump(frame + "\n");
    }
  }
};






let _loggers = {};

this.getLogger = function(aIdentifier, aEnablingPref) {
  let key = aIdentifier;
  if (aEnablingPref) {
    key = key + '-' + aEnablingPref;
  }
  if (!_loggers[key]) {
    _loggers[key] = new Logger(aIdentifier, aEnablingPref);
  }
  return _loggers[key];
}
