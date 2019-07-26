













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
    'PC_LOCAL_CREATE_OFFER',
    function (test) {
      test.pcLocal.createOffer(function () {
        test.next();
      });
    }
  ],
  [
    'PC_LOCAL_SET_LOCAL_DESCRIPTION',
    function (test) {
      test.pcLocal.setLocalDescription(test.pcLocal._last_offer, function () {
        test.next();
      });
    }
  ],
  [
    'PC_REMOTE_SET_REMOTE_DESCRIPTION',
    function (test) {
      test.pcRemote.setRemoteDescription(test.pcLocal._last_offer, function () {
        test.next();
      });
    }
  ],
  [
    'PC_REMOTE_CREATE_ANSWER',
    function (test) {
      test.pcRemote.createAnswer(function () {
        test.next();
      });
    }
  ],
  [
    'PC_LOCAL_SET_REMOTE_DESCRIPTION',
    function (test) {
      test.pcLocal.setRemoteDescription(test.pcRemote._last_answer, function () {
        test.next();
      });
    }
  ],
  [
    'PC_REMOTE_SET_LOCAL_DESCRIPTION',
    function (test) {
      test.pcRemote.setLocalDescription(test.pcRemote._last_answer, function () {
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

  
  this._desktopSupportedOnly = options.desktopSupportedOnly || false;

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








PeerConnectionTest.prototype.setMediaConstraints =
function PCT_setMediaConstraints(constraintsLocal, constraintsRemote) {
  this.pcLocal.constraints = constraintsLocal;
  this.pcRemote.constraints = constraintsRemote;
};




PeerConnectionTest.prototype.run = function PCT_run() {
  var self = this;

  info("Starting tests");
  runTest(function () {
    self.next();
  }, this._desktopSupportedOnly);
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
  this.streams = [ ];

  info("Creating new PeerConnectionWrapper: " + this.label);
  this._pc = new mozRTCPeerConnection(this.configuration);

  var self = this;
  this._pc.onaddstream = function (event) {
    
    self.attachMedia(event.stream, 'video', 'remote');
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
        }, unexpectedCallbackAndFinish);
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
    }, unexpectedCallbackAndFinish);
  },

  





  createAnswer : function PCW_createAnswer(onSuccess) {
    var self = this;

    this._pc.createAnswer(function (answer) {
      info('Got answer for ' + self.label + ': ' + JSON.stringify(answer));
      self._last_answer = answer;
      onSuccess(answer);
    }, unexpectedCallbackAndFinish);
  },

  







  setLocalDescription : function PCW_setLocalDescription(sdp, onSuccess) {
    var self = this;

    this._pc.setLocalDescription(sdp, function () {
      info("Successfully set the local description for " + self.label);
      onSuccess();
    }, unexpectedCallbackAndFinish);
  },

  







  setRemoteDescription : function PCW_setRemoteDescription(sdp, onSuccess) {
    var self = this;

    this._pc.setRemoteDescription(sdp, function () {
      info("Successfully set remote description for " + self.label);
      onSuccess();
    }, unexpectedCallbackAndFinish);
  },

  





  checkMedia : function PCW_checkMedia(constraintsRemote) {
    is(this._pc.localStreams.length, this.constraints.length,
       this.label + ' has ' + this.constraints.length + ' local streams');

    
    is(this._pc.remoteStreams.length, 1,
       this.label + ' has ' + 1 + ' remote streams');
  },

  


  close : function PCW_close() {
    
    
    try {
      this._pc.close();
      info(this.label + ": Closed connection.");
    } catch (e) {
      info(this.label + ": Failure in closing connection - " + e.message);
    }
  }
};
