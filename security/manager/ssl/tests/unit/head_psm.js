



"use strict";

const { 'classes': Cc, 'interfaces': Ci, 'utils': Cu, 'results': Cr } = Components;

let { NetUtil } = Cu.import("resource://gre/modules/NetUtil.jsm", {});
let { FileUtils } = Cu.import("resource://gre/modules/FileUtils.jsm", {});
let { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
let { Promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
let { HttpServer } = Cu.import("resource://testing-common/httpd.js", {});
let { ctypes } = Cu.import("resource://gre/modules/ctypes.jsm");

let gIsWindows = ("@mozilla.org/windows-registry-key;1" in Cc);

const isDebugBuild = Cc["@mozilla.org/xpcom/debug;1"]
                       .getService(Ci.nsIDebug2).isDebugBuild;

const SSS_STATE_FILE_NAME = "SiteSecurityServiceState.txt";

const SEC_ERROR_BASE = Ci.nsINSSErrorsService.NSS_SEC_ERROR_BASE;
const SSL_ERROR_BASE = Ci.nsINSSErrorsService.NSS_SSL_ERROR_BASE;
const MOZILLA_PKIX_ERROR_BASE = Ci.nsINSSErrorsService.MOZILLA_PKIX_ERROR_BASE;


const SEC_ERROR_INVALID_ARGS                            = SEC_ERROR_BASE +   5; 
const SEC_ERROR_BAD_DER                                 = SEC_ERROR_BASE +   9;
const SEC_ERROR_EXPIRED_CERTIFICATE                     = SEC_ERROR_BASE +  11;
const SEC_ERROR_REVOKED_CERTIFICATE                     = SEC_ERROR_BASE +  12; 
const SEC_ERROR_UNKNOWN_ISSUER                          = SEC_ERROR_BASE +  13;
const SEC_ERROR_BAD_DATABASE                            = SEC_ERROR_BASE +  18;
const SEC_ERROR_UNTRUSTED_ISSUER                        = SEC_ERROR_BASE +  20; 
const SEC_ERROR_UNTRUSTED_CERT                          = SEC_ERROR_BASE +  21; 
const SEC_ERROR_EXPIRED_ISSUER_CERTIFICATE              = SEC_ERROR_BASE +  30; 
const SEC_ERROR_EXTENSION_VALUE_INVALID                 = SEC_ERROR_BASE +  34; 
const SEC_ERROR_EXTENSION_NOT_FOUND                     = SEC_ERROR_BASE +  35; 
const SEC_ERROR_CA_CERT_INVALID                         = SEC_ERROR_BASE +  36;
const SEC_ERROR_INVALID_KEY                             = SEC_ERROR_BASE +  40; 
const SEC_ERROR_UNKNOWN_CRITICAL_EXTENSION              = SEC_ERROR_BASE +  41;
const SEC_ERROR_INADEQUATE_KEY_USAGE                    = SEC_ERROR_BASE +  90; 
const SEC_ERROR_INADEQUATE_CERT_TYPE                    = SEC_ERROR_BASE +  91; 
const SEC_ERROR_CERT_NOT_IN_NAME_SPACE                  = SEC_ERROR_BASE + 112; 
const SEC_ERROR_CERT_BAD_ACCESS_LOCATION                = SEC_ERROR_BASE + 117; 
const SEC_ERROR_OCSP_MALFORMED_REQUEST                  = SEC_ERROR_BASE + 120;
const SEC_ERROR_OCSP_SERVER_ERROR                       = SEC_ERROR_BASE + 121; 
const SEC_ERROR_OCSP_TRY_SERVER_LATER                   = SEC_ERROR_BASE + 122;
const SEC_ERROR_OCSP_REQUEST_NEEDS_SIG                  = SEC_ERROR_BASE + 123;
const SEC_ERROR_OCSP_UNAUTHORIZED_REQUEST               = SEC_ERROR_BASE + 124;
const SEC_ERROR_OCSP_UNKNOWN_CERT                       = SEC_ERROR_BASE + 126; 
const SEC_ERROR_OCSP_MALFORMED_RESPONSE                 = SEC_ERROR_BASE + 129;
const SEC_ERROR_OCSP_UNAUTHORIZED_RESPONSE              = SEC_ERROR_BASE + 130;
const SEC_ERROR_OCSP_OLD_RESPONSE                       = SEC_ERROR_BASE + 132;
const SEC_ERROR_OCSP_INVALID_SIGNING_CERT               = SEC_ERROR_BASE + 144;
const SEC_ERROR_POLICY_VALIDATION_FAILED                = SEC_ERROR_BASE + 160; 
const SEC_ERROR_OCSP_BAD_SIGNATURE                      = SEC_ERROR_BASE + 157;
const SEC_ERROR_CERT_SIGNATURE_ALGORITHM_DISABLED       = SEC_ERROR_BASE + 176;
const SEC_ERROR_APPLICATION_CALLBACK_ERROR              = SEC_ERROR_BASE + 178;

const SSL_ERROR_BAD_CERT_DOMAIN                         = SSL_ERROR_BASE +  12;
const SSL_ERROR_BAD_CERT_ALERT                          = SSL_ERROR_BASE +  17;

const MOZILLA_PKIX_ERROR_KEY_PINNING_FAILURE            = MOZILLA_PKIX_ERROR_BASE +   0;
const MOZILLA_PKIX_ERROR_CA_CERT_USED_AS_END_ENTITY     = MOZILLA_PKIX_ERROR_BASE +   1;
const MOZILLA_PKIX_ERROR_INADEQUATE_KEY_SIZE            = MOZILLA_PKIX_ERROR_BASE +   2; 


const certificateUsageSSLClient              = 0x0001;
const certificateUsageSSLServer              = 0x0002;
const certificateUsageSSLCA                  = 0x0008;
const certificateUsageEmailSigner            = 0x0010;
const certificateUsageEmailRecipient         = 0x0020;
const certificateUsageObjectSigner           = 0x0040;
const certificateUsageVerifyCA               = 0x0100;
const certificateUsageStatusResponder        = 0x0400;

const NO_FLAGS = 0;

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

function constructCertFromFile(filename) {
  let certFile = do_get_file(filename, false);
  let certDER = readFile(certFile);
  let certdb = Cc["@mozilla.org/security/x509certdb;1"]
                  .getService(Ci.nsIX509CertDB);
  return certdb.constructX509(certDER, certDER.length);
}

function setCertTrust(cert, trustString) {
  let certdb = Cc["@mozilla.org/security/x509certdb;1"]
                  .getService(Ci.nsIX509CertDB);
  certdb.setCertTrustFromString(cert, trustString);
}

function getXPCOMStatusFromNSS(statusNSS) {
  let nssErrorsService = Cc["@mozilla.org/nss_errors_service;1"]
                           .getService(Ci.nsINSSErrorsService);
  return nssErrorsService.getXPCOMFromNSSError(statusNSS);
}

function checkCertErrorGeneric(certdb, cert, expectedError, usage) {
  let hasEVPolicy = {};
  let verifiedChain = {};
  let error = certdb.verifyCertNow(cert, usage, NO_FLAGS, verifiedChain,
                                   hasEVPolicy);
  
  if (expectedError != -1 ) {
    do_check_eq(error, expectedError);
  } else {
    do_check_neq (error, 0);
  }
}

function _getLibraryFunctionWithNoArguments(functionName, libraryName) {
  
  let path = ctypes.libraryName(libraryName);

  
  let nsslib;
  try {
    nsslib = ctypes.open(path);
  } catch(e) {
    
    
    let file = Services.dirsvc.get("GreD", Ci.nsILocalFile);
    file.append(path);
    nsslib = ctypes.open(file.path);
  }

  let SECStatus = ctypes.int;
  let func = nsslib.declare(functionName, ctypes.default_abi, SECStatus);
  return func;
}

function clearOCSPCache() {
  let certdb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);
  certdb.clearOCSPCache();
}

function clearSessionCache() {
  let SSL_ClearSessionCache = null;
  try {
    SSL_ClearSessionCache =
      _getLibraryFunctionWithNoArguments("SSL_ClearSessionCache", "ssl3");
  } catch (e) {
    
    SSL_ClearSessionCache =
      _getLibraryFunctionWithNoArguments("SSL_ClearSessionCache", "nss3");
  }
  if (!SSL_ClearSessionCache || SSL_ClearSessionCache() != 0) {
    throw "Failed to clear SSL session cache";
  }
}


















































function add_tls_server_setup(serverBinName) {
  add_test(function() {
    _setupTLSServerTest(serverBinName);
  });
}










function add_connection_test(aHost, aExpectedResult,
                             aBeforeConnect, aWithSecurityInfo,
                             aAfterStreamOpen) {
  const REMOTE_PORT = 8443;

  function Connection(aHost) {
    this.host = aHost;
    let threadManager = Cc["@mozilla.org/thread-manager;1"]
                          .getService(Ci.nsIThreadManager);
    this.thread = threadManager.currentThread;
    this.defer = Promise.defer();
    let sts = Cc["@mozilla.org/network/socket-transport-service;1"]
                .getService(Ci.nsISocketTransportService);
    this.transport = sts.createTransport(["ssl"], 1, aHost, REMOTE_PORT, null);
    this.transport.setEventSink(this, this.thread);
    this.inputStream = null;
    this.outputStream = null;
    this.connected = false;
  }

  Connection.prototype = {
    
    onTransportStatus: function(aTransport, aStatus, aProgress, aProgressMax) {
      if (!this.connected && aStatus == Ci.nsISocketTransport.STATUS_CONNECTED_TO) {
        this.connected = true;
        this.outputStream.asyncWait(this, 0, 0, this.thread);
      }
    },

    
    onInputStreamReady: function(aStream) {
      try {
        
        let str = NetUtil.readInputStreamToString(aStream, aStream.available());
        do_check_eq(str, "0");
        this.inputStream.close();
        this.outputStream.close();
        this.result = Cr.NS_OK;
      } catch (e) {
        this.result = e.result;
      }
      this.defer.resolve(this);
    },

    
    onOutputStreamReady: function(aStream) {
      if (aAfterStreamOpen) {
        aAfterStreamOpen(this.transport);
      }
      let sslSocketControl = this.transport.securityInfo
                               .QueryInterface(Ci.nsISSLSocketControl);
      sslSocketControl.proxyStartSSL();
      this.outputStream.write("0", 1);
      let inStream = this.transport.openInputStream(0, 0, 0)
                       .QueryInterface(Ci.nsIAsyncInputStream);
      this.inputStream = inStream;
      this.inputStream.asyncWait(this, 0, 0, this.thread);
    },

    go: function() {
      this.outputStream = this.transport.openOutputStream(0, 0, 0)
                            .QueryInterface(Ci.nsIAsyncOutputStream);
      return this.defer.promise;
    }
  };

  

  function connectTo(aHost) {
    Services.prefs.setCharPref("network.dns.localDomains", aHost);
    let connection = new Connection(aHost);
    return connection.go();
  }

  add_test(function() {
    if (aBeforeConnect) {
      aBeforeConnect();
    }
    connectTo(aHost).then(function(conn) {
      do_print("handling " + aHost);
      do_check_eq(conn.result, aExpectedResult);
      if (aWithSecurityInfo) {
        aWithSecurityInfo(conn.transport.securityInfo
                              .QueryInterface(Ci.nsITransportSecurityInfo));
      }
      run_next_test();
    });
  });
}

function _getBinaryUtil(binaryUtilName) {
  let directoryService = Cc["@mozilla.org/file/directory_service;1"]
                           .getService(Ci.nsIProperties);

  let utilBin = directoryService.get("CurProcD", Ci.nsILocalFile);
  utilBin.append(binaryUtilName + (gIsWindows ? ".exe" : ""));
  
  
  if (!utilBin.exists()) {
    utilBin = directoryService.get("CurWorkD", Ci.nsILocalFile);
    while (utilBin.path.indexOf("xpcshell") != -1) {
      utilBin = utilBin.parent;
    }
    utilBin.append("bin");
    utilBin.append(binaryUtilName + (gIsWindows ? ".exe" : ""));
  }
  
  if (!utilBin.exists()) {
    utilBin.initWithPath("/data/local/xpcb/");
    utilBin.append(binaryUtilName);
  }
  do_check_true(utilBin.exists());
  return utilBin;
}


function _setupTLSServerTest(serverBinName)
{
  let certdb = Cc["@mozilla.org/security/x509certdb;1"]
                  .getService(Ci.nsIX509CertDB);
  
  addCertFromFile(certdb, "tlsserver/test-ca.der", "CTu,u,u");

  const CALLBACK_PORT = 8444;

  let directoryService = Cc["@mozilla.org/file/directory_service;1"]
                           .getService(Ci.nsIProperties);
  let envSvc = Cc["@mozilla.org/process/environment;1"]
                 .getService(Ci.nsIEnvironment);
  let greDir = directoryService.get("GreD", Ci.nsIFile);
  envSvc.set("DYLD_LIBRARY_PATH", greDir.path);
  envSvc.set("LD_LIBRARY_PATH", greDir.path);
  envSvc.set("MOZ_TLS_SERVER_DEBUG_LEVEL", "3");
  envSvc.set("MOZ_TLS_SERVER_CALLBACK_PORT", CALLBACK_PORT);

  let httpServer = new HttpServer();
  httpServer.registerPathHandler("/",
      function handleServerCallback(aRequest, aResponse) {
        aResponse.setStatusLine(aRequest.httpVersion, 200, "OK");
        aResponse.setHeader("Content-Type", "text/plain");
        let responseBody = "OK!";
        aResponse.bodyOutputStream.write(responseBody, responseBody.length);
        do_execute_soon(function() {
          httpServer.stop(run_next_test);
        });
      });
  httpServer.start(CALLBACK_PORT);

  let serverBin = _getBinaryUtil(serverBinName);
  let process = Cc["@mozilla.org/process/util;1"].createInstance(Ci.nsIProcess);
  process.init(serverBin);
  let certDir = directoryService.get("CurWorkD", Ci.nsILocalFile);
  certDir.append("tlsserver");
  do_check_true(certDir.exists());
  
  process.run(false, [ "sql:" + certDir.path ], 1);

  do_register_cleanup(function() {
    process.kill();
  });
}





function generateOCSPResponses(ocspRespArray, nssDBlocation)
{
  let utilBinName =  "GenerateOCSPResponse";
  let ocspGenBin = _getBinaryUtil(utilBinName);
  let retArray = new Array();

  for (let i = 0; i < ocspRespArray.length; i++) {
    let argArray = new Array();
    let ocspFilepre = do_get_file(i.toString() + ".ocsp", true);
    let filename = ocspFilepre.path;
    
    argArray.push("sql:" + nssDBlocation);
    argArray.push(ocspRespArray[i][0]); 
    argArray.push(ocspRespArray[i][1]); 
    argArray.push(ocspRespArray[i][2]); 
    argArray.push(filename);
    do_print("arg_array ="+argArray);

    let process = Cc["@mozilla.org/process/util;1"]
                    .createInstance(Ci.nsIProcess);
    process.init(ocspGenBin);
    process.run(true, argArray, 5);
    do_check_eq(0, process.exitValue);
    let ocspFile = do_get_file(i.toString() + ".ocsp", false);
    retArray.push(readFile(ocspFile));
    ocspFile.remove(false);
  }
  return retArray;
}




function getFailingHttpServer(serverPort, serverIdentities) {
  let httpServer = new HttpServer();
  httpServer.registerPrefixHandler("/", function(request, response) {
    do_check_true(false);
  });
  httpServer.identity.setPrimary("http", serverIdentities.shift(), serverPort);
  serverIdentities.forEach(function(identity) {
    httpServer.identity.add("http", identity, serverPort);
  });
  httpServer.start(serverPort);
  return httpServer;
}




















function startOCSPResponder(serverPort, identity, invalidIdentities,
                            nssDBLocation, expectedCertNames,
                            expectedBasePaths, expectedMethods,
                            expectedResponseTypes) {
  let ocspResponseGenerationArgs = expectedCertNames.map(
    function(expectedNick) {
      let responseType = "good";
      if (expectedResponseTypes && expectedResponseTypes.length >= 1) {
        responseType = expectedResponseTypes.shift();
      }
      return [responseType, expectedNick, "unused"];
    }
  );
  let ocspResponses = generateOCSPResponses(ocspResponseGenerationArgs,
                                            nssDBLocation);
  let httpServer = new HttpServer();
  httpServer.registerPrefixHandler("/",
    function handleServerCallback(aRequest, aResponse) {
      invalidIdentities.forEach(function(identity) {
        do_check_neq(aRequest.host, identity)
      });
      do_print("got request for: " + aRequest.path);
      let basePath = aRequest.path.slice(1).split("/")[0];
      if (expectedBasePaths.length >= 1) {
        do_check_eq(basePath, expectedBasePaths.shift());
      }
      do_check_true(expectedCertNames.length >= 1);
      if (expectedMethods && expectedMethods.length >= 1) {
        do_check_eq(aRequest.method, expectedMethods.shift());
      }
      aResponse.setStatusLine(aRequest.httpVersion, 200, "OK");
      aResponse.setHeader("Content-Type", "application/ocsp-response");
      aResponse.write(ocspResponses.shift());
    });
  httpServer.identity.setPrimary("http", identity, serverPort);
  invalidIdentities.forEach(function(identity) {
    httpServer.identity.add("http", identity, serverPort);
  });
  httpServer.start(serverPort);
  return {
    stop: function(callback) {
      
      do_check_eq(ocspResponses.length, 0);
      if (expectedMethods) {
        do_check_eq(expectedMethods.length, 0);
      }
      if (expectedBasePaths) {
        do_check_eq(expectedBasePaths.length, 0);
      }
      if (expectedResponseTypes) {
        do_check_eq(expectedResponseTypes.length, 0);
      }
      httpServer.stop(callback);
    }
  };
}
