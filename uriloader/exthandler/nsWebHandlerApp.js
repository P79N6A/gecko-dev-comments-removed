






































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

  launchWithURI: function nWHA__launchWithURI(aURI, aWindowContext) {

    
    
    
    

    
    var escapedUriSpecToHandle = encodeURIComponent(aURI.spec);

    
    var uriToSend = this.uriTemplate.replace("%s", escapedUriSpecToHandle);

    
    var ioService = Components.classes["@mozilla.org/network/io-service;1"].
                    getService(Components.interfaces.nsIIOService);
    var channel = ioService.newChannel(uriToSend, null, null);
    channel.loadFlags = Components.interfaces.nsIChannel.LOAD_DOCUMENT_URI;

    
    var uriLoader = Components.classes["@mozilla.org/uriloader;1"].
                    getService(Components.interfaces.nsIURILoader);
    
    
    
    
    
    uriLoader.openURI(channel, true, aWindowContext);

    return;
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

