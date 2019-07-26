





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

const CONNECTION_STATUS_DISCONNECTED  = "disconnected";
const CONNECTION_STATUS_CONNECTING    = "connecting";
const CONNECTION_STATUS_CONNECTED     = "connected";
const CONNECTION_STATUS_DISCONNECTING = "disconnecting";

const DEBUG = false;

this.EXPORTED_SYMBOLS = ["WifiP2pWorkerObserver"];












this.WifiP2pWorkerObserver = function(aDomMsgResponder) {
  function debug(aMsg) {
    if (DEBUG) {
      dump('-------------- WifiP2pWorkerObserver: ' + aMsg);
    }
  }

  
  let _localDevice;
  let _peerList = {}; 
  let _domManagers = [];

  
  
  
  
  
  
  
  function P2pDevice(aPeer) {
    this.address = aPeer.address;
    this.name = (aPeer.name ? aPeer.name : aPeer.address);
    this.isGroupOwner = aPeer.isGroupOwner;
    this.wpsCapabilities = aPeer.wpsCapabilities;
    this.connectionStatus = CONNECTION_STATUS_DISCONNECTED;

    
    
    this.__exposedProps__ = {
      address: "r",
      name: "r",
      isGroupOwner: "r",
      wpsCapabilities: "r",
      connectionStatus: "r"
    };
  }

  
  
  
  
  
  
  
  
  
  function P2pGroupOwner(aGroupOwner) {
    this.macAddress = aGroupOwner.macAddress; 
    this.ipAddress = aGroupOwner.ipAddress;
    this.passphrase = aGroupOwner.passphrase;
    this.ssid = aGroupOwner.ssid; 
    this.freq = aGroupOwner.freq;
    this.isLocal = aGroupOwner.isLocal;

    let detail = _peerList[aGroupOwner.macAddress];
    if (detail) {
      this.name = detail.name;
      this.wpsCapabilities = detail.wpsCapabilities;
    } else if (_localDevice.address === this.macAddress) {
      this.name = _localDevice.name;
      this.wpsCapabilities = _localDevice.wpsCapabilities;
    } else {
      debug("We don't know this group owner: " + aGroupOwner.macAddress);
      this.name = aGroupOwner.macAddress;
      this.wpsCapabilities = [];
    }
  }

  function fireEvent(aMessage, aData) {
    debug('domManager: ' + JSON.stringify(_domManagers));
    _domManagers.forEach(function(manager) {
      
      
      manager.sendAsyncMessage("WifiP2pManager:" + aMessage, aData);
    });
  }

  function addDomManager(aMsg) {
    if (-1 === _domManagers.indexOf(aMsg.manager)) {
      _domManagers.push(aMsg.manager);
    }
  }

  function returnMessage(aMessage, aSuccess, aData, aMsg) {
    let rMsg = aMessage + ":Return:" + (aSuccess ? "OK" : "NO");
    aMsg.manager.sendAsyncMessage(rMsg,
                                 { data: aData, rid: aMsg.rid, mid: aMsg.mid });
  }

  function handlePeerListUpdated() {
    fireEvent("onpeerinfoupdate", {});
  }

  
  return {
    onLocalDeviceChanged: function(aDevice) {
      _localDevice = aDevice;
      debug('Local device updated to: ' + JSON.stringify(_localDevice));
    },

    onEnabled: function() {
      _peerList = [];
      fireEvent("p2pUp", {});
    },

    onDisbaled: function() {
      fireEvent("p2pDown", {});
    },

    onPeerFound: function(aPeer) {
      let newFoundPeer = new P2pDevice(aPeer);
      let origianlPeer = _peerList[aPeer.address];
      _peerList[aPeer.address] = newFoundPeer;
      if (origianlPeer) {
        newFoundPeer.connectionStatus = origianlPeer.connectionStatus;
      }
      handlePeerListUpdated();
    },

    onPeerLost: function(aPeer) {
      let lostPeer = _peerList[aPeer.address];
      if (!lostPeer) {
        debug('Unknown peer lost: ' + aPeer.address);
        return;
      }
      delete _peerList[aPeer.address];
      handlePeerListUpdated();
    },

    onConnecting: function(aPeer) {
      let peer = _peerList[aPeer.address];
      if (!peer) {
        debug('Unknown peer connecting: ' + aPeer.address);
        peer = new P2pDevice(aPeer);
        _peerList[aPeer.address] = peer;
        handlePeerListUpdated();
      }
      peer.connectionStatus = CONNECTION_STATUS_CONNECTING;

      fireEvent('onconnecting', { peer: peer });
    },

    onConnected: function(aGroupOwner, aPeer) {
      let go = new P2pGroupOwner(aGroupOwner);
      let peer = _peerList[aPeer.address];
      if (!peer) {
        debug('Unknown peer connected: ' + aPeer.address);
        peer = new P2pDevice(aPeer);
        _peerList[aPeer.address] = peer;
        handlePeerListUpdated();
      }
      peer.connectionStatus = CONNECTION_STATUS_CONNECTED;
      peer.isGroupOwner = (aPeer.address === aGroupOwner.address);

      fireEvent('onconnected', { groupOwner: go, peer: peer });
    },

    onDisconnected: function(aPeer) {
      let peer = _peerList[aPeer.address];
      if (!peer) {
        debug('Unknown peer disconnected: ' + aPeer.address);
        return;
      }

      peer.connectionStatus = CONNECTION_STATUS_DISCONNECTED;
      fireEvent('ondisconnected', { peer: peer });
    },

    getObservedDOMMessages: function() {
      return [
        "WifiP2pManager:getState",
        "WifiP2pManager:getPeerList",
        "WifiP2pManager:setScanEnabled",
        "WifiP2pManager:connect",
        "WifiP2pManager:disconnect",
        "WifiP2pManager:setPairingConfirmation",
        "WifiP2pManager:setDeviceName"
      ];
    },

    onDOMMessage: function(aMessage) {
      let msg = aMessage.data || {};
      msg.manager = aMessage.target;

      if ("child-process-shutdown" === aMessage.name) {
        let i;
        if (-1 !== (i = _domManagers.indexOf(msg.manager))) {
          _domManagers.splice(i, 1);
        }
        return;
      }

      if (!aMessage.target.assertPermission("wifi-manage")) {
        return;
      }

      switch (aMessage.name) {
        case "WifiP2pManager:getState": 
          addDomManager(msg);
          return { peerList: _peerList, }; 

        case "WifiP2pManager:setScanEnabled":
          {
            let enabled = msg.data;

            aDomMsgResponder.setScanEnabled(enabled, function(success) {
              returnMessage(aMessage.name, success, (success ? true : "ERROR"), msg);
            });
          }
          break;

        case "WifiP2pManager:getPeerList":
          {
            
            let peerArray = [];
            for (let key in _peerList) {
              if (_peerList.hasOwnProperty(key)) {
                peerArray.push(_peerList[key]);
              }
            }

            returnMessage(aMessage.name, true, peerArray, msg);
          }
          break;

        case "WifiP2pManager:connect":
          {
            let peer = msg.data;

            let onDoConnect = function(success) {
              returnMessage(aMessage.name, success, (success ? true : "ERROR"), msg);
            };

            aDomMsgResponder.connect(peer.address, peer.wpsMethod,
                                     peer.goIntent, onDoConnect);
          }
          break;

        case "WifiP2pManager:disconnect":
          {
            let address = msg.data;

            aDomMsgResponder.disconnect(address, function(success) {
              returnMessage(aMessage.name, success, (success ? true : "ERROR"), msg);
            });
          }
          break;

        case "WifiP2pManager:setPairingConfirmation":
          {
            let result = msg.data;
            aDomMsgResponder.setPairingConfirmation(result);
            returnMessage(aMessage.name, true, true, msg);
          }
          break;

        case "WifiP2pManager:setDeviceName":
          {
            let newDeviceName = msg.data;
            aDomMsgResponder.setDeviceName(newDeviceName, function(success) {
              returnMessage(aMessage.name, success, (success ? true : "ERROR"), msg);
            });
          }
          break;

        default:
          if (0 === aMessage.name.indexOf("WifiP2pManager:")) {
            debug("DOM WifiP2pManager message not handled: " + aMessage.name);
          }
      } 
    }
  };
};
