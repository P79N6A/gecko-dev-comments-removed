















'use strict';

var Promise = require('../util/promise').Promise;




var connectors = {};
















function Connection() {
}




Connection.prototype.on = function(event, action) {
  if (!this._listeners) {
    this._listeners = {};
  }
  if (!this._listeners[event]) {
    this._listeners[event] = [];
  }
  this._listeners[event].push(action);
};




Connection.prototype.off = function(event, action) {
  if (!this._listeners) {
    return;
  }
  var actions = this._listeners[event];
  if (actions) {
    this._listeners[event] = actions.filter(function(li) {
      return li !== action;
    }.bind(this));
  }
};




Connection.prototype._emit = function(event, data) {
  if (this._listeners == null || this._listeners[event] == null) {
    return;
  }

  var listeners = this._listeners[event];
  listeners.forEach(function(listener) {
    
    if (listeners !== this._listeners[event]) {
      throw new Error('Listener list changed while emitting');
    }

    try {
      listener.call(null, data);
    }
    catch (ex) {
      console.log('Error calling listeners to ' + event);
      console.error(ex);
    }
  }.bind(this));
};




Connection.prototype.call = function(feature, data) {
  throw new Error('Not implemented');
};





Connection.prototype.disconnect = function() {
  return Promise.resolve();
};

exports.Connection = Connection;





exports.addConnector = function(connector) {
  connectors[connector.name] = connector;
};




exports.removeConnector = function(connector) {
  var name = typeof connector === 'string' ? connector : connector.name;
  delete connectors[name];
};




exports.getConnectors = function() {
  return Object.keys(connectors).map(function(name) {
    return connectors[name];
  });
};





exports.get = function(name) {
  if (name == null) {
    name = Object.keys(connectors)[0];
  }
  return connectors[name];
};
