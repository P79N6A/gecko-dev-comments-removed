





"use strict";
Components.utils.import("resource://gre/modules/NetUtil.jsm");



































this.DebuggerTransport = function DebuggerTransport(aInput, aOutput)
{
  this._input = aInput;
  this._output = aOutput;

  this._converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
    .createInstance(Ci.nsIScriptableUnicodeConverter);
  this._converter.charset = "UTF-8";

  this._outgoing = "";
  this._incoming = "";

  this.hooks = null;
}

DebuggerTransport.prototype = {
  







  send: function DT_send(aPacket) {
    let data = wantLogging
      ? JSON.stringify(aPacket, null, 2)
      : JSON.stringify(aPacket);
    data = this._converter.ConvertFromUnicode(data);
    data = data.length + ':' + data;
    this._outgoing += data;
    this._flushOutgoing();
  },

  


  close: function DT_close() {
    this._input.close();
    this._output.close();
  },

  


  _flushOutgoing: function DT_flushOutgoing() {
    if (this._outgoing.length > 0) {
      var threadManager = Cc["@mozilla.org/thread-manager;1"].getService();
      this._output.asyncWait(this, 0, 0, threadManager.currentThread);
    }
  },

  onOutputStreamReady:
  makeInfallible(function DT_onOutputStreamReady(aStream) {
    let written = 0;
    try {
      written = aStream.write(this._outgoing, this._outgoing.length);
    } catch(e if e.result == Components.results.NS_BASE_STREAM_CLOSED) {
      dumpn("Connection closed.");
      this.close();
      return;
    }
    this._outgoing = this._outgoing.slice(written);
    this._flushOutgoing();
  }, "DebuggerTransport.prototype.onOutputStreamReady"),

  




  ready: function DT_ready() {
    let pump = Cc["@mozilla.org/network/input-stream-pump;1"]
      .createInstance(Ci.nsIInputStreamPump);
    pump.init(this._input, -1, -1, 0, 0, false);
    pump.asyncRead(this, null);
  },

  
  onStartRequest:
  makeInfallible(function DT_onStartRequest(aRequest, aContext) {},
                 "DebuggerTransport.prototype.onStartRequest"),

  onStopRequest:
  makeInfallible(function DT_onStopRequest(aRequest, aContext, aStatus) {
    this.close();
    if (this.hooks) {
      this.hooks.onClosed(aStatus);
      this.hooks = null;
    }
  }, "DebuggerTransport.prototype.onStopRequest"),

  onDataAvailable:
  makeInfallible(function DT_onDataAvailable(aRequest, aContext,
                                             aStream, aOffset, aCount) {
    this._incoming += NetUtil.readInputStreamToString(aStream,
                                                      aStream.available());
    while (this._processIncoming()) {};
  }, "DebuggerTransport.prototype.onDataAvailable"),

  






  _processIncoming: function DT__processIncoming() {
    
    let sep = this._incoming.indexOf(':');
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

    dumpn("Got: " + packet);
    let self = this;
    Services.tm.currentThread.dispatch(makeInfallible(function() {
      
      
      if (self.hooks) {
        self.hooks.onPacket(parsed);
      }
    }, "DebuggerTransport instance's this.hooks.onPacket"), 0);

    return true;
  }
}













this.LocalDebuggerTransport = function LocalDebuggerTransport(aOther)
{
  this.other = aOther;
  this.hooks = null;

  




  this._serial = this.other ? this.other._serial : { count: 0 };
}

LocalDebuggerTransport.prototype = {
  



  send: function LDT_send(aPacket) {
    let serial = this._serial.count++;
    if (wantLogging) {
      
      if (aPacket.from) {
        dumpn("Packet " + serial + " sent from " + uneval(aPacket.from));
      } else if (aPacket.to) {
        dumpn("Packet " + serial + " sent to " + uneval(aPacket.to));
      }
    }
    this._deepFreeze(aPacket);
    let other = this.other;
    if (other) {
      Services.tm.currentThread.dispatch(makeInfallible(function() {
        
        if (wantLogging) {
          dumpn("Received packet " + serial + ": " + JSON.stringify(aPacket, null, 2));
        }
        if (other.hooks) {
          other.hooks.onPacket(aPacket);
        }
      }, "LocalDebuggerTransport instance's this.other.hooks.onPacket"), 0);
    }
  },

  


  close: function LDT_close() {
    if (this.other) {
      
      
      let other = this.other;
      delete this.other;
      other.close();
    }
    if (this.hooks) {
      try {
        this.hooks.onClosed();
      } catch(ex) {
        Components.utils.reportError(ex);
      }
      this.hooks = null;
    }
  },

  


  ready: function LDT_ready() {},

  


  _deepFreeze: function LDT_deepFreeze(aObject) {
    Object.freeze(aObject);
    for (let prop in aObject) {
      
      
      
      
      if (aObject.hasOwnProperty(prop) && typeof aObject === "object" &&
          !Object.isFrozen(aObject)) {
        this._deepFreeze(o[prop]);
      }
    }
  }
};















function ChildDebuggerTransport(aSender, aPrefix) {
  this._sender = aSender.QueryInterface(Components.interfaces.nsIMessageSender);
  this._messageName = "debug:" + aPrefix + ":packet";
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
