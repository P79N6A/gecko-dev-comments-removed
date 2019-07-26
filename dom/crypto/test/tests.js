



function exists(x) {
  return (x !== undefined);
}

function hasKeyFields(x) {
  return exists(x.algorithm) &&
         exists(x.extractable) &&
         exists(x.type) &&
         exists(x.usages);
}

function error(test) {
  return function(x) {
    console.log("ERROR :: " + x);
    test.complete(false);
    throw x;
  }
}

function complete(test, valid) {
  return function(x) {
    console.log("COMPLETE")
    console.log(x);
    if (valid) {
      test.complete(valid(x));
    } else {
      test.complete(true);
    }
  }
}

function memcmp_complete(test, value) {
  return function(x) {
    console.log("COMPLETE")
    console.log(x);
    test.memcmp_complete(value, x);
  }
}



TestArray.addTest(
  "Test for presence of WebCrypto API methods",
  function() {
    var that = this;
    this.complete(
      exists(window.crypto.subtle) &&
      exists(window.crypto.subtle.encrypt) &&
      exists(window.crypto.subtle.decrypt) &&
      exists(window.crypto.subtle.sign) &&
      exists(window.crypto.subtle.verify) &&
      exists(window.crypto.subtle.digest) &&
      exists(window.crypto.subtle.importKey) &&
      exists(window.crypto.subtle.exportKey) &&
      exists(window.crypto.subtle.generateKey) &&
      exists(window.crypto.subtle.deriveKey) &&
      exists(window.crypto.subtle.deriveBits)
    );
  }
);



TestArray.addTest(
  "Clean failure on a mal-formed algorithm",
  function() {
    var that = this;
    var alg = {
      get name() {
        throw "Oh no, no name!";
      }
    };

    crypto.subtle.importKey("raw", tv.raw, alg, true, ["encrypt"])
      .then(
        error(that),
        complete(that, function(x) { return true; })
      );
  }
)


TestArray.addTest(
  "Import / export round-trip with 'raw'",
  function() {
    var that = this;
    var alg = "AES-GCM";

    function doExport(x) {
      if (!hasKeyFields(x)) {
        throw "Invalid key; missing field(s)";
      } else if ((x.algorithm.name != alg) ||
        (x.algorithm.length != 8 * tv.raw.length) ||
        (x.type != "secret") ||
        (!x.extractable) ||
        (x.usages.length != 1) ||
        (x.usages[0] != 'encrypt')){
        throw "Invalid key: incorrect key data";
      }
      return crypto.subtle.exportKey("raw", x);
    }

    crypto.subtle.importKey("raw", tv.raw, alg, true, ["encrypt"])
      .then(doExport, error(that))
      .then(
        memcmp_complete(that, tv.raw),
        error(that)
      );
  }
);


TestArray.addTest(
  "Import failure with format 'raw'",
  function() {
    var that = this;
    var alg = "AES-GCM";

    crypto.subtle.importKey("raw", tv.negative_raw, alg, true, ["encrypt"])
      .then(error(that), complete(that));
  }
);


TestArray.addTest(
  "Proper handling of an ABV representing part of a buffer",
  function() {
    var that = this;
    var alg = "AES-GCM";

    var u8 = new Uint8Array([0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                             0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                             0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                             0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f]);
    var u32 = new Uint32Array(u8.buffer, 8, 4);
    var out = u8.subarray(8, 24)

    function doExport(x) {
      return crypto.subtle.exportKey("raw", x);
    }

    crypto.subtle.importKey("raw", u32, alg, true, ["encrypt"])
      .then(doExport, error(that))
      .then(memcmp_complete(that, out), error(that));
  }
);


TestArray.addTest(
  "Import / export round-trip with 'pkcs8'",
  function() {
    var that = this;
    var alg = { name: "RSASSA-PKCS1-v1_5", hash: "SHA1" };

    function doExport(x) {
      if (!hasKeyFields(x)) {
        throw "Invalid key; missing field(s)";
      } else if ((x.algorithm.name != alg.name) ||
        (x.algorithm.hash.name != alg.hash) ||
        (x.algorithm.modulusLength != 512) ||
        (x.algorithm.publicExponent.byteLength != 3) ||
        (x.type != "private") ||
        (!x.extractable) ||
        (x.usages.length != 1) ||
        (x.usages[0] != 'sign')){
        throw "Invalid key: incorrect key data";
      }
      return crypto.subtle.exportKey("pkcs8", x);
    }

    crypto.subtle.importKey("pkcs8", tv.pkcs8, alg, true, ["sign"])
      .then(doExport, error(that))
      .then(
        memcmp_complete(that, tv.pkcs8),
        error(that)
      );
  }
);


TestArray.addTest(
  "Import failure with format 'pkcs8'",
  function() {
    var that = this;
    var alg = { name: "RSASSA-PKCS1-v1_5", hash: "SHA1" };

    crypto.subtle.importKey("pkcs8", tv.negative_pkcs8, alg, true, ["encrypt"])
      .then(error(that), complete(that));
  }
);


TestArray.addTest(
  "Import / export round-trip with 'spki'",
  function() {
    var that = this;
    var alg = "RSAES-PKCS1-v1_5";

    function doExport(x) {
      if (!hasKeyFields(x)) {
        throw "Invalid key; missing field(s)";
      } else if ((x.algorithm.name != alg) ||
        (x.algorithm.modulusLength != 1024) ||
        (x.algorithm.publicExponent.byteLength != 3) ||
        (x.type != "public") ||
        (!x.extractable) ||
        (x.usages.length != 1) ||
        (x.usages[0] != 'encrypt')){
        throw "Invalid key: incorrect key data";
      }
      return crypto.subtle.exportKey("spki", x);
    }

    crypto.subtle.importKey("spki", tv.spki, alg, true, ["encrypt"])
      .then(doExport, error(that))
      .then(
        memcmp_complete(that, tv.spki),
        error(that)
      );
  }
);


TestArray.addTest(
  "Import failure with format 'spki'",
  function() {
    var that = this;
    var alg = "RSAES-PKCS1-v1_5";

    crypto.subtle.importKey("spki", tv.negative_spki, alg, true, ["encrypt"])
      .then(error(that), complete(that));
  }
);


TestArray.addTest(
  "Refuse to export non-extractable key",
  function() {
    var that = this;
    var alg = "AES-GCM";

    function doExport(x) {
      return crypto.subtle.exportKey("raw", x);
    }

    crypto.subtle.importKey("raw", tv.raw, alg, false, ["encrypt"])
      .then(doExport, error(that))
      .then(
        error(that),
        complete(that)
      );
  }
);


TestArray.addTest(
  "IndexedDB store / retrieve round-trip",
  function() {
    var that = this;
    var alg = "AES-GCM";
    var importedKey;
    var dbname = "keyDB";
    var dbstore = "keystore";
    var dbversion = 1;
    var dbkey = 0;

    function doIndexedDB(x) {
      importedKey = x;
      var req = indexedDB.deleteDatabase(dbname);
      req.onerror = error(that);
      req.onsuccess = doCreateDB;
    }

    function doCreateDB() {
      var req = indexedDB.open(dbname, dbversion);
      req.onerror = error(that);
      req.onupgradeneeded = function(e) {
        db = e.target.result;
        db.createObjectStore(dbstore, {keyPath: "id"});
      }

      req.onsuccess = doPut;
    }

    function doPut() {
      req = db.transaction([dbstore], "readwrite")
              .objectStore(dbstore)
              .add({id: dbkey, val: importedKey});
      req.onerror = error(that);
      req.onsuccess = doGet;
    }

    function doGet() {
      req = db.transaction([dbstore], "readwrite")
              .objectStore(dbstore)
              .get(dbkey);
      req.onerror = error(that);
      req.onsuccess = complete(that, function(e) {
        return hasKeyFields(e.target.result.val);
      });
    }

    crypto.subtle.importKey("raw", tv.raw, alg, false, ['encrypt'])
      .then(doIndexedDB, error(that));
  }
);
