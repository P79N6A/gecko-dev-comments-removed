



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

  swapMapEntries: function (map, key, otherKey) {
    
    
    if (!map.has(key)) {
      [key, otherKey] = [otherKey, key];
      if (!map.has(key)) {
        return;
      }
    }

    
    
    let value = map.get(key);
    if (map.has(otherKey)) {
      let otherValue = map.get(otherKey);
      map.set(key, otherValue);
      map.set(otherKey, value);
    } else {
      map.set(otherKey, value);
      map.delete(key);
    }
  },

  
  copy: function (from) {
    let to = {};

    for (let key of Object.keys(from)) {
      to[key] = from[key];
    }

    return to;
  }
});
