




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/debug.js", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/DeferredTask.jsm", this);
Cu.import("resource://gre/modules/Preferences.jsm");

const PREF_BRANCH = "toolkit.telemetry.";
const PREF_SERVER = PREF_BRANCH + "server";
const PREF_ENABLED = PREF_BRANCH + "enabled";
const PREF_CACHED_CLIENTID = PREF_BRANCH + "cachedClientID"
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";


const TELEMETRY_DELAY = 60000;

const TELEMETRY_TEST_DELAY = 100;

XPCOMUtils.defineLazyServiceGetter(this, "Telemetry",
                                   "@mozilla.org/base/telemetry;1",
                                   "nsITelemetry");
XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
                                  "resource://gre/modules/AsyncShutdown.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryFile",
                                  "resource://gre/modules/TelemetryFile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryLog",
                                  "resource://gre/modules/TelemetryLog.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ThirdPartyCookieProbe",
                                  "resource://gre/modules/ThirdPartyCookieProbe.jsm");

function generateUUID() {
  let str = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  
  return str.substring(1, str.length - 1);
}

this.EXPORTED_SYMBOLS = ["TelemetryPing"];

this.TelemetryPing = Object.freeze({
  Constants: Object.freeze({
    PREF_ENABLED: PREF_ENABLED,
    PREF_SERVER: PREF_SERVER,
  }),
  


  reset: function() {
    Impl._clientID = null;
    return this.setup();
  },
  


  setup: function() {
    return Impl.setupTelemetry(true);
  },

  


  observe: function (aSubject, aTopic, aData) {
    return Impl.observe(aSubject, aTopic, aData);
  },

  


  setServer: function(aServer) {
    return Impl.setServer(aServer);
  },

  


  send: function(aReason, aPingPayload) {
    return Impl.send(aReason, aPingPayload);
  },

  




   get clientID() {
    return Impl.clientID;
   },
});

let Impl = {
  _initialized: false,
  _prevValues: {},
  
  
  _previousBuildID: undefined,
  _clientID: null,

  popPayloads: function popPayloads(reason, externalPayload) {
    function payloadIter() {
      if (externalPayload && reason != "overdue-flush") {
        yield externalPayload;
      }
      let iterator = TelemetryFile.popPendingPings(reason);
      for (let data of iterator) {
        yield data;
      }
    }

    let payloadIterWithThis = payloadIter.bind(this);
    return { __iterator__: payloadIterWithThis };
  },

  


  setServer: function (aServer) {
    this._server = aServer;
  },

  


  send: function send(reason, aPayload) {
    return this.sendPingsFromIterator(this._server, reason,
                                      Iterator(this.popPayloads(reason, aPayload)));
  },

  sendPingsFromIterator: function sendPingsFromIterator(server, reason, i) {
    let p = [data for (data in i)].map((data) =>
      this.doPing(server, data).then(null, () => TelemetryFile.savePing(data, true)));

    return Promise.all(p);
  },

  finishPingRequest: function finishPingRequest(success, startTime, ping) {
    let hping = Telemetry.getHistogramById("TELEMETRY_PING");
    let hsuccess = Telemetry.getHistogramById("TELEMETRY_SUCCESS");

    hsuccess.add(success);
    hping.add(new Date() - startTime);

    if (success) {
      return TelemetryFile.cleanupPingFile(ping);
    } else {
      return Promise.resolve();
    }
  },

  submissionPath: function submissionPath(ping) {
    let slug;
    if (!ping) {
      slug = this._uuid;
    } else {
      let info = ping.payload.info;
      let pathComponents = [ping.slug, info.reason, info.appName,
                            info.appVersion, info.appUpdateChannel,
                            info.appBuildID];
      slug = pathComponents.join("/");
    }
    return "/submit/telemetry/" + slug;
  },

  doPing: function doPing(server, ping) {
    let deferred = Promise.defer();
    let url = server + this.submissionPath(ping);
    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);
    request.mozBackgroundRequest = true;
    request.open("POST", url, true);
    request.overrideMimeType("text/plain");
    request.setRequestHeader("Content-Type", "application/json; charset=UTF-8");

    let startTime = new Date();

    function handler(success) {
      return function(event) {
        this.finishPingRequest(success, startTime, ping).then(() => {
          if (success) {
            deferred.resolve();
          } else {
            deferred.reject(event);
          }
        });
      };
    }
    request.addEventListener("error", handler(false).bind(this), false);
    request.addEventListener("load", handler(true).bind(this), false);

    request.setRequestHeader("Content-Encoding", "gzip");
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let utf8Payload = converter.ConvertFromUnicode(JSON.stringify(ping.payload));
    utf8Payload += converter.Finish();
    let payloadStream = Cc["@mozilla.org/io/string-input-stream;1"]
                        .createInstance(Ci.nsIStringInputStream);
    payloadStream.data = this.gzipCompressString(utf8Payload);
    request.send(payloadStream);
    return deferred.promise;
  },

  gzipCompressString: function gzipCompressString(string) {
    let observer = {
      buffer: "",
      onStreamComplete: function(loader, context, status, length, result) {
        this.buffer = String.fromCharCode.apply(this, result);
      }
    };

    let scs = Cc["@mozilla.org/streamConverters;1"]
              .getService(Ci.nsIStreamConverterService);
    let listener = Cc["@mozilla.org/network/stream-loader;1"]
                  .createInstance(Ci.nsIStreamLoader);
    listener.init(observer);
    let converter = scs.asyncConvertData("uncompressed", "gzip",
                                         listener, null);
    let stringStream = Cc["@mozilla.org/io/string-input-stream;1"]
                       .createInstance(Ci.nsIStringInputStream);
    stringStream.data = string;
    converter.onStartRequest(null, null);
    converter.onDataAvailable(null, null, stringStream, 0, string.length);
    converter.onStopRequest(null, null, null);
    return observer.buffer;
  },

  


  enableTelemetryRecording: function enableTelemetryRecording(testing) {

#ifdef MOZILLA_OFFICIAL
    if (!Telemetry.canSend && !testing) {
      
      
      
      Telemetry.canRecord = false;
      return false;
    }
#endif

    let enabled = Preferences.get(PREF_ENABLED, false);
    this._server = Preferences.get(PREF_SERVER, undefined);
    if (!enabled) {
      
      
      Telemetry.canRecord = false;
      return false;
    }

    return true;
  },

  


  setupTelemetry: function setupTelemetry(testing) {
    
    this._thirdPartyCookies = new ThirdPartyCookieProbe();
    this._thirdPartyCookies.init();

    if (!this.enableTelemetryRecording(testing)) {
      return Promise.resolve();
    }

    
    
    
    
    this._clientID = Preferences.get(PREF_CACHED_CLIENTID, null);

    
    
    
    let deferred = Promise.defer();
    let delayedTask = new DeferredTask(function* () {
      this._initialized = true;

      yield TelemetryFile.loadSavedPings();
      
      
      if (TelemetryFile.pingsOverdue > 0) {
        
        
        
        yield this.send("overdue-flush");
      }

      if ("@mozilla.org/datareporting/service;1" in Cc) {
        let drs = Cc["@mozilla.org/datareporting/service;1"]
                    .getService(Ci.nsISupports)
                    .wrappedJSObject;
        this._clientID = yield drs.getClientID();
        
        Preferences.set(PREF_CACHED_CLIENTID, this._clientID);
      } else {
        
        Preferences.reset(PREF_CACHED_CLIENTID);
      }

      Telemetry.asyncFetchTelemetryData(function () {});
      deferred.resolve();

    }.bind(this), testing ? TELEMETRY_TEST_DELAY : TELEMETRY_DELAY);

    delayedTask.arm();
    return deferred.promise;
  },

  


  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
    case "profile-after-change":
      
      return this.setupTelemetry();
    }
  },

  get clientID() {
    return this._clientID;
  },
};
