



"use strict";












let {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let {require} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
let {CssLogic} = require("devtools/styleinspector/css-logic");










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

let dumpn = msg => dump(msg + "\n");
