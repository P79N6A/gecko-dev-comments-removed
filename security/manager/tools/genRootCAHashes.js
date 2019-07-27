











const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const nsX509CertDB = "@mozilla.org/security/x509certdb;1";
const CertDb = Components.classes[nsX509CertDB].getService(Ci.nsIX509CertDB);

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://services-common/utils.js");

const FILENAME_OUTPUT = "RootHashes.inc";
const FILENAME_TRUST_ANCHORS = "KnownRootHashes.json";
const ROOT_NOT_ASSIGNED = -1;

const JSON_HEADER = "// This Source Code Form is subject to the terms of the Mozilla Public\n" +
"// License, v. 2.0. If a copy of the MPL was not distributed with this\n" +
"// file, You can obtain one at http://mozilla.org/MPL/2.0/. */\n" +
"//\n" +
"//***************************************************************************\n" +
"// This is an automatically generated file. It's used to maintain state for\n" +
"// runs of genRootCAHashes.js; you should never need to manually edit it\n" +
"//***************************************************************************\n" +
"\n";

const FILE_HEADER = "/* This Source Code Form is subject to the terms of the Mozilla Public\n" +
" * License, v. 2.0. If a copy of the MPL was not distributed with this\n" +
" * file, You can obtain one at http://mozilla.org/MPL/2.0/. */\n" +
"\n" +
"/*****************************************************************************/\n" +
"/* This is an automatically generated file. If you're not                    */\n" +
"/* RootCertificateTelemetryUtils.cpp, you shouldn't be #including it.        */\n" +
"/*****************************************************************************/\n" +
"\n" +
"#define HASH_LEN 32\n";

const FP_PREAMBLE = "struct CertAuthorityHash {\n" +
" const uint8_t hash[HASH_LEN];\n" +
" const int32_t binNumber;\n" +
"};\n\n" +
"static const struct CertAuthorityHash ROOT_TABLE[] = {\n";

const FP_POSTAMBLE = "};\n";


function writeString(fos, string) {
  fos.write(string, string.length);
}


function stripColons(hexString) {
  return hexString.replace(/:/g, '');
}


function hexSlice(bytes, start, end) {
  let ret = "";
  for (let i = start; i < end; i++) {
    let hex = (0 + bytes.charCodeAt(i).toString(16)).slice(-2).toUpperCase();
    ret += "0x" + hex;
    if (i < end - 1) {
      ret += ", ";
    }
  }
  return ret;
}

function stripComments(buf) {
  let lines = buf.split("\n");
  let entryRegex = /^\s*\/\//;
  let data = "";
  for (let i = 0; i < lines.length; i++) {
    let match = entryRegex.exec(lines[i]);
    if (!match) {
      data = data + lines[i];
    }
  }
  return data;
}



function loadTrustAnchors(file) {
  if (file.exists()) {
    let stream = Cc["@mozilla.org/network/file-input-stream;1"]
                   .createInstance(Ci.nsIFileInputStream);
    stream.init(file, -1, 0, 0);
    let buf = NetUtil.readInputStreamToString(stream, stream.available());
    return JSON.parse(stripComments(buf));
  }
  
  return { roots: [], maxBin: 0 };
}



function writeTrustAnchors(file) {
  let fos = FileUtils.openSafeFileOutputStream(file);

  let serializedData = JSON.stringify(gTrustAnchors, null, '  ');
  fos.write(JSON_HEADER, JSON_HEADER.length);
  fos.write(serializedData, serializedData.length);

  FileUtils.closeSafeFileOutputStream(fos);
}



function writeRootHashes(fos) {
  try {
    writeString(fos, FILE_HEADER);

    
    writeString(fos, FP_PREAMBLE);
    gTrustAnchors.roots.forEach(function(fp) {
      let fpBytes = atob(fp.sha256Fingerprint);

      writeString(fos, "  {\n");
      writeString(fos, "    /* "+fp.label+" */\n");
      writeString(fos, "    { " + hexSlice(fpBytes, 0, 16) + ",\n");
      writeString(fos, "      " + hexSlice(fpBytes, 16, 32) + " },\n");
      writeString(fos, "      " + fp.binNumber + " /* Bin Number */\n");

      writeString(fos, "  },\n");
    });
    writeString(fos, FP_POSTAMBLE);

    writeString(fos, "\n");

  }
  catch (e) {
    dump("ERROR: problem writing output: " + e + "\n");
  }
}


function findTrustAnchorByFingerprint(sha256Fingerprint) {
  for (let i = 0; i < gTrustAnchors.roots.length; i++) {
    if (sha256Fingerprint == gTrustAnchors.roots[i].sha256Fingerprint) {
      return i;
    }
  }
  return ROOT_NOT_ASSIGNED;
}


function getLabelForCert(cert) {
  let label = cert.commonName;

  if (label.length < 5) {
    label = cert.subjectName;
  }

  
  label = label.replace( /[^[:ascii:]]/g, "_");
  
  label = label.replace(/[^A-Za-z0-9]/g ,"_");
  return label;
}


function insertTrustAnchorsFromDatabase(){
  
  const CERT_TYPE = Ci.nsIX509Cert.CA_CERT;
  const TRUST_TYPE = Ci.nsIX509CertDB.TRUSTED_SSL;

  
  let enumerator = CertDb.getCerts().getEnumerator();
  while (enumerator.hasMoreElements()) {
    let cert = enumerator.getNext().QueryInterface(Ci.nsIX509Cert);

    
    

    
    if (CertDb.isCertTrusted(cert, CERT_TYPE, TRUST_TYPE)) {
      
      let binaryFingerprint = CommonUtils.hexToBytes(stripColons(cert.sha256Fingerprint));
      let encodedFingerprint = btoa(binaryFingerprint);

       
      if (findTrustAnchorByFingerprint(encodedFingerprint) == ROOT_NOT_ASSIGNED) {

        
        let label = getLabelForCert(cert);

        
        gTrustAnchors.maxBin += 1;
        gTrustAnchors.roots.push(
          {
            "label": label,
            "binNumber": gTrustAnchors.maxBin,
            "sha256Fingerprint": encodedFingerprint
          });
      }
    }
  }
}





if (arguments.length < 1) {
  throw "Usage: genRootCAHashes.js <absolute path to current RootHashes.inc>";
}

let trustAnchorsFile = FileUtils.getFile("CurWorkD", [FILENAME_TRUST_ANCHORS]);

let rootHashesFile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
rootHashesFile.initWithPath(arguments[0]);


let gTrustAnchors = loadTrustAnchors(trustAnchorsFile);


insertTrustAnchorsFromDatabase();


writeTrustAnchors(trustAnchorsFile);



gTrustAnchors.roots.sort(function(a, b) {
  
  let aBin = atob(a.sha256Fingerprint);
  let bBin = atob(b.sha256Fingerprint)

  if (aBin < bBin)
     return -1;
  else if (aBin > bBin)
     return 1;
   else
     return 0;
});


let rootHashesFileOutputStream = FileUtils.openSafeFileOutputStream(rootHashesFile);
writeRootHashes(rootHashesFileOutputStream);
FileUtils.closeSafeFileOutputStream(rootHashesFileOutputStream);
