


"use strict";

exports["test local vs sdk module"] = function (assert) {
  assert.notEqual(require("memory"),
                  require("sdk/deprecated/memory"),
                  "Local module takes the priority over sdk modules");
  assert.ok(require("memory").local,
            "this module is really the local one");
}

exports["test 3rd party vs sdk module"] = function (assert) {
  
  

  
  
  
  

  
  assert.equal(require("page-mod"),
               require("sdk/page-mod"),
               "Third party modules don't overload sdk modules");
  assert.ok(require("page-mod").PageMod,
            "page-mod module is really the sdk one");

  assert.equal(require("tabs/page-mod").id, "page-mod",
               "tabs/page-mod is the 3rd party");

  
  
  
  assert.equal(require("tabs").id, "tabs-main",
               "Third party main module overload sdk modules");
  assert.equal(require("tabs"),
               require("tabs/main"),
               "require(tabs) maps to require(tabs/main)");
  
  assert.equal(require("./tabs").id,
               "local-tabs",
               "require(./tabs) maps to the local module");

  
  assert.ok(require("sdk/tabs").open,
            "We can bypass this overloading with absolute path to sdk modules");
  assert.equal(require("sdk/tabs"),
               require("addon-kit/tabs"),
               "Old and new layout both work");
}





exports.testRelativeRequire = function (assert) {
  assert.equal(require('./same-folder').id, 'same-folder');
}

exports.testRelativeSubFolderRequire = function (assert) {
  assert.equal(require('./sub-folder/module').id, 'sub-folder');
}

exports.testMultipleRequirePerLine = function (assert) {
  var a=require('./multiple/a'),b=require('./multiple/b');
  assert.equal(a.id, 'a');
  assert.equal(b.id, 'b');
}

exports.testSDKRequire = function (assert) {
  assert.deepEqual(Object.keys(require('sdk/page-worker')), ['Page']);
  assert.equal(require('page-worker'), require('sdk/page-worker'));
}

require("sdk/test/runner").runTestsFromModule(module);
