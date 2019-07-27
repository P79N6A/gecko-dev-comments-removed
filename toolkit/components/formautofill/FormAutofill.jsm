







"use strict";

this.EXPORTED_SYMBOLS = [
  "FormAutofill",
];

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FormAutofillIntegration",
                                  "resource://gre/modules/FormAutofillIntegration.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");




this.FormAutofill = {
  




  get integration() {
    
    this._refreshIntegrations();
    return this.integration;
  },

  

















  registerIntegration: function (aIntegrationFn) {
    this._integrationFns.add(aIntegrationFn);
    this._refreshIntegrations();
  },

  









  unregisterIntegration: function (aIntegrationFn) {
    this._integrationFns.delete(aIntegrationFn);
    this._refreshIntegrations();
  },

  


  _integrationFns: new Set(),

  



  _refreshIntegrations: function () {
    delete this.integration;

    let combined = FormAutofillIntegration;
    for (let integrationFn of this._integrationFns) {
      try {
        
        
        let integration = integrationFn.call(null, combined);

        
        
        
        let descriptors = {};
        for (let name of Object.getOwnPropertyNames(integration)) {
          descriptors[name] = Object.getOwnPropertyDescriptor(integration, name);
        }
        combined = Object.create(combined, descriptors);
      } catch (ex) {
        
        Cu.reportError(ex);
      }
    }

    this.integration = combined;
  },

  








  processRequestAutocomplete: Task.async(function* (aData) {
    let ui = yield FormAutofill.integration.createRequestAutocompleteUI(aData);
    return yield ui.show();
  }),
};
