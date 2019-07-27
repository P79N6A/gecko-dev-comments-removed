


"use strict";

module.metadata = {
  "stability": "experimental"
};

var { Cu } = require("chrome");
var { XulApp } = Cu.import("resource://gre/modules/sdk/system/XulApp.js", {});

Object.keys(XulApp).forEach(k => exports[k] = XulApp[k]);
