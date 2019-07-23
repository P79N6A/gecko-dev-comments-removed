# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Google Safe Browsing.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Fritz Schneider <fritz@google.com> (original author)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****


















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

    if (this._stripCookies)
      new PROT_CookieStripper(this._request.channel);

    
    var self = this;
    this._request.onreadystatechange = function() {
      self.readyStateChange(self);
    }

    this._request.send(null);
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

