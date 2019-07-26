



"use strict";

const Cu = Components.utils;
const ProfilerProps = "chrome://browser/locale/devtools/profiler.properties";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = ["L10N"];




let L10N = {
  





  getStr: function L10N_getStr(name) {
    return this.stringBundle.GetStringFromName(name);
  },

  






  getFormatStr: function L10N_getFormatStr(name, params) {
    return this.stringBundle.formatStringFromName(name, params, params.length);
  }
};

XPCOMUtils.defineLazyGetter(L10N, "stringBundle", function () {
  return Services.strings.createBundle(ProfilerProps);
});