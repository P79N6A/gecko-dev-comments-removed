



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

var wait = (time) => new Promise(r => setTimeout(r, time));










function MediaElementChecker(element) {
  this.element = element;
  this.canPlayThroughFired = false;
  this.timeUpdateFired = false;
  this.timePassed = false;

  var elementId = this.element.getAttribute('id');

  
  
  var canPlayThroughCallback = () => {
    info('canplaythrough fired for media element ' + elementId);
    this.canPlayThroughFired = true;
    this.element.removeEventListener('canplaythrough', canPlayThroughCallback,
                                     false);
  };

  
  
  var timeUpdateCallback = () => {
    this.timeUpdateFired = true;
    info('timeupdate fired for media element ' + elementId);

    
    
    if (element.mozSrcObject && element.mozSrcObject.currentTime > 0 &&
        element.currentTime > 0) {
      info('time passed for media element ' + elementId);
      this.timePassed = true;
      this.element.removeEventListener('timeupdate', timeUpdateCallback,
                                       false);
    }
  };

  element.addEventListener('canplaythrough', canPlayThroughCallback, false);
  element.addEventListener('timeupdate', timeUpdateCallback, false);
}

MediaElementChecker.prototype = {

  



  waitForMediaFlow: function() {
    var elementId = this.element.getAttribute('id');
    info('Analyzing element: ' + elementId);

    return waitUntil(() => this.canPlayThroughFired && this.timeUpdateFired && this.timePassed)
      .then(() => ok(true, 'Media flowing for ' + elementId));
  },

  



  checkForNoMediaFlow: function() {
    ok(this.element.readyState === HTMLMediaElement.HAVE_METADATA,
       'Media element has a ready state of HAVE_METADATA');
  }
};




function removeVP8(sdp) {
  var updated_sdp = sdp.replace("a=rtpmap:120 VP8/90000\r\n","");
  updated_sdp = updated_sdp.replace("RTP/SAVPF 120 126 97\r\n","RTP/SAVPF 126 97\r\n");
  updated_sdp = updated_sdp.replace("RTP/SAVPF 120 126\r\n","RTP/SAVPF 126\r\n");
  updated_sdp = updated_sdp.replace("a=rtcp-fb:120 nack\r\n","");
  updated_sdp = updated_sdp.replace("a=rtcp-fb:120 nack pli\r\n","");
  updated_sdp = updated_sdp.replace("a=rtcp-fb:120 ccm fir\r\n","");
  return updated_sdp;
}

var makeDefaultCommands = () => {
  return [].concat(commandsPeerConnectionInitial,
                   commandsGetUserMedia,
                   commandsPeerConnectionOfferAnswer);
};



















function PeerConnectionTest(options) {
  
  options = options || { };
  options.commands = options.commands || makeDefaultCommands();
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

  this.steeplechase = this.pcLocal === null || this.pcRemote === null;

  
  this.chain = new CommandChain(this, options.commands);
  if (!options.is_local) {
    this.chain.filterOut(/^PC_LOCAL/);
  }
  if (!options.is_remote) {
    this.chain.filterOut(/^PC_REMOTE/);
  }
}


function timerGuard(p, time, message) {
  return Promise.race([
    p,
    wait(time).then(() => {
      throw new Error('timeout after ' + (time / 1000) + 's: ' + message);
    })
  ]);
}




PeerConnectionTest.prototype.closePC = function() {
  info("Closing peer connections");

  var closeIt = pc => {
    if (!pc || pc.signalingState === "closed") {
      return Promise.resolve();
    }

    return new Promise(resolve => {
      pc.onsignalingstatechange = e => {
        is(e.target.signalingState, "closed", "signalingState is closed");
        resolve();
      };
      pc.close();
    });
  };

  return timerGuard(Promise.all([
    closeIt(this.pcLocal),
    closeIt(this.pcRemote)
  ]), 60000, "failed to close peer connection");
};




PeerConnectionTest.prototype.close = function() {
  var allChannels = (this.pcLocal || this.pcRemote).dataChannels;
  return timerGuard(
    Promise.all(allChannels.map((channel, i) => this.closeDataChannels(i))),
    60000, "failed to close data channels")
    .then(() => this.closePC());
};







PeerConnectionTest.prototype.closeDataChannels = function(index) {
  info("closeDataChannels called with index: " + index);
  var localChannel = null;
  if (this.pcLocal) {
    localChannel = this.pcLocal.dataChannels[index];
  }
  var remoteChannel = null;
  if (this.pcRemote) {
    remoteChannel = this.pcRemote.dataChannels[index];
  }

  
  var setupClosePromise = channel => {
    if (!channel) {
      return Promise.resolve();
    }
    return new Promise(resolve => {
      channel.onclose = () => {
        is(channel.readyState, "closed", name + " channel " + index + " closed");
        resolve();
      };
    });
  };

  
  var allClosed = Promise.all([
    setupClosePromise(localChannel),
    setupClosePromise(remoteChannel)
  ]);
  var complete = timerGuard(allClosed, 60000, "failed to close data channel pair");

  
  if (remoteChannel) {
    remoteChannel.close();
  } else if (localChannel) {
    localChannel.close();
  }

  return complete;
};













PeerConnectionTest.prototype.send = function(data, options) {
  options = options || { };
  var source = options.sourceChannel ||
           this.pcLocal.dataChannels[this.pcLocal.dataChannels.length - 1];
  var target = options.targetChannel ||
           this.pcRemote.dataChannels[this.pcRemote.dataChannels.length - 1];

  return new Promise(resolve => {
    
    target.onmessage = e => {
      resolve({ channel: target, data: e.data });
    };

    source.send(data);
  });
};







PeerConnectionTest.prototype.createDataChannel = function(options) {
  var remotePromise;
  if (!options.negotiated) {
    this.pcRemote.expectDataChannel();
    remotePromise = this.pcRemote.nextDataChannel;
  }

  
  var localChannel = this.pcLocal.createDataChannel(options)
  var localPromise = localChannel.opened;

  if (options.negotiated) {
    remotePromise = localPromise.then(localChannel => {
      
      options.id = options.id || channel.id;  
      var remoteChannel = this.pcRemote.createDataChannel(options);
      return remoteChannel.opened;
    });
  }

  
  
  return Promise.all([this.pcLocal.observedNegotiationNeeded,
                      this.pcRemote.observedNegotiationNeeded]).then(() => {
    return Promise.all([localPromise, remotePromise]).then(result => {
      return { local: result[0], remote: result[1] };
    });
  });
};








PeerConnectionTest.prototype.createAnswer = function(peer) {
  return peer.createAnswer().then(answer => {
    
    this.originalAnswer = new mozRTCSessionDescription(JSON.parse(JSON.stringify(answer)));
    return answer;
  });
};








PeerConnectionTest.prototype.createOffer = function(peer) {
  return peer.createOffer().then(offer => {
    
    this.originalOffer = new mozRTCSessionDescription(JSON.parse(JSON.stringify(offer)));
    return offer;
  });
};










PeerConnectionTest.prototype.setLocalDescription =
function(peer, desc, stateExpected) {
  var eventFired = new Promise(resolve => {
    peer.onsignalingstatechange = e => {
      info(peer + ": 'signalingstatechange' event received");
      var state = e.target.signalingState;
      if (stateExpected === state) {
        peer.setLocalDescStableEventDate = new Date();
        resolve();
      } else {
        ok(false, "This event has either already fired or there has been a " +
           "mismatch between event received " + state +
           " and event expected " + stateExpected);
      }
    };
  });

  var stateChanged = peer.setLocalDescription(desc).then(() => {
    peer.setLocalDescDate = new Date();
  });

  return Promise.all([eventFired, stateChanged]);
};








PeerConnectionTest.prototype.setMediaConstraints =
function(constraintsLocal, constraintsRemote) {
  if (this.pcLocal) {
    this.pcLocal.constraints = constraintsLocal;
  }
  if (this.pcRemote) {
    this.pcRemote.constraints = constraintsRemote;
  }
};






PeerConnectionTest.prototype.setOfferOptions = function(options) {
  if (this.pcLocal) {
    this.pcLocal.offerOptions = options;
  }
};










PeerConnectionTest.prototype.setRemoteDescription =
function(peer, desc, stateExpected) {
  var eventFired = new Promise(resolve => {
    peer.onsignalingstatechange = e => {
      info(peer + ": 'signalingstatechange' event received");
      var state = e.target.signalingState;
      if (stateExpected === state) {
        peer.setRemoteDescStableEventDate = new Date();
        resolve();
      } else {
        ok(false, "This event has either already fired or there has been a " +
           "mismatch between event received " + state +
           " and event expected " + stateExpected);
      }
    };
  });

  var stateChanged = peer.setRemoteDescription(desc).then(() => {
    peer.setRemoteDescDate = new Date();
  });

  return Promise.all([eventFired, stateChanged]);
};




PeerConnectionTest.prototype.run = function() {
  return this.chain.execute()
    .then(() => this.close())
    .then(() => {
      if (window.SimpleTest) {
        networkTestFinished();
      } else {
        finish();
      }
    })
    .catch(e =>
           ok(false, 'Error in test execution: ' + e +
              ((typeof e.stack === 'string') ?
               (' ' + e.stack.split('\n').join(' ... ')) : '')));
};




PeerConnectionTest.prototype.iceCandidateHandler = function(caller, candidate) {
  info("Received: " + JSON.stringify(candidate) + " from " + caller);

  var target = null;
  if (caller.contains("pcLocal")) {
    if (this.pcRemote) {
      target = this.pcRemote;
    }
  } else if (caller.contains("pcRemote")) {
    if (this.pcLocal) {
      target = this.pcLocal;
    }
  } else {
    ok(false, "received event from unknown caller: " + caller);
    return;
  }

  if (target) {
    target.storeOrAddIceCandidate(candidate);
  } else {
    info("sending ice candidate to signaling server");
    send_message({"type": "ice_candidate", "ice_candidate": candidate});
  }
};





PeerConnectionTest.prototype.setupSignalingClient = function() {
  this.signalingMessageQueue = [];
  this.signalingCallbacks = {};
  this.signalingLoopRun = true;

  var queueMessage = message => {
    info("Received signaling message: " + JSON.stringify(message));
    var fired = false;
    Object.keys(this.signalingCallbacks).forEach(name => {
      if (name === message.type) {
        info("Invoking callback for message type: " + name);
        this.signalingCallbacks[name](message);
        fired = true;
      }
    });
    if (!fired) {
      this.signalingMessageQueue.push(message);
      info("signalingMessageQueue.length: " + this.signalingMessageQueue.length);
    }
    if (this.signalingLoopRun) {
      wait_for_message().then(queueMessage);
    } else {
      info("Exiting signaling message event loop");
    }
  };
  wait_for_message().then(queueMessage);
}




PeerConnectionTest.prototype.signalingMessagesFinished = function() {
  this.signalingLoopRun = false;
}












PeerConnectionTest.prototype.registerSignalingCallback = function(messageType, onMessage) {
  this.signalingCallbacks[messageType] = onMessage;
};









PeerConnectionTest.prototype.getSignalingMessage = function(messageType) {
    var i = this.signalingMessageQueue.findIndex(m => m.type === messageType);
  if (i >= 0) {
    info("invoking callback on message " + i + " from message queue, for message type:" + messageType);
    return Promise.resolve(this.signalingMessageQueue.splice(i, 1)[0]);
  }
  return new Promise(resolve =>
                     this.registerSignalingCallback(messageType, resolve));
};









function DataChannelWrapper(dataChannel, peerConnectionWrapper) {
  this._channel = dataChannel;
  this._pc = peerConnectionWrapper;

  info("Creating " + this);

  


  createOneShotEventWrapper(this, this._channel, 'close');
  createOneShotEventWrapper(this, this._channel, 'error');
  createOneShotEventWrapper(this, this._channel, 'message');

  this.opened = timerGuard(new Promise(resolve => {
    this._channel.onopen = () => {
      this._channel.onopen = unexpectedEvent(this, 'onopen');
      is(this.readyState, "open", "data channel is 'open' after 'onopen'");
      resolve(this);
    };
  }), 60000, "channel didn't open in time");
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

  





  send: function(data) {
    info(this + ": Sending data '" + data + "'");
    this._channel.send(data);
  },

  




  toString: function() {
    return "DataChannelWrapper (" + this._pc.label + '_' + this._channel.label + ")";
  }
};










function AudioStreamAnalyser(stream) {
  if (stream.getAudioTracks().length === 0) {
    throw new Error("No audio track in stream");
  }
  this.stream = stream;
  this.audioContext = new AudioContext();
  this.sourceNode = this.audioContext.createMediaStreamSource(this.stream);
  this.analyser = this.audioContext.createAnalyser();
  this.sourceNode.connect(this.analyser);
  this.data = new Uint8Array(this.analyser.frequencyBinCount);
}

AudioStreamAnalyser.prototype = {
  




  getByteFrequencyData: function() {
    this.analyser.getByteFrequencyData(this.data);
    return this.data;
  }
};











function PeerConnectionWrapper(label, configuration, h264) {
  this.configuration = configuration;
  if (configuration && configuration.label_suffix) {
    label = label + "_" + configuration.label_suffix;
  }
  this.label = label;
  this.whenCreated = Date.now();

  this.constraints = [ ];
  this.offerOptions = {};
  this.streams = [ ];
  this.mediaCheckers = [ ];

  this.dataChannels = [ ];

  this._local_ice_candidates = [];
  this._remote_ice_candidates = [];
  this.holdIceCandidates = new Promise(r => this.releaseIceCandidates = r);
  this.localRequiresTrickleIce = false;
  this.remoteRequiresTrickleIce = false;
  this.localMediaElements = [];

  this.expectedLocalTrackInfoById = {};
  this.expectedRemoteTrackInfoById = {};
  this.observedRemoteTrackInfoById = {};

  this.disableRtpCountChecking = false;

  this.iceCheckingRestartExpected = false;

  this.h264 = typeof h264 !== "undefined" ? true : false;

  info("Creating " + this);
  this._pc = new mozRTCPeerConnection(this.configuration);

  


  
  this.ice_connection_callbacks = {};

  this._pc.oniceconnectionstatechange = e => {
    isnot(typeof this._pc.iceConnectionState, "undefined",
          "iceConnectionState should not be undefined");
    info(this + ": oniceconnectionstatechange fired, new state is: " + this._pc.iceConnectionState);
    Object.keys(this.ice_connection_callbacks).forEach(name => {
      this.ice_connection_callbacks[name]();
    });
  };

  createOneShotEventWrapper(this, this._pc, 'datachannel');
  this._pc.addEventListener('datachannel', e => {
    var wrapper = new DataChannelWrapper(e.channel, this);
    this.dataChannels.push(wrapper);
  });

  createOneShotEventWrapper(this, this._pc, 'signalingstatechange');
  createOneShotEventWrapper(this, this._pc, 'negotiationneeded');
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

  










  attachMedia : function(stream, type, side) {
    info("Got media stream: " + type + " (" + side + ")");
    this.streams.push(stream);

    if (side === 'local') {
      this.expectNegotiationNeeded();
      
      
      if (type == "video") {
        this._pc.addStream(stream);
        ok(this._pc.getSenders().find(sender => sender.track == stream.getVideoTracks()[0]),
           "addStream adds sender");
      } else {
        stream.getTracks().forEach(track => {
          var sender = this._pc.addTrack(track, stream);
          is(sender.track, track, "addTrack returns sender");
        });
      }

      stream.getTracks().forEach(track => {
        ok(track.id, "track has id");
        ok(track.kind, "track has kind");
        this.expectedLocalTrackInfoById[track.id] = {
            type: track.kind,
            streamId: stream.id
          };
      });
    }

    var element = createMediaElement(type, this.label + '_' + side + this.streams.length);
    this.mediaCheckers.push(new MediaElementChecker(element));
    element.mozSrcObject = stream;
    element.play();

    
    
    if (side === 'local') {
      this.localMediaElements.push(element);
      return this.observedNegotiationNeeded;
    }
  },

  removeSender : function(index) {
    var sender = this._pc.getSenders()[index];
    delete this.expectedLocalTrackInfoById[sender.track.id];
    this.expectNegotiationNeeded();
    this._pc.removeTrack(sender);
    return this.observedNegotiationNeeded;
  },

  senderReplaceTrack : function(index, withTrack, withStreamId) {
    var sender = this._pc.getSenders()[index];
    delete this.expectedLocalTrackInfoById[sender.track.id];
    this.expectedLocalTrackInfoById[withTrack.id] = {
        type: withTrack.kind,
        streamId: withStreamId
      };
    return sender.replaceTrack(withTrack);
  },

  





  getAllUserMedia : function(constraintsList) {
    if (constraintsList.length === 0) {
      info("Skipping GUM: no UserMedia requested");
      return Promise.resolve();
    }

    info("Get " + constraintsList.length + " local streams");
    return Promise.all(constraintsList.map(constraints => {
      return getUserMedia(constraints).then(stream => {
        var type = '';
        if (constraints.audio) {
          type = 'audio';
          stream.getAudioTracks().map(track => {
            info(this + " gUM local stream " + stream.id +
              " with audio track " + track.id);
          });
        }
        if (constraints.video) {
          type += 'video';
          stream.getVideoTracks().map(track => {
            info(this + " gUM local stream " + stream.id +
              " with video track " + track.id);
          });
        }
        return this.attachMedia(stream, type, 'local');
      });
    }));
  },

  



  expectDataChannel: function(message) {
    this.nextDataChannel = new Promise(resolve => {
      this.ondatachannel = e => {
        ok(e.channel, message);
        resolve(e.channel);
      };
    });
  },

  






  createDataChannel : function(options) {
    var label = 'channel_' + this.dataChannels.length;
    info(this + ": Create data channel '" + label);

    if (!this.dataChannels.length) {
      this.expectNegotiationNeeded();
    }
    var channel = this._pc.createDataChannel(label, options);
    var wrapper = new DataChannelWrapper(channel, this);
    this.dataChannels.push(wrapper);
    return wrapper;
  },

  


  createOffer : function() {
    return this._pc.createOffer(this.offerOptions).then(offer => {
      info("Got offer: " + JSON.stringify(offer));
      
      this._latest_offer = offer;
      if (this.h264) {
        isnot(offer.sdp.search("H264/90000"), -1, "H.264 should be present in the SDP offer");
        offer.sdp = removeVP8(offer.sdp);
      }
      return offer;
    });
  },

  


  createAnswer : function() {
    return this._pc.createAnswer().then(answer => {
      info(this + ": Got answer: " + JSON.stringify(answer));
      this._last_answer = answer;
      return answer;
    });
  },

  





  setLocalDescription : function(desc) {
    this.observedNegotiationNeeded = undefined;
    return this._pc.setLocalDescription(desc).then(() => {
      info(this + ": Successfully set the local description");
    });
  },

  








  setLocalDescriptionAndFail : function(desc) {
    return this._pc.setLocalDescription(desc).then(
      generateErrorCallback("setLocalDescription should have failed."),
      err => {
        info(this + ": As expected, failed to set the local description");
        return err;
      });
  },

  





  setRemoteDescription : function(desc) {
    this.observedNegotiationNeeded = undefined;
    return this._pc.setRemoteDescription(desc).then(() => {
      info(this + ": Successfully set remote description");
      this.releaseIceCandidates();
    });
  },

  








  setRemoteDescriptionAndFail : function(desc) {
    return this._pc.setRemoteDescription(desc).then(
      generateErrorCallback("setRemoteDescription should have failed."),
      err => {
        info(this + ": As expected, failed to set the remote description");
        return err;
    });
  },

  



  logSignalingState: function() {
    this.signalingStateLog = [this._pc.signalingState];
    this._pc.addEventListener('signalingstatechange', e => {
      var newstate = this._pc.signalingState;
      var oldstate = this.signalingStateLog[this.signalingStateLog.length - 1]
      if (Object.keys(signalingStateTransitions).indexOf(oldstate) >= 0) {
        ok(signalingStateTransitions[oldstate].indexOf(newstate) >= 0, this + ": legal signaling state transition from " + oldstate + " to " + newstate);
      } else {
        ok(false, this + ": old signaling state " + oldstate + " missing in signaling transition array");
      }
      this.signalingStateLog.push(newstate);
    });
  },

  




  checkTrackIsExpected : function(track,
                                  expectedTrackInfoById,
                                  observedTrackInfoById) {
    ok(expectedTrackInfoById[track.id], "track id " + track.id + " was expected");
    ok(!observedTrackInfoById[track.id], "track id " + track.id + " was not yet observed");
    var observedKind = track.kind;
    var expectedKind = expectedTrackInfoById[track.id].type;
    is(observedKind, expectedKind,
        "track id " + track.id + " was of kind " +
        observedKind + ", which matches " + expectedKind);
    observedTrackInfoById[track.id] = expectedTrackInfoById[track.id];
  },

  allExpectedTracksAreObserved: function(expected, observed) {
    return Object.keys(expected).every(trackId => observed[trackId]);
  },

  setupAddStreamEventHandler: function() {
    var resolveAllAddStreamEventsDone;

    
    this.allAddStreamEventsDonePromise =
      new Promise(resolve => resolveAllAddStreamEventsDone = resolve);

    this._pc.addEventListener('addstream', event => {
      info(this + ": 'onaddstream' event fired for " + JSON.stringify(event.stream));

      
      

      event.stream.getTracks().forEach(track => {
        this.checkTrackIsExpected(track,
                                  this.expectedRemoteTrackInfoById,
                                  this.observedRemoteTrackInfoById);
      });

      if (this.allExpectedTracksAreObserved(this.expectedRemoteTrackInfoById,
                                            this.observedRemoteTrackInfoById)) {
        resolveAllAddStreamEventsDone();
      }

      var type = '';
      if (event.stream.getAudioTracks().length > 0) {
        type = 'audio';
        event.stream.getAudioTracks().map(track => {
          info(this + " remote stream " + event.stream.id + " with audio track " +
               track.id);
        });
      }
      if (event.stream.getVideoTracks().length > 0) {
        type += 'video';
        event.stream.getVideoTracks().map(track => {
          info(this + " remote stream " + event.stream.id + " with video track " +
            track.id);
        });
      }
      this.attachMedia(event.stream, type, 'remote');
    });
  },

  






  storeOrAddIceCandidate : function(candidate) {
    this._remote_ice_candidates.push(candidate);
    if (this.signalingState === 'closed') {
      info("Received ICE candidate for closed PeerConnection - discarding");
      return;
    }
    this.holdIceCandidates.then(() => {
      this.addIceCandidate(candidate);
    });
  },

  





  addIceCandidate : function(candidate) {
    info(this + ": adding ICE candidate " + JSON.stringify(candidate));
    return this._pc.addIceCandidate(candidate).then(() => {
      info(this + ": Successfully added an ICE candidate");
    });
  },

  




  isIceConnected : function() {
    info(this + ": iceConnectionState = " + this.iceConnectionState);
    return this.iceConnectionState === "connected";
  },

  




  isIceChecking : function() {
    return this.iceConnectionState === "checking";
  },

  




  isIceNew : function() {
    return this.iceConnectionState === "new";
  },

  






  isIceConnectionPending : function() {
    return (this.isIceChecking() || this.isIceNew());
  },

  



  logIceConnectionState: function() {
    this.iceConnectionLog = [this._pc.iceConnectionState];
    this.ice_connection_callbacks.logIceStatus = () => {
      var newstate = this._pc.iceConnectionState;
      var oldstate = this.iceConnectionLog[this.iceConnectionLog.length - 1]
      if (Object.keys(iceStateTransitions).indexOf(oldstate) != -1) {
        if (this.iceCheckingRestartExpected) {
          is(newstate, "checking",
             "iceconnectionstate event \'" + newstate +
             "\' matches expected state \'checking\'");
          this.iceCheckingRestartExpected = false;
        } else {
          ok(iceStateTransitions[oldstate].indexOf(newstate) != -1, this + ": legal ICE state transition from " + oldstate + " to " + newstate);
        }
      } else {
        ok(false, this + ": old ICE state " + oldstate + " missing in ICE transition array");
      }
      this.iceConnectionLog.push(newstate);
    };
  },

  







  waitForIceConnected : function() {
    return new Promise((resolve, reject) => {
      var iceConnectedChanged = () => {
        if (this.isIceConnected()) {
          delete this.ice_connection_callbacks.waitForIceConnected;
          resolve();
        } else if (! this.isIceConnectionPending()) {
          delete this.ice_connection_callbacks.waitForIceConnected;
          resolve();
        }
      }

      this.ice_connection_callbacks.waitForIceConnected = iceConnectedChanged;
    });
  },

  






  setupIceCandidateHandler : function(test, candidateHandler) {
    candidateHandler = candidateHandler || test.iceCandidateHandler.bind(test);

    var resolveEndOfTrickle;
    this.endOfTrickleIce = new Promise(r => resolveEndOfTrickle = r);

    this.endOfTrickleIce.then(() => {
      this._pc.onicecandidate = () =>
        ok(false, this.label + " received ICE candidate after end of trickle");
    });

    this._pc.onicecandidate = anEvent => {
      if (!anEvent.candidate) {
        info(this.label + ": received end of trickle ICE event");
        resolveEndOfTrickle(this.label);
        return;
      }

      info(this.label + ": iceCandidate = " + JSON.stringify(anEvent.candidate));
      ok(anEvent.candidate.candidate.length > 0, "ICE candidate contains candidate");
      
      ok(anEvent.candidate.sdpMid.length === 0, "SDP MID has length zero");
      ok(typeof anEvent.candidate.sdpMLineIndex === 'number', "SDP MLine Index needs to exist");
      this._local_ice_candidates.push(anEvent.candidate);
      candidateHandler(this.label, anEvent.candidate);
    };
  },

  





  countTracksInConstraint : function(type, constraints) {
    if (!Array.isArray(constraints)) {
      return 0;
    }
    return constraints.reduce((sum, c) => sum + (c[type] ? 1 : 0), 0);
  },

  





  audioInOfferOptions : function(options) {
    if (!options) {
      return 0;
    }

    var offerToReceiveAudio = options.offerToReceiveAudio;

    
    if (options.mandatory && options.mandatory.OfferToReceiveAudio !== undefined) {
      offerToReceiveAudio = options.mandatory.OfferToReceiveAudio;
    } else if (options.optional && options.optional[0] &&
               options.optional[0].OfferToReceiveAudio !== undefined) {
      offerToReceiveAudio = options.optional[0].OfferToReceiveAudio;
    }

    if (offerToReceiveAudio) {
      return 1;
    } else {
      return 0;
    }
  },

  





  videoInOfferOptions : function(options) {
    if (!options) {
      return 0;
    }

    var offerToReceiveVideo = options.offerToReceiveVideo;

    
    if (options.mandatory && options.mandatory.OfferToReceiveVideo !== undefined) {
      offerToReceiveVideo = options.mandatory.OfferToReceiveVideo;
    } else if (options.optional && options.optional[0] &&
               options.optional[0].OfferToReceiveVideo !== undefined) {
      offerToReceiveVideo = options.optional[0].OfferToReceiveVideo;
    }

    if (offerToReceiveVideo) {
      return 1;
    } else {
      return 0;
    }
  },

  checkLocalMediaTracks : function() {
    var observed = {};
    info(this + " Checking local tracks " + JSON.stringify(this.expectedLocalTrackInfoById));
    this._pc.getSenders().forEach(sender => {
      this.checkTrackIsExpected(sender.track, this.expectedLocalTrackInfoById, observed);
    });

    Object.keys(this.expectedLocalTrackInfoById).forEach(
        id => ok(observed[id], this + " local id " + id + " was observed"));
  },

  





  checkMediaTracks : function() {
    this.checkLocalMediaTracks();

    info(this + " Checking remote tracks " +
         JSON.stringify(this.expectedRemoteTrackInfoById));

    
    if (this.allExpectedTracksAreObserved(this.expectedRemoteTrackInfoById,
                                          this.observedRemoteTrackInfoById)) {
      return;
    }

    return timerGuard(this.allAddStreamEventsDonePromise, 60000, "onaddstream never fired");
  },

  checkMsids: function() {
    var checkSdpForMsids = (desc, expectedTrackInfo, side) => {
      Object.keys(expectedTrackInfo).forEach(trackId => {
        var streamId = expectedTrackInfo[trackId].streamId;
        ok(desc.sdp.match(new RegExp("a=msid:" + streamId + " " + trackId)),
           this + ": " + side + " SDP contains stream " + streamId +
           " and track " + trackId );
      });
    };

    checkSdpForMsids(this.localDescription, this.expectedLocalTrackInfoById,
                     "local");
    checkSdpForMsids(this.remoteDescription, this.expectedRemoteTrackInfoById,
                     "remote");
  },

  verifySdp: function(desc, expectedType, offerConstraintsList, offerOptions, isLocal) {
    info("Examining this SessionDescription: " + JSON.stringify(desc));
    info("offerConstraintsList: " + JSON.stringify(offerConstraintsList));
    info("offerOptions: " + JSON.stringify(offerOptions));
    ok(desc, "SessionDescription is not null");
    is(desc.type, expectedType, "SessionDescription type is " + expectedType);
    ok(desc.sdp.length > 10, "SessionDescription body length is plausible");
    ok(desc.sdp.contains("a=ice-ufrag"), "ICE username is present in SDP");
    ok(desc.sdp.contains("a=ice-pwd"), "ICE password is present in SDP");
    ok(desc.sdp.contains("a=fingerprint"), "ICE fingerprint is present in SDP");
    
    ok(!desc.sdp.contains(LOOPBACK_ADDR), "loopback interface is absent from SDP");
    var requiresTrickleIce = !desc.sdp.contains("a=candidate");
    if (requiresTrickleIce) {
      info("at least one ICE candidate is present in SDP");
    } else {
      info("No ICE candidate in SDP -> requiring trickle ICE");
    }
    if (isLocal) {
      this.localRequiresTrickleIce = requiresTrickleIce;
    } else {
      this.remoteRequiresTrickleIce = requiresTrickleIce;
    }

    

    var audioTracks =
        this.countTracksInConstraint('audio', offerConstraintsList) ||
      this.audioInOfferOptions(offerOptions);

    info("expected audio tracks: " + audioTracks);
    if (audioTracks == 0) {
      ok(!desc.sdp.contains("m=audio"), "audio m-line is absent from SDP");
    } else {
      ok(desc.sdp.contains("m=audio"), "audio m-line is present in SDP");
      ok(desc.sdp.contains("a=rtpmap:109 opus/48000/2"), "OPUS codec is present in SDP");
      
      
      ok(desc.sdp.contains("a=rtcp-mux"), "RTCP Mux is offered in SDP");
    }

    var videoTracks =
        this.countTracksInConstraint('video', offerConstraintsList) ||
      this.videoInOfferOptions(offerOptions);

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

  



  checkMediaFlowPresent : function() {
    return Promise.all(this.mediaCheckers.map(checker => checker.waitForMediaFlow()));
  },

  










  checkReceivingToneFrom : function(from) {
    var inputElem = from.localMediaElements[0];

    
    var inputSenderTracks = from._pc.getSenders().map(sn => sn.track);
    var inputAudioStream = from._pc.getLocalStreams()
      .find(s => s.getAudioTracks().some(t => inputSenderTracks.includes(t)));
    var inputAnalyser = new AudioStreamAnalyser(inputAudioStream);

    
    
    var outputAudioStream = this._pc.getRemoteStreams()
      .find(s => s.getAudioTracks().length > 0);
    var outputAnalyser = new AudioStreamAnalyser(outputAudioStream);

    var maxWithIndex = (a, b, i) => (b >= a.value) ? { value: b, index: i } : a;
    var initial = { value: -1, index: -1 };

    return new Promise((resolve, reject) => inputElem.ontimeupdate = () => {
      var inputData = inputAnalyser.getByteFrequencyData();
      var outputData = outputAnalyser.getByteFrequencyData();

      var inputMax = inputData.reduce(maxWithIndex, initial);
      var outputMax = outputData.reduce(maxWithIndex, initial);
      info("Comparing maxima; input[" + inputMax.index + "] = " + inputMax.value +
           ", output[" + outputMax.index + "] = " + outputMax.value);
      if (!inputMax.value || !outputMax.value) {
        return;
      }

      
      
      
      if (Math.abs(inputMax.index - outputMax.index) < 10) {
        ok(true, "input and output audio data matches");
        inputElem.ontimeupdate = null;
        resolve();
      }
    });
  },

  


  getStats : function(selector) {
    return this._pc.getStats(selector).then(stats => {
      info(this + ": Got stats: " + JSON.stringify(stats));
      this._last_stats = stats;
      return stats;
    });
  },

  





  checkStats : function(stats, twoMachines) {
    var toNum = obj => obj? obj : 0;

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
        } else if (!twoMachines) {
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
                  ok(rem.packetsLost !== undefined, "Rtcp packetsLost");
                  ok(rem.bytesReceived >= rem.packetsReceived, "Rtcp bytesReceived");
                  if (!this.disableRtpCountChecking) {
                    ok(rem.packetsReceived <= res.packetsSent, "No more than sent packets");
                    ok(rem.bytesReceived <= res.bytesSent, "No more than sent bytes");
                  }
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
    stats.forEach(res => {
        if (!res.isRemote) {
          counters2[res.type] = toNum(counters2[res.type]) + 1;
        }
      });
    is(JSON.stringify(counters), JSON.stringify(counters2),
       "Spec and MapClass variant of RTCStatsReport enumeration agree");
    var nin = Object.keys(this.expectedRemoteTrackInfoById).length;
    var nout = Object.keys(this.expectedLocalTrackInfoById).length;
    var ndata = this.dataChannels.length;

    
    
    ok(toNum(counters.inboundrtp) >= nin, "Have at least " + nin + " inboundrtp stat(s) *");

    is(toNum(counters.outboundrtp), nout, "Have " + nout + " outboundrtp stat(s)");

    var numLocalCandidates  = toNum(counters.localcandidate);
    var numRemoteCandidates = toNum(counters.remotecandidate);
    
    if (nin + nout + ndata > 0) {
      ok(numLocalCandidates, "Have localcandidate stat(s)");
      ok(numRemoteCandidates, "Have remotecandidate stat(s)");
    } else {
      is(numLocalCandidates, 0, "Have no localcandidate stats");
      is(numRemoteCandidates, 0, "Have no remotecandidate stats");
    }
  },

  






  checkStatsIceConnectionType : function(stats) {
    var lId;
    var rId;
    Object.keys(stats).forEach(name => {
      if ((stats[name].type === "candidatepair") &&
          (stats[name].selected)) {
        lId = stats[name].localCandidateId;
        rId = stats[name].remoteCandidateId;
      }
    });
    info("checkStatsIceConnectionType verifying: local=" +
         JSON.stringify(stats[lId]) + " remote=" + JSON.stringify(stats[rId]));
    if ((typeof stats[lId] === 'undefined') ||
        (typeof stats[rId] === 'undefined')) {
      info("checkStatsIceConnectionType failed to find candidatepair IDs");
      return;
    }
    var lType = stats[lId].candidateType;
    var rType = stats[rId].candidateType;
    var lIp = stats[lId].ipAddress;
    var rIp = stats[rId].ipAddress;
    if ((this.configuration) && (typeof this.configuration.iceServers !== 'undefined')) {
      info("Ice Server configured");
      
      
      var serverIp = this.configuration.iceServers[0].url.split(':')[1];
      ok((lType === "relayed" || rType === "relayed") ||
         (lIp === serverIp || rIp === serverIp), "One peer uses a relay");
    } else {
      info("P2P configured");
      ok(((lType !== "relayed") && (rType !== "relayed")), "Pure peer to peer call without a relay");
    }
  },

  











  checkStatsIceConnections : function(stats,
      offerConstraintsList, offerOptions, answer) {
    var numIceConnections = 0;
    Object.keys(stats).forEach(key => {
      if ((stats[key].type === "candidatepair") && stats[key].selected) {
        numIceConnections += 1;
      }
    });
    info("ICE connections according to stats: " + numIceConnections);
    if (answer.sdp.contains('a=group:BUNDLE')) {
      is(numIceConnections, 1, "stats reports exactly 1 ICE connection");
    } else {
      
      
      var numAudioTracks =
          this.countTracksInConstraint('audio', offerConstraintsList) ||
        this.audioInOfferOptions(offerOptions);

      var numVideoTracks =
          this.countTracksInConstraint('video', offerConstraintsList) ||
        this.videoInOfferOptions(offerOptions);

      var numDataTracks = this.dataChannels.length;

      var numAudioVideoDataTracks = numAudioTracks + numVideoTracks + numDataTracks;
      info("expected audio + video + data tracks: " + numAudioVideoDataTracks);
      is(numAudioVideoDataTracks, numIceConnections, "stats ICE connections matches expected A/V tracks");
    }
  },

  expectNegotiationNeeded : function() {
    if (!this.observedNegotiationNeeded) {
      this.observedNegotiationNeeded = new Promise((resolve) => {
        this.onnegotiationneeded = resolve;
      });
    }
  },

  








  hasStat : function(stats, props) {
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

  


  close : function() {
    this._pc.close();
    this.localMediaElements.forEach(e => e.pause());
    info(this + ": Closed connection.");
  },

  




  toString : function() {
    return "PeerConnectionWrapper (" + this.label + ")";
  }
};


function addLoadEvent() {}

var scriptsReady = Promise.all([
  "/tests/SimpleTest/SimpleTest.js",
  "head.js",
  "templates.js",
  "turnConfig.js",
  "dataChannel.js",
  "network.js"
].map(script  => {
  var el = document.createElement("script");
  if (typeof scriptRelativePath === 'string' && script.charAt(0) !== '/') {
    script = scriptRelativePath + script;
  }
  el.src = script;
  document.head.appendChild(el);
  return new Promise(r => { el.onload = r; el.onerror = r; });
}));

function createHTML(options) {
  return scriptsReady.then(() => realCreateHTML(options));
}

function runNetworkTest(testFunction) {
  return scriptsReady.then(() => {
    return runTestWhenReady(options => {
      startNetworkAndTest()
        .then(() => testFunction(options));
    });
  });
}
