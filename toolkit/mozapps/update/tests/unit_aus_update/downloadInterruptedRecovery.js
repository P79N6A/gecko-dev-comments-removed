






const INC_CONTRACT_ID = "@mozilla.org/network/incremental-download;1";

var gIncrementalDownloadClassID, gIncOldFactory;


var gIncrementalDownloadErrorType = 0;

var gNextRunFunc;
var gExpectedStatusResult;

function run_test() {
  setupTestCommon();

  logTestInfo("testing mar downloads, mar hash verification, and " +
              "mar download interrupted recovery");

  Services.prefs.setBoolPref(PREF_APP_UPDATE_STAGING_ENABLED, false);
  
  start_httpserver();
  setUpdateURLOverride(gURLData + "update.xml");
  
  overrideXHR(callHandleEvent);
  standardInit();
  do_execute_soon(run_test_pt1);
}


function finish_test() {
  stop_httpserver(doTestFinish);
}

function end_test() {
  cleanupMockIncrementalDownload();
}



function callHandleEvent() {
  gXHR.status = 400;
  gXHR.responseText = gResponseBody;
  try {
    let parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(Ci.nsIDOMParser);
    gXHR.responseXML = parser.parseFromString(gResponseBody, "application/xml");
  } catch (e) {
  }
  let e = { target: gXHR };
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
  let bestUpdate = gAUS.selectUpdate(gUpdates, gUpdateCount);
  let state = gAUS.downloadUpdate(bestUpdate, false);
  if (state == STATE_NONE || state == STATE_FAILED) {
    do_throw("nsIApplicationUpdateService:downloadUpdate returned " + state);
  }
  gAUS.addDownloadListener(downloadListener);
}

function check_test_helper_pt1_2() {
  do_check_eq(gStatusResult, gExpectedStatusResult);
  gAUS.removeDownloadListener(downloadListener);
  gNextRunFunc();
}

function setResponseBody(aHashFunction, aHashValue, aSize) {
  let patches = getRemotePatchString(null, null,
                                     aHashFunction, aHashValue, aSize);
  let updates = getRemoteUpdateString(patches);
  gResponseBody = getRemoteUpdatesXMLString(updates);
}

const newFactory = {
  createInstance: function(aOuter, aIID) {
    if (aOuter) {
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }
    return new IncrementalDownload().QueryInterface(aIID);
  },
  lockFactory: function(aLock) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFactory])
};

function initMockIncrementalDownload() {
  let registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
  gIncrementalDownloadClassID = registrar.contractIDToCID(INC_CONTRACT_ID);
  gIncOldFactory = Cm.getClassObject(Cc[INC_CONTRACT_ID], Ci.nsIFactory);
  registrar.unregisterFactory(gIncrementalDownloadClassID, gIncOldFactory);
  let components = [IncrementalDownload];
  registrar.registerFactory(gIncrementalDownloadClassID, "",
                            INC_CONTRACT_ID, newFactory);
}

function cleanupMockIncrementalDownload() {
  if (gIncOldFactory) {
    let registrar = Cm.QueryInterface(Ci.nsIComponentRegistrar);
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
  
  init: function(uri, file, chunkSize, intervalInSeconds) {
    this._destination = file;
    this._URI = uri;
    this._finalURI = uri;
  },

  start: function(observer, ctxt) {
    let tm = Cc["@mozilla.org/thread-manager;1"].
             getService(Ci.nsIThreadManager);
    
    
    tm.mainThread.dispatch(function() {
        this._observer = observer.QueryInterface(Ci.nsIRequestObserver);
        this._ctxt = ctxt;
        this._observer.onStartRequest(this, this._ctxt);
        let mar = getTestDirFile(FILE_SIMPLE_MAR);
        mar.copyTo(this._destination.parent, this._destination.leafName);
        let status = Cr.NS_OK
        switch (gIncrementalDownloadErrorType++) {
          case 0:
            status = Cr.NS_ERROR_NET_RESET;
            break;
          case 1:
            status = Cr.NS_ERROR_CONNECTION_REFUSED;
            break;
          case 2:
            status = Cr.NS_ERROR_NET_RESET;
            break;
          case 3:
            status = Cr.NS_OK;
            break;
          case 4:
            status = Cr.NS_ERROR_OFFLINE;
            
            
            let tm = Cc["@mozilla.org/thread-manager;1"].
                     getService(Ci.nsIThreadManager);
            tm.mainThread.dispatch(function() {
              Services.obs.notifyObservers(gAUS,
                                           "network:offline-status-changed",
                                           "online");
            }, Ci.nsIThread.DISPATCH_NORMAL);
            break;
        }
        this._observer.onStopRequest(this, this._ctxt, status);
      }.bind(this), Ci.nsIThread.DISPATCH_NORMAL);
  },

  get URI() {
    return this._URI;
  },

  get currentSize() {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  get destination() {
    return this._destination;
  },

  get finalURI() {
    return this._finalURI;
  },

  get totalSize() {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  
  cancel: function(aStatus) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  suspend: function() {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },
  isPending: function() {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
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
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIIncrementalDownload]),
};


function run_test_pt1() {
  initMockIncrementalDownload();
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with connection interruption",
                      Cr.NS_OK, run_test_pt2);
}


function run_test_pt2() {
  gIncrementalDownloadErrorType = 0;
  Services.prefs.setIntPref(PREF_APP_UPDATE_SOCKET_ERRORS, 2);
  Services.prefs.setIntPref(PREF_APP_UPDATE_RETRY_TIMEOUT, 0);
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR);

  let expectedResult;
  if (IS_TOOLKIT_GONK) {
    
    
    
    
    expectedResult = Cr.NS_OK;
  } else {
    expectedResult = Cr.NS_ERROR_NET_RESET;
  }
  run_test_helper_pt1("mar download with connection interruption without recovery",
                      expectedResult, run_test_pt3);
}


function run_test_pt3() {
  gIncrementalDownloadErrorType = 4;
  setResponseBody("MD5", MD5_HASH_SIMPLE_MAR);
  run_test_helper_pt1("mar download with offline mode",
                      Cr.NS_OK, finish_test);
}
