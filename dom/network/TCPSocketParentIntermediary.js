



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let global = this;

function TCPSocketParentIntermediary() {
}

TCPSocketParentIntermediary.prototype = {
  _setCallbacks: function(aParentSide, socket) {
    aParentSide.initJS(this);
    this._socket = socket.getInternalSocket();

    
    
    
    
    ["open", "data", "error", "close"].forEach(
      function(p) {
        socket["on" + p] = function(data) {
          aParentSide.sendEvent(p, data.data, socket.readyState,
                                socket.bufferedAmount);
        };
      }
    );
  },

  _onUpdateBufferedAmountHandler: function(aParentSide, aBufferedAmount, aTrackingNumber) {
    aParentSide.sendUpdateBufferedAmount(aBufferedAmount, aTrackingNumber);
  },

  open: function(aParentSide, aHost, aPort, aUseSSL, aBinaryType,
                 aAppId, aInBrowser) {
    let socket = new global.mozTCPSocket(aHost, aPort, {useSecureTransport: aUseSSL, binaryType: aBinaryType});

    let socketInternal = socket.getInternalSocket();
    socketInternal.initWithGlobal(global);
    socketInternal.setAppId(aAppId);
    socketInternal.setInBrowser(aInBrowser);

    
    socketInternal.setOnUpdateBufferedAmountHandler(
      this._onUpdateBufferedAmountHandler.bind(this, aParentSide));

    
    this._setCallbacks(aParentSide, socket);
    return socketInternal;
  },

  listen: function(aTCPServerSocketParent, aLocalPort, aBacklog, aBinaryType,
                   aAppId, aInBrowser) {
    let serverSocket = new global.mozTCPServerSocket(aLocalPort, { binaryType: aBinaryType }, aBacklog);
    let serverSocketInternal = serverSocket.getInternalSocket();
    serverSocketInternal.initWithGlobal(global);

    let localPort = serverSocket.localPort;

    serverSocket["onconnect"] = function(event) {
      var socketParent = Cc["@mozilla.org/tcp-socket-parent;1"]
                            .createInstance(Ci.nsITCPSocketParent);
      var intermediary = new TCPSocketParentIntermediary();

      let socketInternal = event.socket.getInternalSocket();
      socketInternal.setAppId(aAppId);
      socketInternal.setInBrowser(aInBrowser);
      socketInternal.setOnUpdateBufferedAmountHandler(
        intermediary._onUpdateBufferedAmountHandler.bind(intermediary, socketParent));

      
      
      
      intermediary._setCallbacks(socketParent, event.socket);
      
      
      
      socketParent.setSocketAndIntermediary(socketInternal, intermediary);
      aTCPServerSocketParent.sendCallbackAccept(socketParent);
    };

    serverSocket["onerror"] = function(data) {
        var error = data.data;

        aTCPServerSocketParent.sendCallbackError(error.message, error.filename,
                                                 error.lineNumber, error.columnNumber);
    };

    return serverSocketInternal;
  },

  onRecvSendString: function(aData, aTrackingNumber) {
    let socketInternal = this._socket.QueryInterface(Ci.nsITCPSocketInternal);
    return socketInternal.onRecvSendFromChild(aData, 0, 0, aTrackingNumber);
  },

  onRecvSendArrayBuffer: function(aData, aTrackingNumber) {
    let socketInternal = this._socket.QueryInterface(Ci.nsITCPSocketInternal);
    return socketInternal.onRecvSendFromChild(aData, 0, aData.byteLength,
                                              aTrackingNumber);
  },

  classID: Components.ID("{afa42841-a6cb-4a91-912f-93099f6a3d18}"),
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsITCPSocketIntermediary
  ])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TCPSocketParentIntermediary]);
