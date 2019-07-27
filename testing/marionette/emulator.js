



"use strict";

const {classes: Cc, interfaces: Ci} = Components;
const uuidGen = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);
this.EXPORTED_SYMBOLS = ["emulator", "Emulator", "EmulatorCallback"];

this.emulator = {};




this.emulator.isCallback = function(cmdId) {
  return cmdId < 0;
};













this.Emulator = function(sendFn) {
  this.send = sendFn;
  this.cbs = [];
};











Emulator.prototype.popCallback = function(id) {
  let f, fi;
  for (let i = 0; i < this.cbs.length; ++i) {
    if (this.cbs[i].id == id) {
      f = this.cbs[i];
      fi = i;
    }
  }

  if (!f) {
    return null;
  }

  this.cbs.splice(fi, 1);
  return f;
};








Emulator.prototype.pushCallback = function(cb) {
  cb.send_ = this.sendFn;
  this.cbs.push(cb);
};














this.EmulatorCallback = function() {
  this.id = uuidGen.generateUUID().toString();
  this.onresult = null;
  this.onerror = null;
  this.send_ = null;
};

EmulatorCallback.prototype.command = function(cmd, cb) {
  this.onresult = cb;
  this.send_({emulator_cmd: cmd, id: this.id});
};

EmulatorCallback.prototype.shell = function(args, cb) {
  this.onresult = cb;
  this.send_({emulator_shell: args, id: this.id});
};

EmulatorCallback.prototype.result = function(msg) {
  if (this.send_ === null) {
    throw new TypeError(
      "EmulatorCallback must be registered with Emulator to fire");
  }

  try {
    if (!this.onresult) {
      return;
    }
    this.onresult(msg.result);
  } catch (e) {
    if (this.onerror) {
      this.onerror(e);
    }
  }
};
