










"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FormAutofill",
                                  "resource://gre/modules/FormAutofill.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");




function FormHandler(aForm, aWindow) {
  this.form = aForm;
  this.window = aWindow;
}

FormHandler.prototype = {
  


  form: null,

  


  window: null,

  


  handleRequestAutocomplete: Task.async(function* () {
    
    
    
    let reason = "";
    try {
      let data = this.collectFormElements();

      let ui = yield FormAutofill.integration.createRequestAutocompleteUI(data);
      let result = yield ui.show();

      
      
      reason = result.canceled ? "cancel" : "success";
    } catch (ex) {
      Cu.reportError(ex);
    }

    
    let event = (reason == "success")
                ? new this.window.Event("autocomplete", { bubbles: true })
                : new this.window.AutocompleteErrorEvent("autocompleteerror",
                                                         { bubbles: true,
                                                           reason: reason });
    yield this.waitForTick();
    this.form.dispatchEvent(event);
  }),

  



  collectFormElements: function () {
    let autofillData = {
      sections: [],
    };

    for (let element of this.form.elements) {
      
      if (!(element instanceof Ci.nsIDOMHTMLInputElement)) {
        continue;
      }

      
      let info = element.getAutocompleteInfo();
      if (!info.fieldName || ["on", "off"].indexOf(info.fieldName) != -1) {
        continue;
      }

      
      let section = autofillData.sections
                                .find(s => s.name == info.section);
      if (!section) {
        section = {
          name: info.section,
          addressSections: [],
        };
        autofillData.sections.push(section);
      }

      
      let addressSection = section.addressSections
                                  .find(s => s.addressType == info.addressType);
      if (!addressSection) {
        addressSection = {
          addressType: info.addressType,
          fields: [],
        };
        section.addressSections.push(addressSection);
      }

      
      let field = {
        fieldName: info.fieldName,
        contactType: info.contactType,
      };
      addressSection.fields.push(field);
    }

    return autofillData;
  },

  


  waitForTick: function () {
    return new Promise(function (resolve) {
      Services.tm.currentThread.dispatch(resolve, Ci.nsIThread.DISPATCH_NORMAL);
    });
  },
};





function FormAutofillContentService() {
}

FormAutofillContentService.prototype = {
  classID: Components.ID("{ed9c2c3c-3f86-4ae5-8e31-10f71b0f19e6}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFormAutofillContentService]),

  
  requestAutocomplete: function (aForm, aWindow) {
    new FormHandler(aForm, aWindow).handleRequestAutocomplete()
                                   .catch(Cu.reportError);
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([FormAutofillContentService]);
