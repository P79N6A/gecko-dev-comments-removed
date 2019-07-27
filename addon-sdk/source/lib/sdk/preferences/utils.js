


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { openTab, getBrowserForTab, getTabId } = require("sdk/tabs/utils");
const { defer, all } = require("sdk/core/promise");
const { on, off } = require("sdk/system/events");
const { setTimeout } = require("sdk/timers");
const { getMostRecentBrowserWindow } = require('../window/utils');

const open = function open({ id }) {
  let showing = defer();
  let loaded = defer();
  let result = { id: id };
  let tab = openTab(getMostRecentBrowserWindow(), "about:addons", {
    inBackground: true
  });
  let browser = getBrowserForTab(tab);

  browser.addEventListener("load", function onPageLoad() {
    browser.removeEventListener("load", onPageLoad, true);
    let window = browser.contentWindow;

    
    on("addon-options-displayed", function onPrefDisplayed({ subject: doc, data }) {
      if (data === id) {
        off("addon-options-displayed", onPrefDisplayed);
        result.tabId = getTabId(tab);
        result.document = doc;
        loaded.resolve();
      }
    }, true);

    
    window.gViewController.commands.cmd_showItemDetails.doCommand({ id: id }, true);
    let { node } = window.gViewController.viewObjects.detail;
    node.addEventListener("ViewChanged", function whenViewChanges() {
      node.removeEventListener("ViewChanged", whenViewChanges, false);
      showing.resolve();
    }, false);
  }, true);

  return all([ showing.promise, loaded.promise ]).then(_ => result);
}
exports.open = open;
