


"use strict";

const { packed } = require("sdk/self");
const url = require("sdk/url");

exports["test self.packed"] = function (assert) {
  assert.ok(packed, "require('sdk/self').packed is correct");
}

exports["test url.toFilename"] = function (assert) {
  assert.throws(
      function() { url.toFilename(module.uri); },
      /cannot map to filename: /,
      "url.toFilename() can fail for packed XPIs");
}

require("sdk/test/runner").runTestsFromModule(module);
