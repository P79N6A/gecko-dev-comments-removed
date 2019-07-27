



"use strict";

this.EXPORTED_SYMBOLS = ["TabStateCache"];











this.TabStateCache = Object.freeze({
  








  get: function (browserOrTab) {
    return TabStateCacheInternal.get(browserOrTab);
  },

  








  update: function (browserOrTab, newData) {
    TabStateCacheInternal.update(browserOrTab, newData);
  }
});

let TabStateCacheInternal = {
  _data: new WeakMap(),

  








  get: function (browserOrTab) {
    return this._data.get(browserOrTab.permanentKey);
  },

  








  update: function (browserOrTab, newData) {
    let data = this._data.get(browserOrTab.permanentKey) || {};

    for (let key of Object.keys(newData)) {
      let value = newData[key];
      if (value === null) {
        delete data[key];
      } else {
        data[key] = value;
      }
    }

    this._data.set(browserOrTab.permanentKey, data);
  }
};
