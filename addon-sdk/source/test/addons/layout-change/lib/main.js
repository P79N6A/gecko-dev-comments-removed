


"use strict";

const { isNative } = require("@loader/options");

if (isNative) {
  module.exports = require("./test-toolkit-loader");
}
else {
  module.exports = require("./test-cuddlefish-loader");
}

require("sdk/test/runner").runTestsFromModule(module);
