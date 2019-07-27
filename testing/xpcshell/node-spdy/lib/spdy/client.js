var spdy = require('../spdy');
var assert = require('assert');
var util = require('util');
var net = require('net');
var https = require('https');
var EventEmitter = require('events').EventEmitter;

var proto = {};

function instantiate(base) {
  function Agent(options) {
    base.call(this, options);

    if (!this.options.spdy)
      this.options.spdy = {};

    this._init();

    
    this.createConnection = proto.createConnection;

    
    this.keepAlive = false;
  };
  util.inherits(Agent, base);

  
  Object.keys(proto).forEach(function(key) {
    this[key] = proto[key];
  }, Agent.prototype);

  return Agent;
};

proto._init = function init() {
  var self = this;

  var state = {};
  
  var createConnection;
  var cons = this.constructor.super_;
  do {
    createConnection = cons.prototype.createConnection;

    if (cons.super_ === EventEmitter || !cons.super_)
      break;
    cons = cons.super_;
  } while (!createConnection);

  if (!createConnection)
    createConnection = this.createConnection || net.createConnection;

  
  var socket = createConnection.call(this, util._extend({
    NPNProtocols: ['spdy/3.1', 'spdy/3', 'spdy/2'],
    ALPNProtocols: ['spdy/3.1', 'spdy/3', 'spdy/2']
  }, this.options));
  state.socket = socket;

  
  
  if (this.options.spdy.headerCompression !== true)
    this.options.spdy.headerCompression = false;

  
  var connection = new spdy.Connection(socket,
                                       util._extend(this.options.spdy, {
    isServer: false
  }));
  state.connection = connection;

  connection.on('error', function(err) {
    self.emit('error', err);
  });

  
  if (this.options.spdy.ssl !== false && !this.options.spdy.version) {
    
    socket.once('secureConnect', function() {
      var selectedProtocol = socket.npnProtocol || socket.alpnProtocol;
      if (selectedProtocol === 'spdy/2')
        connection._setVersion(2);
      else if (selectedProtocol === 'spdy/3')
        connection._setVersion(3);
      else if (selectedProtocol === 'spdy/3.1')
        connection._setVersion(3.1);
      else
        socket.emit('error', new Error('No supported SPDY version'));
    });
  } else {
    
    connection._setVersion(this.options.spdy.version || 3);
  }

  
  connection.on('stream', function(stream) {
    stream.on('error', function() {});
    stream.destroy();
  });
  state.pushServer = null;

  this.on('newListener', this._onNewListener.bind(this));
  this.on('removeListener', this._onRemoveListener.bind(this));

  
  state.id = 1;
  this._spdyState = state;
};






proto._onNewListener = function onNewListener(type, listener) {
  if (type !== 'push')
    return;

  var state = this._spdyState;
  if (state.pushServer)
    return state.pushServer.on('translated-request', listener);

  state.pushServer = require('http').createServer(listener);
  state.connection.removeAllListeners('stream');
  state.connection.on('stream', function(stream) {
    state.pushServer.emit('connection', stream);
  });

  state.pushServer.on('request', function(req) {
    
    req.connection.on('headers', function(headers) {
      Object.keys(headers).forEach(function(key) {
        req.trailers[key] = headers[key];
      });
      req.emit('trailers', headers);
    });

    this.emit('translated-request', req);
  });
};






proto._onRemoveListener = function onRemoveListener(type, listener) {
  if (type !== 'push')
    return;

  var state = this._spdyState;
  if (!state.pushServer)
    return;

  state.pushServer.removeListener('translated-request', listener);
};




proto.createConnection = function createConnection(options) {
  if (!options)
    options = {};

  var state = this._spdyState;
  var stream = new spdy.Stream(state.connection, {
    id: state.id,
    priority: options.priority || 7,
    client: true,
    decompress: options.spdy.decompress == undefined ? true :
                                                       options.spdy.decompress
  });
  state.id += 2;
  state.connection._addStream(stream);

  return stream;
};






proto.close = function close(callback) {
  this._spdyState.socket.destroySoon();
  if (callback)
    this._spdyState.socket.once('close', callback);
};






proto.ping = function ping(callback) {
  return this._spdyState.connection.ping(callback);
};




exports.Agent = instantiate(https.Agent);







exports.create = function create(base, options) {
  var agent;
  if (typeof base === 'function') {
    agent = instantiate(base);
  } else {
    agent = exports.Agent;

    options = base;
    base = null;
  }

  
  if (!base &&
      options &&
      options.spdy &&
      options.spdy.plain &&
      options.spdy.ssl === false) {
    return exports.create(require('http').Agent, options);
  }

  return new agent(options);
};
