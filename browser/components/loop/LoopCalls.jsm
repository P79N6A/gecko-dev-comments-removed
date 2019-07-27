



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

this.EXPORTED_SYMBOLS = ["LoopCalls"];

const EMAIL_OR_PHONE_RE = /^(:?\S+@\S+|\+\d+)$/;

XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService",
                                  "resource:///modules/loop/MozLoopService.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LOOP_SESSION_TYPE",
                                  "resource:///modules/loop/MozLoopService.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoopContacts",
                                  "resource:///modules/loop/LoopContacts.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

 







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
      (reason => {MozLoopService.log.warn("LoopCalls::callProgessSocket - ", reason);});

    if (!onSuccess) {
      this._onError("missing onSuccess argument");
      return;
    }

    if (Services.io.offline) {
      this._onError("IO offline");
      return;
    }

    let uri = Services.io.newURI(this._progressUrl, null, null);

    
    if (!this._websocket) {
      this._websocket = Cc["@mozilla.org/network/protocol;1?name=" + uri.scheme]
                          .createInstance(Ci.nsIWebSocketChannel);

      this._websocket.initLoadInfo(null, 
                                   Services.scriptSecurityManager.getSystemPrincipal(),
                                   null, 
                                   Ci.nsILoadInfo.SEC_NORMAL,
                                   Ci.nsIContentPolicy.TYPE_WEBSOCKET);
    }

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
    } catch (error) {
      MozLoopService.log.error("LoopCalls: error parsing progress message - ", error);
      return;
    }

    if (msg.messageType && msg.messageType === 'hello') {
      this._handshakeComplete = true;
      this._onSuccess();
    }
  },


  




  _send: function(aMsg) {
    if (!this._handshakeComplete) {
      MozLoopService.log.warn("LoopCalls::_send error - handshake not complete");
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
  mocks: {
    webSocket: undefined,
  },

  conversationInProgress: {},

  





  onNotification: function(version, channelID) {
    if (MozLoopService.doNotDisturb) {
      return;
    }

    
    
    
    Services.prefs.setCharPref("loop.seenToS", "seen");

    
    
    
    

    if (channelID == MozLoopService.channelIDs.callsFxA && MozLoopService.userProfile) {
      this._getCalls(LOOP_SESSION_TYPE.FXA, version);
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
          if ("id" in this.conversationInProgress) {
            this._returnBusy(callData);
          } else {
            callData.sessionType = sessionType;
            callData.type = "incoming";
            this._startCall(callData);
          }
        });
      } else {
        MozLoopService.log.warn("Error: missing calls[] in response");
      }
    } catch (err) {
      MozLoopService.log.warn("Error parsing calls info", err);
    }
  },

  






  _startCall: function(callData) {
    const openChat = () => {
      let windowId = MozLoopService.openChatWindow(callData);
      if (windowId) {
        this.conversationInProgress.id = windowId;
      }
    };

    if (callData.type == "incoming" && ("callerId" in callData) &&
        EMAIL_OR_PHONE_RE.test(callData.callerId)) {
      LoopContacts.search({
        q: callData.callerId,
        field: callData.callerId.contains("@") ? "email" : "tel"
      }, (err, contacts) => {
        if (err) {
          
          openChat();
          return;
        }

        for (let contact of contacts) {
          if (contact.blocked) {
            
            this._returnBusy(callData);
            return;
          }
        }

        openChat();
      })
    } else {
      openChat();
    }
  },

  






  startDirectCall: function(contact, callType) {
    if ("id" in this.conversationInProgress)
      return false;

    var callData = {
      contact: contact,
      callType: callType,
      type: "outgoing"
    };

    this._startCall(callData);
    return true;
  },

  










  blockDirectCaller: function(callerId, callback) {
    let field = callerId.contains("@") ? "email" : "tel";
    Task.spawn(function* () {
      
      let contacts = yield LoopContacts.promise("search", {
        q: callerId,
        field: field
      });

      let contact;
      if (contacts.length) {
        for (contact of contacts) {
          yield LoopContacts.promise("block", contact._guid);
        }
      } else {
        
        contact = {
          id: MozLoopService.generateUUID(),
          name: [callerId],
          category: ["local"],
          blocked: true
        };
        
        contact[field] = [{
          pref: true,
          value: callerId
        }];

        yield LoopContacts.promise("add", contact);
      }
    }).then(callback, callback);
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

  




  setCallInProgress: function(conversationWindowId) {
    if ("id" in LoopCallsInternal.conversationInProgress &&
        LoopCallsInternal.conversationInProgress.id != conversationWindowId) {
      MozLoopService.log.error("Starting a new conversation when one is already in progress?");
      return;
    }

    LoopCallsInternal.conversationInProgress.id = conversationWindowId;
  },

  






  clearCallInProgress: function(conversationWindowId) {
    if ("id" in LoopCallsInternal.conversationInProgress &&
        LoopCallsInternal.conversationInProgress.id == conversationWindowId) {
      delete LoopCallsInternal.conversationInProgress.id;
    }
  },

    






  startDirectCall: function(contact, callType) {
    LoopCallsInternal.startDirectCall(contact, callType);
  },

  


  blockDirectCaller: function(callerId, callback) {
    return LoopCallsInternal.blockDirectCaller(callerId, callback);
  }
};
Object.freeze(LoopCalls);
