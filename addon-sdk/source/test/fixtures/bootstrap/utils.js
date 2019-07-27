


"use strict";

const { Cu, Cc, Ci } = require("chrome");
const { evaluate } = require("sdk/loader/sandbox");

const ROOT = require.resolve("sdk/base64").replace("/sdk/base64.js", "");



const BOOTSTRAP_REASONS = {
  APP_STARTUP     : 1,
  APP_SHUTDOWN    : 2,
  ADDON_ENABLE    : 3,
  ADDON_DISABLE   : 4,
  ADDON_INSTALL   : 5,
  ADDON_UNINSTALL : 6,
  ADDON_UPGRADE   : 7,
  ADDON_DOWNGRADE : 8
};

function createBootstrapScope(options) {
  let { uri, id: aId } = options;
  let principal = Cc["@mozilla.org/systemprincipal;1"].
                  createInstance(Ci.nsIPrincipal);

  let bootstrapScope = new Cu.Sandbox(principal, {
    sandboxName: uri,
    wantGlobalProperties: ["indexedDB"],
    addonId: aId,
    metadata: { addonID: aId, URI: uri }
  });

  
  for (let name in BOOTSTRAP_REASONS)
    bootstrapScope[name] = BOOTSTRAP_REASONS[name];

  return bootstrapScope;
}
exports.create = createBootstrapScope;

function evaluateBootstrap(options) {
  let { uri, scope } = options;

  evaluate(scope,
    `${"Components"}.classes['@mozilla.org/moz/jssubscript-loader;1']
                    .createInstance(${"Components"}.interfaces.mozIJSSubScriptLoader)
                    .loadSubScript("${uri}");`, "ECMAv5");
}
exports.evaluate = evaluateBootstrap;
