


"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {readURISync} = require("sdk/net/url");

const systemPrincipal = Cc["@mozilla.org/systemprincipal;1"].
                        createInstance(Ci.nsIPrincipal);


const FakeCu = function() {
  const sandbox = Cu.Sandbox(systemPrincipal, {wantXrays: false});
  sandbox.toString = function() {
    return "[object BackstagePass]";
  }
  this.sandbox = sandbox;
}
FakeCu.prototype = {
  ["import"](url, scope) {
    const {sandbox} = this;
    sandbox.__URI__ = url;
    const target = Cu.createObjectIn(sandbox);
    target.toString = sandbox.toString;
    Cu.evalInSandbox(`(function(){` + readURISync(url) + `\n})`,
                     sandbox, "1.8", url).call(target);
    
    
    if (!Array.isArray(target.EXPORTED_SYMBOLS)) {
      throw Error("EXPORTED_SYMBOLS is not an array.");
    }

    for (let key of target.EXPORTED_SYMBOLS) {
      scope[key] = target[key];
    }

    return target;
  }
};
exports.FakeCu = FakeCu;
