



const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

const EXPORTED_SYMBOLS = ["CryptoUtils"];

Cu.import("resource://services-common/observers.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let CryptoUtils = {
  


  generateRandomBytes: function generateRandomBytes(length) {
    let rng = Cc["@mozilla.org/security/random-generator;1"]
                .createInstance(Ci.nsIRandomGenerator);
    let bytes = rng.generateRandomBytes(length);
    return CommonUtils.byteArrayToString(bytes);
  },

  



  digestUTF8: function digestUTF8(message, hasher) {
    let data = this._utf8Converter.convertToByteArray(message, {});
    hasher.update(data, data.length);
    let result = hasher.finish(false);
    if (hasher instanceof Ci.nsICryptoHMAC) {
      hasher.reset();
    }
    return result;
  },

  




  digestBytes: function digestBytes(message, hasher) {
    
    let bytes = [b.charCodeAt() for each (b in message)];
    hasher.update(bytes, bytes.length);
    let result = hasher.finish(false);
    if (hasher instanceof Ci.nsICryptoHMAC) {
      hasher.reset();
    }
    return result;
  },

  













  UTF8AndSHA1: function UTF8AndSHA1(message) {
    let hasher = Cc["@mozilla.org/security/hash;1"]
                 .createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA1);

    return CryptoUtils.digestUTF8(message, hasher);
  },

  sha1: function sha1(message) {
    return CommonUtils.bytesAsHex(CryptoUtils.UTF8AndSHA1(message));
  },

  sha1Base32: function sha1Base32(message) {
    return CommonUtils.encodeBase32(CryptoUtils.UTF8AndSHA1(message));
  },

  


  makeHMACKey: function makeHMACKey(str) {
    return Svc.KeyFactory.keyFromString(Ci.nsIKeyObject.HMAC, str);
  },

  


  makeHMACHasher: function makeHMACHasher(type, key) {
    let hasher = Cc["@mozilla.org/security/hmac;1"]
                   .createInstance(Ci.nsICryptoHMAC);
    hasher.init(type, key);
    return hasher;
  },

  


  hkdfExpand: function hkdfExpand(prk, info, len) {
    const BLOCKSIZE = 256 / 8;
    let h = CryptoUtils.makeHMACHasher(Ci.nsICryptoHMAC.SHA256,
                                       CryptoUtils.makeHMACKey(prk));
    let T = "";
    let Tn = "";
    let iterations = Math.ceil(len/BLOCKSIZE);
    for (let i = 0; i < iterations; i++) {
      Tn = CryptoUtils.digestBytes(Tn + info + String.fromCharCode(i + 1), h);
      T += Tn;
    }
    return T.slice(0, len);
  },

  














  pbkdf2Generate : function pbkdf2Generate(P, S, c, dkLen) {
    
    
    if (!dkLen) {
      dkLen = SYNC_KEY_DECODED_LENGTH;
    }

    
    const HLEN = 20;

    function F(S, c, i, h) {

      function XOR(a, b, isA) {
        if (a.length != b.length) {
          return false;
        }

        let val = [];
        for (let i = 0; i < a.length; i++) {
          if (isA) {
            val[i] = a[i] ^ b[i];
          } else {
            val[i] = a.charCodeAt(i) ^ b.charCodeAt(i);
          }
        }

        return val;
      }

      let ret;
      let U = [];

      
      let I = [];
      I[0] = String.fromCharCode((i >> 24) & 0xff);
      I[1] = String.fromCharCode((i >> 16) & 0xff);
      I[2] = String.fromCharCode((i >> 8) & 0xff);
      I[3] = String.fromCharCode(i & 0xff);

      U[0] = CryptoUtils.digestBytes(S + I.join(''), h);
      for (let j = 1; j < c; j++) {
        U[j] = CryptoUtils.digestBytes(U[j - 1], h);
      }

      ret = U[0];
      for (j = 1; j < c; j++) {
        ret = CommonUtils.byteArrayToString(XOR(ret, U[j]));
      }

      return ret;
    }

    let l = Math.ceil(dkLen / HLEN);
    let r = dkLen - ((l - 1) * HLEN);

    
    let h = CryptoUtils.makeHMACHasher(Ci.nsICryptoHMAC.SHA1,
                                       CryptoUtils.makeHMACKey(P));

    T = [];
    for (let i = 0; i < l;) {
      T[i] = F(S, c, ++i, h);
    }

    let ret = "";
    for (i = 0; i < l-1;) {
      ret += T[i++];
    }
    ret += T[l - 1].substr(0, r);

    return ret;
  },

  deriveKeyFromPassphrase: function deriveKeyFromPassphrase(passphrase,
                                                            salt,
                                                            keyLength,
                                                            forceJS) {
    if (Svc.Crypto.deriveKeyFromPassphrase && !forceJS) {
      return Svc.Crypto.deriveKeyFromPassphrase(passphrase, salt, keyLength);
    }
    else {
      
      
      return CryptoUtils.pbkdf2Generate(passphrase, atob(salt), 4096,
                                        keyLength);
    }
  },

  




































  computeHTTPMACSHA1: function computeHTTPMACSHA1(identifier, key, method,
                                                  uri, extra) {
    let ts = (extra && extra.ts) ? extra.ts : Math.floor(Date.now() / 1000);
    let nonce_bytes = (extra && extra.nonce_bytes > 0) ? extra.nonce_bytes : 8;

    
    let nonce = (extra && extra.nonce)
                ? extra.nonce
                : btoa(CryptoUtils.generateRandomBytes(nonce_bytes));

    let host = uri.asciiHost;
    let port;
    let usedMethod = method.toUpperCase();

    if (uri.port != -1) {
      port = uri.port;
    } else if (uri.scheme == "http") {
      port = "80";
    } else if (uri.scheme == "https") {
      port = "443";
    } else {
      throw new Error("Unsupported URI scheme: " + uri.scheme);
    }

    let ext = (extra && extra.ext) ? extra.ext : "";

    let requestString = ts.toString(10) + "\n" +
                        nonce           + "\n" +
                        usedMethod      + "\n" +
                        uri.path        + "\n" +
                        host            + "\n" +
                        port            + "\n" +
                        ext             + "\n";

    let hasher = CryptoUtils.makeHMACHasher(Ci.nsICryptoHMAC.SHA1,
                                            CryptoUtils.makeHMACKey(key));
    let mac = CryptoUtils.digestBytes(requestString, hasher);

    function getHeader() {
      return CryptoUtils.getHTTPMACSHA1Header(this.identifier, this.ts,
                                              this.nonce, this.mac, this.ext);
    }

    return {
      identifier: identifier,
      key:        key,
      method:     usedMethod,
      hostname:   host,
      port:       port,
      mac:        mac,
      nonce:      nonce,
      ts:         ts,
      ext:        ext,
      getHeader:  getHeader
    };
  },


  















  getHTTPMACSHA1Header: function getHTTPMACSHA1Header(identifier, ts, nonce,
                                                      mac, ext) {
    let header ='MAC id="' + identifier + '", ' +
                'ts="'     + ts         + '", ' +
                'nonce="'  + nonce      + '", ' +
                'mac="'    + btoa(mac)  + '"';

    if (!ext) {
      return header;
    }

    return header += ', ext="' + ext +'"';
  },
};

XPCOMUtils.defineLazyGetter(CryptoUtils, "_utf8Converter", function() {
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";

  return converter;
});

let Svc = {};

XPCOMUtils.defineLazyServiceGetter(Svc,
                                   "KeyFactory",
                                   "@mozilla.org/security/keyobjectfactory;1",
                                   "nsIKeyObjectFactory");

Svc.__defineGetter__("Crypto", function() {
  let ns = {};
  Cu.import("resource://services-crypto/WeaveCrypto.js", ns);

  let wc = new ns.WeaveCrypto();
  delete Svc.Crypto;
  return Svc.Crypto = wc;
});

Observers.add("xpcom-shutdown", function unloadServices() {
  Observers.remove("xpcom-shutdown", unloadServices);

  for (let k in Svc) {
    delete Svc[k];
  }
});
