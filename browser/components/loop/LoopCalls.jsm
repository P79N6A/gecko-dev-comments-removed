



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["LoopCalls"];

XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService",
                                  "resource:///modules/loop/MozLoopService.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LOOP_SESSION_TYPE",
                                  "resource:///modules/loop/MozLoopService.jsm");

 







function CallProgressSocket(progressUrl, callId, token) {
  if (!progressUrl || !callId || !token) {
    throw new Error("missing required arguments");
  }

  this._progressUrl = progressUrl;
  this._callId = callId;
  this._token = token;
}

CallProgressSocket.prototype = {
  







  connect: function(onSuccess, onError) {
    this._onSuccess = onSuccess;
    this._onError = onError ||
      (reason => {MozLoopService.logwarn("LoopCalls::callProgessSocket - ", reason);});

    if (!onSuccess) {
      this._onError("missing onSuccess argument");
      return;
    }

    if (Services.io.offline) {
      this._onError("IO offline");
      return;
    }

    let uri = Services.io.newURI(this._progressUrl, null, null);

    
    this._websocket = this._websocket ||
      Cc["@mozilla.org/network/protocol;1?name=" + uri.scheme]
        .createInstance(Ci.nsIWebSocketChannel);

    this._websocket.asyncOpen(uri, this._progressUrl, this, null);
  },

  





  onStart: function() {
    let helloMsg = {
      messageType: "hello",
      callId: this._callId,
      auth: this._token,
    };
    try { 
      this._websocket.sendMsg(JSON.stringify(helloMsg));
    }
    catch (error) {
      this._onError(error);
    }
  },

  





  onStop: function(aContext, aStatusCode) {
    if (!this._handshakeComplete) {
      this._onError("[" + aStatusCode + "]");
    }
  },

  








  onServerClose: function(aContext, aCode, aReason) {
    if (!this._handshakeComplete) {
      this._onError("[" + aCode + "]" + aReason);
    }
  },

  





  onMessageAvailable: function(aContext, aMsg) {
    let msg = {};
    try {
      msg = JSON.parse(aMsg);
    }
    catch (error) {
      MozLoopService.logerror("LoopCalls: error parsing progress message - ", error);
      return;
    }

    if (msg.messageType && msg.messageType === 'hello') {
      this._handshakeComplete = true;
      this._onSuccess();
    }
  },


  




  _send: function(aMsg) {
    if (!this._handshakeComplete) {
      MozLoopService.logwarn("LoopCalls::_send error - handshake not complete");
      return;
    }

    try {
      this._websocket.sendMsg(JSON.stringify(aMsg));
    }
    catch (error) {
      this._onError(error);
    }
  },

  



  sendBusy: function() {
    this._send({
      messageType: "action",
      event: "terminate",
      reason: "busy"
    });
  },
};








let LoopCallsInternal = {
  callsData: {
    inUse: false,
  },

  mocks: {
    webSocket: undefined,
  },

  





  onNotification: function(version, channelID) {
    if (MozLoopService.doNotDisturb) {
      return;
    }

    
    
    
    Services.prefs.setCharPref("loop.seenToS", "seen");

    
    
    
    

    if (channelID == MozLoopService.channelIDs.callsFxA && MozLoopService.userProfile) {
      this._getCalls(LOOP_SESSION_TYPE.FXA, version);
    } else {
      this._getCalls(LOOP_SESSION_TYPE.GUEST, version);
    }
  },

  










  _getCalls: function(sessionType, version) {
    return MozLoopService.hawkRequest(sessionType, "/calls?version=" + version, "GET").then(
      response => {this._processCalls(response, sessionType);}
    );
  },

  










  _processCalls: function(response, sessionType) {
    try {
      let respData = JSON.parse(response.body);
      if (respData.calls && Array.isArray(respData.calls)) {
        respData.calls.forEach((callData) => {
          if (!this.callsData.inUse) {
            callData.sessionType = sessionType;
            this._startCall(callData, "incoming");
          } else {
            this._returnBusy(callData);
          }
        });
      } else {
        MozLoopService.logwarn("Error: missing calls[] in response");
      }
    } catch (err) {
      MozLoopService.logwarn("Error parsing calls info", err);
    }
  },

  






  _startCall: function(callData, conversationType) {
    this.callsData.inUse = true;
    this.callsData.data = callData;
    MozLoopService.openChatWindow(
      null,
      
      "",
      "about:loopconversation#" + conversationType + "/" + callData.callId);
  },

  






  startDirectCall: function(contact, callType) {
    if (this.callsData.inUse)
      return false;

    var callData = {
      contact: contact,
      callType: callType,
      
      callId: Math.floor((Math.random() * 100000000))
    };

    this._startCall(callData, "outgoing");
    return true;
  },

   






  _returnBusy: function(callData) {
    let callProgress = new CallProgressSocket(
      callData.progressURL,
      callData.callId,
      callData.websocketToken);
    if (this.mocks.webSocket) {
      callProgress._websocket = this.mocks.webSocket;
    }
    
    
    callProgress.connect(() => {callProgress.sendBusy();});
  }
};
Object.freeze(LoopCallsInternal);




this.LoopCalls = {
  





  onNotification: function(version, channelID) {
    LoopCallsInternal.onNotification(version, channelID);
  },

  








  getCallData: function(loopCallId) {
    if (LoopCallsInternal.callsData.data &&
        LoopCallsInternal.callsData.data.callId == loopCallId) {
      return LoopCallsInternal.callsData.data;
    } else {
      return undefined;
    }
  },

  






  releaseCallData: function(loopCallId) {
    if (LoopCallsInternal.callsData.data &&
        LoopCallsInternal.callsData.data.callId == loopCallId) {
      LoopCallsInternal.callsData.data = undefined;
      LoopCallsInternal.callsData.inUse = false;
    }
  },

    






  startDirectCall: function(contact, callType) {
    LoopCallsInternal.startDirectCall(contact, callType);
  }
};
Object.freeze(LoopCalls);
