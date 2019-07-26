













function CommandChain(framework) {
  this._framework = framework;

  this._commands = [ ];
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

        info("Run step: " + step[0]);  
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
  }
};





var commandsPeerConnection = [
  [
    'PC_LOCAL_GUM',
    function (test) {
      test.pcLocal.getAllUserMedia(function () {
        test.next();
      });
    }
  ],
  [
    'PC_REMOTE_GUM',
    function (test) {
      test.pcRemote.getAllUserMedia(function () {
        test.next();
      });
    }
  ],
  [
    'PC_CHECK_INITIAL_SIGNALINGSTATE',
    function (test) {
      is(test.pcLocal.signalingState, "stable",
         "Initial local signalingState is 'stable'");
      is(test.pcRemote.signalingState, "stable",
         "Initial remote signalingState is 'stable'");
      test.next();
    }
  ],
  [
    'PC_LOCAL_CREATE_OFFER',
    function (test) {
      test.createOffer(test.pcLocal, function () {
        is(test.pcLocal.signalingState, "stable",
           "Local create offer does not change signaling state");
        test.next();
      });
    }
  ],
  [
    'PC_LOCAL_SET_LOCAL_DESCRIPTION',
    function (test) {
      test.setLocalDescription(test.pcLocal, test.pcLocal._last_offer, function () {
        is(test.pcLocal.signalingState, "have-local-offer",
           "signalingState after local setLocalDescription is 'have-local-offer'");
        test.next();
      });
    }
  ],
  [
    'PC_REMOTE_SET_REMOTE_DESCRIPTION',
    function (test) {
      test.setRemoteDescription(test.pcRemote, test.pcLocal._last_offer, function () {
        is(test.pcRemote.signalingState, "have-remote-offer",
           "signalingState after remote setRemoteDescription is 'have-remote-offer'");
        test.next();
      });
    }
  ],
  [
    'PC_REMOTE_CREATE_ANSWER',
    function (test) {
      test.createAnswer(test.pcRemote, function () {
        is(test.pcRemote.signalingState, "have-remote-offer",
           "Remote createAnswer does not change signaling state");
        test.next();
      });
    }
  ],
  [
    'PC_LOCAL_SET_REMOTE_DESCRIPTION',
    function (test) {
      test.setRemoteDescription(test.pcLocal, test.pcRemote._last_answer, function () {
        is(test.pcLocal.signalingState, "stable",
           "signalingState after local setRemoteDescription is 'stable'");
        test.next();
      });
    }
  ],
  [
    'PC_REMOTE_SET_LOCAL_DESCRIPTION',
    function (test) {
      test.setLocalDescription(test.pcRemote, test.pcRemote._last_answer, function () {
        is(test.pcRemote.signalingState, "stable",
           "signalingState after remote setLocalDescription is 'stable'");
        test.next();
      });
    }
  ],
  [
    'PC_LOCAL_CHECK_MEDIA',
    function (test) {
      test.pcLocal.checkMedia(test.pcRemote.constraints);
      test.next();
    }
  ],
  [
    'PC_REMOTE_CHECK_MEDIA',
    function (test) {
      test.pcRemote.checkMedia(test.pcLocal.constraints);
      test.next();
    }
  ]
];













function PeerConnectionTest(options) {
  
  options = options || { };

  this.pcLocal = new PeerConnectionWrapper('pcLocal', options.config_pc1);
  this.pcRemote = new PeerConnectionWrapper('pcRemote', options.config_pc2 || options.config_pc1);

  
  this.chain = new CommandChain(this);
  this.chain.commands = commandsPeerConnection;

  var self = this;
  this.chain.onFinished = function () {
    self.teardown();
  }
}




PeerConnectionTest.prototype.next = function PCT_next() {
  this.chain.executeNext();
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
    info(peer + ": 'onsignalingstatechange' event registered for async check");

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
  this.pcLocal.constraints = constraintsLocal;
  this.pcRemote.constraints = constraintsRemote;
};






PeerConnectionTest.prototype.setOfferConstraints =
function PCT_setOfferConstraints(constraints) {
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
    info(peer + ": 'onsignalingstatechange' event registered for async check");

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
  if (this.pcLocal) {
    this.pcLocal.close();
    this.pcLocal = null;
  }

  if (this.pcRemote) {
    this.pcRemote.close();
    this.pcRemote = null;
  }

  info("Test finished");
  SimpleTest.finish();
};











function PeerConnectionWrapper(label, configuration) {
  this.configuration = configuration;
  this.label = label;

  this.constraints = [ ];
  this.offerConstraints = {};
  this.streams = [ ];

  info("Creating new PeerConnectionWrapper: " + this);
  this._pc = new mozRTCPeerConnection(this.configuration);

  



  this.onsignalingstatechange = unexpectedEventAndFinish(this, 'onsignalingstatechange');


  var self = this;
  this._pc.onaddstream = function (event) {
    
    self.attachMedia(event.stream, 'video', 'remote');
  };

  







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

  




  get remoteDescription() {
    return this._pc.remoteDescription;
  },

  





  set remoteDescription(desc) {
    this._pc.remoteDescription = desc;
  },

  




  get signalingState() {
    return this._pc.signalingState;
  },

  










  attachMedia : function PCW_attachMedia(stream, type, side) {
    info("Got media stream: " + type + " (" + side + ")");
    this.streams.push(stream);

    if (side === 'local') {
      this._pc.addStream(stream);
    }

    var element = createMediaElement(type, this._label + '_' + side);
    element.mozSrcObject = stream;
    element.play();
  },

  





  getAllUserMedia : function PCW_GetAllUserMedia(onSuccess) {
    var self = this;

    function _getAllUserMedia(constraintsList, index) {
      if (index < constraintsList.length) {
        var constraints = constraintsList[index];

        getUserMedia(constraints, function (stream) {
          var type = constraints;

          if (constraints.audio) {
            type = 'audio';
          }

          if (constraints.video) {
            type += 'video';
          }

          self.attachMedia(stream, type, 'local');

          _getAllUserMedia(constraintsList, index + 1);
        }, unexpectedCallbackAndFinish());
      } else {
        onSuccess();
      }
    }

    info("Get " + this.constraints.length + " local streams");
    _getAllUserMedia(this.constraints, 0);
  },

  





  createOffer : function PCW_createOffer(onSuccess) {
    var self = this;

    this._pc.createOffer(function (offer) {
      info("Got offer: " + JSON.stringify(offer));
      self._last_offer = offer;
      onSuccess(offer);
    }, unexpectedCallbackAndFinish(), this.offerConstraints);
  },

  





  createAnswer : function PCW_createAnswer(onSuccess) {
    var self = this;

    this._pc.createAnswer(function (answer) {
      info(self + ": Got answer: " + JSON.stringify(answer));
      self._last_answer = answer;
      onSuccess(answer);
    }, unexpectedCallbackAndFinish());
  },

  







  setLocalDescription : function PCW_setLocalDescription(desc, onSuccess) {
    var self = this;
    this._pc.setLocalDescription(desc, function () {
      info(self + ": Successfully set the local description");
      onSuccess();
    }, unexpectedCallbackAndFinish());
  },

  








  setLocalDescriptionAndFail : function PCW_setLocalDescriptionAndFail(desc, onFailure) {
    var self = this;
    this._pc.setLocalDescription(desc,
      unexpectedCallbackAndFinish("setLocalDescription should have failed."),
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
    }, unexpectedCallbackAndFinish());
  },

  








  setRemoteDescriptionAndFail : function PCW_setRemoteDescriptionAndFail(desc, onFailure) {
    var self = this;
    this._pc.setRemoteDescription(desc,
      unexpectedCallbackAndFinish("setRemoteDescription should have failed."),
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
    }, unexpectedCallbackAndFinish());
  },

  








  addIceCandidateAndFail : function PCW_addIceCandidateAndFail(candidate, onFailure) {
    var self = this;

    this._pc.addIceCandidate(candidate,
      unexpectedCallbackAndFinish("addIceCandidate should have failed."),
      function (err) {
        info(self + ": As expected, failed to add an ICE candidate");
        onFailure(err);
    }) ;
  },

  





  checkMedia : function PCW_checkMedia(constraintsRemote) {
    is(this._pc.localStreams.length, this.constraints.length,
       this + ' has ' + this.constraints.length + ' local streams');

    
    is(this._pc.remoteStreams.length, 1,
       this + ' has ' + 1 + ' remote streams');
  },

  


  close : function PCW_close() {
    
    
    try {
      this._pc.close();
      info(this + ": Closed connection.");
    } catch (e) {
      info(this + ": Failure in closing connection - " + e.message);
    }
  },

  


  toString : function PCW_toString() {
    return "PeerConnectionWrapper (" + this.label + ")";
  }
};
