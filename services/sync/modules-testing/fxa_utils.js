"use strict";

this.EXPORTED_SYMBOLS = [
  "Assert_rejects",
  "initializeIdentityWithTokenServerResponse",
];

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-sync/main.js");
Cu.import("resource://services-sync/browserid_identity.js");
Cu.import("resource://services-common/tokenserverclient.js");
Cu.import("resource://testing-common/services/common/logging.js");
Cu.import("resource://testing-common/services/sync/utils.js");



function Assert_rejects(promise, message) {
  let deferred = Promise.defer();
  promise.then(
    () => deferred.reject(message || "Expected the promise to be rejected"),
    deferred.resolve
  );
  return deferred.promise;
}



this.initializeIdentityWithTokenServerResponse = function(response) {
  
  
  let requestLog = Log.repository.getLogger("testing.mock-rest");
  if (!requestLog.appenders.length) { 
    requestLog.addAppender(new Log.DumpAppender());
    requestLog.level = Log.Level.Trace;
  }

  
  function MockRESTRequest(url) {};
  MockRESTRequest.prototype = {
    _log: requestLog,
    setHeader: function() {},
    get: function(callback) {
      this.response = response;
      callback.call(this);
    }
  }
  
  function MockTSC() { }
  MockTSC.prototype = new TokenServerClient();
  MockTSC.prototype.constructor = MockTSC;
  MockTSC.prototype.newRESTRequest = function(url) {
    return new MockRESTRequest(url);
  }
  
  MockTSC.prototype.observerPrefix = "weave:service";

  
  Weave.Status.__authManager = Weave.Service.identity = new BrowserIDManager();
  Weave.Service._clusterManager = Weave.Service.identity.createClusterManager(Weave.Service);
  let browseridManager = Weave.Service.identity;
  
  if (!(browseridManager instanceof BrowserIDManager)) {
    throw new Error("sync isn't configured for browserid_identity");
  }
  let mockTSC = new MockTSC()
  configureFxAccountIdentity(browseridManager);
  browseridManager._tokenServerClient = mockTSC;
}
