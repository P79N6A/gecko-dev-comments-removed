



"use strict";

this.EXPORTED_SYMBOLS = ["TabAttributes"];




this.TabAttributes = Object.freeze({
  persist: function (name) {
    return TabAttributesInternal.persist(name);
  },

  get: function (tab) {
    return TabAttributesInternal.get(tab);
  },

  set: function (tab, data = {}) {
    TabAttributesInternal.set(tab, data);
  }
});

let TabAttributesInternal = {
  _attrs: new Set(),

  
  
  
  
  _skipAttrs: new Set(["image", "pending"]),

  persist: function (name) {
    if (this._attrs.has(name) || this._skipAttrs.has(name)) {
      return false;
    }

    this._attrs.add(name);
    return true;
  },

  get: function (tab) {
    let data = {};

    for (let name of this._attrs) {
      if (tab.hasAttribute(name)) {
        data[name] = tab.getAttribute(name);
      }
    }

    return data;
  },

  set: function (tab, data = {}) {
    
    for (let name of this._attrs) {
      tab.removeAttribute(name);
    }

    
    for (let name in data) {
      if (!this._skipAttrs.has(name)) {
        tab.setAttribute(name, data[name]);
      }
    }
  }
};

