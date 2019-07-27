















'use strict';

var Cu = require('chrome').Cu;

var DebuggerClient = Cu.import('resource://gre/modules/devtools/dbg-client.jsm', {}).DebuggerClient;

var Promise = require('../util/promise').Promise;
var Connection = require('./connectors').Connection;




Object.defineProperty(exports, 'defaultPort', {
  get: function() {
    var Services = Cu.import('resource://gre/modules/Services.jsm', {}).Services;
    try {
      return Services.prefs.getIntPref('devtools.debugger.chrome-debugging-port');
    }
    catch (ex) {
      console.error('Can\'t use default port from prefs. Using 9999');
      return 9999;
    }
  },
  enumerable: true
});

exports.items = [
  {
    item: 'connector',
    name: 'rdp',

    connect: function(url) {
      return RdpConnection.create(url);
    }
  }
];




function RdpConnection(url) {
  throw new Error('Use RdpConnection.create');
}




RdpConnection.create = function(url) {
  this.host = url;
  this.port = undefined; 

  this.requests = {};
  this.nextRequestId = 0;

  this._emit = this._emit.bind(this);

  return new Promise(function(resolve, reject) {
    this.transport = DebuggerClient.socketConnect(this.host, this.port);
    this.client = new DebuggerClient(this.transport);
    this.client.connect(function() {
      this.client.listTabs(function(response) {
        this.actor = response.gcliActor;
        resolve();
      }.bind(this));
    }.bind(this));
  }.bind(this));
};

RdpConnection.prototype = Object.create(Connection.prototype);

RdpConnection.prototype.call = function(command, data) {
  return new Promise(function(resolve, reject) {
    var request = { to: this.actor, type: command, data: data };

    this.client.request(request, function(response) {
      resolve(response.commandSpecs);
    });
  }.bind(this));
};

RdpConnection.prototype.disconnect = function() {
  return new Promise(function(resolve, reject) {
    this.client.close(function() {
      resolve();
    });

    delete this._emit;
  }.bind(this));
};






function Request(actor, typed, args) {
  this.json = {
    to: actor,
    type: 'execute',
    typed: typed,
    args: args,
    requestId: 'id-' + Request._nextRequestId++,
  };

  this.promise = new Promise(function(resolve, reject) {
    this._resolve = resolve;
  }.bind(this));
}

Request._nextRequestId = 0;







Request.prototype.complete = function(error, type, data) {
  this._resolve({
    error: error,
    type: type,
    data: data
  });
};
