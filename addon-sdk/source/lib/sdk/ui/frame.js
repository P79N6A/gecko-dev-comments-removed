


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

require("./frame/view");
const { Frame } = require("./frame/model");

exports.Frame = Frame;
