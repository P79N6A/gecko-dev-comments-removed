# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:















this.PROT_NewXMLHttpRequest = function PROT_NewXMLHttpRequest() {
  var Cc = Components.classes;
  var Ci = Components.interfaces;
  var request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);
  
  request.QueryInterface(Ci.nsIJSXMLHttpRequest);
  return request;
}












this.PROT_XMLFetcher = function PROT_XMLFetcher() {
  this.debugZone = "xmlfetcher";
  this._request = PROT_NewXMLHttpRequest();
  
  this.appId = Ci.nsIScriptSecurityManager.SAFEBROWSING_APP_ID;
  this.isInBrowserElement = false;
  this.usePrivateBrowsing = false;
  this.isContent = false;
}

PROT_XMLFetcher.prototype = {
  


  _callback: null,
  

  





  get: function(page, callback) {
    this._request.abort();                
    this._request = PROT_NewXMLHttpRequest();
    this._callback = callback;
    var asynchronous = true;
    this._request.open("GET", page, asynchronous);
    this._request.channel.notificationCallbacks = this;

    
    var self = this;
    this._request.addEventListener("readystatechange", function() {
      self.readyStateChange(self);
    }, false);

    this._request.send(null);
  },

  cancel: function() {
    this._request.abort();
    this._request = null;
  },

  



  readyStateChange: function(fetcher) {
    if (fetcher._request.readyState != 4)
      return;

    
    
    
    
    
    var responseText = null;
    var status = Components.results.NS_ERROR_NOT_AVAILABLE;
    try {
      G_Debug(this, "xml fetch status code: \"" + 
              fetcher._request.status + "\"");
      status = fetcher._request.status;
      responseText = fetcher._request.responseText;
    } catch(e) {
      G_Debug(this, "Caught exception trying to read xmlhttprequest " +
              "status/response.");
      G_Debug(this, e);
    }
    if (fetcher._callback)
      fetcher._callback(responseText, status);
  },

  
  getInterface: function(iid) {
    return this.QueryInterface(iid);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIInterfaceRequestor,
                                         Ci.nsISupports,
                                         Ci.nsILoadContext])
};
