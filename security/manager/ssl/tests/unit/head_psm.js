



"use strict";

const { 'classes': Cc, 'interfaces': Ci, 'utils': Cu, 'results': Cr } = Components;

let { NetUtil } = Cu.import("resource://gre/modules/NetUtil.jsm", {});
let { FileUtils } = Cu.import("resource://gre/modules/FileUtils.jsm", {});
let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});

let gIsWindows = ("@mozilla.org/windows-registry-key;1" in Cc);

function readFile(file) {
  let fstream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  fstream.init(file, -1, 0, 0);
  let data = NetUtil.readInputStreamToString(fstream, fstream.available());
  fstream.close();
  return data;
}

function addCertFromFile(certdb, filename, trustString) {
  let certFile = do_get_file(filename, false);
  let der = readFile(certFile);
  certdb.addCert(der, trustString, null);
}

function getXPCOMStatusFromNSS(offset) {
  let nssErrorsService = Cc["@mozilla.org/nss_errors_service;1"]
                           .getService(Ci.nsINSSErrorsService);
  let statusNSS = Ci.nsINSSErrorsService.NSS_SEC_ERROR_BASE + offset;
  return nssErrorsService.getXPCOMFromNSSError(statusNSS);
}
