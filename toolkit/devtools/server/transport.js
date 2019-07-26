







(function (factory) { 
  if (this.module && module.id.indexOf("transport") >= 0) { 
    factory(require, exports);
  } else { 
    if (this.require) {
      factory(require, this);
    } else {
      const Cu = Components.utils;
      const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
      factory(devtools.require, this);
    }
  }
}).call(this, function (require, exports) {

"use strict";

const { Cc, Ci, Cr, Cu } = require("chrome");
const Services = require("Services");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const { dumpn } = DevToolsUtils;

Cu.import("resource://gre/modules/NetUtil.jsm");



































function DebuggerTransport(input, output) {
  this._input = input;
  this._output = output;

  this._converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
    .createInstance(Ci.nsIScriptableUnicodeConverter);
  this._converter.charset = "UTF-8";

  this._outgoing = "";
  this._incoming = "";

  this.hooks = null;
}

DebuggerTransport.prototype = {
  







  send: function(packet) {
    let data = JSON.stringify(packet);
    data = this._converter.ConvertFromUnicode(data);
    data = data.length + ":" + data;
    this._outgoing += data;
    this._flushOutgoing();
  },

  


  close: function() {
    this._input.close();
    this._output.close();
  },

  


  _flushOutgoing: function() {
    if (this._outgoing.length > 0) {
      var threadManager = Cc["@mozilla.org/thread-manager;1"].getService();
      this._output.asyncWait(this, 0, 0, threadManager.currentThread);
    }
  },

  onOutputStreamReady:
  DevToolsUtils.makeInfallible(function(stream) {
    let written = 0;
    try {
      written = stream.write(this._outgoing, this._outgoing.length);
    } catch(e if e.result == Cr.NS_BASE_STREAM_CLOSED) {
      dumpn("Connection closed.");
      this.close();
      return;
    }
    this._outgoing = this._outgoing.slice(written);
    this._flushOutgoing();
  }, "DebuggerTransport.prototype.onOutputStreamReady"),

  




  ready: function() {
    let pump = Cc["@mozilla.org/network/input-stream-pump;1"]
      .createInstance(Ci.nsIInputStreamPump);
    pump.init(this._input, -1, -1, 0, 0, false);
    pump.asyncRead(this, null);
  },

  
  onStartRequest:
  DevToolsUtils.makeInfallible(function(request, context) {},
                 "DebuggerTransport.prototype.onStartRequest"),

  onStopRequest:
  DevToolsUtils.makeInfallible(function(request, context, status) {
    this.close();
    if (this.hooks) {
      this.hooks.onClosed(status);
      this.hooks = null;
    }
  }, "DebuggerTransport.prototype.onStopRequest"),

  onDataAvailable:
  DevToolsUtils.makeInfallible(function(request, context, stream,
                                        offset, count) {
    this._incoming += NetUtil.readInputStreamToString(stream,
                                                      stream.available());
    while (this._processIncoming()) {}
  }, "DebuggerTransport.prototype.onDataAvailable"),

  






  _processIncoming: function() {
    
    let sep = this._incoming.indexOf(":");
    if (sep < 0) {
      
      if (this._incoming.length > 20) {
        this.close();
      }

      return false;
    }

    let count = this._incoming.substring(0, sep);
    
    if (!/^[0-9]+$/.exec(count)) {
      this.close();
      return false;
    }

    count = +count;
    if (this._incoming.length - (sep + 1) < count) {
      
      return false;
    }

    
    this._incoming = this._incoming.substring(sep + 1);
    let packet = this._incoming.substring(0, count);
    this._incoming = this._incoming.substring(count);

    try {
      packet = this._converter.ConvertToUnicode(packet);
      var parsed = JSON.parse(packet);
    } catch(e) {
      let msg = "Error parsing incoming packet: " + packet + " (" + e + " - " + e.stack + ")";
      if (Cu.reportError) {
        Cu.reportError(msg);
      }
      dump(msg + "\n");
      return true;
    }

    if (dumpn.wantLogging) {
      dumpn("Got: " + JSON.stringify(parsed, null, 2));
    }
    let self = this;
    Services.tm.currentThread.dispatch(DevToolsUtils.makeInfallible(function() {
      
      
      if (self.hooks) {
        self.hooks.onPacket(parsed);
      }
    }, "DebuggerTransport instance's this.hooks.onPacket"), 0);

    return true;
  }
};

exports.DebuggerTransport = DebuggerTransport;












function LocalDebuggerTransport(other) {
  this.other = other;
  this.hooks = null;

  




  this._serial = this.other ? this.other._serial : { count: 0 };
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
      Services.tm.currentThread.dispatch(DevToolsUtils.makeInfallible(() => {
        
        if (dumpn.wantLogging) {
          dumpn("Received packet " + serial + ": " + JSON.stringify(packet, null, 2));
        }
        if (other.hooks) {
          other.hooks.onPacket(packet);
        }
      }, "LocalDebuggerTransport instance's this.other.hooks.onPacket"), 0);
    }
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
  }
};

exports.ChildDebuggerTransport = ChildDebuggerTransport;

});
