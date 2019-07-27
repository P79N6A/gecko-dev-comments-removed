


"use strict";

const { Cc, Ci, Cu, Cm, components } = require("chrome");
const xulApp = require("sdk/system/xul-app");
const self = require("sdk/self");
const { Loader, main, unload } = require("toolkit/loader");
const loaderOptions = require("@loader/options");

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

exports.testSelfHandlesLackingLoaderOptions = function (assert) {
  let root = module.uri.substr(0, module.uri.lastIndexOf('/'));
  let uri = root + '/fixtures/loader/self/';
  let sdkPath = loaderOptions.paths[''] + 'sdk';
  let loader = Loader({ paths: { '': uri, 'sdk': sdkPath }});
  let program = main(loader, 'main');
  let self = program.self;
  assert.pass("No errors thrown when including sdk/self without loader options");
  assert.equal(self.isPrivateBrowsingSupported, false,
    "safely checks sdk/self.isPrivateBrowsingSupported");
  assert.equal(self.packed, false,
    "safely checks sdk/self.packed");
  unload(loader);
};

require("sdk/test").run(exports);
