


"use strict";

const { openTab: makeTab, getTabContentWindow } = require("sdk/tabs/utils");

function openTab(rawWindow, url) {
  return new Promise(resolve => {
    let tab = makeTab(rawWindow, url);
    let window = getTabContentWindow(tab);
    if (window.document.readyState == "complete") {
      return resolve();
    }

    window.addEventListener("load", function onLoad() {
      window.removeEventListener("load", onLoad, true);
      resolve();
    }, true);

    return null;
  })
}
exports.openTab = openTab;
