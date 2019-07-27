



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");

Cu.import("chrome://marionette/content/command.js");
Cu.import("chrome://marionette/content/emulator.js");
Cu.import("chrome://marionette/content/error.js");
Cu.import("chrome://marionette/content/driver.js");

this.EXPORTED_SYMBOLS = ["Dispatcher"];

const logger = Log.repository.getLogger("Marionette");
const uuidGen = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);















this.Dispatcher = function(connId, transport, driverFactory, stopSignal) {
  this.id = connId;
  this.conn = transport;

  
  
  
  
  this.actorId = "0";

  
  this.onclose = null;

  
  
  this.conn.hooks = this;

  this.emulator = new Emulator(msg => this.sendResponse(msg, -1));
  this.driver = driverFactory(this.emulator);
  this.commandProcessor = new CommandProcessor(this.driver);

  this.stopSignal_ = stopSignal;
};






Dispatcher.prototype.onPacket = function(packet) {
  
  
  
  if (logger.level <= Log.Level.Debug) {
    logger.debug(this.id + " -> (" + JSON.stringify(packet) + ")");
  }

  if (this.requests && this.requests[packet.name]) {
    this.requests[packet.name].bind(this)(packet);
  } else {
    let id = this.beginNewCommand();
    let ok = this.sendOk.bind(this);
    let send = this.send.bind(this);
    this.commandProcessor.execute(packet, ok, send, id);
  }
};





Dispatcher.prototype.onClosed = function(status) {
  this.driver.sessionTearDown();
  if (this.onclose) {
    this.onclose(this);
  }
};



Dispatcher.prototype.getMarionetteID = function() {
  let id = this.beginNewCommand();
  this.sendResponse({from: "root", id: this.actorId}, id);
};

Dispatcher.prototype.emulatorCmdResult = function(msg) {
  switch (this.driver.context) {
    case Context.CONTENT:
      this.driver.sendAsync("emulatorCmdResult", msg);
      break;
    case Context.CHROME:
      let cb = this.emulator.popCallback(msg.id);
      if (!cb) {
        return;
      }
      cb.result(msg);
      break;
  }
};





Dispatcher.prototype.quitApplication = function(msg) {
  let id = this.beginNewCommand();

  if (this.driver.appName != "Firefox") {
    this.sendError({
      "message": "In app initiated quit only supported on Firefox",
      "status": "webdriver error",
    }, id);
    return;
  }

  let flags = Ci.nsIAppStartup.eAttemptQuit;
  for (let k of msg.parameters.flags) {
    flags |= Ci.nsIAppStartup[k];
  }

  this.stopSignal_();
  this.sendOk(id);

  this.driver.sessionTearDown();
  Services.startup.quit(flags);
};



Dispatcher.prototype.sayHello = function() {
  let id = this.beginNewCommand();
  let yo = {from: "root", applicationType: "gecko", traits: []};
  this.sendResponse(yo, id);
};

Dispatcher.prototype.sendOk = function(cmdId) {
  this.sendResponse({from: this.actorId, ok: true}, cmdId);
};

Dispatcher.prototype.sendError = function(err, cmdId) {
  let packet = {
    from: this.actorId,
    status: err.status,
    sessionId: this.driver.sessionId,
    error: err
  };
  this.sendResponse(packet, cmdId);
};

















Dispatcher.prototype.send = function(msg, cmdId) {
  let packet = {
    from: this.actorId,
    value: msg.value,
    status: msg.status,
    sessionId: msg.sessionId,
  };

  if (typeof packet.value == "undefined") {
    packet.value = null;
  }

  
  
  if (!error.isSuccess(msg.status)) {
    packet.error = packet.value;
    delete packet.value;
  }

  this.sendResponse(packet, cmdId);
};




















Dispatcher.prototype.sendResponse = function(payload, cmdId) {
  if (emulator.isCallback(cmdId)) {
    this.sendToEmulator(payload);
  } else {
    this.sendToClient(payload, cmdId);
    this.commandId = null;
  }
};





Dispatcher.prototype.sendToEmulator = function(payload) {
  this.sendRaw("emulator", payload);
};









Dispatcher.prototype.sendToClient = function(payload, cmdId) {
  if (!cmdId) {
    logger.warn("Got response with no command ID");
    return;
  } else if (this.commandId === null) {
    logger.warn(`No current command, ignoring response: ${payload.toSource}`);
    return;
  } else if (this.isOutOfSync(cmdId)) {
    logger.warn(`Ignoring out-of-sync response with command ID: ${cmdId}`);
    return;
  }
  this.driver.responseCompleted();
  this.sendRaw("client", payload);
};





Dispatcher.prototype.sendRaw = function(dest, payload) {
  
  
  
  if (logger.level <= Log.Level.Debug) {
    logger.debug(this.id + " " + dest + " <- (" + JSON.stringify(payload) + ")");
  }
  this.conn.send(payload);
};








Dispatcher.prototype.beginNewCommand = function() {
  let uuid = uuidGen.generateUUID().toString();
  this.commandId = uuid;
  return uuid;
};

Dispatcher.prototype.isOutOfSync = function(cmdId) {
  return this.commandId !== cmdId;
};

Dispatcher.prototype.requests = {
  getMarionetteID: Dispatcher.prototype.getMarionetteID,
  emulatorCmdResult: Dispatcher.prototype.emulatorCmdResult,
  quitApplication: Dispatcher.prototype.quitApplication
};
