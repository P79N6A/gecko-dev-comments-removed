


"use strict";

const { name } = require("sdk/self");

exports["test self.name"] = (assert) => {
  assert.equal(name, "5", "using '5' works.");
}

require("sdk/test/runner").runTestsFromModule(module);
