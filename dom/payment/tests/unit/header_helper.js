


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let subscriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
                        .getService(Ci.mozIJSSubScriptLoader);










function newPaymentModule(custom_ns) {
  let payment_ns = {
    importScripts: function fakeImportScripts() {
      Array.slice(arguments).forEach(function (script) {
        subscriptLoader.loadSubScript("resource://gre/modules/" + script, this);
      }, this);
    },
  };

  
  for (let key in custom_ns) {
    payment_ns[key] = custom_ns[key];
  }

  
  payment_ns.importScripts("Payment.jsm");

  return payment_ns;
}
