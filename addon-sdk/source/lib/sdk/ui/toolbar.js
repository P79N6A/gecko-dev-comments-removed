


"use strict";

module.metadata = {
  "stability": "experimental",
  "engines": {
    "Firefox": "> 28"
  }
};




try {
  require("chrome").Cu.import("resource:///modules/CustomizableUI.jsm", {});
}
catch (e) {
  throw Error("Unsupported Application: The module"  + module.id +
              " does not support this application.");
}

const { Toolbar } = require("./toolbar/model");
require("./toolbar/view");

exports.Toolbar = Toolbar;
