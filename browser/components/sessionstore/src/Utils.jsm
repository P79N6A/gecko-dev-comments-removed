



"use strict";

this.EXPORTED_SYMBOLS = ["Utils"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm", this);

this.Utils = Object.freeze({
  makeURI: function (url) {
    return Services.io.newURI(url, null, null);
  },

  





  hasRootDomain: function (url, domain) {
    let host;

    try {
      host = this.makeURI(url).host;
    } catch (e) {
      
      return false;
    }

    let index = host.indexOf(domain);
    if (index == -1)
      return false;

    if (host == domain)
      return true;

    let prevChar = host[index - 1];
    return (index == (host.length - domain.length)) &&
           (prevChar == "." || prevChar == "/");
  },

  shallowCopy: function (obj) {
    let retval = {};

    for (let key of Object.keys(obj)) {
      retval[key] = obj[key];
    }

    return retval;
  }
});
