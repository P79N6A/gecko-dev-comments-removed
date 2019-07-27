



"use strict";






this.EXPORTED_SYMBOLS = [ "jsBeautify" ];

const { devtools } = Components.utils.import("resource://gre/modules/devtools/Loader.jsm", {});
const { beautify } = devtools.require("devtools/jsbeautify");
const jsBeautify = beautify.js;
