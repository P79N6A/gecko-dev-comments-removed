





var loop = loop || {};
var inChrome = typeof Components != "undefined" && "utils" in Components;

(function(rootObject) {
  "use strict";

  var sharedUtils;
  if (inChrome) {
    this.EXPORTED_SYMBOLS = ["LoopCrypto"];
    var Cu = Components.utils;
    Cu.importGlobalProperties(["crypto"]);
    rootObject = {
      crypto: crypto
    };
    sharedUtils = Cu.import("resource:///modules/loop/utils.js", {}).utils;
  } else {
    sharedUtils = this.shared.utils;
  }

  var ALGORITHM = "AES-GCM";
  var KEY_LENGTH = 128;
  
  
  var KEY_FORMAT = "jwk";
  
  var KEY_TYPE = "oct";
  var ENCRYPT_TAG_LENGTH = 128;
  var INITIALIZATION_VECTOR_LENGTH = 12;

  








  function setRootObject(obj) {
    console.log("loop.crpyto.mixins: rootObject set to " + obj);
    rootObject = obj;
  }

  




  function isSupported() {
    return "crypto" in rootObject;
  }

  





  function generateKey() {
    if (!isSupported()) {
      throw new Error("Web Crypto is not supported");
    }

    return new Promise(function(resolve, reject) {
      
      rootObject.crypto.subtle.generateKey({name: ALGORITHM, length: KEY_LENGTH },
        
        true,
        
        ["encrypt", "decrypt"]
      ).then(function(cryptoKey) {
        
        return rootObject.crypto.subtle.exportKey(KEY_FORMAT, cryptoKey);
      }).then(function(exportedKey) {
        
        resolve(exportedKey.k);
      }).catch(function(error) {
        reject(error);
      });
    });
  }

  









  function encryptBytes(key, data) {
    if (!isSupported()) {
      throw new Error("Web Crypto is not supported");
    }

    var iv = new Uint8Array(INITIALIZATION_VECTOR_LENGTH);

    return new Promise(function(resolve, reject) {
      
      rootObject.crypto.subtle.importKey(KEY_FORMAT,
        {k: key, kty: KEY_TYPE},
        ALGORITHM,
        
        true,
        
        ["encrypt"]
      ).then(function(cryptoKey) {
        

        
        var dataBuffer = sharedUtils.strToUint8Array(data);

        
        
        rootObject.crypto.getRandomValues(iv);

        return rootObject.crypto.subtle.encrypt({
            name: ALGORITHM,
            iv: iv,
            tagLength: ENCRYPT_TAG_LENGTH
          }, cryptoKey,
          dataBuffer);
      }).then(function(cipherText) {
        
        var joinedData = _mergeIVandCipherText(iv, new DataView(cipherText));

        
        var encryptedData = sharedUtils.btoa(joinedData);

        resolve(encryptedData);
      }).catch(function(error) {
        reject(error);
      });
    });
  }

  








  function decryptBytes(key, encryptedData) {
    if (!isSupported()) {
      throw new Error("Web Crypto is not supported");
    }

    return new Promise(function(resolve, reject) {
      
      rootObject.crypto.subtle.importKey(KEY_FORMAT,
        {k: key, kty: KEY_TYPE},
        ALGORITHM,
        
        true,
        
        ["decrypt"]
      ).then(function(cryptoKey) {
        
        var splitData = _splitIVandCipherText(encryptedData);

        return rootObject.crypto.subtle.decrypt({
          name: ALGORITHM,
          iv: splitData.iv,
          tagLength: ENCRYPT_TAG_LENGTH
        }, cryptoKey, splitData.cipherText);
      }).then(function(plainText) {
        
        resolve(sharedUtils.Uint8ArrayToStr(new Uint8Array(plainText)));
      }).catch(function(error) {
        reject(error);
      });
    });
  }

  







  function _mergeIVandCipherText(ivArray, cipherTextDataView) {
    
    
    var cipherText = new Uint8Array(cipherTextDataView.buffer);
    var cipherTextLength = cipherText.length;

    var joinedContext = new Uint8Array(INITIALIZATION_VECTOR_LENGTH + cipherTextLength);

    var i;
    for (i = 0; i < INITIALIZATION_VECTOR_LENGTH; i++) {
      joinedContext[i] = ivArray[i];
    }

    for (i = 0; i < cipherTextLength; i++) {
      joinedContext[i + INITIALIZATION_VECTOR_LENGTH] = cipherText[i];
    }

    return joinedContext;
  }

  







  function _splitIVandCipherText(encryptedData) {
    
    var encryptedDataArray = sharedUtils.atob(encryptedData);

    
    var iv = encryptedDataArray.slice(0, INITIALIZATION_VECTOR_LENGTH);
    var cipherText = encryptedDataArray.slice(INITIALIZATION_VECTOR_LENGTH,
                                              encryptedDataArray.length);

    return {
      iv: iv,
      cipherText: cipherText
    };
  }

  this[inChrome ? "LoopCrypto" : "crypto"] = {
    decryptBytes: decryptBytes,
    encryptBytes: encryptBytes,
    generateKey: generateKey,
    isSupported: isSupported,
    setRootObject: setRootObject
  };
}).call(inChrome ? this : loop, this);
