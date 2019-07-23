




































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const Ci = Components.interfaces;




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

