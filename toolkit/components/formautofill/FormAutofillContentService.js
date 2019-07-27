










"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FormAutofill",
                                  "resource://gre/modules/FormAutofill.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

function FormAutofillContentService() {
}

FormAutofillContentService.prototype = {
  classID: Components.ID("{ed9c2c3c-3f86-4ae5-8e31-10f71b0f19e6}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFormAutofillContentService]),

  
  requestAutocomplete: function (aForm, aWindow) {
    Task.spawn(function* () {
      
      
      
      let reason = "";
      try {
        let ui = yield FormAutofill.integration.createRequestAutocompleteUI({});
        let result = yield ui.show();

        
        
        reason = result.canceled ? "cancel" : "success";
      } catch (ex) {
        Cu.reportError(ex);
      }

      
      let event = (reason == "success")
                  ? new aWindow.Event("autocomplete", { bubbles: true })
                  : new aWindow.AutocompleteErrorEvent("autocompleteerror",
                                                       { bubbles: true,
                                                         reason: reason });

      
      Services.tm.currentThread.dispatch(() => aForm.dispatchEvent(event),
                                         Ci.nsIThread.DISPATCH_NORMAL);
    }.bind(this)).catch(Cu.reportError);
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([FormAutofillContentService]);
