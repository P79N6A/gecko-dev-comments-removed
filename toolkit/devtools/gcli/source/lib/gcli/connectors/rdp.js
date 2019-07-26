















'use strict';

var Cu = require('chrome').Cu;

var debuggerSocketConnect = Cu.import('resource://gre/modules/devtools/dbg-client.jsm', {}).debuggerSocketConnect;
var DebuggerClient = Cu.import('resource://gre/modules/devtools/dbg-client.jsm', {}).DebuggerClient;

var promise = require('../util/promise');
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

  var deferred = promise.defer();

  this.transport = debuggerSocketConnect(this.host, this.port);
  this.client = new DebuggerClient(this.transport);

  this.client.connect(function() {
    this.client.listTabs(function(response) {
      this.actor = response.gcliActor;
      deferred.resolve();
    }.bind(this));
  }.bind(this));

  return deferred.promise;
};

RdpConnection.prototype = Object.create(Connection.prototype);

RdpConnection.prototype.call = function(command, data) {
  var deferred = promise.defer();

  var request = { to: this.actor, type: command, data: data };

  this.client.request(request, function(response) {
    deferred.resolve(response.commandSpecs);
  });

  return deferred.promise;
};

RdpConnection.prototype.disconnect = function() {
  var deferred = promise.defer();

  this.client.close(function() {
    deferred.resolve();
  });

  delete this._emit;

  return deferred.promise;
};






function Request(actor, typed, args) {
  this.json = {
    to: actor,
    type: 'execute',
    typed: typed,
    args: args,
    requestId: 'id-' + Request._nextRequestId++,
  };

  this._deferred = promise.defer();
  this.promise = this._deferred.promise;
}

Request._nextRequestId = 0;







Request.prototype.complete = function(error, type, data) {
  this._deferred.resolve({
    error: error,
    type: type,
    data: data
  });
};
