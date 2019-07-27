


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

exports["test minimal bootstrap.js"] = function(assert) {
  let aId = "test-min-boot@jetpack";
  let uri = require.resolve("./fixtures/addon/bootstrap.js");

  let principal = Cc["@mozilla.org/systemprincipal;1"].
                  createInstance(Ci.nsIPrincipal);

  let bootstrapScope = new Cu.Sandbox(principal, {
    sandboxName: uri,
    wantGlobalProperties: ["indexedDB"],
    addonId: aId,
    metadata: { addonID: aId, URI: uri }
  });

  try {
    
    for (let name in BOOTSTRAP_REASONS)
      bootstrapScope[name] = BOOTSTRAP_REASONS[name];

    
    
    
    bootstrapScope.ROOT = ROOT;

    assert.equal(typeof bootstrapScope.install, "undefined", "install DNE");
    assert.equal(typeof bootstrapScope.startup, "undefined", "startup DNE");
    assert.equal(typeof bootstrapScope.shutdown, "undefined", "shutdown DNE");
    assert.equal(typeof bootstrapScope.uninstall, "undefined", "uninstall DNE");

    evaluate(bootstrapScope,
      `${"Components"}.classes['@mozilla.org/moz/jssubscript-loader;1']
                      .createInstance(${"Components"}.interfaces.mozIJSSubScriptLoader)
                      .loadSubScript("${uri}");`, "ECMAv5");

    assert.equal(typeof bootstrapScope.install, "function", "install exists");
    assert.equal(typeof bootstrapScope.startup, "function", "startup exists");
    assert.equal(typeof bootstrapScope.shutdown, "function", "shutdown exists");
    assert.equal(typeof bootstrapScope.uninstall, "function", "uninstall exists");

    bootstrapScope.shutdown(null, BOOTSTRAP_REASONS.ADDON_DISABLE);
  }
  catch(e) {
    console.exception(e)
    assert.fail(e)
  }
}

require("sdk/test").run(exports);
