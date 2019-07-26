



"use strict";

this.EXPORTED_SYMBOLS = ["BrowserUITelemetry"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "UITelemetry",
  "resource://gre/modules/UITelemetry.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
  "resource:///modules/RecentWindow.jsm");

this.BrowserUITelemetry = {
  init: function() {
    UITelemetry.addSimpleMeasureFunction("toolbars",
                                         this.getToolbarMeasures.bind(this));
  },

  getToolbarMeasures: function() {
    
    
    let win = RecentWindow.getMostRecentBrowserWindow({
      private: false,
      allowPopups: false
    });

    
    if (!win) {
      return {};
    }

    let document = win.document;
    let result = {};

    return result;
  },
};
