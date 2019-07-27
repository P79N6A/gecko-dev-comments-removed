





"use strict";

const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const Services = require("Services");
const { Cc, Ci } = require("chrome");

Object.defineProperty(this, "addonManager", {
  get: (function () {
    let cached;
    return () => cached
      ? cached
      : (cached = Cc["@mozilla.org/addons/integration;1"]
                    .getService(Ci.amIAddonManager))
  }())
});

const B2G_ID = "{3c2e2abc-06d4-11e1-ac3b-374f68613e61}";







module.exports = function mapURIToAddonID(uri, id) {
  if (!Services.appinfo
      || Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT
      || Services.appinfo.ID == B2G_ID
      || !uri
      || !addonManager) {
    return false;
  }

  try {
    return addonManager.mapURIToAddonID(uri, id);
  }
  catch (e) {
    DevToolsUtils.reportException("mapURIToAddonID", e);
    return false;
  }
};
