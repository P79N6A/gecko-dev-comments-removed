# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:


































const kKeyFilename = "urlclassifierkey3.txt";





















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

  this.serializeKey_(this.clientKey_, this.wrappedKey_);

  if (this.onNewKey_) {
    this.onNewKey_();
  }
}







PROT_UrlCryptoKeyManager.prototype.serializeKey_ = function() {

  var map = {};
  map[this.CLIENT_KEY_NAME] = this.clientKey_;
  map[this.WRAPPED_KEY_NAME] = this.wrappedKey_;
  
  try {  

    var keyfile = Cc["@mozilla.org/file/directory_service;1"]
                 .getService(Ci.nsIProperties)
                 .get("ProfD", Ci.nsILocalFile); 
    keyfile.append(this.keyFilename_);

    if (!this.clientKey_ || !this.wrappedKey_) {
      keyfile.remove(true);
      return;
    }

    var data = (new G_Protocol4Parser()).serialize(map);

    try {
      var stream = Cc["@mozilla.org/network/file-output-stream;1"]
                   .createInstance(Ci.nsIFileOutputStream);
      stream.init(keyfile,
                  0x02 | 0x08 | 0x20 ,
                  -1 , 0 );
      stream.write(data, data.length);
    } finally {
      stream.close();
    }
    return true;

  } catch(e) {

    G_Error(this, "Failed to serialize new key: " + e);
    return false;

  }
}






 
PROT_UrlCryptoKeyManager.prototype.onGetKeyResponse = function(responseText) {

  var response = (new G_Protocol4Parser).parse(responseText);
  var clientKey = response[this.CLIENT_KEY_NAME];
  var wrappedKey = response[this.WRAPPED_KEY_NAME];

  this.updating_ = false;
  this.fetcher_ = null;

  if (response && clientKey && wrappedKey) {
    G_Debug(this, "Got new key from: " + responseText);
    this.replaceKey_(clientKey, wrappedKey);
  } else {
    G_Debug(this, "Not a valid response for /newkey");
  }
}






PROT_UrlCryptoKeyManager.prototype.onNewKey = function(callback) 
{
  this.onNewKey_ = callback;
}









 
PROT_UrlCryptoKeyManager.prototype.maybeLoadOldKey = function() {
  
  var oldKey = null;
  try {  
    var keyfile = Cc["@mozilla.org/file/directory_service;1"]
                 .getService(Ci.nsIProperties)
                 .get("ProfD", Ci.nsILocalFile); 
    keyfile.append(this.keyFilename_);
    if (keyfile.exists()) {
      try {
        var fis = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
        fis.init(keyfile, 0x01 , 0444, 0);
        var stream = Cc["@mozilla.org/scriptableinputstream;1"]
                     .createInstance(Ci.nsIScriptableInputStream);
        stream.init(fis);
        oldKey = stream.read(stream.available());
      } finally {
        if (stream)
          stream.close();
      }
    }
  } catch(e) {
    G_Debug(this, "Caught " + e + " trying to read keyfile");
    return;
  }
   
  if (!oldKey) {
    G_Debug(this, "Couldn't find old key.");
    return;
  }

  oldKey = (new G_Protocol4Parser).parse(oldKey);
  var clientKey = oldKey[this.CLIENT_KEY_NAME];
  var wrappedKey = oldKey[this.WRAPPED_KEY_NAME];

  if (oldKey && clientKey && wrappedKey && !this.hasKey()) {
    G_Debug(this, "Read old key from disk.");
    this.replaceKey_(clientKey, wrappedKey);
  }
}

PROT_UrlCryptoKeyManager.prototype.shutdown = function() {
  if (this.fetcher_) {
    this.fetcher_.cancel();
    this.fetcher_ = null;
  }
}


#ifdef DEBUG



function TEST_PROT_UrlCryptoKeyManager() {
  if (G_GDEBUG) {
    var z = "urlcryptokeymanager UNITTEST";
    G_debugService.enableZone(z);

    G_Debug(z, "Starting");

    
    var kf = "keytest.txt";

    
    function removeTestFile(f) {
      var file = Cc["@mozilla.org/file/directory_service;1"]
                 .getService(Ci.nsIProperties)
                 .get("ProfD", Ci.nsILocalFile); 
      file.append(f);
      if (file.exists())
        file.remove(false );
    };
    removeTestFile(kf);

    var km = new PROT_UrlCryptoKeyManager(kf, true );

    

    G_Assert(z, !km.hasKey(), "KM already has key?");
    km.maybeLoadOldKey();
    G_Assert(z, !km.hasKey(), "KM loaded nonexistent key?");
    km.onGetKeyResponse(null);
    G_Assert(z, !km.hasKey(), "KM got key from null response?");
    km.onGetKeyResponse("");
    G_Assert(z, !km.hasKey(), "KM got key from empty response?");
    km.onGetKeyResponse("aslkaslkdf:34:a230\nskdjfaljsie");
    G_Assert(z, !km.hasKey(), "KM got key from garbage response?");
    
    var realResponse = "clientkey:24:zGbaDbx1pxoYe7siZYi8VA==\n" +
                       "wrappedkey:24:MTr1oDt6TSOFQDTvKCWz9PEn";
    km.onGetKeyResponse(realResponse);
    
    G_Assert(z, km.hasKey(), "KM couldn't get key from real response?");
    G_Assert(z, km.clientKey_ == "zGbaDbx1pxoYe7siZYi8VA==", 
             "Parsed wrong client key from response?");
    G_Assert(z, km.wrappedKey_ == "MTr1oDt6TSOFQDTvKCWz9PEn", 
             "Parsed wrong wrapped key from response?");

    
    
    km = new PROT_UrlCryptoKeyManager(kf, true );
    G_Assert(z, !km.hasKey(), "KM already has key?");
    km.maybeLoadOldKey();
    G_Assert(z, km.hasKey(), "KM couldn't load existing key from disk?");
    G_Assert(z, km.clientKey_ == "zGbaDbx1pxoYe7siZYi8VA==", 
             "Parsed wrong client key from disk?");
    G_Assert(z, km.wrappedKey_ == "MTr1oDt6TSOFQDTvKCWz9PEn", 
             "Parsed wrong wrapped key from disk?");
    var realResponse2 = "clientkey:24:dtmbEN1kgN/LmuEoYifaFw==\n" +
                        "wrappedkey:24:MTpPH3pnLDKihecOci+0W5dk";
    km.onGetKeyResponse(realResponse2);
    
    G_Assert(z, km.hasKey(), "KM couldn't replace key from server response?");
    G_Assert(z, km.clientKey_ == "dtmbEN1kgN/LmuEoYifaFw==",
             "Replace client key from server failed?");
    G_Assert(z, km.wrappedKey == "MTpPH3pnLDKihecOci+0W5dk", 
             "Replace wrapped key from server failed?");

    

    km = new PROT_UrlCryptoKeyManager(kf, true );
    G_Assert(z, !km.hasKey(), "KM already has key?");
    km.maybeLoadOldKey();
    G_Assert(z, km.hasKey(), "KM couldn't load existing key from disk?");
    G_Assert(z, km.clientKey_ == "dtmbEN1kgN/LmuEoYifaFw==",
             "Replace client on from disk failed?");
    G_Assert(z, km.wrappedKey_ == "MTpPH3pnLDKihecOci+0W5dk", 
             "Replace wrapped key on disk failed?");

    

    km = new PROT_UrlCryptoKeyManager(kf, true );
    km.reKey();
    for (var i = 0; i < km.MAX_REKEY_TRIES; i++)
      G_Assert(z, km.maybeReKey(), "Couldn't rekey?");
    G_Assert(z, !km.maybeReKey(), "Rekeyed when max hit");
    
    removeTestFile(kf);

    G_Debug(z, "PASSED");  

  }
}
#endif
