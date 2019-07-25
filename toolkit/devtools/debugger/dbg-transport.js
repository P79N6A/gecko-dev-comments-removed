






































"use strict";
Cu.import("resource://gre/modules/NetUtil.jsm");















function DebuggerTransport(aInput, aOutput)
{
  this._input = aInput;
  this._output = aOutput;
  this._outgoing = "";
  this._incoming = "";
}

DebuggerTransport.prototype = {
  _hooks: null,
  get hooks() { return this._hooks; },
  set hooks(aHooks) { this._hooks = aHooks; },

  


  send: function DT_send(aPacket) {
    
    let data = JSON.stringify(aPacket, null, 2);
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
      dumpn("Unexpected error reading from debugging connection: " + e + " - " + e.stack);
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
      var parsed = JSON.parse(packet);
    } catch(e) {
      dumpn("Error parsing incoming packet: " + packet + " (" + e + " - " + e.stack + ")");
      return true;
    }

    try {
      dumpn("Got: " + packet);
      let thr = Cc["@mozilla.org/thread-manager;1"].getService().currentThread;
      let self = this;
      thr.dispatch({run: function() {
        self.hooks.onPacket(parsed);
      }}, 0);
    } catch(e) {
      dumpn("Error handling incoming packet: " + e + " - " + e.stack);
      dumpn("Packet was: " + packet);
    }

    return true;
  }
}
