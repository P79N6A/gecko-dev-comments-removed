



"use strict";

const LOOPBACK_ADDR = "127.0.0.";

const iceStateTransitions = {
  "new": ["checking", "closed"], 
                                 
  "checking": ["new", "connected", "failed", "closed"], 
                                                        
                                                        
  "connected": ["new", "completed", "disconnected", "closed"],
  "completed": ["new", "disconnected", "closed"],
  "disconnected": ["new", "connected", "completed", "failed", "closed"],
  "failed": ["new", "disconnected", "closed"],
  "closed": []
  }

const signalingStateTransitions = {
  "stable": ["have-local-offer", "have-remote-offer", "closed"],
  "have-local-offer": ["have-remote-pranswer", "stable", "closed", "have-local-offer"],
  "have-remote-pranswer": ["stable", "closed", "have-remote-pranswer"],
  "have-remote-offer": ["have-local-pranswer", "stable", "closed", "have-remote-offer"],
  "have-local-pranswer": ["stable", "closed", "have-local-pranswer"],
  "closed": []
}












function CommandChain(framework, commandList) {
  this._framework = framework;

  this._commands = commandList || [ ];
  this._current = 0;

  this.onFinished = null;
}

CommandChain.prototype = {

  




  get current() {
    return this._current;
  },

  




  get finished() {
    return this._current === this._commands.length;
  },

  




  get commands() {
    return this._commands;
  },

  





  set commands(commands) {
    this._commands = commands;
  },

  


  executeNext : function () {
    var self = this;

    function _executeNext() {
      if (!self.finished) {
        var step = self._commands[self._current];
        self._current++;

        self.currentStepLabel = step[0];
        info("Run step: " + self.currentStepLabel);
        step[1](self._framework);      
      }
      else if (typeof(self.onFinished) === 'function') {
        self.onFinished();
      }
    }

    
    
    window.setTimeout(_executeNext, 0);
  },

  





  append: function (commands) {
    this._commands = this._commands.concat(commands);
  },

  






  indexOf: function (id) {
    for (var i = 0; i < this._commands.length; i++) {
      if (this._commands[i][0] === id) {
        return i;
      }
    }

    return -1;
  },

  







  insertAfter: function (id, commands) {
    var index = this.indexOf(id);

    if (index > -1) {
      var tail = this.removeAfter(id);

      this.append(commands);
      this.append(tail);
    }
  },

  







  insertBefore: function (id, commands) {
    var index = this.indexOf(id);

    if (index > -1) {
      var tail = this.removeAfter(id);
      var object = this.remove(id);

      this.append(commands);
      this.append(object);
      this.append(tail);
    }
  },

  






  remove : function (id) {
    return this._commands.splice(this.indexOf(id), 1);
  },

  






  removeAfter : function (id) {
    var index = this.indexOf(id);

    if (index > -1) {
      return this._commands.splice(index + 1);
    }

    return null;
  },

  






  removeBefore : function (id) {
    var index = this.indexOf(id);

    if (index > -1) {
      return this._commands.splice(0, index);
    }

    return null;
  },

  






  replaceAfter : function (id, commands) {
    var oldCommands = this.removeAfter(id);
    this.append(commands);

    return oldCommands;
  },

  






  replaceBefore : function (id, commands) {
    var oldCommands = this.removeBefore(id);
    this.insertBefore(id, commands);

    return oldCommands;
  },

  





  filterOut : function (id_match) {
    for (var i = this._commands.length - 1; i >= 0; i--) {
      if (id_match.test(this._commands[i][0])) {
        this._commands.splice(i, 1);
      }
    }
  }
};










function MediaElementChecker(element) {
  this.element = element;
  this.canPlayThroughFired = false;
  this.timeUpdateFired = false;
  this.timePassed = false;

  var self = this;
  var elementId = self.element.getAttribute('id');

  
  
  var canPlayThroughCallback = function() {
    info('canplaythrough fired for media element ' + elementId);
    self.canPlayThroughFired = true;
    self.element.removeEventListener('canplaythrough', canPlayThroughCallback,
                                     false);
  };

  
  
  var timeUpdateCallback = function() {
    self.timeUpdateFired = true;
    info('timeupdate fired for media element ' + elementId);

    
    
    if(element.mozSrcObject && element.mozSrcObject.currentTime > 0 &&
       element.currentTime > 0) {
      info('time passed for media element ' + elementId);
      self.timePassed = true;
      self.element.removeEventListener('timeupdate', timeUpdateCallback,
                                       false);
    }
  };

  element.addEventListener('canplaythrough', canPlayThroughCallback, false);
  element.addEventListener('timeupdate', timeUpdateCallback, false);
}

MediaElementChecker.prototype = {

  






  waitForMediaFlow : function MEC_WaitForMediaFlow(onSuccess) {
    var self = this;
    var elementId = self.element.getAttribute('id');
    info('Analyzing element: ' + elementId);

    if(self.canPlayThroughFired && self.timeUpdateFired && self.timePassed) {
      ok(true, 'Media flowing for ' + elementId);
      onSuccess();
    } else {
      setTimeout(function() {
        self.waitForMediaFlow(onSuccess);
      }, 100);
    }
  },

  



  checkForNoMediaFlow : function MEC_CheckForNoMediaFlow() {
    ok(this.element.readyState === HTMLMediaElement.HAVE_METADATA,
       'Media element has a ready state of HAVE_METADATA');
  }
};




function safeInfo(message) {
  if (typeof(info) === "function") {
    info(message);
  }
}




function removeVP8(sdp) {
  var updated_sdp = sdp.replace("a=rtpmap:120 VP8/90000\r\n","");
  updated_sdp = updated_sdp.replace("RTP/SAVPF 120 126 97\r\n","RTP/SAVPF 126 97\r\n");
  updated_sdp = updated_sdp.replace("RTP/SAVPF 120 126\r\n","RTP/SAVPF 126\r\n");
  updated_sdp = updated_sdp.replace("a=rtcp-fb:120 nack\r\n","");
  updated_sdp = updated_sdp.replace("a=rtcp-fb:120 nack pli\r\n","");
  updated_sdp = updated_sdp.replace("a=rtcp-fb:120 ccm fir\r\n","");
  return updated_sdp;
}







function isNetworkReady() {
  
  if ("nsINetworkInterfaceListService" in SpecialPowers.Ci) {
    var listService = SpecialPowers.Cc["@mozilla.org/network/interface-list-service;1"]
                        .getService(SpecialPowers.Ci.nsINetworkInterfaceListService);
    var itfList = listService.getDataInterfaceList(
          SpecialPowers.Ci.nsINetworkInterfaceListService.LIST_NOT_INCLUDE_MMS_INTERFACES |
          SpecialPowers.Ci.nsINetworkInterfaceListService.LIST_NOT_INCLUDE_SUPL_INTERFACES |
          SpecialPowers.Ci.nsINetworkInterfaceListService.LIST_NOT_INCLUDE_IMS_INTERFACES |
          SpecialPowers.Ci.nsINetworkInterfaceListService.LIST_NOT_INCLUDE_DUN_INTERFACES);
    var num = itfList.getNumberOfInterface();
    for (var i = 0; i < num; i++) {
      var ips = {};
      var prefixLengths = {};
      var length = itfList.getInterface(i).getAddresses(ips, prefixLengths);

      for (var j = 0; j < length; j++) {
        var ip = ips.value[j];
        
        if (ip.indexOf(":") < 0) {
          safeInfo("Network interface is ready with address: " + ip);
          return true;
        }
      }
    }
    
    safeInfo("Network interface is not ready, required additional network setup");
    return false;
  }
  safeInfo("Network setup is not required");
  return true;
}






function getNetworkUtils() {
  var url = SimpleTest.getTestFileURL("NetworkPreparationChromeScript.js");
  var script = SpecialPowers.loadChromeScript(url);

  var utils = {
    




    prepareNetwork: function(onSuccess) {
      script.addMessageListener('network-ready', function (message) {
        info("Network interface is ready");
        onSuccess();
      });
      info("Setting up network interface");
      script.sendAsyncMessage("prepare-network", true);
    },
    




    tearDownNetwork: function(onSuccess, onFailure) {
      if (isNetworkReady()) {
        script.addMessageListener('network-disabled', function (message) {
          info("Network interface torn down");
          script.destroy();
          onSuccess();
        });
        info("Tearing down network interface");
        script.sendAsyncMessage("network-cleanup", true);
      } else {
        info("No network to tear down");
        onFailure();
      }
    }
  };

  return utils;
}





function startNetworkAndTest(onSuccess) {
  if (!isNetworkReady()) {
    SimpleTest.waitForExplicitFinish();
    var utils = getNetworkUtils();
    
    utils.prepareNetwork(onSuccess);
  } else {
    onSuccess();
  }
}




function networkTestFinished() {
  if ("nsINetworkInterfaceListService" in SpecialPowers.Ci) {
    var utils = getNetworkUtils();
    utils.tearDownNetwork(SimpleTest.finish, SimpleTest.finish);
  } else {
    SimpleTest.finish();
  }
}




function runNetworkTest(testFunction) {
  startNetworkAndTest(function() {
    runTest(testFunction);
  });
}



















function PeerConnectionTest(options) {
  
  options = options || { };
  options.commands = options.commands || commandsPeerConnection;
  options.is_local = "is_local" in options ? options.is_local : true;
  options.is_remote = "is_remote" in options ? options.is_remote : true;

  if (typeof turnServers !== "undefined") {
    if ((!options.turn_disabled_local) && (turnServers.local)) {
      if (!options.hasOwnProperty("config_local")) {
        options.config_local = {};
      }
      if (!options.config_local.hasOwnProperty("iceServers")) {
        options.config_local.iceServers = turnServers.local.iceServers;
      }
    }
    if ((!options.turn_disabled_remote) && (turnServers.remote)) {
      if (!options.hasOwnProperty("config_remote")) {
        options.config_remote = {};
      }
      if (!options.config_remote.hasOwnProperty("iceServers")) {
        options.config_remote.iceServers = turnServers.remote.iceServers;
      }
    }
  }

  if (options.is_local)
    this.pcLocal = new PeerConnectionWrapper('pcLocal', options.config_local, options.h264);
  else
    this.pcLocal = null;

  if (options.is_remote)
    this.pcRemote = new PeerConnectionWrapper('pcRemote', options.config_remote || options.config_local, options.h264);
  else
    this.pcRemote = null;

  
  this.chain = new CommandChain(this, options.commands);
  if (!options.is_local) {
    this.chain.filterOut(/^PC_LOCAL/);
  }
  if (!options.is_remote) {
    this.chain.filterOut(/^PC_REMOTE/);
  }

  var self = this;
  this.chain.onFinished = function () {
    self.teardown();
  };
}







PeerConnectionTest.prototype.close = function PCT_close(onSuccess) {
  info("Closing peer connections");

  var self = this;
  var closeTimeout = null;
  var waitingForLocal = false;
  var waitingForRemote = false;
  var everythingClosed = false;

  function verifyClosed() {
    if ((self.waitingForLocal || self.waitingForRemote) ||
      (self.pcLocal && (self.pcLocal.signalingState !== "closed")) ||
      (self.pcRemote && (self.pcRemote.signalingState !== "closed"))) {
      info("still waiting for closure");
    }
    else if (!everythingClosed) {
      info("No closure pending");
      if (self.pcLocal) {
        is(self.pcLocal.signalingState, "closed", "pcLocal is in 'closed' state");
      }
      if (self.pcRemote) {
        is(self.pcRemote.signalingState, "closed", "pcRemote is in 'closed' state");
      }
      clearTimeout(closeTimeout);
      everythingClosed = true;
      onSuccess();
    }
  }

  function signalingstatechangeLocalClose(state) {
    info("'onsignalingstatechange' event '" + state + "' received");
    is(state, "closed", "onsignalingstatechange event is closed");
    self.waitingForLocal = false;
    verifyClosed();
  }

  function signalingstatechangeRemoteClose(state) {
    info("'onsignalingstatechange' event '" + state + "' received");
    is(state, "closed", "onsignalingstatechange event is closed");
    self.waitingForRemote = false;
    verifyClosed();
  }

  function closeEverything() {
    if ((self.pcLocal) && (self.pcLocal.signalingState !== "closed")) {
      info("Closing pcLocal");
      self.pcLocal.onsignalingstatechange = signalingstatechangeLocalClose;
      self.waitingForLocal = true;
      self.pcLocal.close();
    }
    if ((self.pcRemote) && (self.pcRemote.signalingState !== "closed")) {
      info("Closing pcRemote");
      self.pcRemote.onsignalingstatechange = signalingstatechangeRemoteClose;
      self.waitingForRemote = true;
      self.pcRemote.close();
    }
    
    setTimeout(verifyClosed, 1000);
  }

  closeTimeout = setTimeout(function() {
    var closed = ((self.pcLocal && (self.pcLocal.signalingState === "closed")) &&
      (self.pcRemote && (self.pcRemote.signalingState === "closed")));
    ok(closed, "Closing PeerConnections timed out");
    
    onSuccess();
  }, 60000);

  closeEverything();
};




PeerConnectionTest.prototype.next = function PCT_next() {
  if (this._stepTimeout) {
    clearTimeout(this._stepTimeout);
    this._stepTimeout = null;
  }
  this.chain.executeNext();
};





PeerConnectionTest.prototype.setStepTimeout = function(ms) {
  this._stepTimeout = setTimeout(function() {
    ok(false, "Step timed out: " + this.chain.currentStepLabel);
    this.next();
  }.bind(this), ms);
};










PeerConnectionTest.prototype.createAnswer =
function PCT_createAnswer(peer, onSuccess) {
  peer.createAnswer(function (answer) {
    onSuccess(answer);
  });
};










PeerConnectionTest.prototype.createOffer =
function PCT_createOffer(peer, onSuccess) {
  peer.createOffer(function (offer) {
    onSuccess(offer);
  });
};

PeerConnectionTest.prototype.setIdentityProvider =
function(peer, provider, protocol, identity) {
  peer.setIdentityProvider(provider, protocol, identity);
};












PeerConnectionTest.prototype.setLocalDescription =
function PCT_setLocalDescription(peer, desc, stateExpected, onSuccess) {
  var eventFired = false;
  var stateChanged = false;

  function check_next_test() {
    if (eventFired && stateChanged) {
      onSuccess();
    }
  }

  peer.onsignalingstatechange = function (state) {
    info(peer + ": 'onsignalingstatechange' event '" + state + "' received");
    if(stateExpected === state && eventFired == false) {
      eventFired = true;
      peer.setLocalDescStableEventDate = new Date();
      check_next_test();
    } else {
      ok(false, "This event has either already fired or there has been a " +
                "mismatch between event received " + state +
                " and event expected " + stateExpected);
    }
  };

  peer.setLocalDescription(desc, function () {
    stateChanged = true;
    peer.setLocalDescDate = new Date();
    check_next_test();
  });
};








PeerConnectionTest.prototype.setMediaConstraints =
function PCT_setMediaConstraints(constraintsLocal, constraintsRemote) {
  if (this.pcLocal)
    this.pcLocal.constraints = constraintsLocal;
  if (this.pcRemote)
    this.pcRemote.constraints = constraintsRemote;
};






PeerConnectionTest.prototype.setOfferOptions =
function PCT_setOfferOptions(options) {
  if (this.pcLocal)
    this.pcLocal.offerOptions = options;
};












PeerConnectionTest.prototype.setRemoteDescription =
function PCT_setRemoteDescription(peer, desc, stateExpected, onSuccess) {
  var eventFired = false;
  var stateChanged = false;

  function check_next_test() {
    if (eventFired && stateChanged) {
      onSuccess();
    }
  }

  peer.onsignalingstatechange = function (state) {
    info(peer + ": 'onsignalingstatechange' event '" + state + "' received");
    if(stateExpected === state && eventFired == false) {
      eventFired = true;
      peer.setRemoteDescStableEventDate = new Date();
      check_next_test();
    } else {
      ok(false, "This event has either already fired or there has been a " +
                "mismatch between event received " + state +
                " and event expected " + stateExpected);
    }
  };

  peer.setRemoteDescription(desc, function () {
    stateChanged = true;
    peer.setRemoteDescDate = new Date();
    check_next_test();
  });
};




PeerConnectionTest.prototype.run = function PCT_run() {
  this.next();
};




PeerConnectionTest.prototype.teardown = function PCT_teardown() {
  this.close(function () {
    info("Test finished");
    if (window.SimpleTest)
      networkTestFinished();
    else
      finish();
  });
};















function DataChannelTest(options) {
  options = options || { };
  options.commands = options.commands || commandsDataChannel;

  PeerConnectionTest.call(this, options);
}

DataChannelTest.prototype = Object.create(PeerConnectionTest.prototype, {
  close : {
    





    value : function DCT_close(onSuccess) {
      var self = this;
      var pendingDcClose = []
      var closeTimeout = null;

      info("DataChannelTest.close() called");

      function _closePeerConnection() {
        info("DataChannelTest closing PeerConnection");
        PeerConnectionTest.prototype.close.call(self, onSuccess);
      }

      function _closePeerConnectionCallback(index) {
        info("_closePeerConnection called with index " + index);
        var pos = pendingDcClose.indexOf(index);
        if (pos != -1) {
          pendingDcClose.splice(pos, 1);
        }
        else {
          info("_closePeerConnection index " + index + " is missing from pendingDcClose: " + pendingDcClose);
        }
        if (pendingDcClose.length === 0) {
          clearTimeout(closeTimeout);
          _closePeerConnection();
        }
      }

      var myDataChannels = null;
      if (self.pcLocal) {
        myDataChannels = self.pcLocal.dataChannels;
      }
      else if (self.pcRemote) {
        myDataChannels = self.pcRemote.dataChannels;
      }
      var length = myDataChannels.length;
      for (var i = 0; i < length; i++) {
        var dataChannel = myDataChannels[i];
        if (dataChannel.readyState !== "closed") {
          pendingDcClose.push(i);
          self.closeDataChannels(i, _closePeerConnectionCallback);
        }
      }
      if (pendingDcClose.length === 0) {
        _closePeerConnection();
      }
      else {
        closeTimeout = setTimeout(function() {
          ok(false, "Failed to properly close data channels: " +
            pendingDcClose);
          _closePeerConnection();
        }, 60000);
      }
    }
  },

  closeDataChannels : {
    







    value : function DCT_closeDataChannels(index, onSuccess) {
      info("_closeDataChannels called with index: " + index);
      var localChannel = null;
      if (this.pcLocal) {
        localChannel = this.pcLocal.dataChannels[index];
      }
      var remoteChannel = null;
      if (this.pcRemote) {
        remoteChannel = this.pcRemote.dataChannels[index];
      }

      var self = this;
      var wait = false;
      var pollingMode = false;
      var everythingClosed = false;
      var verifyInterval = null;
      var remoteCloseTimer = null;

      function _allChannelsAreClosed() {
        var ret = null;
        if (localChannel) {
          ret = (localChannel.readyState === "closed");
        }
        if (remoteChannel) {
          if (ret !== null) {
            ret = (ret && (remoteChannel.readyState === "closed"));
          }
          else {
            ret = (remoteChannel.readyState === "closed");
          }
        }
        return ret;
      }

      function verifyClosedChannels() {
        if (everythingClosed) {
          
          return;
        }
        if (_allChannelsAreClosed) {
          ok(true, "DataChannel(s) have reached 'closed' state for data channel " + index);
          if (remoteCloseTimer !== null) {
            clearTimeout(remoteCloseTimer);
          }
          if (verifyInterval !== null) {
            clearInterval(verifyInterval);
          }
          everythingClosed = true;
          onSuccess(index);
        }
        else {
          info("Still waiting for DataChannel closure");
        }
      }

      if ((localChannel) && (localChannel.readyState !== "closed")) {
        
        if (remoteChannel) {
          remoteChannel.onclose = function () {
            is(remoteChannel.readyState, "closed", "remoteChannel is in state 'closed'");
            verifyClosedChannels();
          };
        }
        else {
          pollingMode = true;
          verifyInterval = setInterval(verifyClosedChannels, 1000);
        }

        localChannel.close();
        wait = true;
      }
      if ((remoteChannel) && (remoteChannel.readyState !== "closed")) {
        if (localChannel) {
          localChannel.onclose = function () {
            is(localChannel.readyState, "closed", "localChannel is in state 'closed'");
            verifyClosedChannels();
          };

          
          
          
          
          remoteCloseTimer = setTimeout(function() {
            todo(false, "localChannel.close() did not resulted in close signal on remote side");
            remoteChannel.close();
            verifyClosedChannels();
          }, 30000);
        }
        else {
          pollingMode = true;
          verifyTimer = setInterval(verifyClosedChannels, 1000);

          remoteChannel.close();
        }

        wait = true;
      }

      if (!wait) {
        onSuccess(index);
      }
    }
  },

  createDataChannel : {
    







    value : function DCT_createDataChannel(options, onSuccess) {
      var localChannel = null;
      var remoteChannel = null;
      var self = this;

      
      function check_next_test() {
        if (localChannel && remoteChannel) {
          onSuccess(localChannel, remoteChannel);
        }
      }

      if (!options.negotiated) {
        
        this.pcRemote.registerDataChannelOpenEvents(function (channel) {
          remoteChannel = channel;
          check_next_test();
        });
      }

      
      this.pcLocal.createDataChannel(options, function (channel) {
        localChannel = channel;

        if (options.negotiated) {
          
          options.id = options.id || channel.id;  
          self.pcRemote.createDataChannel(options, function (channel) {
            remoteChannel = channel;
            check_next_test();
          });
        } else {
          check_next_test();
        }
      });
    }
  },

  send : {
    













    value : function DCT_send(data, onSuccess, options) {
      options = options || { };
      var source = options.sourceChannel ||
               this.pcLocal.dataChannels[this.pcLocal.dataChannels.length - 1];
      var target = options.targetChannel ||
               this.pcRemote.dataChannels[this.pcRemote.dataChannels.length - 1];

      
      target.onmessage = function (recv_data) {
        onSuccess(target, recv_data);
      };

      source.send(data);
    }
  },

  setLocalDescription : {
    










    value : function DCT_setLocalDescription(peer, desc, state, onSuccess) {
      PeerConnectionTest.prototype.setLocalDescription.call(this, peer,
                                                              desc, state, onSuccess);

    }
  },

  waitForInitialDataChannel : {
    







    value : function DCT_waitForInitialDataChannel(peer, onSuccess, onFailure) {
      var dcConnectionTimeout = null;
      var dcOpened = false;

      function dataChannelConnected(channel) {
        
        
        if (!dcOpened) {
          clearTimeout(dcConnectionTimeout);
          is(channel.readyState, "open", peer + " dataChannels[0] switched to state: 'open'");
          dcOpened = true;
          onSuccess();
        }
      }

      
      
      if (peer == this.pcLocal) {
        peer.dataChannels[0].onopen = dataChannelConnected;
      } else {
        peer.registerDataChannelOpenEvents(dataChannelConnected);
      }

      if (peer.dataChannels.length >= 1) {
        
        const readyState = peer.dataChannels[0].readyState;
        switch (readyState) {
          case "open": {
            is(readyState, "open", peer + " dataChannels[0] is already in state: 'open'");
            dcOpened = true;
            onSuccess();
            break;
          }
          case "connecting": {
            is(readyState, "connecting", peer + " dataChannels[0] is in state: 'connecting'");
            if (onFailure) {
              dcConnectionTimeout = setTimeout(function () {
                is(peer.dataChannels[0].readyState, "open", peer + " timed out while waiting for dataChannels[0] to open");
                onFailure();
              }, 60000);
            }
            break;
          }
          default: {
            ok(false, "dataChannels[0] is in unexpected state " + readyState);
            if (onFailure) {
              onFailure()
            }
          }
        }
      }
    }
  }

});








function DataChannelWrapper(dataChannel, peerConnectionWrapper) {
  this._channel = dataChannel;
  this._pc = peerConnectionWrapper;

  info("Creating " + this);

  



  this.onclose = unexpectedEventAndFinish(this, 'onclose');
  this.onerror = unexpectedEventAndFinish(this, 'onerror');
  this.onmessage = unexpectedEventAndFinish(this, 'onmessage');
  this.onopen = unexpectedEventAndFinish(this, 'onopen');

  var self = this;

  




  this._channel.onclose = function () {
    info(self + ": 'onclose' event fired");

    self.onclose(self);
    self.onclose = unexpectedEventAndFinish(self, 'onclose');
  };

  







  this._channel.onmessage = function (event) {
    info(self + ": 'onmessage' event fired for '" + event.data + "'");

    self.onmessage(event.data);
    self.onmessage = unexpectedEventAndFinish(self, 'onmessage');
  };

  




  this._channel.onopen = function () {
    info(self + ": 'onopen' event fired");

    self.onopen(self);
    self.onopen = unexpectedEventAndFinish(self, 'onopen');
  };
}

DataChannelWrapper.prototype = {
  




  get binaryType() {
    return this._channel.binaryType;
  },

  





  set binaryType(type) {
    this._channel.binaryType = type;
  },

  




  get label() {
    return this._channel.label;
  },

  




  get protocol() {
    return this._channel.protocol;
  },

  




  get id() {
    return this._channel.id;
  },

  




  get reliable() {
    return this._channel.reliable;
  },

  

  




  get readyState() {
    return this._channel.readyState;
  },

  


  close : function () {
    info(this + ": Closing channel");
    this._channel.close();
  },

  





  send: function DCW_send(data) {
    info(this + ": Sending data '" + data + "'");
    this._channel.send(data);
  },

  




  toString: function DCW_toString() {
    return "DataChannelWrapper (" + this._pc.label + '_' + this._channel.label + ")";
  }
};











function PeerConnectionWrapper(label, configuration, h264) {
  this.configuration = configuration;
  this.label = label;
  this.whenCreated = Date.now();

  this.constraints = [ ];
  this.offerOptions = {};
  this.streams = [ ];
  this.mediaCheckers = [ ];

  this.dataChannels = [ ];

  this.onAddStreamFired = false;
  this.addStreamCallbacks = {};

  this.h264 = typeof h264 !== "undefined" ? true : false;

  info("Creating " + this);
  this._pc = new mozRTCPeerConnection(this.configuration);

  


  var self = this;
  
  this.next_ice_state = ""; 
  
  this.ice_connection_callbacks = {};

  this._pc.oniceconnectionstatechange = function() {
    ok(self._pc.iceConnectionState !== undefined, "iceConnectionState should not be undefined");
    info(self + ": oniceconnectionstatechange fired, new state is: " + self._pc.iceConnectionState);
    Object.keys(self.ice_connection_callbacks).forEach(function(name) {
      self.ice_connection_callbacks[name]();
    });
    if (self.next_ice_state !== "") {
      is(self._pc.iceConnectionState, self.next_ice_state, "iceConnectionState changed to '" +
         self.next_ice_state + "'");
      self.next_ice_state = "";
    }
  };

  





  this._pc.onaddstream = function (event) {
    info(self + ": 'onaddstream' event fired for " + JSON.stringify(event.stream));
    
    self.onAddStreamFired = true;

    var type = '';
    if (event.stream.getAudioTracks().length > 0) {
      type = 'audio';
    }
    if (event.stream.getVideoTracks().length > 0) {
      type += 'video';
    }
    self.attachMedia(event.stream, type, 'remote');

    Object.keys(self.addStreamCallbacks).forEach(function(name) {
      info(self + " calling addStreamCallback " + name);
      self.addStreamCallbacks[name]();
    });
   };

  this.ondatachannel = unexpectedEventAndFinish(this, 'ondatachannel');

  







  this._pc.ondatachannel = function (event) {
    info(self + ": 'ondatachannel' event fired for " + event.channel.label);

    self.ondatachannel(new DataChannelWrapper(event.channel, self));
    self.ondatachannel = unexpectedEventAndFinish(self, 'ondatachannel');
  };

  this.onsignalingstatechange = unexpectedEventAndFinish(this, 'onsignalingstatechange');
  this.signalingStateCallbacks = {};

  







  this._pc.onsignalingstatechange = function (anEvent) {
    info(self + ": 'onsignalingstatechange' event fired");

    Object.keys(self.signalingStateCallbacks).forEach(function(name) {
      self.signalingStateCallbacks[name](anEvent);
    });
    
    
    self.onsignalingstatechange(anEvent);
    self.onsignalingstatechange = unexpectedEventAndFinish(self, 'onsignalingstatechange');
  };
}

PeerConnectionWrapper.prototype = {

  




  get localDescription() {
    return this._pc.localDescription;
  },

  





  set localDescription(desc) {
    this._pc.localDescription = desc;
  },

  




  get remoteDescription() {
    return this._pc.remoteDescription;
  },

  





  set remoteDescription(desc) {
    this._pc.remoteDescription = desc;
  },

  




  get signalingState() {
    return this._pc.signalingState;
  },
  




  get iceConnectionState() {
    return this._pc.iceConnectionState;
  },

  setIdentityProvider: function(provider, protocol, identity) {
      this._pc.setIdentityProvider(provider, protocol, identity);
  },

  










  attachMedia : function PCW_attachMedia(stream, type, side) {
    info("Got media stream: " + type + " (" + side + ")");
    this.streams.push(stream);

    if (side === 'local') {
      
      
      if (type == "video") {
        this._pc.addStream(stream);
      } else {
        stream.getTracks().forEach(function(track) {
          this._pc.addTrack(track, stream);
        }.bind(this));
      }
    }

    var element = createMediaElement(type, this.label + '_' + side);
    this.mediaCheckers.push(new MediaElementChecker(element));
    element.mozSrcObject = stream;
    element.play();
  },

  





  getAllUserMedia : function PCW_GetAllUserMedia(onSuccess) {
    var self = this;

    function _getAllUserMedia(constraintsList, index) {
      if (index < constraintsList.length) {
        var constraints = constraintsList[index];

        getUserMedia(constraints, function (stream) {
          var type = '';

          if (constraints.audio) {
            type = 'audio';
          }

          if (constraints.video) {
            type += 'video';
          }

          self.attachMedia(stream, type, 'local');

          _getAllUserMedia(constraintsList, index + 1);
        }, generateErrorCallback());
      } else {
        onSuccess();
      }
    }

    if (this.constraints.length === 0) {
      info("Skipping GUM: no UserMedia requested");
      onSuccess();
    }
    else {
      info("Get " + this.constraints.length + " local streams");
      _getAllUserMedia(this.constraints, 0);
    }
  },

  








  createDataChannel : function PCW_createDataChannel(options, onCreation) {
    var label = 'channel_' + this.dataChannels.length;
    info(this + ": Create data channel '" + label);

    var channel = this._pc.createDataChannel(label, options);
    var wrapper = new DataChannelWrapper(channel, this);

    if (onCreation) {
      wrapper.onopen = function () {
        onCreation(wrapper);
      };
    }

    this.dataChannels.push(wrapper);
    return wrapper;
  },

  





  createOffer : function PCW_createOffer(onSuccess) {
    var self = this;

    this._pc.createOffer(function (offer) {
      info("Got offer: " + JSON.stringify(offer));
      self._last_offer = offer;
      if (self.h264) {
        isnot(offer.sdp.search("H264/90000"), -1, "H.264 should be present in the SDP offer");
        offer.sdp = removeVP8(offer.sdp);
      }
      onSuccess(offer);
    }, generateErrorCallback(), this.offerOptions);
  },

  





  createAnswer : function PCW_createAnswer(onSuccess) {
    var self = this;

    this._pc.createAnswer(function (answer) {
      info(self + ": Got answer: " + JSON.stringify(answer));
      self._last_answer = answer;
      onSuccess(answer);
    }, generateErrorCallback());
  },

  







  setLocalDescription : function PCW_setLocalDescription(desc, onSuccess) {
    var self = this;
    this._pc.setLocalDescription(desc, function () {
      info(self + ": Successfully set the local description");
      onSuccess();
    }, generateErrorCallback());
  },

  








  setLocalDescriptionAndFail : function PCW_setLocalDescriptionAndFail(desc, onFailure) {
    var self = this;
    this._pc.setLocalDescription(desc,
      generateErrorCallback("setLocalDescription should have failed."),
      function (err) {
        info(self + ": As expected, failed to set the local description");
        onFailure(err);
    });
  },

  







  setRemoteDescription : function PCW_setRemoteDescription(desc, onSuccess) {
    var self = this;
    this._pc.setRemoteDescription(desc, function () {
      info(self + ": Successfully set remote description");
      onSuccess();
    }, generateErrorCallback());
  },

  








  setRemoteDescriptionAndFail : function PCW_setRemoteDescriptionAndFail(desc, onFailure) {
    var self = this;
    this._pc.setRemoteDescription(desc,
      generateErrorCallback("setRemoteDescription should have failed."),
      function (err) {
        info(self + ": As expected, failed to set the remote description");
        onFailure(err);
    });
  },

  



  logSignalingState: function PCW_logSignalingState() {
    var self = this;

    function _logSignalingState(state) {
      var newstate = self._pc.signalingState;
      var oldstate = self.signalingStateLog[self.signalingStateLog.length - 1]
      if (Object.keys(signalingStateTransitions).indexOf(oldstate) != -1) {
        ok(signalingStateTransitions[oldstate].indexOf(newstate) != -1, self + ": legal signaling state transition from " + oldstate + " to " + newstate);
      } else {
        ok(false, self + ": old signaling state " + oldstate + " missing in signaling transition array");
      }
      self.signalingStateLog.push(newstate);
    }

    self.signalingStateLog = [self._pc.signalingState];
    self.signalingStateCallbacks.logSignalingStatus = _logSignalingState;
  },

  







  addIceCandidate : function PCW_addIceCandidate(candidate, onSuccess) {
    var self = this;

    this._pc.addIceCandidate(candidate, function () {
      info(self + ": Successfully added an ICE candidate");
      onSuccess();
    }, generateErrorCallback());
  },

  








  addIceCandidateAndFail : function PCW_addIceCandidateAndFail(candidate, onFailure) {
    var self = this;

    this._pc.addIceCandidate(candidate,
      generateErrorCallback("addIceCandidate should have failed."),
      function (err) {
        info(self + ": As expected, failed to add an ICE candidate");
        onFailure(err);
    }) ;
  },

  




  isIceConnected : function PCW_isIceConnected() {
    info("iceConnectionState: " + this.iceConnectionState);
    return this.iceConnectionState === "connected";
  },

  




  isIceChecking : function PCW_isIceChecking() {
    return this.iceConnectionState === "checking";
  },

  




  isIceNew : function PCW_isIceNew() {
    return this.iceConnectionState === "new";
  },

  






  isIceConnectionPending : function PCW_isIceConnectionPending() {
    return (this.isIceChecking() || this.isIceNew());
  },

  



  logIceConnectionState: function PCW_logIceConnectionState() {
    var self = this;

    function logIceConState () {
      var newstate = self._pc.iceConnectionState;
      var oldstate = self.iceConnectionLog[self.iceConnectionLog.length - 1]
      if (Object.keys(iceStateTransitions).indexOf(oldstate) != -1) {
        ok(iceStateTransitions[oldstate].indexOf(newstate) != -1, self + ": legal ICE state transition from " + oldstate + " to " + newstate);
      } else {
        ok(false, self + ": old ICE state " + oldstate + " missing in ICE transition array");
      }
      self.iceConnectionLog.push(newstate);
    }

    self.iceConnectionLog = [self._pc.iceConnectionState];
    self.ice_connection_callbacks.logIceStatus = logIceConState;
  },

  










  waitForIceConnected : function PCW_waitForIceConnected(onSuccess, onFailure) {
    var self = this;
    var mySuccess = onSuccess;
    var myFailure = onFailure;

    function iceConnectedChanged () {
      if (self.isIceConnected()) {
        delete self.ice_connection_callbacks.waitForIceConnected;
        mySuccess();
      } else if (! self.isIceConnectionPending()) {
        delete self.ice_connection_callbacks.waitForIceConnected;
        myFailure();
      }
    }

    self.ice_connection_callbacks.waitForIceConnected = iceConnectedChanged;
  },

  





  countAudioTracksInMediaConstraint : function
    PCW_countAudioTracksInMediaConstraint(constraints) {
    if ((!constraints) || (constraints.length === 0)) {
      return 0;
    }
    var audioTracks = 0;
    for (var i = 0; i < constraints.length; i++) {
      if (constraints[i].audio) {
        audioTracks++;
      }
    }
    return audioTracks;
  },

  





  audioInOfferOptions : function
    PCW_audioInOfferOptions(options) {
    if (!options) {
      return 0;
    }
    if (options.offerToReceiveAudio) {
      return 1;
    } else {
      return 0;
    }
  },

  





  countVideoTracksInMediaConstraint : function
    PCW_countVideoTracksInMediaConstraint(constraints) {
    if ((!constraints) || (constraints.length === 0)) {
      return 0;
    }
    var videoTracks = 0;
    for (var i = 0; i < constraints.length; i++) {
      if (constraints[i].video) {
        videoTracks++;
      }
    }
    return videoTracks;
  },

  





  videoInOfferOptions : function
    PCW_videoInOfferOptions(options) {
    if (!options) {
      return 0;
    }
    if (options.offerToReceiveVideo) {
      return 1;
    } else {
      return 0;
    }
  },

  






  countAudioTracksInStreams : function PCW_countAudioTracksInStreams(streams) {
    if (!streams || (streams.length === 0)) {
      return 0;
    }
    var audioTracks = 0;
    streams.forEach(function(st) {
      audioTracks += st.getAudioTracks().length;
    });
    return audioTracks;
  },

  






  countVideoTracksInStreams: function PCW_countVideoTracksInStreams(streams) {
    if (!streams || (streams.length === 0)) {
      return 0;
    }
    var videoTracks = 0;
    streams.forEach(function(st) {
      videoTracks += st.getVideoTracks().length;
    });
    return videoTracks;
  },

  





  checkMediaTracks : function PCW_checkMediaTracks(constraintsRemote, onSuccess) {
    var self = this;
    var addStreamTimeout = null;

    function _checkMediaTracks(constraintsRemote, onSuccess) {
      if (addStreamTimeout !== null) {
        clearTimeout(addStreamTimeout);
      }

      var localConstraintAudioTracks =
        self.countAudioTracksInMediaConstraint(self.constraints);
      var localStreams = self._pc.getLocalStreams();
      var localAudioTracks = self.countAudioTracksInStreams(localStreams, false);
      is(localAudioTracks, localConstraintAudioTracks, self + ' has ' +
        localAudioTracks + ' local audio tracks');

      var localConstraintVideoTracks =
        self.countVideoTracksInMediaConstraint(self.constraints);
      var localVideoTracks = self.countVideoTracksInStreams(localStreams, false);
      is(localVideoTracks, localConstraintVideoTracks, self + ' has ' +
        localVideoTracks + ' local video tracks');

      var remoteConstraintAudioTracks =
        self.countAudioTracksInMediaConstraint(constraintsRemote);
      var remoteStreams = self._pc.getRemoteStreams();
      var remoteAudioTracks = self.countAudioTracksInStreams(remoteStreams, false);
      is(remoteAudioTracks, remoteConstraintAudioTracks, self + ' has ' +
        remoteAudioTracks + ' remote audio tracks');

      var remoteConstraintVideoTracks =
        self.countVideoTracksInMediaConstraint(constraintsRemote);
      var remoteVideoTracks = self.countVideoTracksInStreams(remoteStreams, false);
      is(remoteVideoTracks, remoteConstraintVideoTracks, self + ' has ' +
        remoteVideoTracks + ' remote video tracks');

      onSuccess();
    }

    
    
    var expectedRemoteTracks =
      self.countAudioTracksInMediaConstraint(constraintsRemote) +
      self.countVideoTracksInMediaConstraint(constraintsRemote);

    
    if ((self.onAddStreamFired) || (expectedRemoteTracks == 0)) {
      _checkMediaTracks(constraintsRemote, onSuccess);
    } else {
      info(self + " checkMediaTracks() got called before onAddStream fired");
      
      
      self.addStreamCallbacks.checkMediaTracks = function() {
        _checkMediaTracks(constraintsRemote, onSuccess);
      };
      addStreamTimeout = setTimeout(function () {
        ok(self.onAddStreamFired, self + " checkMediaTracks() timed out waiting for onaddstream event to fire");
        if (!self.onAddStreamFired) {
          onSuccess();
        }
      }, 60000);
    }
  },

  verifySdp : function PCW_verifySdp(desc, expectedType, constraints, offerOptions) {
    info("Examining this SessionDescription: " + JSON.stringify(desc));
    info("constraints: " + JSON.stringify(constraints));
    info("offerOptions: " + JSON.stringify(offerOptions));
    ok(desc, "SessionDescription is not null");
    is(desc.type, expectedType, "SessionDescription type is " + expectedType);
    ok(desc.sdp.length > 10, "SessionDescription body length is plausible");
    ok(desc.sdp.contains("a=ice-ufrag"), "ICE username is present in SDP");
    ok(desc.sdp.contains("a=ice-pwd"), "ICE password is present in SDP");
    ok(desc.sdp.contains("a=fingerprint"), "ICE fingerprint is present in SDP");
    
    ok(!desc.sdp.contains(LOOPBACK_ADDR), "loopback interface is absent from SDP");
    
    ok(desc.sdp.contains("a=candidate"), "at least one ICE candidate is present in SDP");
    

    
    var audioTracks = this.countAudioTracksInMediaConstraint(constraints);
    if (constraints.length === 0) {
      audioTracks = this.audioInOfferOptions(offerOptions);
    }
    info("expected audio tracks: " + audioTracks);
    if (audioTracks == 0) {
      ok(!desc.sdp.contains("m=audio"), "audio m-line is absent from SDP");
    } else {
      ok(desc.sdp.contains("m=audio"), "audio m-line is present in SDP");
      ok(desc.sdp.contains("a=rtpmap:109 opus/48000/2"), "OPUS codec is present in SDP");
      
      
      ok(desc.sdp.contains("a=rtcp-mux"), "RTCP Mux is offered in SDP");

    }

    
    var videoTracks = this.countVideoTracksInMediaConstraint(constraints);
    if (constraints.length === 0) {
      videoTracks = this.videoInOfferOptions(offerOptions);
    }
    info("expected video tracks: " + videoTracks);
    if (videoTracks == 0) {
      ok(!desc.sdp.contains("m=video"), "video m-line is absent from SDP");
    } else {
      ok(desc.sdp.contains("m=video"), "video m-line is present in SDP");
      if (this.h264) {
        ok(desc.sdp.contains("a=rtpmap:126 H264/90000"), "H.264 codec is present in SDP");
      } else {
        ok(desc.sdp.contains("a=rtpmap:120 VP8/90000"), "VP8 codec is present in SDP");
      }
      ok(desc.sdp.contains("a=rtcp-mux"), "RTCP Mux is offered in SDP");
    }

  },

  






  checkMediaFlowPresent : function PCW_checkMediaFlowPresent(onSuccess) {
    var self = this;

    function _checkMediaFlowPresent(index, onSuccess) {
      if(index >= self.mediaCheckers.length) {
        onSuccess();
      } else {
        var mediaChecker = self.mediaCheckers[index];
        mediaChecker.waitForMediaFlow(function() {
          _checkMediaFlowPresent(index + 1, onSuccess);
        });
      }
    }

    _checkMediaFlowPresent(0, onSuccess);
  },

  




  getStats : function PCW_getStats(selector, onSuccess) {
    var self = this;

    this._pc.getStats(selector, function(stats) {
      info(self + ": Got stats: " + JSON.stringify(stats));
      self._last_stats = stats;
      onSuccess(stats);
    }, generateErrorCallback());
  },

  





  checkStats : function PCW_checkStats(stats) {
    function toNum(obj) {
      return obj? obj : 0;
    }
    function numTracks(streams) {
      var n = 0;
      streams.forEach(function(stream) {
          n += stream.getAudioTracks().length + stream.getVideoTracks().length;
        });
      return n;
    }

    const isWinXP = navigator.userAgent.indexOf("Windows NT 5.1") != -1;

    
    var counters = {};
    for (var key in stats) {
      if (stats.hasOwnProperty(key)) {
        var res = stats[key];
        
        ok(res.id == key, "Coherent stats id");
        var nowish = Date.now() + 1000;        
        var minimum = this.whenCreated - 1000; 
        if (isWinXP) {
          todo(false, "Can't reliably test rtcp timestamps on WinXP (Bug 979649)");
        } else {
          ok(res.timestamp >= minimum,
             "Valid " + (res.isRemote? "rtcp" : "rtp") + " timestamp " +
                 res.timestamp + " >= " + minimum + " (" +
                 (res.timestamp - minimum) + " ms)");
          ok(res.timestamp <= nowish,
             "Valid " + (res.isRemote? "rtcp" : "rtp") + " timestamp " +
                 res.timestamp + " <= " + nowish + " (" +
                 (res.timestamp - nowish) + " ms)");
        }
        if (!res.isRemote) {
          counters[res.type] = toNum(counters[res.type]) + 1;

          switch (res.type) {
            case "inboundrtp":
            case "outboundrtp": {
              
              ok(res.ssrc.length > 0, "Ssrc has length");
              ok(res.ssrc.length < 11, "Ssrc not lengthy");
              ok(!/[^0-9]/.test(res.ssrc), "Ssrc numeric");
              ok(parseInt(res.ssrc) < Math.pow(2,32), "Ssrc within limits");

              if (res.type == "outboundrtp") {
                ok(res.packetsSent !== undefined, "Rtp packetsSent");
                
                ok(res.bytesSent >= res.packetsSent, "Rtp bytesSent");
              } else {
                ok(res.packetsReceived !== undefined, "Rtp packetsReceived");
                ok(res.bytesReceived >= res.packetsReceived, "Rtp bytesReceived");
              }
              if (res.remoteId) {
                var rem = stats[res.remoteId];
                ok(rem.isRemote, "Remote is rtcp");
                ok(rem.remoteId == res.id, "Remote backlink match");
                if(res.type == "outboundrtp") {
                  ok(rem.type == "inboundrtp", "Rtcp is inbound");
                  ok(rem.packetsReceived !== undefined, "Rtcp packetsReceived");
                  ok(rem.packetsReceived <= res.packetsSent, "No more than sent");
                  ok(rem.packetsLost !== undefined, "Rtcp packetsLost");
                  ok(rem.bytesReceived >= rem.packetsReceived, "Rtcp bytesReceived");
                  ok(rem.bytesReceived <= res.bytesSent, "No more than sent bytes");
                  ok(rem.jitter !== undefined, "Rtcp jitter");
                  ok(rem.mozRtt !== undefined, "Rtcp rtt");
                  ok(rem.mozRtt >= 0, "Rtcp rtt " + rem.mozRtt + " >= 0");
                  ok(rem.mozRtt < 60000, "Rtcp rtt " + rem.mozRtt + " < 1 min");
                } else {
                  ok(rem.type == "outboundrtp", "Rtcp is outbound");
                  ok(rem.packetsSent !== undefined, "Rtcp packetsSent");
                  
                  ok(rem.bytesSent >= rem.packetsSent, "Rtcp bytesSent");
                }
                ok(rem.ssrc == res.ssrc, "Remote ssrc match");
              } else {
                info("No rtcp info received yet");
              }
            }
            break;
          }
        }
      }
    }

    
    var counters2 = {};
    stats.forEach(function(res) {
        if (!res.isRemote) {
          counters2[res.type] = toNum(counters2[res.type]) + 1;
        }
      });
    is(JSON.stringify(counters), JSON.stringify(counters2),
       "Spec and MapClass variant of RTCStatsReport enumeration agree");
    var nin = numTracks(this._pc.getRemoteStreams());
    var nout = numTracks(this._pc.getLocalStreams());

    
    
    ok(toNum(counters.inboundrtp) >= nin, "Have at least " + nin + " inboundrtp stat(s) *");

    is(toNum(counters.outboundrtp), nout, "Have " + nout + " outboundrtp stat(s)");

    var numLocalCandidates  = toNum(counters.localcandidate);
    var numRemoteCandidates = toNum(counters.remotecandidate);
    
    if (nin + nout > 0) {
      ok(numLocalCandidates, "Have localcandidate stat(s)");
      ok(numRemoteCandidates, "Have remotecandidate stat(s)");
    } else {
      is(numLocalCandidates, 0, "Have no localcandidate stats");
      is(numRemoteCandidates, 0, "Have no remotecandidate stats");
    }
  },

  








  hasStat : function PCW_hasStat(stats, props) {
    for (var key in stats) {
      if (stats.hasOwnProperty(key)) {
        var res = stats[key];
        var match = true;
        for (var prop in props) {
          if (res[prop] !== props[prop]) {
            match = false;
            break;
          }
        }
        if (match) {
          return true;
        }
      }
    }
    return false;
  },

  


  close : function PCW_close() {
    
    
    try {
      this._pc.close();
      info(this + ": Closed connection.");
    }
    catch (e) {
      info(this + ": Failure in closing connection - " + e.message);
    }
  },

  





  registerDataChannelOpenEvents : function (onDataChannelOpened) {
    info(this + ": Register callbacks for 'ondatachannel' and 'onopen'");

    this.ondatachannel = function (targetChannel) {
      targetChannel.onopen = onDataChannelOpened;
      this.dataChannels.push(targetChannel);
    };
  },

  




  toString : function PCW_toString() {
    return "PeerConnectionWrapper (" + this.label + ")";
  }
};
