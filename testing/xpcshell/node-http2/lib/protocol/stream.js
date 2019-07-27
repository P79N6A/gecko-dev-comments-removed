var assert = require('assert');










var Duplex = require('stream').Duplex;

exports.Stream = Stream;


































function Stream(log, connection) {
  Duplex.call(this);

  
  this._log = log.child({ component: 'stream', s: this });

  
  this._initializeManagement();

  
  this._initializeDataFlow();

  
  this._initializeState();

  this.connection = connection;
}

Stream.prototype = Object.create(Duplex.prototype, { constructor: { value: Stream } });





var DEFAULT_PRIORITY = Math.pow(2, 30);
var MAX_PRIORITY = Math.pow(2, 31) - 1;


Stream.prototype._initializeManagement = function _initializeManagement() {
  this._resetSent = false;
  this._priority = DEFAULT_PRIORITY;
  this._letPeerPrioritize = true;
};

Stream.prototype.promise = function promise(headers) {
  var stream = new Stream(this._log, this.connection);
  stream._priority = Math.min(this._priority + 1, MAX_PRIORITY);
  this._pushUpstream({
    type: 'PUSH_PROMISE',
    flags: {},
    stream: this.id,
    promised_stream: stream,
    headers: headers
  });
  return stream;
};

Stream.prototype._onPromise = function _onPromise(frame) {
  this.emit('promise', frame.promised_stream, frame.headers);
};

Stream.prototype.headers = function headers(headers) {
  this._pushUpstream({
    type: 'HEADERS',
    flags: {},
    stream: this.id,
    headers: headers
  });
};

Stream.prototype._onHeaders = function _onHeaders(frame) {
  if (frame.priority !== undefined) {
    this.priority(frame.priority, true);
  }
  this.emit('headers', frame.headers);
};

Stream.prototype.priority = function priority(priority, peer) {
  if ((peer && this._letPeerPrioritize) || !peer) {
    if (!peer) {
      this._letPeerPrioritize = false;

      var lastFrame = this.upstream.getLastQueuedFrame();
      if (lastFrame && ((lastFrame.type === 'HEADERS') || (lastFrame.type === 'PRIORITY'))) {
        lastFrame.priority = priority;
      } else {
        this._pushUpstream({
          type: 'PRIORITY',
          flags: {},
          stream: this.id,
          priority: priority
        });
      }
    }

    this._log.debug({ priority: priority }, 'Changing priority');
    this.emit('priority', priority);
    this._priority = priority;
  }
};

Stream.prototype._onPriority = function _onPriority(frame) {
  this.priority(frame.priority, true);
};



Stream.prototype.reset = function reset(error) {
  if (!this._resetSent) {
    this._resetSent = true;
    this._pushUpstream({
      type: 'RST_STREAM',
      flags: {},
      stream: this.id,
      error: error
    });
  }
};


Stream.prototype.altsvc = function altsvc(host, port, protocolID, maxAge, origin) {
    var stream;
    if (origin) {
        stream = 0;
    } else {
        stream = this.id;
    }
    this._pushUpstream({
        type: 'ALTSVC',
        flags: {},
        stream: stream,
        host: host,
        port: port,
        protocolID: protocolID,
        origin: origin,
        maxAge: maxAge
    });
};


































var Flow = require('./flow').Flow;

Stream.prototype._initializeDataFlow = function _initializeDataFlow() {
  this.id = undefined;

  this._ended = false;

  this.upstream = new Flow();
  this.upstream._log = this._log;
  this.upstream._send = this._send.bind(this);
  this.upstream._receive = this._receive.bind(this);
  this.upstream.write = this._writeUpstream.bind(this);
  this.upstream.on('error', this.emit.bind(this, 'error'));

  this.on('finish', this._finishing);
};

Stream.prototype._pushUpstream = function _pushUpstream(frame) {
  this.upstream.push(frame);
  this._transition(true, frame);
};



Stream.prototype._writeUpstream = function _writeUpstream(frame) {
  this._log.debug({ frame: frame }, 'Receiving frame');

  var moreNeeded = Flow.prototype.write.call(this.upstream, frame);

  
  this._transition(false, frame);

  
  if (frame.type === 'HEADERS') {
    if (this._processedHeaders && !frame.flags['END_STREAM']) {
      this.emit('error', 'PROTOCOL_ERROR');
    }
    this._processedHeaders = true;
    this._onHeaders(frame);
  } else if (frame.type === 'PUSH_PROMISE') {
    this._onPromise(frame);
  } else if (frame.type === 'PRIORITY') {
    this._onPriority(frame);
  } else if (frame.type === 'ALTSVC') {
    
  } else if (frame.type === 'BLOCKED') {
    
  }

  
  else if ((frame.type !== 'DATA') &&
           (frame.type !== 'WINDOW_UPDATE') &&
           (frame.type !== 'RST_STREAM')) {
    this._log.error({ frame: frame }, 'Invalid stream level frame');
    this.emit('error', 'PROTOCOL_ERROR');
  }

  return moreNeeded;
};


Stream.prototype._receive = function _receive(frame, ready) {
  
  
  if (!this._ended && (frame.type === 'DATA')) {
    var moreNeeded = this.push(frame.data);
    if (!moreNeeded) {
      this._receiveMore = ready;
    }
  }

  
  if (!this._ended && (frame.flags.END_STREAM || (frame.type === 'RST_STREAM'))) {
    this.push(null);
    this._ended = true;
  }

  
  if (this._receiveMore !== ready) {
    ready();
  }
};



Stream.prototype._read = function _read() {
  if (this._receiveMore) {
    var receiveMore = this._receiveMore;
    delete this._receiveMore;
    receiveMore();
  }
};


Stream.prototype._write = function _write(buffer, encoding, ready) {
  
  var moreNeeded = this._pushUpstream({
    type: 'DATA',
    flags: {},
    stream: this.id,
    data: buffer
  });

  
  if (moreNeeded) {
    ready();
  } else {
    this._sendMore = ready;
  }
};




Stream.prototype._send = function _send() {
  if (this._sendMore) {
    var sendMore = this._sendMore;
    delete this._sendMore;
    sendMore();
  }
};





var emptyBuffer = new Buffer(0);
Stream.prototype._finishing = function _finishing() {
  var endFrame = {
    type: 'DATA',
    flags: { END_STREAM: true },
    stream: this.id,
    data: emptyBuffer
  };
  var lastFrame = this.upstream.getLastQueuedFrame();
  if (lastFrame && ((lastFrame.type === 'DATA') || (lastFrame.type === 'HEADERS'))) {
    this._log.debug({ frame: lastFrame }, 'Marking last frame with END_STREAM flag.');
    lastFrame.flags.END_STREAM = true;
    this._transition(true, endFrame);
  } else {
    this._pushUpstream(endFrame);
  }
};































Stream.prototype._initializeState = function _initializeState() {
  this.state = 'IDLE';
  this._initiated = undefined;
  this._closedByUs = undefined;
  this._closedWithRst = undefined;
  this._processedHeaders = false;
};



Stream.prototype._setState = function transition(state) {
  assert(this.state !== state);
  this._log.debug({ from: this.state, to: state }, 'State transition');
  this.state = state;
  this.emit('state', state);
};



function activeState(state) {
  return ((state === 'HALF_CLOSED_LOCAL') || (state === 'HALF_CLOSED_REMOTE') || (state === 'OPEN'));
}




Stream.prototype._transition = function transition(sending, frame) {
  var receiving = !sending;
  var connectionError;
  var streamError;

  var DATA = false, HEADERS = false, PRIORITY = false, ALTSVC = false, BLOCKED = false;
  var RST_STREAM = false, PUSH_PROMISE = false, WINDOW_UPDATE = false;
  switch(frame.type) {
    case 'DATA'         : DATA          = true; break;
    case 'HEADERS'      : HEADERS       = true; break;
    case 'PRIORITY'     : PRIORITY      = true; break;
    case 'RST_STREAM'   : RST_STREAM    = true; break;
    case 'PUSH_PROMISE' : PUSH_PROMISE  = true; break;
    case 'WINDOW_UPDATE': WINDOW_UPDATE = true; break;
    case 'ALTSVC'       : ALTSVC        = true; break;
    case 'BLOCKED'      : BLOCKED       = true; break;
  }

  var previousState = this.state;

  switch (this.state) {
    
    
    
    
    
    case 'IDLE':
      if (HEADERS) {
        this._setState('OPEN');
        if (frame.flags.END_STREAM) {
          this._setState(sending ? 'HALF_CLOSED_LOCAL' : 'HALF_CLOSED_REMOTE');
        }
        this._initiated = sending;
      } else if (sending && RST_STREAM) {
        this._setState('CLOSED');
      } else if (PRIORITY) {
        
      } else {
        connectionError = 'PROTOCOL_ERROR';
      }
      break;

    
    
    
    
    
    
    
    
    
    case 'RESERVED_LOCAL':
      if (sending && HEADERS) {
        this._setState('HALF_CLOSED_REMOTE');
      } else if (RST_STREAM) {
        this._setState('CLOSED');
      } else if (PRIORITY) {
        
      } else {
        connectionError = 'PROTOCOL_ERROR';
      }
      break;

    
    
    
    
    
    
    
    case 'RESERVED_REMOTE':
      if (RST_STREAM) {
        this._setState('CLOSED');
      } else if (receiving && HEADERS) {
        this._setState('HALF_CLOSED_LOCAL');
      } else if (BLOCKED || PRIORITY) {
        
      } else {
        connectionError = 'PROTOCOL_ERROR';
      }
      break;

    
    
    
    
    
    
    
    
    
    case 'OPEN':
      if (frame.flags.END_STREAM) {
        this._setState(sending ? 'HALF_CLOSED_LOCAL' : 'HALF_CLOSED_REMOTE');
      } else if (RST_STREAM) {
        this._setState('CLOSED');
      } else {
        
      }
      break;

    
    
    
    
    
    
    case 'HALF_CLOSED_LOCAL':
      if (RST_STREAM || (receiving && frame.flags.END_STREAM)) {
        this._setState('CLOSED');
      } else if (BLOCKED || ALTSVC || receiving || PRIORITY || (sending && WINDOW_UPDATE)) {
        
      } else {
        connectionError = 'PROTOCOL_ERROR';
      }
      break;

    
    
    
    
    
    
    
    
    
    
    case 'HALF_CLOSED_REMOTE':
      if (RST_STREAM || (sending && frame.flags.END_STREAM)) {
        this._setState('CLOSED');
      } else if (BLOCKED || ALTSVC || sending || PRIORITY || (receiving && WINDOW_UPDATE)) {
        
      } else {
        connectionError = 'PROTOCOL_ERROR';
      }
      break;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    case 'CLOSED':
      if (PRIORITY || (sending && RST_STREAM) ||
          (receiving && this._closedByUs &&
           (this._closedWithRst || WINDOW_UPDATE || RST_STREAM || ALTSVC))) {
        
      } else {
        streamError = 'STREAM_CLOSED';
      }
      break;
  }

  
  
  
  if ((this.state === 'CLOSED') && (previousState !== 'CLOSED')) {
    this._closedByUs = sending;
    this._closedWithRst = RST_STREAM;
  }

  
  
  
  
  
  
  if (PUSH_PROMISE && !connectionError && !streamError) {
    


    assert(frame.promised_stream.state === 'IDLE', frame.promised_stream.state);
    frame.promised_stream._setState(sending ? 'RESERVED_LOCAL' : 'RESERVED_REMOTE');
    frame.promised_stream._initiated = sending;
  }

  
  if (this._initiated) {
    var change = (activeState(this.state) - activeState(previousState));
    if (sending) {
      frame.count_change = change;
    } else {
      frame.count_change(change);
    }
  } else if (sending) {
    frame.count_change = 0;
  }

  
  if (connectionError || streamError) {
    var info = {
      error: connectionError,
      frame: frame,
      state: this.state,
      closedByUs: this._closedByUs,
      closedWithRst: this._closedWithRst
    };

    
    if (sending) {
      this._log.error(info, 'Sending illegal frame.');
      throw new Error('Sending illegal frame (' + frame.type + ') in ' + this.state + ' state.');
    }

    
    
    
    
    else {
      this._log.error(info, 'Received illegal frame.');
      if (connectionError) {
        this.emit('connectionError', connectionError);
      } else {
        this.reset(streamError);
        this.emit('error', streamError);
      }
    }
  }
};




exports.serializers = {};

var nextId = 0;
exports.serializers.s = function(stream) {
  if (!('_id' in stream)) {
    stream._id = nextId;
    nextId += 1;
  }
  return stream._id;
};
