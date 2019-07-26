





"use strict";

const {Cc, Ci, Cu} = require("chrome");

loader.lazyImporter(this, "LongStringClient", "resource://gre/modules/devtools/dbg-client.jsm");










function WebConsoleClient(aDebuggerClient, aActor)
{
  this._actor = aActor;
  this._client = aDebuggerClient;
  this._longStrings = {};
}
exports.WebConsoleClient = WebConsoleClient;

WebConsoleClient.prototype = {
  _longStrings: null,

  









  getCachedMessages: function WCC_getCachedMessages(aTypes, aOnResponse)
  {
    let packet = {
      to: this._actor,
      type: "getCachedMessages",
      messageTypes: aTypes,
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
    };
    this._client.request(packet, aOnResponse);
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

  






  close: function WCC_close(aOnResponse)
  {
    this.stopListeners(null, aOnResponse);
    this._longStrings = null;
    this._client = null;
  },
};
