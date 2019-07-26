"use strict";
var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;


















const PR_RDWR        = 0x04; 
const PR_CREATE_FILE = 0x08;
const PR_TRUNCATE    = 0x20;

let tempScope = {};
Cu.import("resource://gre/modules/NetUtil.jsm", tempScope);
let NetUtil = tempScope.NetUtil;

Cu.import("resource://gre/modules/FileUtils.jsm"); 
Cu.import("resource://gre/modules/Services.jsm");  

do_get_profile(); 
const certdb = Cc["@mozilla.org/security/x509certdb;1"].getService(Ci.nsIX509CertDB);




function tamper(inFilePath, outFilePath, modifications, newEntries) {
  var writer = Cc["@mozilla.org/zipwriter;1"].createInstance(Ci.nsIZipWriter);
  writer.open(outFilePath, PR_RDWR | PR_CREATE_FILE | PR_TRUNCATE);
  try {
    var reader = Cc["@mozilla.org/libjar/zip-reader;1"].createInstance(Ci.nsIZipReader);
    reader.open(inFilePath);
    try {
      var entries = reader.findEntries("");
      while (entries.hasMore()) {
        var entryName = entries.getNext();
        var inEntry = reader.getEntry(entryName);
        var entryInput = reader.getInputStream(entryName);
        try {
          var f = modifications[entryName];
          var outEntry, outEntryInput;
          if (f) {
            [outEntry, outEntryInput] = f(inEntry, entryInput);
            delete modifications[entryName];
          } else {
            [outEntry, outEntryInput] = [inEntry, entryInput];
          }
          
          
          if (outEntryInput) {
            try {
              writer.addEntryStream(entryName,
                                    outEntry.lastModifiedTime,
                                    outEntry.compression,
                                    outEntryInput,
                                    false);
            } finally {
              if (entryInput != outEntryInput)
                outEntryInput.close();
            }
          }
        } finally {
          entryInput.close();
        }
      }
    } finally {
      reader.close();
    }
    
    
    
    for(var name in modifications) {
      if (modifications.hasOwnProperty(name)) {
        throw "input file was missing expected entries: " + name;
      }
    }
    
    
    newEntries.forEach(function(newEntry) {
      var sis = Cc["@mozilla.org/io/string-input-stream;1"]
                  .createInstance(Ci.nsIStringInputStream);
      try {
        sis.setData(newEntry.content, newEntry.content.length);
        writer.addEntryStream(newEntry.name,
                              new Date(),
                              Ci.nsIZipWriter.COMPRESSION_BEST,
                              sis,
                              false);
      } finally {
        sis.close();
      }
    });
  } finally {
    writer.close();
  }
}

function removeEntry(entry, entryInput) { return [null, null]; }

function truncateEntry(entry, entryInput) {
  if (entryInput.available() == 0)
    throw "Truncating already-zero length entry will result in identical entry.";

  var content = Cc["@mozilla.org/io/string-input-stream;1"]
                  .createInstance(Ci.nsIStringInputStream);
  content.data = "";

  return [entry, content]
}

function readFile(file) {
  let fstream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  fstream.init(file, -1, 0, 0);
  let data = NetUtil.readInputStreamToString(fstream, fstream.available());
  fstream.close();
  return data;
}

function run_test() {
  var root_cert_der = 
    do_get_file("test_signed_apps/trusted_ca1.der", false);
  var der = readFile(root_cert_der);
  certdb.addCert(der, ",,CTu", "test-root");
  run_next_test();
}

function check_open_result(name, expectedRv) {
  return function openSignedJARFileCallback(rv, aZipReader, aSignerCert) {
    do_print("openSignedJARFileCallback called for " + name);
    do_check_eq(rv, expectedRv);
    do_check_eq(aZipReader != null,  Components.isSuccessCode(expectedRv));
    do_check_eq(aSignerCert != null, Components.isSuccessCode(expectedRv));
    run_next_test();
  };
}

function original_app_path(test_name) {
  return do_get_file("test_signed_apps/" + test_name + ".zip", false);
}

function tampered_app_path(test_name) {
  return FileUtils.getFile("TmpD", ["test_signed_app-" + test_name + ".zip"]);
}

add_test(function () {
  certdb.openSignedJARFileAsync(original_app_path("valid"),
                                check_open_result("valid", Cr.NS_OK));
});

add_test(function () {
  certdb.openSignedJARFileAsync(original_app_path("unsigned"),
             check_open_result("unsigned", Cr.NS_ERROR_SIGNED_JAR_NOT_SIGNED));
});

add_test(function () {
  
  
  
  
  
  
  
  
  
  
  const NS_ERROR_SEC_ERROR_UNKNOWN_ISSUER = 0x80000000 
                                          + (    (0x45 + 21) << 16)
                                          + (-(-0x2000 + 13)      );
  certdb.openSignedJARFileAsync(original_app_path("unknown_issuer"),
    check_open_result("unknown_issuer",
                      NS_ERROR_SEC_ERROR_UNKNOWN_ISSUER));
});


add_test(function () {
  var tampered = tampered_app_path("identity_tampering");
  tamper(original_app_path("valid"), tampered, { }, []);
  certdb.openSignedJARFileAsync(original_app_path("valid"),
    check_open_result("identity_tampering", Cr.NS_OK));
});

add_test(function () {
  var tampered = tampered_app_path("missing_rsa");
  tamper(original_app_path("valid"), tampered, { "META-INF/A.RSA" : removeEntry }, []);
  certdb.openSignedJARFileAsync(tampered,
    check_open_result("missing_rsa", Cr.NS_ERROR_SIGNED_JAR_NOT_SIGNED));
});

add_test(function () {
  var tampered = tampered_app_path("missing_sf");
  tamper(original_app_path("valid"), tampered, { "META-INF/A.SF" : removeEntry }, []);
  certdb.openSignedJARFileAsync(tampered,
    check_open_result("missing_sf", Cr.NS_ERROR_SIGNED_JAR_MANIFEST_INVALID));
});

add_test(function () {
  var tampered = tampered_app_path("missing_manifest_mf");
  tamper(original_app_path("valid"), tampered, { "META-INF/MANIFEST.MF" : removeEntry }, []);
  certdb.openSignedJARFileAsync(tampered,
    check_open_result("missing_manifest_mf",
                      Cr.NS_ERROR_SIGNED_JAR_MANIFEST_INVALID));
});

add_test(function () {
  var tampered = tampered_app_path("missing_entry");
  tamper(original_app_path("valid"), tampered, { "manifest.webapp" : removeEntry }, []);
  certdb.openSignedJARFileAsync(tampered,
      check_open_result("missing_entry", Cr.NS_ERROR_SIGNED_JAR_ENTRY_MISSING));
});

add_test(function () {
  var tampered = tampered_app_path("truncated_entry");
  tamper(original_app_path("valid"), tampered, { "manifest.webapp" : truncateEntry }, []);
  certdb.openSignedJARFileAsync(tampered,
    check_open_result("truncated_entry", Cr.NS_ERROR_SIGNED_JAR_MODIFIED_ENTRY));
});

add_test(function () {
  var tampered = tampered_app_path("unsigned_entry");
  tamper(original_app_path("valid"), tampered, {},
    [ { "name": "unsigned.txt", "content": "unsigned content!" } ]);
  certdb.openSignedJARFileAsync(tampered,
    check_open_result("unsigned_entry", Cr.NS_ERROR_SIGNED_JAR_UNSIGNED_ENTRY));
});

add_test(function () {
  var tampered = tampered_app_path("unsigned_metainf_entry");
  tamper(original_app_path("valid"), tampered, {},
    [ { name: "META-INF/unsigned.txt", content: "unsigned content!" } ]);
  certdb.openSignedJARFileAsync(tampered,
    check_open_result("unsigned_metainf_entry",
                      Cr.NS_ERROR_SIGNED_JAR_UNSIGNED_ENTRY));
});






