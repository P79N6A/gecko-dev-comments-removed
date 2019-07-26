



"use strict";

module.metadata = {
  "stability": "unstable"
};

let { emit, on, off } = require("./core");




function open(target, type, options) {
  let output = {};
  let capture = options && options.capture ? true : false;

  target.addEventListener(type, function(event) {
    emit(output, "data", event);
  }, capture);

  return output;
}
exports.open = open;
