











"use strict";

this.EXPORTED_SYMBOLS = ["DownloadsLogger"];
const PREF_DEBUG = "browser.download.debug";

const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

this.DownloadsLogger = {
  _generateLogMessage: function _generateLogMessage(args) {
    
    let strings = [];

    for (let arg of args) {
      if (typeof arg === 'string') {
        strings.push(arg);
      } else if (arg === undefined) {
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
    };
    return 'Downloads: ' + strings.join(' ');
  },

  




  log: function DL_log(...args) {
    let output = this._generateLogMessage(args);
    dump(output + "\n");

    
    Services.console.logStringMessage(output);
  },

  



  reportError: function DL_reportError(...aArgs) {
    
    let output = this._generateLogMessage(aArgs);
    Cu.reportError(output);
    dump("ERROR:" + output + "\n");
    for (let frame = Components.stack.caller; frame; frame = frame.caller) {
      dump("\t" + frame + "\n");
    }
  }

};
