# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:


































const kKeyFilename = "urlclassifierkey3.txt";

Components.utils.import("resource://gre/modules/osfile.jsm");
Components.utils.import("resource://gre/modules/Promise.jsm");





















function PROT_UrlCryptoKeyManager(opt_keyFilename, opt_testing) {
  this.debugZone = "urlcryptokeymanager";
  this.testing_ = !!opt_testing;
  this.clientKey_ = null;          
  this.clientKeyArray_ = null;     
  this.wrappedKey_ = null;         
  this.rekeyTries_ = 0;
  this.updating_ = false;

  
  this.keyUrl_ = null;

  this.keyFilename_ = opt_keyFilename ? 
                      opt_keyFilename : kKeyFilename;

  this.onNewKey_ = null;

  
  this.MAX_REKEY_TRIES = PROT_UrlCryptoKeyManager.MAX_REKEY_TRIES;
  this.CLIENT_KEY_NAME = PROT_UrlCryptoKeyManager.CLIENT_KEY_NAME;
  this.WRAPPED_KEY_NAME = PROT_UrlCryptoKeyManager.WRAPPED_KEY_NAME;

  if (!this.testing_) {
    this.maybeLoadOldKey();
  }
}


PROT_UrlCryptoKeyManager.MAX_REKEY_TRIES = 2;



PROT_UrlCryptoKeyManager.NEXT_REKEY_PREF = "urlclassifier.keyupdatetime.";


PROT_UrlCryptoKeyManager.KEY_MIN_UPDATE_TIME = 30 * 24 * 60 * 60;


PROT_UrlCryptoKeyManager.CLIENT_KEY_NAME = "clientkey";
PROT_UrlCryptoKeyManager.WRAPPED_KEY_NAME = "wrappedkey";





PROT_UrlCryptoKeyManager.prototype.getClientKey = function() {
  return this.clientKey_;
}







PROT_UrlCryptoKeyManager.prototype.getClientKeyArray = function() {
  return this.clientKeyArray_;
}







PROT_UrlCryptoKeyManager.prototype.getWrappedKey = function() {
  return this.wrappedKey_;
}





PROT_UrlCryptoKeyManager.prototype.setKeyUrl = function(keyUrl) {
  
  if (keyUrl == this.keyUrl_)
    return;

  this.keyUrl_ = keyUrl;
  this.rekeyTries_ = 0;

  
  var prefs = new G_Preferences(PROT_UrlCryptoKeyManager.NEXT_REKEY_PREF);
  var nextRekey = prefs.getPref(this.getPrefName_(this.keyUrl_), 0);
  if (nextRekey < parseInt(Date.now() / 1000, 10)) {
    this.reKey();
  }
}







PROT_UrlCryptoKeyManager.prototype.getPrefName_ = function(url) {
  var queryParam = url.indexOf("?");
  if (queryParam != -1) {
    return url.substring(0, queryParam);
  }
  return url;
}







PROT_UrlCryptoKeyManager.prototype.reKey = function() {
  if (this.updating_) {
    G_Debug(this, "Already re-keying, ignoring this request");
    return true;
  }

  if (this.rekeyTries_ > this.MAX_REKEY_TRIES)
    throw new Error("Have already rekeyed " + this.rekeyTries_ + " times");

  this.rekeyTries_++;

  G_Debug(this, "Attempting to re-key");
  
  if (!this.testing_ && this.keyUrl_) {
    this.fetcher_ = new PROT_XMLFetcher();
    this.fetcher_.get(this.keyUrl_, BindToObject(this.onGetKeyResponse, this));
    this.updating_ = true;

    
    var prefs = new G_Preferences(PROT_UrlCryptoKeyManager.NEXT_REKEY_PREF);
    var nextRekey = parseInt(Date.now() / 1000, 10)
                  + PROT_UrlCryptoKeyManager.KEY_MIN_UPDATE_TIME;
    prefs.setPref(this.getPrefName_(this.keyUrl_), nextRekey);
  }
}









PROT_UrlCryptoKeyManager.prototype.maybeReKey = function() {
  if (this.rekeyTries_ > this.MAX_REKEY_TRIES) {
    G_Debug(this, "Not re-keying; already at max");
    return false;
  }

  this.reKey();
  return true;
}





PROT_UrlCryptoKeyManager.prototype.dropKey = function() {
  this.rekeyTries_ = 0;
  this.replaceKey_(null, null);
}




PROT_UrlCryptoKeyManager.prototype.hasKey = function() {
  return this.clientKey_ != null && this.wrappedKey_ != null;
}

PROT_UrlCryptoKeyManager.prototype.unUrlSafe = function(key)
{
    return key ? key.replace(/-/g, "+").replace(/_/g, "/") : "";
}












PROT_UrlCryptoKeyManager.prototype.replaceKey_ = function(clientKey,
                                                          wrappedKey) {
  if (this.clientKey_)
    G_Debug(this, "Replacing " + this.clientKey_ + " with " + clientKey);

  this.clientKey_ = clientKey;
  this.clientKeyArray_ = Array.map(atob(this.unUrlSafe(clientKey)),
                                   function(c) { return c.charCodeAt(0); });
  this.wrappedKey_ = wrappedKey;

  let promise = this.serializeKey_(this.clientKey_, this.wrappedKey_);

  return promise.then(() => {
    if (this.onNewKey_) {
      this.onNewKey_();
    }
    return true;
  });
}







PROT_UrlCryptoKeyManager.prototype.serializeKey_ = function() {

  var map = {};
  map[this.CLIENT_KEY_NAME] = this.clientKey_;
  map[this.WRAPPED_KEY_NAME] = this.wrappedKey_;

  let keypath = OS.Path.join(OS.Constants.Path.profileDir, this.keyFilename_);

  
  
  if (!this.clientKey_ || !this.wrappedKey_) {
    return OS.File.remove(keypath).then(() => false,
                                         e => {
                                          if (!e.becauseNoSuchFile)
                                            throw e;
                                          return false;
                                         });
  }

  let data = (new G_Protocol4Parser()).serialize(map);

  let encoder = new TextEncoder();
  let array = encoder.encode(data);
  let promise = OS.File.writeAtomic(keypath, array, { tmpPath: keypath + ".tmp",
                                                      flush:   false });
  return promise.then(() => true,
                       e => {
    G_Error(this, "Failed to serialize new key: " + e);
    return false;
  });
}










PROT_UrlCryptoKeyManager.prototype.onGetKeyResponse = function(responseText) {

  var response = (new G_Protocol4Parser).parse(responseText);
  var clientKey = response[this.CLIENT_KEY_NAME];
  var wrappedKey = response[this.WRAPPED_KEY_NAME];

  this.updating_ = false;
  this.fetcher_ = null;

  if (response && clientKey && wrappedKey) {
    G_Debug(this, "Got new key from: " + responseText);
    return this.replaceKey_(clientKey, wrappedKey);
  } else {
    G_Debug(this, "Not a valid response for /newkey");
    return Promise.resolve(false);
  }
}






PROT_UrlCryptoKeyManager.prototype.onNewKey = function(callback) 
{
  this.onNewKey_ = callback;
}













PROT_UrlCryptoKeyManager.prototype.maybeLoadOldKey = function() {
  let keypath = OS.Path.join(OS.Constants.Path.profileDir, this.keyFilename_);

  let decoder = new TextDecoder();
  let promise = OS.File.read(keypath);
  return promise.then(array => {
    let oldKey = decoder.decode(array);
    if (!oldKey) {
      G_Debug(this, "Couldn't find old key.");
      return false;
    }

    oldKey = (new G_Protocol4Parser).parse(oldKey);
    var clientKey = oldKey[this.CLIENT_KEY_NAME];
    var wrappedKey = oldKey[this.WRAPPED_KEY_NAME];

    if (oldKey && clientKey && wrappedKey && !this.hasKey()) {
      G_Debug(this, "Read old key from disk.");
      return this.replaceKey_(clientKey, wrappedKey);
    }
  }, e => {
    G_Debug(this, "Caught " + e + " trying to read keyfile");
    return false;
  });
}

PROT_UrlCryptoKeyManager.prototype.shutdown = function() {
  if (this.fetcher_) {
    this.fetcher_.cancel();
    this.fetcher_ = null;
  }
}
