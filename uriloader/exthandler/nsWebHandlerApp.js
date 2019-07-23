





































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const Ci = Components.interfaces;
const Cr = Components.results;




function nsWebHandlerApp() {}

nsWebHandlerApp.prototype = {
  
  

  classDescription: "A web handler for protocols and content",
  classID: Components.ID("8b1ae382-51a9-4972-b930-56977a57919d"),
  contractID: "@mozilla.org/uriloader/web-handler-app;1",

  _name: null,
  _uriTemplate: null,

  
  

  get name() {
    return this._name;
  },

  set name(aName) {
    this._name = aName;
  },

  equals: function(aHandlerApp) {
    if (!aHandlerApp)
      throw Cr.NS_ERROR_NULL_POINTER;

    if (aHandlerApp instanceof Ci.nsIWebHandlerApp &&
        aHandlerApp.uriTemplate &&
        this.uriTemplate &&
        aHandlerApp.uriTemplate == this.uriTemplate)
      return true;

    return false;
  },

  
  

  get uriTemplate() {
    return this._uriTemplate;
  },

  set uriTemplate(aURITemplate) {
    this._uriTemplate = aURITemplate;
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebHandlerApp, Ci.nsIHandlerApp])
};




let components = [nsWebHandlerApp];

function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule(components);
}

