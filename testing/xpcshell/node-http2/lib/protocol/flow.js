var assert = require('assert');








var Duplex  = require('stream').Duplex;

exports.Flow = Flow;











































var INITIAL_WINDOW_SIZE = 65535;


function Flow(flowControlId) {
  Duplex.call(this, { objectMode: true });

  this._window = this._initialWindow = INITIAL_WINDOW_SIZE;
  this._flowControlId = flowControlId;
  this._queue = [];
  this._ended = false;
  this._received = 0;
  this._blocked = false;
}
Flow.prototype = Object.create(Duplex.prototype, { constructor: { value: Flow } });





Flow.prototype._receive = function _receive(frame, callback) {
  throw new Error('The _receive(frame, callback) method has to be overridden by the child class!');
};





Flow.prototype._write = function _write(frame, encoding, callback) {
  if (frame.flags.END_STREAM || (frame.type === 'RST_STREAM')) {
    this._ended = true;
  }

  if ((frame.type === 'DATA') && (frame.data.length > 0)) {
    this._receive(frame, function() {
      this._received += frame.data.length;
      if (!this._restoreWindowTimer) {
        this._restoreWindowTimer = setImmediate(this._restoreWindow.bind(this));
      }
      callback();
    }.bind(this));
  }

  else {
    this._receive(frame, callback);
  }

  if ((frame.type === 'WINDOW_UPDATE') &&
      ((this._flowControlId === undefined) || (frame.stream === this._flowControlId))) {
    this._updateWindow(frame);
  }
};




Flow.prototype._restoreWindow = function _restoreWindow() {
  delete this._restoreWindowTimer;
  if (!this._ended && (this._received > 0)) {
    this.push({
      type: 'WINDOW_UPDATE',
      flags: {},
      stream: this._flowControlId,
      window_size: this._received
    });
    this._received = 0;
  }
};




















Flow.prototype._send = function _send() {
  throw new Error('The _send() method has to be overridden by the child class!');
};




Flow.prototype._read = function _read() {
  
  if (this._queue.length === 0) {
    this._send();
  }

  
  
  else if (this._window > 0) {
    this._blocked = false;
    this._readableState.sync = true; 
    do {
      var moreNeeded = this._push(this._queue[0]);
      if (moreNeeded !== null) {
        this._queue.shift();
      }
    } while (moreNeeded && (this._queue.length > 0));
    this._readableState.sync = false;

    assert((moreNeeded == false) ||                              
           (this._queue.length === 0) ||                         
           (!this._window && (this._queue[0].type === 'DATA'))); 
  }

  
  else if (!this._blocked) {
    this._parentPush({
      type: 'BLOCKED',
      flags: {},
      stream: this._flowControlId
    });
    this.once('window_update', this._read);
    this._blocked = true;
  }
};

var MAX_PAYLOAD_SIZE = 4096; 




Flow.prototype.read = function read(limit) {
  if (limit === 0) {
    return Duplex.prototype.read.call(this, 0);
  } else if (limit === -1) {
    limit = 0;
  } else if ((limit === undefined) || (limit > MAX_PAYLOAD_SIZE)) {
    limit = MAX_PAYLOAD_SIZE;
  }

  
  var frame = this._readableState.buffer[0];
  if (!frame && !this._readableState.ended) {
    this._read();
    frame = this._readableState.buffer[0];
  }

  if (frame && (frame.type === 'DATA')) {
    
    
    
    if (limit === 0) {
      return Duplex.prototype.read.call(this, 0);
    }

    else if (frame.data.length > limit) {
      this._log.trace({ frame: frame, size: frame.data.length, forwardable: limit },
        'Splitting out forwardable part of a DATA frame.');
      this.unshift({
        type: 'DATA',
        flags: {},
        stream: frame.stream,
        data: frame.data.slice(0, limit)
      });
      frame.data = frame.data.slice(limit);
    }
  }

  return Duplex.prototype.read.call(this);
};


Flow.prototype._parentPush = function _parentPush(frame) {
  this._log.trace({ frame: frame }, 'Pushing frame into the output queue');

  if (frame && (frame.type === 'DATA') && (this._window !== Infinity)) {
    this._log.trace({ window: this._window, by: frame.data.length },
                    'Decreasing flow control window size.');
    this._window -= frame.data.length;
    assert(this._window >= 0);
  }

  return Duplex.prototype.push.call(this, frame);
};





Flow.prototype._push = function _push(frame) {
  var data = frame && (frame.type === 'DATA') && frame.data;

  if (!data || (data.length <= this._window)) {
    return this._parentPush(frame);
  }

  else if (this._window <= 0) {
    return null;
  }

  else {
    this._log.trace({ frame: frame, size: frame.data.length, forwardable: this._window },
                    'Splitting out forwardable part of a DATA frame.');
    frame.data = data.slice(this._window);
    this._parentPush({
      type: 'DATA',
      flags: {},
      stream: frame.stream,
      data: data.slice(0, this._window)
    });
    return null;
  }
};


Flow.prototype.push = function push(frame) {
  if (frame === null) {
    this._log.debug('Enqueueing outgoing End Of Stream');
  } else {
    this._log.debug({ frame: frame }, 'Enqueueing outgoing frame');
  }

  var moreNeeded = null;
  if (this._queue.length === 0) {
    moreNeeded = this._push(frame);
  }

  if (moreNeeded === null) {
    this._queue.push(frame);
  }

  return moreNeeded;
};



Flow.prototype.getLastQueuedFrame = function getLastQueuedFrame() {
  var readableQueue = this._readableState.buffer;
  return this._queue[this._queue.length - 1] || readableQueue[readableQueue.length - 1];
};












var WINDOW_SIZE_LIMIT = Math.pow(2, 31) - 1;

Flow.prototype._increaseWindow = function _increaseWindow(size) {
  if ((this._window === Infinity) && (size !== Infinity)) {
    this._log.error('Trying to increase flow control window after flow control was turned off.');
    this.emit('error', 'FLOW_CONTROL_ERROR');
  } else {
    this._log.trace({ window: this._window, by: size }, 'Increasing flow control window size.');
    this._window += size;
    if ((this._window !== Infinity) && (this._window > WINDOW_SIZE_LIMIT)) {
      this._log.error('Flow control window grew too large.');
      this.emit('error', 'FLOW_CONTROL_ERROR');
    } else {
      this.emit('window_update');
    }
  }
};









Flow.prototype._updateWindow = function _updateWindow(frame) {
  this._increaseWindow(frame.flags.END_FLOW_CONTROL ? Infinity : frame.window_size);
};





Flow.prototype.setInitialWindow = function setInitialWindow(initialWindow) {
  this._increaseWindow(initialWindow - this._initialWindow);
  this._initialWindow = initialWindow;
};
