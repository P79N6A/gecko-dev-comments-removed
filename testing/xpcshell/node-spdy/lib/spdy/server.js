var spdy = require('../spdy'),
    util = require('util'),
    https = require('https'),
    stream = require('stream'),
    Buffer = require('buffer').Buffer;






function instantiate(HTTPSServer) {
  
  
  
  
  
  
  function Server(options, requestListener) {
    if (!options) options = {};
    if (!options.maxStreams) options.maxStreams = 100;
    options.NPNProtocols = ['spdy/2', 'http/1.1', 'http/1.0'];

    HTTPSServer.call(this, options, requestListener);

    
    if (!process.features.tls_npn && !options.debug) return;

    
    var self = this,
        connectionHandler = this.listeners('secureConnection')[0];

    var pool = spdy.zlibpool.create();

    this.removeAllListeners('secureConnection');
    this.on('secureConnection', function secureConnection(socket) {
      
      if (socket.npnProtocol !== 'spdy/2' && !options.debug) {
        return connectionHandler.call(this, socket);
      }

      
      var connection = new Connection(socket, pool, options);

      
      connection.on('stream', connectionHandler);

      connection.on('request', function(req, res) {
        res._renderHeaders = spdy.response._renderHeaders;
        res.writeHead = spdy.response.writeHead;
        res.push = spdy.response.push;
        res.streamID = req.streamID = req.socket.id;
        res.isSpdy = req.isSpdy = true;

        res.on('finish', function() {
          req.connection.end();
        });
        self.emit('request', req, res);
      });

      connection.on('error', function(e) {
        socket.destroy(e.errno === 'EPIPE' ? undefined : e);
      });
    });
  }
  util.inherits(Server, HTTPSServer);

  return Server;
}
exports.instantiate = instantiate;


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
  var pair = pool.get();

  this.deflate = pair.deflate;
  this.inflate = pair.inflate;

  
  this.streams = {};
  this.streamsCount = 0;
  this.pushId = 0;
  this.goaway = false;

  this.framer = null;

  
  this.parser = spdy.parser.create(this, this.deflate, this.inflate);
  this.parser.on('frame', function (frame) {
    if (this._closed) return;

    var stream;

    
    if (frame.type === 'SYN_STREAM') {
      self.streamsCount++;

      stream = self.streams[frame.id] = new Stream(self, frame);

      
      if (self.streamsCount > options.maxStreams) {
        stream.once('error', function() {});
        
        stream.rstCode = 3;
        stream.destroy(true);
      } else {
        self.emit('stream', stream);

        stream.init();
      }
    } else {
      if (frame.id) {
        
        stream = self.streams[frame.id];

        
        if (stream === undefined) {
          if (frame.type === 'RST_STREAM') return;
          return self.emit('error', 'Stream ' + frame.id + ' not found');
        }
      }

      
      if (frame.type === 'DATA') {
        if (frame.data.length > 0){
          if (stream.closedBy.client) {
            stream.rstCode = 2;
            stream.emit('error', 'Writing to half-closed stream');
          } else {
            stream._read(frame.data);
          }
        }
      
      } else if (frame.type === 'RST_STREAM') {
        stream.rstCode = 0;
        if (frame.status === 5) {
          
          
          stream.pushes.forEach(function(stream) {
            stream.close();
          });
          stream.close();
        } else {
          
          stream.destroy(new Error('Received rst: ' + frame.status));
        }
      
      } else if (frame.type === 'PING') {
        self.write(self.framer.pingFrame(frame.pingId));
      
      } else if (frame.type === 'SETTINGS' || frame.type === 'NOOP') {
        
      } else if (frame.type === 'GOAWAY') {
        self.goaway = frame.lastId;
      } else {
        console.error('Unknown type: ', frame.type);
      }
    }

    
    if (frame.fin) {
      
      if (stream.closedBy.client) {
        stream.rstCode = 2;
        stream.emit('error', 'Already half-closed');
      } else {
        stream.closedBy.client = true;
        stream.handleClose();
      }
    }
  });

  this.parser.on('_framer', function(framer) {
    
    self.write(
      framer.maxStreamsFrame(options.maxStreams)
    );
  });

  
  this.scheduler = spdy.scheduler.create(this);

  
  this.socket = socket;

  socket.pipe(this.parser);

  
  socket.setTimeout(2 * 60 * 1000);
  socket.once('timeout', function() {
    socket.destroy();
  });

  
  socket.on('error', function(e) {
    self.emit('error', e);
  });

  socket.on('close', function() {
    self._closed = true;
    pool.put(pair);
  });

  socket.on('drain', function () {
    self.emit('drain');
  });
}
util.inherits(Connection, process.EventEmitter);
exports.Connection = Connection;







Connection.prototype.write = function write(data, encoding) {
  if (this.socket.writable) {
    return this.socket.write(data, encoding);
  }
};







function Stream(connection, frame) {
  var self = this;
  stream.Stream.call(this);

  this.connection = connection;
  this.framer = connection.framer;

  this.ondata = this.onend = null;

  
  this.rstCode = 1;
  this._destroyed = false;

  this.closedBy = {
    client: false,
    server: false
  };

  
  this.locked = false;
  this.lockBuffer = [];

  
  this.id = frame.id;

  
  this.priority = frame.priority;

  
  this.pushes = [];

  this._paused = false;
  this._buffer = [];

  
  this.deflate = connection.deflate;
  this.inflate = connection.inflate;

  
  this.headers = frame.headers;

  this.frame = frame;

  this.readable = this.writable = true;
}
util.inherits(Stream, stream.Stream);
exports.Stream = Stream;





Stream.prototype.isGoaway = function isGoaway() {
  return this.connection.goaway && this.id > this.connection.goaway;
};





Stream.prototype.init = function init() {
  var headers = this.headers,
      req = [headers.method + ' ' + headers.url + ' ' + headers.version];

  Object.keys(headers).forEach(function (key) {
    if (key !== 'method' && key !== 'url' && key !== 'version' &&
        key !== 'scheme') {
      req.push(key + ': ' + headers[key]);
    }
  });

  
  req.push('', '');

  req = new Buffer(req.join('\r\n'));

  this._read(req);
};






Stream.prototype.lock = function lock(callback) {
  if (!callback) return;

  if (this.locked) {
    this.lockBuffer.push(callback);
  } else {
    this.locked = true;
    callback.call(this, null);
  }
};





Stream.prototype.unlock = function unlock() {
  if (this.locked) {
    this.locked = false;
    this.lock(this.lockBuffer.shift());
  }
};





Stream.prototype.setTimeout = function setTimeout(time) {};





Stream.prototype.handleClose = function handleClose() {
  if (this.closedBy.client && this.closedBy.server) {
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
  this.connection.streamsCount--;

  if (error) {
    if (this.rstCode) {
      this.connection.write(this.framer.rstFrame(this.id, this.rstCode));
    }
  }

  var self = this;
  process.nextTick(function() {
    if (error) self.emit('error', error);
    self.emit('close');
  });
};







Stream.prototype._writeData = function _writeData(fin, buffer) {
  this.lock(function() {
    var stream = this,
        frame = this.framer.dataFrame(this.id, fin, buffer);

    stream.connection.scheduler.schedule(stream, frame);
    stream.connection.scheduler.tick();

    if (fin) this.close();

    this.unlock();
  });
};







Stream.prototype.write = function write(data, encoding) {
  
  if (this.isGoaway()) return;

  var buffer;

  
  if (typeof data === 'string') {
    buffer = new Buffer(data, encoding);
  } else {
    buffer = data;
  }

  
  
  
  
  
  
  var MTU = 1300;

  
  if (buffer.length < MTU) {
    this._writeData(false, buffer);
  } else {
    var len = buffer.length - MTU;
    for (var offset = 0; offset < len; offset += MTU) {
      this._writeData(false, buffer.slice(offset, offset + MTU));
    }
    this._writeData(false, buffer.slice(offset));
  }
};







Stream.prototype.end = function end(data, encoding) {
  
  if (this.isGoaway()) return;

  if (data) this.write(data, encoding);

  this._writeData(true, []);
  this.closedBy.server = true;
  this.handleClose();
};





Stream.prototype.pause = function pause() {
  if (this._paused) return;
  this._paused = true;
};





Stream.prototype.resume = function resume() {
  if (!this._paused) return;
  this._paused = false;

  var self = this,
      buffer = this._buffer;

  this._buffer = [];

  process.nextTick(function() {
    buffer.forEach(function(chunk) {
      self._read(chunk);
    });
  });
};






Stream.prototype._read = function _read(data) {
  if (this._paused) {
    this._buffer.push(data);
    return;
  }

  var self = this;
  process.nextTick(function() {
    if (self.ondata) self.ondata(data, 0, data.length);
    self.emit('data', data);
  });
};
