



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Cu } = require("chrome");
const { Class } = require("../sdk/core/heritage");
const { MessagePort, MessageChannel } = require("../sdk/messaging");
const { DebuggerServer } = Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});

const outputs = new WeakMap();
const inputs = new WeakMap();
const targets = new WeakMap();
const transports = new WeakMap();

const inputFor = port => inputs.get(port);
const outputFor = port => outputs.get(port);
const transportFor = port => transports.get(port);


const fromTarget = target => {
  const debuggee = new Debuggee();
  const { port1, port2 } = new MessageChannel();
  inputs.set(debuggee, port1);
  outputs.set(debuggee, port2);
  targets.set(debuggee, target);

  return debuggee;
};
exports.fromTarget = fromTarget;

const Debuggee = Class({
  extends: MessagePort.prototype,
  close: function() {
    const server = transportFor(this);
    if (server) {
      transports.delete(this);
      server.close();
    }
    outputFor(this).close();
  },
  start: function() {
    const target = targets.get(this);
    if (target.isLocalTab) {
      
      
      if (!DebuggerServer.initialized) {
        DebuggerServer.init();
        DebuggerServer.addBrowserActors();
      }

      transports.set(this, DebuggerServer.connectPipe());
    }
    
    else {
      throw Error("Remote targets are not yet supported");
    }

    
    
    inputFor(this).addEventListener("message", ({data}) =>
      transportFor(this).send(data));

    
    
    transportFor(this).hooks = {
      onPacket: packet => inputFor(this).postMessage(packet),
      onClosed: () => inputFor(this).close()
    };

    inputFor(this).start();
    outputFor(this).start();
  },
  postMessage: function(data) {
    return outputFor(this).postMessage(data);
  },
  get onmessage() {
    return outputFor(this).onmessage;
  },
  set onmessage(onmessage) {
    outputFor(this).onmessage = onmessage;
  },
  addEventListener: function(...args) {
    return outputFor(this).addEventListener(...args);
  },
  removeEventListener: function(...args) {
    return outputFor(this).removeEventListener(...args);
  }
});
exports.Debuggee = Debuggee;
