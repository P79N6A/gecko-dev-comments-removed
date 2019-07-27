







(function (factory) { 
  if (this.module && module.id.indexOf("transport") >= 0) { 
    factory.call(this, require, exports);
  } else { 
    if (this.require) {
      factory.call(this, require, this);
    } else {
      const Cu = Components.utils;
      const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
      factory.call(this, devtools.require, this);
    }
  }
}).call(this, function (require, exports) {

"use strict";

const { Cc, Ci, Cr, Cu, CC } = require("chrome");
const Services = require("Services");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const { dumpn, dumpv } = DevToolsUtils;
const StreamUtils = require("devtools/toolkit/transport/stream-utils");
const { Packet, JSONPacket, BulkPacket } =
  require("devtools/toolkit/transport/packets");
const promise = require("promise");

DevToolsUtils.defineLazyGetter(this, "Pipe", () => {
  return CC("@mozilla.org/pipe;1", "nsIPipe", "init");
});

DevToolsUtils.defineLazyGetter(this, "ScriptableInputStream", () => {
  return CC("@mozilla.org/scriptableinputstream;1",
            "nsIScriptableInputStream", "init");
});

const PACKET_HEADER_MAX = 200;


























































function DebuggerTransport(input, output) {
  this._input = input;
  this._scriptableInput = new ScriptableInputStream(input);
  this._output = output;

  
  
  this._incomingHeader = "";
  
  this._incoming = null;
  
  this._outgoing = [];

  this.hooks = null;
  this.active = false;

  this._incomingEnabled = true;
  this._outgoingEnabled = true;

  this.close = this.close.bind(this);
}

DebuggerTransport.prototype = {
  







  send: function(object) {
    let packet = new JSONPacket(this);
    packet.object = object;
    this._outgoing.push(packet);
    this._flushOutgoing();
  },

  








































  startBulkSend: function(header) {
    let packet = new BulkPacket(this);
    packet.header = header;
    this._outgoing.push(packet);
    this._flushOutgoing();
    return packet.streamReadyForWriting;
  },

  





  close: function(reason) {
    this.active = false;
    this._input.close();
    this._scriptableInput.close();
    this._output.close();
    this._destroyIncoming();
    this._destroyAllOutgoing();
    if (this.hooks) {
      this.hooks.onClosed(reason);
      this.hooks = null;
    }
    if (reason) {
      dumpn("Transport closed: " + DevToolsUtils.safeErrorString(reason));
    } else {
      dumpn("Transport closed.");
    }
  },

  


  get _currentOutgoing() { return this._outgoing[0]; },

  



  _flushOutgoing: function() {
    if (!this._outgoingEnabled || this._outgoing.length === 0) {
      return;
    }

    
    if (this._currentOutgoing.done) {
      this._finishCurrentOutgoing();
    }

    if (this._outgoing.length > 0) {
      var threadManager = Cc["@mozilla.org/thread-manager;1"].getService();
      this._output.asyncWait(this, 0, 0, threadManager.currentThread);
    }
  },

  




  pauseOutgoing: function() {
    this._outgoingEnabled = false;
  },

  


  resumeOutgoing: function() {
    this._outgoingEnabled = true;
    this._flushOutgoing();
  },

  
  




  onOutputStreamReady: DevToolsUtils.makeInfallible(function(stream) {
    if (this._outgoing.length === 0) {
      return;
    }

    try {
      this._currentOutgoing.write(stream);
    } catch(e if e.result != Cr.NS_BASE_STREAM_WOULD_BLOCK) {
      this.close(e.result);
      return;
    }

    this._flushOutgoing();
  }, "DebuggerTransport.prototype.onOutputStreamReady"),

  


  _finishCurrentOutgoing: function() {
    if (this._currentOutgoing) {
      this._currentOutgoing.destroy();
      this._outgoing.shift();
    }
  },

  


  _destroyAllOutgoing: function() {
    for (let packet of this._outgoing) {
      packet.destroy();
    }
    this._outgoing = [];
  },

  




  ready: function() {
    this.active = true;
    this._waitForIncoming();
  },

  



  _waitForIncoming: function() {
    if (this._incomingEnabled) {
      let threadManager = Cc["@mozilla.org/thread-manager;1"].getService();
      this._input.asyncWait(this, 0, 0, threadManager.currentThread);
    }
  },

  




  pauseIncoming: function() {
    this._incomingEnabled = false;
  },

  


  resumeIncoming: function() {
    this._incomingEnabled = true;
    this._flushIncoming();
    this._waitForIncoming();
  },

  
  


  onInputStreamReady:
  DevToolsUtils.makeInfallible(function(stream) {
    try {
      while(stream.available() && this._incomingEnabled &&
            this._processIncoming(stream, stream.available())) {}
      this._waitForIncoming();
    } catch(e if e.result != Cr.NS_BASE_STREAM_WOULD_BLOCK) {
      this.close(e.result);
    }
  }, "DebuggerTransport.prototype.onInputStreamReady"),

  









  _processIncoming: function(stream, count) {
    dumpv("Data available: " + count);

    if (!count) {
      dumpv("Nothing to read, skipping");
      return false;
    }

    try {
      if (!this._incoming) {
        dumpv("Creating a new packet from incoming");

        if (!this._readHeader(stream)) {
          return false; 
        }

        
        
        this._incoming = Packet.fromHeader(this._incomingHeader, this);
        if (!this._incoming) {
          throw new Error("No packet types for header: " +
                          this._incomingHeader);
        }
      }

      if (!this._incoming.done) {
        
        dumpv("Existing packet incomplete, keep reading");
        this._incoming.read(stream, this._scriptableInput);
      }
    } catch(e) {
      let msg = "Error reading incoming packet: (" + e + " - " + e.stack + ")";
      dumpn(msg);

      
      this.close();
      return false;
    }

    if (!this._incoming.done) {
      
      dumpv("Packet not done, wait for more");
      return true;
    }

    
    this._flushIncoming();
    return true;
  },

  






  _readHeader: function() {
    let amountToRead = PACKET_HEADER_MAX - this._incomingHeader.length;
    this._incomingHeader +=
      StreamUtils.delimitedRead(this._scriptableInput, ":", amountToRead);
    if (dumpv.wantVerbose) {
      dumpv("Header read: " + this._incomingHeader);
    }

    if (this._incomingHeader.endsWith(":")) {
      if (dumpv.wantVerbose) {
        dumpv("Found packet header successfully: " + this._incomingHeader);
      }
      return true;
    }

    if (this._incomingHeader.length >= PACKET_HEADER_MAX) {
      throw new Error("Failed to parse packet header!");
    }

    
    return false;
  },

  


  _flushIncoming: function() {
    if (!this._incoming.done) {
      return;
    }
    if (dumpn.wantLogging) {
      dumpn("Got: " + this._incoming);
    }
    this._destroyIncoming();
  },

  



  _onJSONObjectReady: function(object) {
    DevToolsUtils.executeSoon(DevToolsUtils.makeInfallible(() => {
      
      if (this.active) {
        this.hooks.onPacket(object);
      }
    }, "DebuggerTransport instance's this.hooks.onPacket"));
  },

  





  _onBulkReadReady: function(...args) {
    DevToolsUtils.executeSoon(DevToolsUtils.makeInfallible(() => {
      
      if (this.active) {
        this.hooks.onBulkPacket(...args);
      }
    }, "DebuggerTransport instance's this.hooks.onBulkPacket"));
  },

  



  _destroyIncoming: function() {
    if (this._incoming) {
      this._incoming.destroy();
    }
    this._incomingHeader = "";
    this._incoming = null;
  }

};

exports.DebuggerTransport = DebuggerTransport;












function LocalDebuggerTransport(other) {
  this.other = other;
  this.hooks = null;

  




  this._serial = this.other ? this.other._serial : { count: 0 };
  this.close = this.close.bind(this);
}

LocalDebuggerTransport.prototype = {
  



  send: function(packet) {
    let serial = this._serial.count++;
    if (dumpn.wantLogging) {
      
      if (packet.from) {
        dumpn("Packet " + serial + " sent from " + uneval(packet.from));
      } else if (packet.to) {
        dumpn("Packet " + serial + " sent to " + uneval(packet.to));
      }
    }
    this._deepFreeze(packet);
    let other = this.other;
    if (other) {
      DevToolsUtils.executeSoon(DevToolsUtils.makeInfallible(() => {
        
        if (dumpn.wantLogging) {
          dumpn("Received packet " + serial + ": " + JSON.stringify(packet, null, 2));
        }
        if (other.hooks) {
          other.hooks.onPacket(packet);
        }
      }, "LocalDebuggerTransport instance's this.other.hooks.onPacket"));
    }
  },

  








  startBulkSend: function({actor, type, length}) {
    let serial = this._serial.count++;

    dumpn("Sent bulk packet " + serial + " for actor " + actor);
    if (!this.other) {
      return;
    }

    let pipe = new Pipe(true, true, 0, 0, null);

    DevToolsUtils.executeSoon(DevToolsUtils.makeInfallible(() => {
      dumpn("Received bulk packet " + serial);
      if (!this.other.hooks) {
        return;
      }

      
      let deferred = promise.defer();

      this.other.hooks.onBulkPacket({
        actor: actor,
        type: type,
        length: length,
        copyTo: (output) => {
          let copying =
            StreamUtils.copyStream(pipe.inputStream, output, length);
          deferred.resolve(copying);
          return copying;
        },
        stream: pipe.inputStream,
        done: deferred
      });

      
      deferred.promise.then(() => pipe.inputStream.close(), this.close);
    }, "LocalDebuggerTransport instance's this.other.hooks.onBulkPacket"));

    
    let sendDeferred = promise.defer();

    
    
    DevToolsUtils.executeSoon(() => {
      let copyDeferred = promise.defer();

      sendDeferred.resolve({
        copyFrom: (input) => {
          let copying =
            StreamUtils.copyStream(input, pipe.outputStream, length);
          copyDeferred.resolve(copying);
          return copying;
        },
        stream: pipe.outputStream,
        done: copyDeferred
      });

      
      copyDeferred.promise.then(() => pipe.outputStream.close(), this.close);
    });

    return sendDeferred.promise;
  },

  


  close: function() {
    if (this.other) {
      
      
      let other = this.other;
      this.other = null;
      other.close();
    }
    if (this.hooks) {
      try {
        this.hooks.onClosed();
      } catch(ex) {
        Cu.reportError(ex);
      }
      this.hooks = null;
    }
  },

  


  ready: function() {},

  


  _deepFreeze: function(object) {
    Object.freeze(object);
    for (let prop in object) {
      
      
      
      
      if (object.hasOwnProperty(prop) && typeof object === "object" &&
          !Object.isFrozen(object)) {
        this._deepFreeze(o[prop]);
      }
    }
  }
};

exports.LocalDebuggerTransport = LocalDebuggerTransport;















function ChildDebuggerTransport(sender, prefix) {
  this._sender = sender.QueryInterface(Ci.nsIMessageSender);
  this._messageName = "debug:" + prefix + ":packet";
}






ChildDebuggerTransport.prototype = {
  constructor: ChildDebuggerTransport,

  hooks: null,

  ready: function () {
    this._sender.addMessageListener(this._messageName, this);
  },

  close: function () {
    this._sender.removeMessageListener(this._messageName, this);
    this.hooks.onClosed();
  },

  receiveMessage: function ({data}) {
    this.hooks.onPacket(data);
  },

  send: function (packet) {
    this._sender.sendAsyncMessage(this._messageName, packet);
  },

  startBulkSend: function() {
    throw new Error("Can't send bulk data to child processes.");
  }
};

exports.ChildDebuggerTransport = ChildDebuggerTransport;

});
