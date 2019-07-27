












"use strict";

this.EXPORTED_SYMBOLS = [
  "FormAutofillIntegration",
];

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "RequestAutocompleteUI",
                                  "resource://gre/modules/RequestAutocompleteUI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");





this.FormAutofillIntegration = {
  





















  createRequestAutocompleteUI: Task.async(function* (aAutofillData) {
    return new RequestAutocompleteUI(aAutofillData);
  }),
};
