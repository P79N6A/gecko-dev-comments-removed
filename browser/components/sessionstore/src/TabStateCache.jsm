



"use strict";

this.EXPORTED_SYMBOLS = ["TabStateCache"];

const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "Utils",
  "resource:///modules/sessionstore/Utils.jsm");











this.TabStateCache = Object.freeze({
  







  onBrowserContentsSwapped: function(browser, otherBrowser) {
    TabStateCacheInternal.onBrowserContentsSwapped(browser, otherBrowser);
  },

  







  get: function (browser) {
    return TabStateCacheInternal.get(browser);
  },

  







  update: function (browser, newData) {
    TabStateCacheInternal.update(browser, newData);
  }
});

let TabStateCacheInternal = {
  _data: new WeakMap(),

  







  onBrowserContentsSwapped: function(browser, otherBrowser) {
    
    Utils.swapMapEntries(this._data, browser, otherBrowser);
  },

  







  get: function (browser) {
    return this._data.get(browser);
  },

  







  update: function (browser, newData) {
    let data = this._data.get(browser) || {};

    for (let key of Object.keys(newData)) {
      let value = newData[key];
      if (value === null) {
        delete data[key];
      } else {
        data[key] = value;
      }
    }

    this._data.set(browser, data);
  }
};
