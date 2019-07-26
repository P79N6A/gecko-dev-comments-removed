var spdy = require('../spdy'),
    util = require('util'),
    https = require('https'),
    stream = require('stream'),
    Buffer = require('buffer').Buffer;

var crlf = new Buffer('\r\n');
var last_frag = new Buffer('0\r\n\r\n');

var legacy = !stream.Duplex;

if (legacy) {
  var DuplexStream = stream;
} else {
  var DuplexStream = stream.Duplex;
}






function instantiate(HTTPSServer) {
  
  
  
  
  
  
  function Server(options, requestListener) {
    
    this._init(HTTPSServer, options, requestListener);

    
    this._wrap();
  };
  util.inherits(Server, HTTPSServer);

  
  Object.keys(proto).forEach(function(key) {
    this[key] = proto[key];
  }, Server.prototype);

  return Server;
}
exports.instantiate = instantiate;


var proto = {};








proto._init = function _init(base, options, handler) {
  var state = {};
  this._spdyState = state;

  if (!options) options = {};
  if (!options.maxStreams) options.maxStreams = 100;
  if (!options.sinkSize) {
    options.sinkSize = 1 << 16;
  }
  if (!options.windowSize) {
    options.windowSize = 1 << 20; 
  }

  options.NPNProtocols = ['spdy/3', 'spdy/2', 'http/1.1', 'http/1.0'];
  state.options = options;
  state.reqHandler = handler;

  if (options.plain && !options.ssl) {
    base.call(this, handler);
  } else {
    base.call(this, options, handler);
  }

  
  if (!process.features.tls_npn && !options.debug && !options.plain) {
    return;
  }
};





proto._wrap = function _wrap() {
  var self = this,
      state = this._spdyState;

  
  var event = state.options.plain && !state.options.ssl ? 'connection' :
                                                          'secureConnection',
      handler = this.listeners(event)[0];

  state.pool = spdy.zlibpool.create();
  state.handler = handler;

  this.removeAllListeners(event);

  
  if (!state.options.plain) {
    return this.on(event, this._onConnection.bind(this));
  }

  
  
  this.on(event, function(socket) {
    var history = [],
        _emit = socket.emit;

    
    if (legacy) {
      function ondata() {};
      socket.once('data', ondata);
    }

    
    socket.setTimeout(self.timeout || 2 * 60 * 1000);

    socket.emit = function emit(event, data) {
      history.push(Array.prototype.slice.call(arguments));

      if (event === 'data') {
        
        onFirstByte.call(socket, data);
      } else if (event === 'readable') {
        
        onReadable.call(socket);
      } else if (event === 'end' ||
                 event === 'close' ||
                 event === 'error' ||
                 event === 'timeout') {
        
        fail();
      }
    };

    function fail() {
      socket.emit = _emit;
      history = null;
      try {
        socket.destroy();
      } catch (e) {
      }
    }

    function restore() {
      var copy = history.slice();
      history = null;

      if (legacy) socket.removeListener('data', ondata);
      socket.emit = _emit;
      for (var i = 0; i < copy.length; i++) {
        socket.emit.apply(socket, copy[i]);
        if (copy[i][0] === 'end') {
          if (socket.onend) socket.onend();
        }
      }
    }

    function onFirstByte(data) {
      
      if (data.length === 0) return;

      if (data[0] === 0x80) {
        self._onConnection(socket);
      } else  {
        handler.call(self, socket);
      }

      
      restore();

      
      
    };

    if (!legacy) {
      
      socket.on('readable', onReadable);
    }

    function onReadable() {
      var data = socket.read(1);

      
      if (!data) return;
      socket.removeListener('readable', onReadable);

      
      
      socket.emit = _emit;

      
      socket.unshift(data);

      if (data[0] === 0x80) {
        self._onConnection(socket);
      } else  {
        handler.call(self, socket);
      }

      
      restore();

      if (socket.ondata) {
        data = socket.read(socket._readableState.length);
        if (data) socket.ondata(data, 0, data.length);
      }
    }
  });
};






proto._onConnection = function _onConnection(socket) {
  var self = this,
      state = this._spdyState;

  
  if ((!socket.npnProtocol || !socket.npnProtocol.match(/spdy/)) &&
      !state.options.debug && !state.options.plain) {
    return state.handler.call(this, socket);
  }

  
  var connection = new Connection(socket, state.pool, state.options);

  
  connection.on('stream', state.handler);

  connection.on('connect', function onconnect(req, socket) {
    socket.streamID = req.streamID = req.socket.id;
    socket.isSpdy = req.isSpdy = true;
    socket.spdyVersion = req.spdyVersion = req.socket.version;

    socket.once('finish', function onfinish() {
      req.connection.end();
    });

    self.emit('connect', req, socket);
  });

  connection.on('request', function onrequest(req, res) {
    res._renderHeaders = spdy.response._renderHeaders;
    res.writeHead = spdy.response.writeHead;
    res.push = spdy.response.push;
    res.streamID = req.streamID = req.socket.id;
    res.spdyVersion = req.spdyVersion = req.socket.version;
    res.isSpdy = req.isSpdy = true;

    
    res.useChunkedEncodingByDefault = false;

    res.once('finish', function onfinish() {
      req.connection.end();
    });

    self.emit('request', req, res);
  });

  connection.on('error', function onerror(e) {
    console.log('[secureConnection] error ' + e);
    socket.destroy(e.errno === 'EPIPE' ? undefined : e);
  });
};


var Server = instantiate(https.Server);
exports.Server = Server;








exports.create = function create(base, options, requestListener) {
  var server;
  if (typeof base === 'function') {
    server = instantiate(base);
  } else {
    server = Server;

    requestListener = options;
    options = base;
    base = null;
  }

  return new server(options, requestListener);
};








function Connection(socket, pool, options) {
  process.EventEmitter.call(this);

  var self = this;

  this._closed = false;

  this.pool = pool;
  var pair = null;

  this._deflate = null;
  this._inflate = null;

  this.encrypted = socket.encrypted;

  
  this.streams = {};
  this.streamsCount = 0;
  this.pushId = 0;
  this._goaway = false;

  this._framer = null;

  
  this.windowSize = options.windowSize;
  this.sinkSize = options.sinkSize;

  
  this.scheduler = spdy.scheduler.create(this);

  
  this.socket = socket;

  
  this.parser = spdy.parser.create(this);
  this.parser.on('frame', function (frame) {
    if (this._closed) return;

    var stream;

    
    if (frame.type === 'SYN_STREAM') {
      self.streamsCount++;

      stream = self.streams[frame.id] = new Stream(self, frame);

      
      if (self.streamsCount > options.maxStreams) {
        stream.once('error', function onerror() {});
        
        stream._rstCode = 3;
        stream.destroy(true);
      } else {
        self.emit('stream', stream);

        stream._init();
      }
    } else {
      if (frame.id) {
        
        stream = self.streams[frame.id];

        
        if (stream === undefined) {
          if (frame.type === 'RST_STREAM') return;
          self.write(self._framer.rstFrame(frame.id, 2));
          return;
        }
      }

      
      if (frame.type === 'DATA') {
        if (frame.data.length > 0){
          if (stream._closedBy.client) {
            stream._rstCode = 2;
            stream.emit('error', 'Writing to half-closed stream');
          } else {
            stream._recv(frame.data);
          }
        }
      
      } else if (frame.type === 'RST_STREAM') {
        stream._rstCode = 0;
        if (frame.status === 5) {
          
          
          stream.pushes.forEach(function(stream) {
            stream.close();
          });
          stream.close();
        } else {
          
          stream.destroy(new Error('Received rst: ' + frame.status));
        }
      
      } else if (frame.type === 'PING') {
        self.write(self._framer.pingFrame(frame.pingId));
      } else if (frame.type === 'SETTINGS') {
        self._setDefaultWindow(frame.settings);
      } else if (frame.type === 'GOAWAY') {
        self._goaway = frame.lastId;
      } else if (frame.type === 'WINDOW_UPDATE') {
        stream._drainSink(frame.delta);
      } else {
        console.error('Unknown type: ', frame.type);
      }
    }

    
    if (frame.fin) {
      
      if (stream._closedBy.client) {
        stream._rstCode = 2;
        stream.emit('error', 'Already half-closed');
      } else {
        stream._closedBy.client = true;

        
        if (stream._forceChunked) {
          stream._recv(last_frag, true);
        }

        stream._handleClose();
      }
    }
  });

  this.parser.on('version', function onversion(version) {
    if (!pair) {
      pair = pool.get('spdy/' + version);
      self._deflate = pair.deflate;
      self._inflate = pair.inflate;
    }
  });

  this.parser.on('framer', function onframer(framer) {
    
    self.write(framer.settingsFrame(options));
  });

  
  this.parser.on('error', function onParserError(err) {
    self.emit('error', err);
  });

  socket.pipe(this.parser);

  
  socket.setTimeout(2 * 60 * 1000);
  socket.once('timeout', function ontimeout() {
    socket.destroy();
  });

  
  socket.on('error', function onSocketError(e) {
    self.emit('error', e);
  });

  socket.once('close', function onclose() {
    self._closed = true;
    if (pair) pool.put(pair);
  });

  if (legacy) {
    socket.on('drain', function ondrain() {
      self.emit('drain');
    });
  }
}
util.inherits(Connection, process.EventEmitter);
exports.Connection = Connection;







Connection.prototype.write = function write(data, encoding) {
  if (this.socket.writable) {
    return this.socket.write(data, encoding);
  }
};







Connection.prototype._setDefaultWindow = function _setDefaultWindow(settings) {
  if (!settings) return;
  if (!settings.initial_window_size ||
      settings.initial_window_size.persisted) {
    return;
  }

  this.sinkSize = settings.initial_window_size.value;

  Object.keys(this.streams).forEach(function(id) {
    this.streams[id]._updateSinkSize(settings.initial_window_size.value);
  }, this);
};







function Stream(connection, frame) {
  DuplexStream.call(this);

  this.connection = connection;
  this.socket = connection.socket;
  this.encrypted = connection.encrypted;
  this._framer = connection._framer;
  this._initialized = false;

  
  this._forceChunked = false;

  this.ondata = this.onend = null;

  
  this._rstCode = 1;
  this._destroyed = false;

  this._closedBy = {
    client: false,
    server: false
  };

  
  this._locked = false;
  this._lockBuffer = [];

  
  this.id = frame.id;
  this.version = frame.version;

  
  this.priority = frame.priority;

  
  this.pushes = [];

  
  this._sinkSize = connection.sinkSize;
  this._initialSinkSize = connection.sinkSize;

  
  
  this._sinkBuffer = [];

  
  this._initialWindowSize = connection.windowSize;
  this._windowSize = connection.windowSize;

  
  this._deflate = connection._deflate;
  this._inflate = connection._inflate;

  
  this.headers = frame.headers;
  this.url = frame.url;

  this._frame = frame;

  if (legacy) {
    this.readable = this.writable = true;
  }

  
  this.once('end', function() {
    var self = this;
    process.nextTick(function() {
      if (self.onend) self.onend();
    });
  });

  
  this.once('finish', function() {
    this._writeData(true, []);
    this._closedBy.server = true;
    if (this._sinkBuffer.length !== 0) return;
    this._handleClose();
  });
};
util.inherits(Stream, DuplexStream);
exports.Stream = Stream;

if (legacy) {
  Stream.prototype.pause = function pause() {};
  Stream.prototype.resume = function resume() {};
}





Stream.prototype._isGoaway = function _isGoaway() {
  return this.connection._goaway && this.id > this.connection._goaway;
};





Stream.prototype._init = function init() {
  var headers = this.headers,
      req = [headers.method + ' ' + this.url + ' ' + headers.version];

  Object.keys(headers).forEach(function (key) {
    if (key !== 'method' && key !== 'url' && key !== 'version' &&
        key !== 'scheme') {
      req.push(key + ': ' + headers[key]);
    }
  });

  
  if (!headers['content-length'] && !headers['transfer-encoding']) {
    req.push('Transfer-Encoding: chunked');
    this._forceChunked = true;
  }

  
  req.push('', '');

  req = new Buffer(req.join('\r\n'));

  this._recv(req, true);
  this._initialized = true;
};






Stream.prototype._lock = function lock(callback) {
  if (!callback) return;

  if (this._locked) {
    this._lockBuffer.push(callback);
  } else {
    this._locked = true;
    callback.call(this, null);
  }
};





Stream.prototype._unlock = function unlock() {
  if (this._locked) {
    this._locked = false;
    this._lock(this._lockBuffer.shift());
  }
};





Stream.prototype.setTimeout = function setTimeout(time) {};





Stream.prototype._handleClose = function _handleClose() {
  if (this._closedBy.client && this._closedBy.server) {
    this.close();
  }
};





Stream.prototype.close = function close() {
  this.destroy();
};






Stream.prototype.destroy = function destroy(error) {
  if (this._destroyed) return;
  this._destroyed = true;

  delete this.connection.streams[this.id];
  if (this.id % 2 === 1) {
    this.connection.streamsCount--;
  }

  
  
  if (error || !this._closedBy.server) {
    
    if (!this._closedBy.server) this._rstCode = 3;

    if (this._rstCode) {
      this._lock(function() {
        this.connection.scheduler.schedule(
          this,
          this._framer.rstFrame(this.id, this._rstCode));
        this.connection.scheduler.tick();

        this._unlock();
      });
    }
  }

  if (legacy) {
    this.emit('end');
  } else {
    this.push(null);
  }

  if (error) this.emit('error', error);

  var self = this;
  process.nextTick(function() {
    self.emit('close', !!error);
  });
};

Stream.prototype.destroySoon = function destroySoon(error) {
  return this.destroy(error);
};

Stream.prototype._drainSink = function _drainSink(size) {
  var oldBuffer = this._sinkBuffer;
  this._sinkBuffer = [];

  this._sinkSize += size;

  for (var i = 0; i < oldBuffer.length; i++) {
    this._writeData(oldBuffer[i][0], oldBuffer[i][1], oldBuffer[i][2]);
  }

  
  if (this._sinkBuffer.length === 0 && this._closedBy.server) {
    this._handleClose();
  }

  if (legacy) this.emit('drain');
};








Stream.prototype._writeData = function _writeData(fin, buffer, cb) {
  if (this._framer.version === 3) {
    
    if (this._sinkSize <= 0) {
      this._sinkBuffer.push([fin, buffer, cb]);
      return false;
    }

    var len = Math.min(this._sinkSize, buffer.length);
    this._sinkSize -= len;

    
    if (len < buffer.length) {
      this._sinkBuffer.push([fin, buffer.slice(len)]);
      buffer = buffer.slice(0, len);
      fin = false;
    }
  }

  this._lock(function() {
    var stream = this,
        frame = this._framer.dataFrame(this.id, fin, buffer);

    stream.connection.scheduler.schedule(stream, frame);
    stream.connection.scheduler.tick();

    this._unlock();

    if (cb) cb();
  });

  return true;
};







Stream.prototype._write = function write(data, encoding, cb) {
  
  if (this._isGoaway()) {
    if (cb) cb();
    return false;
  }

  return this._writeData(false, data, cb);
};

if (legacy) {
  Stream.prototype.write = function write(data, encoding, cb) {
    if (!Buffer.isBuffer(data)) {
      return this._write(new Buffer(data, encoding), null, cb);
    } else {
      return this._write(data, encoding, cb);
    }
  };

  
  
  
  
  
  
  Stream.prototype.end = function end(data, encoding) {
    
    if (this._isGoaway()) return;

    if (data) this.write(data, encoding);
    this.emit('finish');
  };
}







Stream.prototype._recv = function _recv(data, chunked) {
  
  if (!chunked && this._framer.version >= 3 && this._initialized) {
    this._windowSize -= data.length;

    if (this._windowSize <= 0) {
      var delta = this._initialWindowSize - this._windowSize;
      this._windowSize += delta;
      this.connection.write(this._framer.windowUpdateFrame(this.id, delta));
    }
  }

  
  if (this._forceChunked && !chunked) {
    
    if (data.length === 0) return;

    this._recv(new Buffer(data.length.toString(16)), true);
    this._recv(crlf, true);
    this._recv(data, true);
    this._recv(crlf, true);
    return;
  }

  if (legacy) {
    var self = this;
    process.nextTick(function() {
      self.emit('data', data);
      if (self.ondata) {
        self.ondata(data, 0, data.length);
      }
    });
  } else {
    
    if (this.ondata) {
      this.ondata(data, 0, data.length);
    } else {
      this.push(data);
    }
  }
};






Stream.prototype._read = function read(bytes) {
  
};






Stream.prototype._updateSinkSize = function _updateSinkSize(size) {
  var diff = size - this._initialSinkSize;

  this._initialSinkSize = size;
  this._drainSink(diff);
};





Stream.prototype.address = function address() {
  return this.socket && this.socket.address();
};

Stream.prototype.__defineGetter__('remoteAddress', function remoteAddress() {
  return this.socket && this.socket.remoteAddress;
});

Stream.prototype.__defineGetter__('remotePort', function remotePort() {
  return this.socket && this.socket.remotePort;
});
