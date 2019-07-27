










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

  this.fieldDetails = [];
}

FormHandler.prototype = {
  


  form: null,

  


  window: null,

  












  fieldDetails: null,

  


  handleRequestAutocomplete: Task.async(function* () {
    
    
    
    let reason = "";
    try {
      reason = yield this.promiseRequestAutocomplete();
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

  







  promiseRequestAutocomplete: Task.async(function* () {
    let data = this.collectFormFields();
    if (!data) {
      return "disabled";
    }

    
    let rootDocShell = this.window.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIDocShell)
                                  .sameTypeRootTreeItem
                                  .QueryInterface(Ci.nsIDocShell);
    let frameMM = rootDocShell.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIContentFrameMessageManager);

    
    
    
    
    let promiseRequestAutocompleteResult = new Promise((resolve, reject) => {
      frameMM.addMessageListener("FormAutofill:RequestAutocompleteResult",
        function onResult(aMessage) {
          frameMM.removeMessageListener(
                        "FormAutofill:RequestAutocompleteResult", onResult);
          
          
          if ("exception" in aMessage.data) {
            reject(aMessage.data.exception);
          } else {
            resolve(aMessage.data);
          }
        });
    });

    
    
    frameMM.sendAsyncMessage("FormAutofill:RequestAutocomplete", data);
    let result = yield promiseRequestAutocompleteResult;
    if (result.canceled) {
      return "cancel";
    }

    this.autofillFormFields(result);

    return "success";
  }),

  







  collectFormFields: function () {
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

      
      if (this.fieldDetails.some(f => f.section == info.section &&
                                      f.addressType == info.addressType &&
                                      f.contactType == info.contactType &&
                                      f.fieldName == info.fieldName)) {
        
        return null;
      }
      this.fieldDetails.push({
        section: info.section,
        addressType: info.addressType,
        contactType: info.contactType,
        fieldName: info.fieldName,
        element: element,
      });

      
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

  















  autofillFormFields: function (aAutofillResult) {
    for (let field of aAutofillResult.fields) {
      
      let fieldDetail = this.fieldDetails
                            .find(f => f.section == field.section &&
                                       f.addressType == field.addressType &&
                                       f.contactType == field.contactType &&
                                       f.fieldName == field.fieldName);
      if (!fieldDetail) {
        continue;
      }

      fieldDetail.element.value = field.value;
    }
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
