




"use strict";

module.metadata = {
  "stability": "unstable"
};

const { Cc, Ci } = require("chrome");
const runtime = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime);

exports.inSafeMode = runtime.inSafeMode;
exports.OS = runtime.OS;
exports.processType = runtime.processType;
exports.widgetToolkit = runtime.widgetToolkit;
exports.XPCOMABI = runtime.XPCOMABI;
