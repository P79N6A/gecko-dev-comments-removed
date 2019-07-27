



var loop = loop || {};
loop.CallConnectionWebSocket = (function() {
  "use strict";

  var WEBSOCKET_REASONS = loop.shared.utils.WEBSOCKET_REASONS;

  
  var kResponseTimeout = 5000;

  











  function CallConnectionWebSocket(options) {
    this.options = options || {};

    if (!this.options.url) {
      throw new Error("No url in options");
    }
    if (!this.options.callId) {
      throw new Error("No callId in options");
    }
    if (!this.options.websocketToken) {
      throw new Error("No websocketToken in options");
    }

    this._lastServerState = "init";

    
    
    this._debugWebSocket =
      loop.shared.utils.getBoolPreference("debug.websocket");

    _.extend(this, Backbone.Events);
  }

  CallConnectionWebSocket.prototype = {
    







    promiseConnect: function() {
      var promise = new Promise(
        function(resolve, reject) {
          this.socket = new WebSocket(this.options.url);
          this.socket.onopen = this._onopen.bind(this);
          this.socket.onmessage = this._onmessage.bind(this);
          this.socket.onerror = this._onerror.bind(this);
          this.socket.onclose = this._onclose.bind(this);

          var timeout = setTimeout(function() {
            if (this.connectDetails && this.connectDetails.reject) {
              this.connectDetails.reject(WEBSOCKET_REASONS.TIMEOUT);
              this._clearConnectionFlags();
            }
          }.bind(this), kResponseTimeout);
          this.connectDetails = {
            resolve: resolve,
            reject: reject,
            timeout: timeout
          };
        }.bind(this));

      return promise;
    },

    





    close: function() {
      if (this.socket) {
        this.socket.close();
      }
    },

    _clearConnectionFlags: function() {
      clearTimeout(this.connectDetails.timeout);
      delete this.connectDetails;
    },

    







    _completeConnection: function(progressState) {
      if (this.connectDetails && this.connectDetails.resolve) {
        this.connectDetails.resolve(progressState);
        this._clearConnectionFlags();
        return;
      }

      console.error("Failed to complete connection promise - no promise available");
    },

    






    _checkConnectionFailed: function(event) {
      if (this.connectDetails && this.connectDetails.reject) {
        this.connectDetails.reject(event);
        this._clearConnectionFlags();
        return true;
      }

      return false;
    },

    


    decline: function() {
      this._send({
        messageType: "action",
        event: "terminate",
        reason: WEBSOCKET_REASONS.REJECT
      });
    },

    


    accept: function() {
      this._send({
        messageType: "action",
        event: "accept"
      });
    },

    



    mediaUp: function() {
      this._send({
        messageType: "action",
        event: "media-up"
      });
    },

    



    cancel: function() {
      this._send({
        messageType: "action",
        event: "terminate",
        reason: WEBSOCKET_REASONS.CANCEL
      });
    },

    


    mediaFail: function() {
      this._send({
        messageType: "action",
        event: "terminate",
        reason: WEBSOCKET_REASONS.MEDIA_FAIL
      });
    },

    




    _send: function(data) {
      this._log("WS Sending", data);

      this.socket.send(JSON.stringify(data));
    },

    





    get _stateIsCompleted() {
      return this._lastServerState === "terminated" ||
             this._lastServerState === "connected";
    },

    



    _onopen: function() {
      
      this._send({
        messageType: "hello",
        callId: this.options.callId,
        auth: this.options.websocketToken
      });
    },

    




    _onmessage: function(event) {
      var msgData;
      try {
        msgData = JSON.parse(event.data);
      } catch (x) {
        console.error("Error parsing received message:", x);
        return;
      }

      this._log("WS Receiving", event.data);

      var previousState = this._lastServerState;
      this._lastServerState = msgData.state;

      switch(msgData.messageType) {
        case "hello":
          this._completeConnection(msgData.state);
          break;
        case "progress":
          this.trigger("progress:" + msgData.state);
          this.trigger("progress", msgData, previousState);
          break;
      }
    },

    




    _onerror: function(event) {
      this._log("WS Error", event);

      if (!this._stateIsCompleted &&
          !this._checkConnectionFailed(event)) {
        this.trigger("error", event);
      }
    },

    




    _onclose: function(event) {
      this._log("WS Close", event);

      
      
      
      if (!this._stateIsCompleted &&
          !this._checkConnectionFailed(event)) {
        this.trigger("closed", event);
      }
    },

    




    _log: function() {
      if (this._debugWebSocket) {
        console.log.apply(console, arguments);
      }
    }
  };

  return CallConnectionWebSocket;
})();
