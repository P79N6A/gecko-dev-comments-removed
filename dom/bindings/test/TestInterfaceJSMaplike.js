




"use strict";
const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function TestInterfaceJSMaplike() {}

TestInterfaceJSMaplike.prototype = {
  classID: Components.ID("{4bc6f6f3-e005-4f0a-b42d-4d1663a9013a}"),
  contractID: "@mozilla.org/dom/test-interface-js-maplike;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.nsIDOMGlobalPropertyInitializer]),

  init: function(win) { this._win = win; },

  __init: function () {},

  setInternal: function(aKey, aValue) {
    return this.__DOM_IMPL__.__set(aKey, aValue);
  },

  deleteInternal: function(aKey) {
    return this.__DOM_IMPL__.__delete(aKey);
  },

  clearInternal: function() {
    return this.__DOM_IMPL__.__clear();
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TestInterfaceJSMaplike])
