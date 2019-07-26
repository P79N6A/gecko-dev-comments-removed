



"use strict";













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
          info("Network interface is ready with address: " + ip);
          return true;
        }
      }
    }
    
    info("Network interface is not ready, required additional network setup");
    return false;
  }
  info("Network setup is not required");
  return true;
}






function getNetworkUtils() {
  var url = SimpleTest.getTestFileURL("NetworkPreparationChromeScript.js");
  var script = SpecialPowers.loadChromeScript(url);

  var utils = {
    




    prepareNetwork: function(aCallback) {
      script.addMessageListener('network-ready', function (message) {
        info("Network interface is ready");
        aCallback();
      });
      info("Setup network interface");
      script.sendAsyncMessage("prepare-network", true);
    },
    




    tearDownNetwork: function(aCallback) {
      script.addMessageListener('network-disabled', function (message) {
        ok(true, 'network-disabled');
        script.destroy();
        aCallback();
      });
      script.sendAsyncMessage("network-cleanup", true);
    }
  };

  return utils;
}



















function PeerConnectionTest(options) {
  
  options = options || { };
  options.commands = options.commands || commandsPeerConnection;
  options.is_local = "is_local" in options ? options.is_local : true;
  options.is_remote = "is_remote" in options ? options.is_remote : true;

  var netTeardownCommand = null;
  if (!isNetworkReady()) {
    var utils = getNetworkUtils();
    
    utils.prepareNetwork(function() {
      ok(isNetworkReady(),'setup network connection successfully');
    });

    netTeardownCommand = [
      [
        'TEARDOWN_NETWORK',
        function(test) {
          utils.tearDownNetwork(function() {
            info('teardown network connection');
            test.next();
          });
        }
      ]
    ];
  }

  if (options.is_local)
    this.pcLocal = new PeerConnectionWrapper('pcLocal', options.config_pc1);
  else
    this.pcLocal = null;

  if (options.is_remote)
    this.pcRemote = new PeerConnectionWrapper('pcRemote', options.config_pc2 || options.config_pc1);
  else
    this.pcRemote = null;

  this.connected = false;

  
  this.chain = new CommandChain(this, options.commands);
  if (!options.is_local) {
    this.chain.filterOut(/^PC_LOCAL/);
  }
  if (!options.is_remote) {
    this.chain.filterOut(/^PC_REMOTE/);
  }

  
  if (netTeardownCommand) {
    this.chain.append(netTeardownCommand);
  }

  var self = this;
  this.chain.onFinished = function () {
    self.teardown();
  };
}







PeerConnectionTest.prototype.close = function PCT_close(onSuccess) {
  info("Closing peer connections. Connection state=" + this.connected);

  
  
  if (this.pcLocal)
    this.pcLocal.close();
  if (this.pcRemote)
    this.pcRemote.close();
  this.connected = false;

  onSuccess();
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
function PCT_setLocalDescription(peer, desc, onSuccess) {
  var eventFired = false;
  var stateChanged = false;

  function check_next_test() {
    if (eventFired && stateChanged) {
      onSuccess();
    }
  }

  peer.onsignalingstatechange = function () {
    info(peer + ": 'onsignalingstatechange' event registered, signalingState: " + peer.signalingState);

    eventFired = true;
    check_next_test();
  };

  peer.setLocalDescription(desc, function () {
    stateChanged = true;
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






PeerConnectionTest.prototype.setOfferConstraints =
function PCT_setOfferConstraints(constraints) {
  if (this.pcLocal)
    this.pcLocal.offerConstraints = constraints;
};












PeerConnectionTest.prototype.setRemoteDescription =
function PCT_setRemoteDescription(peer, desc, onSuccess) {
  var eventFired = false;
  var stateChanged = false;

  function check_next_test() {
    if (eventFired && stateChanged) {
      onSuccess();
    }
  }

  peer.onsignalingstatechange = function () {
    info(peer + ": 'onsignalingstatechange' event registered, signalingState: " + peer.signalingState);

    eventFired = true;
    check_next_test();
  };

  peer.setRemoteDescription(desc, function () {
    stateChanged = true;
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
      SimpleTest.finish();
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

      function _closeChannels() {
        var length = self.pcLocal.dataChannels.length;

        if (length > 0) {
          self.closeDataChannel(length - 1, function () {
            _closeChannels();
          });
        }
        else {
          PeerConnectionTest.prototype.close.call(self, onSuccess);
        }
      }

      _closeChannels();
    }
  },

  closeDataChannel : {
    







    value : function DCT_closeDataChannel(index, onSuccess) {
      var localChannel = this.pcLocal.dataChannels[index];
      var remoteChannel = this.pcRemote.dataChannels[index];

      var self = this;

      
      
      remoteChannel.onclose = function () {
        self.pcRemote.dataChannels.splice(index, 1);

        onSuccess(remoteChannel);
      };

      localChannel.close();
      this.pcLocal.dataChannels.splice(index, 1);
    }
  },

  createDataChannel : {
    







    value : function DCT_createDataChannel(options, onSuccess) {
      var localChannel = null;
      var remoteChannel = null;
      var self = this;

      
      function check_next_test() {
        if (self.connected && localChannel && remoteChannel) {
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
      source = options.sourceChannel ||
               this.pcLocal.dataChannels[this.pcLocal.dataChannels.length - 1];
      target = options.targetChannel ||
               this.pcRemote.dataChannels[this.pcRemote.dataChannels.length - 1];

      
      target.onmessage = function (recv_data) {
        onSuccess(target, recv_data);
      };

      source.send(data);
    }
  },

  setLocalDescription : {
    











    value : function DCT_setLocalDescription(peer, desc, onSuccess) {
      
      
      
      if (peer.signalingState === 'have-remote-offer') {
        this.waitForInitialDataChannel(peer, desc, onSuccess);
      }
      else {
        PeerConnectionTest.prototype.setLocalDescription.call(this, peer,
                                                              desc, onSuccess);
      }

    }
  },

  waitForInitialDataChannel : {
    









    value : function DCT_waitForInitialDataChannel(peer, desc, onSuccess) {
      var self = this;

      var targetPeer = peer;
      var targetChannel = null;

      var sourcePeer = (peer == this.pcLocal) ? this.pcRemote : this.pcLocal;
      var sourceChannel = null;

      
      
      
      function check_next_test() {
        if (self.connected && sourceChannel && targetChannel) {
          onSuccess(sourceChannel, targetChannel);
        }
      }

      
      sourcePeer.dataChannels[0].onopen = function (channel) {
        sourceChannel = channel;
        check_next_test();
      };

      
      targetPeer.registerDataChannelOpenEvents(function (channel) {
        targetChannel = channel;
        check_next_test();
      });

      PeerConnectionTest.prototype.setLocalDescription.call(this, targetPeer, desc,
        function () {
          self.connected = true;
          check_next_test();
        }
      );
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











function PeerConnectionWrapper(label, configuration) {
  this.configuration = configuration;
  this.label = label;
  this.whenCreated = Date.now();

  this.constraints = [ ];
  this.offerConstraints = {};
  this.streams = [ ];
  this.mediaCheckers = [ ];

  this.dataChannels = [ ];

  info("Creating " + this);
  this._pc = new mozRTCPeerConnection(this.configuration);
  is(this._pc.iceConnectionState, "new", "iceConnectionState starts at 'new'");

  


  var self = this;
  
  this.next_ice_state = ""; 
  
  this.ice_connection_callbacks = [ ];

  this._pc.oniceconnectionstatechange = function() {
      ok(self._pc.iceConnectionState != undefined, "iceConnectionState should not be undefined");
      info(self + ": oniceconnectionstatechange fired, new state is: " + self._pc.iceConnectionState);
      if (Object.keys(self.ice_connection_callbacks).length >= 1) {
        var it = Iterator(self.ice_connection_callbacks);
        var name = "";
        var callback = "";
        for ([name, callback] in it) {
          callback();
        }
      }
      if (self.next_ice_state != "") {
        is(self._pc.iceConnectionState, self.next_ice_state, "iceConnectionState changed to '" +
           self.next_ice_state + "'");
        self.next_ice_state = "";
      }
  };
  this.ondatachannel = unexpectedEventAndFinish(this, 'ondatachannel');
  this.onsignalingstatechange = unexpectedEventAndFinish(this, 'onsignalingstatechange');

  





  this._pc.onaddstream = function (event) {
    info(self + ": 'onaddstream' event fired for " + JSON.stringify(event.stream));

    var type = '';
    if (event.stream.getAudioTracks().length > 0) {
      type = 'audio';
    }
    if (event.stream.getVideoTracks().length > 0) {
      type += 'video';
    }
    self.attachMedia(event.stream, type, 'remote');
   };

  







  this._pc.ondatachannel = function (event) {
    info(self + ": 'ondatachannel' event fired for " + event.channel.label);

    self.ondatachannel(new DataChannelWrapper(event.channel, self));
    self.ondatachannel = unexpectedEventAndFinish(self, 'ondatachannel');
  }

  







  this._pc.onsignalingstatechange = function (aEvent) {
    info(self + ": 'onsignalingstatechange' event fired");

    self.onsignalingstatechange();
    self.onsignalingstatechange = unexpectedEventAndFinish(self, 'onsignalingstatechange');
  }
}

PeerConnectionWrapper.prototype = {

  




  get localDescription() {
    return this._pc.localDescription;
  },

  





  set localDescription(desc) {
    this._pc.localDescription = desc;
  },

  




  get readyState() {
    return this._pc.readyState;
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
      this._pc.addStream(stream);
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

    info("Get " + this.constraints.length + " local streams");
    _getAllUserMedia(this.constraints, 0);
  },

  








  createDataChannel : function PCW_createDataChannel(options, onCreation) {
    var label = 'channel_' + this.dataChannels.length;
    info(this + ": Create data channel '" + label);

    var channel = this._pc.createDataChannel(label, options);
    var wrapper = new DataChannelWrapper(channel, this);

    if (onCreation) {
      wrapper.onopen = function () {
        onCreation(wrapper);
      }
    }

    this.dataChannels.push(wrapper);
    return wrapper;
  },

  





  createOffer : function PCW_createOffer(onSuccess) {
    var self = this;

    this._pc.createOffer(function (offer) {
      info("Got offer: " + JSON.stringify(offer));
      self._last_offer = offer;
      onSuccess(offer);
    }, generateErrorCallback(), this.offerConstraints);
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

  










  waitForIceConnected : function PCW_waitForIceConnected(onSuccess, onFailure) {
    var self = this;
    var mySuccess = onSuccess;
    var myFailure = onFailure;

    function iceConnectedChanged () {
      if (self.isIceConnected()) {
        delete self.ice_connection_callbacks["waitForIceConnected"];
        mySuccess();
      } else if (! self.isIceConnectionPending()) {
        delete self.ice_connection_callbacks["waitForIceConnected"];
        myFailure();
      }
    };

    self.ice_connection_callbacks["waitForIceConnected"] = (function() {iceConnectedChanged()});
  },

  





  checkMediaStreams : function PCW_checkMediaStreams(constraintsRemote) {
    is(this._pc.getLocalStreams().length, this.constraints.length,
       this + ' has ' + this.constraints.length + ' local streams');

    
    is(this._pc.getRemoteStreams().length, 1,
       this + ' has ' + 1 + ' remote streams');
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
                
                ok(res.bytesSent >= res.packetsSent * 8, "Rtp bytesSent");
              } else {
                ok(res.packetsReceived !== undefined, "Rtp packetsReceived");
                ok(res.bytesReceived >= res.packetsReceived * 8, "Rtp bytesReceived");
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
                  ok(rem.bytesReceived >= rem.packetsReceived * 8, "Rtcp bytesReceived");
                  ok(rem.bytesReceived <= res.bytesSent, "No more than sent bytes");
                  ok(rem.jitter !== undefined, "Rtcp jitter");
                  ok(rem.mozRtt !== undefined, "Rtcp rtt");
                  ok(rem.mozRtt >= 0, "Rtcp rtt " + rem.mozRtt + " >= 0");
                  ok(rem.mozRtt < 60000, "Rtcp rtt " + rem.mozRtt + " < 1 min");
                } else {
                  ok(rem.type == "outboundrtp", "Rtcp is outbound");
                  ok(rem.packetsSent !== undefined, "Rtcp packetsSent");
                  
                  ok(rem.bytesSent >= rem.packetsSent * 8, "Rtcp bytesSent");
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

    
    
    ok(toNum(counters["inboundrtp"]) >= nin, "Have at least " + nin + " inboundrtp stat(s) *");

    is(toNum(counters["outboundrtp"]), nout, "Have " + nout + " outboundrtp stat(s)");

    var numLocalCandidates  = toNum(counters["localcandidate"]);
    var numRemoteCandidates = toNum(counters["remotecandidate"]);
    
    if (nin + nout > 0) {
      ok(numLocalCandidates, "Have localcandidate stat(s)");
      ok(numRemoteCandidates, "Have remotecandidate stat(s)");
    } else {
      is(numLocalCandidates, 0, "Have no localcandidate stats");
      is(numRemoteCandidates, 0, "Have no remotecandidate stats");
    }
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
      targetChannel.onopen = function (targetChannel) {
        onDataChannelOpened(targetChannel);
      };

      this.dataChannels.push(targetChannel);
    }
  },

  




  toString : function PCW_toString() {
    return "PeerConnectionWrapper (" + this.label + ")";
  }
};
