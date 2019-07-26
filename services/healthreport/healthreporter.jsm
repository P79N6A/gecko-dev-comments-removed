



"use strict";

this.EXPORTED_SYMBOLS = ["HealthReporter"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://services-common/bagheeraclient.js");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/observers.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/commonjs/promise/core.js");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/services/healthreport/policy.jsm");
Cu.import("resource://gre/modules/services/metrics/collector.jsm");




const OLDEST_ALLOWED_YEAR = 2012;





















this.HealthReporter = function HealthReporter(branch) {
  if (!branch.endsWith(".")) {
    throw new Error("Branch argument must end with a period (.): " + branch);
  }

  this._log = Log4Moz.repository.getLogger("Services.HealthReport.HealthReporter");

  this._prefs = new Preferences(branch);

  let policyBranch = new Preferences(branch + "policy.");
  this._policy = new HealthReportPolicy(policyBranch, this);
  this._collector = new MetricsCollector();

  if (!this.serverURI) {
    throw new Error("No server URI defined. Did you forget to define the pref?");
  }

  if (!this.serverNamespace) {
    throw new Error("No server namespace defined. Did you forget a pref?");
  }
}

HealthReporter.prototype = {
  






  get lastPingDate() {
    return CommonUtils.getDatePref(this._prefs, "lastPingTime", 0, this._log,
                                   OLDEST_ALLOWED_YEAR);
  },

  set lastPingDate(value) {
    CommonUtils.setDatePref(this._prefs, "lastPingTime", value,
                            OLDEST_ALLOWED_YEAR);
  },

  





  get serverURI() {
    return this._prefs.get("documentServerURI", null);
  },

  set serverURI(value) {
    if (!value) {
      throw new Error("serverURI must have a value.");
    }

    if (typeof(value) != "string") {
      throw new Error("serverURI must be a string: " + value);
    }

    this._prefs.set("documentServerURI", value);
  },

  


  get serverNamespace() {
    return this._prefs.get("documentServerNamespace", "metrics");
  },

  set serverNamespace(value) {
    if (!value) {
      throw new Error("serverNamespace must have a value.");
    }

    if (typeof(value) != "string") {
      throw new Error("serverNamespace must be a string: " + value);
    }

    this._prefs.set("documentServerNamespace", value);
  },

  








  get lastSubmitID() {
    return this._prefs.get("lastSubmitID", null) || null;
  },

  set lastSubmitID(value) {
    this._prefs.set("lastSubmitID", value || "");
  },

  




  haveRemoteData: function haveRemoteData() {
    return !!this.lastSubmitID;
  },

  








  start: function start() {
    let onExists = function onExists() {
      this._policy.startPolling();
      this._log.info("HealthReporter started.");

      return Promise.resolve();
    }.bind(this);

    return this._ensureDirectoryExists(this._stateDir)
               .then(onExists);
  },

  


  stop: function stop() {
    this._policy.stopPolling();
  },

  








  registerProvider: function registerProvider(provider) {
    return this._collector.registerProvider(provider);
  },

  



























  registerProvidersFromCategoryManager:
    function registerProvidersFromCategoryManager(category) {

    let cm = Cc["@mozilla.org/categorymanager;1"]
               .getService(Ci.nsICategoryManager);

    let enumerator = cm.enumerateCategory(category);
    while (enumerator.hasMoreElements()) {
      let entry = enumerator.getNext()
                            .QueryInterface(Ci.nsISupportsCString)
                            .toString();

      let uri = cm.getCategoryEntry(category, entry);
      this._log.info("Attempting to load provider from category manager: " +
                     entry + " from " + uri);

      try {
        let ns = {};
        Cu.import(uri, ns);

        let provider = new ns[entry]();
        this.registerProvider(provider);
      } catch (ex) {
        this._log.warn("Error registering provider from category manager: " +
                       entry + "; " + CommonUtils.exceptionStr(ex));
        continue;
      }
    }
  },

  


  collectMeasurements: function collectMeasurements() {
    return this._collector.collectConstantMeasurements();
  },

  







  recordPolicyRejection: function recordPolicyRejection(reason) {
    this._policy.recordUserRejection(reason);
  },

  







  recordPolicyAcceptance: function recordPolicyAcceptance(reason) {
    this._policy.recordUserAcceptance(reason);
  },

  





  get dataSubmissionPolicyAccepted() {
    return this._policy.dataSubmissionPolicyAccepted;
  },

  


  get willUploadData() {
    return this._policy.dataSubmissionPolicyAccepted &&
           this._policy.dataUploadEnabled;
  },

  







  requestDeleteRemoteData: function requestDeleteRemoteData(reason) {
    if (!this.lastSubmitID) {
      return;
    }

    return this._policy.deleteRemoteData(reason);
  },

  getJSONPayload: function getJSONPayload() {
    let o = {
      version: 1,
      thisPingDate: this._formatDate(this._now()),
      providers: {},
    };

    let lastPingDate = this.lastPingDate;
    if (lastPingDate.getTime() > 0) {
      o.lastPingDate = this._formatDate(lastPingDate);
    }

    for (let [name, provider] of this._collector.collectionResults) {
      o.providers[name] = provider;
    }

    return JSON.stringify(o);
  },

  _onBagheeraResult: function _onBagheeraResult(request, isDelete, result) {
    this._log.debug("Received Bagheera result.");

    let promise = Promise.resolve(null);

    if (!result.transportSuccess) {
      request.onSubmissionFailureSoft("Network transport error.");
      return promise;
    }

    if (!result.serverSuccess) {
      request.onSubmissionFailureHard("Server failure.");
      return promise;
    }

    let now = this._now();

    if (isDelete) {
      this.lastSubmitID = null;
    } else {
      this.lastSubmitID = result.id;
      this.lastPingDate = now;
    }

    request.onSubmissionSuccess(now);

    return promise;
  },

  _onSubmitDataRequestFailure: function _onSubmitDataRequestFailure(error) {
    this._log.error("Error processing request to submit data: " +
                    CommonUtils.exceptionStr(error));
  },

  _formatDate: function _formatDate(date) {
    
    return date.toISOString().substr(0, 10);
  },


  _uploadData: function _uploadData(request) {
    let id = CommonUtils.generateUUID();

    this._log.info("Uploading data to server: " + this.serverURI + " " +
                   this.serverNamespace + ":" + id);
    let client = new BagheeraClient(this.serverURI);

    let payload = this.getJSONPayload();

    return this._saveLastPayload(payload)
               .then(client.uploadJSON.bind(client,
                                            this.serverNamespace,
                                            id,
                                            payload,
                                            this.lastSubmitID))
               .then(this._onBagheeraResult.bind(this, request, false));
  },

  _deleteRemoteData: function _deleteRemoteData(request) {
    if (!this.lastSubmitID) {
      this._log.info("Received request to delete remote data but no data stored.");
      request.onNoDataAvailable();
      return;
    }

    this._log.warn("Deleting remote data.");
    let client = new BagheeraClient(this.serverURI);

    return client.deleteDocument(this.serverNamespace, this.lastSubmitID)
                 .then(this._onBagheeraResult.bind(this, request, true),
                       this._onSubmitDataRequestFailure.bind(this));

  },

  get _stateDir() {
    let profD = OS.Constants.Path.profileDir;

    
    if (!profD || !profD.length) {
      throw new Error("Could not obtain profile directory. OS.File not " +
                      "initialized properly?");
    }

    return OS.Path.join(profD, "healthreport");
  },

  _ensureDirectoryExists: function _ensureDirectoryExists(path) {
    let deferred = Promise.defer();

    OS.File.makeDir(path).then(
      function onResult() {
        deferred.resolve(true);
      },
      function onError(error) {
        if (error.becauseExists) {
          deferred.resolve(true);
          return;
        }

        deferred.reject(error);
      }
    );

    return deferred.promise;
  },

  get _lastPayloadPath() {
    return OS.Path.join(this._stateDir, "lastpayload.json");
  },

  _saveLastPayload: function _saveLastPayload(payload) {
    let path = this._lastPayloadPath;
    let pathTmp = path + ".tmp";

    let encoder = new TextEncoder();
    let buffer = encoder.encode(payload);

    return OS.File.writeAtomic(path, buffer, {tmpPath: pathTmp});
  },

  








  getLastPayload: function getLoadPayload() {
    let path = this._lastPayloadPath;

    return OS.File.read(path).then(
      function onData(buffer) {
        let decoder = new TextDecoder();
        let json = JSON.parse(decoder.decode(buffer));

        return Promise.resolve(json);
      },
      function onError(error) {
        return Promise.reject(error);
      }
    );
  },

  _now: function _now() {
    return new Date();
  },

  
  
  

  onRequestDataUpload: function onRequestDataSubmission(request) {
    this.collectMeasurements()
        .then(this._uploadData.bind(this, request),
              this._onSubmitDataRequestFailure.bind(this));
  },

  onNotifyDataPolicy: function onNotifyDataPolicy(request) {
    
    
    Observers.notify("healthreport:notify-data-policy:request", request);
  },

  onRequestRemoteDelete: function onRequestRemoteDelete(request) {
    this._deleteRemoteData(request);
  },

  
  
  
};

Object.freeze(HealthReporter.prototype);

