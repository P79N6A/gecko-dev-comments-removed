



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function TCPSocketParentIntermediary() {
}

TCPSocketParentIntermediary.prototype = {
  _setCallbacks: function(aParentSide, socket) {
    aParentSide.initJS(this);
    this._socket = socket;

    
    
    
    
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
    let baseSocket = Cc["@mozilla.org/tcp-socket;1"].createInstance(Ci.nsIDOMTCPSocket);
    let socket = baseSocket.open(aHost, aPort, {useSecureTransport: aUseSSL, binaryType: aBinaryType});
    if (!socket)
      return null;

    let socketInternal = socket.QueryInterface(Ci.nsITCPSocketInternal);
    socketInternal.setAppId(aAppId);
    socketInternal.setInBrowser(aInBrowser);

    
    socketInternal.setOnUpdateBufferedAmountHandler(
      this._onUpdateBufferedAmountHandler.bind(this, aParentSide));

    
    this._setCallbacks(aParentSide, socket);
    return socket;
  },

  listen: function(aTCPServerSocketParent, aLocalPort, aBacklog, aBinaryType,
                   aAppId, aInBrowser) {
    let baseSocket = Cc["@mozilla.org/tcp-socket;1"].createInstance(Ci.nsIDOMTCPSocket);
    let serverSocket = baseSocket.listen(aLocalPort, { binaryType: aBinaryType }, aBacklog);
    if (!serverSocket)
      return null;

    let localPort = serverSocket.localPort;

    serverSocket["onconnect"] = function(socket) {
      var socketParent = Cc["@mozilla.org/tcp-socket-parent;1"]
                            .createInstance(Ci.nsITCPSocketParent);
      var intermediary = new TCPSocketParentIntermediary();

      let socketInternal = socket.QueryInterface(Ci.nsITCPSocketInternal);
      socketInternal.setAppId(aAppId);
      socketInternal.setInBrowser(aInBrowser);
      socketInternal.setOnUpdateBufferedAmountHandler(
        intermediary._onUpdateBufferedAmountHandler.bind(intermediary, socketParent));

      
      
      
      intermediary._setCallbacks(socketParent, socket);
      
      
      
      socketParent.setSocketAndIntermediary(socket, intermediary);
      aTCPServerSocketParent.sendCallbackAccept(socketParent);
    };

    serverSocket["onerror"] = function(data) {
        var error = data.data;

        aTCPServerSocketParent.sendCallbackError(error.message, error.filename,
                                                 error.lineNumber, error.columnNumber);
    };

    return serverSocket;
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
