





"use strict";

const {Cc, Ci, Cu} = require("chrome");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const EventEmitter = require("devtools/toolkit/event-emitter");
const promise = require("promise");

loader.lazyImporter(this, "LongStringClient", "resource://gre/modules/devtools/dbg-client.jsm");











function WebConsoleClient(aDebuggerClient, aResponse)
{
  this._actor = aResponse.from;
  this._client = aDebuggerClient;
  this._longStrings = {};
  this.traits = aResponse.traits || {};
  this.events = [];
  this._networkRequests = new Map();

  this.pendingEvaluationResults = new Map();
  this.onEvaluationResult = this.onEvaluationResult.bind(this);
  this.onNetworkEvent = this._onNetworkEvent.bind(this);
  this.onNetworkEventUpdate = this._onNetworkEventUpdate.bind(this);

  this._client.addListener("evaluationResult", this.onEvaluationResult);
  this._client.addListener("networkEvent", this.onNetworkEvent);
  this._client.addListener("networkEventUpdate", this.onNetworkEventUpdate);
  EventEmitter.decorate(this);
}

exports.WebConsoleClient = WebConsoleClient;

WebConsoleClient.prototype = {
  _longStrings: null,
  traits: null,

  





  _networkRequests: null,

  getNetworkRequest(actorId) {
    return this._networkRequests.get(actorId);
  },

  hasNetworkRequest(actorId) {
    return this._networkRequests.has(actorId);
  },

  removeNetworkRequest(actorId) {
    this._networkRequests.delete(actorId);
  },

  getNetworkEvents() {
    return this._networkRequests.values();
  },

  get actor() { return this._actor; },

  









  _onNetworkEvent: function (type, packet)
  {
    if (packet.from == this._actor) {
      let actor = packet.eventActor;
      let networkInfo = {
        _type: "NetworkEvent",
        timeStamp: actor.timeStamp,
        node: null,
        actor: actor.actor,
        discardRequestBody: true,
        discardResponseBody: true,
        startedDateTime: actor.startedDateTime,
        request: {
          url: actor.url,
          method: actor.method,
        },
        isXHR: actor.isXHR,
        response: {},
        timings: {},
        updates: [], 
        private: actor.private,
        fromCache: actor.fromCache
      };
      this._networkRequests.set(actor.actor, networkInfo);

      this.emit("networkEvent", networkInfo);
    }
  },

  









  _onNetworkEventUpdate: function (type, packet)
  {
    let networkInfo = this.getNetworkRequest(packet.from);
    if (!networkInfo) {
      return;
    }

    networkInfo.updates.push(packet.updateType);

    switch (packet.updateType) {
      case "requestHeaders":
        networkInfo.request.headersSize = packet.headersSize;
        break;
      case "requestPostData":
        networkInfo.discardRequestBody = packet.discardRequestBody;
        networkInfo.request.bodySize = packet.dataSize;
        break;
      case "responseStart":
        networkInfo.response.httpVersion = packet.response.httpVersion;
        networkInfo.response.status = packet.response.status;
        networkInfo.response.statusText = packet.response.statusText;
        networkInfo.response.headersSize = packet.response.headersSize;
        networkInfo.response.remoteAddress = packet.response.remoteAddress;
        networkInfo.response.remotePort = packet.response.remotePort;
        networkInfo.discardResponseBody = packet.response.discardResponseBody;
        break;
      case "responseContent":
        networkInfo.response.content = {
          mimeType: packet.mimeType,
        };
        networkInfo.response.bodySize = packet.contentSize;
        networkInfo.response.transferredSize = packet.transferredSize;
        networkInfo.discardResponseBody = packet.discardResponseBody;
        break;
      case "eventTimings":
        networkInfo.totalTime = packet.totalTime;
        break;
      case "securityInfo":
        networkInfo.securityInfo = packet.state;
        break;
    }

    this.emit("networkEventUpdate", {
      packet: packet,
      networkInfo
    });
  },

  









  getCachedMessages: function WCC_getCachedMessages(types, aOnResponse)
  {
    let packet = {
      to: this._actor,
      type: "getCachedMessages",
      messageTypes: types,
    };
    this._client.request(packet, aOnResponse);
  },

  







  inspectObjectProperties:
  function WCC_inspectObjectProperties(aActor, aOnResponse)
  {
    let packet = {
      to: aActor,
      type: "inspectProperties",
    };
    this._client.request(packet, aOnResponse);
  },

  



































  evaluateJS: function WCC_evaluateJS(aString, aOnResponse, aOptions = {})
  {
    let packet = {
      to: this._actor,
      type: "evaluateJS",
      text: aString,
      bindObjectActor: aOptions.bindObjectActor,
      frameActor: aOptions.frameActor,
      url: aOptions.url,
      selectedNodeActor: aOptions.selectedNodeActor,
    };
    this._client.request(packet, aOnResponse);
  },

  



  evaluateJSAsync: function(aString, aOnResponse, aOptions = {})
  {
    
    if (!this.traits.evaluateJSAsync) {
      this.evaluateJS(aString, aOnResponse, aOptions);
      return;
    }

    let packet = {
      to: this._actor,
      type: "evaluateJSAsync",
      text: aString,
      bindObjectActor: aOptions.bindObjectActor,
      frameActor: aOptions.frameActor,
      url: aOptions.url,
      selectedNodeActor: aOptions.selectedNodeActor,
    };

    this._client.request(packet, response => {
      
      
      if (this.pendingEvaluationResults) {
        this.pendingEvaluationResults.set(response.resultID, aOnResponse);
      }
    });
  },

  


  onEvaluationResult: function(aNotification, aPacket) {
    
    
    
    let onResponse = this.pendingEvaluationResults.get(aPacket.resultID);
    if (onResponse) {
      onResponse(aPacket);
      this.pendingEvaluationResults.delete(aPacket.resultID);
    } else {
      DevToolsUtils.reportException("onEvaluationResult",
        "No response handler for an evaluateJSAsync result (resultID: " + aPacket.resultID + ")");
    }
  },

  











  autocomplete: function WCC_autocomplete(aString, aCursor, aOnResponse, aFrameActor)
  {
    let packet = {
      to: this._actor,
      type: "autocomplete",
      text: aString,
      cursor: aCursor,
      frameActor: aFrameActor,
    };
    this._client.request(packet, aOnResponse);
  },

  


  clearMessagesCache: function WCC_clearMessagesCache()
  {
    let packet = {
      to: this._actor,
      type: "clearMessagesCache",
    };
    this._client.request(packet);
  },

  







  getPreferences: function WCC_getPreferences(aPreferences, aOnResponse)
  {
    let packet = {
      to: this._actor,
      type: "getPreferences",
      preferences: aPreferences,
    };
    this._client.request(packet, aOnResponse);
  },

  







  setPreferences: function WCC_setPreferences(aPreferences, aOnResponse)
  {
    let packet = {
      to: this._actor,
      type: "setPreferences",
      preferences: aPreferences,
    };
    this._client.request(packet, aOnResponse);
  },

  







  getRequestHeaders: function WCC_getRequestHeaders(aActor, aOnResponse)
  {
    let packet = {
      to: aActor,
      type: "getRequestHeaders",
    };
    this._client.request(packet, aOnResponse);
  },

  







  getRequestCookies: function WCC_getRequestCookies(aActor, aOnResponse)
  {
    let packet = {
      to: aActor,
      type: "getRequestCookies",
    };
    this._client.request(packet, aOnResponse);
  },

  







  getRequestPostData: function WCC_getRequestPostData(aActor, aOnResponse)
  {
    let packet = {
      to: aActor,
      type: "getRequestPostData",
    };
    this._client.request(packet, aOnResponse);
  },

  







  getResponseHeaders: function WCC_getResponseHeaders(aActor, aOnResponse)
  {
    let packet = {
      to: aActor,
      type: "getResponseHeaders",
    };
    this._client.request(packet, aOnResponse);
  },

  







  getResponseCookies: function WCC_getResponseCookies(aActor, aOnResponse)
  {
    let packet = {
      to: aActor,
      type: "getResponseCookies",
    };
    this._client.request(packet, aOnResponse);
  },

  







  getResponseContent: function WCC_getResponseContent(aActor, aOnResponse)
  {
    let packet = {
      to: aActor,
      type: "getResponseContent",
    };
    this._client.request(packet, aOnResponse);
  },

  







  getEventTimings: function WCC_getEventTimings(aActor, aOnResponse)
  {
    let packet = {
      to: aActor,
      type: "getEventTimings",
    };
    this._client.request(packet, aOnResponse);
  },

  







  getSecurityInfo: function WCC_getSecurityInfo(aActor, aOnResponse)
  {
    let packet = {
      to: aActor,
      type: "getSecurityInfo",
    };
    this._client.request(packet, aOnResponse);
  },

  







  sendHTTPRequest: function WCC_sendHTTPRequest(aData, aOnResponse) {
    let packet = {
      to: this._actor,
      type: "sendHTTPRequest",
      request: aData
    };
    this._client.request(packet, aOnResponse);
  },

  









  startListeners: function WCC_startListeners(aListeners, aOnResponse)
  {
    let packet = {
      to: this._actor,
      type: "startListeners",
      listeners: aListeners,
    };
    this._client.request(packet, aOnResponse);
  },

  









  stopListeners: function WCC_stopListeners(aListeners, aOnResponse)
  {
    let packet = {
      to: this._actor,
      type: "stopListeners",
      listeners: aListeners,
    };
    this._client.request(packet, aOnResponse);
  },

  







  longString: function WCC_longString(aGrip)
  {
    if (aGrip.actor in this._longStrings) {
      return this._longStrings[aGrip.actor];
    }

    let client = new LongStringClient(this._client, aGrip);
    this._longStrings[aGrip.actor] = client;
    return client;
  },

  






  detach: function WCC_detach(aOnResponse)
  {
    this._client.removeListener("evaluationResult", this.onEvaluationResult);
    this._client.removeListener("networkEvent", this.onNetworkEvent);
    this._client.removeListener("networkEventUpdate", this.onNetworkEventUpdate);
    this.stopListeners(null, aOnResponse);
    this._longStrings = null;
    this._client = null;
    this.pendingEvaluationResults.clear();
    this.pendingEvaluationResults = null;
    this.clearNetworkRequests();
    this._networkRequests = null;
  },

  clearNetworkRequests: function () {
    this._networkRequests.clear();
  },

  










  getString: function(stringGrip) {
    
    if (typeof stringGrip != "object" || stringGrip.type != "longString") {
      return promise.resolve(stringGrip); 
    }

    
    if (stringGrip._fullText) {
      return stringGrip._fullText.promise;
    }

    let deferred = stringGrip._fullText = promise.defer();
    let { actor, initial, length } = stringGrip;
    let longStringClient = this.longString(stringGrip);

    longStringClient.substring(initial.length, length, aResponse => {
      if (aResponse.error) {
        DevToolsUtils.reportException("getString",
            aResponse.error + ": " + aResponse.message);

        deferred.reject(aResponse);
        return;
      }
      deferred.resolve(initial + aResponse.substring);
    });

    return deferred.promise;
  }
};
