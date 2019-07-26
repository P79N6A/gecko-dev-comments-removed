


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

  assert.equal(require("panel/page-mod").id, "page-mod",
               "panel/page-mod is the 3rd party");

  
  
  
  assert.equal(require("panel").id, "panel-main",
               "Third party main module overload sdk modules");
  assert.equal(require("panel"),
               require("panel/main"),
               "require(panel) maps to require(panel/main)");
  
  assert.equal(require("./panel").id,
               "local-panel",
               "require(./panel) maps to the local module");

  
  assert.ok(require("sdk/panel").Panel,
            "We can bypass this overloading with absolute path to sdk modules");
  assert.equal(require("sdk/panel"),
               require("addon-kit/panel"),
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
  assert.deepEqual(Object.keys(require('sdk/widget')), ['Widget']);
  assert.equal(require('widget'), require('sdk/widget'));
}

require("sdk/test/runner").runTestsFromModule(module);
