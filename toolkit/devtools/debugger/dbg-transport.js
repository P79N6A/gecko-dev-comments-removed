





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
    
    let data = JSON.stringify(aPacket, null, 2);
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

  onOutputStreamReady: function DT_onOutputStreamReady(aStream) {
    let written = aStream.write(this._outgoing, this._outgoing.length);
    this._outgoing = this._outgoing.slice(written);
    this._flushOutgoing();
  },

  




  ready: function DT_ready() {
    let pump = Cc["@mozilla.org/network/input-stream-pump;1"]
      .createInstance(Ci.nsIInputStreamPump);
    pump.init(this._input, -1, -1, 0, 0, false);
    pump.asyncRead(this, null);
  },

  
  onStartRequest: function DT_onStartRequest(aRequest, aContext) {},

  onStopRequest: function DT_onStopRequest(aRequest, aContext, aStatus) {
    this.close();
    this.hooks.onClosed(aStatus);
  },

  onDataAvailable: function DT_onDataAvailable(aRequest, aContext,
                                                aStream, aOffset, aCount) {
    try {
      this._incoming += NetUtil.readInputStreamToString(aStream,
                                                        aStream.available());
      while (this._processIncoming()) {};
    } catch(e) {
      let msg = "Unexpected error reading from debugging connection: " + e + " - " + e.stack;
      if (Cu.reportError) {
        Cu.reportError(msg);
      }
      dump(msg + "\n");
      this.close();
      return;
    }
  },

  






  _processIncoming: function DT__processIncoming() {
    
    let sep = this._incoming.indexOf(':');
    if (sep < 0) {
      return false;
    }

    let count = parseInt(this._incoming.substring(0, sep));
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

    try {
      dumpn("Got: " + packet);
      let self = this;
      Services.tm.currentThread.dispatch({run: function() {
        self.hooks.onPacket(parsed);
      }}, 0);
    } catch(e) {
      let msg = "Error handling incoming packet: " + e + " - " + e.stack;
      if (Cu.reportError) {
        Cu.reportError(msg);
      }
      dump(msg + "\n");
      dumpn("Packet was: " + packet);
    }

    return true;
  }
}













function LocalDebuggerTransport(aOther)
{
  this.other = aOther;
  this.hooks = null;
}

LocalDebuggerTransport.prototype = {
  



  send: function LDT_send(aPacket) {
    try {
      
      if (wantLogging) {
        dumpn("Got: " + JSON.stringify(aPacket, null, 2));
      }
      this._deepFreeze(aPacket);
      let other = this.other;
      Services.tm.currentThread.dispatch({run: function() {
        other.hooks.onPacket(aPacket);
      }}, 0);
    } catch(e) {
      let msg = "Error handling incoming packet: " + e + " - " + e.stack;
      if (Cu.reportError) {
        Cu.reportError(msg);
      }
      dump(msg + "\n");
      dumpn("Packet was: " + aPacket + "\n");
    }
  },

  


  close: function LDT_close() {
    if (this.other) {
      
      
      let other = this.other;
      delete this.other;
      other.close();
    }
    this.hooks.onClosed();
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

}

