



"use strict";

const self = require("sdk/self");

exports["test self.data.load"] = assert => {

  assert.equal(self.data.load("data.md").trim(),
               "# hello world",
               "paths work");

  assert.equal(self.data.load("./data.md").trim(),
               "# hello world",
               "relative paths work");
};

require("sdk/test/runner").runTestsFromModule(module);
