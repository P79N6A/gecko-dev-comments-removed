




"use strict";



const Cu = Components.utils;
const Ci = Components.interfaces;
let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
let require = devtools.require;
let {_advanceValidate} = require("devtools/styleinspector/rule-view");



let sampleInput = '\\symbol "string" url(somewhere)';

function testInsertion(where, result, testName) {
  do_print(testName);
  equal(_advanceValidate(Ci.nsIDOMKeyEvent.DOM_VK_SEMICOLON, sampleInput, where),
        result, "testing _advanceValidate at " + where);
}

function run_test() {
  testInsertion(4, true, "inside a symbol");
  testInsertion(1, false, "after a backslash");
  testInsertion(8, true, "after whitespace");
  testInsertion(11, false, "inside a string");
  testInsertion(24, false, "inside a URL");
  testInsertion(31, true, "at the end");
}
