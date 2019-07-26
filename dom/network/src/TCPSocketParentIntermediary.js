



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function TCPSocketParentIntermediary() {
}

TCPSocketParentIntermediary.prototype = {
  open: function(aParentSide, aHost, aPort, aUseSSL, aBinaryType) {
    aParentSide.initJS(this);

    let baseSocket = Cc["@mozilla.org/tcp-socket;1"].createInstance(Ci.nsIDOMTCPSocket);
    let socket = this._socket = baseSocket.open(aHost, aPort,
                                                {useSSL: aUseSSL,
                                                binaryType: aBinaryType});
    if (!socket)
      return null;

    
    
    ["open", "drain", "data", "error", "close"].forEach(
      function(p) {
        socket["on" + p] = function(data) {
          aParentSide.sendCallback(p, data.data, socket.readyState,
                                   socket.bufferedAmount);
        };
      }
    );

    return socket;
  },

  sendString: function(aData) {
    return this._socket.send(aData);
  },

  sendArrayBuffer: function(aData) {
    return this._socket.send(aData);
  },

  classID: Components.ID("{afa42841-a6cb-4a91-912f-93099f6a3d18}"),
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsITCPSocketIntermediary
  ])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TCPSocketParentIntermediary]);
