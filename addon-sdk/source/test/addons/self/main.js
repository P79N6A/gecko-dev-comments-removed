



"use strict";

const self = require("sdk/self");

exports["test self.data.load"] = assert => {

  assert.equal(self.data.load("data.md"),
               "# hello world\n",
               "paths work");

  assert.equal(self.data.load("./data.md"),
               "# hello world\n",
               "relative paths work");
};

require("sdk/test/runner").runTestsFromModule(module);
