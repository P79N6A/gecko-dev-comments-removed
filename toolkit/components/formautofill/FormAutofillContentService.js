










"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function FormAutofillContentService() {
}

FormAutofillContentService.prototype = {
  classID: Components.ID("{ed9c2c3c-3f86-4ae5-8e31-10f71b0f19e6}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFormAutofillContentService]),

  
  requestAutocomplete: function (aForm, aWindow) {
    Services.console.logStringMessage("requestAutocomplete not implemented.");

    
    let event = new aWindow.AutocompleteErrorEvent("autocompleteerror",
                                                   { bubbles: true,
                                                     reason: "disabled" });

    
    Services.tm.currentThread.dispatch(() => aForm.dispatchEvent(event),
                                       Ci.nsIThread.DISPATCH_NORMAL);
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([FormAutofillContentService]);
