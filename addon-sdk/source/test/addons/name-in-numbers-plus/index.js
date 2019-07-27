


"use strict";

const { name } = require("sdk/self");

exports["test self.name"] = (assert) => {
  assert.equal(name, "0-0", "using '0-0' works.");
}

require("sdk/test/runner").runTestsFromModule(module);
