






const INC_CONTRACT_ID = "@mozilla.org/network/incremental-download;1";
AUS_Cu.import("resource://gre/modules/FileUtils.jsm");
AUS_Cu.import("resource://gre/modules/Services.jsm");
AUS_Cu.import("resource://gre/modules/XPCOMUtils.jsm")

var gNextRunFunc;
var gStatusResult;
var gExpectedStatusResult;
var gIncrementalDownloadClassID, gIncOldFactory;



var gIncrementalDownloadErrorType = 0;

function run_test() {
  do_test_pending();
  do_register_cleanup(end_test);
  Services.prefs.setBoolPref(PREF_APP_UPDATE_STAGE_ENABLED, false);
  removeUpdateDirsAndFiles();
  setUpdateURLOverride();
  
  overrideXHR(callHandleEvent);
  standardInit();
  
  start_httpserver(URL_PATH);
  do_execute_soon(run_test_pt1);
}


function finish_test() {
  stop_httpserver(do_test_finished);
}

function end_test() {
  cleanupMockIncrementalDownload();
  cleanUp();
}



function callHandleEvent() {
  gXHR.status = 400;
  gXHR.responseText = gResponseBody;
  try {
    var parser = AUS_Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(AUS_Ci.nsIDOMParser);
    gXHR.responseXML = parser.parseFromString(gResponseBody, "application/xml");
  }
  catch(e) {
  }
  var e = { target: gXHR };
  gXHR.onload(e);
}



function run_test_helper_pt1(aMsg, aExpectedStatusResult, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gStatusResult = null;
  gCheckFunc = check_test_helper_pt1_1;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusResult = aExpectedStatusResult;
  logTestInfo(aMsg, Components.stack.caller);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_pt1_1() {
  do_check_eq(gUpdateCount, 1);
  gCheckFunc = check_test_helper_pt1_2;
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  var state = gAUS.downloadUpdate(bestUpdate, false);
  if (state == STATE_NONE || state == STATE_FAILED)
    do_throw("nsIApplicationUpdateService:downloadUpdate returned " + state);
  gAUS.addDownloadListener(downloadListener);
}

function check_test_helper_pt1_2() {
  do_check_eq(gStatusResult, gExpectedStatusResult);
  gAUS.removeDownloadListener(downloadListener);
  gNextRunFunc();
}




function run_test_helper_bug828858_pt1(aMsg, aExpectedStatusResult, aNextRunFunc) {
  gUpdates = null;
  gUpdateCount = null;
  gStatusResult = null;
  gCheckFunc = check_test_helper_bug828858_pt1_1;
  gNextRunFunc = aNextRunFunc;
  gExpectedStatusResult = aExpectedStatusResult;
  logTestInfo(aMsg, Components.stack.caller);
  gUpdateChecker.checkForUpdates(updateCheckListener, true);
}

function check_test_helper_bug828858_pt1_1() {
  do_check_eq(gUpdateCount, 1);
  gCheckFunc = check_test_helper_bug828858_pt1_2;
  var bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  var state = gAUS.downloadUpdate(bestUpdate, false);
  if (state == STATE_NONE || state == STATE_FAILED)
    do_throw("nsIApplicationUpdateService:downloadUpdate returned " + state);
  gAUS.addDownloadListener(downloadListener);
}

function check_test_helper_bug828858_pt1_2() {
  if (gStatusResult == AUS_Cr.NS_ERROR_CONTENT_CORRUPTED) {
    do_check_eq(gStatusResult, AUS_Cr.NS_ERROR_CONTENT_CORRUPTED);
  } else {
    do_check_eq(gStatusResult, gExpectedStatusResult);
  }
  gAUS.removeDownloadListener(downloadListener);
  gNextRunFunc();
}

function setResponseBody(aHashFunction, aHashValue, aSize) {
  var patches = getRemotePatchString(null, null,
                                     aHashFunction, aHashValue, aSize);
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
}


function run_test_pt1() {
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid MD5 hash",
                      AUS_Cr.NS_OK, run_test_pt2);
}


function run_test_pt2() {
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid MD5 hash",
                      AUS_Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt3);
}


function run_test_pt3() {
  setResponseBody("SHA1", SHA1_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA1 hash",
                      AUS_Cr.NS_OK, run_test_pt4);
}


function run_test_pt4() {
  setResponseBody("SHA1", SHA1_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA1 hash",
                      AUS_Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt5);
}


function run_test_pt5() {
  setResponseBody("SHA256", SHA256_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA256 hash",
                      AUS_Cr.NS_OK, run_test_pt6);
}


function run_test_pt6() {
  setResponseBody("SHA256", SHA256_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA256 hash",
                      AUS_Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt7);
}


function run_test_pt7() {
  setResponseBody("SHA384", SHA384_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA384 hash",
                      AUS_Cr.NS_OK, run_test_pt8);
}


function run_test_pt8() {
  setResponseBody("SHA384", SHA384_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA384 hash",
                      AUS_Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt9);
}


function run_test_pt9() {
  setResponseBody("SHA512", SHA512_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with a valid SHA512 hash",
                      AUS_Cr.NS_OK, run_test_pt10);
}


function run_test_pt10() {
  setResponseBody("SHA512", SHA512_HASH_SIMPLE_MAR + "0");
  run_test_helper_pt1("mar download with an invalid SHA512 hash",
                      AUS_Cr.NS_ERROR_CORRUPTED_CONTENT, run_test_pt11);
}


function run_test_pt11() {
  var patches = getRemotePatchString(null, URL_HOST + URL_PATH + "/missing.mar");
  var updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
  run_test_helper_pt1("mar download with the mar not found",
                      AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt12);
}


function run_test_pt12() {
  const arbitraryFileSize = 1024000;
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR ,arbitraryFileSize);
  if (IS_TOOLKIT_GONK) {
    
    
    
    
    
    
    run_test_helper_bug828858_pt1("mar download with a valid MD5 hash but invalid file size",
                                  AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt13);
  } else {
    run_test_helper_pt1("mar download with a valid MD5 hash but invalid file size",
                        AUS_Cr.NS_ERROR_UNEXPECTED, run_test_pt13);
  }
}

var newFactory = {
  createInstance: function(aOuter, aIID) {
    if (aOuter)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return new IncrementalDownload().QueryInterface(aIID);
  },
  lockFactory: function(aLock) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },
  QueryInterface: XPCOMUtils.generateQI([AUS_Ci.nsIFactory])
};

function initMockIncrementalDownload() {
  var registrar = AUS_Cm.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  gIncrementalDownloadClassID = registrar.contractIDToCID(INC_CONTRACT_ID);
  gIncOldFactory = AUS_Cm.getClassObject(AUS_Cc[INC_CONTRACT_ID],
                                     AUS_Ci.nsIFactory);
  registrar.unregisterFactory(gIncrementalDownloadClassID, gIncOldFactory);
  var components = [IncrementalDownload];
  registrar.registerFactory(gIncrementalDownloadClassID, "",
                            INC_CONTRACT_ID, newFactory);
  gIncOldFactory = AUS_Cm.getClassObject(AUS_Cc[INC_CONTRACT_ID],
                                     AUS_Ci.nsIFactory);
}

function cleanupMockIncrementalDownload() {
  if (gIncOldFactory) {
    var registrar = AUS_Cm.QueryInterface(AUS_Ci.nsIComponentRegistrar);
    registrar.unregisterFactory(gIncrementalDownloadClassID, newFactory);
    registrar.registerFactory(gIncrementalDownloadClassID, "",
                              INC_CONTRACT_ID, gIncOldFactory);
  }
  gIncOldFactory = null;
}







function IncrementalDownload() {
  this.wrappedJSObject = this;
}

IncrementalDownload.prototype = {
  QueryInterface: XPCOMUtils.generateQI([AUS_Ci.nsIIncrementalDownload]),

  
  init: function(uri, file, chunkSize, intervalInSeconds) {
    this._destination = file;
    this._URI = uri;
    this._finalURI = uri;
  },

  start: function(observer, ctxt) {
    var tm = Components.classes["@mozilla.org/thread-manager;1"].
                        getService(AUS_Ci.nsIThreadManager);
    
    
    tm.mainThread.dispatch(function() {
        this._observer = observer.QueryInterface(AUS_Ci.nsIRequestObserver);
        this._ctxt = ctxt;
        this._observer.onStartRequest(this, this.ctxt);
        let mar = do_get_file("data/" + FILE_SIMPLE_MAR);
        mar.copyTo(this._destination.parent, this._destination.leafName);
        var status = AUS_Cr.NS_OK
        switch (gIncrementalDownloadErrorType++) {
          case 0:
            status = AUS_Cr.NS_ERROR_NET_RESET;
          break;
          case 1:
            status = AUS_Cr.NS_ERROR_CONNECTION_REFUSED;
          break;
          case 2:
            status = AUS_Cr.NS_ERROR_NET_RESET;
          break;
          case 3:
            status = AUS_Cr.NS_OK;
            break;
          case 4:
            status = AUS_Cr.NS_ERROR_OFFLINE;
            
            
            var tm = Components.classes["@mozilla.org/thread-manager;1"].
                                getService(AUS_Ci.nsIThreadManager);
            tm.mainThread.dispatch(function() {
              Services.obs.notifyObservers(gAUS,
                                           "network:offline-status-changed",
                                           "online");
            }, AUS_Ci.nsIThread.DISPATCH_NORMAL);
          break;
        }
        this._observer.onStopRequest(this, this._ctxt, status);
      }.bind(this), AUS_Ci.nsIThread.DISPATCH_NORMAL);
  },

  get URI() {
    return this._URI;
  },

  get currentSize() {
    throw AUS_Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  get destination() {
    return this._destination;
  },

  get finalURI() {
    return this._finalURI;
  },

  get totalSize() {
    throw AUS_Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  
  cancel: function(aStatus) {
    throw AUS_Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  suspend: function() {
    throw AUS_Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  isPending: function() {
    throw AUS_Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  _loadFlags: 0,
  get loadFlags() {
    return this._loadFlags;
  },
  set loadFlags(val) {
    this._loadFlags = val;
  },

  _loadGroup: null,
  get loadGroup() {
    return this._loadGroup;
  },
  set loadGroup(val) {
    this._loadGroup = val;
  },

  _name: "",
  get name() {
    return this._name;
  },

  _status: 0,
  get status() {
    return this._status;
  }
}


function run_test_pt13() {
  initMockIncrementalDownload();
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with connection interruption",
                      AUS_Cr.NS_OK, run_test_pt14);
}


function run_test_pt14() {
  gIncrementalDownloadErrorType = 0;
  Services.prefs.setIntPref(PREF_APP_UPDATE_SOCKET_ERRORS, 2);
  Services.prefs.setIntPref(PREF_APP_UPDATE_RETRY_TIMEOUT, 0);
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR);

  var expectedResult;
  if (IS_TOOLKIT_GONK) {
    
    
    
    
    expectedResult = AUS_Cr.NS_OK;
  } else {
    expectedResult = AUS_Cr.NS_ERROR_NET_RESET;
  }
  run_test_helper_pt1("mar download with connection interruption without recovery",
                      expectedResult, run_test_pt15);
}


function run_test_pt15() {
  gIncrementalDownloadErrorType = 4;
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with offline mode",
                      AUS_Cr.NS_OK, finish_test);
}


const downloadListener = {
  onStartRequest: function DL_onStartRequest(request, context) {
  },

  onProgress: function DL_onProgress(request, context, progress, maxProgress) {
  },

  onStatus: function DL_onStatus(request, context, status, statusText) {
  },

  onStopRequest: function DL_onStopRequest(request, context, status) {
    gStatusResult = status;
    
    do_execute_soon(gCheckFunc);
  },

  QueryInterface: function DL_QueryInterface(iid) {
    if (!iid.equals(AUS_Ci.nsIRequestObserver) &&
        !iid.equals(AUS_Ci.nsIProgressEventSink) &&
        !iid.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
