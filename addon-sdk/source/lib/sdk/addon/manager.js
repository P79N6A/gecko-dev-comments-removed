


"use strict";

module.metadata = {
  "stability": "experimental"
};

const { AddonManager } = require("resource://gre/modules/AddonManager.jsm");
const { defer } = require("../core/promise");

function getAddonByID(id) {
  let { promise, resolve } = defer();
  AddonManager.getAddonByID(id, resolve);
  return promise;
}
exports.getAddonByID = getAddonByID;
