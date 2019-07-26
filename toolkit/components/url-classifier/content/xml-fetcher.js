# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:


















function PROT_NewXMLHttpRequest() {
  var Cc = Components.classes;
  var Ci = Components.interfaces;
  var request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);
  
  request.QueryInterface(Ci.nsIJSXMLHttpRequest);
  return request;
}











function PROT_XMLFetcher(opt_stripCookies) {
  this.debugZone = "xmlfetcher";
  this._request = PROT_NewXMLHttpRequest();
  this._stripCookies = !!opt_stripCookies;
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

    if (this._stripCookies)
      new PROT_CookieStripper(this._request.channel);

    
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

  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.nsIInterfaceRequestor) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};












function PROT_CookieStripper(channel) {
  this.debugZone = "cookiestripper";
  this.topic_ = "http-on-modify-request";
  this.channel_ = channel;

  var Cc = Components.classes;
  var Ci = Components.interfaces;
  this.observerService_ = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
  this.observerService_.addObserver(this, this.topic_, false);

  
  var twentySeconds = 20 * 1000;
  this.alarm_ = new G_Alarm(BindToObject(this.stopObserving, this), 
                            twentySeconds);
}




PROT_CookieStripper.prototype.observe = function(subject, topic, data) {
  if (topic != this.topic_ || subject != this.channel_)
    return;

  G_Debug(this, "Stripping cookies for channel.");

  this.channel_.QueryInterface(Components.interfaces.nsIHttpChannel);
  this.channel_.setRequestHeader("Cookie", "", false );
  this.alarm_.cancel();
  this.stopObserving();
}




PROT_CookieStripper.prototype.stopObserving = function() {
  G_Debug(this, "Removing observer");
  this.observerService_.removeObserver(this, this.topic_);
  this.channel_ = this.alarm_ = this.observerService_ = null;
}




PROT_CookieStripper.prototype.QueryInterface = function(iid) {
  var Ci = Components.interfaces;
  if (iid.equals(Ci.nsISupports) || iid.equals(Ci.nsIObserve))
    return this;
  throw Components.results.NS_ERROR_NO_INTERFACE;
}

