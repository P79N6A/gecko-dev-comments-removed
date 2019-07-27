var spdy = require('../spdy');
var zlib = require('zlib');
var utils = spdy.utils;
var assert = require('assert');
var util = require('util');
var stream = require('stream');
var Buffer = require('buffer').Buffer;
var constants = spdy.protocol.constants;

if (!/^v(0\.10|0\.8|0\.9)\./.test(process.version))
  var httpCommon = require('_http_common');








function Stream(connection, options) {
  var self = this;

  utils.DuplexStream.call(this);

  this.connection = connection;
  this.socket = connection.socket;
  this.encrypted = connection.encrypted;
  this.associated = null;

  
  this._handle = {
    readStop: function() { self._readStop() },
    readStart: function() { self._readStart() }
  };

  var state = {};
  this._spdyState = state;
  state.timeout = connection.timeout;
  state.timeoutTimer = null;
  state.framer = connection._spdyState.framer;
  state.initialized = false;
  state.ending = false;
  state.ended = false;
  state.paused = false;
  state.finishAttached = false;
  state.decompress = options.decompress;
  state.decompressor = null;

  
  state.id = options.id;
  state.associated = options.associated;
  state.headers = options.headers || {};
  if (connection._spdyState.xForward !== null)
    state.headers['x-forwarded-for'] = connection._spdyState.xForward;

  
  state.isPush = !connection._spdyState.isServer && state.associated;
  connection._spdyState.counters.streamCount++;
  if (state.isPush)
    connection._spdyState.counters.pushCount++;

  
  state.scheme = state.headers.scheme;
  state.host = state.headers.host;

  if (state.isPush && state.headers['content-encoding']) {
    var compression = state.headers['content-encoding'];

    
    if (compression === 'gzip' || compression === 'deflate')
      this._setupDecompressor(compression);
  }

  
  state.chunkedWrite = false;

  
  state.options = options;
  state.isClient = !!options.client;
  state.parseRequest = !!options.client;

  
  state.rstCode = constants.rst.PROTOCOL_ERROR;
  state.destroyed = false;

  state.closedBy = {
    them: false,
    us: false
  };

  
  state.priority = options.priority;

  
  state.sinkSize = connection._spdyState.initialSinkSize;
  state.initialSinkSize = state.sinkSize;

  
  
  state.sinkBuffer = [];
  state.sinkDraining = false;

  
  state.windowSize = connection._spdyState.initialWindowSize;
  state.initialWindowSize = state.windowSize;

  this._init();
};
util.inherits(Stream, utils.DuplexStream);
exports.Stream = Stream;


Stream.prototype._parserHeadersComplete = function parserHeadersComplete() {
  if (this.parser)
    return this.parser.onHeadersComplete || this.parser[1];
};

Stream.prototype._parserBody = function parserBody() {
  if (this.parser)
    return this.parser.onBody || this.parser[2];
};

Stream.prototype._parserMessageComplete = function parserMessageComplete() {
  if (this.parser)
    return this.parser.onMessageComplete || this.parser[3];
};





Stream.prototype._init = function init() {
  var state = this._spdyState;

  this.setTimeout();
  this.ondata = this.onend = null;

  if (utils.isLegacy)
    this.readable = this.writable = true;

  
  this.once('end', function() {
    var self = this;
    utils.nextTick(function() {
      var onHeadersComplete = self._parserHeadersComplete();
      if (!onHeadersComplete && self.onend)
        self.onend();
    });
  });

  
  this.once('finish', function onfinish() {
    if (state.chunkedWrite)
      return this.once('_chunkDone', onfinish);

    var self = this;
    this._writeData(true, [], function() {
      state.closedBy.us = true;
      if (state.sinkBuffer.length !== 0)
        return;
      if (utils.isLegacy)
        self.emit('full-finish');
      self._handleClose();
    });
  });

  if (state.isClient) {
    var httpMessage;
    Object.defineProperty(this, '_httpMessage', {
      set: function(val) {
        if (val)
          this._attachToRequest(val);
        httpMessage = val;
      },
      get: function() {
        return httpMessage;
      },
      configurable: true,
      enumerable: true
    });
  }
};

Stream.prototype._readStop = function readStop() {
  this._spdyState.paused = true;
};

Stream.prototype._readStart = function readStart() {
  this._spdyState.paused = false;

  
  this._read();
};

if (utils.isLegacy) {
  Stream.prototype.pause = function pause() {
    this._readStop();
  };
  Stream.prototype.resume = function resume() {
    this._readStart();
  };
}





Stream.prototype._isGoaway = function _isGoaway() {
  return this.connection._spdyState.goaway &&
         this._spdyState.id > this.connection._spdyState.goaway;
};







Stream.prototype._start = function start(url, headers) {
  var state = this._spdyState;
  var headerList = [];

  
  Object.keys(headers).map(function(key) {
    if (key !== 'method' &&
        key !== 'url' &&
        key !== 'version' &&
        key !== 'scheme' &&
        key !== 'status' &&
        key !== 'path') {
      headerList.push(key, headers[key]);
    }
  });

  var info = {
    url: url,
    headers: headerList,
    versionMajor: 1,
    versionMinor: 1,
    method: httpCommon ? httpCommon.methods.indexOf(headers.method) :
                         headers.method,
    statusCode: false,
    upgrade: headers.method === 'CONNECT'
  };

  
  var onHeadersComplete = this._parserHeadersComplete();
  if (onHeadersComplete) {
    onHeadersComplete.call(this.parser, info);

    
    if (headers.method === 'CONNECT') {
      
      
      var req = this.parser.incoming;
      if (this.listeners('data').length) {
        
        this.removeAllListeners('data');
        this.skipBodyParsing = true;
      }
      this.connection.emit('connect', req, req.socket);
    }
  } else {
    this.emit('headersComplete', info);
  }

  state.initialized = true;
};






Stream.prototype._attachToRequest = function attachToRequest(res) {
  var self = this;

  res.addTrailers = function addTrailers(headers) {
    self.sendHeaders(headers);
  };

  this.on('headers', function(headers) {
    var req = res.parser.incoming;
    if (req) {
      Object.keys(headers).forEach(function(key) {
        req.trailers[key] = headers[key];
      });
      req.emit('trailers', headers);
    }
  });
};





Stream.prototype.sendHeaders = function sendHeaders(headers) {
  var self = this;
  var state = this._spdyState;

  
  this.setTimeout();

  var connection = this.connection;
  this._lock(function() {
    state.framer.headersFrame(state.id, headers, function(err, frame) {
      if (err) {
        self._unlock();
        return self.emit('error', err);
      }

      connection.write(frame);
      self._unlock();
    });
  });
};






Stream.prototype.setTimeout = function _setTimeout(timeout, callback) {
  var self = this;
  var state = this._spdyState;

  if (callback)
    this.once('timeout', callback);

  
  if (this.associated)
    this.associated.setTimeout();

  state.timeout = timeout !== undefined ? timeout : state.timeout;

  if (state.timeoutTimer) {
    clearTimeout(state.timeoutTimer);
    state.timeoutTimer = null;
  }

  if (!state.timeout)
    return;

  state.timeoutTimer = setTimeout(function() {
    self.emit('timeout');
  }, state.timeout);
};





Stream.prototype._handleClose = function _handleClose() {
  var state = this._spdyState;
  if (state.closedBy.them && state.closedBy.us)
    this.close();
};





Stream.prototype.close = function close() {
  this.destroy();
};






Stream.prototype.destroy = function destroy(error) {
  var state = this._spdyState;
  if (state.destroyed)
    return;
  state.destroyed = true;

  
  this.setTimeout(0);

  
  this.connection._spdyState.counters.streamCount--;
  if (state.isPush)
    this.connection._spdyState.counters.pushCount--;

  
  this.writable = false;
  this.connection._removeStream(this);

  
  
  if (error || !state.closedBy.us) {
    if (!state.closedBy.us)
      
      if (state.isClient)
        state.rstCode = constants.rst.CANCEL;
      
      else
        state.rstCode = constants.rst.REFUSED_STREAM;

    if (state.rstCode) {
      var self = this;
      state.framer.rstFrame(state.id,
                            state.rstCode,
                            null,
                            function(err, frame) {
        if (err)
          return self.emit('error', err);
        var scheduler = self.connection._spdyState.scheduler;

        scheduler.scheduleLast(self, frame);
        scheduler.tick();
      });
    }
  }

  var self = this;
  this._recvEnd(function() {
    if (error)
      self.emit('error', error);

    utils.nextTick(function() {
      self.emit('close', !!error);
    });
  }, true);
};






Stream.prototype.ping = function ping(callback) {
  return this.connection.ping(callback);
};




Stream.prototype.destroySoon = function destroySoon() {
  var self = this;
  var state = this._spdyState;

  
  this.writable = false;

  
  if (state.closedBy.us)
    return;

  
  this.setTimeout();
  this.end();
};






Stream.prototype._drainSink = function drainSink(size) {
  var state = this._spdyState;
  var oldBuffer = state.sinkBuffer;

  state.sinkBuffer = [];
  state.sinkSize += size;
  state.sinkDraining = true;

  for (var i = 0; i < oldBuffer.length; i++) {
    var item = oldBuffer[i];

    
    if (i === oldBuffer.length - 1)
      state.sinkDraining = false;
    this._writeData(item.fin, item.buffer, item.cb, item.chunked);
  }

  
  if (state.sinkBuffer.length === 0 && state.closedBy.us)
    this._handleClose();

  if (utils.isLegacy)
    this.emit('drain');
};









Stream.prototype._writeData = function _writeData(fin, buffer, cb, chunked) {
  
  if (!this.connection.socket || !this.connection.socket.writable)
    return false;

  var state = this._spdyState;
  if (!state.framer.version) {
    var self = this;
    state.framer.on('version', function() {
      self._writeData(fin, buffer, cb, chunked);
      if (utils.isLegacy)
        self.emit('drain');
    });
    return false;
  }

  
  if (state.closedBy.us) {
    this.emit('error', new Error('Write after end!'));
    return false;
  }

  
  
  if (this.connection._spdyState.socketBuffering) {
    state.sinkBuffer.push({
      fin: fin,
      buffer: buffer,
      cb: cb,
      chunked: chunked
    });
    return false;
  }

  if (state.framer.version >= 3) {
    
    if (state.sinkSize <= 0 ||
        (state.framer.version >= 3.1 &&
         this.connection._spdyState.sinkSize <= 0)) {
      state.sinkBuffer.push({
        fin: fin,
        buffer: buffer,
        cb: cb,
        chunked: chunked
      });
      return false;
    }
  }
  if (state.chunkedWrite && !chunked) {
    var self = this;
    function attach() {
      self.once('_chunkDone', function() {
        if (state.chunkedWrite)
          return attach();
        self._writeData(fin, buffer, cb, false);
      });
    }
    attach();
    return true;
  }

  
  if (state.ended && !chunked)
    return false;

  
  if (!fin && !chunked && state.ending) {
    if (utils.isLegacy)
      fin = this.listeners('_chunkDone').length === 0;
    else
      fin = this._writableState.length === 0;
    fin = fin && false && state.sinkBuffer.length === 0 && !state.sinkDraining;
  }
  if (!chunked && fin)
    state.ended = true;

  var maxChunk = this.connection._spdyState.maxChunk;
  
  if (maxChunk && maxChunk < buffer.length) {
    var preend = buffer.length - maxChunk;
    var chunks = [];
    for (var i = 0; i < preend; i += maxChunk)
      chunks.push(buffer.slice(i, i + maxChunk));

    
    chunks.push(buffer.slice(i));

    var self = this;
    function send(err) {
      function done(err) {
        state.chunkedWrite = false;
        self.emit('_chunkDone');
        if (cb)
          cb(err);
      }

      if (err)
        return done(err);

      var chunk = chunks.shift();
      if (chunks.length === 0) {
        self._writeData(fin, chunk, function(err) {
          
          done(err);
        }, true);
      } else {
        self._writeData(false, chunk, send, true);
      }
    }

    state.chunkedWrite = true;
    send();
    return true;
  }

  if (state.framer.version >= 3) {
    var len = Math.min(state.sinkSize, buffer.length);
    if (state.framer.version >= 3.1)
      len = Math.min(this.connection._spdyState.sinkSize, len);
    this.connection._spdyState.sinkSize -= len;
    state.sinkSize -= len;

    
    if (len < buffer.length) {
      state.sinkBuffer.push({
        fin: fin,
        buffer: buffer.slice(len),
        cb: cb,
        chunked: chunked
      });
      buffer = buffer.slice(0, len);
      fin = false;
      cb = null;
    }
  }

  
  this.setTimeout();

  this._lock(function() {
    var stream = this;

    state.framer.dataFrame(state.id, fin, buffer, function(err, frame) {
      if (err) {
        stream._unlock();
        return stream.emit('error', err);
      }

      var scheduler = stream.connection._spdyState.scheduler;
      scheduler.schedule(stream, frame);
      scheduler.tick(cb);

      stream._unlock();
    });
  });

  return true;
};







Stream.prototype._parseClientRequest = function parseClientRequest(data, cb) {
  var state = this._spdyState;

  state.parseRequest = false;

  var lines = data.toString().split(/\r\n\r\n/);
  var body = data.slice(Buffer.byteLength(lines[0]) + 4);
  lines = lines[0].split(/\r\n/g);
  var status = lines[0].match(/^([a-z]+)\s([^\s]+)\s(.*)$/i);
  var headers = {};

  assert(status !== null);
  var method = status[1].toUpperCase();
  var url = status[2];
  var version = status[3].toUpperCase();
  var host = '';

  
  lines.slice(1).forEach(function(line) {
    
    if (!line)
      return;

    
    var match = line.match(/^(.*?):\s*(.*)$/);
    assert(match !== null);

    var key = match[1].toLowerCase();
    var value = match[2];

    if (key === 'host')
      host = value;
    else if (key !== 'connection')
      headers[key] = value;
  }, this);

  
  assert(this._httpMessage);
  var chunkedEncoding = this._httpMessage.chunkedEncoding;
  this._httpMessage.chunkedEncoding = false;

  
  this.setTimeout();

  var self = this;
  var connection = this.connection;
  this._lock(function() {
    state.framer.streamFrame(state.id, 0, {
      method: method,
      host: host,
      url: url,
      version: version,
      priority: self.priority
    }, headers, function(err, frame) {
      if (err) {
        self._unlock();
        return self.emit('error', err);
      }
      connection.write(frame);
      self._unlock();
      connection._addStream(self);

      self.emit('_spdyRequest');
      state.initialized = true;
      if (cb)
        cb();
    })
  });

  
  if (body) {
    if (chunkedEncoding) {
      var i = 0;
      while (i < body.length) {
        var lenStart = i;

        
        for (; i + 1 < body.length; i++)
          if (body[i] === 0xd && body[i + 1] === 0xa)
            break;
        if (i === body.length - 1)
          return self.emit('error', new Error('Incorrect chunk length'));

        
        var len = parseInt(body.slice(lenStart, i).toString(), 16);

        
        if (i + 2 + len >= body.length)
          return self.emit('error', new Error('Chunk length OOB'));

        
        if (len !== 0) {
          var chunk = body.slice(i + 2, i + 2 + len);
          this._write(chunk, null, null);
        }

        
        i += 4 + len;
      }
    }
  }

  return true;
};






Stream.prototype._handleResponse = function handleResponse(frame) {
  var state = this._spdyState;
  assert(state.isClient);

  var headers = frame.headers;
  var headerList = [];
  var compression = null;

  
  Object.keys(headers).map(function(key) {
    var val = headers[key];

    if (state.decompress &&
        key === 'content-encoding' &&
        (val === 'gzip' || val === 'deflate'))
      compression = val;
    else if (key !== 'status' && key !== 'version')
      headerList.push(key, headers[key]);
  });

  
  if (compression)
    this._setupDecompressor(compression);

  var isConnectRequest = this._httpMessage &&
                         this._httpMessage.method === 'CONNECT';

  var info = {
    url: '',
    headers: headerList,
    versionMajor: 1,
    versionMinor: 1,
    method: false,
    statusCode: parseInt(headers.status, 10),
    statusMessage: headers.status.split(/ /)[1],
    upgrade: isConnectRequest
  };

  
  var onHeadersComplete = this._parserHeadersComplete();
  if (onHeadersComplete) {
    onHeadersComplete.call(this.parser, info);

    
    if (isConnectRequest) {
      var req = this._httpMessage;
      var res = this.parser.incoming;
      req.res = res;
      if (this.listeners('data').length) {
        
        this.removeAllListeners('data');
        this.skipBodyParsing = true;
      }
      if (this._httpMessage.listeners('connect').length > 0)
        this._httpMessage.emit('connect', res, res.socket);
      else
        this.destroy();
    }
  } else {
    this.emit('headersComplete', info);
  }

  state.initialized = true;
};






Stream.prototype._setupDecompressor = function setupDecompressor(type) {
  var self = this;
  var state = this._spdyState;
  var options = { flush: zlib.Z_SYNC_FLUSH };

  if (state.decompressor !== null)
    return this.emit('error', new Error('Decompressor already created'));

  state.decompressor = type === 'gzip' ? zlib.createGunzip(options) :
                                         zlib.createInflate(options);
  if (spdy.utils.isLegacy)
    state.decompressor._flush = options.flush;
  state.decompressor.on('data', function(chunk) {
    self._recv(chunk, true);
  });
  state.decompressor.on('error', function(err) {
    self.emit('error', err);
  });
}






Stream.prototype._decompress = function decompress(data) {
  var state = this._spdyState;

  
  state.decompressor.write(data);
};







Stream.prototype._write = function write(data, encoding, cb) {
  var r = true;
  var state = this._spdyState;

  
  if (state.isClient && state.associated) {
    cb();
    return r;
  }

  
  if (state.parseRequest) {
    this._parseClientRequest(data, cb);
  } else {
    
    assert(!this._httpMessage || !this._httpMessage.chunkedEncoding);

    
    if (this._isGoaway()) {
      if (cb)
        cb();
      r = false;
    } else {
      r = this._writeData(false, data, cb);
    }
  }

  if (this._httpMessage && state.isClient && !state.finishAttached) {
    state.finishAttached = true;
    var self = this;

    
    this._httpMessage.once('finish', function() {
      if (self._httpMessage.output &&
          !self._httpMessage.output.length  &&
          self._httpMessage.method !== 'CONNECT')
        self.end();
    });
  }

  return r;
};

if (spdy.utils.isLegacy) {
  Stream.prototype.write = function write(data, encoding, cb) {
    if (typeof encoding === 'function' && !cb) {
      cb = encoding;
      encoding = null;
    }
    if (!Buffer.isBuffer(data))
      return this._write(new Buffer(data, encoding), null, cb);
    else
      return this._write(data, encoding, cb);
  };

  
  
  
  
  
  
  
  Stream.prototype.end = function end(data, encoding, cb) {
    
    if (this._isGoaway())
      return;

    this._spdyState.ending = true;

    if (data)
      this.write(data, encoding, cb);
    this.emit('finish');
  };
} else {

  
  
  
  
  
  
  
  Stream.prototype.end = function end(data, encoding, cb) {
    this._spdyState.ending = true;

    Stream.super_.prototype.end.call(this, data, encoding, cb);
  };
}







Stream.prototype._recv = function _recv(data, decompressed) {
  var state = this._spdyState;

  
  if (state.framer.version >= 3 && state.initialized) {
    state.windowSize -= data.length;

    
    if (spdy.utils.isLegacy && !state.paused)
      this._read();
  }

  
  if (state.decompressor && !decompressed)
    return this._decompress(data);

  
  this.setTimeout();

  if (this.parser && !this.skipBodyParsing) {
    var onBody = this._parserBody();
    if (onBody)
      onBody.call(this.parser, data, 0, data.length);
  }

  if (spdy.utils.isLegacy) {
    var self = this;
    utils.nextTick(function() {
      self.emit('data', data);
      if (self.ondata && !onBody)
        self.ondata(data, 0, data.length);
    });
  } else {
    
    if (!onBody || !this.parser[2]) {
      
      if (this.ondata && !onBody)
        this.ondata(data, 0, data.length);
      else
        this.push(data);
    }
  }

  
  if (!spdy.utils.isLegacy)
    this.read(0);
};







Stream.prototype._recvEnd = function _recvEnd(callback, quite) {
  var state = this._spdyState;

  
  if (state.decompressor) {
    var self = this;
    state.decompressor.write('', function() {
      state.decompressor = null;
      self._recvEnd(callback, quite);
    });
    return;
  }
  if (!quite) {
    var onMessageComplete = this._parserMessageComplete();
    if (onMessageComplete)
      onMessageComplete.call(this.parser);

    if (spdy.utils.isLegacy)
      this.emit('end');
    else
      this.push(null);
  }
  if (callback)
    callback();
};






Stream.prototype._read = function read(bytes) {
  var state = this._spdyState;

  
  if (state.framer.version >= 3 &&
      state.initialized &&
      state.windowSize <= state.initialWindowSize / 2) {
    var delta = state.initialWindowSize - state.windowSize;
    state.windowSize += delta;
    var self = this;
    state.framer.windowUpdateFrame(state.id, delta, function(err, frame) {
      if (err)
        return self.emit('error', err);
      self.connection.write(frame);
    });
  }

  if (!spdy.utils.isLegacy)
    this.push('');
};






Stream.prototype._updateSinkSize = function _updateSinkSize(size) {
  var state = this._spdyState;
  var delta = size - state.initialSinkSize;

  state.initialSinkSize = size;
  this._drainSink(delta);
};






Stream.prototype._lock = function lock(callback) {
  if (!callback)
    return;

  var self = this;
  this.connection._lock(function(err) {
    callback.call(self, err);
  });
};





Stream.prototype._unlock = function unlock() {
  this.connection._unlock();
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

Stream.prototype.setNoDelay = function setNoDelay(enable) {
  return this.socket && this.socket.setNoDelay(enable);
};

Stream.prototype.setKeepAlive = function(setting, msecs) {
  return this.socket && this.socket.setKeepAlive(setting, msecs);
};

Stream.prototype.getPeerCertificate = function() {
  return this.socket && this.socket.getPeerCertificate();
};

Stream.prototype.getSession = function() {
  return this.socket && this.socket.getSession();
};

Stream.prototype.isSessionReused = function() {
  return this.socket && this.socket.isSessionReused();
};

Stream.prototype.getCipher = function() {
  return this.socket && this.socket.getCipher();
};
