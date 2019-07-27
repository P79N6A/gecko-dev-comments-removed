


"use strict";

const xulApp = require("sdk/system/xul-app");
const self = require("sdk/self");
const { Loader, main, unload, override } = require("toolkit/loader");
const { PlainTextConsole } = require("sdk/console/plain-text");
const { Loader: CustomLoader } = require("sdk/test/loader");
const loaderOptions = require("@loader/options");

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

exports.testPreferencesBranch = function (assert) {
  let options = override(loaderOptions, {
    preferencesBranch: 'human-readable',
  });
  let loader = CustomLoader(module, { }, options);
  let { preferencesBranch } = loader.require('sdk/self');
  assert.equal(preferencesBranch, 'human-readable',
    'preferencesBranch is human-readable');
}

exports.testInvalidPreferencesBranch = function (assert) {
  let console = new PlainTextConsole(_ => void _);
  let options = override(loaderOptions, {
    preferencesBranch: 'invalid^branch*name',
    id: 'simple@jetpack'
  });
  let loader = CustomLoader(module, { console }, options);
  let { preferencesBranch } = loader.require('sdk/self');
  assert.equal(preferencesBranch, 'simple@jetpack',
    'invalid preferencesBranch value ignored');
}

require("sdk/test").run(exports);
