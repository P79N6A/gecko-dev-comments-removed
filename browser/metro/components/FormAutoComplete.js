



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function LOG() {
  return; 
  let msg = Array.join(arguments, " ");
  dump(msg + "\n");
  Cu.reportError(msg);
}


XPCOMUtils.defineLazyGetter(this, "FAC", function() {
  return Components.classesByID["{c11c21b2-71c9-4f87-a0f8-5e13f50495fd}"]
                   .getService(Ci.nsIFormAutoComplete);
});

function FormAutoComplete() {
  LOG("new FAC");
}

FormAutoComplete.prototype = {
  classDescription: "Form AutoComplete Plus",
  classID: Components.ID("{cccd414c-3ec2-4cc5-9dc4-36c87cc3c4fe}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFormAutoComplete]),

  
  contactTypes: {
    email: /^(?:.*(?:e-?mail|recipients?).*|(send_)?to(_b?cc)?)$/i,
    tel: /^(?:tel(?:ephone)?|.*phone.*)$/i
  },

  checkQueryType: function checkQueryType(aName, aField) {
    
    if (aField && "type" in aField) {
      let type = aField.type;
      if (type && type in this.contactTypes)
        return type;
    }

    
    let props = [aName];
    if (aField) {
      let specialProps = [aField["className"], aField["id"]];
      props = props.concat(specialProps.filter(function(aValue) {
        return aValue;
      }));
    }

    
    for (let [type, regex] in Iterator(this.contactTypes)) {
      if (props.some(function(prop) prop.search(regex) != -1))
        return type;
    }
    return null;
  },

  autoCompleteSearch: function autoCompleteSearch(aName, aQuery, aField, aPrev) {
    if (!Services.prefs.getBoolPref("browser.formfill.enable"))
      return null;

    LOG("autocomplete search", Array.slice(arguments));
    let result = Cc["@mozilla.org/autocomplete/simple-result;1"].createInstance(Ci.nsIAutoCompleteSimpleResult);
    result.setSearchString(aQuery);

    
    let dupCheck = {};

    
    let normal = FAC.autoCompleteSearch(aName, aQuery, aField, aPrev);
    if (normal.matchCount > 0) {
      for (let i = 0; i < normal.matchCount; i++) {
        dupCheck[normal.getValueAt(i)] = true;
        result.appendMatch(normal.getValueAt(i), normal.getCommentAt(i), normal.getImageAt(i), normal.getStyleAt(i));
      }
    }

    
    let type = this.checkQueryType(aName, aField);

    let resultCode = result.matchCount ? "RESULT_SUCCESS" : "RESULT_NOMATCH";
    result.setSearchResult(Ci.nsIAutoCompleteResult[resultCode]);
    return result;
  }
};

let components = [FormAutoComplete];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
