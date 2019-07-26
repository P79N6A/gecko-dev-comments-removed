



"use strict";





















const { Cc, Ci, Cu } = require("chrome");
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const { dumpn, dumpv } = DevToolsUtils;
const StreamUtils = require("devtools/toolkit/transport/stream-utils");

DevToolsUtils.defineLazyGetter(this, "unicodeConverter", () => {
  const unicodeConverter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                           .createInstance(Ci.nsIScriptableUnicodeConverter);
  unicodeConverter.charset = "UTF-8";
  return unicodeConverter;
});




const PACKET_LENGTH_MAX = Math.pow(2, 40);




function Packet(transport) {
  this._transport = transport;
  this._length = 0;
}












Packet.fromHeader = function(header, transport) {
  return JSONPacket.fromHeader(header, transport) ||
         BulkPacket.fromHeader(header, transport);
};

Packet.prototype = {

  get length() {
    return this._length;
  },

  set length(length) {
    if (length > PACKET_LENGTH_MAX) {
      throw Error("Packet length " + length + " exceeds the max length of " +
                  PACKET_LENGTH_MAX);
    }
    this._length = length;
  },

  destroy: function() {
    this._transport = null;
  }

};

exports.Packet = Packet;










function JSONPacket(transport) {
  Packet.call(this, transport);
  this._data = "";
  this._done = false;
}











JSONPacket.fromHeader = function(header, transport) {
  let match = this.HEADER_PATTERN.exec(header);

  if (!match) {
    return null;
  }

  dumpv("Header matches JSON packet");
  let packet = new JSONPacket(transport);
  packet.length = +match[1];
  return packet;
};

JSONPacket.HEADER_PATTERN = /^(\d+):$/;

JSONPacket.prototype = Object.create(Packet.prototype);

Object.defineProperty(JSONPacket.prototype, "object", {
  


  get: function() { return this._object; },

  


  set: function(object) {
    this._object = object;
    let data = JSON.stringify(object);
    this._data = unicodeConverter.ConvertFromUnicode(data);
    this.length = this._data.length;
  }
});

JSONPacket.prototype.read = function(stream, scriptableStream) {
  dumpv("Reading JSON packet");

  
  this._readData(stream, scriptableStream);

  if (!this.done) {
    
    return;
  }

  let json = this._data;
  try {
    json = unicodeConverter.ConvertToUnicode(json);
    this._object = JSON.parse(json);
  } catch(e) {
    let msg = "Error parsing incoming packet: " + json + " (" + e +
              " - " + e.stack + ")";
    if (Cu.reportError) {
      Cu.reportError(msg);
    }
    dumpn(msg);
    return;
  }

  this._transport._onJSONObjectReady(this._object);
}

JSONPacket.prototype._readData = function(stream, scriptableStream) {
  if (dumpv.wantVerbose) {
    dumpv("Reading JSON data: _l: " + this.length + " dL: " +
          this._data.length + " sA: " + stream.available());
  }
  let bytesToRead = Math.min(this.length - this._data.length,
                             stream.available());
  this._data += scriptableStream.readBytes(bytesToRead);
  this._done = this._data.length === this.length;
}

JSONPacket.prototype.write = function(stream) {
  dumpv("Writing JSON packet");

  if (this._outgoing === undefined) {
    
    this._outgoing = this.length + ":" + this._data;
  }

  let written = stream.write(this._outgoing, this._outgoing.length);
  this._outgoing = this._outgoing.slice(written);
  this._done = !this._outgoing.length;
}

Object.defineProperty(JSONPacket.prototype, "done", {
  get: function() { return this._done; }
});

JSONPacket.prototype.toString = function() {
  return JSON.stringify(this._object, null, 2);
}

exports.JSONPacket = JSONPacket;
















function BulkPacket(transport) {
  Packet.call(this, transport);
  this._done = false;
  this._readyForWriting = promise.defer();
}











BulkPacket.fromHeader = function(header, transport) {
  let match = this.HEADER_PATTERN.exec(header);

  if (!match) {
    return null;
  }

  dumpv("Header matches bulk packet");
  let packet = new BulkPacket(transport);
  packet.header = {
    actor: match[1],
    type: match[2],
    length: +match[3]
  };
  return packet;
};

BulkPacket.HEADER_PATTERN = /^bulk ([^: ]+) ([^: ]+) (\d+):$/;

BulkPacket.prototype = Object.create(Packet.prototype);

BulkPacket.prototype.read = function(stream) {
  dumpv("Reading bulk packet, handing off input stream");

  
  this._transport.pauseIncoming();

  let deferred = promise.defer();

  this._transport._onBulkReadReady({
    actor: this.actor,
    type: this.type,
    length: this.length,
    copyTo: (output) => {
      dumpv("CT length: " + this.length);
      deferred.resolve(StreamUtils.copyStream(stream, output, this.length));
      return deferred.promise;
    },
    stream: stream,
    done: deferred
  });

  
  deferred.promise.then(() => {
    dumpv("onReadDone called, ending bulk mode");
    this._done = true;
    this._transport.resumeIncoming();
  }, this._transport.close);

  
  this.read = () => {
    throw new Error("Tried to read() a BulkPacket's stream multiple times.");
  };
}

BulkPacket.prototype.write = function(stream) {
  dumpv("Writing bulk packet");

  if (this._outgoingHeader === undefined) {
    dumpv("Serializing bulk packet header");
    
    this._outgoingHeader = "bulk " + this.actor + " " + this.type + " " +
                           this.length + ":";
  }

  
  if (this._outgoingHeader.length) {
    dumpv("Writing bulk packet header");
    let written = stream.write(this._outgoingHeader,
                               this._outgoingHeader.length);
    this._outgoingHeader = this._outgoingHeader.slice(written);
    return;
  }

  dumpv("Handing off output stream");

  
  this._transport.pauseOutgoing();

  let deferred = promise.defer();

  this._readyForWriting.resolve({
    copyFrom: (input) => {
      dumpv("CF length: " + this.length);
      deferred.resolve(StreamUtils.copyStream(input, stream, this.length));
      return deferred.promise;
    },
    stream: stream,
    done: deferred
  });

  
  deferred.promise.then(() => {
    dumpv("onWriteDone called, ending bulk mode");
    this._done = true;
    this._transport.resumeOutgoing();
  }, this._transport.close);

  
  this.write = () => {
    throw new Error("Tried to write() a BulkPacket's stream multiple times.");
  };
}

Object.defineProperty(BulkPacket.prototype, "streamReadyForWriting", {
  get: function() {
    return this._readyForWriting.promise;
  }
});

Object.defineProperty(BulkPacket.prototype, "header", {
  get: function() {
    return {
      actor: this.actor,
      type: this.type,
      length: this.length
    };
  },

  set: function(header) {
    this.actor = header.actor;
    this.type = header.type;
    this.length = header.length;
  },
});

Object.defineProperty(BulkPacket.prototype, "done", {
  get: function() { return this._done; },
});


BulkPacket.prototype.toString = function() {
  return "Bulk: " + JSON.stringify(this.header, null, 2);
}

exports.BulkPacket = BulkPacket;









function RawPacket(transport, data) {
  Packet.call(this, transport);
  this._data = data;
  this.length = data.length;
  this._done = false;
}

RawPacket.prototype = Object.create(Packet.prototype);

RawPacket.prototype.read = function(stream) {
  
  throw Error("Not implmented.");
}

RawPacket.prototype.write = function(stream) {
  let written = stream.write(this._data, this._data.length);
  this._data = this._data.slice(written);
  this._done = !this._data.length;
}

Object.defineProperty(RawPacket.prototype, "done", {
  get: function() { return this._done; }
});

exports.RawPacket = RawPacket;
