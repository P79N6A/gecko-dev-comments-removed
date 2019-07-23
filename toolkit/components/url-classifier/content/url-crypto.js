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
#   Monica Chew <mmc@google.com>
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
























function PROT_UrlCrypto() {
  this.debugZone = "urlcrypto";
  this.hasher_ = new G_CryptoHasher();
  this.base64_ = new G_Base64();
  this.streamCipher_ = Cc["@mozilla.org/security/streamcipher;1"]
                       .createInstance(Ci.nsIStreamCipher);

  if (!this.manager_) {
    
    
    
    
    new PROT_UrlCryptoKeyManager();
  }

  
  this.VERSION = PROT_UrlCrypto.VERSION;
  this.RC4_DISCARD_BYTES = PROT_UrlCrypto.RC4_DISCARD_BYTES;
  this.VERSION_QUERY_PARAM_NAME = PROT_UrlCrypto.QPS.VERSION_QUERY_PARAM_NAME;
  this.ENCRYPTED_PARAMS_PARAM_NAME = 
    PROT_UrlCrypto.QPS.ENCRYPTED_PARAMS_PARAM_NAME;
  this.COUNT_QUERY_PARAM_NAME = PROT_UrlCrypto.QPS.COUNT_QUERY_PARAM_NAME;
  this.WRAPPEDKEY_QUERY_PARAM_NAME = 
    PROT_UrlCrypto.QPS.WRAPPEDKEY_QUERY_PARAM_NAME;

  
  this.macer_ = new G_CryptoHasher(); 
  this.macInitialized_ = false;
  
  this.separator_ = ":coolgoog:";
  this.separatorArray_ = this.base64_.arrayifyString(this.separator_);
}


PROT_UrlCrypto.VERSION = "1";

PROT_UrlCrypto.RC4_DISCARD_BYTES = 1600;



PROT_UrlCrypto.QPS = {};
PROT_UrlCrypto.QPS.VERSION_QUERY_PARAM_NAME = "encver";
PROT_UrlCrypto.QPS.ENCRYPTED_PARAMS_PARAM_NAME = "encparams";
PROT_UrlCrypto.QPS.COUNT_QUERY_PARAM_NAME = "nonce";
PROT_UrlCrypto.QPS.WRAPPEDKEY_QUERY_PARAM_NAME = "wrkey";




PROT_UrlCrypto.prototype.getManager = function() {
  return this.manager_;
}












PROT_UrlCrypto.prototype.appendParams_ = function(params) {
  var queryString = "";
  for (var param in params)
    queryString += "&" + param + "=" + encodeURIComponent(params[param]);
                   
  return queryString;
}
















PROT_UrlCrypto.prototype.maybeCryptParams = function(params) {
  if (!this.manager_)
    throw new Error("Need a key manager for UrlCrypto");
  if (typeof params != "object")
    throw new Error("params is an associative array of name/value params");

  var clientKeyArray = this.manager_.getClientKeyArray();
  var wrappedKey = this.manager_.getWrappedKey();

  
  if (!clientKeyArray || !wrappedKey) {
    G_Debug(this, "No key; can't encrypt query params");
    return params;
  }

  
  
  
  var queryString = this.appendParams_(params);
  
  
  var counter = this.getCount_();
  counter = counter & 0xFFFFFFFF;
  
  var encrypted = this.encryptV1(clientKeyArray, 
                                 this.VERSION,
                                 counter,
                                 queryString);

  params = {};
  params[this.VERSION_QUERY_PARAM_NAME] = this.VERSION;
  params[this.COUNT_QUERY_PARAM_NAME] = counter;
  params[this.WRAPPEDKEY_QUERY_PARAM_NAME] = wrappedKey;
  params[this.ENCRYPTED_PARAMS_PARAM_NAME] = encrypted;

  return params;
}

















PROT_UrlCrypto.prototype.encryptV1 = function(clientKeyArray,
                                              version, 
                                              counter,
                                              text) {

  
  if (version != "1") 
    throw new Error("Unknown encryption version");

  var key = this.deriveEncryptionKey(clientKeyArray, counter);

  this.streamCipher_.init(key);

  if (this.RC4_DISCARD_BYTES > 0)
    this.streamCipher_.discard(this.RC4_DISCARD_BYTES);
  
  this.streamCipher_.updateFromString(text);

  var encrypted = this.streamCipher_.finish(true );
  
  
  return encrypted.replace(/\r\n/g, "");
}
  









PROT_UrlCrypto.prototype.deriveEncryptionKey = function(clientKeyArray, 
                                                        count) {
  G_Assert(this, clientKeyArray instanceof Array,
           "Client key should be an array of bytes");
  G_Assert(this, typeof count == "number", "Count should be a number");
  
  
  var paddingArray = [];
  paddingArray.push(count >> 24);
  paddingArray.push((count >> 16) & 0xFF);
  paddingArray.push((count >> 8) & 0xFF);
  paddingArray.push(count & 0xFF);

  this.hasher_.init(G_CryptoHasher.algorithms.MD5);
  this.hasher_.updateFromArray(clientKeyArray);
  this.hasher_.updateFromArray(paddingArray);

  
  var keyFactory = Cc["@mozilla.org/security/keyobjectfactory;1"]
                   .getService(Ci.nsIKeyObjectFactory);
  var key = keyFactory.keyFromString(Ci.nsIKeyObject.RC4,
                                     this.hasher_.digestRaw());
  return key;
}







PROT_UrlCrypto.prototype.getCount_ = function() {
  return ((new Date).getTime() & 0xFFFFFFFF);
}









PROT_UrlCrypto.prototype.initMac = function(opt_clientKeyArray) {
  if (this.macInitialized_) {
    throw new Error("Can't interleave calls to initMac.  Please use another " +
                    "UrlCrypto object.");
  }

  this.macInitialized_ = true;

  var clientKeyArray = null;

  if (!!opt_clientKeyArray) {
    clientKeyArray = opt_clientKeyArray;
  } else {
    clientKeyArray = this.manager_.getClientKeyArray();
  }

  
  
  this.macer_.init(G_CryptoHasher.algorithms.MD5);

  this.macer_.updateFromArray(clientKeyArray);
  this.macer_.updateFromArray(this.separatorArray_);
}







PROT_UrlCrypto.prototype.updateMacFromString = function(s) {
  if (!this.macInitialized_) {
    throw new Error ("Initialize mac first");
  }

  var arr = this.base64_.arrayifyString(s);
  this.macer_.updateFromArray(arr);
}






PROT_UrlCrypto.prototype.finishMac = function(opt_clientKeyArray) {
  var clientKeyArray = null;
  if (!!opt_clientKeyArray) {
    clientKeyArray = opt_clientKeyArray;
  } else {
    clientKeyArray = this.manager_.getClientKeyArray();
  }

  if (!this.macInitialized_) {
    throw new Error ("Initialize mac first");
  }
  this.macer_.updateFromArray(this.separatorArray_);
  this.macer_.updateFromArray(clientKeyArray);

  this.macInitialized_ = false;

  return this.macer_.digestBase64();
}











PROT_UrlCrypto.prototype.computeMac = function(data, 
                                               opt_outputRaw,
                                               opt_clientKeyArray,
                                               opt_separatorArray) {
  var clientKeyArray = null;
  var separatorArray = null;

  
  if (!!opt_clientKeyArray) {
    clientKeyArray = opt_clientKeyArray;
  } else {
    clientKeyArray = this.manager_.getClientKeyArray();
  }

  if (!!opt_separatorArray) {
    separatorArray = opt_separatorArray;
  } else {
    separatorArray = this.separatorArray_;
  }

  this.macer_.init(G_CryptoHasher.algorithms.MD5);

  this.macer_.updateFromArray(clientKeyArray);
  this.macer_.updateFromArray(separatorArray);

  
  
  
  var arr = this.base64_.arrayifyString(data);
  this.macer_.updateFromArray(arr);

  this.macer_.updateFromArray(separatorArray);
  this.macer_.updateFromArray(clientKeyArray);

  if (!!opt_outputRaw) {
    return this.macer_.digestRaw();
  }
  return this.macer_.digestBase64();
}

#ifdef DEBUG


 
function TEST_PROT_UrlCrypto() {
  if (G_GDEBUG) {
    var z = "urlcrypto UNITTEST";
    G_debugService.enableZone(z);

    G_Debug(z, "Starting");

    
    

    var kf = "test.txt";
    function removeTestFile(f) {
      var appDir = new PROT_ApplicationDirectory();
      var file = appDir.getAppDirFileInterface();
      file.append(f);
      if (file.exists())
        file.remove(false  );
    };
    removeTestFile(kf);

    var km = new PROT_UrlCryptoKeyManager(kf, true );

    

    var c = new PROT_UrlCrypto();

    var fakeManager = {
      getClientKeyArray: function() { return null; },
      getWrappedKey: function() { return null; },
    };
    c.manager_ = fakeManager;

    var params = {
      foo: "bar",
      baz: "bomb",
    };
    G_Assert(z, c.maybeCryptParams(params)["foo"] === "bar",
             "How can we encrypt if we don't have a key?");
    c.manager_ = km;
    G_Assert(z, c.maybeCryptParams(params)["foo"] === "bar",
             "Again, how can we encrypt if we don't have a key?");

    
    var realResponse = "clientkey:24:dtmbEN1kgN/LmuEoYifaFw==\n" +
                       "wrappedkey:24:MTpPH3pnLDKihecOci+0W5dk";
    km.onGetKeyResponse(realResponse);
    var crypted = c.maybeCryptParams(params);
    G_Assert(z, crypted["foo"] === undefined, "We have a key but can't crypt");
    G_Assert(z, crypted["bomb"] === undefined, "We have a key but can't crypt");

    
    for (var p in PROT_UrlCrypto.QPS)
      G_Assert(z, crypted[PROT_UrlCrypto.QPS[p]] != undefined, 
               "Output query params doesn't have: " + PROT_UrlCrypto.QPS[p]);
    
    
    var b64 = new G_Base64();
    
    
    function arrayEquals(a1, a2) {
      if (a1.length != a2.length)
        return false;
      
      for (var i = 0; i < a1.length; i++)
        if (typeof a1[i] != typeof a2[i] || a1[i] != a2[i])
          return false;
      return true;
    };

    function arrayAsString(a) {
      var s = "[";
      for (var i = 0; i < a.length; i++)
        s += a[i] + ",";
      return s + "]";
    };

    function printArray(a) {
      var s = arrayAsString(a);
      G_Debug(z, s);
    };

    var keySizeBytes = km.clientKeyArray_.length;

    var startCrypt = (new Date).getTime();
    var numCrypts = 0;

    
    var doLongTest = false;
    if (doLongTest) {
      
      
      
      

      
      for (var i = 0; i < 2 * keySizeBytes; i++) {
        var clientKeyArray = [];
        
        
        for (var j = 0; j < keySizeBytes; j++)
          clientKeyArray[j] = i + j;
        
        
        for (var count = 0; count < 40; count++) {
        
          var payload = "";

          
          for (var payloadPadding = 0; payloadPadding < count; payloadPadding++)
            payload += "a";
          
          var plaintext1 = b64.arrayifyString(payload);
          var plaintext2 = b64.arrayifyString(payload);
          var plaintext3 = b64.arrayifyString(payload);
          
          
          numCrypts++;
          var ciphertext1 = c.encryptV1(clientKeyArray, 
                                      "1", 
                                        count,
                                        plaintext1);
          
          numCrypts++;        
          var ciphertext2 = c.encryptV1(clientKeyArray, 
                                        "1", 
                                        count,
                                        plaintext2);
          
          G_Assert(z, ciphertext1 === ciphertext2,
                   "Two plaintexts having different ciphertexts:" +
                   ciphertext1 + " " + ciphertext2);
          
          numCrypts++;

          
          var ciphertext3 = c.encryptV1(clientKeyArray, 
                                        "1", 
                                        count,
                                        b64.decodeString(ciphertext2), 
                                        true );
          
          G_Assert(z, arrayEquals(plaintext3, b64.decodeString(ciphertext3, 
                                                              true)),
                   "Encryption and decryption not symmetrical");
        }
      }
      
      
      var endCrypt = (new Date).getTime();
      var totalMS = endCrypt - startCrypt;
      G_Debug(z, "Info: Did " + numCrypts + " encryptions in " +
              totalMS + "ms, for an average of " + 
              (totalMS / numCrypts) + "ms per crypt()");
    }
      
    

    var ciphertexts = {}; 
    
    

    ciphertexts[0]="";
    ciphertexts[1]="dA==";
    ciphertexts[2]="akY=";
    ciphertexts[3]="u5mV";
    ciphertexts[4]="bhtioQ==";
    ciphertexts[5]="m2wSZnQ=";
    ciphertexts[6]="zd6gWyDO";
    ciphertexts[7]="bBN0WVrlCg==";
    ciphertexts[8]="Z6U_6bMelFM=";
    ciphertexts[9]="UVoiytL-gHzp";
    ciphertexts[10]="3Xr_ZMmdmvg7zw==";
    ciphertexts[11]="PIIyif7NFRS57mY=";
    ciphertexts[12]="QEKXrRWdZ3poJVSp";
    ciphertexts[13]="T3zsAsooHuAnflNsNQ==";
    ciphertexts[14]="qgYtOJjZSIByo0KtOG0=";
    ciphertexts[15]="NsEGHGK6Ju6FjD59Byai";
    ciphertexts[16]="1RVIsC0HYoUEycoA_0UL2w==";
    ciphertexts[17]="0xXe6Lsb1tZ79T96AJTT-ps=";
    ciphertexts[18]="cVXQCYoA4RV8t1CODXuCS88y";
    ciphertexts[19]="hVf4pd4WP4wPwSyqEXRRkQZSQA==";
    ciphertexts[20]="F6Y9MHwhd1e-bDHhqNSonZbR2Sg=";
    ciphertexts[21]="TiMClYbLUdyYweW8IDytU_HD2wTM";
    ciphertexts[22]="tYQtNqz83KXE4eqn6GhAu6ZZ23SqYw==";
    ciphertexts[23]="qjL-dMpiQ2LYgkYT5IfmE1FlN36wHek=";
    ciphertexts[24]="cL7HHiOZ9PbkvZ9yrJLiv4HXcw4Nf7y7";
    ciphertexts[25]="k4I-fdR6CyzxOpR_QEG5rnvPB8IbmRnpFg==";
    ciphertexts[26]="7LjCfA1dCMjAVT_O8DpiTQ0G7igwQ1HTUMU=";
    ciphertexts[27]="CAtijc6nB-REwAkqimToMn8RC_eZAaJy9Gn4";
    ciphertexts[28]="z8sEB1lDI32wsOkgYbVZ5pxIbpCrha9BmcqxFQ==";
    ciphertexts[29]="2eysfzsfGav0vPRsSnFl8H8fg9dQCT_bSiZwno0=";
    ciphertexts[30]="2BBNlF_mtV9TB2jZHHqCAtzkJQFdVKFn7N8YxsI9";
    ciphertexts[31]="9h4-nldHAr77Boks7lPzsi8TwVCIQzSkiJp2xatbGg==";
    ciphertexts[32]="DHTB8bDTXpUIrZ2ZlAujXLi-501NoWUVIEQJLaKCpqQ=";
    ciphertexts[33]="E9Av2GgnZg_q5r-JLSzM_ShCu1yPF2VeCaQfPPXSSE4I";
    ciphertexts[34]="UJzEucVBnGEfRNBQ6tvbaro0_I_-mQeJMpU2zQnfFdBuFg==";
    ciphertexts[35]="_p0OYras-Vn2rQ9X-J0dFRnhCfytuTEjheUTU7Ueaf1rIA4=";
    ciphertexts[36]="Q0nZXFPJbpx1WZPP-lLPuSGR-pD08B4CAW-6Uf0eEkS05-oM";
    ciphertexts[37]="XeKfieZGc9bPh7nRtCgujF8OY14zbIZSK20Lwg1HTpHi9HfXVQ==";
    
    var clientKeyArray = b64.decodeString("dtmbEN1kgN/LmuEoYifaFw==");
    
    var count = 0xFEDCBA09;
    var plaintext = "http://www.foobar.com/this?is&some=url";
    
    
    

    for (var i = 0; i < plaintext.length; i++) {
      var plaintextArray = b64.arrayifyString(plaintext.substring(0, i));
      var crypted = c.encryptV1(clientKeyArray,
                                "1",
                                count + i,
                                plaintextArray);
      G_Assert(z, crypted === ciphertexts[i], 
               "Generated unexpected ciphertext");

      
      
    }

    
    
    
    var md5texts = [ "",
                     "a", 
                     "abc", 
                     "message digest",
                     "abcdefghijklmnopqrstuvwxyz",
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
                     "12345678901234567890123456789012345678901234567890123456789012345678901234567890"
		   ];

    var md5digests = [ "d41d8cd98f00b204e9800998ecf8427e", 
                       "0cc175b9c0f1b6a831c399e269772661", 
                       "900150983cd24fb0d6963f7d28e17f72", 
                       "f96b697d7cb7938d525a2f31aaf161d0", 
                       "c3fcd3d76192e4007dfb496cca67e13b", 
                       "d174ab98d277d9f5a5611c2c9f419d9f", 
                       "57edf4a22be3c955ac49da2e2107b67a"
		     ];

    var expected_mac = [];
    expected_mac[0] = "ZOJ6Mk+ccC6R7BwseqCYQQ==";
    expected_mac[1] = "zWM7tvcsuH/MSEviNiRbOA==";
    expected_mac[2] = "ZAUVyls/6ZVN3Np8v3pX3g==";
    expected_mac[3] = "Zq6gF7RkPwKqlicuxrO4mg==";
    expected_mac[4] = "/LOJETSnqSW3q4u1hs/0Pg==";
    expected_mac[5] = "jjOEX7H2uchOznxIGuqzJg==";
    expected_mac[6] = "Tje7aP/Rk/gkSH4he0KMQQ==";

    
    var hasher = new G_CryptoHasher();
    for (var i = 0; i < md5texts.length; i++) {
      var computedMac = c.computeMac(md5texts[i], true , [], []);
      var hex = hasher.toHex_(computedMac).toLowerCase();
      G_Assert(z, hex == md5digests[i], 
               "MD5(" + md5texts[i] + ") = " + md5digests[i] + ", not " + hex);
    }

    for (var i = 0; i < md5texts.length; i++) {
      var computedMac = c.computeMac(md5texts[i], 
                                  false ,
                                  clientKeyArray);
      G_Assert(z, computedMac == expected_mac[i], 
               "Wrong mac generated for " + md5texts[i]);
      
      
    }

    
    
    var wholeString = md5texts[0] + md5texts[1];
    var wholeStringMac = c.computeMac(wholeString,
                                      false ,
				      clientKeyArray);

    expected_mac = "zWM7tvcsuH/MSEviNiRbOA==";
    c.initMac(clientKeyArray);
    c.updateMacFromString(md5texts[0]);
    c.updateMacFromString(md5texts[1]);
    var piecemealMac = c.finishMac(clientKeyArray);
    G_Assert(z, piecemealMac == wholeStringMac,
             "Computed different values for mac when adding line by line!");
    G_Assert(z, piecemealMac == expected_mac, "Didn't generate expected mac");

    
    
    expected_mac = "iA5vLUidpXAPwfcAH9+8OQ==";
    var set3data = "";
    for (var i = 1; i <= 3; i++) {
      set3data += "+white" + i + ".com\t1\n";
    }
    var computedMac = c.computeMac(set3data, false , clientKeyArray);
    G_Assert(z, expected_mac == computedMac, "Expected " + expected_mac, " got " + computedMac);

    removeTestFile(kf);

    G_Debug(z, "PASS");
  }
}
#endif
