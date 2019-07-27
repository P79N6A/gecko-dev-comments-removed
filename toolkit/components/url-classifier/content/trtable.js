# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:








this.UrlClassifierTable = function UrlClassifierTable() {
  this.debugZone = "urlclassifier-table";
  this.name = '';
  this.needsUpdate = false;
  this.enchashDecrypter_ = new PROT_EnchashDecrypter();
  this.wrappedJSObject = this;
}

UrlClassifierTable.prototype.QueryInterface = function(iid) {
  if (iid.equals(Components.interfaces.nsISupports) ||
      iid.equals(Components.interfaces.nsIUrlClassifierTable))
    return this;

  throw Components.results.NS_ERROR_NO_INTERFACE;
}




UrlClassifierTable.prototype.exists = function(url, callback) {
  throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
}



this.UrlClassifierTableUrl = function UrlClassifierTableUrl() {
  UrlClassifierTable.call(this);
}

UrlClassifierTableUrl.inherits = function(parentCtor) {
  var tempCtor = function(){};
  tempCtor.prototype = parentCtor.prototype;
  this.superClass_ = parentCtor.prototype;
  this.prototype = new tempCtor();
}
UrlClassifierTableUrl.inherits(UrlClassifierTable);




UrlClassifierTableUrl.prototype.exists = function(url, callback) {
  
  
  
  
  
  
  var urlUtils = Cc["@mozilla.org/url-classifier/utils;1"]
                 .getService(Ci.nsIUrlClassifierUtils);
  var oldCanonicalized = urlUtils.canonicalizeURL(url);
  var canonicalized = this.enchashDecrypter_.getCanonicalUrl(url);
  G_Debug(this, "Looking up: " + url + " (" + oldCanonicalized + " and " +
                canonicalized + ")");
  (new ExistsMultiQuerier([oldCanonicalized, canonicalized],
                          this.name,
                          callback)).run();
}




this.UrlClassifierTableDomain = function UrlClassifierTableDomain() {
  UrlClassifierTable.call(this);
  this.debugZone = "urlclassifier-table-domain";
  this.ioService_ = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService);
}

UrlClassifierTableDomain.inherits = function(parentCtor) {
  var tempCtor = function(){};
  tempCtor.prototype = parentCtor.prototype;
  this.superClass_ = parentCtor.prototype;
  this.prototype = new tempCtor();
}
UrlClassifierTableDomain.inherits(UrlClassifierTable);








UrlClassifierTableDomain.prototype.exists = function(url, callback) {
  var canonicalized = this.enchashDecrypter_.getCanonicalUrl(url);
  var urlObj = this.ioService_.newURI(canonicalized, null, null);
  var host = '';
  try {
    host = urlObj.host;
  } catch (e) { }
  var hostComponents = host.split(".");

  
  
  
  var path = ""
  try {
    urlObj.QueryInterface(Ci.nsIURL);
    path = urlObj.filePath;
  } catch (e) { }

  var pathComponents = path.split("/");

  
  
  var possible = [];
  for (var i = 0; i < hostComponents.length - 1; i++) {
    host = hostComponents.slice(i).join(".");
    possible.push(host);

    
    
    if (pathComponents.length >= 2 && pathComponents[1].length > 0) {
      host = host + "/" + pathComponents[1];
      possible.push(host);
    }
  }

  
  (new ExistsMultiQuerier(possible, this.name, callback)).run();
}




this.UrlClassifierTableEnchash = function UrlClassifierTableEnchash() {
  UrlClassifierTable.call(this);
  this.debugZone = "urlclassifier-table-enchash";
}

UrlClassifierTableEnchash.inherits = function(parentCtor) {
  var tempCtor = function(){};
  tempCtor.prototype = parentCtor.prototype;
  this.superClass_ = parentCtor.prototype;
  this.prototype = new tempCtor();
}
UrlClassifierTableEnchash.inherits(UrlClassifierTable);




UrlClassifierTableEnchash.prototype.exists = function(url, callback) {
  url = this.enchashDecrypter_.getCanonicalUrl(url);
  var host = this.enchashDecrypter_.getCanonicalHost(url,
                                               PROT_EnchashDecrypter.MAX_DOTS);

  var possible = [];
  for (var i = 0; i < PROT_EnchashDecrypter.MAX_DOTS + 1; i++) {
    possible.push(host);

    var index = host.indexOf(".");
    if (index == -1)
      break;
    host = host.substring(index + 1);
  }
  
  (new EnchashMultiQuerier(possible, this.name, callback, url)).run();
}
