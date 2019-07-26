



"use strict";

this.EXPORTED_SYMBOLS = ["PrivacyLevelFilter"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "PrivacyLevel",
  "resource:///modules/sessionstore/PrivacyLevel.jsm");









function checkPrivacyLevel(url, isPinned) {
  let isHttps = url.startsWith("https:");
  return PrivacyLevel.canSave({isHttps: isHttps, isPinned: isPinned});
}





this.PrivacyLevelFilter = Object.freeze({
  








  filterSessionStorageData: function (data, isPinned) {
    let retval = {};

    for (let host of Object.keys(data)) {
      if (checkPrivacyLevel(host, isPinned)) {
        retval[host] = data[host];
      }
    }

    return Object.keys(retval).length ? retval : null;
  },

  








  filterFormData: function (data, isPinned) {
    
    
    
    if (data.url && !checkPrivacyLevel(data.url, isPinned)) {
      return;
    }

    let retval = {};

    for (let key of Object.keys(data)) {
      if (key === "children") {
        let recurse = child => this.filterFormData(child, isPinned);
        let children = data.children.map(recurse).filter(child => child);

        if (children.length) {
          retval.children = children;
        }
      
      
      } else if (data.url) {
        retval[key] = data[key];
      }
    }

    return Object.keys(retval).length ? retval : null;
  }
});
