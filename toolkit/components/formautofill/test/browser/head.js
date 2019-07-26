








"use strict";

let ChromeUtils = {};
Services.scriptloader.loadSubScript(
  "chrome://mochikit/content/tests/SimpleTest/ChromeUtils.js", ChromeUtils);



let Output = {
  print: info,
};

let Assert = {
  ok: function (actual) {
    let stack = Components.stack.caller;
    ok(actual, "[" + stack.name + " : " + stack.lineNumber + "] " + actual +
               " == true");
  },
  equal: function (actual, expected) {
    let stack = Components.stack.caller;
    is(actual, expected, "[" + stack.name + " : " + stack.lineNumber + "] " +
               actual + " == " + expected);
  },
};



Services.scriptloader.loadSubScript(
  "chrome://mochitests/content/browser/" +
  "toolkit/components/formautofill/test/browser/head_common.js", this);
