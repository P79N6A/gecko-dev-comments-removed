


"use strict";

const { Cc, Ci, Cu, Cm, components } = require("chrome");
const xulApp = require("sdk/system/xul-app");
const self = require("sdk/self");

const { AddonManager } = Cu.import("resource://gre/modules/AddonManager.jsm", {});

exports.testSelf = function(assert) {
  
  
  
  var url = self.data.url("test-content-symbiont.js");
  assert.equal(typeof(url), "string", "self.data.url('x') returns string");
  assert.equal(/\/test-content-symbiont\.js$/.test(url), true);

  
  url = self.data.url();
  assert.equal(typeof(url), "string", "self.data.url() returns string");
  assert.equal(/\/undefined$/.test(url), false);

  
  
  assert.equal(self.name, "addon-sdk", "self.name is addon-sdk");

  
  
  let testLoadReason = xulApp.versionInRange(xulApp.platformVersion,
                                             "23.0a1", "*") ? "install"
                                                            : "startup";
  assert.equal(self.loadReason, testLoadReason,
               "self.loadReason is either startup or install on test runs");

  assert.equal(self.isPrivateBrowsingSupported, false,
               'usePrivateBrowsing property is false by default');
};

exports.testSelfID = function(assert, done) {
  var self = require("sdk/self");
  
  
  
  
  assert.equal(typeof(self.id), "string", "self.id is a string");
  assert.ok(self.id.length > 0);

  AddonManager.getAddonByID(self.id, function(addon) {
    if (!addon) {
      assert.fail("did not find addon with self.id");
    }
    else {
      assert.pass("found addon with self.id");
    }

    done();
  });
}

require("sdk/test").run(exports);
