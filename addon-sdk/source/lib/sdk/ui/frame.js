


"use strict";

module.metadata = {
  "stability": "experimental",
  "engines": {
    "Firefox": "> 28"
  }
};

require("./frame/view");
const { Frame } = require("./frame/model");

exports.Frame = Frame;
