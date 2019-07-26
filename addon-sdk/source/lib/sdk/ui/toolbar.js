


"use strict";

module.metadata = {
  "stability": "experimental",
  "engines": {
    "Firefox": "> 28"
  }
};

const { Toolbar } = require("./toolbar/model");
require("./toolbar/view");

exports.Toolbar = Toolbar;
