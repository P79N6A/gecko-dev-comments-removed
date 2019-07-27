



"use strict";

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Task.jsm");

Cu.import("chrome://marionette/content/error.js");

this.EXPORTED_SYMBOLS = ["CommandProcessor", "Response"];
const logger = Log.repository.getLogger("Marionette");























this.Response = function(cmdId, okHandler, respHandler, msg, sanitizer) {
  const removeEmpty = function(map) {
    let rv = {};
    for (let [key, value] of map) {
      if (typeof value == "undefined") {
        value = null;
      }
      rv[key] = value;
    }
    return rv;
  };

  this.id = cmdId;
  this.ok = true;
  this.okHandler = okHandler;
  this.respHandler = respHandler;
  this.sanitizer = sanitizer || removeEmpty;

  this.data = new Map([
    ["sessionId", msg.sessionId ? msg.sessionId : null],
    ["status", msg.status ? msg.status : 0 ],
    ["value", msg.value ? msg.value : undefined],
  ]);
};

Response.prototype = {
  get name() { return this.data.get("name"); },
  set name(n) { this.data.set("name", n); },
  get sessionId() { return this.data.get("sessionId"); },
  set sessionId(id) { this.data.set("sessionId", id); },
  get status() { return this.data.get("status"); },
  set status(ns) { this.data.set("status", ns); },
  get value() { return this.data.get("value"); },
  set value(val) {
    this.data.set("value", val);
    this.ok = false;
  }
};

Response.prototype.send = function() {
  if (this.sent) {
    logger.warn("Skipped sending response to command ID " +
      this.id + " because response has already been sent");
    return;
  }

  if (this.ok) {
    this.okHandler(this.id);
  } else {
    let rawData = this.sanitizer(this.data);
    this.respHandler(rawData, this.id);
  }
};






Response.prototype.sendError = function(err) {
  this.status = "code" in err ? err.code : new UnknownError().code;
  this.value = error.toJSON(err);
  this.send();

  
  if (!error.isWebDriverError(err)) {
    throw err;
  }
};









this.CommandProcessor = function(driver) {
  this.driver = driver;
};





















CommandProcessor.prototype.execute = function(payload, okHandler, respHandler, cmdId) {
  let cmd = payload;
  let resp = new Response(
    cmdId, okHandler, respHandler, {sessionId: this.driver.sessionId});
  let sendResponse = resp.send.bind(resp);
  let sendError = resp.sendError.bind(resp);

  
  
  
  cmd.id = cmdId;

  
  
  
  this.driver.listener.curCmdId = cmd.id;

  let req = Task.spawn(function*() {
    let fn = this.driver.commands[cmd.name];
    if (typeof fn == "undefined") {
      throw new UnknownCommandError(cmd.name);
    }

    yield fn.bind(this.driver)(cmd, resp);
  }.bind(this));

  req.then(sendResponse, sendError).catch(error.report);
};
