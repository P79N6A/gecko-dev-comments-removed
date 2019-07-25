



































const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/nsFormAutoCompleteResult.jsm");

function InputListAutoComplete() {}

InputListAutoComplete.prototype = {
  classID       : Components.ID("{bf1e01d0-953e-11df-981c-0800200c9a66}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIInputListAutoComplete]),

  autoCompleteSearch : function (formHistoryResult, aUntrimmedSearchString, aField) {
    let comments = []; 
    let [values, labels] = this.getListSuggestions(aField);
    let historyResults = [];
    let historyComments = [];

    
    
    if (formHistoryResult) {
      entries = formHistoryResult.wrappedJSObject.entries;
      for (let i = 0; i < entries.length; ++i) {
        historyResults.push(entries[i].text);
        historyComments.push("");
      }
    }

    
    
    if (values.length) {
      comments[0] = "separator";
    }
    for (let i = 1; i < values.length; ++i) {
      comments.push("");
    }

    
    let finalValues = historyResults.concat(values);
    let finalLabels = historyResults.concat(labels);
    let finalComments = historyComments.concat(comments);

    return new FormAutoCompleteResult(aUntrimmedSearchString,
                                      Ci.nsIAutoCompleteResult.RESULT_SUCCESS,
                                      0, "", finalValues, finalLabels,
                                      finalComments, formHistoryResult);
  },

  getListSuggestions : function (aField) {
    let values = [];
    let labels = [];

    if (aField) {
      if (aField.list) {
        let options = aField.list.options;
        let length = options.length;
        for (let i = 0; i < length; i++) {
          let item = options.item(i);
          if (item.label) {
            labels.push(item.label);
          } else if (item.text) {
            labels.push(item.text);
          } else {
            labels.push(item.value);
          }
          values.push(item.value);
        }
      }
    }

    return [values, labels];
  }
};

let component = [InputListAutoComplete];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(component);
