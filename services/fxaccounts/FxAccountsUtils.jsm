



"use strict";

this.EXPORTED_SYMBOLS = ["FxAccountsUtils"];

this.FxAccountsUtils = Object.freeze({
  
















  copyObjectProperties: function (from, to, opts = {}) {
    let keys = (opts && opts.keys) || Object.keys(from);
    let thisArg = (opts && opts.bind) || to;

    for (let prop of keys) {
      let desc = Object.getOwnPropertyDescriptor(from, prop);

      if (typeof(desc.value) == "function") {
        desc.value = desc.value.bind(thisArg);
      }

      if (desc.get) {
        desc.get = desc.get.bind(thisArg);
      }

      if (desc.set) {
        desc.set = desc.set.bind(thisArg);
      }

      Object.defineProperty(to, prop, desc);
    }
  }
});
