



const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = ["CryptoUtils"];

Cu.import("resource://services-common/observers.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.CryptoUtils = {
  xor: function xor(a, b) {
    let bytes = [];

    if (a.length != b.length) {
      throw new Error("can't xor unequal length strings: "+a.length+" vs "+b.length);
    }

    for (let i = 0; i < a.length; i++) {
      bytes[i] = a.charCodeAt(i) ^ b.charCodeAt(i);
    }

    return String.fromCharCode.apply(String, bytes);
  },

  


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

  





  updateUTF8: function(message, hasher) {
    let bytes = this._utf8Converter.convertToByteArray(message, {});
    hasher.update(bytes, bytes.length);
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

  


  hkdf: function hkdf(ikm, xts, info, len) {
    const BLOCKSIZE = 256 / 8;
    if (typeof xts === undefined)
      xts = String.fromCharCode(0, 0, 0, 0,  0, 0, 0, 0,
                                0, 0, 0, 0,  0, 0, 0, 0,
                                0, 0, 0, 0,  0, 0, 0, 0,
                                0, 0, 0, 0,  0, 0, 0, 0);
    let h = CryptoUtils.makeHMACHasher(Ci.nsICryptoHMAC.SHA256,
                                       CryptoUtils.makeHMACKey(xts));
    let prk = CryptoUtils.digestBytes(ikm, h);
    return CryptoUtils.hkdfExpand(prk, info, len);
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

  



















  pbkdf2Generate : function pbkdf2Generate(P, S, c, dkLen,
                       hmacAlg=Ci.nsICryptoHMAC.SHA1, hmacLen=20) {

    
    
    if (!dkLen) {
      dkLen = SYNC_KEY_DECODED_LENGTH;
    }

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
      for (let j = 1; j < c; j++) {
        ret = CommonUtils.byteArrayToString(XOR(ret, U[j]));
      }

      return ret;
    }

    let l = Math.ceil(dkLen / hmacLen);
    let r = dkLen - ((l - 1) * hmacLen);

    
    let h = CryptoUtils.makeHMACHasher(hmacAlg,
                                       CryptoUtils.makeHMACKey(P));

    let T = [];
    for (let i = 0; i < l;) {
      T[i] = F(S, c, ++i, h);
    }

    let ret = "";
    for (let i = 0; i < l-1;) {
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

  



  stripHeaderAttributes: function(value) {
    value = value || "";
    let i = value.indexOf(";");
    return value.substring(0, (i >= 0) ? i : undefined).trim().toLowerCase();
  },

  




















































  computeHAWK: function(uri, method, options) {
    let credentials = options.credentials;
    let ts = options.ts || Math.floor(((options.now || Date.now()) +
                                       (options.localtimeOffsetMsec || 0))
                                      / 1000);

    let hash_algo, hmac_algo;
    if (credentials.algorithm == "sha1") {
      hash_algo = Ci.nsICryptoHash.SHA1;
      hmac_algo = Ci.nsICryptoHMAC.SHA1;
    } else if (credentials.algorithm == "sha256") {
      hash_algo = Ci.nsICryptoHash.SHA256;
      hmac_algo = Ci.nsICryptoHMAC.SHA256;
    } else {
      throw new Error("Unsupported algorithm: " + credentials.algorithm);
    }

    let port;
    if (uri.port != -1) {
      port = uri.port;
    } else if (uri.scheme == "http") {
      port = 80;
    } else if (uri.scheme == "https") {
      port = 443;
    } else {
      throw new Error("Unsupported URI scheme: " + uri.scheme);
    }

    let artifacts = {
      ts: ts,
      nonce: options.nonce || btoa(CryptoUtils.generateRandomBytes(8)),
      method: method.toUpperCase(),
      resource: uri.path, 
      host: uri.asciiHost.toLowerCase(), 
      port: port.toString(10),
      hash: options.hash,
      ext: options.ext,
    };

    let contentType = CryptoUtils.stripHeaderAttributes(options.contentType);

    if (!artifacts.hash && options.hasOwnProperty("payload")
        && options.payload) {
      let hasher = Cc["@mozilla.org/security/hash;1"]
                     .createInstance(Ci.nsICryptoHash);
      hasher.init(hash_algo);
      CryptoUtils.updateUTF8("hawk.1.payload\n", hasher);
      CryptoUtils.updateUTF8(contentType+"\n", hasher);
      CryptoUtils.updateUTF8(options.payload, hasher);
      CryptoUtils.updateUTF8("\n", hasher);
      let hash = hasher.finish(false);
      
      
      let hash_b64 = btoa(hash);
      artifacts.hash = hash_b64;
    }

    let requestString = ("hawk.1.header"        + "\n" +
                         artifacts.ts.toString(10) + "\n" +
                         artifacts.nonce        + "\n" +
                         artifacts.method       + "\n" +
                         artifacts.resource     + "\n" +
                         artifacts.host         + "\n" +
                         artifacts.port         + "\n" +
                         (artifacts.hash || "") + "\n");
    if (artifacts.ext) {
      requestString += artifacts.ext.replace("\\", "\\\\").replace("\n", "\\n");
    }
    requestString += "\n";

    let hasher = CryptoUtils.makeHMACHasher(hmac_algo,
                                            CryptoUtils.makeHMACKey(credentials.key));
    artifacts.mac = btoa(CryptoUtils.digestBytes(requestString, hasher));
    

    function escape(attribute) {
      
      return attribute.replace(/\\/g, "\\\\").replace(/\"/g, '\\"');
    }
    let header = ('Hawk id="' + credentials.id + '", ' +
                  'ts="' + artifacts.ts + '", ' +
                  'nonce="' + artifacts.nonce + '", ' +
                  (artifacts.hash ? ('hash="' + artifacts.hash + '", ') : "") +
                  (artifacts.ext ? ('ext="' + escape(artifacts.ext) + '", ') : "") +
                  'mac="' + artifacts.mac + '"');
    return {
      artifacts: artifacts,
      field: header,
    };
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
