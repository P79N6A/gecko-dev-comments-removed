


"use strict";

const { packed } = require("sdk/self");
const url = require("sdk/url");

exports["test self.packed"] = function (assert) {
  assert.ok(!packed, "require('sdk/self').packed is correct");
}

exports["test url.toFilename"] = function (assert) {
  assert.ok(/.*main\.js$/.test(url.toFilename(module.uri)),
            "url.toFilename() on resource: URIs should work");
}

require("sdk/test/runner").runTestsFromModule(module);
