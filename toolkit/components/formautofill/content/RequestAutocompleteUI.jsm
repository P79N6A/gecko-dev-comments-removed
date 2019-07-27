







"use strict";

this.EXPORTED_SYMBOLS = [
  "RequestAutocompleteUI",
];

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");




this.RequestAutocompleteUI = function (aAutofillData) {
  this._autofillData = aAutofillData;
}

this.RequestAutocompleteUI.prototype = {
  _autofillData: null,

  show: Task.async(function* () {
    
    
    let resolveFn;
    let uiPromise = new Promise(resolve => resolveFn = resolve);

    
    let args = {
      resolveFn: resolveFn,
      autofillData: this._autofillData,
    };
    args.wrappedJSObject = args;

    
    Services.ww.openWindow(null,
                           "chrome://formautofill/content/requestAutocomplete.xhtml",
                           "Toolkit:RequestAutocomplete",
                           "chrome,dialog=no,resizable",
                           args);

    
    return yield uiPromise;
  }),
};
