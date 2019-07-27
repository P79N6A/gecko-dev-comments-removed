


"use strict";

module.metadata = {
  "stability": "experimental"
};

const { XulApp } = require("./xul-app.jsm");

Object.keys(XulApp).forEach(k => exports[k] = XulApp[k]);
