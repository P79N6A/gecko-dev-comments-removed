



"use strict";

this.EXPORTED_SYMBOLS = ["DocShellCapabilities"];




this.DocShellCapabilities = Object.freeze({
  collect: function (docShell) {
    return DocShellCapabilitiesInternal.collect(docShell);
  },

  restore: function (docShell, disallow) {
    return DocShellCapabilitiesInternal.restore(docShell, disallow);
  },
});




let DocShellCapabilitiesInternal = {
  
  
  
  
  caps: null,

  allCapabilities: function (docShell) {
    if (!this.caps) {
      let keys = Object.keys(docShell);
      this.caps = keys.filter(k => k.startsWith("allow")).map(k => k.slice(5));
    }
    return this.caps;
  },

  collect: function (docShell) {
    let caps = this.allCapabilities(docShell);
    return caps.filter(cap => !docShell["allow" + cap]);
  },

  restore: function (docShell, disallow) {
    let caps = this.allCapabilities(docShell);
    for (let cap of caps)
      docShell["allow" + cap] = !disallow.has(cap);
  },
};
