var util = require('util');
var assert = require('assert');
var spdy = require('../spdy');
var constants = spdy.protocol.constants;

var Stream = spdy.Stream;








function Connection(socket, options, server) {
  process.EventEmitter.call(this);

  var state = {};
  this._spdyState = state;

  
  
  
  this.httpAllowHalfOpen = true;

  
  this.timeout = server && server.timeout || 0;

  
  state.maxStreams = options.maxStreams || Infinity;
  state.initialSinkSize = constants.DEFAULT_WINDOW;
  state.initialWindowSize = options.windowSize || 1 << 20;
  state.autoSpdy31 = options.autoSpdy31;

  
  state.sinkSize = state.initialSinkSize;
  state.windowSize = constants.DEFAULT_WINDOW;

  
  state.maxChunk = options.maxChunk === undefined ? 8 * 1024 : options.maxChunk;

  
  state.closed = false;
  state.pool = spdy.zlibpool.create(options.headerCompression);
  state.counters = {
    pushCount: 0,
    streamCount: 0
  };
  state.socketBuffering = false;

  state.version = null;
  state.deflate = null;
  state.inflate = null;

  
  state.isServer = options.isServer;
  state.streams = {};
  state.streamCount = 0;
  state.lastId = 0;
  state.pushId = 0;
  state.pingId = state.isServer ? 0 : 1;
  state.pings = {};
  state.goaway = false;

  
  state.xForward = null;

  
  state.scheduler = spdy.scheduler.create(this);

  
  state.parser = spdy.protocol.parser.create(this);
  state.framer = spdy.protocol.framer.create();

  
  state.locked = false;
  state.lockQueue = [];

  this.socket = socket;
  this.encrypted = socket.encrypted;
  this.readable = true;
  this.writable = true;

  this._init();
}
util.inherits(Connection, process.EventEmitter);
exports.Connection = Connection;

Connection.prototype._init = function init() {
  var self = this;
  var state = this._spdyState;
  var pool = state.pool;
  var pair = null;
  var sentSettings = false;

  
  this._spdyState.parser.on('frame', this._handleFrame.bind(this));

  this._spdyState.parser.on('version', function onversion(version) {
    var prev = state.version;
    var framer = state.framer;

    state.version = version;

    
    if (!prev) {
      pair = pool.get('spdy/' + version);
      state.deflate = pair.deflate;
      state.inflate = pair.inflate;
      framer.setCompression(pair.deflate, pair.inflate);
    }
    framer.setVersion(version);

    
    if (!sentSettings) {
      sentSettings = true;
      framer.settingsFrame({
        maxStreams: state.maxStreams,
        windowSize: state.initialWindowSize
      }, function(err, frame) {
        if (err)
          return self.emit('error', err);
        self.write(frame);
      });

      
      self._sendWindowUpdate(true);
    }
  });

  
  state.parser.on('error', function onParserError(err) {
    self.emit('error', err);
  });

  this.socket.pipe(state.parser);

  
  this.socket.setTimeout(this.timeout);
  this.socket.once('timeout', function ontimeout() {
    self.socket.destroy();
  });

  
  this.socket.on('error', function onSocketError(e) {
    self.emit('error', e);
  });

  this.socket.once('close', function onclose() {
    var err = new Error('socket hang up');
    err.code = 'ECONNRESET';
    self._destroyStreams(err);
    self.emit('close');

    state.closed = true;
    if (pair)
      pool.put(pair);
  });

  
  this.socket.allowHalfOpen = false;

  if (spdy.utils.isLegacy) {
    this.socket.on('drain', function ondrain() {
      self.emit('drain');
    });
  }

  
  
  this.socket.on('drain', function () {
    state.socketBuffering = false;
    Object.keys(state.streams).forEach(function(id) {
      state.streams[id]._drainSink(0);
    });
  });
};





Connection.prototype._handleFrame = function handleFrame(frame) {
  var state = this._spdyState;

  if (state.closed)
    return;

  var stream;

  
  if (frame.type === 'SYN_STREAM') {
    stream = this._handleSynStream(frame);
  } else {
    if (frame.id !== undefined) {
      
      stream = state.streams[frame.id];

      
      if (stream === undefined &&
          !(frame.type === 'WINDOW_UPDATE' && frame.id === 0)) {
        if (frame.type === 'RST_STREAM')
          return;
        this._rst(frame.id, constants.rst.INVALID_STREAM);
        return;
      }
    }

    
    if (frame.type === 'DATA') {
      this._handleData(stream, frame);
    
    } else if (frame.type === 'SYN_REPLY') {
      
      if (!stream._spdyState.isClient) {
        this._rst(frame.id, constants.rst.PROTOCOL_ERROR);
        return;
      }

      stream._handleResponse(frame);

    
    } else if (frame.type === 'RST_STREAM') {
      stream._spdyState.rstCode = 0;
      stream._spdyState.closedBy.us = true;
      stream._spdyState.closedBy.them = true;

      
      var err = new Error('Received rst: ' + frame.status);
      err.code = 'RST_STREAM';
      err.status = frame.status;
      stream.destroy(err);
    
    } else if (frame.type === 'PING') {
      this._handlePing(frame.pingId);
    } else if (frame.type === 'SETTINGS') {
      this._setDefaultWindow(frame.settings);
    } else if (frame.type === 'GOAWAY') {
      state.goaway = frame.lastId;
      this.writable = false;
    } else if (frame.type === 'WINDOW_UPDATE') {
      if (stream)
        stream._drainSink(frame.delta);
      else
        this._drainSink(frame.delta);
    } else if (frame.type === 'HEADERS') {
      stream.emit('headers', frame.headers);
    } else if (frame.type === 'X_FORWARDED') {
      state.xForward = frame.host;
    } else {
      console.error('Unknown type: ', frame.type);
    }
  }

  
  if (frame.fin) {
    
    if (stream._spdyState.closedBy.them) {
      stream._spdyState.rstCode = constants.rst.PROTOCOL_ERROR;
      stream.emit('error', 'Already half-closed');
    } else {
      stream._spdyState.closedBy.them = true;

      
      stream._recvEnd();

      stream._handleClose();
    }
  }
};





Connection.prototype._handleSynStream = function handleSynStream(frame) {
  var state = this._spdyState;
  var associated;

  if (state.goaway && state.goaway < frame.id)
    return this._rst(frame.id, constants.rst.REFUSED_STREAM);

  
  if (!state.isServer) {
    
    if (frame.id % 2 === 1 || frame.associated % 2 === 0)
      return this._rst(frame.id, constants.rst.PROTOCOL_ERROR);

    associated = state.streams[frame.associated];

    
    if (associated === undefined) {
      if (frame.type === 'RST_STREAM')
        return;
      this._rst(frame.id, constants.rst.INVALID_STREAM);
      return;
    }
  }

  var stream = new Stream(this, frame);
  this._addStream(stream);

  
  if (associated) {
    stream.associated = associated;
  }

  
  this.emit('stream', stream);
  stream._start(frame.url, frame.headers);

  return stream;
};






Connection.prototype._handleData = function handleData(stream, frame) {
  var state = this._spdyState;

  if (frame.data.length > 0) {
    if (stream._spdyState.closedBy.them) {
      stream._spdyState.rstCode = constants.rst.PROTOCOL_ERROR;
      stream.emit('error', 'Writing to half-closed stream');
    } else {
      
      state.windowSize -= frame.data.length;
      this._sendWindowUpdate();
      stream._recv(frame.data);
    }
  }
};






Connection.prototype._sendWindowUpdate = function sendWindowUpdate(force) {
  var state = this._spdyState;

  if (state.version < 3.1 && (!state.isServer || !state.autoSpdy31) ||
      state.windowSize > state.initialWindowSize / 2 && !force) {
    return;
  }

  var self = this;
  var delta = state.initialWindowSize - state.windowSize;
  if (delta === 0)
    return;

  state.windowSize += delta;
  state.framer.windowUpdateFrame(0, delta, function(err, frame) {
    if (err)
      return self.emit('error', err);
    self.write(frame);
  });
};





Connection.prototype._drainSink = function drainSink(delta) {
  var state = this._spdyState;

  
  if (state.version !== 3.1) {
    this._setVersion(3.1);
    this._sendWindowUpdate();
  }

  state.sinkSize += delta;

  if (state.sinkSize <= 0)
    return;

  this.emit('drain');

  
  Object.keys(state.streams).forEach(function(id) {
    state.streams[id]._drainSink(0);
  });
};






Connection.prototype._setVersion = function setVersion(version) {
  this._spdyState.parser.setVersion(version);
};





Connection.prototype._addStream = function addStream(stream) {
  var state = this._spdyState;
  var id = stream._spdyState.id;
  if (state.streams[id])
    return;
  state.streams[id] = stream;

  
  state.lastId = Math.max(state.lastId, id);

  var isClient = id % 2 == 1;
  if (isClient && state.isServer || !isClient && !state.isServer)
    state.streamCount++;
};





Connection.prototype._removeStream = function removeStream(stream) {
  var state = this._spdyState;
  var id = stream._spdyState.id;
  if (!state.streams[id])
    return;

  delete state.streams[id];

  var isClient = id % 2 == 1;
  if (isClient && state.isServer || !isClient && !state.isServer)
    state.streamCount--;

  if (!state.isServer &&
      state.goaway &&
      state.streamCount === 0 &&
      this.socket) {
    this.socket.destroySoon();
  }
};






Connection.prototype._destroyStreams = function destroyStreams(err) {
  var state = this._spdyState;
  var streams = state.streams;
  state.streams = {};
  state.streamCount = 0;
  Object.keys(streams).forEach(function(id) {
    streams[id].destroy();
  });
};








Connection.prototype._rst = function rst(streamId, code, extra) {
  var self = this;
  this._spdyState.framer.rstFrame(streamId, code, extra, function(err, frame) {
    if (err)
      return self.emit('error', err);
    self.write(frame);
  });
};






Connection.prototype._lock = function lock(callback) {
  if (!callback)
    return;

  var state = this._spdyState;
  if (state.locked) {
    state.lockQueue.push(callback);
  } else {
    state.locked = true;
    callback(null);
  }
};





Connection.prototype._unlock = function unlock() {
  var state = this._spdyState;
  if (state.locked) {
    if (state.lockQueue.length) {
      var cb = state.lockQueue.shift();
      cb(null);
    } else {
      state.locked = false;
    }
  }
};







Connection.prototype.write = function write(data, encoding) {
  if (this.socket.writable) {
    var wroteThrough = this.socket.write(data, encoding);
    
    
    if (!wroteThrough)
      this._spdyState.socketBuffering = true;
    return wroteThrough;
  }
};







Connection.prototype._setDefaultWindow = function _setDefaultWindow(settings) {
  if (!settings)
    return;
  if (!settings.initial_window_size ||
      settings.initial_window_size.persisted) {
    return;
  }
  var state = this._spdyState;
  state.initialSinkSize = settings.initial_window_size.value;

  Object.keys(state.streams).forEach(function(id) {
    state.streams[id]._updateSinkSize(settings.initial_window_size.value);
  });
};





Connection.prototype._handlePing = function handlePing(id) {
  var self = this;
  var state = this._spdyState;

  var ours = state.isServer && (id % 2 === 0) ||
             !state.isServer && (id % 2 === 1);

  
  if (!ours) {
    state.framer.pingFrame(id, function(err, frame) {
      if (err)
        return self.emit('error', err);
      self.write(frame);
    });
    return;
  }

  
  if (!state.pings[id])
    return;
  var ping = state.pings[id];
  delete state.pings[id];

  if (ping.cb)
    ping.cb(null);
};






Connection.prototype.ping = function ping(callback) {
  var self = this;
  var state = this._spdyState;
  var id = state.pingId;

  state.pingId += 2;

  state.framer.pingFrame(id, function(err, frame) {
    if (err)
      return self.emit('error', err);

    state.pings[id] = { cb: callback };
    self.write(frame);
  });
};






Connection.prototype.getCounter = function getCounter(name) {
  return this._spdyState.counters[name];
};





Connection.prototype.cork = function cork() {
  if (this.socket && this.socket.cork)
    this.socket.cork();
};





Connection.prototype.uncork = function uncork() {
  if (this.socket && this.socket.uncork)
    this.socket.uncork();
};

Connection.prototype.end = function end() {
  var self = this;
  var state = this._spdyState;

  state.framer.goawayFrame(state.lastId,
                           constants.goaway.OK,
                           function(err, frame) {
    if (err)
      return self.emit('error', err);

    self.write(frame, function() {
      state.goaway = state.lastId;

      
      if (!state.isServer &&
          state.goaway &&
          state.streamCount === 0 &&
          self.socket) {
        self.socket.destroySoon();
      }
    });
  });
};
