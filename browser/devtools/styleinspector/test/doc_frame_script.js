



"use strict";












let {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let {require} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
let {CssLogic} = require("devtools/styleinspector/css-logic");
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});










addMessageListener("Test:GetRulePropertyValue", function(msg) {
  let {name, styleSheetIndex, ruleIndex} = msg.data;
  let value = null;

  dumpn("Getting the value for property name " + name + " in sheet " +
    styleSheetIndex + " and rule " + ruleIndex);

  let sheet = content.document.styleSheets[styleSheetIndex];
  if (sheet) {
    let rule = sheet.cssRules[ruleIndex];
    if (rule) {
      value = rule.style.getPropertyValue(name);
    }
  }

  sendAsyncMessage("Test:GetRulePropertyValue", value);
});








addMessageListener("Test:GetStyleSheetsInfoForNode", function(msg) {
  let target = msg.objects.target;
  let sheets = [];

  let domUtils = Cc["@mozilla.org/inspector/dom-utils;1"]
    .getService(Ci.inIDOMUtils);
  let domRules = domUtils.getCSSStyleRules(target);

  for (let i = 0, n = domRules.Count(); i < n; i++) {
    let sheet = domRules.GetElementAt(i).parentStyleSheet;
    sheets.push({
      href: sheet.href,
      isContentSheet: CssLogic.isContentStylesheet(sheet)
    });
  }

  sendAsyncMessage("Test:GetStyleSheetsInfoForNode", sheets);
});









addMessageListener("Test:GetComputedStylePropertyValue", function(msg) {
  let {selector, pseudo, name} = msg.data;
  let element = content.document.querySelector(selector);
  let value = content.document.defaultView.getComputedStyle(element, pseudo).getPropertyValue(name);
  sendAsyncMessage("Test:GetComputedStylePropertyValue", value);
});










addMessageListener("Test:WaitForComputedStylePropertyValue", function(msg) {
  let {selector, pseudo, name, expected} = msg.data;
  let element = content.document.querySelector(selector);
  waitForSuccess(() => {
    let value = content.document.defaultView.getComputedStyle(element, pseudo)
                                            .getPropertyValue(name);

    return value === expected;
  }).then(() => {
    sendAsyncMessage("Test:WaitForComputedStylePropertyValue");
  })
});


let dumpn = msg => dump(msg + "\n");












function waitForSuccess(validatorFn, name="untitled") {
  let def = promise.defer();

  function wait(validatorFn) {
    if (validatorFn()) {
      def.resolve();
    } else {
      setTimeout(() => wait(validatorFn), 200);
    }
  }
  wait(validatorFn);

  return def.promise;
}
