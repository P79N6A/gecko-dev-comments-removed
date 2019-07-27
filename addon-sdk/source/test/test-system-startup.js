


"use strict";

const { Cu } = require("chrome");
const Startup = Cu.import("resource://gre/modules/sdk/system/Startup.js", {}).exports;

exports["test startup initialized"] = function(assert) {
  assert.ok(Startup.initialized, "Startup.initialized is true");
}

exports["test startup onceInitialized"] = function*(assert) {
  yield Startup.onceInitialized.then(() => {
    assert.pass("onceInitialized promise was resolved");
  }).catch(assert.fail);
}

require('sdk/test').run(exports);
