



"use strict";

const {Cc, Ci, Cu, Cm, components} = require('chrome');
Cu.import("resource://gre/modules/AddonManager.jsm", this);

exports.testSelf = function(test) {
  var self = require("sdk/self");

  var source = self.data.load("test-content-symbiont.js");
  test.assert(source.match(/test-content-symbiont/), "self.data.load() works");

  
  
  
  var url = self.data.url("test-content-symbiont.js");
  test.assertEqual(typeof(url), "string", "self.data.url('x') returns string");
  test.assertEqual(/\/test-content-symbiont\.js$/.test(url), true);

  
  url = self.data.url();
  test.assertEqual(typeof(url), "string", "self.data.url() returns string");
  test.assertEqual(/\/undefined$/.test(url), false);

  
  
  test.assert(self.name == "addon-sdk", "self.name is addon-sdk");

  
  test.assertEqual(self.loadReason, "startup",
                   "self.loadReason is always `startup` on test runs");

  test.assertEqual(self.isPrivateBrowsingSupported, false,
                   'usePrivateBrowsing property is false by default');
};

exports.testSelfID = function(test) {
  test.waitUntilDone();

  var self = require("sdk/self");
  
  
  
  
  test.assertEqual(typeof(self.id), "string", "self.id is a string");
  test.assert(self.id.length > 0);

  AddonManager.getAddonByID(self.id, function(addon) {
    if (!addon) {
      test.fail("did not find addon with self.id");
    }
    else {
      test.pass("found addon with self.id");
    }
    test.done();
  });
}
