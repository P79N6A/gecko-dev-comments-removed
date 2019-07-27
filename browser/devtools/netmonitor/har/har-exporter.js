


"use strict";

const { Cu, Cc, Ci } = require("chrome");
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
const { defer, resolve } = require("sdk/core/promise");
const { HarUtils } = require("./har-utils.js");
const { HarBuilder } = require("./har-builder.js");

XPCOMUtils.defineLazyGetter(this, "clipboardHelper", function() {
  return Cc["@mozilla.org/widget/clipboardhelper;1"].
    getService(Ci.nsIClipboardHelper);
});

var uid = 1;


const trace = {
  log: function(...args) {
  }
}






const HarExporter = {
  

  




































  save: function(options) {
    
    options.defaultFileName = Services.prefs.getCharPref(
      "devtools.netmonitor.har.defaultFileName");
    options.compress = Services.prefs.getBoolPref(
      "devtools.netmonitor.har.compress");

    
    
    let file = HarUtils.getTargetFile(options.defaultFileName,
      options.jsonp, options.compress);

    if (!file) {
      return resolve();
    }

    trace.log("HarExporter.save; " + options.defaultFileName, options);

    return this.fetchHarData(options).then(jsonString => {
      if (!HarUtils.saveToFile(file, jsonString, options.compress)) {
        let msg = "Failed to save HAR file at: " + options.defaultFileName;
        Cu.reportError(msg);
      }
      return jsonString;
    });
  },

  





  copy: function(options) {
    return this.fetchHarData(options).then(jsonString => {
      clipboardHelper.copyString(jsonString);
      return jsonString;
    });
  },

  

  fetchHarData: function(options) {
    
    options.id = options.id || uid++;

    
    options.jsonp = options.jsonp ||
      Services.prefs.getBoolPref("devtools.netmonitor.har.jsonp");
    options.includeResponseBodies = options.includeResponseBodies ||
      Services.prefs.getBoolPref("devtools.netmonitor.har.includeResponseBodies");
    options.jsonpCallback = options.jsonpCallback ||
      Services.prefs.getCharPref( "devtools.netmonitor.har.jsonpCallback");
    options.forceExport = options.forceExport ||
      Services.prefs.getBoolPref("devtools.netmonitor.har.forceExport");

    
    return this.buildHarData(options).then(har => {
      
      
      if (!har.log.entries.length && !options.forceExport) {
        return resolve();
      }

      let jsonString = this.stringify(har);
      if (!jsonString) {
        return resolve();
      }

      
      if (options.jsonp) {
        
        
        let callbackName = options.jsonpCallback || "onInputData";
        jsonString = callbackName + "(" + jsonString + ");";
      }

      return jsonString;
    }).then(null, function onError(err) {
      Cu.reportError(err);
    });
  },

  





  buildHarData: function(options) {
    
    let builder = new HarBuilder(options);
    return builder.build();
  },

  


  stringify: function(har) {
    if (!har) {
      return null;
    }

    try {
      return JSON.stringify(har, null, "  ");
    }
    catch (err) {
      Cu.reportError(err);
    }
  },
};


exports.HarExporter = HarExporter;
