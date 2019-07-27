


"use strict";



exports["test part 1"] = function(assert) {
  let test = require("./addons/addon-manager/lib/test-main.js");
  assert.equal(Object.keys(test).length, 1, "there is only one test");
  assert.ok("test getAddonByID" in test, "the test is corret");
};

exports["test getAddonByID"] = require("./addons/addon-manager/lib/test-main.js")["test getAddonByID"];

require("sdk/test").run(exports);
