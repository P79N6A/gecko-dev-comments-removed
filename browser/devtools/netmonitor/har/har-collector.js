


"use strict";

const { Cu, Ci, Cc } = require("chrome");
const { defer, all } = require("sdk/core/promise");
const { setTimeout, clearTimeout } = require("sdk/timers");
const { makeInfallible } = require("devtools/toolkit/DevToolsUtils.js");

const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});


const trace = {
  log: function(...args) {
  }
}





function HarCollector(options) {
  this.webConsoleClient = options.webConsoleClient;
  this.debuggerClient = options.debuggerClient;
  this.collector = options.collector;

  this.onNetworkEvent = this.onNetworkEvent.bind(this);
  this.onNetworkEventUpdate = this.onNetworkEventUpdate.bind(this);
  this.onRequestHeaders = this.onRequestHeaders.bind(this);
  this.onRequestCookies = this.onRequestCookies.bind(this);
  this.onRequestPostData = this.onRequestPostData.bind(this);
  this.onResponseHeaders = this.onResponseHeaders.bind(this);
  this.onResponseCookies = this.onResponseCookies.bind(this);
  this.onResponseContent = this.onResponseContent.bind(this);
  this.onEventTimings = this.onEventTimings.bind(this);

  this.onPageLoadTimeout = this.onPageLoadTimeout.bind(this);

  this.clear();
}

HarCollector.prototype = {
  

  start: function() {
    this.debuggerClient.addListener("networkEvent", this.onNetworkEvent);
    this.debuggerClient.addListener("networkEventUpdate", this.onNetworkEventUpdate);
  },

  stop: function() {
    this.debuggerClient.removeListener("networkEvent", this.onNetworkEvent);
    this.debuggerClient.removeListener("networkEventUpdate", this.onNetworkEventUpdate);
  },

  clear: function() {
    
    
    this.files = new Map();
    this.items = [];
    this.firstRequestStart = -1;
    this.lastRequestStart = -1;
    this.requests = [];
  },

  waitForHarLoad: function() {
    
    
    let deferred = defer();
    this.waitForResponses().then(() => {
      trace.log("HarCollector.waitForHarLoad; DONE HAR loaded!");
      deferred.resolve(this);
    });

    return deferred.promise;
  },

  waitForResponses: function() {
    trace.log("HarCollector.waitForResponses; " + this.requests.length);

    
    
    
    
    return waitForAll(this.requests).then(() => {
      
      
      
      
      
      
      return this.waitForTimeout().then(() => {
        
      }, () => {
        trace.log("HarCollector.waitForResponses; NEW requests " +
          "appeared during page timeout!");

        
        return this.waitForResponses();
      })
    });
  },

  

  




  waitForTimeout: function() {
    
    
    
    let timeout = Services.prefs.getIntPref(
      "devtools.netmonitor.har.pageLoadedTimeout");

    trace.log("HarCollector.waitForTimeout; " + timeout);

    this.pageLoadDeferred = defer();

    if (timeout <= 0) {
      this.pageLoadDeferred.resolve();
      return this.pageLoadDeferred.promise;
    }

    this.pageLoadTimeout = setTimeout(this.onPageLoadTimeout, timeout);

    return this.pageLoadDeferred.promise;
  },

  onPageLoadTimeout: function() {
    trace.log("HarCollector.onPageLoadTimeout;");

    
    this.pageLoadDeferred.resolve();
  },

  resetPageLoadTimeout: function() {
    
    if (this.pageLoadTimeout) {
      trace.log("HarCollector.resetPageLoadTimeout;");

      clearTimeout(this.pageLoadTimeout);
      this.pageLoadTimeout = null;
    }

    
    if (this.pageLoadDeferred) {
      this.pageLoadDeferred.reject();
      this.pageLoadDeferred = null;
    }
  },

  

  getFile: function(actorId) {
    return this.files.get(actorId);
  },

  getItems: function() {
    return this.items;
  },

  

  onNetworkEvent: function(type, packet) {
    
    if (packet.from != this.webConsoleClient.actor) {
      return;
    }

    trace.log("HarCollector.onNetworkEvent; " + type, packet);

    let { actor, startedDateTime, method, url, isXHR } = packet.eventActor;
    let startTime = Date.parse(startedDateTime);

    if (this.firstRequestStart == -1) {
      this.firstRequestStart = startTime;
    }

    if (this.lastRequestEnd < startTime) {
      this.lastRequestEnd = startTime;
    }

    let file = this.getFile(actor);
    if (file) {
      Cu.reportError("HarCollector.onNetworkEvent; ERROR " +
        "existing file conflict!");
      return;
    }

    file = {
      startedDeltaMillis: startTime - this.firstRequestStart,
      startedMillis: startTime,
      method: method,
      url: url,
      isXHR: isXHR
    };

    this.files.set(actor, file);

    
    this.items.push({
      attachment: file
    });
  },

  onNetworkEventUpdate: function(type, packet) {
    let actor = packet.from;

    
    
    let file = this.getFile(packet.from);
    if (!file) {
      Cu.reportError("HarCollector.onNetworkEventUpdate; ERROR " +
        "Unknown event actor: " + type, packet);
      return;
    }

    trace.log("HarCollector.onNetworkEventUpdate; " +
      packet.updateType, packet);

    let includeResponseBodies = Services.prefs.getBoolPref(
      "devtools.netmonitor.har.includeResponseBodies");

    let request;
    switch (packet.updateType) {
      case "requestHeaders":
        request = this.getData(actor, "getRequestHeaders", this.onRequestHeaders);
        break;
      case "requestCookies":
        request = this.getData(actor, "getRequestCookies", this.onRequestCookies);
        break;
      case "requestPostData":
        request = this.getData(actor, "getRequestPostData", this.onRequestPostData);
        break;
      case "responseHeaders":
        request = this.getData(actor, "getResponseHeaders", this.onResponseHeaders);
        break;
      case "responseCookies":
        request = this.getData(actor, "getResponseCookies", this.onResponseCookies);
        break;
      case "responseStart":
        file.httpVersion = packet.response.httpVersion;
        file.status = packet.response.status;
        file.statusText = packet.response.statusText;
        break;
      case "responseContent":
        file.contentSize = packet.contentSize;
        file.mimeType = packet.mimeType;
        file.transferredSize = packet.transferredSize;

        if (includeResponseBodies) {
          request = this.getData(actor, "getResponseContent", this.onResponseContent);
        }
        break;
      case "eventTimings":
        request = this.getData(actor, "getEventTimings", this.onEventTimings);
        break;
    }

    if (request) {
      this.requests.push(request);
    }

    this.resetPageLoadTimeout();
  },

  getData: function(actor, method, callback) {
    let deferred = defer();

    if (!this.webConsoleClient[method]) {
      Cu.reportError("HarCollector.getData; ERROR " +
        "Unknown method!");
      return;
    }

    let file = this.getFile(actor);

    trace.log("HarCollector.getData; REQUEST " + method +
      ", " + file.url, file);

    this.webConsoleClient[method](actor, response => {
      trace.log("HarCollector.getData; RESPONSE " + method +
        ", " + file.url, response);

      callback(response);
      deferred.resolve(response);
    });

    return deferred.promise;
  },

  





  onRequestHeaders: function(response) {
    let file = this.getFile(response.from);
    file.requestHeaders = response;

    this.getLongHeaders(response.headers);
  },

  





  onRequestCookies: function(response) {
    let file = this.getFile(response.from);
    file.requestCookies = response;

    this.getLongHeaders(response.cookies);
  },

  





  onRequestPostData: function(response) {
    trace.log("HarCollector.onRequestPostData;", response);

    let file = this.getFile(response.from);
    file.requestPostData = response;

    
    let text = response.postData.text;
    if (typeof text == "object") {
      this.getString(text).then(value => {
          response.postData.text = value;
      })
    }
  },

  





  onResponseHeaders: function(response) {
    let file = this.getFile(response.from);
    file.responseHeaders = response;

    this.getLongHeaders(response.headers);
  },

  





  onResponseCookies: function(response) {
    let file = this.getFile(response.from);
    file.responseCookies = response;

    this.getLongHeaders(response.cookies);
  },

  





  onResponseContent: function(response) {
    let file = this.getFile(response.from);
    file.mimeType = "text/plain";
    file.responseContent = response;

    
    let text = response.content.text;
    if (typeof text == "object") {
      this.getString(text).then(value => {
        response.content.text = value;
      })
    }
  },

  





  onEventTimings: function(response) {
    let file = this.getFile(response.from);
    file.eventTimings = response;

    let totalTime = response.totalTime;
    file.totalTime = totalTime;
    file.endedMillis = file.startedMillis + totalTime;
  },

  

  getLongHeaders: makeInfallible(function(headers) {
    for (let header of headers) {
      if (typeof header.value == "object") {
        this.getString(header.value).then(value => {
          header.value = value;
        });
      }
    }
  }),

  










  getString: function(stringGrip) {
    let promise = this.collector.getString(stringGrip);
    this.requests.push(promise);
    return promise;
  }
};









function waitForAll(promises) {
  
  let clone = promises.splice(0, promises.length);

  
  return all(clone).then(() => {
    
    
    if (promises.length) {
      return waitForAll(promises);
    }
  });
}


exports.HarCollector = HarCollector;
