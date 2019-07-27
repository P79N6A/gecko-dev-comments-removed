



"use strict";

this.EXPORTED_SYMBOLS = ["TabStateCache"];











this.TabStateCache = Object.freeze({
  







  get: function (browser) {
    return TabStateCacheInternal.get(browser);
  },

  







  update: function (browser, newData) {
    TabStateCacheInternal.update(browser, newData);
  }
});

let TabStateCacheInternal = {
  _data: new WeakMap(),

  







  get: function (browser) {
    return this._data.get(browser.permanentKey);
  },

  







  update: function (browser, newData) {
    let data = this._data.get(browser.permanentKey) || {};

    for (let key of Object.keys(newData)) {
      let value = newData[key];
      if (value === null) {
        delete data[key];
      } else {
        data[key] = value;
      }
    }

    this._data.set(browser.permanentKey, data);
  }
};
