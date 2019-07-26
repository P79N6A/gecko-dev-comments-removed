





"use strict";


const Cu = Components.utils;
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/identity/LogUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this,
                                   "IdentityCryptoService",
                                   "@mozilla.org/identity/crypto-service;1",
                                   "nsIIdentityCryptoService");

this.EXPORTED_SYMBOLS = ["jwcrypto"];

const ALGORITHMS = { RS256: "RS256", DS160: "DS160" };
const DURATION_MS = 1000 * 60 * 2; 

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["jwcrypto"].concat(aMessageArgs));
}

function generateKeyPair(aAlgorithmName, aCallback) {
  log("Generate key pair; alg =", aAlgorithmName);

  IdentityCryptoService.generateKeyPair(aAlgorithmName, function(rv, aKeyPair) {
    if (!Components.isSuccessCode(rv)) {
      return aCallback("key generation failed");
    }

    var publicKey;

    switch (aKeyPair.keyType) {
     case ALGORITHMS.RS256:
      publicKey = {
        algorithm: "RS",
        exponent:  aKeyPair.hexRSAPublicKeyExponent,
        modulus:   aKeyPair.hexRSAPublicKeyModulus
      };
      break;

     case ALGORITHMS.DS160:
      publicKey = {
        algorithm: "DS",
        y: aKeyPair.hexDSAPublicValue,
        p: aKeyPair.hexDSAPrime,
        q: aKeyPair.hexDSASubPrime,
        g: aKeyPair.hexDSAGenerator
      };
      break;

    default:
      return aCallback("unknown key type");
    }

    let keyWrapper = {
      serializedPublicKey: JSON.stringify(publicKey),
      _kp: aKeyPair
    };

    return aCallback(null, keyWrapper);
  });
}

function sign(aPayload, aKeypair, aCallback) {
  aKeypair._kp.sign(aPayload, function(rv, signature) {
    if (!Components.isSuccessCode(rv)) {
      log("ERROR: signer.sign failed");
      return aCallback("Sign failed");
    }
    log("signer.sign: success");
    return aCallback(null, signature);
  });
}

function jwcryptoClass()
{
}

jwcryptoClass.prototype = {
  












  getExpiration: function(duration=DURATION_MS, localtimeOffsetMsec=0, now=Date.now()) {
    return now + localtimeOffsetMsec + duration;
  },

  isCertValid: function(aCert, aCallback) {
    
    aCallback(true);
  },

  generateKeyPair: function(aAlgorithmName, aCallback) {
    log("generating");
    generateKeyPair(aAlgorithmName, aCallback);
  },

  




























  generateAssertion: function(aCert, aKeyPair, aAudience, aOptions, aCallback) {
    if (typeof aOptions == "function") {
      aCallback = aOptions;
      aOptions = { };
    }

    
    
    var header = {"alg": "DS128"};
    var headerBytes = IdentityCryptoService.base64UrlEncode(
                          JSON.stringify(header));

    var payload = {
      exp: this.getExpiration(
               aOptions.duration, aOptions.localtimeOffsetMsec, aOptions.now),
      aud: aAudience
    };
    var payloadBytes = IdentityCryptoService.base64UrlEncode(
                          JSON.stringify(payload));

    log("payload bytes", payload, payloadBytes);
    sign(headerBytes + "." + payloadBytes, aKeyPair, function(err, signature) {
      if (err)
        return aCallback(err);

      var signedAssertion = headerBytes + "." + payloadBytes + "." + signature;
      return aCallback(null, aCert + "~" + signedAssertion);
    });
  }

};

this.jwcrypto = new jwcryptoClass();
this.jwcrypto.ALGORITHMS = ALGORITHMS;
