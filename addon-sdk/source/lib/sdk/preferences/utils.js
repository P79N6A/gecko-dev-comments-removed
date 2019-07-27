


"use strict";

module.metadata = {
  "stability": "unstable"
};

const { openTab, getBrowserForTab, getTabId } = require("sdk/tabs/utils");
const { on, off } = require("sdk/system/events");
const { getMostRecentBrowserWindow } = require('../window/utils');



const open = ({ id }) => new Promise((resolve, reject) => {
  
  let tab = openTab(getMostRecentBrowserWindow(), "about:addons");
  let browser = getBrowserForTab(tab);

  
  browser.addEventListener("load", function onPageLoad() {
    browser.removeEventListener("load", onPageLoad, true);
    let window = browser.contentWindow;

    
    on("addon-options-displayed", function onPrefDisplayed({ subject: doc, data }) {
      if (data === id) {
        off("addon-options-displayed", onPrefDisplayed);
        resolve({
          id: id,
          tabId: getTabId(tab),
          "document": doc
        });
      }
    }, true);

    
    window.gViewController.commands.cmd_showItemDetails.doCommand({ id: id }, true);
  }, true);
});
exports.open = open;
