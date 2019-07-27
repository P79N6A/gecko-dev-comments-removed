


"use strict";

module.metadata = {
  "stability": "unstable"
};

const mime = "application/javascript";
const requireURI = module.uri.replace("framescript/manager.js",
                                      "toolkit/require.js");

const requireLoadURI = `data:${mime},this["Components"].utils.import("${requireURI}")`






const loadModule = (messageManager, id, allowDelayed, init) => {
  const moduleLoadURI = `${requireLoadURI}.require("${id}")`
  const uri = init ? `${moduleLoadURI}.${init}(this)` : moduleLoadURI;
  messageManager.loadFrameScript(uri, allowDelayed);
};
exports.loadModule = loadModule;
