var spdy = require('../spdy'),
    util = require('util'),
    https = require('https');

var Connection = spdy.Connection;






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

  if (!options)
    options = {};

  
  options = util._extend({}, options);

  var supportedProtocols = [
    'spdy/3.1', 'spdy/3', 'spdy/2',
    'http/1.1', 'http/1.0'
  ];
  options.NPNProtocols = supportedProtocols;
  options.ALPNProtocols = supportedProtocols;
  options.isServer = true;

  
  
  
  if (options.headerCompression !== false)
    options.headerCompression = true;

  state.options = options;
  state.reqHandler = handler;

  if (options.plain && !options.ssl)
    base.call(this, handler);
  else
    base.call(this, options, handler);

  
  if (!process.features.tls_npn && !process.features.tls_alpn &&
      !options.debug && !options.plain)
    return;
};





proto._wrap = function _wrap() {
  var self = this,
      state = this._spdyState;

  
  var event = state.options.plain && !state.options.ssl ? 'connection' :
                                                          'secureConnection',
      handler = this.listeners(event)[0];

  state.handler = handler;

  this.removeAllListeners(event);

  
  if (state.options.timeout !== undefined)
    this.timeout = state.options.timeout;
  else
    this.timeout = this.timeout || 2 * 60 * 1000;

  
  if (!state.options.plain)
    return this.on(event, this._onConnection.bind(this));

  
  
  this.on(event, function(socket) {
    var history = [],
        _emit = socket.emit;

    
    if (spdy.utils.isLegacy) {
      function ondata() {};
      socket.once('data', ondata);
    }

    
    socket.setTimeout(self.timeout);

    socket.emit = function emit(event, data) {
      history.push(Array.prototype.slice.call(arguments));

      if (event === 'data') {
        
        if (spdy.utils.isLegacy)
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
      socket.removeListener('readable', onReadable);

      try {
        socket.destroy();
      } catch (e) {
      }
    }

    function restore() {
      var copy = history.slice();
      history = null;

      socket.removeListener('readable', onReadable);
      if (spdy.utils.isLegacy)
        socket.removeListener('data', ondata);
      socket.emit = _emit;
      for (var i = 0; i < copy.length; i++) {
        if (copy[i][0] !== 'data' || spdy.utils.isLegacy)
          socket.emit.apply(socket, copy[i]);
        if (copy[i][0] === 'end' && socket.onend)
          socket.onend();
      }
    }

    function onFirstByte(data) {
      
      if (data.length === 0)
        return;

      if (data[0] === 0x80)
        self._onConnection(socket);
      else
        handler.call(self, socket);

      
      restore();

      
      
    };

    
    if (!spdy.utils.isLegacy)
      socket.on('readable', onReadable);

    function onReadable() {
      var data = socket.read(1);

      
      if (!data)
        return;
      socket.removeListener('readable', onReadable);

      
      
      socket.emit = _emit;

      
      socket.unshift(data);

      if (data[0] === 0x80)
        self._onConnection(socket);
      else
        handler.call(self, socket);

      
      restore();

      if (socket.ondata) {
        data = socket.read(socket._readableState.length);
        if (data)
          socket.ondata(data, 0, data.length);
      }
    }
  });
};






proto._onConnection = function _onConnection(socket) {
  var self = this,
      state = this._spdyState;

  
  var selectedProtocol = socket.npnProtocol || socket.alpnProtocol;
  if ((!selectedProtocol || !selectedProtocol.match(/spdy/)) &&
      !state.options.debug && !state.options.plain) {
    return state.handler.call(this, socket);
  }

  
  var connection = new Connection(socket, state.options, this);
  if (selectedProtocol === 'spdy/3.1')
    connection._setVersion(3.1);
  else if (selectedProtocol === 'spdy/3')
    connection._setVersion(3);
  else if (selectedProtocol === 'spdy/2')
    connection._setVersion(2);

  
  connection.on('stream', state.handler);

  connection.on('connect', function onconnect(req, socket) {
    socket.streamID = req.streamID = req.socket._spdyState.id;
    socket.isSpdy = req.isSpdy = true;
    socket.spdyVersion = req.spdyVersion = req.socket._spdyState.framer.version;

    socket.once('finish', function onfinish() {
      req.connection.end();
    });

    self.emit('connect', req, socket);
  });

  connection.on('request', function onrequest(req, res) {
    
    res._renderHeaders = spdy.response._renderHeaders;
    res.writeHead = spdy.response.writeHead;
    res.end = spdy.response.end;
    res.push = spdy.response.push;
    res.streamID = req.streamID = req.socket._spdyState.id;
    res.spdyVersion = req.spdyVersion = req.socket._spdyState.framer.version;
    res.isSpdy = req.isSpdy = true;
    res.addTrailers = function addTrailers(headers) {
      res.socket.sendHeaders(headers);
    };

    
    res._last = true;

    
    res.useChunkedEncodingByDefault = false;

    
    req.connection.on('headers', function(headers) {
      Object.keys(headers).forEach(function(key) {
        req.trailers[key] = headers[key];
      });
      req.emit('trailers', headers);
    });

    self.emit('request', req, res);
  });

  connection.on('error', function onerror(e) {
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

  
  if (!base && options && options.plain && options.ssl === false)
    return exports.create(require('http').Server, options, requestListener);

  return new server(options, requestListener);
};
