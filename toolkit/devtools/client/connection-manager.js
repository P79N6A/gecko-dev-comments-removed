





"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {setTimeout, clearTimeout} = require('sdk/timers');
const EventEmitter = require("devtools/toolkit/event-emitter");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");
Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
DevToolsUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");

























































let ConnectionManager = {
  _connections: new Set(),
  createConnection: function(host, port) {
    let c = new Connection(host, port);
    c.once("destroy", (event) => this.destroyConnection(c));
    this._connections.add(c);
    this.emit("new", c);
    return c;
  },
  destroyConnection: function(connection) {
    if (this._connections.has(connection)) {
      this._connections.delete(connection);
      if (connection.status != Connection.Status.DESTROYED) {
        connection.destroy();
      }
    }
  },
  get connections() {
    return [c for (c of this._connections)];
  },
  getFreeTCPPort: function () {
    let serv = Cc['@mozilla.org/network/server-socket;1']
                 .createInstance(Ci.nsIServerSocket);
    serv.init(-1, true, -1);
    let port = serv.port;
    serv.close();
    return port;
  },
}

EventEmitter.decorate(ConnectionManager);

let lastID = -1;

function Connection(host, port) {
  EventEmitter.decorate(this);
  this.uid = ++lastID;
  this.host = host;
  this.port = port;
  this._setStatus(Connection.Status.DISCONNECTED);
  this._onDisconnected = this._onDisconnected.bind(this);
  this._onConnected = this._onConnected.bind(this);
  this._onTimeout = this._onTimeout.bind(this);
  this.keepConnecting = false;
  this.encryption = false;
}

Connection.Status = {
  CONNECTED: "connected",
  DISCONNECTED: "disconnected",
  CONNECTING: "connecting",
  DISCONNECTING: "disconnecting",
  DESTROYED: "destroyed",
}

Connection.Events = {
  CONNECTED: Connection.Status.CONNECTED,
  DISCONNECTED: Connection.Status.DISCONNECTED,
  CONNECTING: Connection.Status.CONNECTING,
  DISCONNECTING: Connection.Status.DISCONNECTING,
  DESTROYED: Connection.Status.DESTROYED,
  TIMEOUT: "timeout",
  STATUS_CHANGED: "status-changed",
  HOST_CHANGED: "host-changed",
  PORT_CHANGED: "port-changed",
  NEW_LOG: "new_log"
}

Connection.prototype = {
  logs: "",
  log: function(str) {
    let d = new Date();
    let hours = ("0" + d.getHours()).slice(-2);
    let minutes = ("0" + d.getMinutes()).slice(-2);
    let seconds = ("0" + d.getSeconds()).slice(-2);
    let timestamp = [hours, minutes, seconds].join(":") + ": ";
    str = timestamp + str;
    this.logs +=  "\n" + str;
    this.emit(Connection.Events.NEW_LOG, str);
  },

  get client() {
    return this._client
  },

  get host() {
    return this._host
  },

  set host(value) {
    if (this._host && this._host == value)
      return;
    this._host = value;
    this.emit(Connection.Events.HOST_CHANGED);
  },

  get port() {
    return this._port
  },

  set port(value) {
    if (this._port && this._port == value)
      return;
    this._port = value;
    this.emit(Connection.Events.PORT_CHANGED);
  },

  disconnect: function(force) {
    if (this.status == Connection.Status.DESTROYED) {
      return;
    }
    clearTimeout(this._timeoutID);
    if (this.status == Connection.Status.CONNECTED ||
        this.status == Connection.Status.CONNECTING) {
      this.log("disconnecting");
      this._setStatus(Connection.Status.DISCONNECTING);
      this._client.close();
    }
  },

  connect: function(transport) {
    if (this.status == Connection.Status.DESTROYED) {
      return;
    }
    if (!this._client) {
      this._customTransport = transport;
      if (this._customTransport) {
        this.log("connecting (custom transport)");
      } else {
        this.log("connecting to " + this.host + ":" + this.port);
      }
      this._setStatus(Connection.Status.CONNECTING);

      let delay = Services.prefs.getIntPref("devtools.debugger.remote-timeout");
      this._timeoutID = setTimeout(this._onTimeout, delay);
      this._clientConnect();
    } else {
      let msg = "Can't connect. Client is not fully disconnected";
      this.log(msg);
      throw new Error(msg);
    }
  },

  destroy: function() {
    this.log("killing connection");
    clearTimeout(this._timeoutID);
    this.keepConnecting = false;
    if (this._client) {
      this._client.close();
      this._client = null;
    }
    this._setStatus(Connection.Status.DESTROYED);
  },

  _getTransport: Task.async(function*() {
    if (this._customTransport) {
      return this._customTransport;
    }
    if (!this.host) {
      return DebuggerServer.connectPipe();
    }
    let transport = yield DebuggerClient.socketConnect({
      host: this.host,
      port: this.port,
      encryption: this.encryption
    });
    return transport;
  }),

  _clientConnect: function () {
    this._getTransport().then(transport => {
      if (!transport) {
        return;
      }
      this._client = new DebuggerClient(transport);
      this._client.addOneTimeListener("closed", this._onDisconnected);
      this._client.connect(this._onConnected);
    }, e => {
      console.error(e);
      
      
      
      
      
      this._onDisconnected();
    });
  },

  get status() {
    return this._status
  },

  _setStatus: function(value) {
    if (this._status && this._status == value)
      return;
    this._status = value;
    this.emit(value);
    this.emit(Connection.Events.STATUS_CHANGED, value);
  },

  _onDisconnected: function() {
    this._client = null;
    this._customTransport = null;

    if (this._status == Connection.Status.CONNECTING && this.keepConnecting) {
      setTimeout(() => this._clientConnect(), 100);
      return;
    }

    clearTimeout(this._timeoutID);

    switch (this.status) {
      case Connection.Status.CONNECTED:
        this.log("disconnected (unexpected)");
        break;
      case Connection.Status.CONNECTING:
        this.log("connection error. Possible causes: USB port not connected, port not forwarded (adb forward), wrong host or port, remote debugging not enabled on the device.");
        break;
      default:
        this.log("disconnected");
    }
    this._setStatus(Connection.Status.DISCONNECTED);
  },

  _onConnected: function() {
    this.log("connected");
    clearTimeout(this._timeoutID);
    this._setStatus(Connection.Status.CONNECTED);
  },

  _onTimeout: function() {
    this.log("connection timeout. Possible causes: didn't click on 'accept' (prompt).");
    this.emit(Connection.Events.TIMEOUT);
    this.disconnect();
  },
}

exports.ConnectionManager = ConnectionManager;
exports.Connection = Connection;
